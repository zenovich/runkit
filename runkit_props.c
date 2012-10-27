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
	int parent_offset = va_arg(args, int);
	zend_bool was_static = va_arg(args, int);

	RUNKIT_UNDER53_TSRMLS_FETCH();

#ifdef ZEND_ENGINE_2
	ce = *((zend_class_entry**)ce);
#endif

	if (ce->parent != parent_class || !hash_key || !hash_key->arKey || !hash_key->arKey[0]) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	php_runkit_def_prop_remove_int(ce, pname, pname_len, definer_class, parent_offset, was_static TSRMLS_CC);
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
/* {{{ php_runkit_remove_shadowed_property_from_childs
	Clean private properties by offset */
int php_runkit_remove_shadowed_property_from_childs(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class = va_arg(args, zend_class_entry*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	int offset = va_arg(args, int);
	zend_bool is_static = va_arg(args, int);
	int i;
	zval **table;

	ce = *((zend_class_entry**)ce);

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	table = is_static ? ce->default_static_members_table : ce->default_properties_table;
	if (table[offset]) {
		zval_ptr_dtor(&table[offset]);
		table[offset] = NULL;
	}

	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_remove_shadowed_property_from_childs,
	                               5, ce, pname, pname_len, offset, is_static);
	php_runkit_remove_property_from_reflection_objects(ce, pname, pname_len TSRMLS_CC);

	if (is_static || !EG(objects_store).object_buckets) {
		return ZEND_HASH_APPLY_KEEP;
	}
	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		   EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == ce) {
				if (object->properties_table[offset]) {
					if (!object->properties) {
						zval_ptr_dtor(&object->properties_table[offset]);
					} else {
						zend_hash_del(object->properties, pname, pname_len+1);
					}
					object->properties_table[offset] = NULL;
				}
			}
		}
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */
#endif

/* {{{ php_runkit_def_prop_add_int */
int php_runkit_def_prop_add_int(zend_class_entry *ce, const char *propname, int propname_len, zval *copyval, long visibility,
				const char *doc_comment, int doc_comment_len, zend_class_entry *definer_class, int override TSRMLS_DC)
{
#if PHP_MAJOR_VERSION >= 5
	int i;
#endif
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	int offset;
#endif
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3
	HashTable *symt;
#endif
#if PHP_MAJOR_VERSION >= 5
	zend_property_info *prop_info_ptr = NULL;
	long h = zend_get_hash_value((char *) propname, propname_len + 1);
#endif
	zval *pcopyval = copyval;

#if PHP_MAJOR_VERSION >= 5
	if ((visibility & ZEND_ACC_PRIVATE) && (visibility & ZEND_ACC_STATIC) && definer_class && definer_class != ce) {
		return SUCCESS;
	}

	Z_ADDREF_P(pcopyval);

	if (visibility & ZEND_ACC_STATIC) {
		if (definer_class == NULL || ce == definer_class) {
			zval_ptr_dtor(&pcopyval);
			SEPARATE_ARG_IF_REF(pcopyval);
		} else {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 2)
			pcopyval->is_ref = 1;
#else
			Z_SET_ISREF_P(pcopyval);
#endif
		}
	}
#endif // PHP_MAJOR_VERSION >= 5

#if PHP_MAJOR_VERSION >= 5
	if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void*) &prop_info_ptr) == SUCCESS) {
		if (!override) {
			zval_ptr_dtor(&pcopyval);
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s%s%s already exists, not adding",
			                 ce->name, (prop_info_ptr->flags & ZEND_ACC_STATIC) ? "::$" : "->", propname);
			return FAILURE;
		} else {
			php_runkit_def_prop_remove_int(ce, propname, propname_len, NULL, -1, 0 TSRMLS_CC);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif
		}
	}
	prop_info_ptr = NULL;
#else
	if (zend_hash_exists(&ce->default_properties, (char *) propname, propname_len + 1)) {
		if (override) {
			if (zend_hash_del(&ce->default_properties, (char *) propname, propname_len + 1) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Failed to remove property %s->%s, not adding",
				                 ce->name, propname);
				return FAILURE;
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s->%s already exists, not adding",
			                 ce->name, propname);
			return FAILURE;
		}
	}
	Z_ADDREF_P(pcopyval);
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
		if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void*) &prop_info_ptr) != SUCCESS) {
			zval_ptr_dtor(&pcopyval);
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
			if (zend_hash_find(symt, oldkey, oldkey_len + 1, (void*) &prop) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find just added property");
				return FAILURE;
			}
			Z_ADDREF_P(*prop);
			zend_hash_update(symt, newkey, newkey_len + 1, prop, sizeof(zval *), NULL);
			zend_hash_del(symt, oldkey, oldkey_len + 1);
			pefree((void*)oldkey, ce->type & ZEND_INTERNAL_CLASS);
#endif
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3
			pefree((void*)prop_info_ptr->name, ce->type & ZEND_INTERNAL_CLASS);
#endif
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			str_efree(prop_info_ptr->name);
#endif
			prop_info_ptr->name = newkey;
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
	if (!prop_info_ptr && zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void*) &prop_info_ptr) != SUCCESS) {
		zval_ptr_dtor(&pcopyval);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find just added property's info");
		return FAILURE;
	}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	offset = prop_info_ptr->offset;
#endif

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
					object->properties_table = pemalloc(sizeof(void*) * (offset + 1), 0);
				} else {
					object->properties_table = perealloc(object->properties_table, sizeof(void*) * (offset + 1), 0);
				}

				if (ce->default_properties_table[offset]) {
					if (!object->properties) {
						object->properties_table[offset] = ce->default_properties_table[offset];
					} else {
						zend_hash_quick_update(object->properties, propname, propname_len+1, h, &ce->default_properties_table[offset], sizeof(zval *), (void**)&object->properties_table[offset]);
					}
					Z_ADDREF_P(ce->default_properties_table[offset]);
				}
#else
				if (!object->properties) {
					ALLOC_HASHTABLE(object->properties);
					zend_hash_init(object->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
				}
				Z_ADDREF_P(pcopyval);
				zend_hash_quick_del(object->properties, (char *) prop_info_ptr->name, prop_info_ptr->name_length + 1, prop_info_ptr->h);
				zend_hash_quick_add(object->properties, (char *) prop_info_ptr->name, prop_info_ptr->name_length + 1, prop_info_ptr->h, &pcopyval, sizeof(zval *), NULL);
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
	zend_property_info *existing_prop;
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

#if PHP_MAJOR_VERSION >= 5
	/* Check for existing property by this name */
	/* Existing public? */
	if (zend_hash_find(&ce->properties_info, key, key_len + 1, (void *) &existing_prop) == SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s%s%s already exists", classname,
		                 (existing_prop->flags & ZEND_ACC_STATIC) ? "::$" : "->", propname);
		return FAILURE;
	}
	if (
	    Z_TYPE_P(copyval) == IS_CONSTANT_ARRAY
#	if RUNKIT_ABOVE53
	    || (Z_TYPE_P(copyval) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#	endif
	) {
		zval_update_constant_ex(&copyval, (void*) 1, ce TSRMLS_CC);
	}
#else
	if (zend_hash_exists(&ce->default_properties, key, key_len + 1)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s->%s already exists", classname, propname);
		return FAILURE;
	}
#endif // PHP_MAJOR_VERSION >= 5


	if (php_runkit_def_prop_add_int(ce, propname, propname_len, copyval, visibility, NULL, 0, ce, 0 TSRMLS_CC) != SUCCESS) {
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_def_prop_remove */
int php_runkit_def_prop_remove_int(zend_class_entry *ce, const char *propname, int propname_len, zend_class_entry *definer_class,
                                   int parent_offset, zend_bool was_static TSRMLS_DC)
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
	zend_hash_apply_with_arguments(EG(class_table), (apply_func_args_t)php_runkit_remove_children_def_props, 6, ce, propname, propname_len, -1, 0, NULL);
#else
	int i;
	long h;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	int offset;
#endif
	int flags;
	zend_property_info *property_info_ptr;

	h = zend_get_hash_value((char *) propname, propname_len + 1);
	if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void*)&property_info_ptr) == SUCCESS) {
		if (definer_class && property_info_ptr->ce != definer_class) {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
			char *private;
			int private_len;
			zend_mangle_property_name(&private, &private_len, definer_class->name, definer_class->name_length, (char *) propname, propname_len, 0);
			php_runkit_remove_property_from_reflection_objects(ce, private, private_len TSRMLS_CC);
			if (zend_hash_exists(&ce->default_properties, private, private_len + 1)) {
				zend_hash_del(&ce->default_properties, private, private_len + 1);
			}
			if (zend_hash_exists(&ce->default_static_members, private, private_len + 1)) {
				zend_hash_del(&ce->default_static_members, private, private_len + 1);
			}
			efree(private);
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_remove_children_def_props,
			                               6, ce, propname, propname_len, definer_class, -1, 0);
#elif (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			if (parent_offset >= 0 && parent_offset != property_info_ptr->offset) {
				zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
				                               (apply_func_args_t)php_runkit_remove_shadowed_property_from_childs,
			                                       5, ce, propname, propname_len, parent_offset, was_static);
			}
#endif // PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
			return SUCCESS;
		} else if (!definer_class) {
			definer_class = property_info_ptr->ce;
		}
		if (property_info_ptr->flags & ZEND_ACC_STATIC) {
			was_static = 1;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			if (ce->default_static_members_table[property_info_ptr->offset]) {
				zval_ptr_dtor(&ce->default_static_members_table[property_info_ptr->offset]);
				ce->default_static_members_table[property_info_ptr->offset] = NULL;
			}
#else
			php_runkit_remove_property_from_reflection_objects(ce, property_info_ptr->name, property_info_ptr->name_length TSRMLS_CC);
			if (zend_hash_quick_del(&ce->default_static_members, property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property %s::%s", ce->name, propname);
				return FAILURE;
			}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		} else {
			was_static = 0;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			if (ce->default_properties_table[property_info_ptr->offset]) {
				zval_ptr_dtor(&ce->default_properties_table[property_info_ptr->offset]);
				ce->default_properties_table[property_info_ptr->offset] = NULL;
			}
#else
			php_runkit_remove_property_from_reflection_objects(ce, property_info_ptr->name, property_info_ptr->name_length TSRMLS_CC);
			if (zend_hash_quick_del(&ce->default_properties, property_info_ptr->name, property_info_ptr->name_length + 1, property_info_ptr->h) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property %s->%s", ce->name, propname);
				return FAILURE;
			}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		}

		flags = property_info_ptr->flags;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		offset = property_info_ptr->offset;

		if (property_info_ptr->flags & ZEND_ACC_PRIVATE) {
			if (offset >= 0) {
				zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
				                               (apply_func_args_t)php_runkit_remove_shadowed_property_from_childs,
			                                       5, ce, propname, propname_len, offset, property_info_ptr->flags & ZEND_ACC_STATIC);
			}
		}
#endif
		zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)), (apply_func_args_t)php_runkit_remove_children_def_props,
		                               6, ce, propname, propname_len, definer_class,
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		                               offset, was_static
#else
		                               -1, 0
#endif
		);

		php_runkit_remove_property_from_reflection_objects(ce, propname, propname_len TSRMLS_CC);
		if (zend_hash_quick_del(&ce->properties_info, (char *) propname, propname_len + 1, h) != SUCCESS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to remove the property %s::%s", ce->name, propname);
			return FAILURE;
		}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s does not exist", ce->name, propname);
		return FAILURE;
	}

	if ((flags & ZEND_ACC_STATIC) || !EG(objects_store).object_buckets) {
		return SUCCESS;
	}
	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		   EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == ce) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
				if (object->properties_table[offset]) {
					if (!object->properties) {
						zval_ptr_dtor(&object->properties_table[offset]);
					} else {
						zend_hash_quick_del(object->properties, propname, propname_len+1, h);
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

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif
	return php_runkit_def_prop_remove_int(ce, propname, propname_len, NULL, -1, 0 TSRMLS_CC);
}
/* }}} */

#ifdef ZEND_ENGINE_2
/* {{{ php_runkit_remove_property_from_reflection_objects */
void php_runkit_remove_property_from_reflection_objects(zend_class_entry *ce, const char *propname, int propname_len TSRMLS_DC) {
	int i;
	extern PHPAPI zend_class_entry *reflection_property_ptr;

	if (!EG(objects_store).object_buckets) {
		return;
	}

	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		   EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == reflection_property_ptr) {
				reflection_object *refl_obj = (reflection_object *) object;
				if (refl_obj->ptr && refl_obj->ce == ce) {
					property_reference *prop_ref = (property_reference *) refl_obj->ptr;
					if (prop_ref->ce == ce && prop_ref->prop.name_length == propname_len &&
					    !memcmp(prop_ref->prop.name, propname, propname_len)) {
#if RUNKIT_ABOVE53
						if (refl_obj->ref_type == REF_TYPE_DYNAMIC_PROPERTY) {
							efree((char *)prop_ref->prop.name);
						}
						efree(refl_obj->ptr);
#else
						if (refl_obj->free_ptr) {
							efree(refl_obj->ptr);
						}
#endif
						refl_obj->ptr = NULL;
					}
					PHP_RUNKIT_UPDATE_REFLECTION_OBJECT_NAME(object, i, RUNKIT_G(removed_property_str));
				}
			}
		}
	}
}
/* }}} */
#endif

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
