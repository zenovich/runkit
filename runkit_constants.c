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

#ifdef PHP_RUNKIT_MANIPULATION
/* {{{ php_runkit_fetch_const
 */
static int php_runkit_fetch_const(char *cname, int cname_len, zend_constant **constant, char **found_cname TSRMLS_DC)
{
	char *lcase = NULL;
	char *old_cname = cname;
	int constant_name_len = cname_len;

#if RUNKIT_ABOVE53
	char *separator;

	if (cname_len > 0 && cname[0] == '\\') {
		++cname;
		--cname_len;
	}

	separator = (char *) zend_memrchr(cname, '\\', cname_len);
	if (separator) {
		int nsname_len = separator - cname;
		char *constant_name = separator + 1;
		constant_name_len = cname_len - nsname_len - 1;

		lcase = emalloc(nsname_len + 1 + constant_name_len + 1);
		memcpy(lcase, cname, nsname_len + 1);
		memcpy(lcase + nsname_len + 1, constant_name, constant_name_len + 1);
		zend_str_tolower(lcase, nsname_len);
		cname = lcase;
	}
#endif
	if (zend_hash_find(EG(zend_constants), cname, cname_len + 1, (void*)constant) == FAILURE) {
		if (!lcase) {
			lcase = estrndup(cname, cname_len);
			zend_str_tolower(lcase, cname_len);
		} else {
			zend_str_tolower(lcase + cname_len - constant_name_len, constant_name_len);
		}
		cname = lcase;
		if (zend_hash_find(EG(zend_constants), cname, cname_len + 1, (void*)constant) == FAILURE || ((*constant)->flags & CONST_CS)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Constant %s not found", old_cname);
			efree(lcase);
			return FAILURE;
		}
	}
	if (found_cname) {
		*found_cname = lcase ? lcase : estrndup(cname, cname_len);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_update_children_consts
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_consts(RUNKIT_53_TSRMLS_ARG(void *pDest), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *ce = (zend_class_entry *) pDest;
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zval **c = va_arg(args, zval**);
	char *cname = va_arg(args, char*);
	int cname_len = va_arg(args, int);
	RUNKIT_UNDER53_TSRMLS_FETCH();

	ce = *((zend_class_entry**)ce);

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), php_runkit_update_children_consts, 4, ce, c, cname, cname_len);

	Z_ADDREF_P(*c);

	zend_hash_del(&ce->constants_table, cname, cname_len + 1);
	if (zend_hash_add(&ce->constants_table, cname, cname_len + 1, (void *) c, sizeof(zval**), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
		return ZEND_HASH_APPLY_KEEP;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_constant_remove
 */
static int php_runkit_constant_remove(char *classname, int classname_len, char *constname, int constname_len TSRMLS_DC)
{
	zend_constant *constant;
	char *found_constname;

	if (classname && (classname_len > 0)) {
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
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif
		return SUCCESS;
	}

	if (php_runkit_fetch_const(constname, constname_len, &constant, &found_constname TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (constant->module_number != PHP_USER_CONSTANT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Only user defined constants may be removed.");
		return FAILURE;
	}

	if (zend_hash_del(EG(zend_constants), found_constname, constant->name_len) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove constant");
		efree(found_constname);
		return FAILURE;
	}
	efree(found_constname);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_constant_add
 */
static int php_runkit_constant_add(char *classname, int classname_len, char *constname, int constname_len, zval *value TSRMLS_DC)
{
	zend_class_entry *ce;
	zval *copyval;

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

#if RUNKIT_ABOVE53
		if (constname_len > 0 && constname[0] == '\\') {
			++constname;
			--constname_len;
		}
#endif

		/* Traditional global constant */
		c.value = *value;
		zval_copy_ctor(&c.value);
		c.flags = CONST_CS;
		c.name = zend_strndup(constname, constname_len);
		c.name_len = constname_len + 1;
		c.module_number = PHP_USER_CONSTANT;
		return zend_register_constant(&c TSRMLS_CC);
	}

	if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC)==FAILURE) {
		return FAILURE;
	}

	ALLOC_ZVAL(copyval);
	*copyval = *value;
	zval_copy_ctor(copyval);
	if (zend_hash_add(&ce->constants_table, constname, constname_len + 1, &copyval, sizeof(zval *), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add constant to class definition");
		zval_ptr_dtor(&copyval);
		return FAILURE;
	}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_consts, 4, ce, &copyval, constname, constname_len);

	return SUCCESS;
}
/* }}} */

/* *****************
   * Constants API *
   ***************** */

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

