/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
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

/* {{{ php_runkit_update_children_def_props
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_def_props(zend_class_entry *ce, int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zval *p = va_arg(args, zval*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	TSRMLS_FETCH();

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_def_props, 4, ce, p, pname, pname_len);

	zend_hash_del(&ce->default_properties, pname, pname_len + 1);
	ZVAL_ADDREF(p);
	if (zend_hash_add(&ce->default_properties, pname, pname_len + 1, p, sizeof(zval*), NULL) ==  FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
		return ZEND_HASH_APPLY_KEEP;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_prop_add
 */
static int php_runkit_def_prop_add(char *classname, int classname_len, char *propname, int propname_len, zval *value, int visibility TSRMLS_DC)
{
	zend_class_entry *ce;
	zval *copyval;
	char *temp, *key = propname;
	int temp_len, key_len = propname_len;

	switch (value->type) {
		case IS_LONG:
		case IS_DOUBLE:
		case IS_STRING:
		case IS_BOOL:
		case IS_RESOURCE:
		case IS_NULL:
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Default properties may only evaluate to scalar values");
			return FAILURE;
	}

	if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC)==FAILURE) {
		return FAILURE;
	}

	if (ce->type != ZEND_USER_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Adding properties to internal classes is not allowed");
		return FAILURE;
	}

	/* Check for existing property by this name */
	/* Existing public? */
	if (zend_hash_exists(&ce->default_properties, key, key_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}

	/* Existing Protected? */
	zend_mangle_property_name(&temp, &temp_len, "*", 1, propname, propname_len, 0);
	if (zend_hash_exists(&ce->default_properties, temp, temp_len + 1)) {
		efree(temp);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
	if (visibility == ZEND_ACC_PROTECTED) {
		key = temp;
		key_len = temp_len;
	} else {
		efree(temp);
	}

	/* Existing Private? */
	zend_mangle_property_name(&temp, &temp_len, classname, classname_len, propname, propname_len, 0);
	if (zend_hash_exists(&ce->default_properties, temp, temp_len + 1)) {
		efree(temp);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
	if (visibility == ZEND_ACC_PRIVATE) {
		key = temp;
		key_len = temp_len;
	} else {
		efree(temp);
	}


	ALLOC_ZVAL(copyval);
	*copyval = *value;
	zval_copy_ctor(copyval);
	copyval->refcount = 1;
	copyval->is_ref = 0;

	if (zend_hash_add(&ce->default_properties, key, key_len + 1, &copyval, sizeof(zval *), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add default property to class definition");
		zval_ptr_dtor(&copyval);
		return FAILURE;
	}

	if (visibility != ZEND_ACC_PRIVATE) {
		zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_def_props, 4, ce, copyval, key, key_len);
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
	long visibility = ZEND_ACC_PUBLIC;
	int existing_visibility;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/z|l", &classname, &classname_len, &propname, &propname_len, &value, &visibility) == FAILURE) {
		RETURN_FALSE;
	}

	php_strtolower(classname, classname_len);
	php_strtolower(propname, propname_len);

	RETURN_BOOL(php_runkit_def_prop_add(classname, classname_len, propname, propname_len, value, visibility TSRMLS_CC) == SUCCESS);
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

