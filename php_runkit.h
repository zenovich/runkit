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

#define PHP_RUNKIT_VERSION					"0.4"
#define PHP_RUNKIT_SANDBOX_CLASSNAME		"Runkit_Sandbox"
#define PHP_RUNKIT_SANDBOX_PROPNAME			"__sandbox"
#define PHP_RUNKIT_SANDBOX_OHPROPNAME		"__output_handler"
#define PHP_RUNKIT_SANDBOX_RESNAME			"Runkit Sandbox State Information"

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
PHP_FUNCTION(runkit_filter);

typedef struct _php_runkit_sandbox_data php_runkit_sandbox_data;

ZEND_BEGIN_MODULE_GLOBALS(runkit)
	HashTable *superglobals;
	php_runkit_sandbox_data *current_sandbox;
ZEND_END_MODULE_GLOBALS(runkit)

extern ZEND_DECLARE_MODULE_GLOBALS(runkit);

#ifdef ZTS
#define		RUNKIT_G(v)		TSRMG(runkit_globals_id, zend_runkit_globals *, v)
#else
#define		RUNKIT_G(v)		(runkit_globals.v)
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

#if PHP_MAJOR_VERSION > 4 || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION >= 3)
extern php_stream_wrapper *php_runkit_plainfiles_wrapper_ptr;
#endif

/* Expects: void
 * If a typecast is possible, convert, if not: default */
#define PHP_RUNKIT_FILTER_NULL				0x0001
#define PHP_RUNKIT_FILTER_BOOL				0x0002
#define PHP_RUNKIT_FILTER_INT				0x0003
#define PHP_RUNKIT_FILTER_FLOAT				0x0004
#define PHP_RUNKIT_FILTER_STRING			0x0005
#define PHP_RUNKIT_FILTER_ARRAY				0x0006
#define PHP_RUNKIT_FILTER_OBJECT			0x0007
#define PHP_RUNKIT_FILTER_RESOURCE			0x0008

/* Expects: classname */
#define PHP_RUNKIT_FILTER_INSTANCE			0x000F

/* Expects: void */
#define PHP_RUNKIT_FILTER_NONEMPTY			0x0010
#define PHP_RUNKIT_FILTER_ALPHA				0x0011
#define PHP_RUNKIT_FILTER_DIGIT				0x0012
#define PHP_RUNKIT_FILTER_NUMERIC			0x0013
#define PHP_RUNKIT_FILTER_ALNUM				0x0014
#define PHP_RUNKIT_FILTER_GPC_ON			0x0015
#define PHP_RUNKIT_FILTER_GPC_OFF			0x0016
#define PHP_RUNKIT_FILTER_IPV4				0x0017
#define PHP_RUNKIT_FILTER_IPV6				0x0018
#define PHP_RUNKIT_FILTER_IP				0x0019
#define PHP_RUNKIT_FILTER_HOSTNAME			0x001A
#define PHP_RUNKIT_FILTER_SCHEME_FILE		0x001B
#define PHP_RUNKIT_FILTER_SCHEME_ANY		0x001C
#define PHP_RUNKIT_FILTER_SCHEME_NETWORK	0x001D
#define PHP_RUNKIT_FILTER_SCHEME_LOCAL		0x001E

/* Expects: string wrappernames (coma separated list of acceptable) */
#define PHP_RUNKIT_FILTER_SCHEME			0x001F
/* Expects: callback (callback returns bool) */
#define PHP_RUNKIT_FILTER_USER				0x0020
/* Expects: callback (callback returns result) */
#define PHP_RUNKIT_FILTER_INLINE			0x0021

/* Expects: string pattern */
#define PHP_RUNKIT_FILTER_EREG				0x0022
#define PHP_RUNKIT_FILTER_EREGI				0x0023
#define PHP_RUNKIT_FILTER_PREG				0x0024

#endif	/* PHP_RUNKIT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
