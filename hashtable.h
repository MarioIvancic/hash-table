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

// hashtable.h
// Minimalistic hash table implementation in C
// Does not require dynamic memory allocation
// Hash table is table of void* vals with char* keys
// Vals and keys are never copied

/*
    Usage:

    hashtable_t hashtable;
    hashentry_t* table[16];
    hashentry_t  pool[32];
    char* val;

    hashtable_init( &hashtable, table, 16, pool, 32 );

    hashtable_set( &hashtable, "key1", "inky" );
    hashtable_set( &hashtable, "key2", "pinky" );
    hashtable_set( &hashtable, "key3", "blinky" );
    hashtable_set( &hashtable, "key4", "floyd" );

    hashtable_get( &hashtable, "key1", (void*)&val );
    printf( "%s\n", val );

    hashtable_get( &hashtable, "key2", (void*)&val );
    printf( "%s\n", val );

    hashtable_get( &hashtable, "key3", (void*)&val );
    printf( "%s\n", val );

    hashtable_get( &hashtable, "key4", (void*)&val );
    printf( "%s\n", val );
*/


#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <stdint.h>

typedef struct hashentry_s
{
    const char *key;            // pointer to null terminated key
    const void *value;          // pointer to value or value itself
    struct hashentry_s *next;   // pointer to next element in bin
} hashentry_t;

typedef struct hashtable_s
{
    hashentry_t**   table;      // array of pointers to elements
    int             table_size; // array size, we need this for bin index
    // table_size MUST NOT CHANGE during object lifetime

    hashentry_t*    pool;       // pool of unused elements
    int             pool_size;  // pool size. If 0 the hash is full.
} hashtable_t;


// error codes
enum
{
    HASHTABLE_PARTIAL   =  1,   // partial success
    HASHTABLE_OK        =  0,   // no error
    HASHTABLE_INVARG    = -1,   // invalid argument
    HASHTABLE_FULL      = -2,   // hash table is full
    HASHTABLE_NOTFOUND  = -3,   // key not found
    HASHTABLE_DUPL      = -4,   // if key is already in hash
    HASHTABLE_LASTERR   = -5,   // not used but must be last entry in enum
};


// One-at-a-Time Hash.
// Taken from http://www.burtleburtle.net/bob/hash/doobs.html and lookup3.c,
// by Bob Jenkins, May 2006, Public Domain.
// Equivalent to Perl's hash from Don Owens libcfu.
// Modified by Mario Ivancic to support null terminated strings without len.
uint32_t hash_function(const char *key, uint32_t initval);


// Initialize a new hashtable.
// arguments:
// hash         pointer to hashtable_t object
// table        pointer to array of pointers to hashentry_t objects
// table_size   size of table array, must be power of 2
// pool         pointer to array of hashentry_t objects
// pool_size    size of hash pool (this is hash capacity)
// returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_OK         if success
int hashtable_init(hashtable_t* hash, hashentry_t** table, int table_size,
                   hashentry_t* pool, int pool_size);


// Insert a key-value pair into a hash table and return the old one.
// returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_FULL       if hash is full
// HASHTABLE_OK         if set is successful and *value_out is valid
// HASHTABLE_PARTIAL    if set is successful and *value_out is not valid (*value_out = 0)
// value_aut pointer may be 0 if we just want to SET but not GET.
// if value_aut is 0 hashtable_getset will never return HASHTABLE_PARTIAL.
// Note: if key already axists key pointer will not be replaced
int hashtable_getset( hashtable_t *hashtable, const char *key, const void *value_in,
                     const void** value_aut );



// Retrieve a key-value pair from a hash table.
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_NOTFOUND   if key is not found
// HASHTABLE_OK         if success
// val pointer may be 0 if we just want to test for existance
int hashtable_get( hashtable_t *hashtable, const char *key, const void **val);



// Insert a key-value pair into a hash table.
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_FULL       if hash is full
// HASHTABLE_DUPL       if key is already in hash
// HASHTABLE_OK         if successful
int hashtable_set( hashtable_t *hashtable, const char *key, const void *value );



// Delete entry from hash
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_NOTFOUND   if key is not found
// HASHTABLE_OK         if success
// After return *value will be set to deleted value
// Note: key_out and/or value can be 0
int hashtable_delex( hashtable_t *hashtable, const char *key_in, const char** key_out,
                  const void** value);


// Test for entry existance
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_NOTFOUND   if key is not found
// HASHTABLE_OK         if success
#define hashtable_test(hash, key) hashtable_get((hash), (key), 0)


// Replace a key-value pair into a hash table.
// If key already exists value will be overwritten
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_FULL       if hash is full
// HASHTABLE_OK         if successful
#define hashtable_replace(hash, key, val) hashtable_getset((hash), (key), (val), 0)


// Delete entry from hash
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_NOTFOUND   if key is not found
// HASHTABLE_OK         if success
#define hashtable_del( hash, key) hashtable_delex((hash), (key), 0, 0)


// function for walking through all entries in hash
// callback function fcn is called for every entry. If it returns 0
// walk will continue; if it returns != 0 walk will stop
void hashtable_walk(hashtable_t *hashtable, int (*fcn)(const hashentry_t*));

void hashtable_print(hashtable_t *hashtable);


// copy all entries from old hash into new hash.
// new_hash must be initialized before this call.
// after this call memory occupied by this hash table may be destroyed.
// It means old_hash object, hashentry_t* table array and hashentry_t pool
// can be deleted or reused.
// This function is usefull whan you ran out of hash entries and you
// don't want to just add pool because it will increase load factor.
// Downside is that this function will rehash each entry from old hash.
void hashtable_copy(hashtable_t *old_hash, hashtable_t* new_hash);


// add new hashentry_t array 'pool' of size 'pool_size' to hash pool
// 'hash->pool'. 'hash->pool_size' is incremented by 'pool_size'.
// this function is usefull when you run out of hash entries but you don't want
// to make a new hash and rehash everything. Downside is that load factor will
// increase because table size is still the same.
void hashtable_add2pool(hashtable_t* hash, hashentry_t* pool, int pool_size);


#endif // __HASHTABLE_H__
