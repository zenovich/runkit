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

#ifdef PHP_RUNKIT_SANDBOX

static zend_object_handlers php_runkit_sandbox_parent_handlers;
static zend_class_entry *php_runkit_sandbox_parent_entry;

typedef struct _php_runkit_sandbox_parent_object {
	zend_object obj;
	/* This object is for referring to the parent,
	 * internally it tracks itself
	 */
	php_runkit_sandbox_object *self;
} php_runkit_sandbox_parent_object;

#define PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval) \
{ \
	void *prior_context = tsrm_set_interpreter_context(objval->self->parent_context); \
	TSRMLS_FETCH(); \
	zend_try {

#define PHP_RUNKIT_SANDBOX_PARENT_ABORT(objval) \
{ \
	tsrm_set_interpreter_context(prior_context); \
}

#define PHP_RUNKIT_SANDBOX_PARENT_END(objval) \
	} zend_catch { \
		objval->self->bailed_out_in_eval = 1; \
	} zend_end_try(); \
	PHP_RUNKIT_SANDBOX_PARENT_ABORT(objval) \
	if (objval->self->bailed_out_in_eval) { \
		/* If the parent is dying, so are we */ \
		zend_bailout(); \
	} \
}

#define PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(zval_p) (php_runkit_sandbox_parent_object*)zend_objects_get_address(zval_p TSRMLS_CC)
#define PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX_VERIFY_ACCESS(objval, pzv) \
{ \
	objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(pzv); \
\
	if (!objval) { \
		/* Should never ever happen.... */ \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "HELP! HELP! MY PARENT HAS ABANDONED ME!"); \
		RETURN_FALSE; \
	} \
\
	if (!objval->self->parent_access) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to the parent has been suspended"); \
		RETURN_FALSE; \
	} \
}

static HashTable *php_runkit_sandbox_parent_resolve_symbol_table(php_runkit_sandbox_parent_object *objval TSRMLS_DC)
{
	int i;
	zend_execute_data *ex = EG(current_execute_data);

	if (objval->self->parent_scope <= 0) {
		HashTable *ht = &EG(symbol_table);

		if (objval->self->parent_scope_name) {
			zval **symtable;

			if (zend_hash_find(ht, objval->self->parent_scope_name, objval->self->parent_scope_namelen + 1, (void**)&symtable) == SUCCESS) {
				if (Z_TYPE_PP(symtable) == IS_ARRAY) {
					ht = Z_ARRVAL_PP(symtable);
				} else {
					/* Variable exists but is not an array,
					 * Make a dummy array that contains this var */
					zval *hidden;

					ALLOC_INIT_ZVAL(hidden);
					array_init(hidden);
					ht = Z_ARRVAL_P(hidden);
					zend_hash_update(ht, objval->self->parent_scope_name, objval->self->parent_scope_namelen + 1, (void*)symtable, sizeof(zval*), NULL);

					/* Store that dummy array in the sandbox's hidden properties table so that it gets cleaned up on dtor */
					zend_hash_update(objval->obj.properties, "scope", sizeof("scope"), (void*)&hidden, sizeof(zval*), NULL);
				}
			} else {
				/* Create variable as an array */
				zval *newval;

				ALLOC_INIT_ZVAL(newval);
				array_init(newval);
				zend_hash_update(&EG(symbol_table), objval->self->parent_scope_name, objval->self->parent_scope_namelen + 1, (void*)&newval, sizeof(zval*), NULL);
				ht = Z_ARRVAL_P(newval);
			}
		}

		return ht;
	}
	if (objval->self->parent_scope == 1) {
		return EG(active_symbol_table);
	}

	for(i = 1; i < objval->self->parent_scope; i++) {
		if (!ex->prev_execute_data) {
			/* Symbol table exceeded */
			return &EG(symbol_table);
		}
		ex = ex->prev_execute_data;
	}

	return ex->symbol_table ? ex->symbol_table : &EG(symbol_table);
}

/* {{{ proto Runkit_Sandbox_Parent::__call(mixed function_name, array args)
	Call User Function */
PHP_METHOD(Runkit_Sandbox_Parent,__call)
{
	zval *func_name, *args, *retval = NULL;
	php_runkit_sandbox_parent_object *objval;
	int argc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "za", &func_name, &args) == FAILURE) {
		RETURN_NULL();
	}

	PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX_VERIFY_ACCESS(objval, this_ptr);
	if (!objval->self->parent_call) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to call functions in the parent context is not enabled");
		RETURN_FALSE;
	}

	argc = zend_hash_num_elements(Z_ARRVAL_P(args));

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
	{
		HashPosition pos;
		zval **tmpzval, ***sandbox_args;
		char *name = NULL;
		int i;


		if (!zend_is_callable(func_name, IS_CALLABLE_CHECK_NO_ACCESS, &name)) {
			php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Function not defined");
			if (name) {
				efree(name);
			}
			PHP_RUNKIT_SANDBOX_PARENT_ABORT(objval)
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
		if (name) {
			efree(name);
		}

		for(i = 0; i < argc; i++) {
			zval_ptr_dtor(sandbox_args[i]);
			efree(sandbox_args[i]);
		}
		efree(sandbox_args);
	}
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(return_value);

	if (retval) {
		PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		zval_ptr_dtor(&retval);
		PHP_RUNKIT_SANDBOX_PARENT_END(objval)
	}
}
/* }}} */

/* {{{ php_runkit_sandbox_parent_include_or_eval
	What's the point of running in a sandbox if you can leave whenever you want to???
 */
static void php_runkit_sandbox_parent_include_or_eval(INTERNAL_FUNCTION_PARAMETERS, int type, int once)
{
	php_runkit_sandbox_parent_object *objval;
	zval *zcode;
	int bailed_out = 0;
	zval *retval = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zcode) == FAILURE) {
		RETURN_FALSE;
	}

	convert_to_string(zcode);

	PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX_VERIFY_ACCESS(objval, this_ptr);
	if (type == ZEND_EVAL && !objval->self->parent_eval) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to eval() code in the parent context is not enabled");
		RETURN_FALSE;
	}
	if (type != ZEND_EVAL && !objval->self->parent_include) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to include()/include_once()/require()/require_once() in the parent context is not enabled");
		RETURN_FALSE;
	}

	RETVAL_NULL();

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		zend_op_array *op_array = NULL;
		int already_included = 0;

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
					already_included = 1;
				}
			}
		}

		if (op_array) {
			HashTable *old_symbol_table = EG(active_symbol_table);
			zval **orig_retvalpp = EG(return_value_ptr_ptr);
			zend_op_array *orig_act_oparray = EG(active_op_array);

			EG(return_value_ptr_ptr) = &retval;
			EG(active_op_array) = op_array;
			EG(active_symbol_table) = php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC);

			zend_execute(op_array TSRMLS_CC);

			if (retval) {
				*return_value = *retval;
			} else {
				RETVAL_TRUE;
			}

			destroy_op_array(op_array TSRMLS_CC);
			efree(op_array);

			EG(return_value_ptr_ptr) = orig_retvalpp;
			EG(active_op_array) = orig_act_oparray;
			EG(active_symbol_table) = old_symbol_table;
		} else if ((type != ZEND_INCLUDE) && !already_included) {
			/* include can fail to parse peacefully,
			 * require and eval should die on failure
			 */
			bailed_out = 1;
		}
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (bailed_out) {
		CG(unclean_shutdown) = 1;
		CG(in_compilation) = EG(in_execution) = 0;
		EG(current_execute_data) = NULL;
		PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
			zend_bailout();
		PHP_RUNKIT_SANDBOX_PARENT_END(objval)
	}

	PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(return_value);

	/* Don't confuse the memory manager */
	if (retval) {
		PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		zval_ptr_dtor(&retval);
		PHP_RUNKIT_SANDBOX_PARENT_END(objval)
	}
}
/* }}} */

/* {{{ proto Runkit_Sandbox_Parent::eval(string phpcode)
	Evaluate php code within the parent */
PHP_METHOD(Runkit_Sandbox_Parent,eval)
{
	php_runkit_sandbox_parent_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_EVAL, 0);
}
/* }}} */

/* {{{ proto Runkit_Sandbox_Parent::include(string filename)
	Evaluate php code from a file within the parent */
PHP_METHOD(Runkit_Sandbox_Parent,include)
{
	php_runkit_sandbox_parent_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_INCLUDE, 0);
}
/* }}} */

/* {{{ proto Runkit_Sandbox_Parent::include_once(string filename)
	Evaluate php code from a file within the sandbox environment */
PHP_METHOD(Runkit_Sandbox_Parent,include_once)
{
	php_runkit_sandbox_parent_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_INCLUDE, 1);
}
/* }}} */

/* {{{ proto Runkit_Sandbox_Parent::require(string filename)
	Evaluate php code from a file within the sandbox environment */
PHP_METHOD(Runkit_Sandbox_Parent,require)
{
	php_runkit_sandbox_parent_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_REQUIRE, 0);
}
/* }}} */

/* {{{ proto Runkit_Sandbox_Parent::require_once(string filename)
	Evaluate php code from a file within the sandbox environment */
PHP_METHOD(Runkit_Sandbox_Parent,require_once)
{
	php_runkit_sandbox_parent_include_or_eval(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_REQUIRE, 1);
}
/* }}} */

/* {{{ proto null Runkit_Sandbox_Parent::echo(mixed var[, mixed var[, ... ]])
	Echo through out of the sandbox
	Avoid the sandbox's output handler */
PHP_METHOD(Runkit_Sandbox_Parent,echo)
{
	php_runkit_sandbox_parent_object *objval;
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

	PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX_VERIFY_ACCESS(objval, this_ptr);
	if (!objval->self->parent_echo) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to echo data in the parent context is not enabled");
		RETURN_FALSE;
	}

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		for(i = 0; i < argc; i++) {
			PHPWRITE(Z_STRVAL_P(argv[i]),Z_STRLEN_P(argv[i]));
		}
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	RETURN_NULL();
}
/* }}} */

/* {{{ proto bool Runkit_Sandbox_Parent::print(mixed var)
	Echo through the sandbox
	Avoid the sandbox's output_handler */
PHP_METHOD(Runkit_Sandbox_Parent,print)
{
	php_runkit_sandbox_parent_object *objval;
	char *str;
	int len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &len) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX_VERIFY_ACCESS(objval, this_ptr);
	if (!objval->self->parent_echo) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to echo data in the parent context is not enabled");
		RETURN_FALSE;
	}

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		PHPWRITE(str,len);
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	RETURN_BOOL(len > 1 || (len == 1 && str[0] != '0'));
}
/* }}} */

/* {{{ proto void Runkit_Sandbox_Parent::die(mixed message)
	MALIAS(exit)
	PATRICIDE!!!!!!!! */
PHP_METHOD(Runkit_Sandbox_Parent,die)
{
	php_runkit_sandbox_parent_object *objval;
	zval *message = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &message) == FAILURE) {
		RETURN_FALSE;
	}

	RETVAL_NULL();

	if (message && Z_TYPE_P(message) != IS_LONG) {
		convert_to_string(message);
	}

	PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX_VERIFY_ACCESS(objval, this_ptr);
	if (!objval->self->parent_die) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Patricide is disabled.  Shame on you Oedipus.");
		/* Sent as a warning, but we'll really implement it as an E_ERROR */
		objval->self->active = 0;
		RETURN_FALSE;
	}

	CG(unclean_shutdown) = 1;
	CG(in_compilation) = EG(in_execution) = 0;
	EG(current_execute_data) = NULL;

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		if (message) {
			if (Z_TYPE_P(message) == IS_LONG) {
				EG(exit_status) = Z_LVAL_P(message);
			} else {
				PHPWRITE(Z_STRVAL_P(message), Z_STRLEN_P(message));
			}
		}
		zend_bailout();
		/* More of a murder-suicide really... */
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)
}
/* }}} */

/* *********************
   * Property Handlers *
   ********************* */

/* {{{ php_runkit_sandbox_parent_read_property
	read_property handler */
static zval *php_runkit_sandbox_parent_read_property(zval *object, zval *member, int type TSRMLS_DC)
{
	php_runkit_sandbox_parent_object *objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(object);
	zval *tmp_member = NULL;
	zval retval;
	int prop_found = 0;

	if (!objval) {
		return EG(uninitialized_zval_ptr);
	}
	if (!objval->self->parent_access || !objval->self->parent_read) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to read parent's symbol table is disallowed");
		return EG(uninitialized_zval_ptr);
	}

	if (Z_TYPE_P(member) != IS_STRING) {
		ALLOC_ZVAL(tmp_member);
		*tmp_member = *member;
		INIT_PZVAL(tmp_member);
		zval_copy_ctor(tmp_member);
		convert_to_string(tmp_member);
		member = tmp_member;
	}

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		zval **value;

		if (zend_hash_find(php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, (void**)&value) == SUCCESS) {
			retval = **value;
			prop_found = 1;				
		}
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (tmp_member) {
		zval_ptr_dtor(&tmp_member);
	}

	if (prop_found) {
		zval *return_value;

		ALLOC_ZVAL(return_value);
		*return_value = retval;

		/* ZE expects refcount == 0 for unowned values */
		INIT_PZVAL(return_value);
		PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(return_value);
		return_value->refcount--;

		return return_value;
	} else {
		return EG(uninitialized_zval_ptr);
	}
}
/* }}} */

/* {{{ php_runkit_sandbox_parent_write_property
	write_property handler */
static void php_runkit_sandbox_parent_write_property(zval *object, zval *member, zval *value TSRMLS_DC)
{
	php_runkit_sandbox_parent_object *objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(object);
	zval *tmp_member = NULL;

	if (!objval) {
		return;
	}
	if (!objval->self->parent_access || !objval->self->parent_write) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to modify parent's symbol table is disallowed");
		return;
	}

	if (Z_TYPE_P(member) != IS_STRING) {
		ALLOC_ZVAL(tmp_member);
		*tmp_member = *member;
		INIT_PZVAL(tmp_member);
		zval_copy_ctor(tmp_member);
		convert_to_string(tmp_member);
		member = tmp_member;
	}

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		zval *copyval;

		MAKE_STD_ZVAL(copyval);
		*copyval = *value;
		PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(copyval);
		ZEND_SET_SYMBOL(php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC), Z_STRVAL_P(member), copyval);
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (tmp_member) {
		zval_ptr_dtor(&tmp_member);
	}
}
/* }}} */

/* {{{ php_runkit_sandbox_parent_has_property
	has_property handler */
static int php_runkit_sandbox_parent_has_property(zval *object, zval *member, int has_set_exists TSRMLS_DC)
{
	php_runkit_sandbox_parent_object* objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(object);
	zval member_copy;
	int result = 0;

#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 1
	/* Map PHP 5.0 has_property flag to PHP 5.1+ flag */
	has_set_exists = (has_set_exists == 0) ? 2 : 1;
#endif

	if (!objval) {
		return 0;
	}
	if (!objval->self->parent_access || !objval->self->parent_read) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to read parent's symbol table is disallowed");
		return 0;
	}

	if (Z_TYPE_P(member) != IS_STRING) {
		member_copy = *member;
		member = &member_copy;
		zval_copy_ctor(member);
		member->refcount = 1;
		convert_to_string(member);
	}

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
	{
		zval **tmpzval;

		if (zend_hash_find(php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, (void**)&tmpzval) == SUCCESS) {
			switch (has_set_exists) {
				case 0:
					result = (Z_TYPE_PP(tmpzval) != IS_NULL);
					break;
				case 1:
					switch (Z_TYPE_PP(tmpzval)) {
						case IS_BOOL: case IS_LONG: case IS_RESOURCE:
							result = (Z_LVAL_PP(tmpzval) != 0);
							break;
						case IS_DOUBLE:
							result = (Z_DVAL_PP(tmpzval) != 0);
							break;
						case IS_STRING:
							result = (Z_STRLEN_PP(tmpzval) > 1 || (Z_STRLEN_PP(tmpzval) == 1 && Z_STRVAL_PP(tmpzval)[0] != '0'));
							break;
						case IS_ARRAY:
							result = zend_hash_num_elements(Z_ARRVAL_PP(tmpzval)) > 0;
							break;
						case IS_OBJECT:
							/* TODO: Use ZE2 logic for this rather than ZE1 logic */
							result = zend_hash_num_elements(Z_OBJPROP_PP(tmpzval)) > 0;
							break;
						case IS_NULL:
						default:
							result = 0;
					}
					break;
				case 2:
					result = 1;
					break;
			}
		} else {
			result = 0;
		}
	}
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (member == &member_copy) {
		zval_dtor(member);
	}

	return result;
}
/* }}} */

/* {{{ php_runkit_sandbox_parent_unset_property
	unset_property handler */
static void php_runkit_sandbox_parent_unset_property(zval *object, zval *member TSRMLS_DC)
{
	php_runkit_sandbox_parent_object *objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(object);
	zval member_copy;

	if (!objval) {
		return;
	}
	if (!objval->self->parent_access || !objval->self->parent_write) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to modify parent's symbol table is disallowed");
		return;
	}

	if (Z_TYPE_P(member) != IS_STRING) {
		member_copy = *member;
		member = &member_copy;
		zval_copy_ctor(member);
		member->refcount = 1;
		convert_to_string(member);
	}

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		if (zend_hash_exists(php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1)) {
			/* Simply removing the zval* causes weirdness with CVs */
			zend_hash_update(php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, (void*)&EG(uninitialized_zval_ptr), sizeof(zval*), NULL);
		}
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (member == &member_copy) {
		zval_dtor(member);
	}
}
/* }}} */

/* ********************
   * Class Definition *
   ******************** */

static
	ZEND_BEGIN_ARG_INFO_EX(php_runkit_require_two_args, 0, 0, 2)
		ZEND_ARG_PASS_INFO(0)
		ZEND_ARG_PASS_INFO(0)
	ZEND_END_ARG_INFO()

static function_entry php_runkit_sandbox_parent_functions[] = {
	PHP_ME(Runkit_Sandbox_Parent,		__call,						php_runkit_require_two_args,		ZEND_ACC_PUBLIC)
	/* Language Constructs */
	PHP_ME(Runkit_Sandbox_Parent,		eval,						NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox_Parent,		include,					NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox_Parent,		include_once,				NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox_Parent,		require,					NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox_Parent,		require_once,				NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox_Parent,		echo,						NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox_Parent,		print,						NULL,								ZEND_ACC_PUBLIC)
	PHP_ME(Runkit_Sandbox_Parent,		die,						NULL,								ZEND_ACC_PUBLIC)
	PHP_MALIAS(Runkit_Sandbox_Parent,	exit,	die,				NULL,								ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static void php_runkit_sandbox_parent_dtor(php_runkit_sandbox_parent_object *objval TSRMLS_DC)
{
	zend_hash_destroy(objval->obj.properties);
	FREE_HASHTABLE(objval->obj.properties);

	efree(objval);
}

static zend_object_value php_runkit_sandbox_parent_ctor(zend_class_entry *ce TSRMLS_DC)
{
	php_runkit_sandbox_parent_object *objval;
	zend_object_value retval;

	if (RUNKIT_G(current_sandbox)) {
		objval = ecalloc(1, sizeof(php_runkit_sandbox_parent_object));
		objval->obj.ce = ce;
		objval->self = RUNKIT_G(current_sandbox);
	} else {
		/* Assign a "blind" stdClass when invoked from the top-scope */
		objval = ecalloc(1, sizeof(zend_object));
		objval->obj.ce = zend_standard_class_def;
	}
	ALLOC_HASHTABLE(objval->obj.properties);
	zend_hash_init(objval->obj.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	retval.handle = zend_objects_store_put(objval, NULL, (zend_objects_free_object_storage_t)php_runkit_sandbox_parent_dtor, NULL TSRMLS_CC);

	retval.handlers = RUNKIT_G(current_sandbox) ? &php_runkit_sandbox_parent_handlers : zend_get_std_object_handlers();

	return retval;
}

int php_runkit_init_sandbox_parent(INIT_FUNC_ARGS)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, PHP_RUNKIT_SANDBOX_PARENT_CLASSNAME, php_runkit_sandbox_parent_functions);
	php_runkit_sandbox_parent_entry = zend_register_internal_class(&ce TSRMLS_CC);
	php_runkit_sandbox_parent_entry->create_object = php_runkit_sandbox_parent_ctor;

	/* Make a new object handler struct with a couple minor changes */
	memcpy(&php_runkit_sandbox_parent_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_runkit_sandbox_parent_handlers.read_property			= php_runkit_sandbox_parent_read_property;
	php_runkit_sandbox_parent_handlers.write_property			= php_runkit_sandbox_parent_write_property;
	php_runkit_sandbox_parent_handlers.has_property				= php_runkit_sandbox_parent_has_property;
	php_runkit_sandbox_parent_handlers.unset_property			= php_runkit_sandbox_parent_unset_property;

	/* No dimension access for parent (initially) */
	php_runkit_sandbox_parent_handlers.read_dimension			= NULL;
	php_runkit_sandbox_parent_handlers.write_dimension			= NULL;
	php_runkit_sandbox_parent_handlers.has_dimension			= NULL;
	php_runkit_sandbox_parent_handlers.unset_dimension			= NULL;

	/* ZE has no concept of modifying properties in place via zval** across contexts */
	php_runkit_sandbox_parent_handlers.get_property_ptr_ptr		= NULL;

	return SUCCESS;
}

int php_runkit_shutdown_sandbox_parent(SHUTDOWN_FUNC_ARGS)
{
	return SUCCESS;
}

#endif /* PHP_RUNKIT_SANDBOX */

