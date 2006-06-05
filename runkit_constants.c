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

#ifdef PHP_RUNKIT_MANIPULATION
/* {{{ php_runkit_fetch_const
 */
static int php_runkit_fetch_const(char *cname, int cname_len, zend_constant **constant TSRMLS_DC)
{
	if (zend_hash_find(EG(zend_constants), cname, cname_len + 1, (void**)constant) == FAILURE) {
		char *lcase;

		lcase = estrndup(cname, cname_len);
		if (zend_hash_find(EG(zend_constants), lcase, cname_len + 1, (void**)constant) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Constant %s not found", cname);
			efree(lcase);
			return FAILURE;
		}
		efree(lcase);

		if ((*constant)->flags & CONST_CS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Constant %s not found", cname);
			return FAILURE;
		}
	}

	return SUCCESS;
}
/* }}} */

#ifdef ZEND_ENGINE_2
/* {{{ php_runkit_update_children_consts
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_consts(zend_class_entry *ce, int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zval *c = va_arg(args, zval*);
	char *cname = va_arg(args, char*);
	int cname_len = va_arg(args, int);
	TSRMLS_FETCH();

/* Redundant I know, but it's too keep these things consistent */
#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_consts, 4, ce, c, cname, cname_len);

	zend_hash_del(&ce->constants_table, cname, cname_len + 1);
	ZVAL_ADDREF(c);
	if (zend_hash_add(&ce->constants_table, cname, cname_len + 1, c, sizeof(zval*), NULL) ==  FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
		return ZEND_HASH_APPLY_KEEP;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */
#endif

/* {{{ php_runkit_constant_remove
 */
static int php_runkit_constant_remove(char *classname, int classname_len, char *constname, int constname_len TSRMLS_DC)
{
	zend_constant *constant;
	char *lcase = NULL;

	if (classname && (classname_len > 0)) {
#ifdef ZEND_ENGINE_2
		zend_class_entry *ce;

		if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC)==FAILURE) {
			return FAILURE;
		}

		if (!zend_hash_exists(&ce->constants_table, constname, constname_len+1)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Constant %s::%s does not exist", classname, constname);
			return FAILURE;
		}
		if (zend_hash_del(&ce->constants_table, constname, constname_len+1)==FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove constant %s::%s", classname, constname);
			return FAILURE;
		}
		return SUCCESS;
#else
		/* PHP4 doesn't support class constants */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Class constants require PHP 5.0 or above");
		return FAILURE;
#endif
	}

	if (php_runkit_fetch_const(constname, constname_len, &constant TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

#ifdef ZEND_ENGINE_2
	if (constant->module_number != PHP_USER_CONSTANT) {
#else
	/* Not strictly legal/safe
	 * module_number can't necessarily be counted on to == 0 since it's not initialized (thanks guys)
	 * But do you know of any internal constants that aren't persistent?  I don't.
	 */
	if (constant->flags & CONST_PERSISTENT) {
#endif
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Only user defined constants may be removed.");
		return FAILURE;
	}

	if ((constant->flags & CONST_CS) == 0) {
		lcase = estrndup(constant->name, constant->name_len);
		php_strtolower(lcase, constant->name_len);
		constname = lcase;
	} else {
		constname = constant->name;
	}

	if (zend_hash_del(EG(zend_constants), constname, constant->name_len) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove constant");
		if (lcase) {
			efree(lcase);
		}
		return FAILURE;
	}
	if (lcase) {
		efree(lcase);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_constant_add
 */
static int php_runkit_constant_add(char *classname, int classname_len, char *constname, int constname_len, zval *value TSRMLS_DC)
{
#ifdef ZEND_ENGINE_2
	zend_class_entry *ce;
	zval *copyval;
#endif

	switch (value->type) {
		case IS_LONG:
		case IS_DOUBLE:
		case IS_STRING:
		case IS_BOOL:
		case IS_RESOURCE:
		case IS_NULL:
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Constants may only evaluate to scalar values");
			return FAILURE;
	}

	if ((classname == NULL) || (classname_len == 0)) {
		zend_constant c;

		/* Traditional global constant */
		c.value = *value;
		zval_copy_ctor(&c.value);
		c.flags = CONST_CS;
		c.name = zend_strndup(constname, constname_len);
		c.name_len = constname_len + 1;
#ifdef ZEND_ENGINE_2
		c.module_number = PHP_USER_CONSTANT;
#else
		c.module_number = 0;
#endif
		return zend_register_constant(&c TSRMLS_CC);
	}

#ifdef ZEND_ENGINE_2
	if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC)==FAILURE) {
		return FAILURE;
	}

	ALLOC_ZVAL(copyval);
	*copyval = *value;
	zval_copy_ctor(copyval);
	copyval->refcount = 1;
	copyval->is_ref = 0;
	if (zend_hash_add(&ce->constants_table, constname, constname_len + 1, &copyval, sizeof(zval *), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add constant to class definition");
		zval_ptr_dtor(&copyval);
		return FAILURE;
	}

	zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_update_children_consts, 4, ce, copyval, constname, constname_len);

	return SUCCESS;
#else
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Class constants require PHP 5.0 or above");

	return FAILURE;
#endif
}
/* }}} */

/* *****************
   * Constants API *
   ***************** */

/* Note: While PHP4 does not actually allow class based constants,
 * the parameter is left in so that the API remains consistent
 */

/* {{{ proto bool runkit_constant_redefine(string constname, mixed newvalue)
 */
PHP_FUNCTION(runkit_constant_redefine)
{
	char *classname = NULL, *constname;
	int classname_len = 0, constname_len;
	zval *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &constname, &constname_len, &value) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_RUNKIT_SPLIT_PN(classname, classname_len, constname, constname_len);

	php_runkit_constant_remove(classname, classname_len, constname, constname_len TSRMLS_CC);
	RETURN_BOOL(php_runkit_constant_add(classname, classname_len, constname, constname_len, value TSRMLS_CC) == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_constant_remove(string constname)
 */
PHP_FUNCTION(runkit_constant_remove)
{
	char *classname = NULL, *constname;
	int classname_len = 0, constname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &constname, &constname_len) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_RUNKIT_SPLIT_PN(classname, classname_len, constname, constname_len);

	RETURN_BOOL(php_runkit_constant_remove(classname, classname_len, constname, constname_len TSRMLS_CC)==SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_constant_add(string constname, mixed value)
	Similar to define(), but allows defining in class definitions as well
 */
PHP_FUNCTION(runkit_constant_add)
{
	char *classname = NULL, *constname;
	int classname_len = 0, constname_len;
	zval *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &constname, &constname_len, &value) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_RUNKIT_SPLIT_PN(classname, classname_len, constname, constname_len);

	RETURN_BOOL(php_runkit_constant_add(classname, classname_len, constname, constname_len, value TSRMLS_CC) == SUCCESS);
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

