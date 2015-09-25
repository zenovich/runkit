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

#ifndef PHP_RUNKIT_SANDBOX_H
#define PHP_RUNKIT_SANDBOX_H

/* {{{ php_runkit_sandbox_has_property_int */
inline static int php_runkit_sandbox_has_property_int(int has_set_exists, zval *member TSRMLS_DC) {
	zval **tmpzval;
	int result = 0;

#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 1
	/* Map PHP 5.0 has_property flag to PHP 5.1+ flag */
	has_set_exists = (has_set_exists == 0) ? 2 : 1;
#endif

	if (zend_hash_find(&EG(symbol_table), Z_STRVAL_P(member), Z_STRLEN_P(member) + 1, (void*)&tmpzval) == SUCCESS) {
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
	return result;
}
/* }}} */

/* {{{ php_runkit_sandbox_include_or_eval_int */
inline static zend_op_array *php_runkit_sandbox_include_or_eval_int(zval *return_value, zval *zcode, int type, int once, int *already_included TSRMLS_DC) {
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
			if (zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void*)&dummy, sizeof(int), NULL)==SUCCESS) {
				op_array = zend_compile_file(&file_handle, type TSRMLS_CC);
				zend_destroy_file_handle(&file_handle TSRMLS_CC);
			} else {
				RUNKIT_FILE_HANDLE_DTOR(&file_handle);
				RETVAL_TRUE;
				*already_included = 1;
			}
		}
	}
	return op_array;
}
/* }}} */

/* {{{ php_runkit_sandbox_call_int */
inline static void php_runkit_sandbox_call_int(zval *func_name, char **pname, zval **pretval, zval *args, zval *return_value, void *prior_context TSRMLS_DC) {
	HashPosition pos;
	int i;
	zval **tmpzval;
	int argc = zend_hash_num_elements(Z_ARRVAL_P(args));
	zval ***sandbox_args = safe_emalloc(sizeof(zval**), argc, 0);

	for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(args), &pos), i = 0;
		(zend_hash_get_current_data_ex(Z_ARRVAL_P(args), (void*)&tmpzval, &pos) == SUCCESS) && (i < argc);
		zend_hash_move_forward_ex(Z_ARRVAL_P(args), &pos), i++) {
		sandbox_args[i] = emalloc(sizeof(zval*));
		MAKE_STD_ZVAL(*sandbox_args[i]);
		**sandbox_args[i] = **tmpzval;

#if RUNKIT_ABOVE53
		if (Z_TYPE_P(*sandbox_args[i]) == IS_OBJECT && zend_get_class_entry(*sandbox_args[i], prior_context) == zend_ce_closure) {
			zend_closure *closure;
			zend_object_store_bucket *bucket;
			bucket = php_runkit_zend_object_store_get_obj(*sandbox_args[i], prior_context);
			closure = (zend_closure *) bucket->bucket.obj.object;
			(*sandbox_args[i])->value.obj.handle = zend_objects_store_put(closure, NULL, NULL, bucket->bucket.obj.clone TSRMLS_CC);
		} else
#endif
			PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(*sandbox_args[i]);
	}

	/* Shouldn't be necessary */
	argc = i;

	/* Note: If this function is disabled by disable_functions or disable_classes,
	 * The user will get a confusing error message about (null)() being disabled for security reasons on line 0
	 * This will be fixable with a properly set EG(function_state_ptr)....just not yet
	 */
	if (call_user_function_ex(EG(function_table), NULL, func_name, pretval, argc, sandbox_args, 0, NULL TSRMLS_CC) == SUCCESS) {
		if (*pretval) {
			*return_value = **pretval;
		} else {
			RETVAL_TRUE;
		}
	} else {
		php_error_docref1(NULL TSRMLS_CC, *pname, E_WARNING, "Unable to call function");
		RETVAL_FALSE;
	}
	if (*pname) {
		efree(*pname);
		*pname = NULL;
	}

	for(i = 0; i < argc; i++) {
#if RUNKIT_ABOVE53
		if (Z_TYPE_P(*sandbox_args[i]) == IS_OBJECT && zend_get_class_entry(*sandbox_args[i] TSRMLS_CC) == zend_ce_closure) {
			zend_object_store_bucket *bucket = php_runkit_zend_object_store_get_obj(*sandbox_args[i] TSRMLS_CC);
			zend_objects_store_del_ref(*sandbox_args[i] TSRMLS_CC);
			zval_ptr_dtor(sandbox_args[i]);
			bucket->bucket.obj.object = NULL;
		}
#endif
		zval_ptr_dtor(sandbox_args[i]);
		efree(sandbox_args[i]);
	}
	efree(sandbox_args);
}
/* }}} */

/* {{{ php_runkit_sandbox_return_property_value */
inline static zval *php_runkit_sandbox_return_property_value(int prop_found, zval *retval TSRMLS_DC) {
	if (prop_found) {
		zval *return_value;

		ALLOC_ZVAL(return_value);
		*return_value = *retval;

		/* ZE expects refcount == 0 for unowned values */
		INIT_PZVAL(return_value);
		PHP_SANDBOX_CROSS_SCOPE_ZVAL_COPY_CTOR(return_value);
		return_value->RUNKIT_REFCOUNT--;

		return return_value;
	} else {
		return EG(uninitialized_zval_ptr);
	}
}
/* }}} */

#endif

