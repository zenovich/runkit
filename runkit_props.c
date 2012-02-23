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

#ifdef PHP_RUNKIT_MANIPULATION

/* {{{ php_runkit_update_children_def_props
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_def_props(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zval *p = va_arg(args, zval*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	RUNKIT_UNDER53_TSRMLS_FETCH();

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	if (
		Z_TYPE_P(p) == IS_CONSTANT_ARRAY
#if RUNKIT_ABOVE53
		|| (Z_TYPE_P(p) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#endif
	) {
		zval_update_constant_ex(&p, (void*) 1, ce TSRMLS_CC);
	}

	Z_ADDREF_P(p);
	zend_hash_del(&ce->default_properties, pname, pname_len + 1);
	if (zend_hash_add(&ce->default_properties, pname, pname_len + 1, (void *) &p, sizeof(zval*), NULL) ==  FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_def_props, 4, ce, p, pname, pname_len);

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_remove_children_def_props
	Scan the class_table for children of the class just removed */
int php_runkit_remove_children_def_props(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	RUNKIT_UNDER53_TSRMLS_FETCH();

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_remove_children_def_props, 4, ce, pname, pname_len);

	if (zend_hash_del(&ce->default_properties, pname, pname_len + 1) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error removing default property from child class");
		return ZEND_HASH_APPLY_KEEP;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_update_children_static_props
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_static_props(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zval *p = va_arg(args, zval*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	RUNKIT_UNDER53_TSRMLS_FETCH();

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_static_props, 4, ce, p, pname, pname_len);

	zend_hash_del((HashTable*) (CE_STATIC_MEMBERS(ce)), pname, pname_len + 1);
	Z_ADDREF_P(p);
	if (zend_hash_add((HashTable *) (CE_STATIC_MEMBERS(ce)), pname, pname_len + 1, p, sizeof(zval*), NULL) ==  FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
		return ZEND_HASH_APPLY_KEEP;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_def_prop_add
 */
static int php_runkit_def_prop_add(char *classname, int classname_len, char *propname, int propname_len, zval *value, long visibility TSRMLS_DC)
{
	zend_class_entry *ce;
	zval *copyval;
	char *temp, *key = propname;
	int temp_len, key_len = propname_len;

	if (php_runkit_fetch_class_int(classname, classname_len, &ce TSRMLS_CC)==FAILURE) {
		return FAILURE;
	}

	if (ce->type & ZEND_INTERNAL_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Adding properties to internal classes is not allowed");
		return FAILURE;
	} else {
		ALLOC_ZVAL(copyval);
		*copyval = *value;
		zval_copy_ctor(copyval);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION >= 6)
		Z_SET_REFCOUNT_P(copyval, 1);
		Z_UNSET_ISREF_P(copyval);
#else
		copyval->RUNKIT_REFCOUNT = 1;
		copyval->is_ref = 0;
#endif
	}

	/* Check for existing property by this name */
	/* Existing public? */
	if (zend_hash_exists(&ce->default_properties, key, key_len + 1)) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}

#ifdef ZEND_ENGINE_2
	/* Existing Protected? */
	zend_mangle_property_name(&temp, &temp_len, "*", 1, propname, propname_len, 0);
	if (zend_hash_exists(&ce->default_properties, temp, temp_len + 1)) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		efree(temp);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
	efree(temp);

	/* Existing Private? */
	zend_mangle_property_name(&temp, &temp_len, classname, classname_len, propname, propname_len, 0);
	if (zend_hash_exists(&ce->default_properties, temp, temp_len + 1)) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		efree(temp);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
	efree(temp);

	if (zend_declare_property(ce, propname, propname_len, copyval, visibility TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		return FAILURE;
	}
	if (visibility != ZEND_ACC_PRIVATE) {
		if (visibility == ZEND_ACC_PROTECTED) {
			char *fullpname;
			int fullpname_len;
			zend_mangle_property_name(&fullpname, &fullpname_len, "*", 1, propname, propname_len, 0);
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_def_props, 4, ce, copyval, fullpname, fullpname_len);
			efree(fullpname);
		} else {
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_def_props, 4, ce, copyval, propname, propname_len);
		}
	}

	return SUCCESS;
#else
	if (zend_hash_add(&ce->default_properties, key, key_len+1, &copyval, sizeof(zval*), NULL) == FAILURE) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add default property to class definition");
		return FAILURE;
	}

	return SUCCESS;
#endif
}
/* }}} */

/* {{{ php_runkit_def_prop_remove */
static int php_runkit_def_prop_remove(char *classname, int classname_len, char *propname, int propname_len TSRMLS_DC)
{
	zend_class_entry *ce;
	char *protected=0, *private=0, *key = 0;
	int protected_len, private_len, key_len = 0;
	zval **actual_value;
	int is_private = 0;

	if (php_runkit_fetch_class_int(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (ce->type & ZEND_INTERNAL_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Removing properties from internal classes is not allowed");
		return FAILURE;
	}

	/* Resolve the property's name */
	if (zend_hash_exists(&ce->default_properties, propname, propname_len + 1)) {
		key = propname;
		key_len = propname_len;
	}

#ifdef ZEND_ENGINE_2
	if (!key) {
		zend_mangle_property_name(&protected, &protected_len, "*", 1, propname, propname_len, 0);
		if (zend_hash_exists(&ce->default_properties, protected, protected_len + 1)) {
			key = protected;
			key_len = protected_len;
		} else {
			efree(protected);
		}
	}

	if (!key) {
		zend_mangle_property_name(&private, &private_len, classname, classname_len, propname, propname_len, 0);
		if (zend_hash_exists(&ce->default_properties, private, private_len + 1)) {
			key = private;
			key_len = private_len;
			is_private = 1;
		} else {
			efree(private);
		}
	}
#endif

	if (!key) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s does not exist", classname, propname);
		return FAILURE;
	}

	if (zend_hash_del(&ce->default_properties, key, key_len + 1) == FAILURE) {
		if (key != propname) {
			efree(key);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property from class: %s::%s", classname, propname);
		return FAILURE;
	}

	if (!is_private) {
		zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_remove_children_def_props, 4, ce, key, key_len);
	}

	if (key != propname) {
		efree(key);
	}
	return SUCCESS;
}
/* }}} */

/* ******************
   * Properties API *
   ****************** */

/* {{{ proto bool runkit_default_property_add(string classname, string propname, mixed initialvalue[, int visibility])
Add a property to a class with a given visibility
 */
PHP_FUNCTION(runkit_default_property_add)
{
	char *classname, *propname;
	int classname_len, propname_len;
	zval *value;
	long visibility;

#ifdef ZEND_ENGINE_2
	visibility = ZEND_ACC_PUBLIC;
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/z|l", &classname, &classname_len, &propname, &propname_len, &value, &visibility) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_BOOL(php_runkit_def_prop_add(classname, classname_len, propname, propname_len, value, visibility TSRMLS_CC) == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_default_property_remove(string classname, string propname)
Remove a property from a class
 */
PHP_FUNCTION(runkit_default_property_remove)
{
       char *classname, *propname;
       int classname_len, propname_len;

       if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/", &classname, &classname_len, &propname, &propname_len) == FAILURE) {
               RETURN_FALSE;
       }

       RETURN_BOOL(php_runkit_def_prop_remove(classname, classname_len, propname, propname_len TSRMLS_CC) == SUCCESS);
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
