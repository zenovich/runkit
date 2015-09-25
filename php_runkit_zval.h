/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | (c) 2008-2015 Dmitry Zenovich                                        |
  +----------------------------------------------------------------------+
  | This source file is subject to the new BSD license,                  |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.opensource.org/licenses/BSD-3-Clause                      |
  | If you did not receive a copy of the license and are unable to       |
  | obtain it through the world-wide-web, please send a note to          |
  | dzenovich@gmail.com so we can mail you a copy immediately.           |
  +----------------------------------------------------------------------+
  | Author: Dmitry Zenovich <dzenovich@gmail.com>                        |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_RUNKIT_ZVAL_H
#define PHP_RUNKIT_ZVAL_H

/* {{{ php_runkit_zval_resolve_class_constant */
inline static void php_runkit_zval_resolve_class_constant(zval **pp, zend_class_entry *ce TSRMLS_DC) {
	if (
	    Z_TYPE_PP(pp) == IS_CONSTANT_AST
#if RUNKIT_ABOVE53
	    || (Z_TYPE_PP(pp) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#endif
	) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2) || (PHP_MAJOR_VERSION > 5)
		zval_update_constant_ex(pp, PHP_RUNKIT_CONSTANT_INDEX(1), ce TSRMLS_CC);
#else
		zval_update_constant(pp, ce TSRMLS_CC);
#endif
	}
}
/* }}} */


#define PHP_RUNKIT_ZVAL_CONVERT_TO_STRING_IF_NEEDED(val, val_copy) \
	if (Z_TYPE_P(member) != IS_STRING) { \
		val_copy = *val; \
		val = &val_copy; \
		zval_copy_ctor(val); \
		val->RUNKIT_REFCOUNT = 1; \
		convert_to_string(val); \
	}

#endif
