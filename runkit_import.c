/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_runkit.h"

#ifdef PHP_RUNKIT_MANIPULATION
/* {{{ php_runkit_import_functions
 */
static int php_runkit_import_functions(int original_num_functions TSRMLS_DC)
{
	HashPosition pos;
	zend_function **fe_array;
	int i, func = 0, func_count = (zend_hash_num_elements(EG(function_table)) - original_num_functions);

	fe_array = ecalloc(sizeof(zend_function*) * func_count, 0);

	zend_hash_internal_pointer_end_ex(EG(function_table), &pos);
	for(i = zend_hash_num_elements(EG(function_table)); i > original_num_functions; i--) {
		zend_function *fe = NULL;
		char *key;
		int key_len, type;
		long idx;

		zend_hash_get_current_data_ex(EG(function_table), (void**)&fe, &pos);
		PHP_RUNKIT_FUNCTION_ADD_REF(fe);
		fe_array[func++] = fe;

		if (((type = zend_hash_get_current_key_ex(EG(function_table), &key, &key_len, &idx, 0, &pos)) != HASH_KEY_NON_EXISTANT) && 
			fe && fe->type == ZEND_USER_FUNCTION) {

			if (type == HASH_KEY_IS_STRING) {
				if (zend_hash_del(EG(function_table), key, key_len) == FAILURE) {
					goto err_exit;
				}
			} else {
				if (zend_hash_index_del(EG(function_table), idx) == FAILURE) {
					goto err_exit;
				}
			}
		} else {
			goto err_exit;
		}
		zend_hash_move_backwards_ex(EG(function_table), &pos);
	}

	while (func >= 0) {
		zend_function *fe = fe_array[func];

		if (fe) {
			char *lcase = estrdup(fe->common.function_name);
			int lcase_len = strlen(lcase);

			php_strtolower(lcase, lcase_len);
			if (zend_hash_add(EG(function_table), lcase, lcase_len + 1, fe, sizeof(zend_function), NULL) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failure importing %s()", fe->common.function_name); 
#ifdef ZEND_ENGINE_2
				destroy_zend_function(fe TSRMLS_CC);
#else
				destroy_zend_function(fe);
#endif
				efree(lcase);
			}
		}
		func--;
	}

	return SUCCESS;
err_exit:
	while (func >= 0) {
		if (fe_array[func]) {
#ifdef ZEND_ENGINE_2
			destroy_zend_function(fe_array[func] TSRMLS_CC);
#else
			destroy_zend_function(fe_array[func]);
#endif
		}
		func--;
	}
	efree(fe_array);
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Inconsistency cleaning up import environment");
	return FAILURE;
}
/* }}} */

/* {{{ php_runkit_import_class_methods
 */
static int php_runkit_import_class_methods(zend_class_entry *dce, zend_class_entry *ce, int override TSRMLS_DC)
{
	HashPosition pos;
	zend_function *fe;
	char *fn;
	int fn_maxlen;

	/* Generic strtolower buffer for function names */
	fn_maxlen = 64;
	fn = emalloc(fn_maxlen);

	zend_hash_internal_pointer_reset_ex(&ce->function_table, &pos);
	while (zend_hash_get_current_data_ex(&ce->function_table, (void**)&fe, &pos) == SUCCESS) {
		zend_function *dfe;
		int fn_len = strlen(fe->common.function_name);
		zend_class_entry *fe_scope = php_runkit_locate_scope(ce, fe, fe->common.function_name, fn_len);
				
		if (fe_scope != ce) {
			/* This is an inhereted function, let's skip it */
			zend_hash_move_forward_ex(&ce->function_table, &pos);
			continue;
		}

		if (fn_len > fn_maxlen - 1) {
			fn_maxlen = fn_len + 33;
			fn = erealloc(fn, fn_maxlen);
		}
		memcpy(fn, fe->common.function_name, fn_len + 1);
		php_strtolower(fn, fn_len);

		if (zend_hash_find(&dce->function_table, fn, fn_len + 1, (void**)&dfe) == SUCCESS) {
			zend_class_entry *scope = php_runkit_locate_scope(dce, dfe, fn, fn_len);

			if (php_runkit_check_call_stack(&dfe->op_array TSRMLS_CC) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot override active method %s::%s(). Skipping.", dce->name, fe->common.function_name);
				zend_hash_move_forward_ex(&ce->function_table, &pos);
				continue;
			}

			zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_clean_children_methods, 4, scope, dce, fn, fn_len);
			if (zend_hash_del(&dce->function_table, fn, fn_len + 1) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error removing old method in destination class %s::%s", dce->name, fe->common.function_name);
				zend_hash_move_forward_ex(&ce->function_table, &pos);
				continue;
			}
		}

		PHP_RUNKIT_FUNCTION_ADD_REF(fe);
#ifdef ZEND_ENGINE_2
		fe->common.scope = dce;
#endif
		zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_methods, 5, dce, dce, fe, fn, fn_len);

		if (zend_hash_add(&dce->function_table, fn, fn_len + 1, fe, sizeof(zend_function), NULL) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failure importing %s::%s()", ce->name, fe->common.function_name);
			zend_hash_move_forward_ex(&ce->function_table, &pos);
			continue;
		}

		zend_hash_move_forward_ex(&ce->function_table, &pos);
	}

	efree(fn);

	return SUCCESS;
}
/* }}} */

#ifdef ZEND_ENGINE_2
/* {{{ php_runkit_import_class_consts
 */
static int php_runkit_import_class_consts(zend_class_entry *dce, zend_class_entry *ce, int override TSRMLS_DC)
{
	HashPosition pos;
	char *key;
	int key_len;
	long idx;
	zval **c;

	zend_hash_internal_pointer_reset_ex(&ce->constants_table, &pos);
	while (zend_hash_get_current_data_ex(&ce->constants_table, (void**)&c, &pos) == SUCCESS && c && *c) {
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
			ZVAL_ADDREF(*c);
			if (zend_hash_add_or_update(&dce->constants_table, key, key_len, (void*)c, sizeof(zval*), NULL, action) == FAILURE) {
				zval_ptr_dtor(c);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to import %s::%s", dce->name, key);
			}

			zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_consts, 4, dce, c, key, key_len - 1);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Constant has invalid key name");
		}
import_const_skip:
		zend_hash_move_forward_ex(&ce->constants_table, &pos);
	}
	return SUCCESS;
}
/* }}} */
#endif

/* {{{ php_runkit_import_class_props
 */
static int php_runkit_import_class_props(zend_class_entry *dce, zend_class_entry *ce, int override TSRMLS_DC)
{
	HashPosition pos;
	char *key;
	int key_len;
	long idx;
	zval **p;

	zend_hash_internal_pointer_reset_ex(&ce->default_properties, &pos);
	while (zend_hash_get_current_data_ex(&ce->default_properties, (void**)&p, &pos) == SUCCESS && p && *p) {
		long action = HASH_ADD;

		if (zend_hash_get_current_key_ex(&ce->default_properties, &key, &key_len, &idx, 0, &pos) == HASH_KEY_IS_STRING) {
			char *cname = NULL, *pname = key;

#ifdef ZEND_ENGINE_2_2
			zend_unmangle_property_name(key, key_len - 1, &cname, &pname);
#elif defined(ZEND_ENGINE_2)
			zend_unmangle_property_name(key, &cname, &pname);
#endif
			if (zend_hash_exists(&dce->default_properties, key, key_len)) {
				if (override) {
					action = HASH_UPDATE;
				} else {
					php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s->%s already exists, not importing", dce->name, pname);
					goto import_prop_skip;
				}
			}
			ZVAL_ADDREF(*p);
			if (zend_hash_add_or_update(&dce->default_properties, key, key_len, (void*)p, sizeof(zval*), NULL, action) == FAILURE) {
				zval_ptr_dtor(p);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to import %s->%s", dce->name, pname);
			}

			if (!cname || strcmp(cname, "*") == 0) {
				/* PUBLIC || PROTECTED */
				zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_def_props, 4, dce, p, key, key_len - 1);
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Property has invalid key name");
		}
import_prop_skip:
		zend_hash_move_forward_ex(&ce->default_properties, &pos);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_import_classes
 */
static int php_runkit_import_classes(int original_num_classes, long flags TSRMLS_DC)
{
	HashPosition pos;
	int i;

	/* Pop the "new" classes off the class table */
	zend_hash_internal_pointer_end_ex(EG(class_table), &pos);
	for(i = zend_hash_num_elements(EG(class_table));
		i > original_num_classes; i--) {
		zend_class_entry *ce = NULL;
		char *key;
		int key_len, type;
		long idx;

		zend_hash_get_current_data_ex(EG(class_table), (void**)&ce, &pos);
#ifdef ZEND_ENGINE_2
		if (ce) {
			ce = *((zend_class_entry**)ce);
		}
#endif
		if (!ce) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Non-class in class table!");
			return FAILURE;
		}

		if (((type = zend_hash_get_current_key_ex(EG(class_table), &key, &key_len, &idx, 0, &pos)) != HASH_KEY_NON_EXISTANT) && 
			ce && ce->type == ZEND_USER_CLASS) {
			zend_class_entry *dce;

			if (ce->name_length == key_len - 1) {
				/* If their lengths are the same it's all the evidence we need that their names are the same.
					If their names are the same its because this class was freshly declared, no need for aggregation. */
				continue;
			}
			/* Otherwise aggregate methods/consts/props from temp class to real one then demolish it */

			/* We can clobber the temp class's name, it'll be freed soon anyway */
			php_strtolower(ce->name, ce->name_length);

			if (php_runkit_fetch_class(ce->name, ce->name_length, &dce TSRMLS_CC) == FAILURE) {
				/* Oddly non-existant target class or error retreiving it... Or it's an internal class... */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot redeclare class %s", ce->name);
				continue;
			}
			
			if (flags & PHP_RUNKIT_IMPORT_CLASS_METHODS) {
				php_runkit_import_class_methods(dce, ce, (flags & PHP_RUNKIT_IMPORT_OVERRIDE) TSRMLS_CC);
			}

#ifdef ZEND_ENGINE_2
			if (flags & PHP_RUNKIT_IMPORT_CLASS_CONSTS) {
				php_runkit_import_class_consts(dce, ce, (flags & PHP_RUNKIT_IMPORT_OVERRIDE) TSRMLS_CC);
			}
#endif

			if (flags & PHP_RUNKIT_IMPORT_CLASS_PROPS) {
				php_runkit_import_class_props(dce, ce, (flags & PHP_RUNKIT_IMPORT_OVERRIDE) TSRMLS_CC);
			}

			zend_hash_move_backwards_ex(EG(class_table), &pos);

			if (type == HASH_KEY_IS_STRING) {
				if (zend_hash_del(EG(class_table), key, key_len) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove temporary version of class %s", ce->name);
					continue;
				}
			} else {
				if (zend_hash_index_del(EG(class_table), idx) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove temporary version of class %s", ce->name);
					continue;
				}
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can not find class definition in class table");
			return FAILURE;
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
#if PHP_MAJOR_VERSION > 5 || (PHP_MINOR_VERSION == 5 && PHP_MINOR_VERSION > 0)
	file_handle.handle.fp = NULL;
#endif

	/* Use builtin compiler only -- bypass accelerators and whatnot */
	retval = compile_file(&file_handle, type TSRMLS_CC);
#ifdef ZEND_ENGINE_2
	if (retval && file_handle.handle.stream.handle) {
#else /* ZEND ENGINE 1 */
	if (retval && ZEND_IS_VALID_FILE_HANDLE(&file_handle)) {
#endif
		int dummy = 1;

		if (!file_handle.opened_path) {
			file_handle.opened_path = opened_path = estrndup(filename->value.str.val, filename->value.str.len);
		}

		zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy, sizeof(int), NULL);

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

/* {{{ array runkit_import(string filename[, long flags])
	Import functions and class definitions from a file 
	Similar to include(), but doesn't execute root op_array, and allows pre-existing functions/methods to be overridden */
PHP_FUNCTION(runkit_import)
{
	int original_num_functions = zend_hash_num_elements(EG(function_table));
	int original_num_classes = zend_hash_num_elements(EG(class_table));
	zend_op_array *new_op_array;
	zval *filename;
	long flags = PHP_RUNKIT_IMPORT_CLASS_METHODS;
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
	new_op_array = local_compile_filename(ZEND_INCLUDE, filename TSRMLS_CC);
	if (!new_op_array) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Import Failure");
		RETURN_FALSE;
	}
	/* We never really needed the main loop opcodes to begin with */
#ifdef ZEND_ENGINE_2
	destroy_op_array(new_op_array TSRMLS_CC);
#else
	destroy_op_array(new_op_array);
#endif
	efree(new_op_array);

	if (flags & PHP_RUNKIT_IMPORT_FUNCTIONS) {
		php_runkit_import_functions(original_num_functions TSRMLS_CC);
	}

	if (flags & PHP_RUNKIT_IMPORT_CLASSES) {
		php_runkit_import_classes(original_num_classes, flags TSRMLS_CC);
	}

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
