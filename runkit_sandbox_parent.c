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

#ifdef PHP_RUNKIT_SANDBOX
#include "php_runkit_sandbox.h"
#include "php_runkit_zval.h"

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
#if (RUNKIT_ABOVE53)
	HashTable *oldActiveSymbolTable, *result;
	zend_execute_data *oldCurExData;
#endif
	zend_execute_data *ex = EG(current_execute_data);

#if (RUNKIT_UNDER53 == 0)
	if (!EG(active_symbol_table)) {
		zend_rebuild_symbol_table(TSRMLS_C);
	}
#endif

	if (objval->self->parent_scope <= 0) {
		HashTable *ht = &EG(symbol_table);

		if (objval->self->parent_scope_name) {
			zval **symtable;

			if (zend_hash_find(ht, objval->self->parent_scope_name, objval->self->parent_scope_namelen + 1, (void*)&symtable) == SUCCESS) {
				if (Z_TYPE_PP(symtable) == IS_ARRAY) {
					ht = Z_ARRVAL_PP(symtable);
				} else {
					/* Variable exists but is not an array,
					 * Make a dummy array that contains this var */
					zval *hidden;

					ALLOC_INIT_ZVAL(hidden);
					array_init(hidden);
					ht = Z_ARRVAL_P(hidden);
					if ((*symtable)->RUNKIT_REFCOUNT > 1 &&
						!(*symtable)->RUNKIT_IS_REF) {
						zval *cv;

						MAKE_STD_ZVAL(cv);
						*cv = **symtable;
						zval_copy_ctor(cv);
						zval_ptr_dtor(symtable);
						INIT_PZVAL(cv);
						*symtable = cv;
					}
					(*symtable)->RUNKIT_IS_REF = 1;
					(*symtable)->RUNKIT_REFCOUNT++;
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

#if (RUNKIT_UNDER53)
	return ex->symbol_table ? ex->symbol_table : &EG(symbol_table);
#else
	oldActiveSymbolTable = EG(active_symbol_table);
	EG(active_symbol_table) = NULL;
	oldCurExData = EG(current_execute_data);
	EG(current_execute_data) = ex;
	zend_rebuild_symbol_table(TSRMLS_C);
	result = EG(active_symbol_table);
	EG(active_symbol_table) = oldActiveSymbolTable;
	EG(current_execute_data) = oldCurExData;
	return result;
#endif
}

/* {{{ proto Runkit_Sandbox_Parent::__call(mixed function_name, array args)
	Call User Function */
PHP_METHOD(Runkit_Sandbox_Parent,__call)
{
	zval *func_name, *args, *retval = NULL;
	php_runkit_sandbox_parent_object *objval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "za", &func_name, &args) == FAILURE) {
		RETURN_NULL();
	}

	PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX_VERIFY_ACCESS(objval, this_ptr);
	if (!objval->self->parent_call) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to call functions in the parent context is not enabled");
		RETURN_FALSE;
	}

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
	{
		char *name = NULL;

		if (!RUNKIT_IS_CALLABLE(func_name, IS_CALLABLE_CHECK_NO_ACCESS, &name)) {
			php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Function not defined");
			if (name) {
				efree(name);
			}
			PHP_RUNKIT_SANDBOX_PARENT_ABORT(objval)
			RETURN_FALSE;
		}

		php_runkit_sandbox_call_int(func_name, &name, &retval, args, return_value, prior_context TSRMLS_CC);
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

		op_array = php_runkit_sandbox_include_or_eval_int(return_value, zcode, type, once, &already_included TSRMLS_CC);

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
static zval *php_runkit_sandbox_parent_read_property(zval *object, zval *member, int type
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 6)
	, const zend_literal *key
#endif
	TSRMLS_DC)
{
	php_runkit_sandbox_parent_object *objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(object);
	zval tmp_member;
	zval retval;
	int prop_found = 0;

	if (!objval) {
		return EG(uninitialized_zval_ptr);
	}
	if (!objval->self->parent_access || !objval->self->parent_read) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to read parent's symbol table is disallowed");
		return EG(uninitialized_zval_ptr);
	}

	PHP_RUNKIT_ZVAL_CONVERT_TO_STRING_IF_NEEDED(member, tmp_member);

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		zval **value;

		if (zend_hash_find(php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, (void*)&value) == SUCCESS) {
			retval = **value;
			prop_found = 1;
		}
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return php_runkit_sandbox_return_property_value(prop_found, &retval TSRMLS_CC);
}
/* }}} */

/* {{{ php_runkit_sandbox_parent_write_property
	write_property handler */
static void php_runkit_sandbox_parent_write_property(zval *object, zval *member, zval *value
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 6)
	, const zend_literal *key
#endif
	TSRMLS_DC)
{
	php_runkit_sandbox_parent_object *objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(object);
	zval tmp_member;

	if (!objval) {
		return;
	}
	if (!objval->self->parent_access || !objval->self->parent_write) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to modify parent's symbol table is disallowed");
		return;
	}

	PHP_RUNKIT_ZVAL_CONVERT_TO_STRING_IF_NEEDED(member, tmp_member);

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		zval *copyval;

		MAKE_STD_ZVAL(copyval);
		*copyval = *value;
		PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(copyval);
		ZEND_SET_SYMBOL(php_runkit_sandbox_parent_resolve_symbol_table(objval TSRMLS_CC), Z_STRVAL_P(member), copyval);
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (member == &tmp_member) {
		zval_dtor(member);
	}
}
/* }}} */

/* {{{ php_runkit_sandbox_parent_has_property
	has_property handler */
static int php_runkit_sandbox_parent_has_property(zval *object, zval *member, int has_set_exists
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	, const zend_literal *key
#endif
	TSRMLS_DC)
{
	php_runkit_sandbox_parent_object* objval = PHP_RUNKIT_SANDBOX_PARENT_FETCHBOX(object);
	zval member_copy;
	int result = 0;

	if (!objval || !objval->self->parent_access || !objval->self->parent_read) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Access to read parent's symbol table is disallowed");
		return 0;
	}

	PHP_RUNKIT_ZVAL_CONVERT_TO_STRING_IF_NEEDED(member, member_copy);

	PHP_RUNKIT_SANDBOX_PARENT_BEGIN(objval)
		php_runkit_sandbox_has_property_int(has_set_exists, member TSRMLS_CC);
	PHP_RUNKIT_SANDBOX_PARENT_END(objval)

	if (member == &member_copy) {
		zval_dtor(member);
	}

	return result;
}
/* }}} */

/* {{{ php_runkit_sandbox_parent_unset_property
	unset_property handler */
static void php_runkit_sandbox_parent_unset_property(zval *object, zval *member
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 6)
	, const zend_literal *key
#endif
	TSRMLS_DC)
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

	PHP_RUNKIT_ZVAL_CONVERT_TO_STRING_IF_NEEDED(member, member_copy);

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

ZEND_BEGIN_ARG_INFO_EX(arginfo_runkit_sandbox_parent__call, 0, 0, 2)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
ZEND_END_ARG_INFO()

static zend_function_entry php_runkit_sandbox_parent_functions[] = {
	PHP_ME(Runkit_Sandbox_Parent,		__call,						arginfo_runkit_sandbox_parent__call,ZEND_ACC_PUBLIC)
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

