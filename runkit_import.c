/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group, (c) 2008-2012 Dmitry Zenovich |
  +----------------------------------------------------------------------+
  | This source file is subject to the new BSD license,                  |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.opensource.org/licenses/BSD-3-Clause                      |
  | If you did not receive a copy of the license and are unable to       |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  | Modified by Dmitry Zenovich <dzenovich@gmail.com>                    |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_runkit.h"

#ifndef IS_CONSTANT_AST
#define IS_CONSTANT_AST IS_CONSTANT_ARRAY
#endif

#if PHP_VERSION_ID < 50600
#define _CONSTANT_INDEX(a) (void*) a
#else
#define _CONSTANT_INDEX(a) a
#endif

#ifdef PHP_RUNKIT_MANIPULATION
/* {{{ php_runkit_import_functions
 */
static int php_runkit_import_functions(HashTable *function_table, long flags
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
                                       , zend_bool *clear_cache
#endif
                                       TSRMLS_DC)
{
	HashPosition pos;
	int i, func_count = zend_hash_num_elements(function_table);

	zend_hash_internal_pointer_reset_ex(function_table, &pos);
	for(i = 0; i < func_count; i++) {
		zend_function *fe = NULL, *orig_fe;
		char *key;
		const char *new_key;
		uint key_len, new_key_len;
		int type;
		ulong idx;
		zend_bool add_function = 1;
		zend_bool exists = 0;

		zend_hash_get_current_data_ex(function_table, (void*)&fe, &pos);

		new_key = fe->common.function_name;
		new_key_len = strlen(new_key) + 1;

		if (((type = zend_hash_get_current_key_ex(function_table, &key, &key_len, &idx, 0, &pos)) != HASH_KEY_NON_EXISTANT) &&
			fe && fe->type == ZEND_USER_FUNCTION) {

			if (type == HASH_KEY_IS_STRING) {
				new_key = key;
				new_key_len = key_len;
				exists = (zend_hash_find(EG(function_table), (char *) new_key, new_key_len, (void *) &orig_fe) == SUCCESS);
			} else {
				exists = (zend_hash_index_find(EG(function_table), idx, (void *) &orig_fe) == SUCCESS);
			}

			if (exists) {
				php_runkit_remove_function_from_reflection_objects(orig_fe TSRMLS_CC);
				if (flags & PHP_RUNKIT_IMPORT_OVERRIDE) {
					if (type == HASH_KEY_IS_STRING) {
						if (zend_hash_del(EG(function_table), (char *) new_key, new_key_len) == FAILURE) {
							php_error_docref(NULL TSRMLS_CC, E_WARNING, "Inconsistency cleaning up import environment");
							return FAILURE;
						}
					} else {
						if (zend_hash_index_del(EG(function_table), idx) == FAILURE) {
							php_error_docref(NULL TSRMLS_CC, E_WARNING, "Inconsistency cleaning up import environment");
							return FAILURE;
						}
					}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
					*clear_cache = 1;
#endif
				} else {
					add_function = 0;
				}
			}
		}

		if (add_function) {
			if (zend_hash_add(EG(function_table), (char *) new_key, new_key_len, fe, sizeof(zend_function), NULL) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failure importing %s()", fe->common.function_name);
				PHP_RUNKIT_DESTROY_FUNCTION(fe);
				return FAILURE;
			} else {
				PHP_RUNKIT_FUNCTION_ADD_REF(fe);
			}
		}
		zend_hash_move_forward_ex(function_table, &pos);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_import_class_methods
 */
static int php_runkit_import_class_methods(zend_class_entry *dce, zend_class_entry *ce, int override
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
                                           , zend_bool *clear_cache
#endif
                                           TSRMLS_DC)
{
	HashPosition pos;
	zend_function *fe;
	char *fn;
	int fn_maxlen;

	/* Generic strtolower buffer for function names */
	fn_maxlen = 64;
	fn = emalloc(fn_maxlen);

	zend_hash_internal_pointer_reset_ex(&ce->function_table, &pos);
	while (zend_hash_get_current_data_ex(&ce->function_table, (void*)&fe, &pos) == SUCCESS) {
		zend_function *dfe = NULL;
		int fn_len = strlen(fe->common.function_name);
		zend_class_entry *fe_scope;

		if (fn_len > fn_maxlen - 1) {
			fn_maxlen = fn_len + 33;
			fn = erealloc(fn, fn_maxlen);
		}
		memcpy(fn, fe->common.function_name, fn_len + 1);
		php_strtolower(fn, fn_len);

		fe_scope = php_runkit_locate_scope(ce, fe, fn, fn_len);

		if (fe_scope != ce) {
			/* This is an inhereted function, let's skip it */
			zend_hash_move_forward_ex(&ce->function_table, &pos);
			continue;
		}

		dfe = NULL;
		if (zend_hash_find(&dce->function_table, fn, fn_len + 1, (void*)&dfe) == SUCCESS) {
			zend_class_entry *scope;
			if (!override) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s::%s() already exists, not importing", dce->name, fe->common.function_name);
				zend_hash_move_forward_ex(&ce->function_table, &pos);
				continue;
			}

			scope = php_runkit_locate_scope(dce, dfe, fn, fn_len);

			if (php_runkit_check_call_stack(&dfe->op_array TSRMLS_CC) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot override active method %s::%s(). Skipping.", dce->name, fe->common.function_name);
				zend_hash_move_forward_ex(&ce->function_table, &pos);
				continue;
			}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			*clear_cache = 1;
#endif

			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_clean_children_methods, 5, scope, dce, fn, fn_len, dfe);
			php_runkit_remove_function_from_reflection_objects(dfe TSRMLS_CC);
			if (zend_hash_del(&dce->function_table, fn, fn_len + 1) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error removing old method in destination class %s::%s", dce->name, fe->common.function_name);
				zend_hash_move_forward_ex(&ce->function_table, &pos);
				continue;
			}
		}

		fe->common.scope = dce;
		if (zend_hash_add(&dce->function_table, fn, fn_len + 1, fe, sizeof(zend_function), NULL) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failure importing %s::%s()", ce->name, fe->common.function_name);
			zend_hash_move_forward_ex(&ce->function_table, &pos);
			continue;
		} else {
			PHP_RUNKIT_FUNCTION_ADD_REF(fe);
		}

		if (zend_hash_find(&dce->function_table, fn, fn_len + 1, (void*)&fe) != SUCCESS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot get newly created method %s::%s()", ce->name, fn);
			zend_hash_move_forward_ex(&ce->function_table, &pos);
			continue;
		}
		PHP_RUNKIT_ADD_MAGIC_METHOD(dce, fn, fn_len, fe, dfe);
		zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_methods, 7,
		                               dce, dce, fe, fn, fn_len, dfe, 0);
		zend_hash_move_forward_ex(&ce->function_table, &pos);
	}

	efree(fn);

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_import_class_consts
 */
static int php_runkit_import_class_consts(zend_class_entry *dce, zend_class_entry *ce, int override TSRMLS_DC)
{
	HashPosition pos;
	char *key;
	uint key_len;
	ulong idx;
	zval **c;

	zend_hash_internal_pointer_reset_ex(&ce->constants_table, &pos);
	while (zend_hash_get_current_data_ex(&ce->constants_table, (void*)&c, &pos) == SUCCESS && c && *c) {
		long action = HASH_ADD;

		if (zend_hash_get_current_key_ex(&ce->constants_table, &key, &key_len, &idx, 0, &pos) == HASH_KEY_IS_STRING) {
			if (zend_hash_exists(&dce->constants_table, key, key_len)) {
				if (override) {
					action = HASH_UPDATE;
				} else {
					php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s::%s already exists, not importing", dce->name, key);
					goto import_const_skip;
				}
			}
			if (
				Z_TYPE_PP(c) == IS_CONSTANT_AST
#if RUNKIT_ABOVE53
				|| (Z_TYPE_PP(c) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#endif
			) {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2 || PHP_MAJOR_VERSION > 5
				zval_update_constant_ex(c, _CONSTANT_INDEX(1), dce TSRMLS_CC);
#else
				zval_update_constant(c, dce TSRMLS_CC);
#endif
			}
			Z_ADDREF_P(*c);
			if (zend_hash_add_or_update(&dce->constants_table, key, key_len, (void*)c, sizeof(zval*), NULL, action) == FAILURE) {
				zval_ptr_dtor(c);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to import %s::%s", dce->name, key);
			}

			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_consts, 4, dce, c, key, key_len - 1);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Constant has invalid key name");
		}
import_const_skip:
		zend_hash_move_forward_ex(&ce->constants_table, &pos);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_import_class_static_props
 */
static int php_runkit_import_class_static_props(zend_class_entry *dce, zend_class_entry *ce, int override, int remove_from_objects TSRMLS_DC)
{
	HashPosition pos;
	char *key;
	uint key_len;
	ulong idx;
	zend_property_info *property_info_ptr = NULL;

	zend_hash_internal_pointer_reset_ex(&ce->properties_info, &pos);
	while (zend_hash_get_current_data_ex(&ce->properties_info, (void*) &property_info_ptr, &pos) == SUCCESS && property_info_ptr) {
		if (zend_hash_get_current_key_ex(&ce->properties_info, &key, &key_len, &idx, 0, &pos) == HASH_KEY_IS_STRING) {
			zval **pp;
			zend_property_info *ex_property_info_ptr;
			if (!(property_info_ptr->flags & ZEND_ACC_STATIC)) {
				goto import_st_prop_skip;
			}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			pp = &CE_STATIC_MEMBERS(ce)[property_info_ptr->offset];
#else
			zend_hash_quick_find(CE_STATIC_MEMBERS(ce), property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h, (void*) &pp);
			if (
				Z_TYPE_PP(pp) == IS_CONSTANT_AST
#if RUNKIT_ABOVE53
				|| (Z_TYPE_PP(pp) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#endif // RUNKIT_ABOVE53
			) {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2 || PHP_MAJOR_VERSION > 5
				zval_update_constant_ex(pp, (void*) 1, dce TSRMLS_CC);
#else
				zval_update_constant(pp, dce TSRMLS_CC);
#endif
			}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			if (zend_hash_find(&dce->properties_info, key, key_len, (void*) &ex_property_info_ptr) == SUCCESS && ex_property_info_ptr) {
				if (override) {
					if (php_runkit_def_prop_remove_int(dce, key, key_len - 1, NULL, 0, 0, NULL TSRMLS_CC) != SUCCESS) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to import %s::$%s (cannot remove old member)", dce->name, key);
						goto import_st_prop_skip;
					}
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2 || PHP_MAJOR_VERSION > 5					
					zval_update_constant_ex(pp, _CONSTANT_INDEX(1), dce TSRMLS_CC);
#else
					zval_update_constant(pp, dce TSRMLS_CC);
#endif
					if (php_runkit_def_prop_add_int(dce, key, key_len - 1, *pp, property_info_ptr->flags,
					                                property_info_ptr->doc_comment,
					                                property_info_ptr->doc_comment_len, dce,
					                                override, remove_from_objects TSRMLS_CC) != SUCCESS) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to import %s::$%s (cannot add new member)", dce->name, key);
						goto import_st_prop_skip;
					}
					goto import_st_prop_skip;
				} else {
					php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s::$%s already exists, not importing", dce->name, key);
					goto import_st_prop_skip;
				}
			} else {
				if (php_runkit_def_prop_add_int(dce, key, key_len - 1, *pp, property_info_ptr->flags,
				                                property_info_ptr->doc_comment,
				                                property_info_ptr->doc_comment_len, dce,
				                                override, remove_from_objects TSRMLS_CC) != SUCCESS) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to import %s::$%s (cannot add new member)", dce->name, key);
					goto import_st_prop_skip;
				}
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Property has invalid key name");
		}
import_st_prop_skip:
		zend_hash_move_forward_ex(&ce->properties_info, &pos);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_import_class_props
 */
static int php_runkit_import_class_props(zend_class_entry *dce, zend_class_entry *ce, int override, int remove_from_objects TSRMLS_DC)
{
	HashPosition pos;
	char *key;
	uint key_len;
	zval **p;
	ulong idx;

	zend_property_info *property_info_ptr;

	zend_hash_internal_pointer_reset_ex(&ce->properties_info, &pos);
	while (zend_hash_get_current_data_ex(&ce->properties_info, (void*) &property_info_ptr, &pos) == SUCCESS &&
	       property_info_ptr) {
		if (zend_hash_get_current_key_ex(&ce->properties_info, &key, &key_len, &idx, 0, &pos) == HASH_KEY_IS_STRING) {
			if (property_info_ptr->flags & ZEND_ACC_STATIC) {
				goto import_st54_prop_skip;
			}
			if (zend_hash_exists(&dce->properties_info, key, key_len)) {
				if (!override) {
					php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s%s%s already exists, not importing",
					                 dce->name, (property_info_ptr->flags & ZEND_ACC_STATIC) ? "::$" : "->", key);
					goto import_st54_prop_skip;
				}
			}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4)
			p = &ce->default_properties_table[property_info_ptr->offset];
#else
			if (zend_hash_quick_find(&ce->default_properties, property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h, (void*) &p) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING,
				                 "Cannot import broken default property %s->%s", ce->name, key);
				goto import_st54_prop_skip;
			}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			if (
				Z_TYPE_PP(p) == IS_CONSTANT_AST
#if RUNKIT_ABOVE53
				|| (Z_TYPE_PP(p) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#endif // RUNKIT_ABOVE53
			) {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2 || PHP_MAJOR_VERSION > 5
				zval_update_constant_ex(p, _CONSTANT_INDEX(1), dce TSRMLS_CC);
#else
				zval_update_constant(p, dce TSRMLS_CC);
#endif
			}

			php_runkit_def_prop_add_int(dce, key, key_len - 1, *p,
			                            property_info_ptr->flags, property_info_ptr->doc_comment,
			                            property_info_ptr->doc_comment_len, dce, override, remove_from_objects TSRMLS_CC);
		}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4)
		else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Property has invalid key name");
		}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4) || (PHP_MAJOR_VERSION < 5)
import_st54_prop_skip:
		zend_hash_move_forward_ex(&ce->properties_info, &pos);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_import_classes
 */
static int php_runkit_import_classes(HashTable *class_table, long flags
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
                                     , zend_bool *clear_cache
#endif
                                     TSRMLS_DC)
{
	HashPosition pos;
	int i, class_count;

	class_count = zend_hash_num_elements(class_table);
	zend_hash_internal_pointer_reset_ex(class_table, &pos);
	for(i = 0; i < class_count; i++) {
		zend_class_entry *ce = NULL;
		char *key;
		uint key_len;
		int type;
		ulong idx;
		char *lcname;
		zend_class_entry *dce;

		zend_hash_get_current_data_ex(class_table, (void*)&ce, &pos);
		if (ce) {
			ce = *((zend_class_entry**)ce);
		}

		if (!ce) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Non-class in class table!");
			return FAILURE;
		}

		if (ce->type != ZEND_USER_CLASS) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot import internal class!");
			return FAILURE;
		}

		type = zend_hash_get_current_key_ex(class_table, &key, &key_len, &idx, 0, &pos);

		if (key[0] != 0) {
			lcname = estrndup(ce->name, ce->name_length);
			if (lcname == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
				return FAILURE;
			}
			php_strtolower(lcname, ce->name_length);

			if (!zend_hash_exists(EG(class_table), lcname, ce->name_length + 1)) {
				php_runkit_class_copy(ce, ce->name, ce->name_length TSRMLS_CC);
			}
			efree(lcname);

			if (php_runkit_fetch_class(ce->name, ce->name_length, &dce TSRMLS_CC) == FAILURE) {
				/* Oddly non-existant target class or error retreiving it... Or it's an internal class... */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot redeclare class %s", ce->name);
				continue;
			}

			if (flags & PHP_RUNKIT_IMPORT_CLASS_CONSTS) {
				php_runkit_import_class_consts(dce, ce, (flags & PHP_RUNKIT_IMPORT_OVERRIDE) TSRMLS_CC);
			}
			if (flags & PHP_RUNKIT_IMPORT_CLASS_STATIC_PROPS) {
				php_runkit_import_class_static_props(dce, ce, (flags & PHP_RUNKIT_IMPORT_OVERRIDE) != 0,
				                                     (flags & PHP_RUNKIT_OVERRIDE_OBJECTS) != 0
				                                     TSRMLS_CC);
			}

			if (flags & PHP_RUNKIT_IMPORT_CLASS_PROPS) {
				php_runkit_import_class_props(dce, ce, (flags & PHP_RUNKIT_IMPORT_OVERRIDE) != 0,
				                              (flags & PHP_RUNKIT_OVERRIDE_OBJECTS) != 0
				                              TSRMLS_CC);
			}

			if (flags & PHP_RUNKIT_IMPORT_CLASS_METHODS) {
				php_runkit_import_class_methods(dce, ce, (flags & PHP_RUNKIT_IMPORT_OVERRIDE)
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
				                                , clear_cache
#endif
				                                TSRMLS_CC);
			}
		}
		zend_hash_move_forward_ex(class_table, &pos);

		if (type == HASH_KEY_IS_STRING) {
			if (zend_hash_del(class_table, key, key_len) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove temporary version of class %s", ce->name);
				continue;
			}
		} else {
			if (zend_hash_index_del(class_table, idx) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove temporary version of class %s", ce->name);
				continue;
			}
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_compile_filename
 * Duplicate of Zend's compile_filename which explicitly calls the internal compile_file() implementation.
 *
 * This is only used when an accelerator has replaced zend_compile_file() with an alternate method
 * which has been known to cause issues with overly-optimistic early binding.
 *
 * It would be clener to temporarily set zend_compile_file back to compile_file, but that wouldn't be
 * particularly thread-safe so.... */
static zend_op_array *php_runkit_compile_filename(int type, zval *filename TSRMLS_DC)
{
	zend_file_handle file_handle;
	zval tmp;
	zend_op_array *retval;
	char *opened_path = NULL;

	if (filename->type != IS_STRING) {
		tmp = *filename;
		zval_copy_ctor(&tmp);
		convert_to_string(&tmp);
		filename = &tmp;
	}
	file_handle.filename = filename->value.str.val;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = NULL;
#if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 0)
	file_handle.handle.fp = NULL;
#endif

	/* Use builtin compiler only -- bypass accelerators and whatnot */
	retval = compile_file(&file_handle, type TSRMLS_CC);
	if (retval && file_handle.handle.stream.handle) {
		int dummy = 1;

		if (!file_handle.opened_path) {
			file_handle.opened_path = opened_path = estrndup(filename->value.str.val, filename->value.str.len);
		}

		zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void*)&dummy, sizeof(int), NULL);

		if (opened_path) {
			efree(opened_path);
		}
	}
	zend_destroy_file_handle(&file_handle TSRMLS_CC);

	if (filename==&tmp) {
		zval_dtor(&tmp);
	}
	return retval;
}
/* }}} */

static HashTable *current_class_table, *tmp_class_table, *tmp_eg_class_table, *current_eg_class_table, *current_function_table, *tmp_function_table;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION > 5)
static zend_uint php_runkit_old_compiler_options;
#endif
void (*php_runkit_old_error_cb)(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args);

/* {{{ php_runkit_error_cb */
void php_runkit_error_cb(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args) {
	TSRMLS_FETCH();

	zend_error_cb = php_runkit_old_error_cb;
	CG(class_table) = current_class_table;
	EG(class_table) = current_eg_class_table;
	CG(function_table) = current_function_table;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION > 5)
	CG(compiler_options) = php_runkit_old_compiler_options;
#endif

	php_runkit_old_error_cb(type, error_filename, error_lineno, format, args);
}
/* }}} */

/* {{{ array runkit_import(string filename[, long flags])
	Import functions and class definitions from a file
	Similar to include(), but doesn't execute root op_array, and allows pre-existing functions/methods to be overridden */
PHP_FUNCTION(runkit_import)
{
	zend_op_array *new_op_array;
	zval *filename;
	long flags = PHP_RUNKIT_IMPORT_CLASS_METHODS;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	zend_bool clear_cache = 0;
#endif

	zend_op_array *(*local_compile_filename)(int type, zval *filename TSRMLS_DC) = compile_filename;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", &filename, &flags) == FAILURE) {
		RETURN_FALSE;
	}
	convert_to_string(filename);

	if (compile_file != zend_compile_file) {
		/* An accellerator or other dark force is at work
		 * Use the wrapper method to force the builtin compiler
		 * to be used */
		local_compile_filename = php_runkit_compile_filename;
	}

	tmp_class_table = (HashTable *) emalloc(sizeof(HashTable));
	zend_hash_init_ex(tmp_class_table, 10, NULL, ZEND_CLASS_DTOR, 0, 0);
	tmp_eg_class_table = (HashTable *) emalloc(sizeof(HashTable));
	zend_hash_init_ex(tmp_eg_class_table, 10, NULL, ZEND_CLASS_DTOR, 0, 0);
	tmp_function_table = (HashTable *) emalloc(sizeof(HashTable));
	zend_hash_init_ex(tmp_function_table, 100, NULL, ZEND_FUNCTION_DTOR, 0, 0);

	current_class_table = CG(class_table);
	CG(class_table) = tmp_class_table;
	current_eg_class_table = EG(class_table);
	EG(class_table) = tmp_eg_class_table;
	current_function_table = CG(function_table);
	CG(function_table) = tmp_function_table;
	php_runkit_old_error_cb = zend_error_cb;
	zend_error_cb = php_runkit_error_cb;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION > 5)
	php_runkit_old_compiler_options = CG(compiler_options);
	CG(compiler_options) |= ZEND_COMPILE_DELAYED_BINDING;
#endif

	new_op_array = local_compile_filename(ZEND_INCLUDE, filename TSRMLS_CC);

	zend_error_cb = php_runkit_old_error_cb;
	CG(class_table) = current_class_table;
	EG(class_table) = current_eg_class_table;
	CG(function_table) = current_function_table;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION > 5)
	CG(compiler_options) = php_runkit_old_compiler_options;
#endif

	if (!new_op_array) {
		zend_hash_destroy(tmp_class_table);
		efree(tmp_class_table);
		zend_hash_destroy(tmp_eg_class_table);
		efree(tmp_eg_class_table);
		zend_hash_destroy(tmp_function_table);
		efree(tmp_function_table);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Import Failure");
		RETURN_FALSE;
	}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION > 5)
	if (new_op_array->early_binding != -1) {
		zend_bool orig_in_compilation = CG(in_compilation);
		zend_uint opline_num = new_op_array->early_binding;
		zend_class_entry *pce;

		CG(in_compilation) = 1;
		while (opline_num != -1) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			if (php_runkit_fetch_class_int(Z_STRVAL_P(new_op_array->opcodes[opline_num-1].op2.zv), Z_STRLEN_P(new_op_array->opcodes[opline_num-1].op2.zv), &pce TSRMLS_CC) == SUCCESS) {
				do_bind_inherited_class(new_op_array, &new_op_array->opcodes[opline_num], tmp_class_table, pce, 0 TSRMLS_CC);
			}
			opline_num = new_op_array->opcodes[opline_num].result.opline_num;
#else
			if (php_runkit_fetch_class_int(Z_STRVAL(new_op_array->opcodes[opline_num-1].op2.u.constant), Z_STRLEN(new_op_array->opcodes[opline_num-1].op2.u.constant), &pce TSRMLS_CC) == SUCCESS) {
				do_bind_inherited_class(&new_op_array->opcodes[opline_num], tmp_class_table, pce, 1 TSRMLS_CC);
			}
			opline_num = new_op_array->opcodes[opline_num].result.u.opline_num;
#endif
		}
		CG(in_compilation) = orig_in_compilation;
	}
#else
	{
		zend_bool orig_in_compilation = CG(in_compilation);
		zend_op *opline = &new_op_array->opcodes[new_op_array->last-1];
		zend_class_entry *pce;

		CG(in_compilation) = 1;

		while (opline->opcode == ZEND_TICKS && opline > new_op_array->opcodes) {
			opline--;
		}

		for (;opline >= new_op_array->opcodes; --opline) {
			if (opline->opcode == ZEND_DECLARE_INHERITED_CLASS) {
				if (php_runkit_fetch_class_int(Z_STRVAL((opline-1)->op2.u.constant), Z_STRLEN((opline-1)->op2.u.constant), &pce TSRMLS_CC) == SUCCESS) {
					do_bind_inherited_class(opline, tmp_class_table, pce, 1 TSRMLS_CC);
				}
			}
		}
		CG(in_compilation) = orig_in_compilation;
	}
#endif

	/* We never really needed the main loop opcodes to begin with */
	destroy_op_array(new_op_array TSRMLS_CC);
	efree(new_op_array);

	if (flags & PHP_RUNKIT_IMPORT_FUNCTIONS) {
		php_runkit_import_functions(tmp_function_table, flags
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		                            , &clear_cache
#endif
		                            TSRMLS_CC);
	}

	if (flags & PHP_RUNKIT_IMPORT_CLASSES) {
		php_runkit_import_classes(tmp_class_table, flags
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		                          , &clear_cache
#endif
		                          TSRMLS_CC);
	}

	zend_hash_destroy(tmp_class_table);
	efree(tmp_class_table);
	zend_hash_destroy(tmp_eg_class_table);
	efree(tmp_eg_class_table);
	zend_hash_destroy(tmp_function_table);
	efree(tmp_function_table);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	if (clear_cache) {
		php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
	}
#endif

	RETURN_TRUE;
}
/* }}} */
#endif /* PHP_RUNKIT_MANIPULATION */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
