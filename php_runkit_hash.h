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

#ifndef PHP_RUNKIT_HASH_H
#define PHP_RUNKIT_HASH_H

/* {{{ php_runkit_hash_get_bucket */
inline static Bucket *php_runkit_hash_get_bucket(HashTable *ht, zend_hash_key *hash_key) {
	Bucket *p = ht->arBuckets[hash_key->h & ht->nTableMask];
	while (p) {
		if ((p->arKey == hash_key->arKey) ||
		    ((p->h == hash_key->h) && (p->nKeyLength == hash_key->nKeyLength) &&
		                              !memcmp(p->arKey, hash_key->arKey, hash_key->nKeyLength))) {
			return p;
		}
		p = p->pNext;
	}
	return NULL;
}
/* }}} */

/* {{{ php_runkit_hash_move_to_front */
inline static void php_runkit_hash_move_to_front(HashTable *ht, Bucket *p) {
	if (!p) return;

	/* Unlink from global DLList */
	if (p->pListNext) {
		p->pListNext->pListLast = p->pListLast;
	}
	if (p->pListLast) {
		p->pListLast->pListNext = p->pListNext;
	}
	if (ht->pListTail == p) {
		ht->pListTail = p->pListLast;
	}
	if (ht->pListHead == p) {
		ht->pListHead = p->pListNext;
	}

	/* Relink at the front */
	p->pListLast = NULL;
	p->pListNext = ht->pListHead;
	if (p->pListNext) {
		p->pListNext->pListLast = p;
	}
	ht->pListHead = p;
	if (!ht->pListTail) {
		ht->pListTail = p;
	}
}
/* }}} */

#endif
