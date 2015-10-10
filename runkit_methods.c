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

/* {{{ _php_runkit_get_method_prototype
	Locates the prototype method */
static inline zend_function* _php_runkit_get_method_prototype(zend_class_entry *ce, const char* func_lower, int func_len TSRMLS_DC) {
	zend_class_entry *pce = ce;
	zend_function *proto = NULL;

	while (pce) {
		if (zend_hash_find(&pce->function_table, (char *) func_lower, func_len+1, (void*) &proto) != FAILURE) {
			break;
		}
		pce = pce->parent;
	}
	if (!pce) {
		proto = NULL;
	}
	return proto;
}
/* }}} */


/* {{{ php_runkit_fetch_class_int
 */
int php_runkit_fetch_class_int(const char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC)
{
	zend_class_entry *ce;
	zend_class_entry **ze;
	PHP_RUNKIT_DECL_STRING_PARAM(classname_lower)

	/* Ignore leading "\" */
	PHP_RUNKIT_NORMALIZE_NAMESPACE(classname);

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(classname);
	if (classname_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		return FAILURE;
	}

	if (zend_hash_find(EG(class_table), classname_lower, classname_lower_len + 1, (void*)&ze) == FAILURE ||
		!ze || !*ze) {
		efree(classname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Class %s not found", classname);
		return FAILURE;
	}
	ce = *ze;

	if (pce) {
		*pce = ce;
	}

	efree(classname_lower);
	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_fetch_class
 */
int php_runkit_fetch_class(const char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC)
{
	zend_class_entry *ce;

	if (php_runkit_fetch_class_int(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (ce->type != ZEND_USER_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is not a user-defined class", classname);
		return FAILURE;
	}

	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is an interface", classname);
		return FAILURE;
	}

	if (pce) {
		*pce = ce;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_fetch_interface
 */
int php_runkit_fetch_interface(const char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC)
{
	if (php_runkit_fetch_class_int(classname, classname_len, pce TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (!((*pce)->ce_flags & ZEND_ACC_INTERFACE)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is not an interface", classname);
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_fetch_class_method
 */
static int php_runkit_fetch_class_method(const char *classname, int classname_len, const char *fname, int fname_len, zend_class_entry **pce, zend_function **pfe
TSRMLS_DC)
{
	HashTable *ftable = EG(function_table);
	zend_class_entry *ce;
	zend_function *fe;
	PHP_RUNKIT_DECL_STRING_PARAM(fname_lower)

	if (php_runkit_fetch_class_int(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (ce->type != ZEND_USER_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "class %s is not a user-defined class", classname);
		return FAILURE;
	}

	if (pce) {
		*pce = ce;
	}

	ftable = &ce->function_table;

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(fname);
	if (fname_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		return FAILURE;
	}

	if (zend_hash_find(ftable, fname_lower, fname_lower_len + 1, (void*)&fe) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s() not found", classname, fname);
		efree(fname_lower);
		return FAILURE;
	}

	if (fe->type != ZEND_USER_FUNCTION) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s() is not a user function", classname, fname);
		efree(fname_lower);
		return FAILURE;
	}

	if (pfe) {
		*pfe = fe;
	}

	efree(fname_lower);
	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_update_children_methods
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_methods(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *ancestor_class =  va_arg(args, zend_class_entry*);
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zend_class_entry *scope;
	zend_function *fe =  va_arg(args, zend_function*);
	char *fname_lower = va_arg(args, char*);
	int fname_lower_len = va_arg(args, int);
	zend_function *orig_fe = va_arg(args, zend_function*);
	zend_function *cfe = NULL;

	RUNKIT_UNDER53_TSRMLS_FETCH();

	ce = *((zend_class_entry**)ce);

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	if (zend_hash_find(&ce->function_table, fname_lower, fname_lower_len + 1, (void*)&cfe) == SUCCESS) {
		scope = php_runkit_locate_scope(ce, cfe, fname_lower, fname_lower_len);
		if (scope != ancestor_class) {
			/* This method was defined below our current level, leave it be */
			cfe->common.prototype = _php_runkit_get_method_prototype(scope->parent, fname_lower, fname_lower_len TSRMLS_CC);
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_methods,
						       6, ancestor_class, ce, fe, fname_lower, fname_lower_len, orig_fe);
			return ZEND_HASH_APPLY_KEEP;
		}
	}

	if (cfe) {
		php_runkit_remove_function_from_reflection_objects(cfe TSRMLS_CC);
		if (zend_hash_del(&ce->function_table, fname_lower, fname_lower_len + 1) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
			return ZEND_HASH_APPLY_KEEP;
		}
	}

	if (zend_hash_add(&ce->function_table, fname_lower, fname_lower_len + 1, fe, sizeof(zend_function), NULL) ==  FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error updating child class");
		return ZEND_HASH_APPLY_KEEP;
	}
	PHP_RUNKIT_FUNCTION_ADD_REF(fe);
	PHP_RUNKIT_INHERIT_MAGIC(ce, fe, orig_fe TSRMLS_CC);

	/* Process children of this child */
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_methods, 6,
				       ancestor_class, ce, fe, fname_lower, fname_lower_len, orig_fe);

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_clean_children
	Scan the class_table for children of the class just updated */
int php_runkit_clean_children_methods(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *ancestor_class = va_arg(args, zend_class_entry*);
	zend_class_entry *parent_class = va_arg(args, zend_class_entry*);
	zend_class_entry *scope;
	char *fname_lower = va_arg(args, char*);
	int fname_lower_len = va_arg(args, int);
	zend_function *orig_cfe = va_arg(args, zend_function *);
	zend_function *cfe = NULL;

	RUNKIT_UNDER53_TSRMLS_FETCH();

	ce = *((zend_class_entry**)ce);

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	if (zend_hash_find(&ce->function_table, fname_lower, fname_lower_len + 1, (void*)&cfe) == SUCCESS) {
		scope = php_runkit_locate_scope(ce, cfe, fname_lower, fname_lower_len);
		if (scope != ancestor_class) {
			/* This method was defined below our current level, leave it be */
			return ZEND_HASH_APPLY_KEEP;
		}
	}

	if (!cfe) {
		/* Odd.... nothing to destroy.... */
		return ZEND_HASH_APPLY_KEEP;
	}

	/* Process children of this child */
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_clean_children_methods, 5, ancestor_class, ce, fname_lower, fname_lower_len, orig_cfe);

	php_runkit_remove_function_from_reflection_objects(cfe TSRMLS_CC);

	zend_hash_del(&ce->function_table, fname_lower, fname_lower_len + 1);

	PHP_RUNKIT_DEL_MAGIC_METHOD(ce, orig_cfe TSRMLS_CC);

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_method_add_or_update
 */
static void php_runkit_method_add_or_update(INTERNAL_FUNCTION_PARAMETERS, int add_or_update)
{
	const char *classname, *methodname, *arguments = NULL, *phpcode = NULL, *doc_comment = NULL;
	int classname_len, methodname_len, arguments_len = 0, phpcode_len = 0, doc_comment_len = 0;
	zend_class_entry *ce, *ancestor_class = NULL;
	zend_function func, *fe, *source_fe = NULL, *orig_fe = NULL;
	PHP_RUNKIT_DECL_STRING_PARAM(methodname_lower)
	long argc = ZEND_NUM_ARGS();
	long flags = ZEND_ACC_PUBLIC;
	zval ***args;
	long opt_arg_pos = 3;
	zend_bool remove_temp = 0;

	if (argc < 2 || zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, 2 TSRMLS_CC, "ss",
				     &classname, &classname_len,
				     &methodname, &methodname_len) == FAILURE || !classname_len || !methodname_len) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class name and method name should not be empty");
		RETURN_FALSE;
	}

	if (argc < 3) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Method body should be provided");
		RETURN_FALSE;
	}

	if (!php_runkit_parse_args_to_zvals(argc, &args TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (!php_runkit_parse_function_arg(argc, args, 2, &source_fe, &arguments, &arguments_len, &phpcode, &phpcode_len, &opt_arg_pos, "Method" TSRMLS_CC)) {
		efree(args);
		RETURN_FALSE;
	}

	if (argc > opt_arg_pos) {
		if (Z_TYPE_PP(args[opt_arg_pos]) == IS_NULL || Z_TYPE_PP(args[opt_arg_pos]) == IS_LONG) {
			convert_to_long_ex(args[opt_arg_pos]);
			flags = Z_LVAL_PP(args[opt_arg_pos]);
			if (flags & PHP_RUNKIT_ACC_RETURN_REFERENCE && source_fe) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "RUNKIT_ACC_RETURN_REFERENCE flag is not applicable for closures, use & in closure definition instead");
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Flags should be a long integer or NULL");
		}
	}

	php_runkit_parse_doc_comment_arg(argc, args, opt_arg_pos + 1, &doc_comment, &doc_comment_len TSRMLS_CC);

	efree(args);

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(methodname);
	if (methodname_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		RETURN_FALSE;
	}

	if (add_or_update == HASH_UPDATE) {
		if (php_runkit_fetch_class_method(classname, classname_len, methodname, methodname_len, &ce, &fe TSRMLS_CC) == FAILURE) {
			efree(methodname_lower);
			RETURN_FALSE;
		}
		ancestor_class = php_runkit_locate_scope(ce, fe, methodname_lower, methodname_lower_len);
		orig_fe = fe;

		if (php_runkit_check_call_stack(&fe->op_array TSRMLS_CC) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot redefine a method while that method is active.");
			efree(methodname_lower);
			RETURN_FALSE;
		}
	} else {
		if (php_runkit_fetch_class(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
			efree(methodname_lower);
			RETURN_FALSE;
		}
		ancestor_class = ce;
		if (zend_hash_find(&ce->function_table, methodname_lower, methodname_lower_len + 1, (void*)&fe) != FAILURE) {
			if (php_runkit_locate_scope(ce, fe, methodname_lower, methodname_lower_len) == ce) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s() already exists", classname, methodname);
				efree(methodname_lower);
				RETURN_FALSE;
			} else {
				php_runkit_remove_function_from_reflection_objects(fe TSRMLS_CC);
				zend_hash_del(&ce->function_table, methodname_lower, methodname_lower_len + 1);
			}
		}
	}

	if (!source_fe) {
		if (php_runkit_generate_lambda_method(arguments, arguments_len, phpcode, phpcode_len, &source_fe,
						     (flags & PHP_RUNKIT_ACC_RETURN_REFERENCE) == PHP_RUNKIT_ACC_RETURN_REFERENCE
						     TSRMLS_CC) == FAILURE) {
			efree(methodname_lower);
			RETURN_FALSE;
		}
		remove_temp = 1;
	}

	func = *source_fe;
	php_runkit_function_copy_ctor(&func, methodname, methodname_len TSRMLS_CC);

	if (flags & ZEND_ACC_PRIVATE) {
		func.common.fn_flags &= ~ZEND_ACC_PPP_MASK;
		func.common.fn_flags |= ZEND_ACC_PRIVATE;
	} else if (flags & ZEND_ACC_PROTECTED) {
		func.common.fn_flags &= ~ZEND_ACC_PPP_MASK;
		func.common.fn_flags |= ZEND_ACC_PROTECTED;
	} else {
		func.common.fn_flags &= ~ZEND_ACC_PPP_MASK;
		func.common.fn_flags |= ZEND_ACC_PUBLIC;
	}

	if (flags & ZEND_ACC_STATIC) {
		func.common.fn_flags |= ZEND_ACC_STATIC;
	} else {
		func.common.fn_flags |= ZEND_ACC_ALLOW_STATIC;
	}

	if (doc_comment == NULL && source_fe->op_array.doc_comment == NULL &&
	   orig_fe && orig_fe->type == ZEND_USER_FUNCTION && orig_fe->op_array.doc_comment) {
		doc_comment = orig_fe->op_array.doc_comment;
		doc_comment_len = orig_fe->op_array.doc_comment_len;
	}
	php_runkit_modify_function_doc_comment(&func, doc_comment, doc_comment_len);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	if(orig_fe) {
		php_runkit_remove_function_from_reflection_objects(orig_fe TSRMLS_CC);
	}

	if (zend_hash_add_or_update(&ce->function_table, methodname_lower, methodname_lower_len + 1, &func, sizeof(zend_function), NULL, add_or_update) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add method to class");
		zend_function_dtor(&func);
		efree(methodname_lower);
		if (remove_temp && zend_hash_del(EG(function_table), RUNKIT_TEMP_FUNCNAME, sizeof(RUNKIT_TEMP_FUNCNAME)) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove temporary function entry");
		}
		RETURN_FALSE;
	}

	if (remove_temp && zend_hash_del(EG(function_table), RUNKIT_TEMP_FUNCNAME, sizeof(RUNKIT_TEMP_FUNCNAME)) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove temporary function entry");
		efree(methodname_lower);
		RETURN_FALSE;
	}

	if (zend_hash_find(&ce->function_table, methodname_lower, methodname_lower_len + 1, (void*)&fe) == FAILURE ||
		!fe) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to locate newly added method");
		efree(methodname_lower);
		RETURN_FALSE;
	}

	fe->common.scope = ce;
	fe->common.prototype = _php_runkit_get_method_prototype(ce->parent, methodname_lower, methodname_lower_len TSRMLS_CC);

	PHP_RUNKIT_ADD_MAGIC_METHOD(ce, methodname_lower, methodname_lower_len, fe, orig_fe TSRMLS_CC);
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_methods, 6,
				       ancestor_class, ce, fe, methodname_lower, methodname_lower_len, orig_fe);

	efree(methodname_lower);

	RETURN_TRUE;
}
/* }}} */

/* {{{ php_runkit_method_copy
 */
static int php_runkit_method_copy(const char *dclass, int dclass_len, const char *dfunc, int dfunc_len,
                                  const char *sclass, int sclass_len, const char *sfunc, int sfunc_len TSRMLS_DC)
{
	zend_class_entry *dce, *sce;
	zend_function dfe, *sfe, *dfeInHashTable;
	PHP_RUNKIT_DECL_STRING_PARAM(dfunc_lower)
	zend_function *fe;

	if (php_runkit_fetch_class_method(sclass, sclass_len, sfunc, sfunc_len, &sce, &sfe TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (php_runkit_fetch_class(dclass, dclass_len, &dce TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(dfunc);
	if (dfunc_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		return FAILURE;
	}

	if (zend_hash_find(&dce->function_table, dfunc_lower, dfunc_lower_len + 1, (void*)&fe) != FAILURE) {
		if (php_runkit_locate_scope(dce, fe, dfunc_lower, dfunc_lower_len) == dce) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Destination method %s::%s() already exists", dclass, dfunc);
			efree(dfunc_lower);
			return FAILURE;
		} else {
			php_runkit_remove_function_from_reflection_objects(fe TSRMLS_CC);
			zend_hash_del(&dce->function_table, dfunc_lower, dfunc_lower_len + 1);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif
		}
	}

	dfe = *sfe;
	php_runkit_function_copy_ctor(&dfe, dfunc, dfunc_len TSRMLS_CC);

	if (zend_hash_add(&dce->function_table, dfunc_lower, dfunc_lower_len + 1, &dfe, sizeof(zend_function), (void*) &dfeInHashTable) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error adding method to class %s::%s()", dclass, dfunc);
		efree(dfunc_lower);
		return FAILURE;
	}

	dfeInHashTable->common.scope = dce;
	dfeInHashTable->common.prototype = _php_runkit_get_method_prototype(dce->parent, dfunc_lower, dfunc_lower_len TSRMLS_CC);

	PHP_RUNKIT_ADD_MAGIC_METHOD(dce, dfunc_lower, dfunc_lower_len, dfeInHashTable, NULL TSRMLS_CC);

	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_methods, 7,
				       dce, dce, dfeInHashTable, dfunc_lower, dfunc_lower_len, NULL, 0);

	efree(dfunc_lower);
	return SUCCESS;
}
/* }}} */

/* **************
   * Method API *
   ************** */

/* {{{ proto bool runkit_method_add(string classname, string methodname, string args, string code[, long flags[, string doc_comment]])
       proto bool runkit_method_add(string classname, string methodname, closure code[, long flags[, string doc_comment]])
       Add a method to an already defined class */
PHP_FUNCTION(runkit_method_add)
{
	php_runkit_method_add_or_update(INTERNAL_FUNCTION_PARAM_PASSTHRU, HASH_ADD);
}
/* }}} */

/* {{{ proto bool runkit_method_redefine(string classname, string methodname, string args, string code[, long flags[, string doc_comment]])
       proto bool runkit_method_redefine(string classname, string methodname, closure code[, long flags[, string doc_comment]])
       Redefine an already defined class method */
PHP_FUNCTION(runkit_method_redefine)
{
	php_runkit_method_add_or_update(INTERNAL_FUNCTION_PARAM_PASSTHRU, HASH_UPDATE);
}
/* }}} */

/* {{{ proto bool runkit_method_remove(string classname, string methodname)
	Remove a method from a class definition */
PHP_FUNCTION(runkit_method_remove)
{
	zend_class_entry *ce, *ancestor_class = NULL;
	zend_function *fe;
	PHP_RUNKIT_DECL_STRING_PARAM(methodname)
	PHP_RUNKIT_DECL_STRING_PARAM(classname)
	PHP_RUNKIT_DECL_STRING_PARAM(methodname_lower)

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/", &classname, &classname_len,
	                                                             &methodname, &methodname_len) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't parse parameters");
		RETURN_FALSE;
	}

	if (!classname_len || !methodname_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty parameter given");
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class_method(classname, classname_len, methodname, methodname_len, &ce, &fe TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown method %s::%s()", classname, methodname);
		RETURN_FALSE;
	}

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(methodname);
	if (methodname_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		RETURN_FALSE;
	}

	ancestor_class = php_runkit_locate_scope(ce, fe, methodname_lower, methodname_lower_len);

	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_clean_children_methods, 5, ancestor_class, ce, methodname_lower, methodname_lower_len, fe);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	php_runkit_remove_function_from_reflection_objects(fe TSRMLS_CC);

	if (zend_hash_del(&ce->function_table, methodname_lower, methodname_lower_len + 1) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove method from class");
		efree(methodname_lower);
		RETURN_FALSE;
	}

	efree(methodname_lower);
	PHP_RUNKIT_DEL_MAGIC_METHOD(ce, fe TSRMLS_CC);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool runkit_method_rename(string classname, string methodname, string newname)
	Rename a method within a class */
PHP_FUNCTION(runkit_method_rename)
{
	zend_class_entry *ce, *ancestor_class = NULL;
	zend_function *fe, func, *old_fe;
	PHP_RUNKIT_DECL_STRING_PARAM(classname)
	PHP_RUNKIT_DECL_STRING_PARAM(methodname)
	PHP_RUNKIT_DECL_STRING_PARAM(newname)
	PHP_RUNKIT_DECL_STRING_PARAM(newname_lower)
	PHP_RUNKIT_DECL_STRING_PARAM(methodname_lower)


	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/s/",	&classname, &classname_len,
	                          &methodname, &methodname_len, &newname, &newname_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (!classname_len || !methodname_len || !newname_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty parameter given");
		RETURN_FALSE;
	}

	if (php_runkit_fetch_class_method(classname, classname_len, methodname, methodname_len, &ce, &fe TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown method %s::%s()", classname, methodname);
		RETURN_FALSE;
	}

	PHP_RUNKIT_MAKE_LOWERCASE_COPY(newname);
	if (newname_lower == NULL) {
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		RETURN_FALSE;
	}
	PHP_RUNKIT_MAKE_LOWERCASE_COPY(methodname);
	if (methodname_lower == NULL) {
		efree(newname_lower);
		PHP_RUNKIT_NOT_ENOUGH_MEMORY_ERROR;
		RETURN_FALSE;
	}

	if (zend_hash_find(&ce->function_table, newname_lower, newname_lower_len + 1, (void*)&old_fe) != FAILURE) {
		if (php_runkit_locate_scope(ce, old_fe, newname_lower, newname_lower_len) == ce) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s() already exists", classname, methodname);
			efree(methodname_lower);
			efree(newname_lower);
			RETURN_FALSE;
		} else {
			php_runkit_remove_function_from_reflection_objects(old_fe TSRMLS_CC);
			zend_hash_del(&ce->function_table, newname_lower, newname_lower_len + 1);
		}
	}

	ancestor_class = php_runkit_locate_scope(ce, fe, methodname_lower, methodname_lower_len);
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_clean_children_methods, 5,
				       ancestor_class, ce, methodname_lower, methodname_lower_len, fe);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif

	func = *fe;
	php_runkit_function_copy_ctor(&func, newname, newname_len TSRMLS_CC);

	if (zend_hash_add(&ce->function_table, newname_lower, newname_lower_len + 1, &func, sizeof(zend_function), NULL) == FAILURE) {
		efree(newname_lower);
		efree(methodname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add new reference to class method");
		zend_function_dtor(&func);
		RETURN_FALSE;
	}

	php_runkit_remove_function_from_reflection_objects(fe TSRMLS_CC);

	if (zend_hash_del(&ce->function_table, methodname_lower, methodname_lower_len + 1) == FAILURE) {
		efree(newname_lower);
		efree(methodname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to remove old method reference from class");
		RETURN_FALSE;
	}

	PHP_RUNKIT_DEL_MAGIC_METHOD(ce, fe TSRMLS_CC);

	if (php_runkit_fetch_class_method(classname, classname_len, newname, newname_len, &ce, &fe TSRMLS_CC) == FAILURE) {
		efree(newname_lower);
		efree(methodname_lower);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to locate newly renamed method");
		RETURN_FALSE;
	}

	fe->common.scope = ce;
	fe->common.prototype = _php_runkit_get_method_prototype(ce->parent, newname_lower, newname_lower_len TSRMLS_CC);;

	PHP_RUNKIT_ADD_MAGIC_METHOD(ce, newname_lower, newname_lower_len, fe, NULL TSRMLS_CC);
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_update_children_methods, 7,
				       ce, ce, fe, newname_lower, newname_lower_len, NULL, 0);

	efree(newname_lower);
	efree(methodname_lower);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool runkit_method_copy(string destclass, string destmethod, string srcclass[, string srcmethod])
	Copy a method from one name to another or from one class to another */
PHP_FUNCTION(runkit_method_copy)
{
	const char *dclass, *dfunc, *sclass, *sfunc = NULL;
	int dclass_len, dfunc_len, sclass_len, sfunc_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/s/|s/", &dclass,    &dclass_len,
	                                                                  &dfunc,     &dfunc_len,
	                                                                  &sclass,    &sclass_len,
	                                                                  &sfunc,     &sfunc_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (!sfunc) {
		sfunc = dfunc;
		sfunc_len = dfunc_len;
	}

	RETURN_BOOL(php_runkit_method_copy( dclass, dclass_len, dfunc, dfunc_len,
	                                    sclass, sclass_len, sfunc, sfunc_len TSRMLS_CC) == SUCCESS ? 1 : 0);
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

