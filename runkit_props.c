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

#ifndef IS_CONSTANT_AST
#define IS_CONSTANT_AST IS_CONSTANT_ARRAY
#endif

#if PHP_VERSION_ID < 50600
#define _CONSTANT_INDEX(a) (void*) a
#else
#define _CONSTANT_INDEX(a) a
#endif

#ifdef PHP_RUNKIT_MANIPULATION

/* {{{ php_runkit_update_children_def_props
	Scan the class_table for children of the class just updated */
int php_runkit_update_children_def_props(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class = va_arg(args, zend_class_entry*);
	zval *p = va_arg(args, zval*);
	char *pname = va_arg(args, char*);
	int pname_len = va_arg(args, int);
	int access_type = va_arg(args, int);
	zend_class_entry *definer_class = va_arg(args, zend_class_entry*);
	int override = va_arg(args, int);
	int override_in_objects = va_arg(args, int);

	RUNKIT_UNDER53_TSRMLS_FETCH();

	ce = *((zend_class_entry**)ce);

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	php_runkit_def_prop_add_int(ce, pname, pname_len, p, access_type, NULL, 0, definer_class, override, override_in_objects TSRMLS_CC);
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
	zend_class_entry *class_we_originally_removing_from = va_arg(args, zend_class_entry*);
	zend_bool was_static = va_arg(args, int);
	zend_bool remove_from_objects = va_arg(args, int);
	zend_property_info *parent_property = va_arg(args, zend_property_info *);

	RUNKIT_UNDER53_TSRMLS_FETCH();

	ce = *((zend_class_entry**)ce);

	if (ce->parent != parent_class || !hash_key || !hash_key->arKey || !hash_key->arKey[0]) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

	php_runkit_def_prop_remove_int(ce, pname, pname_len, class_we_originally_removing_from, was_static, remove_from_objects,
	                               parent_property TSRMLS_CC);
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_remove_property_by_full_name */
static int php_runkit_remove_property_by_full_name(zend_property_info *prop, zend_property_info *comp_prop, zend_hash_key *key) {
	if (prop->h == comp_prop->h && prop->name_length == comp_prop->name_length &&
	    memcmp(prop->name, comp_prop->name, prop->name_length) == 0) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_remove_overlapped_property_from_childs
       Clean private properties by offset */
int php_runkit_remove_overlapped_property_from_childs(RUNKIT_53_TSRMLS_ARG(zend_class_entry *ce), int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_class_entry *parent_class = va_arg(args, zend_class_entry*);
	char *propname = va_arg(args, char*);
	int propname_len = va_arg(args, int);
	int offset = va_arg(args, int);
	zend_bool is_static = va_arg(args, int);
	zend_bool remove_from_objects = va_arg(args, int);
	zend_property_info *property_info_ptr = va_arg(args, zend_property_info *);
	int i;
	long h;
	zend_property_info *p;
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4 || PHP_MAJOR_VERSION > 5
	zval **table;
#endif
	RUNKIT_UNDER53_TSRMLS_FETCH();

	ce = *((zend_class_entry**)ce);

	if (ce->parent != parent_class) {
		/* Not a child, ignore */
		return ZEND_HASH_APPLY_KEEP;
	}

#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
	if (!remove_from_objects && (property_info_ptr->flags & ZEND_ACC_SHADOW)) {
		return ZEND_HASH_APPLY_KEEP;
	}
#endif

	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
	                               (apply_func_args_t) php_runkit_remove_overlapped_property_from_childs,
	                               8, ce, propname, propname_len, offset, is_static, remove_from_objects, property_info_ptr);
	php_runkit_remove_property_from_reflection_objects(ce, propname, propname_len TSRMLS_CC);

	if (is_static || !EG(objects_store).object_buckets) {
		goto st_success;
	}

	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		    EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == ce) {
				if (!remove_from_objects) {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
					zval **prop_val;
					h = zend_get_hash_value((char *) propname, propname_len + 1);
					if (zend_hash_quick_find(object->properties, (char *) property_info_ptr->name,
					                         property_info_ptr->name_length + 1,
					                         property_info_ptr->h, (void *) &prop_val) == SUCCESS) {
						zval **foo;
						if ((property_info_ptr->flags & (ZEND_ACC_PRIVATE | ZEND_ACC_PROTECTED | ZEND_ACC_SHADOW)) && prop_val) {
							Z_ADDREF_P(*prop_val);
							if (PZVAL_IS_REF(*prop_val)) {
								SEPARATE_ZVAL(prop_val);
							}
							zend_hash_quick_update(object->properties, (char *) propname,
							                       propname_len + 1, h, prop_val,
							                       sizeof(zval *), (void *) &foo);
							php_error_docref(NULL TSRMLS_CC, E_NOTICE,
							                 "Making %s::%s public to remove it "
							                 "from class without objects overriding",
							                 ce->name, propname);
							zend_hash_quick_del(object->properties,
							                    property_info_ptr->name,
							                    property_info_ptr->name_length+1,
							                    property_info_ptr->h);
						}
					}
#endif
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4 || PHP_MAJOR_VERSION > 5
					zval *prop_val = NULL;
					h = zend_get_hash_value((char *) propname, propname_len + 1);
					if (!object->properties) {
						prop_val = object->properties_table[offset];
						rebuild_object_properties(object);
					} else if (object->properties_table[offset]) {
						prop_val = *(zval **) object->properties_table[offset];
					}
					if ((property_info_ptr->flags & (ZEND_ACC_PRIVATE | ZEND_ACC_PROTECTED | ZEND_ACC_SHADOW)) && prop_val) {
						Z_ADDREF_P(prop_val);
						if (h != property_info_ptr->h) {
							zend_hash_quick_del(object->properties, property_info_ptr->name, property_info_ptr->name_length+1, property_info_ptr->h);
						}
						zend_hash_quick_update(object->properties, propname, propname_len+1, h,
						                 &prop_val, sizeof(zval *), (void *) &object->properties_table[offset]);
						php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Making %s::%s public to remove it "
						                 "from class without objects overriding", ce->name, propname);
					}
#endif
				} else {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
					zend_hash_quick_del(object->properties, property_info_ptr->name, property_info_ptr->name_length+1,
					                    property_info_ptr->h);
#else
					if (object->properties_table[offset]) {
						if (!object->properties) {
							zval_ptr_dtor(&object->properties_table[offset]);
							object->properties_table[offset] = NULL;
						} else {
							zend_hash_del(object->properties, propname, propname_len+1);
						}
					}
#endif
				}
			}
		}
	}
st_success:
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4 || PHP_MAJOR_VERSION > 5
	table = is_static ? ce->default_static_members_table : ce->default_properties_table;
	if (table[offset]) {
		zval_ptr_dtor(&table[offset]);
		table[offset] = NULL;
	}
#elif PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
	zend_hash_quick_del(&ce->default_properties, property_info_ptr->name, property_info_ptr->name_length+1,
	                    property_info_ptr->h);
#endif
	h = zend_get_hash_value((char *) propname, propname_len + 1);
	zend_hash_apply_with_argument(&ce->properties_info, (apply_func_arg_t) php_runkit_remove_property_by_full_name,
	                              property_info_ptr TSRMLS_CC);
	if (zend_hash_quick_find(&ce->properties_info, propname, propname_len + 1, h, (void *) &p) == SUCCESS &&
	    p->h == property_info_ptr->h) {
		zend_hash_quick_del(&ce->properties_info, propname, propname_len + 1, h);
	}
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_runkit_def_prop_add_int */
int php_runkit_def_prop_add_int(zend_class_entry *ce, const char *propname, int propname_len, zval *copyval, long visibility,
                                const char *doc_comment, int doc_comment_len, zend_class_entry *definer_class, int override,
                                int override_in_objects TSRMLS_DC)
{
	int i;
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

	if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void*) &prop_info_ptr) == SUCCESS) {
		if (!override) {
			zval_ptr_dtor(&pcopyval);
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s%s%s already exists, not adding",
			                 ce->name, (prop_info_ptr->flags & ZEND_ACC_STATIC) ? "::$" : "->", propname);
			return FAILURE;
		} else {
			php_runkit_def_prop_remove_int(ce, propname, propname_len, NULL, 0, override_in_objects, NULL TSRMLS_CC);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif
		}
	}
	prop_info_ptr = NULL;

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
			char *oldkey;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			const char *interned_name;
#endif
			int oldkey_len;
			zend_property_info new_prop_info;
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3
			zval **prop;
#endif
			zend_mangle_property_name(&newkey, &newkey_len, definer_class->name, definer_class->name_length, (char *) propname, propname_len, ce->type & ZEND_INTERNAL_CLASS);
			zend_mangle_property_name(&oldkey, &oldkey_len, ce->name, ce->name_length, (char *) propname, propname_len, ce->type & ZEND_INTERNAL_CLASS);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 3)
			symt = (visibility & ZEND_ACC_STATIC) ? &ce->default_static_members : &ce->default_properties;
			if (zend_hash_find(symt, oldkey, oldkey_len + 1, (void*) &prop) != SUCCESS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find just added property");
				return FAILURE;
			}
			Z_ADDREF_P(*prop);
			zend_hash_update(symt, newkey, newkey_len + 1, prop, sizeof(zval *), NULL);
			zend_hash_del(symt, oldkey, oldkey_len + 1);
#endif
			memcpy(&new_prop_info, prop_info_ptr, sizeof(zend_property_info));
			new_prop_info.name = newkey;
			new_prop_info.name_length = newkey_len;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2) || (PHP_MAJOR_VERSION > 5)
			new_prop_info.ce = definer_class;
#endif
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			interned_name = zend_new_interned_string(new_prop_info.name, new_prop_info.name_length+1, 0 TSRMLS_CC);
			if (interned_name != new_prop_info.name) {
				efree((char*)new_prop_info.name);
				new_prop_info.name = interned_name;
			}
#endif

			new_prop_info.h = zend_get_hash_value(new_prop_info.name, new_prop_info.name_length + 1);
			new_prop_info.doc_comment = new_prop_info.doc_comment ? estrndup(new_prop_info.doc_comment, new_prop_info.doc_comment_len) : NULL;
			zend_hash_quick_del(&ce->properties_info, (char *) propname, propname_len + 1, h);
			zend_hash_quick_update(&ce->properties_info, (char *) propname, propname_len + 1, h, &new_prop_info, sizeof(zend_property_info), NULL);
			pefree((void*)oldkey, ce->type & ZEND_INTERNAL_CLASS);
			zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void*) &prop_info_ptr);
		}
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2) || (PHP_MAJOR_VERSION > 5)
		prop_info_ptr->ce = definer_class;
#endif
	}
	zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
	                               (apply_func_args_t)php_runkit_update_children_def_props, 8, ce, copyval,
	                               propname, propname_len, visibility, definer_class, override, override_in_objects);

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
				int new_size = offset + 1;
				if (!object->properties_table) {
					object->properties_table = pemalloc(sizeof(void*) * (new_size), 0);
					memset(object->properties_table, 0, new_size * sizeof(void *));
				} else {
					object->properties_table = perealloc(object->properties_table, sizeof(void*) * (new_size), 0);
					object->properties_table[new_size - 1] = NULL;
				}
				if (ce->default_properties_table[offset]) {
					if (!object->properties) {
						if (override_in_objects) {
							Z_ADDREF_P(ce->default_properties_table[offset]);
							object->properties_table[offset] = ce->default_properties_table[offset];
						} else {
							if (object->properties_table[offset]) {
								zval_ptr_dtor(&object->properties_table[offset]);
								object->properties_table[offset] = NULL;
							}
						}
					} else {
						zval **prop_val;
						if (zend_hash_quick_find(object->properties, prop_info_ptr->name,
						                         prop_info_ptr->name_length+1,
						                         prop_info_ptr->h, (void *) &prop_val) != SUCCESS &&
						    (prop_info_ptr->h == h || zend_hash_quick_find(object->properties, propname,
						                                                   propname_len+1,
						                                                   h, (void *) &prop_val) != SUCCESS)) {
							if (override_in_objects) {
								if (object->properties_table[offset]) {
									zval_ptr_dtor((zval **) object->properties_table[offset]);
									object->properties_table[offset] = NULL;
								}
								object->properties_table[offset] = ce->default_properties_table[offset];
#if ZTS
								ALLOC_ZVAL(object->properties_table[offset]);
								MAKE_COPY_ZVAL(&ce->default_properties_table[offset], object->properties_table[offset]);
#else
								Z_ADDREF_P(ce->default_properties_table[offset]);
#endif
								zend_hash_quick_update(object->properties, prop_info_ptr->name, prop_info_ptr->name_length+1,
								                       prop_info_ptr->h, &object->properties_table[offset], sizeof(zval *),
								                       (void*)&object->properties_table[offset]);
								if (prop_info_ptr->h != h) {
									zend_hash_quick_del(object->properties, propname, propname_len+1, h);
								}
							}
						} else {
							if (!override_in_objects) {
								object->properties_table[offset] = (zval *) *prop_val;
								Z_ADDREF_P(object->properties_table[offset]);
								zend_hash_quick_update(object->properties, prop_info_ptr->name,
								                       prop_info_ptr->name_length+1, prop_info_ptr->h,
								                       &object->properties_table[offset],
								                       sizeof(zval *),
								                       (void*)&object->properties_table[offset]);
								if (prop_info_ptr->h != h) {
									zend_hash_quick_del(object->properties, propname, propname_len+1, h);
								}
							} else {
								object->properties_table[offset] = ce->default_properties_table[offset];
								Z_ADDREF_P(object->properties_table[offset]);
								zend_hash_quick_update(object->properties, prop_info_ptr->name,
								                       prop_info_ptr->name_length+1, prop_info_ptr->h,
								                       &object->properties_table[offset],
								                       sizeof(zval *),
								                       (void*)&object->properties_table[offset]);
							}
						}
					}
				}
#else
				if (!object->properties) {
					ALLOC_HASHTABLE(object->properties);
					zend_hash_init(object->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
				}
				if (override_in_objects) {
					Z_ADDREF_P(pcopyval);
					zend_hash_quick_del(object->properties, (char *) prop_info_ptr->name,
					                    prop_info_ptr->name_length + 1, prop_info_ptr->h);
					zend_hash_quick_add(object->properties, (char *) prop_info_ptr->name,
					                    prop_info_ptr->name_length + 1, prop_info_ptr->h, &pcopyval,
					                    sizeof(zval *), NULL);
				} else {
					zval **prop_val = NULL;
					if (zend_hash_quick_find(object->properties, prop_info_ptr->name,
					                         prop_info_ptr->name_length+1,
					                         prop_info_ptr->h, (void *) &prop_val) != SUCCESS &&
					    (prop_info_ptr->h == h ||
					     zend_hash_quick_find(object->properties, (char *) propname, propname_len+1,
					                          h, (void *) &prop_val) != SUCCESS)) {
					} else if (prop_info_ptr->h != h && prop_val) {
						zval **foo;
						Z_ADDREF_P(*prop_val);
						if (PZVAL_IS_REF(*prop_val)) {
							SEPARATE_ZVAL(prop_val);
						}
						zend_hash_quick_update(object->properties, prop_info_ptr->name,
						                       prop_info_ptr->name_length+1,
						                       prop_info_ptr->h, prop_val, sizeof(zval *), (void *) &foo);
						zend_hash_quick_del(object->properties, (char *) propname, propname_len+1, h);
					}
				}
#endif // (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
			}
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_def_prop_add
 */
static int php_runkit_def_prop_add(char *classname, int classname_len, char *propname, int propname_len, zval *value,
                                   long visibility, int override_in_objects TSRMLS_DC)
{
	zend_class_entry *ce;
	zval *copyval;
	char *key = propname;
	int key_len = propname_len;
	zend_property_info *existing_prop;

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
	if (zend_hash_find(&ce->properties_info, key, key_len + 1, (void *) &existing_prop) == SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s%s%s already exists", classname,
		                 (existing_prop->flags & ZEND_ACC_STATIC) ? "::$" : "->", propname);
		return FAILURE;
	}
	if (
	    Z_TYPE_P(copyval) == IS_CONSTANT_AST
#	if RUNKIT_ABOVE53
	    || (Z_TYPE_P(copyval) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT
#	endif
	) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2) || (PHP_MAJOR_VERSION > 5)
		zval_update_constant_ex(&copyval, _CONSTANT_INDEX(1), ce TSRMLS_CC);
#else
		zval_update_constant(&copyval, ce TSRMLS_CC);
#endif
	}

	if (php_runkit_def_prop_add_int(ce, propname, propname_len, copyval, visibility, NULL, 0, ce, 0,
	                                override_in_objects TSRMLS_CC) != SUCCESS) {
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_dump_string */
/*
static inline void php_runkit_dump_string(const char *str, int len) {
	int j;
	for (j=0; j<len; j++) {
		printf("%c", str[j]);
	}
}
*/
/* }}} */

/* {{{ php_runkit_dump_hashtable_keys */
/*
static inline void php_runkit_dump_hashtable_keys(HashTable* ht) {
	HashPosition pos;
	void *ptr;
	for(zend_hash_internal_pointer_end_ex(ht, &pos);
	    zend_hash_get_current_data_ex(ht, (void*)&ptr, &pos) == SUCCESS;
	    zend_hash_move_backwards_ex(ht, &pos)) {
		printf("key = ");
		php_runkit_dump_string(pos->arKey, pos->nKeyLength);
	}
}
*/
/* }}} */

/* {{{ php_runkit_def_prop_remove_int */
int php_runkit_def_prop_remove_int(zend_class_entry *ce, const char *propname, int propname_len, zend_class_entry *class_we_originally_removing_from,
                                   zend_bool was_static, zend_bool remove_from_objects,
                                   zend_property_info *parent_property TSRMLS_DC)
{
	int i;
	long h;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	int offset;
#endif
	int flags;
	zend_property_info *property_info_ptr;

	h = zend_get_hash_value((char *) propname, propname_len + 1);
	if (zend_hash_quick_find(&ce->properties_info, (char *) propname, propname_len + 1, h, (void*)&property_info_ptr) == SUCCESS) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2) || (PHP_MAJOR_VERSION > 5)
		if (class_we_originally_removing_from == NULL) {
			class_we_originally_removing_from = property_info_ptr->ce;
		}
#endif
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		if (parent_property &&
		    ((parent_property->offset >= 0 && parent_property->offset != property_info_ptr->offset) ||
		     parent_property->ce != property_info_ptr->ce ||
		     ((parent_property->flags & ZEND_ACC_STATIC) != (property_info_ptr->flags & ZEND_ACC_STATIC))
		    )) {
			return SUCCESS;
		}
#endif // PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2) || (PHP_MAJOR_VERSION > 5)
		if (class_we_originally_removing_from != property_info_ptr->ce) {
			return SUCCESS;
		}
#endif

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
#if !(PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) && (PHP_MAJOR_VERSION == 5)
			php_runkit_remove_property_from_reflection_objects(ce, property_info_ptr->name, property_info_ptr->name_length TSRMLS_CC);
#endif // !(PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) && (PHP_MAJOR_VERSION == 5)
		}

		flags = property_info_ptr->flags;
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		offset = property_info_ptr->offset;

		if (property_info_ptr->flags & (ZEND_ACC_PRIVATE|ZEND_ACC_SHADOW)) {
			if (offset >= 0) {
				zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
				                               (apply_func_args_t)php_runkit_remove_overlapped_property_from_childs,
				                               8, ce, propname, propname_len, offset,
				                               property_info_ptr->flags & ZEND_ACC_STATIC, remove_from_objects, property_info_ptr);
			}
		}
#elif PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
		if (property_info_ptr->flags & (ZEND_ACC_PRIVATE|ZEND_ACC_SHADOW)) {
			zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
			                               (apply_func_args_t)php_runkit_remove_overlapped_property_from_childs,
			                               8, ce, propname, propname_len, 0,
			                               property_info_ptr->flags & ZEND_ACC_STATIC, remove_from_objects, property_info_ptr);
		}
#endif
		zend_hash_apply_with_arguments(RUNKIT_53_TSRMLS_PARAM(EG(class_table)),
		                               (apply_func_args_t)php_runkit_remove_children_def_props,
		                               8, ce, propname, propname_len, class_we_originally_removing_from,
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		                               was_static
#else
		                               0
#endif
		                               , remove_from_objects, property_info_ptr
		);

		php_runkit_remove_property_from_reflection_objects(ce, propname, propname_len TSRMLS_CC);
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
		php_runkit_clear_all_functions_runtime_cache(TSRMLS_C);
#endif
	} else {
		if (parent_property) {
			return SUCCESS;
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s::%s does not exist", ce->name, propname);
			return FAILURE;
		}
	}

	if ((flags & ZEND_ACC_STATIC) || !EG(objects_store).object_buckets) {
		goto st_success;
	}
	for (i = 1; i < EG(objects_store).top ; i++) {
		if (EG(objects_store).object_buckets[i].valid && (!EG(objects_store).object_buckets[i].destructor_called) &&
		   EG(objects_store).object_buckets[i].bucket.obj.object) {
			zend_object *object;
			object = (zend_object *) EG(objects_store).object_buckets[i].bucket.obj.object;
			if (object->ce == ce) {
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
				if (!remove_from_objects) {
					zval *prop_val = NULL;
					if (!object->properties) {
						prop_val = object->properties_table[offset];
						rebuild_object_properties(object);
					} else if (object->properties_table[offset]) {
						prop_val = *(zval **) object->properties_table[offset];
					}
					if ((property_info_ptr->flags & (ZEND_ACC_PRIVATE | ZEND_ACC_PROTECTED | ZEND_ACC_SHADOW)) && prop_val) {
						Z_ADDREF_P(prop_val);
						if (h != property_info_ptr->h) {
							zend_hash_quick_del(object->properties, property_info_ptr->name, property_info_ptr->name_length+1, property_info_ptr->h);
						}
						zend_hash_quick_update(object->properties, propname, propname_len+1, h,
						                 &prop_val, sizeof(zval *), (void *) &object->properties_table[offset]);
						php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Making %s::%s public to remove it from class without objects overriding", ce->name, propname);
					}
				} else {
					if (object->properties_table[offset]) {
						if (!object->properties) {
							zval_ptr_dtor(&object->properties_table[offset]);
							object->properties_table[offset] = NULL;
						} else {
							zend_hash_quick_del(object->properties, property_info_ptr->name, property_info_ptr->name_length+1,
							                    property_info_ptr->h);
						}
					}
				}
#else
				if (object->properties) {
					if (remove_from_objects) {
						zend_hash_quick_del(object->properties, (char *) property_info_ptr->name, property_info_ptr->name_length + 1,
						                    property_info_ptr->h);
					} else {
						if (property_info_ptr->flags & (ZEND_ACC_PRIVATE | ZEND_ACC_PROTECTED)) {
							zval **prop_val;
							if (zend_hash_quick_find(object->properties, (char *) property_info_ptr->name,
							                         property_info_ptr->name_length + 1,
							                         property_info_ptr->h, (void *) &prop_val) == SUCCESS) {
								if (h != property_info_ptr->h) {
									Z_ADDREF_P(*prop_val);
									if (PZVAL_IS_REF(*prop_val)) {
										SEPARATE_ZVAL(prop_val);
									}
									zend_hash_quick_update(object->properties, (char *) propname,
									                       propname_len+1, h, prop_val,
									                       sizeof(zval *), (void *) NULL);
									php_error_docref(NULL TSRMLS_CC, E_NOTICE,
									                 "Making %s::%s public to remove it from class without objects overriding",
									                 ce->name, propname);
									zend_hash_quick_del(object->properties,
									                    property_info_ptr->name,
									                    property_info_ptr->name_length+1,
									                    property_info_ptr->h);
								}
							}
						}
					}
				}
#endif
			}
		}
	}

st_success:
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 4) || (PHP_MAJOR_VERSION > 5)
	if (was_static == 0 && ce->default_properties_table[property_info_ptr->offset]) {
		zval_ptr_dtor(&ce->default_properties_table[property_info_ptr->offset]);
		ce->default_properties_table[property_info_ptr->offset] = NULL;
	}
#else
	if (was_static == 0) {
		zend_hash_quick_del(&ce->default_properties, property_info_ptr->name, property_info_ptr->name_length + 1,
		                    property_info_ptr->h);
	}
#endif
	zend_hash_quick_del(&ce->properties_info, (char *) propname, propname_len + 1, h);
	return SUCCESS;
}
/* }}} */

/* {{{ php_runkit_def_prop_remove */
static int php_runkit_def_prop_remove(char *classname, int classname_len, char *propname, int propname_len,
                                      zend_bool remove_from_objects TSRMLS_DC)
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
	return php_runkit_def_prop_remove_int(ce, propname, propname_len, NULL, 0, remove_from_objects, NULL TSRMLS_CC);
}
/* }}} */

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
	long visibility = 0;
	int override_in_objects = 0;

	visibility = ZEND_ACC_PUBLIC;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/z|l/", &classname, &classname_len, &propname, &propname_len,
	                          &value, &visibility) == FAILURE) {
		RETURN_FALSE;
	}

	override_in_objects = (visibility & PHP_RUNKIT_OVERRIDE_OBJECTS) != 0;

	visibility &= ~PHP_RUNKIT_OVERRIDE_OBJECTS;

	RETURN_BOOL(php_runkit_def_prop_add(classname, classname_len, propname, propname_len, value, visibility, override_in_objects TSRMLS_CC) == SUCCESS);
}
/* }}} */

/* {{{ proto bool runkit_default_property_remove(string classname, string propname)
Remove a property from a class
 */
PHP_FUNCTION(runkit_default_property_remove)
{
	char *classname, *propname;
	int classname_len, propname_len;
	zend_bool remove_from_objects = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/s/|b", &classname, &classname_len, &propname, &propname_len,
	                          &remove_from_objects) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_BOOL(php_runkit_def_prop_remove(classname, classname_len, propname, propname_len,
	            remove_from_objects TSRMLS_CC) == SUCCESS);
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
