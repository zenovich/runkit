/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group, (c) 2008-2015 Dmitry Zenovich |
  +----------------------------------------------------------------------+
  | This source file is subject to the new BSD license,                  |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.opensource.org/licenses/BSD-3-Clause                      |
  | If you did not receive a copy of the license and are unable to       |
  | obtain it through the world-wide-web, please send a note to          |
  | dzenovich@gmail.com so we can mail you a copy immediately.           |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  | Modified by Dmitry Zenovich <dzenovich@gmail.com>                    |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_runkit.h"
#include "php_runkit_zval.h"

#ifdef PHP_RUNKIT_MANIPULATION
/* {{{ php_runkit_remove_inherited_methods */
static int php_runkit_remove_inherited_methods(zend_function *fe, zend_class_entry *ce TSRMLS_DC)
{
	const char *fname = fe->common.function_name;
	int fname_len = strlen(fname);
	PHP_RUNKIT_DECL_STRING_PARAM(fname_lower)
	zend_class_entry *ancestor_class;

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(fname);
	if (fname_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		return ZEND_HASH_APPLY_KEEP;
	}

	ancestor_class = php_runkit_locate_scope(ce, fe, fname_lower, fname_lower_len);

	if (ancestor_class == ce) {
		efree(fname_lower);
		return ZEND_HASH_APPLY_KEEP;
	}

	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_clean_children_methods, 5,
				       ancestor_class, ce, fname_lower, fname_lower_len, fe);
	PHP_RUNKIT_DEL_MAGIC_METHOD(ce, fe TSRMLS_CC);
	php_runkit_remove_function_from_reflection_objects(fe TSRMLS_CC);

	efree(fname_lower);
	return ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

/* {{{ php_runkit_memrchr */
static inline const void *php_runkit_memrchr(const void *s, int c, size_t n)
{
	register const unsigned char *e;

	if (n <= 0) {
		return NULL;
	}

	for (e = (const unsigned char *)s + n - 1; e >= (const unsigned char *)s; e--) {
		if (*e == (const unsigned char)c) {
			return (const void *)e;
		}
	}

	return NULL;
}
/* }}} */

/* {{{ proto bool runkit_class_emancipate(string classname)
	Convert an inherited class to a base class, removes any method whose scope is ancestral */
PHP_FUNCTION(runkit_class_emancipate)
{
	zend_class_entry *ce;
	PHP_RUNKIT_DECL_STRING_PARAM(classname)
	HashPosition pos;
	char *key;
	uint key_len;
	ulong idx;
	zend_property_info *property_info_ptr = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/", &classname, &classname_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	if (!ce->parent) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Class %s has no parent to emancipate from", classname);
		RETURN_TRUE;
	}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	zend_hash_apply_with_argument(&ce->function_table, (apply_func_arg_t)php_runkit_remove_inherited_methods, ce TSRMLS_CC);

	zend_hash_internal_pointer_reset_ex(&ce->parent->properties_info, &pos);
	while (zend_hash_get_current_data_ex(&ce->parent->properties_info, (void*) &property_info_ptr, &pos) == SUCCESS && property_info_ptr) {
		if (zend_hash_get_current_key_ex(&ce->parent->properties_info, &key, &key_len, &idx, 0, &pos) == HASH_KEY_IS_STRING) {
			const char *propname = property_info_ptr->name;
			int propname_len = property_info_ptr->name_length;
			const char *last_null;

			last_null = php_runkit_memrchr(propname, 0, propname_len);
			if (last_null) {
			    propname_len -= last_null - propname + 1;
			    propname = last_null+1;
			}

			php_runkit_def_prop_remove_int(ce, propname, propname_len,
			                            ce->parent, (property_info_ptr->flags & ZEND_ACC_STATIC) != 0,
			                            1 /* remove_from_objects */, property_info_ptr TSRMLS_CC);
		}
		zend_hash_move_forward_ex(&ce->parent->properties_info, &pos);
	}
	ce->parent = NULL;

	RETURN_TRUE;
}
/* }}} */

/* {{{ php_runkit_inherit_methods
	Inherit methods from a new ancestor */
static int php_runkit_inherit_methods(zend_function *fe, zend_class_entry *ce TSRMLS_DC)
{
	const char *fname = fe->common.function_name;
	int fname_len = strlen(fname);
	PHP_RUNKIT_DECL_STRING_PARAM(fname_lower)
	zend_class_entry *ancestor_class;

	/* method name keys must be lower case */
	PHP_RUNKIT_MAKE_LOWERCASE_COPY(fname);
	if (fname_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		return ZEND_HASH_APPLY_KEEP;
	}

	if (zend_hash_exists(&ce->function_table, (char *) fname_lower, fname_lower_len + 1)) {
		efree(fname_lower);
		return ZEND_HASH_APPLY_KEEP;
	}

	ancestor_class = php_runkit_locate_scope(ce, fe, fname_lower, fname_lower_len);

	if (zend_hash_add_or_update(&ce->function_table, fname_lower, fname_lower_len + 1, fe, sizeof(zend_function), NULL, HASH_ADD) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error inheriting parent method: %s()", fe->common.function_name);
		efree(fname_lower);
		return ZEND_HASH_APPLY_KEEP;
	}

	if (zend_hash_find(&ce->function_table, fname_lower, fname_lower_len + 1, (void*)&fe) == FAILURE ||
	    !fe) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to locate newly added method");
		efree(fname_lower);
		return ZEND_HASH_APPLY_KEEP;
	}

	PHP_RUNKIT_FUNCTION_ADD_REF(fe);
	PHP_RUNKIT_ADD_MAGIC_METHOD(ce, fname_lower, fname_lower_len, fe, NULL TSRMLS_CC);

	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_methods, 6,
				       ancestor_class, ce, fe, fname_lower, fname_lower_len, NULL);
	efree(fname_lower);

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_class_copy
       Copy class into class table */
int php_runkit_class_copy(zend_class_entry *src, const char *classname, int classname_len TSRMLS_DC)
{
	zend_class_entry *new_class_entry, *parent = NULL;
	PHP_RUNKIT_DECL_STRING_PARAM(classname_lower)

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(classname);
	if (!classname_lower) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		return FAILURE;
	}

	new_class_entry = emalloc(sizeof(zend_class_entry));
	if (src->parent && src->parent->name) {
		php_runkit_fetch_class_int(src->parent->name, src->parent->name_length, &parent TSRMLS_CC);
	}
	new_class_entry->type = ZEND_USER_CLASS;
	new_class_entry->name = estrndup(classname, classname_len);
	new_class_entry->name_length = classname_len;

	zend_initialize_class_data(new_class_entry, 1 TSRMLS_CC);
	new_class_entry->parent = parent;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	new_class_entry->info.user.filename = src->info.user.filename;
	new_class_entry->info.user.line_start = src->info.user.line_start;
	new_class_entry->info.user.doc_comment = src->info.user.doc_comment;
	new_class_entry->info.user.doc_comment_len = src->info.user.doc_comment_len;
	new_class_entry->info.user.line_end = src->info.user.line_end;
	new_class_entry->num_traits = src->num_traits;
	new_class_entry->traits = src->traits;
#else
	new_class_entry->filename = src->filename;
	new_class_entry->line_start = src->line_start;
	new_class_entry->doc_comment = src->doc_comment;
	new_class_entry->doc_comment_len = src->doc_comment_len;
	new_class_entry->line_end = src->line_end;
#endif
	new_class_entry->ce_flags = src->ce_flags;

	zend_hash_update(EG(class_table), classname_lower, classname_lower_len + 1, &new_class_entry, sizeof(zend_class_entry *), NULL);

	new_class_entry->num_interfaces = src->num_interfaces;
	efree(classname_lower);

	if (new_class_entry->parent) {
		zend_hash_apply_with_argument(&(new_class_entry->parent->function_table),
		                              (apply_func_arg_t)php_runkit_inherit_methods, new_class_entry TSRMLS_CC);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ proto bool runkit_class_adopt(string classname, string parentname)
	Convert a base class to an inherited class, add ancestral methods when appropriate */
PHP_FUNCTION(runkit_class_adopt)
{
	zend_class_entry *ce, *parent;
	PHP_RUNKIT_DECL_STRING_PARAM(classname)
	PHP_RUNKIT_DECL_STRING_PARAM(parentname)
	HashPosition pos;
	char *key;
	uint key_len;
	ulong idx;
	zend_property_info *property_info_ptr = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/", &classname, &classname_len, &parentname, &parentname_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	if (ce->parent) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Class %s already has a parent", classname);
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class(parentname, parentname_len, &parent TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	ce->parent = parent;

	zend_hash_internal_pointer_reset_ex(&parent->properties_info, &pos);
	while (zend_hash_get_current_data_ex(&parent->properties_info, (void*) &property_info_ptr, &pos) == SUCCESS && property_info_ptr) {
		if (zend_hash_get_current_key_ex(&parent->properties_info, &key, &key_len, &idx, 0, &pos) == HASH_KEY_IS_STRING) {
			zval **pp;
			const char *propname = property_info_ptr->name;
			int propname_len = property_info_ptr->name_length;
			const char *last_null;

			if (property_info_ptr->flags & ZEND_ACC_STATIC) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
				pp = &CE_STATIC_MEMBERS(parent)[property_info_ptr->offset];
#else
				zend_hash_quick_find(CE_STATIC_MEMBERS(parent), property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h, (void*) &pp);
#endif
			} else {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4)
				pp = &parent->default_properties_table[property_info_ptr->offset];
#else
				if (zend_hash_quick_find(&parent->default_properties, property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h, (void*) &pp) != SUCCESS) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING,
					                 "Cannot inherit broken default property %s->%s", parent->name, key);
					zend_hash_move_forward_ex(&ce->properties_info, &pos);
					continue;
				}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			}

			php_runkit_zval_resolve_class_constant(pp, parent TSRMLS_CC);

			last_null = php_runkit_memrchr(propname, 0, propname_len);
			if (last_null) {
			    propname_len -= last_null - propname + 1;
			    propname = last_null+1;
			}

			php_runkit_def_prop_add_int(ce, propname, propname_len,
			                            *pp, property_info_ptr->flags/*visibility*/,
			                            property_info_ptr->doc_comment, property_info_ptr->doc_comment_len,
			                            property_info_ptr->ce /*definer_class*/, 0 /*override*/, 1 /*override_in_objects*/ TSRMLS_CC);
		}
		zend_hash_move_forward_ex(&ce->properties_info, &pos);
	}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	zend_hash_apply_with_argument(&parent->function_table, (apply_func_arg_t)php_runkit_inherit_methods, ce TSRMLS_CC);

	RETURN_TRUE;
}
/* }}} */

#endif /* PHP_RUNKIT_MANIPULATION */

/* {{{ proto int runkit_object_id(object instance)
Fetch the Object Handle ID from an instance */
PHP_FUNCTION(runkit_object_id)
{
	zval *obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &obj) == FAILURE) {
		RETURN_NULL();
	}

	RETURN_LONG(Z_OBJ_HANDLE_P(obj));
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

