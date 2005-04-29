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

/* {{{ php_runkit_remove_inherited_methods */
static int php_runkit_remove_inherited_methods(zend_function *fe, zend_class_entry *ce TSRMLS_DC)
{
	char *function_name = fe->common.function_name;
	int function_name_len = strlen(function_name);
	zend_class_entry *ancestor_class = php_runkit_locate_scope(ce, fe, function_name, function_name_len);

	if (ancestor_class == ce) {
		return ZEND_HASH_APPLY_KEEP;
	}

	zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_clean_children_methods, 4, ancestor_class, ce, function_name, function_name_len);

	PHP_RUNKIT_DEL_MAGIC_METHOD(ce, fe);

	return ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

/* {{{ proto bool runkit_class_emancipate(string classname)
	Convert an inherited class to a base class, removes any method whose scope is ancestral */
PHP_FUNCTION(runkit_class_emancipate)
{
	zend_class_entry *ce;
	char *classname;
	int classname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname, &classname_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	if (!ce->parent) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Class %s has no parent to emancipate from", classname);
		RETURN_TRUE;
	}

	zend_hash_apply_with_argument(&ce->function_table, (apply_func_arg_t)php_runkit_remove_inherited_methods, ce TSRMLS_CC);

	RETURN_TRUE;
}
/* }}} */

/* {{{ php_runkit_inherit_methods
	Inherit methods from a new ancestor */
static int php_runkit_inherit_methods(zend_function *fe, zend_class_entry *ce TSRMLS_DC)
{
	char *function_name = fe->common.function_name;
	int function_name_len = strlen(function_name);
	zend_class_entry *ancestor_class = php_runkit_locate_scope(ce, fe, function_name, function_name_len);

	if (zend_hash_exists(&ce->function_table, function_name, function_name_len + 1)) {
		return ZEND_HASH_APPLY_KEEP;
	}

	zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_methods, 5, ancestor_class, ce, fe, function_name, function_name_len);

	function_add_ref(fe);
	if (zend_hash_add_or_update(&ce->function_table, function_name, function_name_len + 1, fe, sizeof(zend_function), NULL, HASH_ADD) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error inheriting parent methods");
		return ZEND_HASH_APPLY_KEEP;
	}

	PHP_RUNKIT_ADD_MAGIC_METHOD(ce, function_name, fe);

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ proto bool runkit_class_adopt(string classname, string parentname)
	Convert a base class to an inherited class, add ancestral methods when appropriate */
PHP_FUNCTION(runkit_class_adopt)
{
	zend_class_entry *ce, *parent;
	char *classname, *parentname;
	int classname_len, parentname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &classname, &classname_len, &parentname, &parentname_len) == FAILURE) {
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

	zend_hash_apply_with_argument(&parent->function_table, (apply_func_arg_t)php_runkit_inherit_methods, ce TSRMLS_CC);

	RETURN_TRUE;
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

