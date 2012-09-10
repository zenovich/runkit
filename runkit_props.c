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

/* {{{ php_runkit_update_children_def_props
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_def_props(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	zval *p = va_arg(args, zval*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	int access_type = va_arg(args, int);
	zend_class_entry *definer_class = va_arg(args, zend_class_entry*);
	int override = va_arg(args, int);

	RUNKIT_UNDER53_TSRMLS_FETCH();

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	php_runkit_def_prop_add_int(ce, pname, pname_len, p, access_type, NULL, 0, definer_class, override TSRMLS_CC);
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_remove_children_def_props
	Scan the class_table for children of the class just removed */
int php_runkit_remove_children_def_props(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class =  va_arg(args, zend_class_entry*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	zend_class_entry *definer_class = va_arg(args, zend_class_entry*);

	RUNKIT_UNDER53_TSRMLS_FETCH();

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	php_runkit_def_prop_remove_int(ce, pname, pname_len, definer_class TSRMLS_CC);
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_def_prop_add_int */
int php_runkit_def_prop_add_int(zend_class_entry *ce, const char *propname, int propname_len, zval *copyval, long visibility,
				const char *doc_comment, int doc_comment_len, zend_class_entry *definer_class, int override TSRMLS_DC)
{
#if PHP_MAJOR_VERSION >= 5
	int i;
#endif
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3
	HashTable *symt;
#endif
#if PHP_MAJOR_VERSION >= 5
	zend_property_info *prop_info_ptr;
	long h = zend_get_hash_value((char *) propname, propname_len + 1);
#endif
	zval *pcopyval = copyval;

#if PHP_MAJOR_VERSION >= 5
	if ((visibility & ZEND_ACC_PRIVATE) && (visibility &  ZEND_ACC_STATIC) && definer_class && definer_class != ce) {
		return SUCCESS;
	}
	if (visibility & ZEND_ACC_STATIC) {
		if (definer_class == NULL || ce == definer_class) {
			SEPARATE_ARG_IF_REF(pcopyval);
		} else {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 2)
			pcopyval->is_ref = 1;
#else
			Z_SET_ISREF_P(pcopyval);
#endif
		}
	}
	Z_ADDREF_P(pcopyval);
#else
	Z_ADDREF_P(pcopyval);
#endif // PHP_MAJOR_VERSION >= 5

#if PHP_MAJOR_VERSION >= 5
	if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void **) &prop_info_ptr) == SUCCESS && !override) {
		zval_ptr_dtor(&pcopyval);
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s%s%s already exists, not importing",
		                 ce->name, (prop_info_ptr->flags & ZEND_ACC_STATIC) ? "::" : "->", propname);
		return FAILURE;
	}
#else
	if (zend_hash_exists(&ce->default_properties, (char *) propname, propname_len + 1)) {
		if (override) {
			if (zend_hash_del(&ce->default_properties, (char *) propname, propname_len + 1) == FAILURE) {
				zval_ptr_dtor(&pcopyval);
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Failed to remove property %s->%s, not importing",
				                 ce->name, propname);
				return FAILURE;
			}
		} else {
			zval_ptr_dtor(&pcopyval);
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s->%s already exists, not importing",
			                 ce->name, propname);
			return FAILURE;
		}
	}
	if (zend_hash_add(&ce->default_properties, (char *) propname, propname_len + 1, &pcopyval, sizeof(zval*), NULL) == FAILURE) {
		zval_ptr_dtor(&pcopyval);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to add property '%s' to class '%s'", propname, ce->name);
		return FAILURE;
	}
#endif // PHP_MAJOR_VERSION >= 5

#if PHP_MAJOR_VERSION >= 5
	if (zend_declare_property_ex(ce, (char *) propname, propname_len, pcopyval, visibility, (char *) doc_comment, doc_comment_len TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&pcopyval);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot declare new property");
		return FAILURE;
	}

	if (ce != definer_class) {
		if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void **) &prop_info_ptr) != SUCCESS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find just added property's info");
			return FAILURE;
		}
		if (visibility & ZEND_ACC_PRIVATE) {
			char *newkey;
			int newkey_len;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3) || (PHP_MAJOR_VERSION < 5)
			char *oldkey;
			int oldkey_len;
			zval **prop;
#endif
			zend_mangle_property_name(&newkey, &newkey_len, definer_class->name, definer_class->name_length, (char *) propname, propname_len, ce->type & ZEND_INTERNAL_CLASS);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3) || (PHP_MAJOR_VERSION < 5)
			zend_mangle_property_name(&oldkey, &oldkey_len, ce->name, ce->name_length, (char *) propname, propname_len, ce->type & ZEND_INTERNAL_CLASS);
			symt = (visibility & ZEND_ACC_STATIC) ? &ce->default_static_members : &ce->default_properties;
			if (zend_hash_find(symt, oldkey, oldkey_len + 1, (void **) &prop) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find just added property");
				return FAILURE;
			}
			Z_ADDREF_P(*prop);
			zend_hash_update(symt, newkey, newkey_len + 1, prop, sizeof(zval *), NULL);
			zend_hash_del(symt, oldkey, oldkey_len + 1);
			pefree((void *)oldkey, ce->type & ZEND_INTERNAL_CLASS);
#endif
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3
			pefree((void *)prop_info_ptr->name, ce->type & ZEND_INTERNAL_CLASS);
#endif
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			str_efree(prop_info_ptr->name);
#endif
			prop_info_ptr->name = estrndup(newkey, newkey_len);
			prop_info_ptr->name_length = newkey_len;
			prop_info_ptr->h = zend_get_hash_value(prop_info_ptr->name, prop_info_ptr->name_length + 1);
		}
		prop_info_ptr->ce = definer_class;
	}
#endif // PHP_MAJOR_VERSION >= 5
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
	                               (apply_func_args_t)php_runkit_update_children_def_props, 7, ce, copyval,
	                               propname, propname_len, visibility, definer_class, override);

#if PHP_MAJOR_VERSION >= 5
	if ((visibility & ZEND_ACC_STATIC) || !EG(objects_store).object_buckets) {
		return SUCCESS;
	}
	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		   EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == ce) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
				if (!object->properties_table) {
					object->properties_table = pemalloc(sizeof(void *) * ce->default_properties_count, 0);
				} else {
					object->properties_table = perealloc(object->properties_table, sizeof(void *) * ce->default_properties_count, 0);
				}
				object->properties_table[ce->default_properties_count-1] = ce->default_properties_table[ce->default_properties_count-1];
				if (object->properties_table[ce->default_properties_count-1]) {
					Z_ADDREF_P(object->properties_table[ce->default_properties_count-1]);
				}
#else
				if (!object->properties) {
					ALLOC_HASHTABLE(object->properties);
					zend_hash_init(object->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
				}
				SEPARATE_ARG_IF_REF(pcopyval);
				zend_hash_quick_del(object->properties, (char *) propname, propname_len + 1, h);
				zend_hash_quick_add(object->properties, (char *) propname, propname_len + 1, h, &pcopyval, sizeof(zval *), NULL);
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			}
		}
	}
#endif //PHP_MAJOR_VERSION >= 5

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_def_prop_add
 */
static int php_runkit_def_prop_add(char *classname, int classname_len, char *propname, int propname_len, zval *value, long visibility TSRMLS_DC)
{
	zend_class_entry *ce;
	zval *copyval;
	char *key = propname;
	int key_len = propname_len;
#ifdef ZEND_ENGINE_2
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3)
	char *temp;
	int temp_len;
#endif
#endif

	if (php_runkit_fetch_class_int(classname, classname_len, &ce TSRMLS_CC)==FAILURE) {
		return FAILURE;
	}

	if (ce->type & ZEND_INTERNAL_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Adding properties to internal classes is not allowed");
		return FAILURE;
	} else {
		copyval = value;
	}

	/* Check for existing property by this name */
	/* Existing public? */
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6)
	if (zend_hash_exists(&ce->properties_info, key, key_len + 1)) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
#else
	if (zend_hash_exists(&ce->default_properties, key, key_len + 1)) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION >= 6)

#if PHP_MAJOR_VERSION >= 5
	if (
	    Z_TYPE_P(copyval) == IS_CONSTANT_ARRAY
#if RUNKIT_ABOVE53
	    || (Z_TYPE_P(copyval) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#endif
	) {
		zval_update_constant_ex(&copyval, (void*) 1, ce TSRMLS_CC);
	}
#endif

#ifdef ZEND_ENGINE_2
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3)
	/* Existing Protected? */
	zend_mangle_property_name(&temp, &temp_len, "*", 1, propname, propname_len, 0);
	if (zend_hash_exists(&ce->default_properties, temp, temp_len + 1)) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		efree(temp);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
	efree(temp);

	/* Existing Private? */
	zend_mangle_property_name(&temp, &temp_len, classname, classname_len, propname, propname_len, 0);
	if (zend_hash_exists(&ce->default_properties, temp, temp_len + 1)) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		efree(temp);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s already exists", classname, propname);
		return FAILURE;
	}
	efree(temp);
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3)
#endif // ZEND_ENGINE_2
	if (php_runkit_def_prop_add_int(ce, propname, propname_len, copyval, visibility, NULL, 0, ce, 0 TSRMLS_CC) != SUCCESS) {
		zval_ptr_dtor(&copyval);
		FREE_ZVAL(copyval);
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_def_prop_remove */
int php_runkit_def_prop_remove_int(zend_class_entry *ce, const char *propname, int propname_len, zend_class_entry *definer_class TSRMLS_DC)
{
#if PHP_MAJOR_VERSION == 4
	/* Resolve the property's name */
	if (!zend_hash_exists(&ce->default_properties, (char *) propname, propname_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s does not exist", ce->name, propname);
		return FAILURE;
	}
	if (zend_hash_del(&ce->default_properties, (char *) propname, propname_len + 1) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property from class: %s::%s", ce->name, propname);
		return FAILURE;
	}
	zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_remove_children_def_props, 4, ce, propname, propname_len, NULL);
#else
	int i;
	long h;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	int offset;
#endif
	zend_property_info *property_info_ptr;

	h = zend_get_hash_value((char *) propname, propname_len + 1);
	if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void**)&property_info_ptr) == SUCCESS) {
		if (definer_class && property_info_ptr->ce != definer_class) {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
			char *private;
			int private_len;
			zend_mangle_property_name(&private, &private_len, definer_class->name, definer_class->name_length, (char *) propname, propname_len, 0);
			if (zend_hash_exists(&ce->default_properties, private, private_len + 1)) {
				zend_hash_del(&ce->default_properties, private, private_len + 1);
			}
			if (zend_hash_exists(&ce->default_static_members, private, private_len + 1)) {
				zend_hash_del(&ce->default_static_members, private, private_len + 1);
			}
			efree(private);
#endif // PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_remove_children_def_props,
			                               4, ce, propname, propname_len, definer_class);
			return SUCCESS;
		} else {
			definer_class = property_info_ptr->ce;
		}
		if (property_info_ptr->flags & ZEND_ACC_STATIC) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			zval_ptr_dtor(&ce->default_static_members_table[property_info_ptr->offset]);
			ce->default_static_members_table[property_info_ptr->offset] = NULL;
#else
			if (zend_hash_quick_del(&ce->default_static_members, property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property %s::%s", ce->name, propname);
				return FAILURE;
			}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		} else {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			zval_ptr_dtor(&ce->default_properties_table[property_info_ptr->offset]);
			ce->default_properties_table[property_info_ptr->offset] = NULL;
#else
			if (zend_hash_quick_del(&ce->default_properties, property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property %s->%s", ce->name, propname);
				return FAILURE;
			}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)

		}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		offset = property_info_ptr->offset;
#endif
		if (zend_hash_quick_del(&ce->properties_info, (char *) propname, propname_len + 1, h) != SUCCESS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property %s::%s", ce->name, propname);
			return FAILURE;
		}
		zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_remove_children_def_props,
		                               4, ce, propname, propname_len, definer_class);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s does not exist", ce->name, propname);
		return FAILURE;
	}

	if (!EG(objects_store).object_buckets) {
		return SUCCESS;
	}
	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		   EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == ce) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
				if (object->properties_table) {
					if (object->properties_table[offset]) {
						Z_DELREF_P(object->properties_table[offset]);
						if (Z_REFCOUNT_P(object->properties_table[offset]) == 0) {
							zval_ptr_dtor(&object->properties_table[offset]);
							FREE_ZVAL(object->properties_table[offset]);
						}
					}
					object->properties_table[offset] = NULL;
				}
#else
				if (object->properties) {
					zend_hash_del(object->properties, (char *) propname, propname_len + 1);
				}
#endif
			}
		}
	}
#endif // PHP_MAJOR_VERSION == 4
	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_def_prop_remove */
static int php_runkit_def_prop_remove(char *classname, int classname_len, char *propname, int propname_len TSRMLS_DC)
{
	zend_class_entry *ce;

	if (php_runkit_fetch_class_int(classname, classname_len, &ce TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	if (ce->type & ZEND_INTERNAL_CLASS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Removing properties from internal classes is not allowed");
		return FAILURE;
	}

	return php_runkit_def_prop_remove_int(ce, propname, propname_len, NULL TSRMLS_CC);
}
/* }}} */

/* ******************
   * Properties API *
   ****************** */

/* {{{ proto bool runkit_default_property_add(string classname, string propname, mixed initialvalue[, int visibility])
Add a property to a class with a given visibility
 */
PHP_FUNCTION(runkit_default_property_add)
{
	char *classname, *propname;
	int classname_len, propname_len;
	zval *value;
	long visibility;

#ifdef ZEND_ENGINE_2
	visibility = ZEND_ACC_PUBLIC;
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/z|l", &classname, &classname_len, &propname, &propname_len, &value, &visibility) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_BOOL(php_runkit_def_prop_add(classname, classname_len, propname, propname_len, value, visibility TSRMLS_CC) == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_default_property_remove(string classname, string propname)
Remove a property from a class
 */
PHP_FUNCTION(runkit_default_property_remove)
{
       char *classname, *propname;
       int classname_len, propname_len;

       if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/", &classname, &classname_len, &propname, &propname_len) == FAILURE) {
               RETURN_FALSE;
       }

       RETURN_BOOL(php_runkit_def_prop_remove(classname, classname_len, propname, propname_len TSRMLS_CC) == SUCCESS);
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
