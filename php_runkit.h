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

#ifndef PHP_RUNKIT_H
#define PHP_RUNKIT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION >= 6)
#include "Zend/zend_closures.h"
#endif

#define PHP_RUNKIT_VERSION					"1.0.4-dev"
#define PHP_RUNKIT_SANDBOX_CLASSNAME		"Runkit_Sandbox"
#define PHP_RUNKIT_SANDBOX_PARENT_CLASSNAME	"Runkit_Sandbox_Parent"

#define PHP_RUNKIT_IMPORT_FUNCTIONS                 0x0001
#define PHP_RUNKIT_IMPORT_CLASS_METHODS             0x0002
#define PHP_RUNKIT_IMPORT_CLASS_CONSTS              0x0004
#define PHP_RUNKIT_IMPORT_CLASS_PROPS               0x0008
#define PHP_RUNKIT_IMPORT_CLASS_STATIC_PROPS        0x0010
#define PHP_RUNKIT_IMPORT_CLASSES                   (PHP_RUNKIT_IMPORT_CLASS_METHODS|PHP_RUNKIT_IMPORT_CLASS_CONSTS|PHP_RUNKIT_IMPORT_CLASS_PROPS|PHP_RUNKIT_IMPORT_CLASS_STATIC_PROPS)
#define PHP_RUNKIT_IMPORT_OVERRIDE                  0x0020

#if ZEND_MODULE_API_NO > 20050922
#define ZEND_ENGINE_2_2
#endif
#if ZEND_MODULE_API_NO > 20050921
#define ZEND_ENGINE_2_1
#endif

/* The TSRM interpreter patch required by runkit_sandbox was added in 5.1, but this package includes diffs for older versions
 * Those diffs include an additional #define to indicate that they've been applied
 */
#if (defined(ZTS) && (PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 0) || defined(TSRM_INTERPRETER_PATCH_APPLIED))) && defined(PHP_RUNKIT_FEATURE_SANDBOX)
#define PHP_RUNKIT_SANDBOX
#endif

/* Superglobals don't exist until PHP 4.2 */
#if (PHP_MAJOR_VERSION > 4 || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION > 1)) && defined(PHP_RUNKIT_FEATURE_SUPER)
#define PHP_RUNKIT_SUPERGLOBALS
#endif

#ifdef PHP_RUNKIT_FEATURE_MODIFY
#define PHP_RUNKIT_MANIPULATION
#endif

extern zend_module_entry runkit_module_entry;
#define phpext_runkit_ptr &runkit_module_entry

PHP_MINIT_FUNCTION(runkit);
PHP_MSHUTDOWN_FUNCTION(runkit);
PHP_RINIT_FUNCTION(runkit);
PHP_RSHUTDOWN_FUNCTION(runkit);
PHP_MINFO_FUNCTION(runkit);

PHP_FUNCTION(runkit_return_value_used);
#ifdef ZEND_ENGINE_2
PHP_FUNCTION(runkit_object_id);
#endif

#ifdef PHP_RUNKIT_MANIPULATION
PHP_FUNCTION(runkit_function_add);
PHP_FUNCTION(runkit_function_remove);
PHP_FUNCTION(runkit_function_rename);
PHP_FUNCTION(runkit_function_redefine);
PHP_FUNCTION(runkit_function_copy);
PHP_FUNCTION(runkit_method_add);
PHP_FUNCTION(runkit_method_redefine);
PHP_FUNCTION(runkit_method_remove);
PHP_FUNCTION(runkit_method_rename);
PHP_FUNCTION(runkit_method_copy);
PHP_FUNCTION(runkit_constant_redefine);
PHP_FUNCTION(runkit_constant_remove);
PHP_FUNCTION(runkit_constant_add);
PHP_FUNCTION(runkit_default_property_add);
PHP_FUNCTION(runkit_default_property_remove);
PHP_FUNCTION(runkit_class_emancipate);
PHP_FUNCTION(runkit_class_adopt);
PHP_FUNCTION(runkit_import);
#endif /* PHP_RUNKIT_MANIPULATION */

#ifdef PHP_RUNKIT_SANDBOX
PHP_FUNCTION(runkit_sandbox_output_handler);
PHP_FUNCTION(runkit_lint);
PHP_FUNCTION(runkit_lint_file);

typedef struct _php_runkit_sandbox_object php_runkit_sandbox_object;
#endif /* PHP_RUNKIT_SANDBOX */

#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_SANDBOX) || defined(PHP_RUNKIT_MANIPULATION)
ZEND_BEGIN_MODULE_GLOBALS(runkit)
#ifdef PHP_RUNKIT_SUPERGLOBALS
	HashTable *superglobals;
#endif
#ifdef PHP_RUNKIT_SANDBOX
	php_runkit_sandbox_object *current_sandbox;
#endif
#ifdef PHP_RUNKIT_MANIPULATION
	HashTable *misplaced_internal_functions;
	HashTable *replaced_internal_functions;
	zend_bool internal_override;
#endif
ZEND_END_MODULE_GLOBALS(runkit)
#endif

extern ZEND_DECLARE_MODULE_GLOBALS(runkit);

#ifdef ZTS
#define		RUNKIT_G(v)		TSRMG(runkit_globals_id, zend_runkit_globals *, v)
#define RUNKIT_TSRMLS_C		TSRMLS_C
#else
#define		RUNKIT_G(v)		(runkit_globals.v)
#define RUNKIT_TSRMLS_C		NULL
#endif

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION >= 6)
#     define RUNKIT_REFCOUNT  refcount__gc
#     define RUNKIT_IS_REF    is_ref__gc
#     define RUNKIT_IS_CALLABLE(cb_zv, flags, cb_sp) zend_is_callable((cb_zv), (flags), (cb_sp) TSRMLS_CC)
#     define RUNKIT_FILE_HANDLE_DTOR(pHandle)        zend_file_handle_dtor((pHandle) TSRMLS_CC)
#     define RUNKIT_53_TSRMLS_PARAM(param)           (param) TSRMLS_CC
#     define RUNKIT_53_TSRMLS_ARG(arg)               arg TSRMLS_DC
#     define RUNKIT_UNDER53_TSRMLS_FETCH()
#     define RUNKIT_UNDER53                          0
#     define RUNKIT_ABOVE53                          1
#else
#     define RUNKIT_REFCOUNT  refcount
#     define RUNKIT_IS_REF    is_ref
#     define RUNKIT_IS_CALLABLE(cb_zv, flags, cb_sp) zend_is_callable((cb_zv), (flags), (cb_sp))
#     define RUNKIT_FILE_HANDLE_DTOR(pHandle)        zend_file_handle_dtor((pHandle))
#     define RUNKIT_53_TSRMLS_PARAM(param)           (param)
#     define RUNKIT_53_TSRMLS_ARG(arg)               arg
#     define RUNKIT_UNDER53_TSRMLS_FETCH()           TSRMLS_FETCH()
#     define RUNKIT_UNDER53                          1
#     define RUNKIT_ABOVE53                          0
#     define zend_hash_quick_del(ht, key, key_len, h) zend_hash_del(ht, key, key_len)
#endif

#ifndef ALLOC_PERMANENT_ZVAL
# define ALLOC_PERMANENT_ZVAL(z) \
    (z) = (zval*)malloc(sizeof(zval))
#endif

#ifdef PHP_RUNKIT_MANIPULATION
#if defined(ZEND_ENGINE_2) && !defined(zend_hash_add_or_update)
/* Why doesn't ZE2 define this? */
#define zend_hash_add_or_update(ht, arKey, nKeyLength, pData, pDataSize, pDest, flag) \
        _zend_hash_add_or_update((ht), (arKey), (nKeyLength), (pData), (pDataSize), (pDest), (flag) ZEND_FILE_LINE_CC)
#endif

#ifndef Z_ADDREF_P
#     define Z_ADDREF_P(x)                           ZVAL_ADDREF(x)
#endif

/* runkit_functions.c */
#define RUNKIT_TEMP_FUNCNAME  "__runkit_temporary_function__"
int php_runkit_check_call_stack(zend_op_array *op_array TSRMLS_DC);
void php_runkit_function_copy_ctor(zend_function *fe, const char *newname, int newname_len TSRMLS_DC);
int php_runkit_generate_lambda_method(const char *arguments, int arguments_len, const char *phpcode, int phpcode_len, zend_function **pfe TSRMLS_DC);
int php_runkit_destroy_misplaced_functions(void *pDest TSRMLS_DC);
int php_runkit_restore_internal_functions(RUNKIT_53_TSRMLS_ARG(void *pDest), int num_args, va_list args, zend_hash_key *hash_key);
int php_runkit_clean_zval(zval **val TSRMLS_DC);

/* runkit_methods.c */
int php_runkit_fetch_class(const char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC);
int php_runkit_fetch_class_int(const char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC);
int php_runkit_clean_children_methods(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key);
int php_runkit_update_children_methods(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key);
#ifdef ZEND_ENGINE_2
int php_runkit_fetch_interface(const char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC);
#endif

#if PHP_MAJOR_VERSION >= 6
#define PHP_RUNKIT_FUNCTION_ADD_REF(f)	function_add_ref(f TSRMLS_CC)
#define php_runkit_locate_scope(ce, fe, methodname, methodname_len)   fe->common.scope
#define PHP_RUNKIT_DECL_STRING_PARAM(param)		void *param; int32_t param##_len; zend_uchar param##_type;
#define PHP_RUNKIT_STRING_SPEC					"t"
#define PHP_RUNKIT_STRING_PARAM(param)			&param, &param##_len, &param##_type
#define PHP_RUNKIT_STRTOLOWER(param)			php_u_strtolower(param, &param##_len, UG(default_locale))
#define PHP_RUNKIT_STRING_LEN(param,addtl)		(param##_type == IS_UNICODE ? UBYTES(param##_len + (addtl)) : (param##_len + (addtl)))
#define PHP_RUNKIT_STRING_TYPE(param)			(param##_type)
#define PHP_RUNKIT_HASH_FIND(hash,param,ppvar)	zend_u_hash_find(hash, param##_type, (UChar *)param, param##_len + 1, (void**)ppvar)
#define PHP_RUNKIT_HASH_EXISTS(hash,param)		zend_u_hash_exists(hash, param##_type, (UChar *)param, param##_len + 1)
#define PHP_RUNKIT_HASH_KEY(hash_key)			((hash_key)->type == HASH_KEY_IS_UNICODE ? (hash_key)->u.unicode : (hash_key)->u.string)
#define PHP_RUNKIT_HASH_KEYLEN(hash_key)		((hash_key)->type == HASH_KEY_IS_UNICODE ? UBYTES((hash_key)->nKeyLength) : (hash_key)->nKeyLength)

#elif PHP_MAJOR_VERSION >= 5
#define PHP_RUNKIT_FUNCTION_ADD_REF(f)	function_add_ref(f)
#define php_runkit_locate_scope(ce, fe, methodname, methodname_len)   fe->common.scope
#define PHP_RUNKIT_DECL_STRING_PARAM(p)			char *p; int p##_len;
#define PHP_RUNKIT_STRING_SPEC					"s"
#define PHP_RUNKIT_STRING_PARAM(p)				&p, &p##_len
#define PHP_RUNKIT_STRTOLOWER(p)				php_strtolower(p, p##_len)
#define PHP_RUNKIT_STRING_LEN(param,addtl)		(param##_len + (addtl))
#define PHP_RUNKIT_STRING_TYPE(param)			IS_STRING
#define PHP_RUNKIT_HASH_FIND(hash,param,ppvar)	zend_hash_find(hash, param, param##_len + 1, (void**)ppvar)
#define PHP_RUNKIT_HASH_EXISTS(hash,param)		zend_hash_exists(hash, param##_type, param, param##_len + 1)
#define PHP_RUNKIT_HASH_KEY(hash_key)			((hash_key)->arKey)
#define PHP_RUNKIT_HASH_KEYLEN(hash_key)		((hash_key)->nKeyLength)

#else /* PHP4 */
#define PHP_RUNKIT_FUNCTION_ADD_REF(f)			function_add_ref(f)
zend_class_entry *_php_runkit_locate_scope(zend_class_entry *ce, zend_function *fe, const char *methodname, int methodname_len TSRMLS_DC);
#define php_runkit_locate_scope(ce, fe, 		methodname, methodname_len)   _php_runkit_locate_scope((ce), (fe), (methodname), (methodname_len) TSRMLS_CC)
#define PHP_RUNKIT_DECL_STRING_PARAM(p)			char *p; int p##_len;
#define PHP_RUNKIT_STRING_SPEC					"s"
#define PHP_RUNKIT_STRING_PARAM(p)				&p, &p##_len
#define PHP_RUNKIT_STRTOLOWER(p)				php_strtolower(p, p##_len)
#define PHP_RUNKIT_STRING_LEN(param,addtl)		(param##_len + (addtl))
#define PHP_RUNKIT_STRING_TYPE(param)			IS_STRING
#define PHP_RUNKIT_HASH_FIND(hash,param,ppvar)	zend_hash_find(hash, param, param##_len + 1, (void**)ppvar)
#define PHP_RUNKIT_HASH_EXISTS(hash,param)		zend_hash_exists(hash, param##_type, param, param##_len + 1)
#define PHP_RUNKIT_HASH_KEY(hash_key)			((hash_key)->arKey)
#define PHP_RUNKIT_HASH_KEYLEN(hash_key)		((hash_key)->nKeyLength)
#define zend_function_dtor						destroy_zend_function

#endif /* Version Agnosticism */

/* runkit_constants.c */
int php_runkit_update_children_consts(RUNKIT_53_TSRMLS_ARG(void *pDest), int num_args, va_list args, zend_hash_key *hash_key);

/* runkit_props.c */
int php_runkit_update_children_def_props(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key);
int php_runkit_def_prop_add_int(zend_class_entry *ce, const char *propname, int propname_len, zval *copyval, long visibility, const char *doc_comment, int doc_comment_len, zend_class_entry *definer_class, int override TSRMLS_DC);
int php_runkit_def_prop_remove_int(zend_class_entry *ce, const char *propname, int propname_len, zend_class_entry *definer_class TSRMLS_DC);

typedef struct _zend_closure {
    zend_object    std;
    zend_function  func;
    HashTable     *debug_info;
} zend_closure;

#endif /* PHP_RUNKIT_MANIPULATION */

#ifdef PHP_RUNKIT_SANDBOX
/* runkit_sandbox.c */
int php_runkit_init_sandbox(INIT_FUNC_ARGS);
int php_runkit_shutdown_sandbox(SHUTDOWN_FUNC_ARGS);

/* runkit_sandbox_parent.c */
int php_runkit_init_sandbox_parent(INIT_FUNC_ARGS);
int php_runkit_shutdown_sandbox_parent(SHUTDOWN_FUNC_ARGS);
int php_runkit_sandbox_array_deep_copy(RUNKIT_53_TSRMLS_ARG(zval **value), int num_args, va_list args, zend_hash_key *hash_key);

struct _php_runkit_sandbox_object {
	zend_object obj;

	void *context, *parent_context;

	char *disable_functions;
	char *disable_classes;
	zval *output_handler;					/* points to function which lives in the parent_context */

	unsigned char bailed_out_in_eval;		/* Patricide is an ugly thing.  Especially when it leaves bailout address mis-set */

	unsigned char active;					/* A bailout will set this to 0 */
	unsigned char parent_access;			/* May Runkit_Sandbox_Parent be instantiated/used? */
	unsigned char parent_read;				/* May parent vars be read? */
	unsigned char parent_write;				/* May parent vars be written to? */
	unsigned char parent_eval;				/* May arbitrary code be run in the parent? */
	unsigned char parent_include;			/* May arbitrary code be included in the parent? (includes require(), and *_once()) */
	unsigned char parent_echo;				/* May content be echoed from the parent scope? */
	unsigned char parent_call;				/* May functions in the parent scope be called? */
	unsigned char parent_die;				/* Are $PARENT->die() / $PARENT->exit() enabled? */
	unsigned long parent_scope;				/* 0 == Global, 1 == Active, 2 == Active->prior, 3 == Active->prior->prior, etc... */

	char *parent_scope_name;				/* Combines with parent_scope to refer to a named array as a symbol table */
	int parent_scope_namelen;
};


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
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(original_hashtable), (apply_func_args_t)php_runkit_sandbox_array_deep_copy, 1, Z_ARRVAL_P(pzv) TSRMLS_CC); \
			break; \
		} \
		default: \
			zval_copy_ctor(pzv); \
	} \
	(pzv)->RUNKIT_REFCOUNT = 1; \
	(pzv)->RUNKIT_IS_REF = 0; \
}
#endif /* PHP_RUNKIT_SANDBOX */

#ifdef PHP_RUNKIT_MANIPULATION
#define PHP_RUNKIT_SPLIT_PN(classname, classname_len, pnname, pnname_len) { \
	char *colon; \
\
	if ((colon = memchr((pnname), ':', (pnname_len) - 2)) && (colon[1] == ':')) { \
		(classname) = (pnname); \
		(classname_len) = colon - (classname); \
		(pnname) = colon + 2; \
		(pnname_len) -= (classname_len) + 2; \
	} else { \
		(classname) = NULL; \
		(classname_len) = 0; \
	} \
}

#ifdef ZEND_ENGINE_2
#define PHP_RUNKIT_ADD_MAGIC_METHOD(ce, method, fe) { \
	if ((strcmp((method), (ce)->name) == 0) || \
		(strcmp((method), "__construct") == 0)) {	(ce)->constructor	= (fe); (fe)->common.fn_flags = ZEND_ACC_CTOR; } \
	else if (strcmp((method), "__destruct") == 0) {	(ce)->destructor	= (fe); (fe)->common.fn_flags = ZEND_ACC_DTOR; } \
	else if (strcmp((method), "__clone") == 0)  {	(ce)->clone			= (fe); (fe)->common.fn_flags = ZEND_ACC_CLONE; } \
	else if (strcmp((method), "__get") == 0)		(ce)->__get			= (fe); \
	else if (strcmp((method), "__set") == 0)		(ce)->__set			= (fe); \
	else if (strcmp((method), "__call") == 0)		(ce)->__call		= (fe); \
}
#define PHP_RUNKIT_DEL_MAGIC_METHOD(ce, fe) { \
	if ((ce)->constructor == (fe))			(ce)->constructor	= NULL; \
	else if ((ce)->destructor == (fe))		(ce)->destructor	= NULL; \
	else if ((ce)->clone == (fe))			(ce)->clone			= NULL; \
	else if ((ce)->__get == (fe))			(ce)->__get			= NULL; \
	else if ((ce)->__set == (fe))			(ce)->__set			= NULL; \
	else if ((ce)->__call == (fe))			(ce)->__call		= NULL; \
}
#define PHP_RUNKIT_DESTROY_FUNCTION(fe) 	destroy_zend_function(fe TSRMLS_CC);
#else
#define PHP_RUNKIT_DESTROY_FUNCTION(fe) 	destroy_zend_function(fe);
#define PHP_RUNKIT_ADD_MAGIC_METHOD(ce, method, fe)
#define PHP_RUNKIT_DEL_MAGIC_METHOD(ce, fe)
#endif /* ZEND_ENGINE_2 */
#endif /* PHP_RUNKIT_MANIPULATION */

#endif	/* PHP_RUNKIT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
