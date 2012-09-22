/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group, (c) 2008-2012 Dmitry Zenovich |
  +----------------------------------------------------------------------+
  | This source file is subject to the new BSD license,                  |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.opensource.org/licenses/BSD-3-Clause                      |
  | If you did not receive a copy of the license and are unable to       |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  | Modified by Dmitry Zenovich <dzenovich@gmail.com>                    |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_runkit.h"

#ifdef PHP_RUNKIT_MANIPULATION
/* {{{ php_runkit_check_call_stack
 */
int php_runkit_check_call_stack(zend_op_array *op_array TSRMLS_DC)
{
	zend_execute_data *ptr;

	ptr = EG(current_execute_data);

	while (ptr) {
		if (ptr->op_array && ptr->op_array->opcodes == op_array->opcodes) {
			return FAILURE;
		}
		ptr = ptr->prev_execute_data;
	}

	return SUCCESS;
}
/* }}} */

static void php_runkit_hash_key_dtor(void *hash_key)
{
	efree((void*) PHP_RUNKIT_HASH_KEY((zend_hash_key*)hash_key));
}

/* Maintain order */
#define PHP_RUNKIT_FETCH_FUNCTION_INSPECT	0
#define PHP_RUNKIT_FETCH_FUNCTION_REMOVE	1
#define PHP_RUNKIT_FETCH_FUNCTION_RENAME	2

/* {{{ php_runkit_fetch_function
 */
static int php_runkit_fetch_function(int fname_type, const char *fname, int fname_len, zend_function **pfe, int flag TSRMLS_DC)
{
	zend_function *fe;
	char *fname_lower;
	int fname_lower_len = fname_len;

	fname_lower = estrndup(fname, fname_len);
	if (fname_lower == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		return FAILURE;
	}
	PHP_RUNKIT_STRTOLOWER(fname_lower);

	if (zend_hash_find(EG(function_table), fname_lower, fname_len + 1, (void*)&fe) == FAILURE) {
		efree(fname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() not found", fname);
		return FAILURE;
	}

	if (fe->type == ZEND_INTERNAL_FUNCTION &&
		!RUNKIT_G(internal_override)) {
		efree(fname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() is an internal function and runkit.internal_override is disabled", fname);
		return FAILURE;
	}

	if (fe->type != ZEND_USER_FUNCTION &&
		fe->type != ZEND_INTERNAL_FUNCTION) {
		efree(fname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() is not a user or normal internal function", fname);
		return FAILURE;
	}

	if (pfe) {
		*pfe = fe;
	}

	if (fe->type == ZEND_INTERNAL_FUNCTION &&
		flag >= PHP_RUNKIT_FETCH_FUNCTION_REMOVE) {
		if (!RUNKIT_G(replaced_internal_functions)) {
			ALLOC_HASHTABLE(RUNKIT_G(replaced_internal_functions));
			zend_hash_init(RUNKIT_G(replaced_internal_functions), 4, NULL, NULL, 0);
		}
#if PHP_MAJOR_VERSION >= 6
		zend_u_hash_add(RUNKIT_G(replaced_internal_functions), fname_type, fname_lower, fname_len + 1, (void*)fe, sizeof(zend_function), NULL);
#else
		zend_hash_add(RUNKIT_G(replaced_internal_functions), fname_lower, fname_len + 1, (void*)fe, sizeof(zend_function), NULL);
#endif
		if (flag >= PHP_RUNKIT_FETCH_FUNCTION_RENAME) {
			zend_hash_key hash_key;

			if (!RUNKIT_G(misplaced_internal_functions)) {
				ALLOC_HASHTABLE(RUNKIT_G(misplaced_internal_functions));
				zend_hash_init(RUNKIT_G(misplaced_internal_functions), 4, NULL, php_runkit_hash_key_dtor, 0);
			}
			hash_key.nKeyLength = fname_len + 1;
			PHP_RUNKIT_HASH_KEY(&hash_key) = estrndup(fname_lower, PHP_RUNKIT_HASH_KEYLEN(&hash_key));
			zend_hash_next_index_insert(RUNKIT_G(misplaced_internal_functions), (void*)&hash_key, sizeof(zend_hash_key), NULL);
		}
	}
	efree(fname_lower);

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_function_copy_ctor
	Duplicate structures in an op_array where necessary to make an outright duplicate */
void php_runkit_function_copy_ctor(zend_function *fe, const char *newname, int newname_len TSRMLS_DC)
{
#if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4)
	zend_literal *literals;
	void *run_time_cache;
#endif
#if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1)
	zend_compiled_variable *dupvars;
#endif
	zend_op *opcode_copy, *last_op;
	int i;

	if (newname) {
		fe->common.function_name = estrndup(newname, newname_len);
	} else {
		fe->common.function_name = estrdup(fe->common.function_name);
	}

	if (fe->common.type == ZEND_USER_FUNCTION) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1) || (PHP_MAJOR_VERSION >= 6)
		if (fe->op_array.vars) {
			i = fe->op_array.last_var;
			dupvars = safe_emalloc(fe->op_array.last_var, sizeof(zend_compiled_variable), 0);
			while (i > 0) {
				i--;
				dupvars[i].name = estrdup(fe->op_array.vars[i].name);
				dupvars[i].name_len = fe->op_array.vars[i].name_len;
				dupvars[i].hash_value = fe->op_array.vars[i].hash_value;
			}
			fe->op_array.vars = dupvars;
		}
#endif

		if (fe->op_array.static_variables) {
			HashTable *static_variables = fe->op_array.static_variables;
#if !((PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6))
			zval *tmpZval;
#endif

			ALLOC_HASHTABLE(fe->op_array.static_variables);
			zend_hash_init(fe->op_array.static_variables, zend_hash_num_elements(static_variables), NULL, ZVAL_PTR_DTOR, 0);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6)
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(static_variables), (apply_func_args_t)zval_copy_static_var, 1, fe->op_array.static_variables);
#else
			zend_hash_copy(fe->op_array.static_variables, static_variables, (copy_ctor_func_t) zval_add_ref, (void*) &tmpZval, sizeof(zval*));
#endif
		}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6)
		if (fe->op_array.run_time_cache) {
			run_time_cache = emalloc(sizeof(void*) * fe->op_array.last_cache_slot);
			memcpy(run_time_cache, fe->op_array.run_time_cache, sizeof(void*) * fe->op_array.last_cache_slot);
			fe->op_array.run_time_cache = run_time_cache;
		}
#endif

		opcode_copy = safe_emalloc(sizeof(zend_op), fe->op_array.last, 0);
		last_op = fe->op_array.opcodes + fe->op_array.last;
		for(i = 0; i < fe->op_array.last; i++) {
			opcode_copy[i] = fe->op_array.opcodes[i];
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4) || (PHP_MAJOR_VERSION < 5)
			if (opcode_copy[i].op1.op_type == IS_CONST) {
				zval_copy_ctor(&opcode_copy[i].op1.u.constant);
#else
			if (opcode_copy[i].op1_type == IS_CONST) {
#endif
#ifdef ZEND_ENGINE_2
			} else {
				switch (opcode_copy[i].opcode) {
# ifdef ZEND_GOTO
					case ZEND_GOTO:
# endif
					case ZEND_JMP:
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
						if (opcode_copy[i].op1.jmp_addr >= fe->op_array.opcodes &&
							opcode_copy[i].op1.jmp_addr < last_op) {
							opcode_copy[i].op1.jmp_addr = opcode_copy + (fe->op_array.opcodes[i].op1.jmp_addr - fe->op_array.opcodes);
						}
#else
						if (opcode_copy[i].op1.u.jmp_addr >= fe->op_array.opcodes &&
							opcode_copy[i].op1.u.jmp_addr < last_op) {
							opcode_copy[i].op1.u.jmp_addr = opcode_copy + (fe->op_array.opcodes[i].op1.u.jmp_addr - fe->op_array.opcodes);
						}
#endif
				}
#endif
			}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4) || (PHP_MAJOR_VERSION < 5)
			if (opcode_copy[i].op2.op_type == IS_CONST) {
				zval_copy_ctor(&opcode_copy[i].op2.u.constant);
#else
			if (opcode_copy[i].op2_type == IS_CONST) {
#endif
#ifdef ZEND_ENGINE_2
			} else {
				switch (opcode_copy[i].opcode) {
					case ZEND_JMPZ:
					case ZEND_JMPNZ:
					case ZEND_JMPZ_EX:
					case ZEND_JMPNZ_EX:
# ifdef ZEND_JMP_SET
					case ZEND_JMP_SET:
# endif
# ifdef ZEND_JMP_SET_VAR
					case ZEND_JMP_SET_VAR:
# endif
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
						if (opcode_copy[i].op2.jmp_addr >= fe->op_array.opcodes &&
							opcode_copy[i].op2.jmp_addr < last_op) {
							opcode_copy[i].op2.jmp_addr =  opcode_copy + (fe->op_array.opcodes[i].op2.jmp_addr - fe->op_array.opcodes);
						}
#else
						if (opcode_copy[i].op2.u.jmp_addr >= fe->op_array.opcodes &&
							opcode_copy[i].op2.u.jmp_addr < last_op) {
							opcode_copy[i].op2.u.jmp_addr =  opcode_copy + (fe->op_array.opcodes[i].op2.u.jmp_addr - fe->op_array.opcodes);
						}
#endif
				}
#endif
			}
		}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6)
		if (fe->op_array.literals) {
			i = fe->op_array.last_literal;
			literals = safe_emalloc(fe->op_array.last_literal, sizeof(zend_literal), 0);
			while (i > 0) {
				zval *tmpZval;
				int k;

				i--;
				ALLOC_ZVAL(tmpZval);
				literals[i] = fe->op_array.literals[i];
				*tmpZval = literals[i].constant;
				zval_copy_ctor(tmpZval);
				literals[i].constant = *tmpZval;
				for (k=0; k < fe->op_array.last; k++) {
					if (opcode_copy[k].op1_type == IS_CONST && opcode_copy[k].op1.zv == &fe->op_array.literals[i].constant) {
						opcode_copy[k].op1.zv = &literals[i].constant;
					}
					if (opcode_copy[k].op2_type == IS_CONST && opcode_copy[k].op2.zv == &fe->op_array.literals[i].constant) {
						opcode_copy[k].op2.zv = &literals[i].constant;
					}
				}
				literals[i].hash_value = fe->op_array.literals[i].hash_value;
				efree(tmpZval);
			}
			fe->op_array.literals = literals;
		}
#endif

		fe->op_array.opcodes = opcode_copy;
		fe->op_array.refcount = emalloc(sizeof(zend_uint));
		*fe->op_array.refcount = 1;
#if !((PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6))
		fe->op_array.start_op = fe->op_array.opcodes;
#endif
#ifdef ZEND_ENGINE_2
		fe->op_array.doc_comment = estrndup(fe->op_array.doc_comment, fe->op_array.doc_comment_len);
		fe->op_array.try_catch_array = (zend_try_catch_element*)estrndup((char*)fe->op_array.try_catch_array, sizeof(zend_try_catch_element) * fe->op_array.last_try_catch);
#endif
		fe->op_array.brk_cont_array = (zend_brk_cont_element*)estrndup((char*)fe->op_array.brk_cont_array, sizeof(zend_brk_cont_element) * fe->op_array.last_brk_cont);
	}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6)
	fe->common.fn_flags &= ~ZEND_ACC_DONE_PASS_TWO;
#endif
#ifdef ZEND_ENGINE_2
	fe->common.prototype = fe;

	if (fe->common.arg_info) {
		zend_arg_info *tmpArginfo;

		tmpArginfo = safe_emalloc(sizeof(zend_arg_info), fe->common.num_args, 0);
		for(i = 0; i < fe->common.num_args; i++) {
			tmpArginfo[i] = fe->common.arg_info[i];
			tmpArginfo[i].name = estrndup(tmpArginfo[i].name, tmpArginfo[i].name_len);
			if (tmpArginfo[i].class_name) {
				tmpArginfo[i].class_name = estrndup(tmpArginfo[i].class_name, tmpArginfo[i].class_name_len);
			}
		}
		fe->common.arg_info = tmpArginfo;
	}
#endif //ZEND_ENGINE_2
}
/* }}}} */

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
/* {{{ php_runkit_clear_function_runtime_cache */
static int php_runkit_clear_function_runtime_cache(void *pDest TSRMLS_DC)
{
	zend_function *f = (zend_function *) pDest;

	if (pDest == NULL || f->type != ZEND_USER_FUNCTION ||
	    f->op_array.last_cache_slot == 0 || f->op_array.run_time_cache == NULL) {
		return ZEND_HASH_APPLY_KEEP;
	}

	memset(f->op_array.run_time_cache, 0, (f->op_array.last_cache_slot) * sizeof(void*));

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_clear_all_functions_runtime_cache */
void php_runkit_clear_all_functions_runtime_cache(TSRMLS_D)
{
	int i, count;
	zend_execute_data *ptr;
	HashPosition pos;

	zend_hash_apply(EG(function_table), php_runkit_clear_function_runtime_cache TSRMLS_CC);

	zend_hash_internal_pointer_reset_ex(EG(class_table), &pos);
	count = zend_hash_num_elements(EG(class_table));
	for (i = 0; i < count; ++i) {
		zend_class_entry **curce;
		zend_hash_get_current_data_ex(EG(class_table), (void*)&curce, &pos);
		zend_hash_apply(&(*curce)->function_table, php_runkit_clear_function_runtime_cache TSRMLS_CC);
		zend_hash_move_forward_ex(EG(class_table), &pos);
	}

	for (ptr = EG(current_execute_data); ptr != NULL; ptr = ptr->prev_execute_data) {
		if (ptr->op_array == NULL || ptr->op_array->last_cache_slot == 0 || ptr->op_array->run_time_cache == NULL) {
			continue;
		}
		memset(ptr->op_array->run_time_cache, 0, (ptr->op_array->last_cache_slot) * sizeof(void*));
	}

	if (!EG(objects_store).object_buckets) {
		return;
	}

	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		   EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == zend_ce_closure) {
				zend_closure *cl = (zend_closure *) object;
				php_runkit_clear_function_runtime_cache((void*) &cl->func TSRMLS_CC);
			}
		}
	}
}
/* }}} */
#endif

/* {{{ php_runkit_generate_lambda_method
	Heavily borrowed from ZEND_FUNCTION(create_function) */
int php_runkit_generate_lambda_method(const char *arguments, int arguments_len, const char *phpcode, int phpcode_len,
                                      zend_function **pfe, zend_bool return_ref TSRMLS_DC)
{
	char *eval_code, *eval_name;
	int eval_code_length;

	eval_code_length = sizeof("function " RUNKIT_TEMP_FUNCNAME) + arguments_len + 4 + phpcode_len + return_ref;
	eval_code = (char*)emalloc(eval_code_length);
	snprintf(eval_code, eval_code_length, "function %s" RUNKIT_TEMP_FUNCNAME "(%s){%s}", (return_ref ? "&" : ""), arguments, phpcode);
	eval_name = zend_make_compiled_string_description("runkit runtime-created function" TSRMLS_CC);
	if (zend_eval_string(eval_code, NULL, eval_name TSRMLS_CC) == FAILURE) {
		efree(eval_code);
		efree(eval_name);
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot create temporary function");
		return FAILURE;
	}
	efree(eval_code);
	efree(eval_name);

	if (zend_hash_find(EG(function_table), RUNKIT_TEMP_FUNCNAME, sizeof(RUNKIT_TEMP_FUNCNAME), (void*)pfe) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unexpected inconsistency during create_function");
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_destroy_misplaced_functions
	Wipe old internal functions that were renamed to new targets
	They'll get replaced soon enough */
int php_runkit_destroy_misplaced_functions(void *pDest TSRMLS_DC)
{
	zend_hash_key *hash_key = (zend_hash_key *) pDest;
	if (!hash_key->nKeyLength) {
		/* Nonsense, skip it */
		return ZEND_HASH_APPLY_REMOVE;
	}

#if PHP_MAJOR_VERSION >= 6
	zend_u_hash_del(EG(function_table), hash_key->type, hash_key->u.unicode, hash_key->nKeyLength);
#else
	zend_hash_del(EG(function_table), hash_key->arKey, hash_key->nKeyLength);
#endif

	return ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

/* {{{ php_runkit_restore_internal_functions
	Cleanup after modifications to internal functions */
int php_runkit_restore_internal_functions(RUNKIT_53_TSRMLS_ARG(void *pDest), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_internal_function *fe = (zend_internal_function *) pDest;

#ifdef ZTS
#if (RUNKIT_UNDER53)
	void ***tsrm_ls = va_arg(args, void***); /* NULL when !defined(ZTS) */
#endif
#endif

	if (!hash_key->nKeyLength) {
		/* Nonsense, skip it */
		return ZEND_HASH_APPLY_REMOVE;
	}

#if PHP_MAJOR_VERSION >= 6
	zend_u_hash_update(EG(function_table), hash_key->type, hash_key->u.unicode, hash_key->nKeyLength, (void*)fe, sizeof(zend_function), NULL);
#else
	zend_hash_update(EG(function_table), hash_key->arKey, hash_key->nKeyLength, (void*)fe, sizeof(zend_function), NULL);
#endif

	return ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

/* *****************
   * Functions API *
   ***************** */

/* {{{ proto bool runkit_function_add(string funcname, string arglist, string code[, bool return_reference = FALSE])
	Add a new function, similar to create_function, but allows specifying name
	There's nothing about this function that's better than eval(), it's here for completeness */
PHP_FUNCTION(runkit_function_add)
{
	PHP_RUNKIT_DECL_STRING_PARAM(funcname)
	PHP_RUNKIT_DECL_STRING_PARAM(funcname_lower)
	PHP_RUNKIT_DECL_STRING_PARAM(arglist)
	PHP_RUNKIT_DECL_STRING_PARAM(code)
	zend_bool return_ref = 0;
	char *delta = NULL, *delta_desc;
	int retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
			PHP_RUNKIT_STRING_SPEC "/" PHP_RUNKIT_STRING_SPEC PHP_RUNKIT_STRING_SPEC "|b",
			PHP_RUNKIT_STRING_PARAM(funcname),
			PHP_RUNKIT_STRING_PARAM(arglist),
			PHP_RUNKIT_STRING_PARAM(code),
			&return_ref) == FAILURE) {
		RETURN_FALSE;
	}

	/* UTODO */
	funcname_lower = estrndup(funcname, funcname_len);
	if (funcname_lower == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}
	funcname_lower_len = funcname_len;
	PHP_RUNKIT_STRTOLOWER(funcname_lower);

	if (zend_hash_exists(EG(function_table), funcname_lower, funcname_len + 1)) {
		efree(funcname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Function %s() already exists", funcname);
		RETURN_FALSE;
	}

#if PHP_MAJOR_VERSION >= 6
	spprintf(&delta, 0, "function %s%R(%R){%R}", return_ref ? "&" : "", funcname_type, funcname, arglist_type, arglist, code_type, code);
#else
	spprintf(&delta, 0, "function %s%s(%s){%s}", return_ref ? "&" : "", funcname, arglist, code);
#endif

	if (!delta) {
		efree(funcname_lower);
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}

	delta_desc = zend_make_compiled_string_description("runkit created function" TSRMLS_CC);
	retval = zend_eval_string(delta, NULL, delta_desc TSRMLS_CC);
	efree(delta_desc);
	efree(delta);
	efree(funcname_lower);

	RETURN_BOOL(retval == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_function_remove(string funcname)
 */
PHP_FUNCTION(runkit_function_remove)
{
	PHP_RUNKIT_DECL_STRING_PARAM(funcname)
	char *funcname_lower;
	int result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, PHP_RUNKIT_STRING_SPEC "/", PHP_RUNKIT_STRING_PARAM(funcname)) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_runkit_fetch_function(PHP_RUNKIT_STRING_TYPE(funcname), funcname, funcname_len, NULL, PHP_RUNKIT_FETCH_FUNCTION_REMOVE TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	funcname_lower = estrndup(funcname, funcname_len);
	if (funcname_lower == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}
	php_strtolower(funcname_lower, funcname_len);

	result = (zend_hash_del(EG(function_table), funcname_lower, funcname_len + 1) == SUCCESS);
	efree(funcname_lower);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	RETURN_BOOL(result);
}
/* }}} */

/* {{{ proto bool runkit_function_rename(string funcname, string newname)
 */
PHP_FUNCTION(runkit_function_rename)
{
	zend_function *fe, func;
	PHP_RUNKIT_DECL_STRING_PARAM(sfunc)
	PHP_RUNKIT_DECL_STRING_PARAM(dfunc)
	PHP_RUNKIT_DECL_STRING_PARAM(dfunc_lower)
	PHP_RUNKIT_DECL_STRING_PARAM(sfunc_lower)

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, PHP_RUNKIT_STRING_SPEC "/" PHP_RUNKIT_STRING_SPEC "/",
			PHP_RUNKIT_STRING_PARAM(sfunc), PHP_RUNKIT_STRING_PARAM(dfunc)) == FAILURE ) {
		RETURN_FALSE;
	}

	/* UTODO */
	dfunc_lower = estrndup(dfunc, dfunc_len);
	if (dfunc_lower == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}
	dfunc_lower_len = dfunc_len;
	PHP_RUNKIT_STRTOLOWER(dfunc_lower);

	if (zend_hash_exists(EG(function_table), dfunc_lower, dfunc_len + 1)) {
		efree(dfunc_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() already exists", dfunc);
		RETURN_FALSE;
	}

	if (php_runkit_fetch_function(PHP_RUNKIT_STRING_TYPE(sfunc), sfunc, sfunc_len, &fe, PHP_RUNKIT_FETCH_FUNCTION_RENAME TSRMLS_CC) == FAILURE) {
		efree(dfunc_lower);
		RETURN_FALSE;
	}

	sfunc_lower = estrndup(sfunc, sfunc_len);
	if (sfunc_lower == NULL) {
		efree(dfunc_lower);
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}
	sfunc_lower_len = sfunc_len;
	PHP_RUNKIT_STRTOLOWER(sfunc_lower);

	func = *fe;
	PHP_RUNKIT_FUNCTION_ADD_REF(&func);

	if (zend_hash_del(EG(function_table), sfunc_lower, sfunc_len + 1) == FAILURE) {
		efree(dfunc_lower);
		efree(sfunc_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error removing reference to old function name %s()", sfunc);
		zend_function_dtor(&func);
		RETURN_FALSE;
	}
	efree(sfunc_lower);

	if (func.type == ZEND_USER_FUNCTION) {
		efree((void*) func.common.function_name);
		func.common.function_name = estrndup(dfunc, dfunc_len);
	}

	if (zend_hash_add(EG(function_table), dfunc_lower, dfunc_len + 1, &func, sizeof(zend_function), NULL) == FAILURE) {
		efree(dfunc_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add reference to new function name %s()", dfunc);
		zend_function_dtor(fe);
		RETURN_FALSE;
	}
	efree(dfunc_lower);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool runkit_function_redefine(string funcname, string arglist, string code)
 */
PHP_FUNCTION(runkit_function_redefine)
{
	PHP_RUNKIT_DECL_STRING_PARAM(funcname)
	PHP_RUNKIT_DECL_STRING_PARAM(funcname_lower)
	PHP_RUNKIT_DECL_STRING_PARAM(arglist)
	PHP_RUNKIT_DECL_STRING_PARAM(code)
	zend_bool return_ref = 0;
	char *delta = NULL, *delta_desc;
	int retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
			PHP_RUNKIT_STRING_SPEC "/" PHP_RUNKIT_STRING_SPEC PHP_RUNKIT_STRING_SPEC "|b",
			PHP_RUNKIT_STRING_PARAM(funcname),
			PHP_RUNKIT_STRING_PARAM(arglist),
			PHP_RUNKIT_STRING_PARAM(code),
			&return_ref) == FAILURE) {
		RETURN_FALSE;
	}

	/* UTODO */
	if (php_runkit_fetch_function(PHP_RUNKIT_STRING_TYPE(funcname), funcname, funcname_len, NULL, PHP_RUNKIT_FETCH_FUNCTION_REMOVE TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	funcname_lower = estrndup(funcname, funcname_len);
	if (funcname_lower == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}
	funcname_lower_len = funcname_len;
	PHP_RUNKIT_STRTOLOWER(funcname_lower);

	if (zend_hash_del(EG(function_table), funcname_lower, funcname_len + 1) == FAILURE) {
		efree(funcname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove old function definition for %s()", funcname);
		RETURN_FALSE;
	}
	efree(funcname_lower);

	spprintf(&delta, 0, "function %s%s(%s){%s}", return_ref ? "&" : "", funcname, arglist, code);

	if (!delta) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}

	delta_desc = zend_make_compiled_string_description("runkit created function" TSRMLS_CC);
	retval = zend_eval_string(delta, NULL, delta_desc TSRMLS_CC);
	efree(delta_desc);
	efree(delta);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	RETURN_BOOL(retval == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_function_copy(string funcname, string targetname)
 */
PHP_FUNCTION(runkit_function_copy)
{
	PHP_RUNKIT_DECL_STRING_PARAM(sfunc)
	PHP_RUNKIT_DECL_STRING_PARAM(dfunc)
	PHP_RUNKIT_DECL_STRING_PARAM(sfunc_lower)
	PHP_RUNKIT_DECL_STRING_PARAM(dfunc_lower)
	zend_function *sfe, fe;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
			PHP_RUNKIT_STRING_SPEC "/" PHP_RUNKIT_STRING_SPEC "/",
			PHP_RUNKIT_STRING_PARAM(sfunc),
			PHP_RUNKIT_STRING_PARAM(dfunc)) == FAILURE) {
		RETURN_FALSE;
	}

	dfunc_lower = estrndup(dfunc, dfunc_len);
	if (dfunc_lower == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}
	dfunc_lower_len = dfunc_len;
	/* UTODO */
	PHP_RUNKIT_STRTOLOWER(dfunc_lower);

	if (zend_hash_exists(EG(function_table), dfunc_lower, dfunc_len + 1)) {
		efree(dfunc_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() already exists", dfunc);
		RETURN_FALSE;
	}

	if (php_runkit_fetch_function(PHP_RUNKIT_STRING_TYPE(sfunc), sfunc, sfunc_len, &sfe, PHP_RUNKIT_FETCH_FUNCTION_INSPECT TSRMLS_CC) == FAILURE) {
		efree(dfunc_lower);
		RETURN_FALSE;
	}

	sfunc_lower = estrndup(sfunc, sfunc_len);
	if (sfunc_lower == NULL) {
		efree(dfunc_lower);
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Not enough memory");
		RETURN_FALSE;
	}
	sfunc_lower_len = sfunc_len;
	/* UTODO */
	PHP_RUNKIT_STRTOLOWER(sfunc_lower);

	fe = *sfe;
	if (fe.type == ZEND_USER_FUNCTION) {
		php_runkit_function_copy_ctor(&fe, dfunc, dfunc_len TSRMLS_CC);
	} else {
		zend_hash_key hash_key;

		hash_key.nKeyLength = PHP_RUNKIT_STRING_LEN(dfunc, 1);
		PHP_RUNKIT_HASH_KEY(&hash_key) = estrndup(dfunc_lower, PHP_RUNKIT_HASH_KEYLEN(&hash_key));
		if (!RUNKIT_G(misplaced_internal_functions)) {
			ALLOC_HASHTABLE(RUNKIT_G(misplaced_internal_functions));
			zend_hash_init(RUNKIT_G(misplaced_internal_functions), 4, NULL, php_runkit_hash_key_dtor, 0);
		}
		zend_hash_next_index_insert(RUNKIT_G(misplaced_internal_functions), (void*)&hash_key, sizeof(zend_hash_key), NULL);
	}

	if (zend_hash_add(EG(function_table), dfunc_lower, dfunc_len + 1, &fe, sizeof(zend_function), NULL) == FAILURE) {
		efree(dfunc_lower);
		efree(sfunc_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add refernce to new function name %s()", dfunc);
		zend_function_dtor(&fe);
		RETURN_FALSE;
	}

	efree(dfunc_lower);
	efree(sfunc_lower);

	RETURN_TRUE;

}
/* }}} */
#endif /* PHP_RUNKIT_MANIPULATION */

/* {{{ proto bool runkit_return_value_used(void)
Does the calling function do anything with our return value? */
PHP_FUNCTION(runkit_return_value_used)
{
	zend_execute_data *ptr = EG(current_execute_data)->prev_execute_data;

	if (!ptr) {
		/* main() */
		RETURN_FALSE;
	}

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6)
	RETURN_BOOL(!(ptr->opline->result_type & EXT_TYPE_UNUSED));
#else
	RETURN_BOOL(!(ptr->opline->result.u.EA.type & EXT_TYPE_UNUSED));
#endif
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

