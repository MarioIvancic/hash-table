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

// hash table with dynamic resizing

#ifndef __MALLOC_HASH_H__
#define __MALLOC_HASH_H__

#include "hashtable.h"

typedef struct
{
    hashtable_t hash;
    int         pool_size;
    int         pool_increment;
} malloc_hash_t;


// error codes
enum
{
    MHASH_OK = HASHTABLE_OK,                // no error
    MHASH_INVARG = HASHTABLE_INVARG,        // invalid argument
    MHASH_NOTFOUND = HASHTABLE_NOTFOUND,    // key not found
    MHASH_NOMEM = HASHTABLE_LASTERR,        // no memory for key/value
    MHASH_DUPL = HASHTABLE_LASTERR + 1,     // key already exists (duplicate keys)
};


// create new hash object, return 0 if unsuccessful
malloc_hash_t* mhash_create(void);


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
int mhash_init(malloc_hash_t* hash, int pool_size, int pool_increment);



// Retrieve a key-value pair from a hash table when value is void* pointer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// val pointer may be 0 if we just want to test for existance
int mhash_getptr( malloc_hash_t *hashtable, const char *key, const void **val);


// Retrieve a key-value pair from a hash table when value is integer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// val pointer may be 0 if we just want to test for existance
int mhash_getint( malloc_hash_t *hashtable, const char *key, int *val);


// Retrieve a key-value pair from a hash table when value is malloced string
// Retrieved string is strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_NOMEM      if no memory for string copy
// MHASH_OK         if success
// val pointer may be 0 if we just want to test for existance
int mhash_getstr( malloc_hash_t *hashtable, const char *key, char **val);


// Insert a key-value pair into a hash table when value is void* pointer
// key string is strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_DUPL       if key already exists
// MHASH_OK         if successful
int mhash_setptr( malloc_hash_t *hashtable, const char *key, const void *value );


// Insert a key-value pair into a hash table when value is integer
// key string is strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_DUPL       if key already exists
// MHASH_OK         if successful
#define mhash_setint( hash, key, val ) mhash_setptr((hash), (key), (void*)(val))


// Insert a key-value pair into a hash table when value is malloced string
// key and value strings are strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_DUPL       if key already exists
// MHASH_OK         if successful
int mhash_setstr( malloc_hash_t *hashtable, const char *key, const char *value );


// Replace a key-value pair into a hash table when value is void* pointer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_OK         if successful
int mhash_replaceptr( malloc_hash_t *hashtable, const char *key, const void *value );



// Replace a key-value pair into a hash table when value is integer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_OK         if successful
#define mhash_replaceint( hash, key, val ) mhash_replaceptr((hash), (key), (void*)(val))



// Replace a key-value pair into a hash table when value is malloced string
// if key exists old value is feeed and new is strduped. if key dos not exists
// key and value strings are strduped
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOMEM      if no memory
// MHASH_OK         if successful
int mhash_replacestr( malloc_hash_t *hashtable, const char *key, const char *value );


// Delete entry from hash
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// if deallocate_val is true value will be free'd
// key will always be free'd
int mhash_del( malloc_hash_t *hashtable, const char *key, int deallocate_val);


// Delete entry from hash when value is integer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// value will not be free'd
// key will always be free'd
#define mhash_delint( hash, key ) mhash_del((hash), (key), 0)


// Delete entry from hash when value is void* pointer
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// value will not be free'd
// key will always be free'd
#define mhash_delptr( hash, key ) mhash_del((hash), (key), 0)


// Delete entry from hash when value is malloced string
// Returns:
// MHASH_INVARG     if invalid argument
// MHASH_NOTFOUND   if key is not found
// MHASH_OK         if success
// value will always be free'd
// key will always be free'd
#define mhash_delstr( hash, key ) mhash_del((hash), (key), 1)



// function for walking through all entries in hash
// callback function fcn is called for every entry. If it returns 0
// walk will continue; if it returns != 0 walk will stop
void mhash_walk(malloc_hash_t *hashtable, int (*fcn)(const hashentry_t*));


// this function will resize hash table
int mhash_resize(malloc_hash_t *hashtable);


#endif // __MALLOC_HASH_H__
