/*  Copyright (c) 2013, Mario Ivancic
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// malloc_hash.c

#include "malloc_hash.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_POOL_SIZE       2048
#define DEFAULT_POOL_INCREMENT  2048
#define MAX_LOAD_FACTOR         4


// allocete memory block and copy string str to it.
// return allocetad block or 0 if no memory
static char* mystrdup(const char* str)
{
    char* p;
    size_t i;

    if(!str) return 0;

    i = strlen(str);
    p = (char*) malloc(i + 1);
    if(!p) return 0;
    memcpy(p, str, i + 1);
    return p;
}



// create new hash object, return 0 if unsuccessful
malloc_hash_t* mhash_create(void)
{
    malloc_hash_t *p = malloc(sizeof(*p));
    return p;
}


// Initialize a new hashtable and allocate memory for hash storage.
// arguments:
// hash         pointer to malloc_hash_t object
// pool_size    size of hash pool (this is hash capacity)
//              if pool_size == 0 dafault value will be used
// pool_increment this is added to pool_size when hash become full
//              if pool_increment == 0 default_value will be used
// returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no enough memory
// MHASH_OK         if success
int mhash_init(malloc_hash_t* hash, int pool_size, int pool_increment)
{
    hashentry_t **t, *p;                        // table and pool
    int table_size = 1;

    if(!hash) return MHASH_INVARG;              // invalid argument

    if(pool_size == 0) pool_size = DEFAULT_POOL_SIZE;
    if(pool_increment == 0) pool_increment = DEFAULT_POOL_INCREMENT;

    while(table_size < pool_size) table_size <<= 1;

    t = malloc(table_size * sizeof(*t));
    if(!t) return MHASH_NOMEM;                  // no memory

    p = malloc(pool_size * sizeof(*p));
    if(!p)
    {
        free(t);
        return MHASH_NOMEM;                     // no memory
    }

    hashtable_init(&hash->hash, t, table_size, p, pool_size);
    hash->pool_size = pool_size;
    hash->pool_increment = pool_increment;

    return MHASH_OK;
}



// Retrieve a key-value pair from a hash table when value is void* pointer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// val pointer may be 0 if we just want to test for existance
int mhash_getptr( malloc_hash_t *hashtable, const char *key, const void **val)
{
    if(!hashtable) return MHASH_INVARG;
    return hashtable_get(&hashtable->hash, key, val);
}

// Retrieve a key-value pair from a hash table when value is integer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// val pointer may be 0 if we just want to test for existance
int mhash_getint( malloc_hash_t *hashtable, const char *key, int *val)
{
    const void* p;
    int i;

    if(!hashtable) return MHASH_INVARG;

    i = hashtable_get(&hashtable->hash, key, &p);
    if(val) *val = (int) p;
    return i;
}


// Retrieve a key-value pair from a hash table when value is malloced string
// Retrieved string is strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_NOMEM      if no memory for string copy
// MHASH_OK         if success
// val pointer may be 0 if we just want to test for existance
int mhash_getstr( malloc_hash_t *hashtable, const char *key, char **val)
{
    char *p, *p2;
    int i;

    if(!hashtable) return MHASH_INVARG;

    i = hashtable_get(&hashtable->hash, key, (const void**)&p);
    if(i < 0) return i;

    if(val)
    {
        p2 = mystrdup(p);
        if(!p2) return MHASH_NOMEM;
        *val = p2;
    }
    return MHASH_OK;
}




// Insert a key-value pair into a hash table when value is void* pointer
// key string is strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_DUPL       if key already exists
// MHASH_OK         if successful
int mhash_setptr( malloc_hash_t *hashtable, const char *key, const void *value )
{
    char* k;
    int i;

    if(!hashtable) return MHASH_INVARG;
    if(!key) return MHASH_INVARG;

    k = mystrdup(key);
    if(!k) return MHASH_NOMEM;

    i = hashtable_set(&hashtable->hash, k, value);
    if(i == HASHTABLE_DUPL) free(k);
    else if(i == HASHTABLE_FULL)
    {
        // table is full, we have to allocate more space
        // and try again
        i = mhash_resize(hashtable);
        if(i) free(k);
        else
        {
            i = hashtable_set(&hashtable->hash, k, value);
            if(i) free(k);
        }
    }
    else if(i) free(k);

    return i;
}



// Replace a key-value pair into a hash table when value is void* pointer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_OK         if successful
int mhash_replaceptr( malloc_hash_t *hashtable, const char *key, const void *value )
{
    const char *k;
    int i;

    // TODO: iskoristiti value_out da se vidi je li kljuc postojao ranije
    // i ako jeste odmah osloboditi ovu kopiju kljuca. Na taj nacin bi se
    // izbjeglo koristenje hashtable_test() funkcije ali bi se zato uvijek
    // pravila nova kopija kljuca funkcijom mystrdup().

    if(!hashtable) return MHASH_INVARG;
    if(!key) return MHASH_INVARG;

    i = hashtable_test(&hashtable->hash, key);
    if(i == HASHTABLE_INVARG) return MHASH_INVARG;
    if(i == HASHTABLE_NOTFOUND) k = mystrdup(key);
    else k = key;
    if(!k) return MHASH_NOMEM;
    i = hashtable_getset(&hashtable->hash, k, value, 0);
    if(i == HASHTABLE_FULL)
    {
        // table is full, we have to allocate more space
        // and try again
        i = mhash_resize(hashtable);
        if(i) free((void*)k);
        else
        {
            i = hashtable_set(&hashtable->hash, k, value);
            if(i) free((void*)k);
        }
    }
    else if(i) free((void*)k);

    return i;
}





// Insert a key-value pair into a hash table when value is malloced string
// key and value strings are strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_DUPL       if key already exists
// MHASH_OK         if successful
int mhash_setstr( malloc_hash_t *hashtable, const char *key, const char *value )
{
    char *k, *v;
    int i;

    if(!hashtable) return MHASH_INVARG;
    if(!key) return MHASH_INVARG;
    if(!value) return MHASH_INVARG;

    k = mystrdup(key);
    if(!k) return MHASH_NOMEM;

    v = mystrdup(value);
    if(!v)
    {
        free((void*)k);
        return MHASH_NOMEM;
    }

    i = hashtable_set(&hashtable->hash, k, v);
    if(i == HASHTABLE_DUPL) free((void*)k), free((void*)v);
    else if(i == HASHTABLE_FULL)
    {
        // table is full, we have to allocate more space
        // and try again
        i = mhash_resize(hashtable);
        if(i) free((void*)k), free((void*)v);
        else
        {
            i = hashtable_set(&hashtable->hash, k, v);
            if(i) free((void*)k), free((void*)v);
        }
    }
    else if(i) free((void*)k), free((void*)v);

    return i;
}



// Replace a key-value pair into a hash table when value is malloced string
// if key exists old value is feeed and new is strduped. if key dos not exists
// key and value strings are strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_OK         if successful
int mhash_replacestr( malloc_hash_t *hashtable, const char *key, const char *value )
{
    char *k, *v, *oldv;
    int i;

    // TODO: iskoristiti value_out da se vidi je li kljuc postojao ranije
    // i ako jeste odmah osloboditi ovu kopiju kljuca. Na taj nacin bi se
    // izbjeglo koristenje hashtable_test() funkcije ali bi se zato uvijek
    // pravila nova kopija kljuca funkcijom mystrdup().

    if(!hashtable) return MHASH_INVARG;
    if(!key) return MHASH_INVARG;
    if(!value) return MHASH_INVARG;

    k = mystrdup(key);
    if(!k) return MHASH_NOMEM;

    v = mystrdup(value);
    if(!v)
    {
        free((void*)k);
        return MHASH_NOMEM;
    }

    i = hashtable_getset(&hashtable->hash, k, v, (const void**)&oldv);
    if(i == HASHTABLE_PARTIAL)
    {
        // SET OK, GET failed, so we know (k,v) is new pair in hash
        // we have nothing to do, just set i to MHASH_OK.
        i = MHASH_OK;
    }
    else if(i == HASHTABLE_OK)
    {
        // SET OK, GET ok, so we know (k,v) is not new pair in hash
        // we have to deallocate k and oldv
        free((void*)k);
        free((void*)oldv);
    }
    else if(i == HASHTABLE_FULL)
    {
        // table is full, we have to allocate more space and try again
        i = mhash_resize(hashtable);
        if(i) free((void*)k), free((void*)v);
        else
        {
            i = hashtable_set(&hashtable->hash, k, v);
            if(i) free((void*)k), free((void*)v);
        }
    }
    else if(i) free((void*)k), free((void*)v);

    return i;
}




// Delete entry from hash
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// if deallocate_val is true value will be free'd
// key will always be free'd
int mhash_del( malloc_hash_t *hashtable, const char *key, int deallocate_val)
{
    const char *v, *k;
    int i;

    if(!hashtable) return MHASH_INVARG;
    if(!key) return MHASH_INVARG;

    i = hashtable_delex(&hashtable->hash, key, &k, (const void**)&v);
    if(i) return i;

    free((void*)k);
    if(deallocate_val) free((void*)v);

    return MHASH_OK;
}



// function for walking through all entries in hash
// callback function fcn is called for every entry. If it returns 0
// walk will continue; if it returns != 0 walk will stop
void mhash_walk(malloc_hash_t *hashtable, int (*fcn)(const hashentry_t*))
{
    hashtable_walk(&hashtable->hash, fcn);
}



// this function will resize hash table
int mhash_resize(malloc_hash_t *hashtable)
{
    malloc_hash_t t;
    int table_size, pool_size, i;

    // we allow load_factor up to MAX_LOAD_FACTOR
    if(hashtable->hash.pool_size < DEFAULT_POOL_SIZE)
    {
        // by default we addjust only pool size
        table_size = hashtable->hash.table_size;
        pool_size = hashtable->pool_size + hashtable->pool_increment;

        // table size is addjusted only if load factor become >= MAX_LOAD_FACTOR
        if(pool_size > MAX_LOAD_FACTOR * table_size) while(pool_size > table_size) table_size <<= 1;

        i = mhash_init(&t, table_size, pool_size);
        if(i) return i;

        // now we have to walk the old hash and copy values to new hash
        hashtable_copy(&hashtable->hash, &t.hash);

        // now we cak free old hash table and pool
        free(hashtable->hash.table);
        free(hashtable->hash.pool);

        // last step is to copy new hash struct to old one
        hashtable->pool_size = pool_size;
        hashtable->hash.table_size = table_size;
        hashtable->hash.pool_size = t.hash.pool_size;
        hashtable->hash.pool = t.hash.pool;
        hashtable->hash.table = t.hash.table;
    }

    return 0;
}
