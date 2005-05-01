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
ZEND_DECLARE_MODULE_GLOBALS(runkit)

#ifdef PHP_RUNKIT_SUPERGLOBALS
/* {{{ proto array runkit_superglobals(void)
	Return numericly indexed array of registered superglobals */
PHP_FUNCTION(runkit_superglobals)
{
	HashPosition pos;
	char *sg;
	int sg_len, type;
	long idx;

	array_init(return_value);

	for(zend_hash_internal_pointer_reset_ex(CG(auto_globals), &pos);
		(type = zend_hash_get_current_key_ex(CG(auto_globals), &sg, &sg_len, &idx, 0, &pos)) != HASH_KEY_NON_EXISTANT;
		zend_hash_move_forward_ex(CG(auto_globals), &pos)) {
		if (type == HASH_KEY_IS_STRING) {
			add_next_index_stringl(return_value, sg, sg_len, 1);
		}
	}
}
/* }}} */
#endif /* PHP_RUNKIT_SUPERGLOBALS */

/* {{{ runkit_functions[]
 */
function_entry runkit_functions[] = {
	PHP_FE(runkit_class_emancipate,									NULL)
	PHP_FE(runkit_class_adopt,										NULL)

	PHP_FE(runkit_import,											NULL)
#ifdef PHP_RUNKIT_SUPERGLOBALS
	PHP_FE(runkit_superglobals,										NULL)
#endif

	PHP_FE(runkit_function_add,										NULL)
	PHP_FE(runkit_function_remove,									NULL)
	PHP_FE(runkit_function_rename,									NULL)
	PHP_FE(runkit_function_redefine,								NULL)
	PHP_FE(runkit_function_copy,									NULL)

	PHP_FE(runkit_method_add,										NULL)
	PHP_FE(runkit_method_redefine,									NULL)
	PHP_FE(runkit_method_remove,									NULL)
	PHP_FE(runkit_method_rename,									NULL)
	PHP_FE(runkit_method_copy,										NULL)

#ifdef PHP_RUNKIT_CLASSKIT_COMPAT
	PHP_FALIAS(classkit_method_add,			runkit_method_add,		NULL)
	PHP_FALIAS(classkit_method_redefine,	runkit_method_redefine,	NULL)
	PHP_FALIAS(classkit_method_remove,		runkit_method_remove,	NULL)
	PHP_FALIAS(classkit_method_rename,		runkit_method_rename,	NULL)
	PHP_FALIAS(classkit_method_copy,		runkit_method_copy,		NULL)
	PHP_FALIAS(classkit_import,				runkit_import,			NULL)
#endif

	PHP_FE(runkit_constant_redefine,								NULL)
	PHP_FE(runkit_constant_remove,									NULL)
	PHP_FE(runkit_constant_add,										NULL)

#ifdef PHP_RUNKIT_SANDBOX
	PHP_FE(runkit_lint,												NULL)
	PHP_FE(runkit_lint_file,										NULL)
#endif
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ runkit_module_entry
 */
zend_module_entry runkit_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"runkit",
	runkit_functions,
	PHP_MINIT(runkit),
	PHP_MSHUTDOWN(runkit),
	PHP_RINIT(runkit),
	PHP_RSHUTDOWN(runkit),
	PHP_MINFO(runkit),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_RUNKIT_VERSION, 
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

PHP_INI_BEGIN()
#ifdef PHP_RUNKIT_SUPERGLOBALS
	PHP_INI_ENTRY("runkit.superglobal", "", PHP_INI_SYSTEM|PHP_INI_PERDIR, NULL)
#endif
PHP_INI_END()

#ifdef COMPILE_DL_RUNKIT
ZEND_GET_MODULE(runkit)
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(runkit)
{
#ifdef ZTS
	ts_allocate_id(&runkit_globals_id, sizeof(zend_runkit_globals), NULL, NULL);
#endif

	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_FUNCTIONS",		PHP_RUNKIT_IMPORT_FUNCTIONS,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASS_METHODS",	PHP_RUNKIT_IMPORT_CLASS_METHODS,	CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASS_CONSTS",	PHP_RUNKIT_IMPORT_CLASS_CONSTS,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASS_PROPS",		PHP_RUNKIT_IMPORT_CLASS_PROPS,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASSES",			PHP_RUNKIT_IMPORT_CLASSES,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_OVERRIDE",		PHP_RUNKIT_IMPORT_OVERRIDE,			CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("RUNKIT_VERSION",				PHP_RUNKIT_VERSION,					CONST_CS | CONST_PERSISTENT);

#ifdef ZEND_ENGINE_2
	REGISTER_LONG_CONSTANT("RUNKIT_ACC_PUBLIC",				ZEND_ACC_PUBLIC,					CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_ACC_PROTECTED",			ZEND_ACC_PROTECTED,					CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_ACC_PRIVATE",			ZEND_ACC_PRIVATE,					CONST_CS | CONST_PERSISTENT);
#endif

#ifdef PHP_RUNKIT_CLASSKIT_COMPAT
#ifdef ZEND_ENGINE_2
	REGISTER_LONG_CONSTANT("CLASSKIT_ACC_PUBLIC",			ZEND_ACC_PUBLIC,					CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("CLASSKIT_ACC_PROTECTED",		ZEND_ACC_PROTECTED,					CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("CLASSKIT_ACC_PRIVATE",			ZEND_ACC_PRIVATE,					CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_STRING_CONSTANT("CLASSKIT_VERSION",			PHP_RUNKIT_VERSION,					CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("CLASSKIT_AGGREGATE_OVERRIDE",	PHP_RUNKIT_IMPORT_OVERRIDE,			CONST_CS | CONST_PERSISTENT);
#endif

#ifdef PHP_RUNKIT_SANDBOX
	return php_runkit_init_sandbox(INIT_FUNC_ARGS_PASSTHRU);
#else
	return SUCCESS;
#endif
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(runkit)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

#ifdef PHP_RUNKIT_SUPERGLOBALS
/* {{{ php_runkit_register_auto_global
	Register an autoglobal only if it's not already registered */
static void php_runkit_register_auto_global(char *s, int len TSRMLS_DC)
{
	if (zend_hash_exists(CG(auto_globals), s, len + 1)) {
		/* Registered already */
		return;
	}

	if (zend_register_auto_global(s, len, NULL TSRMLS_CC) == SUCCESS) {
		/* This shouldn't be necessary, but it is */
		zend_auto_global_disable_jit(s, len TSRMLS_CC);

		if (!RUNKIT_G(superglobals)) {
			ALLOC_HASHTABLE(RUNKIT_G(superglobals));
			zend_hash_init(RUNKIT_G(superglobals), 2, NULL, NULL, 0);
		}
		zend_hash_next_index_insert(RUNKIT_G(superglobals), s, len + 1, NULL);
	}
}
/* }}} */
#endif /* PHP_RUNKIT_SUPERGLOBALS */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(runkit)
{
#ifdef PHP_RUNKIT_SUPERGLOBALS
	char *s = INI_STR("runkit.superglobal"), *p;
	int len;

	RUNKIT_G(superglobals) = NULL;
	if (!s || !strlen(s)) {
		return SUCCESS;
	}

	s = estrdup(s);
	p = strchr(s, ',');
	while(p) {
		if (p - s) {
			*p = '\0';
			php_runkit_register_auto_global(s, p - s TSRMLS_CC);
		}
		s = p + 1;
		p = strchr(p, ',');
	}

	len = strlen(s);
	if (len) {
		php_runkit_register_auto_global(s, len TSRMLS_CC);
	}
	efree(s);
#endif /* PHP_RUNKIT_SUPERGLOBALS */

	return SUCCESS;
}
/* }}} */

#ifdef PHP_RUNKIT_SUPERGLOBALS
/* {{{ php_runkit_superglobal_dtor */
static int php_runkit_superglobal_dtor(char *pDest TSRMLS_DC)
{
	zend_hash_del(CG(auto_globals), pDest, strlen(pDest) + 1);

	return ZEND_HASH_APPLY_REMOVE;
}
/* }}} */
#endif /* PHP_RUNKIT_SUPERGLOBALS */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(runkit)
{
#ifdef PHP_RUNKIT_SUPERGLOBALS
	if (RUNKIT_G(superglobals)) {
		zend_hash_apply(RUNKIT_G(superglobals), php_runkit_superglobal_dtor TSRMLS_CC);

		zend_hash_destroy(RUNKIT_G(superglobals));
		FREE_HASHTABLE(RUNKIT_G(superglobals));
	}
#endif /* PHP_RUNKIT_SUPERGLOBALS */

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(runkit)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "runkit support", "enabled");
	php_info_print_table_header(2, "version", PHP_RUNKIT_VERSION);

#ifdef PHP_RUNKIT_CLASSKIT_COMPAT
	php_info_print_table_header(2, "classkit compatability", "enabled");
#endif

#ifdef PHP_RUNKIT_SUPERGLOBALS
	php_info_print_table_header(2, "Custom Superglobal support", "enabled");
#else
	php_info_print_table_header(2, "Custom Superglobal support", "unavailable");
#endif /* PHP_RUNKIT_SUPERGLOBALS */

#ifdef PHP_RUNKIT_SANDBOX
	php_info_print_table_header(2, "Sandbox Support", "enabled");
#else
	php_info_print_table_header(2, "Sandbox Support", "unavailable");
#endif /* PHP_RUNKIT_SANDBOX */

	php_info_print_table_end();

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
