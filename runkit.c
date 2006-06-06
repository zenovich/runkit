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
			add_next_index_stringl(return_value, sg, sg_len - 1, 1);
		}
	}
}
/* }}} */
#endif /* PHP_RUNKIT_SUPERGLOBALS */

/* {{{ proto array runkit_zval_inspect(mized var)
 */
PHP_FUNCTION(runkit_zval_inspect)
{
	zval *value;
	char *addr;
	int addr_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
		return;
	}

	array_init(return_value);

	addr_len = spprintf(&addr, 0, "0x%0lx", (long)value);
	add_assoc_stringl(return_value, "address", addr, addr_len, 0);

	add_assoc_long(return_value, "refcount", value->refcount);
	add_assoc_bool(return_value, "is_ref", value->is_ref);
	add_assoc_long(return_value, "type", value->type);
}
/* }}} */

/* {{{ runkit_functions[]
 */
function_entry runkit_functions[] = {

	PHP_FE(runkit_zval_inspect,										NULL)
#ifdef ZEND_ENGINE_2
	PHP_FE(runkit_object_id,										NULL)
#endif
	PHP_FE(runkit_return_value_used,								NULL)

#ifdef PHP_RUNKIT_SUPERGLOBALS
	PHP_FE(runkit_superglobals,										NULL)
#endif

#ifdef PHP_RUNKIT_MANIPULATION
	PHP_FE(runkit_class_emancipate,									NULL)
	PHP_FE(runkit_class_adopt,										NULL)
	PHP_FE(runkit_import,											NULL)

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

	PHP_FE(runkit_default_property_add,								NULL)
#endif /* PHP_RUNKIT_MANIPULATION */

#ifdef PHP_RUNKIT_SANDBOX
	PHP_FE(runkit_sandbox_output_handler,							NULL)
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

#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_MANIPULATION)
PHP_INI_BEGIN()
#ifdef PHP_RUNKIT_SUPERGLOBALS
	PHP_INI_ENTRY("runkit.superglobal", "", PHP_INI_SYSTEM|PHP_INI_PERDIR, NULL)
#endif
#ifdef PHP_RUNKIT_MANIPULATION
	STD_PHP_INI_BOOLEAN("runkit.internal_override", "0", PHP_INI_SYSTEM, OnUpdateBool, internal_override, zend_runkit_globals, runkit_globals)
#endif
PHP_INI_END()
#endif

#ifdef COMPILE_DL_RUNKIT
ZEND_GET_MODULE(runkit)
#endif

static void php_runkit_globals_ctor(zend_runkit_globals *runkit_global TSRMLS_DC)
{
#ifdef PHP_RUNKIT_SANDBOX
	runkit_global->current_sandbox = NULL;
#endif
#ifdef PHP_RUNKIT_MANIPULATION
	runkit_global->replaced_internal_functions = NULL;
	runkit_global->misplaced_internal_functions = NULL;
#endif
}

#define php_runkit_feature_constant(feature, enabled) \
		_php_runkit_feature_constant("RUNKIT_FEATURE_" #feature, sizeof("RUNKIT_FEATURE_" #feature), (enabled), \
									CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC)
static void _php_runkit_feature_constant(const char *name, size_t name_len, zend_bool enabled, 
									int flags, int module_number TSRMLS_DC)
{
	zend_constant c;

	ZVAL_BOOL(&(c.value), enabled);
	c.flags = flags;
	c.name = zend_strndup(name, name_len - 1);
	c.name_len = name_len;
	c.module_number = module_number;
	zend_register_constant(&c TSRMLS_CC);
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(runkit)
{
#ifdef ZTS
	ts_allocate_id(&runkit_globals_id, sizeof(zend_runkit_globals), php_runkit_globals_ctor, NULL);
#else
	php_runkit_globals_ctor(&runkit_globals);
#endif

#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_MANIPULATION)
	REGISTER_INI_ENTRIES();
#endif

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

	/* Feature Identifying constants */
#ifdef PHP_RUNKIT_MANIPULATION
	php_runkit_feature_constant(MANIPULATION, 1);
#else
	php_runkit_feature_constant(MANIPULATION, 0);
#endif
#ifdef PHP_RUNKIT_SUPERGLOBALS
	php_runkit_feature_constant(SUPERGLOBALS, 1);
#else
	php_runkit_feature_constant(SUPERGLOBALS, 0);
#endif
#ifdef PHP_RUNKIT_SANDBOX
	php_runkit_feature_constant(SANDBOX, 1);
#else
	php_runkit_feature_constant(SANDBOX, 0);
#endif

	return (1)
#ifdef PHP_RUNKIT_SANDBOX
		&& (php_runkit_init_sandbox(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS)
		&& (php_runkit_init_sandbox_parent(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS)
#endif
				? SUCCESS : FAILURE;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(runkit)
{
#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_MANIPULATION)
	UNREGISTER_INI_ENTRIES();
#endif

	return (1)
#ifdef PHP_RUNKIT_SANDBOX
		&& (php_runkit_shutdown_sandbox(SHUTDOWN_FUNC_ARGS_PASSTHRU) == SUCCESS)
		&& (php_runkit_shutdown_sandbox_parent(SHUTDOWN_FUNC_ARGS_PASSTHRU) == SUCCESS)
#endif
				? SUCCESS : FAILURE;
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

#ifdef ZEND_ENGINE_2
	if (zend_register_auto_global(s, len, NULL TSRMLS_CC) == SUCCESS) {

		/* This shouldn't be necessary, but it is */
		zend_auto_global_disable_jit(s, len TSRMLS_CC);
#else
	if (zend_register_auto_global(s, len TSRMLS_CC) == SUCCESS) {
#endif

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
	char *dup;
	int len;

	RUNKIT_G(superglobals) = NULL;
	if (!s || !strlen(s)) {
		goto no_superglobals_defined;
	}

	s = dup = estrdup(s);
	p = strchr(s, ',');
	while(p) {
		if (p - s) {
			*p = '\0';
			php_runkit_register_auto_global(s, p - s TSRMLS_CC);
		}
		s = ++p;
		p = strchr(p, ',');
	}

	len = strlen(s);
	if (len) {
		php_runkit_register_auto_global(s, len TSRMLS_CC);
	}
	efree(dup);
no_superglobals_defined:
#endif /* PHP_RUNKIT_SUPERGLOBALS */

#ifdef PHP_RUNKIT_SANDBOX
	RUNKIT_G(current_sandbox) = NULL;
#endif

#ifdef PHP_RUNKIT_MANIPULATION
	RUNKIT_G(replaced_internal_functions) = NULL;
	RUNKIT_G(misplaced_internal_functions) = NULL;
#endif

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

#ifdef PHP_RUNKIT_MANIPULATION
	if (RUNKIT_G(misplaced_internal_functions)) {
		/* Just wipe out rename-to targets before restoring originals */
		zend_hash_apply(RUNKIT_G(misplaced_internal_functions), php_runkit_destroy_misplaced_functions TSRMLS_CC);
		zend_hash_destroy(RUNKIT_G(misplaced_internal_functions));
		FREE_HASHTABLE(RUNKIT_G(misplaced_internal_functions));
		RUNKIT_G(misplaced_internal_functions) = NULL;
	}

	if (RUNKIT_G(replaced_internal_functions)) {
		/* Restore internal functions */
		zend_hash_apply_with_arguments(RUNKIT_G(replaced_internal_functions), php_runkit_restore_internal_functions, 1, RUNKIT_TSRMLS_C);
		zend_hash_destroy(RUNKIT_G(replaced_internal_functions));
		FREE_HASHTABLE(RUNKIT_G(replaced_internal_functions));
		RUNKIT_G(replaced_internal_functions) = NULL;
	}
#endif /* PHP_RUNKIT_MANIPULATION */

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
	php_info_print_table_header(2, "Custom Superglobal support", "disabled or unavailable");
#endif /* PHP_RUNKIT_SUPERGLOBALS */

#ifdef PHP_RUNKIT_SANDBOX
	php_info_print_table_header(2, "Sandbox Support", "enabled");
#else
	php_info_print_table_header(2, "Sandbox Support", "disable or unavailable");
#endif /* PHP_RUNKIT_SANDBOX */

#ifdef PHP_RUNKIT_MANIPULATION
	php_info_print_table_header(2, "Runtime Manipulation", "enabled");
#else
	php_info_print_table_header(2, "Runtime Manipulation", "disabled or unavailable");
#endif /* PHP_RUNKIT_MANIPULATION */

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
