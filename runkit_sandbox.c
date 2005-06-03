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
  |	Props:  Wez Furlong                                                  |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_runkit.h"

#ifdef PHP_RUNKIT_SANDBOX
#include "SAPI.h"

static int le_sandbox_state;
static zend_object_handlers php_runkit_object_handlers;
static zend_class_entry *php_runkit_sandbox_class_entry;

struct _php_runkit_sandbox_data {
	void *context, *parent_context;
	char *disable_functions;
	char *disable_classes;
	zval *output_handler; /* points to function which lives in the parent_context */
};

#define PHP_RUNKIT_SANDBOX_PROP(sandbox_name, sandbox_name_len) \
	zend_mangle_property_name(sandbox_name, sandbox_name_len,	PHP_RUNKIT_SANDBOX_CLASSNAME,	sizeof(PHP_RUNKIT_SANDBOX_CLASSNAME) - 1, \
																PHP_RUNKIT_SANDBOX_PROPNAME,	sizeof(PHP_RUNKIT_SANDBOX_PROPNAME) - 1, 0)

#define PHP_RUNKIT_SANDBOX_BEGIN(data) \
{ \
	void *prior_context = tsrm_set_interpreter_context(data->context); \
	TSRMLS_FETCH(); \
	RUNKIT_G(current_sandbox) = data;

#define PHP_RUNKIT_SANDBOX_ABORT(data) \
	RUNKIT_G(current_sandbox) = NULL; \
	tsrm_set_interpreter_context(prior_context);

#define PHP_RUNKIT_SANDBOX_END(data) \
	PHP_RUNKIT_SANDBOX_ABORT(data); \
}

#define PHP_RUNKIT_SANDBOX_FETCHBOX(obj) \
{ \
	char *sandbox_name; \
	int sandbox_name_len; \
	zval **resptr; \
	PHP_RUNKIT_SANDBOX_PROP(&sandbox_name, &sandbox_name_len); \
	if (zend_hash_find((obj), sandbox_name, sandbox_name_len + 1, (void**)&resptr) == FAILURE) { \
		efree(sandbox_name); \
		RETURN_FALSE; \
	} \
	efree(sandbox_name); \
	ZEND_FETCH_RESOURCE(data, php_runkit_sandbox_data*, resptr, -1, PHP_RUNKIT_SANDBOX_RESNAME, le_sandbox_state); \
}

/* TODO: It'd be nice if objects and resources could make it across... */
#define PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(pzv) \
{ \
	switch (Z_TYPE_P(pzv)) { \
		case IS_RESOURCE: \
		case IS_OBJECT: \
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to translate resource, or object variable to current context."); \
			ZVAL_NULL(pzv); \
			break; \
		case IS_ARRAY: \
		{ \
			HashTable *original_hashtable = Z_ARRVAL_P(pzv); \
			array_init(pzv); \
			zend_hash_apply_with_arguments(original_hashtable, (apply_func_args_t)php_runkit_sandbox_array_deep_copy, 1, Z_ARRVAL_P(pzv) TSRMLS_CC); \
			break; \
		} \
		default: \
			zval_copy_ctor(pzv); \
	} \
	(pzv)->refcount = 1; \
	(pzv)->is_ref = 0; \
}

static int php_runkit_sandbox_array_deep_copy(zval **value, int num_args, va_list args, zend_hash_key *hash_key)
{
	HashTable *target_hashtable = va_arg(args, HashTable*);
	TSRMLS_D = va_arg(args, void***);
	zval *copyval;

	MAKE_STD_ZVAL(copyval);
	*copyval = **value;

	PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(copyval);

	if (hash_key->nKeyLength) {
		zend_hash_quick_update(target_hashtable, hash_key->arKey, hash_key->nKeyLength, hash_key->h, &copyval, sizeof(zval*), NULL);
	} else {
		zend_hash_index_update(target_hashtable, hash_key->h, &copyval, sizeof(zval*), NULL);
	}

	return ZEND_HASH_APPLY_KEEP;
}

/* ******************
   * Runkit_Sandbox *
   ****************** */

/* {{{ proto void Runkit_Sandbox::__construct(array options)
 * Options:
 * safe_mode = true
 *		safe_mode can only be turned on for a sandbox, not off.  Doing so would tend to defeat safe_mode as applied to the calling script
 * open_basedir = /path/to/basedir
 *		open_basedir must be at or below the currently defined basedir for the same reason that safe_mode can only be turned on
 * allow_url_fopen = false
 *		allow_url_fopen may only be turned off for a sandbox, not on.   Once again, don't castrate the existing restrictions
 * disable_functions = coma_separated,list_of,additional_functions
 *		ADDITIONAL functions, on top of already disabled functions to disable
 * disable_classes = coma_separated,list_of,additional_classes
 *		ADDITIONAL classes, on top of already disabled classes to disable
 * runkit.superglobals = coma_separated,list_of,superglobals
 *		ADDITIONAL superglobals to define in the subinterpreter
 */
PHP_METHOD(Runkit_Sandbox,__construct)
{
	zval *zsandbox, *options = NULL, **tmpzval;
	char *sandbox_name;
	int sandbox_name_len;
	php_runkit_sandbox_data *data;
	zend_bool safe_mode = 0, use_open_basedir = 0, disallow_url_fopen = 0;
	char open_basedir[MAXPATHLEN] = {0};
	int open_basedir_len = 0, disable_functions_len = 0, disable_classes_len, superglobals_len;
	char *disable_functions = NULL, *disable_classes = NULL, *superglobals = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &options) == FAILURE) {
		RETURN_NULL();
	}

	if (options &&
		zend_hash_find(Z_ARRVAL_P(options), "safe_mode", sizeof("safe_mode"), (void**)&tmpzval) == SUCCESS &&
		zval_is_true(*tmpzval)) {
		/* Can be turned on from off, but not the other way around */
		safe_mode = 1;
	}

	if (options &&
		zend_hash_find(Z_ARRVAL_P(options), "open_basedir", sizeof("open_basedir"), (void**)&tmpzval) == SUCCESS) {
		zval copyval = **tmpzval;
		char current_basedir[MAXPATHLEN] = {0}, *pcurrent_basedir = INI_STR("open_basedir");
		int current_basedir_len = 0;

		zval_copy_ctor(&copyval);
		copyval.refcount = 1;
		copyval.is_ref = 0;
		convert_to_string(&copyval);
		realpath(Z_STRVAL(copyval), open_basedir);
		open_basedir_len = strlen(open_basedir);
		zval_dtor(&copyval);

		if (!pcurrent_basedir) {
			use_open_basedir = 1;
		} else {
			realpath(pcurrent_basedir, current_basedir);
			current_basedir_len = strlen(current_basedir);
			if (current_basedir_len == 0 || (
				current_basedir_len < open_basedir_len &&
				strncmp(current_basedir, open_basedir, current_basedir_len) == 0)) {
				use_open_basedir = 1;
			}
		}
	}

	if (options &&
		zend_hash_find(Z_ARRVAL_P(options), "allow_url_fopen", sizeof("allow_url_fopen"), (void**)&tmpzval) == SUCCESS &&
		!zval_is_true(*tmpzval)) {
		/* Can be turned off from on, but not the other way around */
		disallow_url_fopen = 1;
	}

	if (options &&
		zend_hash_find(Z_ARRVAL_P(options), "disable_functions", sizeof("disable_functions"), (void**)&tmpzval) == SUCCESS &&
		Z_TYPE_PP(tmpzval) == IS_STRING) {
		/* NOTE: disable_functions doesn't prevent $obj->function_name, it only blocks code inside $obj->eval() statements
		 * This could be brought into consistency, but I actually think it's okay to leave those functions available to calling script
		 */

		/* This buffer needs to be around when the error message occurs since the underlying implementation in Zend expects it to be */
		disable_functions = estrndup(Z_STRVAL_PP(tmpzval), Z_STRLEN_PP(tmpzval));
		disable_functions_len = Z_STRLEN_PP(tmpzval);
	}

	if (options &&
		zend_hash_find(Z_ARRVAL_P(options), "disable_classes", sizeof("disable_classes"), (void**)&tmpzval) == SUCCESS &&
		Z_TYPE_PP(tmpzval) == IS_STRING) {

		disable_classes = estrndup(Z_STRVAL_PP(tmpzval), Z_STRLEN_PP(tmpzval));
		disable_classes_len = Z_STRLEN_PP(tmpzval);
	}

	if (options &&
		zend_hash_find(Z_ARRVAL_P(options), "runkit.superglobal", sizeof("runkit.superglobal"), (void**)&tmpzval) == SUCCESS &&
		Z_TYPE_PP(tmpzval) == IS_STRING) {

		superglobals = Z_STRVAL_PP(tmpzval);
		superglobals_len = Z_STRLEN_PP(tmpzval);
	}

	data = emalloc(sizeof(php_runkit_sandbox_data));
	data->context = tsrm_new_interpreter_context();
	data->disable_functions = disable_functions;
	data->disable_classes = disable_classes;
	data->output_handler = NULL;

	PHP_RUNKIT_SANDBOX_BEGIN(data);
		data->parent_context = prior_context;

		zend_alter_ini_entry("implicit_flush", sizeof("implicit_flush") , "1", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
		zend_alter_ini_entry("max_execution_time", sizeof("max_execution_time") , "0", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
		if (safe_mode) {
			zend_alter_ini_entry("safe_mode", sizeof("safe_mode"), "1", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
		}
		if (use_open_basedir) {
			zend_alter_ini_entry("open_basedir", sizeof("open_basedir"), open_basedir, open_basedir_len, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
		}
		if (disallow_url_fopen) {
			zend_alter_ini_entry("allow_url_fopen", sizeof("allow_url_fopen"), "0", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
		}
		if (disable_functions) {
			char *p, *s = disable_functions;

			while ((p = strchr(s, ','))) {
				*p = '\0';
				zend_disable_function(s, p - s TSRMLS_CC);
				s = p + 1;
			}
			zend_disable_function(s, strlen(s) TSRMLS_CC);
		}
		if (disable_classes) {
			char *p, *s = disable_classes;

			while ((p = strchr(s, ','))) {
				*p = '\0';
				zend_disable_class(s, p - s TSRMLS_CC);
				s = p + 1;
			}
			zend_disable_class(s, strlen(s) TSRMLS_CC);
		}
		if (superglobals) {
			char *p, *s = superglobals;
			int len;

			while ((p = strchr(s, ','))) {
				if (p - s) {
					*p = '\0';
					zend_register_auto_global(s, p - s, NULL TSRMLS_CC);
					zend_auto_global_disable_jit(s, p - s TSRMLS_CC);
					*p = ',';
				}
				s = p + 1;
			}
			len = strlen(s);
			zend_register_auto_global(s, len, NULL TSRMLS_CC);
			zend_auto_global_disable_jit(s, len TSRMLS_CC);
		}
		SG(headers_sent) = 1;
		SG(request_info).no_headers = 1;
		SG(server_context) = data;
		SG(options) = SAPI_OPTION_NO_CHDIR;
		php_request_startup(TSRMLS_C);
		PG(during_request_startup) = 0;
	PHP_RUNKIT_SANDBOX_END(data);

	ALLOC_INIT_ZVAL(zsandbox);
	ZEND_REGISTER_RESOURCE(zsandbox, data, le_sandbox_state);

	PHP_RUNKIT_SANDBOX_PROP(&sandbox_name, &sandbox_name_len);
	if (zend_hash_add(Z_OBJPROP_P(this_ptr), sandbox_name, sandbox_name_len + 1, &zsandbox, sizeof(zval*), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error adding sandbox state resource to object");
		pefree(sandbox_name, 0);
		zval_ptr_dtor(&zsandbox);
		efree(data);
		RETURN_FALSE;
	}
	pefree(sandbox_name, 0);

	/* Override standard handlers */
	Z_OBJ_HT_P(this_ptr) = &php_runkit_object_handlers;

	RETURN_TRUE;
}
/* }}} */	

/* {{{ le_sandbox_state destructor
 */
static void php_runkit_sandbox_state_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	void *prior_context;
	php_runkit_sandbox_data *data = (php_runkit_sandbox_data*)rsrc->ptr;

	if (data->disable_functions) {
		efree(data->disable_functions);
	}

	if (data->disable_classes) {
		efree(data->disable_classes);
	}

	prior_context = tsrm_set_interpreter_context(data->context);
	{
		TSRMLS_FETCH();
		php_request_shutdown(TSRMLS_C);
	}
	tsrm_set_interpreter_context(NULL);
	tsrm_free_interpreter_context(data->context);
	tsrm_set_interpreter_context(prior_context);

	if (data->output_handler) {
		zval_ptr_dtor(&data->output_handler);
	}
	efree(data);
}
/* }}} */

/* {{{ proto Runkit_Sandbox::__get(string member)
 */
PHP_METHOD(Runkit_Sandbox,__get)
{
	char *prop;
	int prop_len;
	php_runkit_sandbox_data *data;
	zval **value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &prop, &prop_len) == FAILURE) {
		RETURN_NULL();
	}

	PHP_RUNKIT_SANDBOX_FETCHBOX(Z_OBJPROP_P(this_ptr));

	PHP_RUNKIT_SANDBOX_BEGIN(data);
		if (zend_hash_find(&EG(symbol_table), prop, prop_len + 1, (void**)&value) == FAILURE) {
			PHP_RUNKIT_SANDBOX_ABORT(data);
			RETURN_NULL();
		}
	PHP_RUNKIT_SANDBOX_END(data);

	*return_value = **value;
	PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(return_value);
}
/* }}} */

/* {{{ proto Runkit_Sandbox::__set(string member, mixed value)
 */
PHP_METHOD(Runkit_Sandbox,__set)
{
	char *prop;
	int prop_len;
	php_runkit_sandbox_data *data;
	zval *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &prop, &prop_len, &value) == FAILURE) {
		RETURN_NULL();
	}

	PHP_RUNKIT_SANDBOX_FETCHBOX(Z_OBJPROP_P(this_ptr));

	PHP_RUNKIT_SANDBOX_BEGIN(data);
	{
		zval *copyval;

		MAKE_STD_ZVAL(copyval);
		*copyval = *value;
		PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(copyval);
		ZEND_SET_SYMBOL(&EG(symbol_table), prop, copyval);
	}
	PHP_RUNKIT_SANDBOX_END(data);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto Runkit_Sandbox::__call(mixed function_name, array args)
	Call User Function */
PHP_METHOD(Runkit_Sandbox,__call)
{
	zval *func_name, *args, *retval = NULL;
	php_runkit_sandbox_data *data;
	int bailed_out = 0, argc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "za", &func_name, &args) == FAILURE) {
		RETURN_NULL();
	}

	PHP_RUNKIT_SANDBOX_FETCHBOX(Z_OBJPROP_P(this_ptr));
	argc = zend_hash_num_elements(Z_ARRVAL_P(args));

	PHP_RUNKIT_SANDBOX_BEGIN(data);
	{
		zval ***sandbox_args;
		char *name;
		int i;

		zend_first_try {
			HashPosition pos;
			zval **tmpzval;

			if (!zend_is_callable(func_name, IS_CALLABLE_CHECK_NO_ACCESS, &name)) {
				php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "First argument is expected to be a valid callback");
				efree(name);
				PHP_RUNKIT_SANDBOX_ABORT(data);
				RETURN_FALSE;
			}

			sandbox_args = safe_emalloc(sizeof(zval**), argc, 0);
			for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(args), &pos), i = 0;
				(zend_hash_get_current_data_ex(Z_ARRVAL_P(args), (void**)&tmpzval, &pos) == SUCCESS) && (i < argc);
				zend_hash_move_forward_ex(Z_ARRVAL_P(args), &pos), i++) {
				sandbox_args[i] = emalloc(sizeof(zval*));
				MAKE_STD_ZVAL(*sandbox_args[i]);
				**sandbox_args[i] = **tmpzval;
				PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(*sandbox_args[i]);
			}
			/* Shouldn't be necessary */
			argc = i;

			/* Note: If this function is disabled by disable_functions or disable_classes,
			 * The user will get a confusing error message about (null)() being disabled for security reasons on line 0
			 * This will be fixable with a properly set EG(function_state_ptr)....just not yet
			 */
			if (call_user_function_ex(EG(function_table), NULL, func_name, &retval, argc, sandbox_args, 0, NULL TSRMLS_CC) == SUCCESS) {
				if (retval) {
					*return_value = *retval;
				} else {
					RETVAL_TRUE;
				}
			} else {
				php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Unable to call function");
				RETVAL_FALSE;
			}
			efree(name);

			for(i = 0; i < argc; i++) {
				zval_ptr_dtor(sandbox_args[i]);
				efree(sandbox_args[i]);
			}
			efree(sandbox_args);
		} zend_catch {
			bailed_out = 1;
		} zend_end_try();
	}
	PHP_RUNKIT_SANDBOX_END(data);

	if (bailed_out) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed calling sandbox function");
		RETURN_FALSE;
	}

	PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(return_value);

	if (retval) {
		PHP_RUNKIT_SANDBOX_BEGIN(data);
		zval_ptr_dtor(&retval);
		PHP_RUNKIT_SANDBOX_END(data);
	}
}
/* }}} */

/* {{{ php_runkit_sandbox_include_or_eval
 */
static void php_runkit_sandbox_include_or_eval(INTERNAL_FUNCTION_PARAMETERS, int type, int once)
{
	int code_len;
	php_runkit_sandbox_data *data;
	zval *zcode;
	int bailed_out = 0;
	zval *retval = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zcode) == FAILURE) {
		RETURN_FALSE;
	}

	convert_to_string(zcode);

	PHP_RUNKIT_SANDBOX_FETCHBOX(Z_OBJPROP_P(this_ptr));

	RETVAL_NULL();

	PHP_RUNKIT_SANDBOX_BEGIN(data);
		zend_first_try {
			zend_op_array *op_array = NULL;

			if (type == ZEND_EVAL) {
				/* eval() */
				char *eval_desc = zend_make_compiled_string_description("Runkit_Sandbox Eval Code" TSRMLS_CC);
				op_array = compile_string(zcode, eval_desc TSRMLS_CC);
				efree(eval_desc);
			} else if (!once) {
				/* include() & requre() */
				op_array = compile_filename(type, zcode TSRMLS_CC);
			} else {
				/* include_once() & require_once() */
				int dummy = 1;
				zend_file_handle file_handle;

				if (SUCCESS == zend_stream_open(Z_STRVAL_P(zcode), &file_handle TSRMLS_CC)) {
					if (!file_handle.opened_path) {
						file_handle.opened_path = estrndup(Z_STRVAL_P(zcode), Z_STRLEN_P(zcode));
					}
					if (zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy, sizeof(int), NULL)==SUCCESS) {
						op_array = zend_compile_file(&file_handle, type TSRMLS_CC);
						zend_destroy_file_handle(&file_handle TSRMLS_CC);
					} else {
						zend_file_handle_dtor(&file_handle);
						RETVAL_TRUE;
					}
				}
			}

			if (op_array) {
				EG(return_value_ptr_ptr) = &retval;
				EG(active_op_array) = op_array;

				zend_execute(op_array TSRMLS_CC);

				if (retval) {
					*return_value = *retval;
				} else {
					RETVAL_TRUE;
				}

				destroy_op_array(op_array TSRMLS_CC);
				efree(op_array);
			}
		} zend_catch {
			bailed_out = 1;
		} zend_end_try();
	PHP_RUNKIT_SANDBOX_END(data);

	if (bailed_out) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error executing sandbox code");
		RETURN_FALSE;
	}

	PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(return_value);

	/* Don't confuse the memory manager */
	if (retval) {
		PHP_RUNKIT_SANDBOX_BEGIN(data);
		zval_ptr_dtor(&retval);
		PHP_RUNKIT_SANDBOX_END(data);
	}
}
/* }}} */

/* {{{ proto Runkit_Sandbox::eval(string phpcode)
	Evaluate php code within the sandbox environment */
PHP_METHOD(Runkit_Sandbox,eval)
{
	php_runkit_sandbox_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_EVAL, 0);
}
/* }}} */

/* {{{ proto Runkit_Sandbox::include(string filename)
	Evaluate php code from a file within the sandbox environment */
PHP_METHOD(Runkit_Sandbox,include)
{
	php_runkit_sandbox_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_INCLUDE, 0);
}
/* }}} */

/* {{{ proto Runkit_Sandbox::include_once(string filename)
	Evaluate php code from a file within the sandbox environment */
PHP_METHOD(Runkit_Sandbox,include_once)
{
	php_runkit_sandbox_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_INCLUDE, 1);
}
/* }}} */

/* {{{ proto Runkit_Sandbox::require(string filename)
	Evaluate php code from a file within the sandbox environment */
PHP_METHOD(Runkit_Sandbox,require)
{
	php_runkit_sandbox_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_REQUIRE, 0);
}
/* }}} */

/* {{{ proto Runkit_Sandbox::require_once(string filename)
	Evaluate php code from a file within the sandbox environment */
PHP_METHOD(Runkit_Sandbox,require_once)
{
	php_runkit_sandbox_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_REQUIRE, 1);
}
/* }}} */

/* {{{ proto null Runkit_Sandbox::echo(mixed var[, mixed var[, ... ]])
	Echo through the sandbox
	The only thing which distinguished this from a non-sandboxed echo
	is that content gets processed through the sandbox's output_handler (if present) */
PHP_METHOD(Runkit_Sandbox,echo)
{
	php_runkit_sandbox_data *data;
	zval **argv;
	int i, argc = ZEND_NUM_ARGS();

	if (zend_get_parameters_array_ex(argc, &argv) == FAILURE) {
		/* Big problems... */
		RETURN_NULL();
	}

	for(i = 0; i < argc; i++) {
		/* Prepare for output */
		convert_to_string(argv[i]);
	}

	PHP_RUNKIT_SANDBOX_FETCHBOX(Z_OBJPROP_P(this_ptr));

	PHP_RUNKIT_SANDBOX_BEGIN(data);
		zend_try {
			for(i = 0; i < argc; i++) {
				PHPWRITE(Z_STRVAL_P(argv[i]),Z_STRLEN_P(argv[i]));
			}
		} zend_catch {
		} zend_end_try();
	PHP_RUNKIT_SANDBOX_END(data);	

	RETURN_NULL();
}
/* }}} */

/* {{{ proto bool Runkit_Sandbox::print(mixed var)
	Echo through the sandbox
	The only thing which distinguished this from a non-sandboxed echo
	is that content gets processed through the sandbox's output_handler (if present) */
PHP_METHOD(Runkit_Sandbox,print)
{
	php_runkit_sandbox_data *data;
	char *str;
	int len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &len) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_RUNKIT_SANDBOX_FETCHBOX(Z_OBJPROP_P(this_ptr));

	PHP_RUNKIT_SANDBOX_BEGIN(data);
		zend_try {
			PHPWRITE(str,len);
		} zend_catch {
		} zend_end_try();
	PHP_RUNKIT_SANDBOX_END(data);	

	RETURN_BOOL(len > 1 || (len == 1 && str[0] != '0'));
}
/* }}} */

/* *******************
   * Object Handlers *
   ******************* */

#define PHP_RUNKIT_SANDBOX_FETCHBOX_HANDLER \
{ \
	char *sandbox_name; \
	int sandbox_name_len; \
	zval **resptr; \
	PHP_RUNKIT_SANDBOX_PROP(&sandbox_name, &sandbox_name_len); \
	if (zend_hash_find(Z_OBJPROP_P(object), sandbox_name, sandbox_name_len + 1, (void**)&resptr) == FAILURE) { \
		data = NULL; \
	} else { \
		data = (php_runkit_sandbox_data*)zend_fetch_resource(resptr TSRMLS_CC, -1, PHP_RUNKIT_SANDBOX_RESNAME, NULL, 1, le_sandbox_state); \
	} \
	efree(sandbox_name); \
}

/* {{{ php_runkit_sandbox_has_property
	has_property handler */
static int php_runkit_sandbox_has_property(zval *object, zval *member, int has_set_exists TSRMLS_DC)
{
	php_runkit_sandbox_data* data;
	zval member_copy;
	int result;

#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 1
	/* Map PHP 5.0 has_property flag to PHP 5.1+ flag */
	has_set_exists = (has_set_exists == 0) ? 2 : 1;
#endif

	PHP_RUNKIT_SANDBOX_FETCHBOX_HANDLER;
	if (!data) {
		return 0;
	}

	if (Z_TYPE_P(member) != IS_STRING) {
		member_copy = *member;
		member = &member_copy;
		zval_copy_ctor(member);
		member->refcount = 1;
		convert_to_string(member);
	}

	PHP_RUNKIT_SANDBOX_BEGIN(data);
	{
		zval **tmpzval;

		if (zend_hash_find(&EG(symbol_table), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, (void**)&tmpzval) == SUCCESS) {
			switch (has_set_exists) {
				case 0:
					result = (Z_TYPE_PP(tmpzval) != IS_NULL);
					break;
				case 1:
					result = zend_is_true(*tmpzval);
					break;
				case 2:
					result = 1;
					break;
			}
		} else {
			result = 0;
		}
	}
	PHP_RUNKIT_SANDBOX_END(data);

	if (member == &member_copy) {
		zval_dtor(member);
	}

	return result;
}
/* }}} */

/* {{{ php_runkit_sandbox_unset_property
	unset_property handler */
static void php_runkit_sandbox_unset_property(zval *object, zval *member TSRMLS_DC)
{
	php_runkit_sandbox_data* data;
	zval member_copy;

	PHP_RUNKIT_SANDBOX_FETCHBOX_HANDLER;
	if (!data) {
		return;
	}

	if (Z_TYPE_P(member) != IS_STRING) {
		member_copy = *member;
		member = &member_copy;
		zval_copy_ctor(member);
		member->refcount = 1;
		convert_to_string(member);
	}

	PHP_RUNKIT_SANDBOX_BEGIN(data);
		zend_hash_del(&EG(symbol_table), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1);
	PHP_RUNKIT_SANDBOX_END(data);

	if (member == &member_copy) {
		zval_dtor(member);
	}
}
/* }}} */

/* ******************
   * Object Support *
   ****************** */

/* Original write function */
static int (*php_runkit_sandbox_sapi_ub_write)(const char *str, uint str_length TSRMLS_DC);

/* {{{ php_runkit_sandbox_body_write
 * Wrap the caller's output handler with a mechanism for switching contexts
 */
static int php_runkit_sandbox_body_write(const char *str, uint str_length TSRMLS_DC)
{
	php_runkit_sandbox_data *data = RUNKIT_G(current_sandbox);
	int bytes_written;

	if (!str_length) {
		return 0;
	}

	if (!data) {
		/* Not in a sandbox -- Use genuine sapi.ub_write handler */
		return php_runkit_sandbox_sapi_ub_write(str, str_length TSRMLS_CC);
	}

	tsrm_set_interpreter_context(data->parent_context);
	{
		char *name = NULL;
		zval **output_buffer[1], *bufcopy, *retval = NULL;
		TSRMLS_FETCH();

		if (!data->output_handler ||
			!zend_is_callable(data->output_handler, IS_CALLABLE_CHECK_NO_ACCESS, &name)) {
			/* No hander, or invalid handler, pass up the line... */
			bytes_written = PHPWRITE(str, str_length);

			tsrm_set_interpreter_context(data->context);
	
			return bytes_written;
		}

		MAKE_STD_ZVAL(bufcopy);
		ZVAL_STRINGL(bufcopy, (char*)str, str_length, 1);
		output_buffer[0] = &bufcopy;

		if (call_user_function_ex(EG(function_table), NULL, data->output_handler, &retval, 1, output_buffer, 0, NULL TSRMLS_CC) == SUCCESS) {
			if (retval) {
				convert_to_string(retval);
				bytes_written = PHPWRITE(Z_STRVAL_P(retval), Z_STRLEN_P(retval));
				zval_ptr_dtor(&retval);
			} else {
				bytes_written = 0;
			}
		} else {
			php_error_docref("runkit.sandbox" TSRMLS_CC, E_WARNING, "Unable to call output buffer callback");
			bytes_written = 0;
		}

		if (bufcopy) {
			zval_ptr_dtor(&bufcopy);
		}
	}
	tsrm_set_interpreter_context(data->context);

	return bytes_written;	
}
/* }}} */

/* {{{ proto mixed runkit_sandbox_output_handler(Runkit_Sandbox sandbox[, mixed callback])
	Returns the output handler which was active prior to calling this method,
	or false if no output handler is active

	If no callback is passed, the current output handler is not changed
	If a non-true output handler is passed(NULL,false,0,0.0,'',array()), output handling is turned off

	If an error occurs (such as callback is not callable), NULL is returned */
PHP_FUNCTION(runkit_sandbox_output_handler)
{
	zval *sandbox;
	zval *callback = NULL;
	php_runkit_sandbox_data *data;
	char *name;
	int callback_is_true = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &sandbox, php_runkit_sandbox_class_entry, &callback) == FAILURE) {
		RETURN_NULL();
	}

	PHP_RUNKIT_SANDBOX_FETCHBOX(Z_OBJPROP_P(sandbox));	

	if (callback) {
		zval callback_copy = *callback;

		zval_copy_ctor(&callback_copy);
		callback_copy.is_ref = 0;
		callback_copy.refcount = 1;
		callback_is_true = zval_is_true(&callback_copy);
		zval_dtor(&callback_copy);
	}

	if (callback && callback_is_true &&
		!zend_is_callable(callback, IS_CALLABLE_CHECK_NO_ACCESS, &name)) {
		php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Second argument (%s) is expected to be a valid callback", name);
		efree(name);
		RETURN_FALSE;
	}

	if (data->output_handler && return_value_used) {
		*return_value = *data->output_handler;
		zval_copy_ctor(return_value);
		return_value->refcount = 1;
		return_value->is_ref = 0;
	} else {
		RETVAL_FALSE;
	}

	if (!callback) {
		return;
	}

	if (data->output_handler) {
		zval_ptr_dtor(&data->output_handler);
		data->output_handler = NULL;
	}

	if (callback && callback_is_true) {
		zval *cb = callback;
		if (callback->is_ref) {
			MAKE_STD_ZVAL(cb);
			*cb = *callback;
			zval_copy_ctor(cb);
			cb->refcount = 0;
			cb->is_ref = 0;
		}
		cb->refcount++;
		data->output_handler = cb;
	}
}

/* ********************
   * Class Definition *
   ******************** */

ZEND_BEGIN_ARG_INFO_EX(php_runkit_require_one_arg, 0, 0, 1)
	ZEND_ARG_PASS_INFO(0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(php_runkit_require_two_args, 0, 0, 2)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
ZEND_END_ARG_INFO()

static function_entry php_runkit_sandbox_functions[] = {
	PHP_ME(Runkit_Sandbox,	__construct,					NULL,								ZEND_ACC_PUBLIC		| ZEND_ACC_CTOR)
	PHP_ME(Runkit_Sandbox,	__get,							php_runkit_require_one_arg,			ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	__set,							php_runkit_require_two_args,		ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	__call,							php_runkit_require_two_args,		ZEND_ACC_PUBLIC)
	/* Language Constructs */
	PHP_ME(Runkit_Sandbox,	eval,							NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	include,						NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	include_once,					NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	require,						NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	require_once,					NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	echo,							NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox,	print,							NULL,								ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

int php_runkit_init_sandbox(INIT_FUNC_ARGS)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, PHP_RUNKIT_SANDBOX_CLASSNAME, php_runkit_sandbox_functions);
	php_runkit_sandbox_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	le_sandbox_state = zend_register_list_destructors_ex(php_runkit_sandbox_state_dtor, NULL, PHP_RUNKIT_SANDBOX_RESNAME, module_number);

	/* Make a new object handler struct with a couple minor changes */
	memcpy(&php_runkit_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_runkit_object_handlers.has_property				= php_runkit_sandbox_has_property;
	php_runkit_object_handlers.unset_property			= php_runkit_sandbox_unset_property;

	php_runkit_sandbox_sapi_ub_write = sapi_module.ub_write;
	sapi_module.ub_write = php_runkit_sandbox_body_write;

	return SUCCESS;
}

int php_runkit_shutdown_sandbox(SHUTDOWN_FUNC_ARGS)
{
	sapi_module.ub_write = php_runkit_sandbox_sapi_ub_write;

	return SUCCESS;
}

/* ***********************
   * Lint Implementation *
   *********************** */

/* {{{ php_runkit_lint_compile
	Central helper function for runkit_lint() and runkit_lint_file() */
static void php_runkit_lint_compile(INTERNAL_FUNCTION_PARAMETERS, int filemode)
{
	void *context, *prior_context;
	zval *zcode;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zcode) == FAILURE) {
		RETURN_FALSE;
	}

	convert_to_string(zcode);

	context = tsrm_new_interpreter_context();
	prior_context = tsrm_set_interpreter_context(context);
	{
		TSRMLS_FETCH();

		php_request_startup(TSRMLS_C);
		PG(during_request_startup) = 0;

		zend_first_try {
			char *eval_desc;
			zend_op_array *op_array;

			if (filemode) {
				op_array = compile_filename(ZEND_INCLUDE, zcode TSRMLS_CC);
			} else {
				eval_desc = zend_make_compiled_string_description("runkit_lint test compile" TSRMLS_CC);
				op_array = compile_string(zcode, eval_desc TSRMLS_CC);
				efree(eval_desc);
			}

			if (op_array)  {
				RETVAL_TRUE;
				destroy_op_array(op_array TSRMLS_CC);
				efree(op_array);
			} else {
				RETVAL_FALSE;
			}
		} zend_catch {
			RETVAL_FALSE;
		} zend_end_try();

		php_request_shutdown(TSRMLS_C);
	}
	tsrm_set_interpreter_context(NULL);
	tsrm_free_interpreter_context(context);
	tsrm_set_interpreter_context(prior_context);
}
/* }}} */

/* {{{ proto bool runkit_lint(string code)
	Attempts to compile a string of code within a sub-interpreter */
PHP_FUNCTION(runkit_lint)
{
	php_runkit_lint_compile(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto bool runkit_lint_file(string filename)
	Attempts to compile a file within a sub-interpreter */
PHP_FUNCTION(runkit_lint_file)
{
	php_runkit_lint_compile(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

#endif /* PHP_RUNKIT_SANDBOX */

