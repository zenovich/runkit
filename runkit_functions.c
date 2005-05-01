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

/* {{{ php_runkit_check_call_stack
 */
int php_runkit_check_call_stack(zend_op_array *op_array TSRMLS_DC)
{
	zend_execute_data *ptr;

	ptr = EG(current_execute_data);

	while (ptr) {
		if (ptr->op_array->opcodes == op_array->opcodes) {
			return FAILURE;
		}
		ptr = ptr->prev_execute_data;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_fetch_function
 */
static int php_runkit_fetch_function(char *fname, int fname_len, zend_function **pfe TSRMLS_DC)
{
	zend_function *fe;

	php_strtolower(fname, fname_len);

	if (zend_hash_find(EG(function_table), fname, fname_len + 1, (void**)&fe) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() not found", fname);
		return FAILURE;
	}

	if (fe->type != ZEND_USER_FUNCTION) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() is not a user function", fname);
		return FAILURE;
	}

	if (pfe) {
		*pfe = fe;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_function_copy_ctor
	Duplicate structures in an op_array where necessary to make an outright duplicate */
void php_runkit_function_copy_ctor(zend_function *fe, char *newname)
{
#if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1)
	zend_compiled_variable *dupvars;
#endif
	zend_op *opcode_copy;
	int i;

	if (fe->op_array.static_variables) {
		HashTable *tmpHash;
		zval tmpZval;

		ALLOC_HASHTABLE(tmpHash);
		zend_hash_init(tmpHash, 2, NULL, ZVAL_PTR_DTOR, 0);
		zend_hash_copy(tmpHash, fe->op_array.static_variables, ZVAL_COPY_CTOR, &tmpZval, sizeof(zval));
		fe->op_array.static_variables = tmpHash;
	}

	fe->op_array.refcount = emalloc(sizeof(zend_uint));
	*(fe->op_array.refcount) = 1;

#if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1)
	i = fe->op_array.last_var;
	dupvars = safe_emalloc(fe->op_array.last_var, sizeof(zend_compiled_variable), 0);
	while (i > 0) {
		i--;
		dupvars[i].name = estrdup(fe->op_array.vars[i].name);
	}
	fe->op_array.vars = dupvars;
#endif

	opcode_copy = safe_emalloc(sizeof(zend_op), fe->op_array.last, 0);
	for(i = 0; i < fe->op_array.last; i++) {
		opcode_copy[i] = fe->op_array.opcodes[i];
		if (opcode_copy[i].op1.op_type == IS_CONST) {
			zval_copy_ctor(&opcode_copy[i].op1.u.constant);
#ifdef ZEND_ENGINE_2
		} else {
			if (opcode_copy[i].op1.u.jmp_addr >= fe->op_array.opcodes &&
				opcode_copy[i].op1.u.jmp_addr <  fe->op_array.opcodes + fe->op_array.last) {
				opcode_copy[i].op1.u.jmp_addr =  opcode_copy + (fe->op_array.opcodes[i].op1.u.jmp_addr - fe->op_array.opcodes);
			}
#endif
        }

		if (opcode_copy[i].op2.op_type == IS_CONST) {
			zval_copy_ctor(&opcode_copy[i].op2.u.constant);
#ifdef ZEND_ENGINE_2
		} else {
			if (opcode_copy[i].op2.u.jmp_addr >= fe->op_array.opcodes &&
				opcode_copy[i].op2.u.jmp_addr <  fe->op_array.opcodes + fe->op_array.last) {
				opcode_copy[i].op2.u.jmp_addr =  opcode_copy + (fe->op_array.opcodes[i].op2.u.jmp_addr - fe->op_array.opcodes);
			}
#endif
		}
	}
	fe->op_array.opcodes = opcode_copy;
	fe->op_array.start_op = fe->op_array.opcodes;

	if (newname) {
		fe->op_array.function_name = newname;
	} else {
		fe->op_array.function_name = estrdup(fe->op_array.function_name);
	}

#ifdef ZEND_ENGINE_2
	fe->op_array.prototype = fe;

	if (fe->op_array.arg_info) {
		zend_arg_info *tmpArginfo;

		tmpArginfo = safe_emalloc(sizeof(zend_arg_info), fe->op_array.num_args, 0);
		for(i = 0; i < fe->op_array.num_args; i++) {
			tmpArginfo[i] = fe->op_array.arg_info[i];
			tmpArginfo[i].name = estrndup(tmpArginfo[i].name, tmpArginfo[i].name_len);
			if (tmpArginfo[i].class_name) {
				tmpArginfo[i].class_name = estrndup(tmpArginfo[i].class_name, tmpArginfo[i].class_name_len);
			}
		}
		fe->op_array.arg_info = tmpArginfo;
	}

	fe->op_array.doc_comment = estrndup(fe->op_array.doc_comment, fe->op_array.doc_comment_len);
	fe->op_array.try_catch_array = (zend_try_catch_element*)estrndup((char*)fe->op_array.try_catch_array, sizeof(zend_try_catch_element) * fe->op_array.last_try_catch);
#endif

	fe->op_array.brk_cont_array = (zend_brk_cont_element*)estrndup((char*)fe->op_array.brk_cont_array, sizeof(zend_brk_cont_element) * fe->op_array.last_brk_cont);
}
/* }}}} */

/* {{{ php_runkit_generate_lambda_method
	Heavily borrowed from ZEND_FUNCTION(create_function) */
int php_runkit_generate_lambda_method(char *arguments, int arguments_len, char *phpcode, int phpcode_len, zend_function **pfe TSRMLS_DC)
{
	char *eval_code, *eval_name;
	int eval_code_length;

	eval_code_length = sizeof("function " RUNKIT_TEMP_FUNCNAME) + arguments_len + 4 + phpcode_len;
	eval_code = (char*)emalloc(eval_code_length);
	snprintf(eval_code, eval_code_length, "function " RUNKIT_TEMP_FUNCNAME "(%s){%s}", arguments, phpcode);
	eval_name = zend_make_compiled_string_description("runkit runtime-created function" TSRMLS_CC);
	if (zend_eval_string(eval_code, NULL, eval_name TSRMLS_CC) == FAILURE) {
		efree(eval_code);
		efree(eval_name);
		return FAILURE;
	}
	efree(eval_code);
	efree(eval_name);

	if (zend_hash_find(EG(function_table), RUNKIT_TEMP_FUNCNAME, sizeof(RUNKIT_TEMP_FUNCNAME), (void **)pfe) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unexpected inconsistency during create_function");
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* *****************
   * Functions API *
   ***************** */

/* {{{ proto bool runkit_function_add(string funcname, string arglist, string code)
	Add a new function, similar to create_function, but allows specifying name
	There's nothing about this function that's better than eval(), it's here for completeness */
PHP_FUNCTION(runkit_function_add)
{
	char *funcname, *arglist, *code, *delta = NULL, *delta_desc;
	int funcname_len, arglist_len, code_len, retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &funcname,	&funcname_len,
																&arglist,	&arglist_len,
																&code,		&code_len) == FAILURE) {
		RETURN_FALSE;
	}

	php_strtolower(funcname, funcname_len);

	if (zend_hash_exists(EG(function_table), funcname, funcname_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Function %s() already exists", funcname);
		RETURN_FALSE;
	}

	spprintf(&delta, 0, "function %s(%s){%s}", funcname, arglist, code);

	if (!delta) {
		RETURN_FALSE;
	}

	delta_desc = zend_make_compiled_string_description("runkit created function" TSRMLS_CC);
	retval = zend_eval_string(delta, NULL, delta_desc TSRMLS_CC);
	efree(delta_desc);
	efree(delta);

	RETURN_BOOL(retval == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_function_remove(string funcname)
 */
PHP_FUNCTION(runkit_function_remove)
{
	char *funcname;
	int funcname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &funcname, &funcname_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_runkit_fetch_function(funcname, funcname_len, NULL TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_BOOL(zend_hash_del(EG(function_table), funcname, funcname_len + 1) == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_function_rename(string funcname, string newname)
 */
PHP_FUNCTION(runkit_function_rename)
{
	zend_function *fe, func;
	char *sfunc, *dfunc;
	int sfunc_len, dfunc_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",	&sfunc, &sfunc_len,
																&dfunc, &dfunc_len) == FAILURE) {
		RETURN_FALSE;
	}

	php_strtolower(dfunc, dfunc_len);

	if (zend_hash_exists(EG(function_table), dfunc, dfunc_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() already exists", dfunc);
		RETURN_FALSE;
	}

	if (php_runkit_fetch_function(sfunc, sfunc_len, &fe TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	func = *fe;

	function_add_ref(&func);

	if (zend_hash_del(EG(function_table), sfunc, sfunc_len + 1) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error removing reference to old function name %s()", sfunc);
		zend_function_dtor(&func);
		RETURN_FALSE;
	}

	efree(func.common.function_name);
	func.common.function_name = estrndup(dfunc, dfunc_len);

	if (zend_hash_add(EG(function_table), dfunc, dfunc_len + 1, &func, sizeof(zend_function), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add refernce to new function name %s()", dfunc);
		zend_function_dtor(fe);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool runkit_function_redefine(string funcname, string arglist, string code)
 */
PHP_FUNCTION(runkit_function_redefine)
{
	char *funcname, *arglist, *code, *delta = NULL, *delta_desc;
	int funcname_len, arglist_len, code_len, retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &funcname,	&funcname_len,
																&arglist,	&arglist_len,
																&code,		&code_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_runkit_fetch_function(funcname, funcname_len, NULL TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	if (zend_hash_del(EG(function_table), funcname, funcname_len + 1) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove old function definition for %s()", funcname);
		RETURN_FALSE;
	}

	spprintf(&delta, 0, "function %s(%s){%s}", funcname, arglist, code);

	if (!delta) {
		RETURN_FALSE;
	}

	delta_desc = zend_make_compiled_string_description("runkit created function" TSRMLS_CC);
	retval = zend_eval_string(delta, NULL, delta_desc TSRMLS_CC);
	efree(delta_desc);
	efree(delta);

	RETURN_BOOL(retval == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_function_copy(string funcname, string targetname)
 */
PHP_FUNCTION(runkit_function_copy)
{
	char *sfunc, *dfunc;
	int sfunc_len, dfunc_len;
	zend_function *fe;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",	&sfunc, &sfunc_len,
																&dfunc, &dfunc_len) == FAILURE) {
		RETURN_FALSE;
	}

	php_strtolower(dfunc, dfunc_len);

	if (zend_hash_exists(EG(function_table), dfunc, dfunc_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() already exists", dfunc);
		RETURN_FALSE;
	}

	if (php_runkit_fetch_function(sfunc, sfunc_len, &fe TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	function_add_ref(fe);
	if (zend_hash_add(EG(function_table), dfunc, dfunc_len + 1, fe, sizeof(zend_function), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add refernce to new function name %s()", dfunc);
		zend_function_dtor(fe);
		RETURN_FALSE;
	}

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

