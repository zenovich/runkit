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
#include "main/php_streams.h"

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long int) -1)
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if PHP_MAJOR_VERSION > 4 || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION >= 3)
extern php_stream_wrapper php_plain_files_wrapper;
#endif

static int php_runkit_filter_parse_parameters(int argc TSRMLS_DC, HashTable **symtable, char **variable, int *variable_len, zval **def, zval ****filters)
{
	zval ***params;

	if (argc < 3) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected at least 3 parameters, %d given", argc);
		return FAILURE;
	}

	params = safe_emalloc(argc, sizeof(zval**), 0);

	if (zend_get_parameters_array_ex(argc, params) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed parsing argumentings");
		efree(params);
		return FAILURE;
	}

	/* Symbol Table */
	if (Z_TYPE_PP(params[0]) == IS_ARRAY) {
		*symtable = Z_ARRVAL_PP(params[0]);
	} else if (Z_TYPE_PP(params[0]) == IS_NULL) {
		*symtable = EG(active_symbol_table);
	} else {
		efree(params);
		return FAILURE;
	}

	/* Index */
	convert_to_string(*(params[1]));
	*variable = Z_STRVAL_PP(params[1]);
	*variable_len = Z_STRLEN_PP(params[1]);

	/* Default */
	*def = *(params[2]);

	*filters = params;

	return SUCCESS;
}

/* {{{ proto mixed runkit_filter(array &symbol_table, string varname, mixed default, ...)
Apply one or more filters to a variable */
PHP_FUNCTION(runkit_filter)
{
	HashTable *symtable;
	char *varname;
	int varname_len;
	zval *def, **pvar, *var = NULL;
	zval **filters, ***f;
	int argc = ZEND_NUM_ARGS();
	int num_filters = argc - 3;

	if (php_runkit_filter_parse_parameters(argc TSRMLS_CC, &symtable, &varname, &varname_len, &def, &f) == FAILURE) {
		return;
	}
	filters = f[3];

	/* Basic isset() check */
	if (zend_hash_find(symtable, varname, varname_len + 1, (void **)&pvar) == FAILURE) {
		goto use_default;
	}
	var = *pvar;

	if (var->refcount > 1 && !var->is_ref) {
		zval *copyval;

		*copyval = *var;
		zval_copy_ctor(copyval);
		zend_hash_del(symtable, varname, varname_len + 1);
		copyval->refcount = 1;
		copyval->is_ref = 0;
		zend_hash_add(symtable, varname, varname_len + 1, (void*)&copyval, sizeof(zval*), NULL);
	}

	while (num_filters) {
		char handled = 1;

		convert_to_long(filters[0]);

		/* No param */
		switch (Z_LVAL_P(filters[0])) {
			case PHP_RUNKIT_FILTER_NULL: /* For completeness */
				convert_to_null(var);
				break;
			case PHP_RUNKIT_FILTER_BOOL:
				convert_to_boolean(var);
				break;
			case PHP_RUNKIT_FILTER_INT:
				convert_to_long(var);
				break;
			case PHP_RUNKIT_FILTER_FLOAT:
				convert_to_double(var);
				break;
			case PHP_RUNKIT_FILTER_STRING:
				convert_to_string(var);
				break;
			case PHP_RUNKIT_FILTER_ARRAY:
				if (Z_TYPE_P(var) == IS_ARRAY || Z_TYPE_P(var) == IS_OBJECT || Z_TYPE_P(var) == IS_NULL) {
					convert_to_array(var);
					break;
				}
				goto use_default;
			case PHP_RUNKIT_FILTER_OBJECT:
				if (Z_TYPE_P(var) == IS_ARRAY || Z_TYPE_P(var) == IS_OBJECT || Z_TYPE_P(var) == IS_NULL) {
					convert_to_object(var);
					break;
				}
				goto use_default;
			case PHP_RUNKIT_FILTER_RESOURCE:
				if (Z_TYPE_P(var) == IS_RESOURCE) {
					break;
				}
				goto use_default;
			case PHP_RUNKIT_FILTER_NONEMPTY:
				if (zend_is_true(var)) {
					break;
				}
				goto use_default;
			case PHP_RUNKIT_FILTER_ALPHA:
			{
				int i;

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				for(i = 0; i < Z_STRLEN_P(var); i++) {
					if ((Z_STRVAL_P(var)[i] >= 'a' && Z_STRVAL_P(var)[i] <= 'z') ||
						(Z_STRVAL_P(var)[i] >= 'A' && Z_STRVAL_P(var)[i] <= 'Z')) {
						continue;
					}
					goto use_default;
				}
			}
			case PHP_RUNKIT_FILTER_DIGIT:
			{
				int i;

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				for(i = 0; i < Z_STRLEN_P(var); i++) {
					if ((Z_STRVAL_P(var)[i] >= '0' && Z_STRVAL_P(var)[i] <= '9')) {
						continue;
					}
					goto use_default;
				}
			}
			case PHP_RUNKIT_FILTER_NUMERIC:
			{
				int i, gotdot = 0;

				if (Z_TYPE_P(var) == IS_LONG || Z_TYPE_P(var) == IS_DOUBLE) {
					break;
				}

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				for(i = 0; i < Z_STRLEN_P(var); i++) {
					if (i == 0 && (Z_STRVAL_P(var)[0] == '-' || Z_STRVAL_P(var)[0] == '+')) {
						continue;
					}
					if (!gotdot && Z_STRVAL_P(var)[i] == '.') {
						gotdot = 1;
						continue;
					}
					if (Z_STRVAL_P(var)[i] >= '0' && Z_STRVAL_P(var)[i] <= '9') {
						continue;
					}
					goto use_default;
				}
			}
			case PHP_RUNKIT_FILTER_ALNUM:
			{
				int i;

				if (Z_TYPE_P(var) == IS_LONG || Z_TYPE_P(var) == IS_DOUBLE) {
					break;
				}

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				for(i = 0; i < Z_STRLEN_P(var); i++) {
					if ((Z_STRVAL_P(var)[i] >= 'a' && Z_STRVAL_P(var)[i] <= 'z') ||
						(Z_STRVAL_P(var)[i] >= 'A' && Z_STRVAL_P(var)[i] <= 'Z') ||
						(Z_STRVAL_P(var)[i] >= '0' && Z_STRVAL_P(var)[i] <= '9')) {
						continue;
					}
					goto use_default;
				}
			}
			case PHP_RUNKIT_FILTER_GPC_ON:
				if (Z_TYPE_P(var) != IS_STRING ||
					PG(magic_quotes_gpc)) {
					break;
				}
				Z_STRVAL_P(var) = php_addslashes(Z_STRVAL_P(var), Z_STRLEN_P(var), &Z_STRLEN_P(var), 1 TSRMLS_CC);
				break;
			case PHP_RUNKIT_FILTER_GPC_OFF:
				if (Z_TYPE_P(var) != IS_STRING ||
					!PG(magic_quotes_gpc)) {
					break;
				}
				php_stripslashes(Z_STRVAL_P(var), &Z_STRLEN_P(var) TSRMLS_CC);
				break;
			case PHP_RUNKIT_FILTER_IPV4:
			case PHP_RUNKIT_FILTER_IP:
			{
				unsigned long int ip;

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				if (((ip = inet_addr(Z_STRVAL_P(var))) == INADDR_NONE) &&
					memcmp(Z_STRVAL_P(var), "255.255.255.255", Z_STRLEN_P(var))) {
					goto use_default;
				}
				if (Z_LVAL_P(filters[0]) == PHP_RUNKIT_FILTER_IPV4) {
					break;
				}
				/* FALLTHROUGH */
			}
			case PHP_RUNKIT_FILTER_IPV6:
				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}
#if defined(HAVE_IPV6) && defined(HAVE_INET_PTON)
			{
				char buffer[16];
				int ret;

				ret = inet_pton(AF_INET6, Z_STRVAL_P(var), buffer);
				if (ret <= 0) {
					goto use_default;
				}
				break;
			}
#else
/* TODO: Fallback manual parser */
				goto use_default;
#endif /* FILTER_IPV6 */
			case PHP_RUNKIT_FILTER_HOSTNAME:
			{
				struct hostent *hp;

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				hp = gethostbyname(Z_STRVAL_P(var));
				if (!hp || !*(hp->h_addr_list)) {
					goto use_default;
				}
				break;
			}
#if PHP_MAJOR_VERSION > 4 || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION >= 3)
			case PHP_RUNKIT_FILTER_SCHEME_FILE:
				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}
				if (strstr(Z_STRVAL_P(var), "://") &&
					strncasecmp(Z_STRVAL_P(var), "file://", sizeof("file://") - 1)) {
					goto use_default;
				}
				break;
			case PHP_RUNKIT_FILTER_SCHEME_ANY:
			{
				php_stream_wrapper *wrapper;

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				wrapper = php_stream_locate_url_wrapper(Z_STRVAL_P(var), NULL, 0 TSRMLS_CC);
				if (!wrapper || wrapper == &php_plain_files_wrapper) {
					goto use_default;
				}
				break;
			}
			case PHP_RUNKIT_FILTER_SCHEME_NETWORK:
			case PHP_RUNKIT_FILTER_SCHEME_LOCAL:
			{
				php_stream_wrapper *wrapper;

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}

				wrapper = php_stream_locate_url_wrapper(Z_STRVAL_P(var), NULL, 0 TSRMLS_CC);
				if (!wrapper) {
					goto use_default;
				}

				if ((Z_LVAL_P(filters[0]) == PHP_RUNKIT_FILTER_SCHEME_NETWORK && !wrapper->is_url) ||
					(Z_LVAL_P(filters[0]) == PHP_RUNKIT_FILTER_SCHEME_LOCAL   &&  wrapper->is_url)) {
					goto use_default;
				}
				break;
			}
#endif
			default:
				handled = 0;
		}

		if (handled) {
			filters++;
			num_filters--;
			continue;
		}

		/* Need 1 param */
		if (num_filters < 2) {
			goto use_default;
		}
		handled = 1;
		switch (Z_LVAL_P(filters[0])) {
			case PHP_RUNKIT_FILTER_INSTANCE:
				if (Z_TYPE_P(var) != IS_OBJECT ||
					Z_TYPE_P(filters[1]) != IS_STRING) {
					goto use_default;
				}
				if (strcasecmp(Z_OBJCE_P(var)->name, Z_STRVAL_P(filters[1]))) {
					goto use_default;
				}
				break;
#if PHP_MAJOR_VERSION > 4 || (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION >= 3)
			case PHP_RUNKIT_FILTER_SCHEME:
			{
				php_stream_wrapper *wrapper;
				char *wrapper_name;
				int wrapper_name_len;

				if (Z_TYPE_P(var) != IS_STRING) {
					goto use_default;
				}
				if (Z_TYPE_P(filters[1]) != IS_STRING && Z_TYPE_P(filters[1]) != IS_ARRAY) {
					goto use_default;
				}

				wrapper = php_stream_locate_url_wrapper(Z_STRVAL_P(var), NULL, 0 TSRMLS_CC);
				if (!wrapper) {
					goto use_default;
				}

				wrapper_name = wrapper->wops->label;
				wrapper_name_len = strlen(wrapper_name);

				if (Z_TYPE_P(filters[1]) == IS_STRING) {
					if (Z_STRLEN_P(filters[1]) != wrapper_name_len ||
						strncasecmp(Z_STRVAL_P(filters[1]), wrapper_name, wrapper_name_len)) {
						goto use_default;
					}
				} else if (Z_TYPE_P(filters[1]) == IS_ARRAY) {
					HashPosition pos;
					zval **tmpzval;

					for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(filters[1]), &pos);
						zend_hash_get_current_data_ex(Z_ARRVAL_P(filters[1]), (void**)&tmpzval, &pos) == SUCCESS;
						zend_hash_move_forward_ex(Z_ARRVAL_P(filters[1]), &pos)) {
						if (Z_TYPE_PP(tmpzval) == IS_STRING &&
							Z_STRLEN_PP(tmpzval) == wrapper_name_len &&
							strncasecmp(Z_STRVAL_PP(tmpzval), wrapper_name, wrapper_name_len) == 0) {
							goto scheme_loop_exit;
						}
					}
					goto use_default;
				} else {
					goto use_default;
				}
 scheme_loop_exit:
				break;
			}
#endif
			case PHP_RUNKIT_FILTER_USER:
			{
				zval *retval = NULL;

				if (call_user_function_ex(EG(function_table), NULL, filters[1], &retval, 1, &pvar, 0, NULL TSRMLS_CC) == FAILURE ||
					!retval || !zval_is_true(retval)) {
					if (retval) {
						zval_ptr_dtor(&retval);
					}
					goto use_default;
				}
				zval_ptr_dtor(&retval);
				break;
			}
			case PHP_RUNKIT_FILTER_INLINE:
			{
				/* Might be borked? */
				zval *retval = NULL;

				if (call_user_function_ex(EG(function_table), NULL, filters[1], &retval, 1, &pvar, 0, NULL TSRMLS_CC) == FAILURE || !retval) {
					goto use_default;
				}
				convert_to_null(var);
				var->type = retval->type;
				var->value = retval->value;
				zval_copy_ctor(var);
				zval_ptr_dtor(&retval);
				break;
			}
			case PHP_RUNKIT_FILTER_EREG:
			case PHP_RUNKIT_FILTER_EREGI:
			{
				regex_t re;
				int sensitivity = (Z_LVAL_P(filters[0]) == PHP_RUNKIT_FILTER_EREGI) ? REG_ICASE : 0;

				convert_to_string(var);
				convert_to_string(filters[1]);
				if (regcomp(&re, Z_STRVAL_P(filters[1]), REG_EXTENDED | REG_NOSUB | sensitivity)) {
					goto use_default;
				}
				if (regexec(&re, Z_STRVAL_P(var), 0, NULL, 0)) {
					regfree(&re);
					goto use_default;
				}
				regfree(&re);
				break;
			}
			case PHP_RUNKIT_FILTER_PREG:
			{
				/* The word you're looking for is "lazy" */
				zval **args[2], *zpreg_match;
				zval *retval;

				MAKE_STD_ZVAL(zpreg_match);
				ZVAL_STRINGL(zpreg_match, "preg_match", sizeof("preg_match") - 1, 1);

				args[0] = &filters[1];
				args[1] = &var;

				if (call_user_function_ex(EG(function_table), NULL, zpreg_match, &retval, 2, args, 0, NULL TSRMLS_CC) == FAILURE ||
					!retval || !zval_is_true(retval)) {
					zval_ptr_dtor(&zpreg_match);
					if (retval) {
						zval_ptr_dtor(&retval);
					}
					goto use_default;
				}
				zval_ptr_dtor(&zpreg_match);
				zval_ptr_dtor(&retval);
				break;
			}
			default:
				handled = 0;
		}

		if (handled) {
			filters += 2;
			num_filters -= 2;
			continue;
		}

		/* Unrecognized filter */
		goto use_default;
	}

	efree(f);

	*return_value = *var;
	zval_copy_ctor(return_value);
	return_value->is_ref = 0;
	return_value->refcount = 1;

	return;

 use_default:
	efree(f);

	if (var) {
		if (var->is_ref) {
			long refcount = var->refcount;

			var->refcount = 1;
			zval_dtor(var);
			*var = *def;
			zval_copy_ctor(var);
			var->refcount = refcount;
			var->is_ref = 1;
		} else {
			zend_hash_del(symtable, varname, varname_len + 1);
			def->refcount++;
			zend_hash_add(symtable, varname, varname_len + 1, (void*)&def, sizeof(zval*), NULL);
			var = def;
		}
	} else {
		def->refcount++;
		zend_hash_add(symtable, varname, varname_len + 1, (void*)&def, sizeof(zval*), NULL);
		var = def;
	}

	*return_value = *var;
	zval_copy_ctor(return_value);
	return_value->is_ref = 0;
	return_value->refcount = 1;
}
/* }}} */
