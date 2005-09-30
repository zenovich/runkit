/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
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

#ifndef PHP_RUNKIT_H
#define PHP_RUNKIT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

#define PHP_RUNKIT_VERSION					"0.6"
#define PHP_RUNKIT_SANDBOX_CLASSNAME		"Runkit_Sandbox"
#define PHP_RUNKIT_SANDBOX_PARENT_CLASSNAME	"Runkit_Sandbox_Parent"

#define PHP_RUNKIT_IMPORT_FUNCTIONS			0x0001
#define PHP_RUNKIT_IMPORT_CLASS_METHODS		0x0002
#define PHP_RUNKIT_IMPORT_CLASS_CONSTS		0x0004
#define PHP_RUNKIT_IMPORT_CLASS_PROPS		0x0008
#define PHP_RUNKIT_IMPORT_CLASSES			(PHP_RUNKIT_IMPORT_CLASS_METHODS|PHP_RUNKIT_IMPORT_CLASS_CONSTS|PHP_RUNKIT_IMPORT_CLASS_PROPS)
#define PHP_RUNKIT_IMPORT_OVERRIDE			0x0010

/* The TSRM interpreter patch required by runkit_sandbox was added in 5.1, but this package includes diffs for older versions
 * Those diffs include an additional #define to indicate that they've been applied
 */
#if defined(ZTS) && (PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 0) || defined(TSRM_INTERPRETER_PATCH_APPLIED))
#define PHP_RUNKIT_SANDBOX
#endif

/* Superglobals don't exist until PHP 4.2 */
#if PHP_MAJOR_VERSION > 4 || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION > 1)
#define PHP_RUNKIT_SUPERGLOBALS
#endif

extern zend_module_entry runkit_module_entry;
#define phpext_runkit_ptr &runkit_module_entry

PHP_MINIT_FUNCTION(runkit);
PHP_MSHUTDOWN_FUNCTION(runkit);
PHP_RINIT_FUNCTION(runkit);
PHP_RSHUTDOWN_FUNCTION(runkit);
PHP_MINFO_FUNCTION(runkit);
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
PHP_FUNCTION(runkit_class_emancipate);
PHP_FUNCTION(runkit_class_adopt);
PHP_FUNCTION(runkit_import);
#ifdef ZEND_ENGINE_2
PHP_FUNCTION(runkit_object_id);
#endif
#ifdef PHP_RUNKIT_SANDBOX
PHP_FUNCTION(runkit_sandbox_output_handler);
PHP_FUNCTION(runkit_lint);
PHP_FUNCTION(runkit_lint_file);
#endif

typedef struct _php_runkit_sandbox_object php_runkit_sandbox_object;

ZEND_BEGIN_MODULE_GLOBALS(runkit)
	HashTable *superglobals;
	php_runkit_sandbox_object *current_sandbox;
	HashTable *misplaced_internal_functions;
	HashTable *replaced_internal_functions;
	zend_bool internal_override;
ZEND_END_MODULE_GLOBALS(runkit)

extern ZEND_DECLARE_MODULE_GLOBALS(runkit);

#ifdef ZTS
#define		RUNKIT_G(v)		TSRMG(runkit_globals_id, zend_runkit_globals *, v)
#define RUNKIT_TSRMLS_C		TSRMLS_C
#else
#define		RUNKIT_G(v)		(runkit_globals.v)
#define RUNKIT_TSRMLS_C		NULL
#endif

#if defined(ZEND_ENGINE_2) && !defined(zend_hash_add_or_update)
/* Why doesn't ZE2 define this? */
#define zend_hash_add_or_update(ht, arKey, nKeyLength, pData, pDataSize, pDest, flag) \
        _zend_hash_add_or_update((ht), (arKey), (nKeyLength), (pData), (pDataSize), (pDest), (flag) ZEND_FILE_LINE_CC)
#endif

/* runkit_functions.c */
#define RUNKIT_TEMP_FUNCNAME  "__runkit_temporary_function__"
int php_runkit_check_call_stack(zend_op_array *op_array TSRMLS_DC);
void php_runkit_function_copy_ctor(zend_function *fe, char *newname);
int php_runkit_generate_lambda_method(char *arguments, int arguments_len, char *phpcode, int phpcode_len, zend_function **pfe TSRMLS_DC);
int php_runkit_destroy_misplaced_functions(zend_hash_key *hash_key TSRMLS_DC);
int php_runkit_restore_internal_functions(zend_internal_function *fe, int num_args, va_list args, zend_hash_key *hash_key);

/* runkit_methods.c */
int php_runkit_fetch_class(char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC);
int php_runkit_clean_children_methods(zend_class_entry *ce, int num_args, va_list args, zend_hash_key *hash_key);
int php_runkit_update_children_methods(zend_class_entry *ce, int num_args, va_list args, zend_hash_key *hash_key);
#ifdef ZEND_ENGINE_2
int php_runkit_fetch_interface(char *classname, int classname_len, zend_class_entry **pce TSRMLS_DC);
#endif

#ifdef ZEND_ENGINE_2
#define php_runkit_locate_scope(ce, fe, methodname, methodname_len)   fe->common.scope
#else
zend_class_entry *_php_runkit_locate_scope(zend_class_entry *ce, zend_function *fe, char *methodname, int methodname_len);
#define php_runkit_locate_scope(ce, fe, methodname, methodname_len)   _php_runkit_locate_scope((ce), (fe), (methodname), (methodname_len))

#define zend_function_dtor		destroy_zend_function
#endif

/* runkit_constants.c */
int php_runkit_update_children_consts(zend_class_entry *ce, int num_args, va_list args, zend_hash_key *hash_key);

#ifdef PHP_RUNKIT_SANDBOX
/* runkit_sandbox.c */
int php_runkit_init_sandbox(INIT_FUNC_ARGS);
int php_runkit_shutdown_sandbox(SHUTDOWN_FUNC_ARGS);

/* runkit_sandbox_parent.c */
int php_runkit_init_sandbox_parent(INIT_FUNC_ARGS);
int php_runkit_shutdown_sandbox_parent(SHUTDOWN_FUNC_ARGS);
int php_runkit_sandbox_array_deep_copy(zval **value, int num_args, va_list args, zend_hash_key *hash_key);

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
			zend_hash_apply_with_arguments(original_hashtable, (apply_func_args_t)php_runkit_sandbox_array_deep_copy, 1, Z_ARRVAL_P(pzv) TSRMLS_CC); \
			break; \
		} \
		default: \
			zval_copy_ctor(pzv); \
	} \
	(pzv)->refcount = 1; \
	(pzv)->is_ref = 0; \
}
#endif

#define PHP_RUNKIT_SPLIT_PN(classname, classname_len, pnname, pnname_len) { \
	char *colon; \
\
	if ((colon = memchr((pnname), ':', (pnname_len) - 2)) && (colon[1] == ':')) { \
		(classname) = (pnname); \
		(classname_len) = colon - (classname); \
		(classname)[(classname_len)] = '\0'; \
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
#else
#define PHP_RUNKIT_ADD_MAGIC_METHOD(ce, method, fe)
#define PHP_RUNKIT_DEL_MAGIC_METHOD(ce, fe)
#endif

#endif	/* PHP_RUNKIT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
