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
// based on https://gist.github.com/tonious/1377667

#include "hashtable.h"
#include <string.h>

#define HASH_INIT_VALUE 0
#define HASHTABLE_PRINT


// One-at-a-Time Hash.
// Taken from http://www.burtleburtle.net/bob/hash/doobs.html and lookup3.c,
// by Bob Jenkins, May 2006, Public Domain.
// Equivalent to Perl's hash from Don Owens libcfu.
// Modified by Mario Ivancic to support null terminated strings without len.
uint32_t hash_function(const char *key, uint32_t initval)
{
    while(*key)
    {
        initval += *key++;
        initval += (initval << 10);
        initval ^= (initval >> 6);
    }
    initval += (initval << 3);
    initval ^= (initval >> 11);
    initval += (initval << 15);
    return initval;
}



// make bin index using hash function
static inline uint32_t hashtable_hash( hashtable_t *hashtable, const char *key )
{
    // make 32 bit hash value
    uint32_t hashval = hash_function(key, HASH_INIT_VALUE);

    // now reduce it to table_size
    return hashval & (hashtable->table_size - 1);
}



// test if number is power of 2
// return !=0 if yes, 0 if not
static int is_power_of_2(int x)
{
	return ( (x > 0) && ((x & (x - 1)) == 0) );
}


// Create a key-value pair.
// Unused entries are in pool in linked list. This function will take one
// entry from the pool (if available) and copy key and value pointers in it.
// Returns:
// pointer to key-value pair if success
// 0 if there is no available entries in pool
static hashentry_t *hashtable_newpair( hashtable_t* hash, const char *key, const void *value )
{
    hashentry_t *newpair;

    if(hash->pool_size < 1) return 0;
    hash->pool_size--;
    newpair = hash->pool;
    hash->pool = hash->pool->next;      // it will become 0 if pool is empty

    newpair->key = key;
    newpair->value = value;
    newpair->next = 0;

    return newpair;
}


// return hash entry to hash pool
static inline void return_to_pool( hashtable_t* hash, hashentry_t* entry )
{
    entry->next = hash->pool;      // it will become 0 if pool was empty
    hash->pool = entry;
    hash->pool_size++;
}




// Initialize a new hashtable.
// arguments:
// hash         pointer to hashtable_t object
// table        pointer to array of pointers to hashentry_t objects
// table_size   size of table array, must be power of 2
// pool         pointer to array of hashentry_t objects
// pool_size    size of hash pool (this is hash capacity)
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_OK         if success
int hashtable_init(hashtable_t* hash, hashentry_t** table, int table_size,
                   hashentry_t* pool, int pool_size)
{
    int i;

    if(!hash) return HASHTABLE_INVARG;                        // error
    if(!table) return HASHTABLE_INVARG;                       // error
    if(!pool) return HASHTABLE_INVARG;                        // error
    if(table_size < 0) return HASHTABLE_INVARG;               // error

    if(!is_power_of_2(table_size)) return HASHTABLE_INVARG;   // error

    hash->table_size = table_size;
    hash->pool_size = pool_size;
    hash->table = table;
    hash->pool = pool;

    // clear table
    for( i = 0; i < table_size; i++ )
    {
        table[i] = 0;
    }

    // make pool as linked list
    for( i = 0; i < pool_size - 1; i++ )
    {
        pool[i].next = &pool[i + 1];
    }

    // last element in pool list must have .next = 0
    pool[i].next = 0;

    return HASHTABLE_OK;
}



// Retrieve a key-value pair from a hash table.
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_NOTFOUND   if key is not found
// HASHTABLE_OK         if success
// val pointer may be 0 if we just want to test for existance
int hashtable_get( hashtable_t *hashtable, const char *key, const void **val)
{
    int bin = 0;
    hashentry_t *pair;

    if(!hashtable) return HASHTABLE_INVARG;
    if(!key) return HASHTABLE_INVARG;

    bin = hashtable_hash( hashtable, key );

    // Step through the bin, looking for our value.
    pair = hashtable->table[ bin ];
    while( pair != 0 && pair->key != 0 && strcmp( key, pair->key ) > 0 )
    {
        pair = pair->next;
    }

    // Did we actually find anything?
    if( pair == 0 || pair->key == 0 || strcmp( key, pair->key ) != 0 )
    {
        return HASHTABLE_NOTFOUND;  // not found
    }

    if(val) *val = pair->value;
    return HASHTABLE_OK;
}



// Insert a key-value pair into a hash table and return the old one.
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_FULL       if hash is full
// HASHTABLE_OK         if set is successful and *value_out is valid
// HASHTABLE_PARTIAL    if set is successful and *value_out is not valid (*value_out = 0)
// value_aut pointer may be 0 if we just want to SET but not GET.
// if value_aut is 0 hashtable_getset will never return HASHTABLE_PARTIAL.
// Note: if key already axists key pointer will not be replaced
int hashtable_getset( hashtable_t *hashtable, const char *key, const void *value_in, const void** value_aut )
{
    int bin = 0;
    hashentry_t *newpair = 0;
    hashentry_t *next = 0;
    hashentry_t *last = 0;

    if(!hashtable) return HASHTABLE_INVARG; // invalid argument
    if(!key) return HASHTABLE_INVARG;       // invalid argument

    // get bin index based on key
    bin = hashtable_hash( hashtable, key );

    // get list head from the bin
    next = hashtable->table[ bin ];

    // walk the list and find the key or find a place for new key
    // keys in list are sorted by key length, shortest first
    while( next != 0 && next->key != 0 && strcmp( key, next->key ) > 0 )
    {
        last = next;
        next = next->next;
    }

    // did we find it?
    if( next != 0 && next->key != 0 && strcmp( key, next->key ) == 0 )
    {
        // There's already a pair. Let's replace that pointer.
        if(value_aut) *value_aut = next->value;
        next->value = value_in;
        return HASHTABLE_OK;
    }

    // Nope, could't find it. Time to grow a pair.
    newpair = hashtable_newpair( hashtable, key, value_in );
    if(!newpair) return HASHTABLE_FULL;     // hash is full

    // we must test are we at the list head?
    if( next == hashtable->table[ bin ] )
    {
        // We're at the start of the linked list in this bin.
        // insert newpair to list head
        newpair->next = next;
        hashtable->table[ bin ] = newpair;
    }
    else
    {
        // We're at the end of the linked list in this bin.
        // or
        // We're in the middle of the list (because the list is sorted).
        newpair->next = next;
        last->next = newpair;
    }

    if(value_aut)
    {
        *value_aut = 0;                 // *value_out is not valid
        return HASHTABLE_PARTIAL;
    }
    return HASHTABLE_OK;                // if value_out is not a valid
                                        // pointer we do not care for GET
                                        // part. SET part was successful so
                                        // we return 0 here.
}




// Insert a key-value pair into a hash table.
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_FULL       if hash is full
// HASHTABLE_DUPL       if key is already in hash
// HASHTABLE_OK         if successful
int hashtable_set( hashtable_t *hashtable, const char *key, const void *value )
{
    int bin = 0;
    hashentry_t *newpair = 0;
    hashentry_t *next = 0;
    hashentry_t *last = 0;

    if(!hashtable) return HASHTABLE_INVARG; // invalid argument
    if(!key) return HASHTABLE_INVARG;       // invalid argument

    // get bin index based on key
    bin = hashtable_hash( hashtable, key );

    // get list head from the bin
    next = hashtable->table[ bin ];

    // walk the list and find the key or find a place for new key
    // keys in list are sorted by key length, shortest first
    while( next != 0 && next->key != 0 && strcmp( key, next->key ) > 0 )
    {
        last = next;
        next = next->next;
    }

    // did we find it?
    if( next != 0 && next->key != 0 && strcmp( key, next->key ) == 0 )
    {
        // There's already a pair. Return error
        return HASHTABLE_DUPL;
    }

    // Nope, could't find it. Time to grow a pair.
    newpair = hashtable_newpair( hashtable, key, value );
    if(!newpair) return HASHTABLE_FULL;     // hash is full

    // we must test are we at the list head?
    if( next == hashtable->table[ bin ] )
    {
        // We're at the start of the linked list in this bin.
        // insert newpair to list head
        newpair->next = next;
        hashtable->table[ bin ] = newpair;
    }
    else
    {
        // We're at the end of the linked list in this bin.
        // or
        // We're in the middle of the list (because the list is sorted).
        newpair->next = next;
        last->next = newpair;
    }

    return HASHTABLE_OK;
}



// Delete entry from hash
// Returns:
// HASHTABLE_INVARG     if invalid argument
// HASHTABLE_NOTFOUND   if key is not found
// HASHTABLE_OK         if success
// After return (*key_out, *value) will be set to deleted pair
// Note: key_out and/or value can be 0
int hashtable_delex( hashtable_t *hashtable, const char *key_in, const char** key_out, const void** value)
{
    int bin = 0;
    hashentry_t *next = 0;
    hashentry_t *last = 0;

    if(!hashtable) return -1;       // invalid argument
    if(!key_in) return -1;             // invalid argument

    // get bin index based on key
    bin = hashtable_hash( hashtable, key_in );

    // get list head from the bin
    next = hashtable->table[ bin ];

    // walk the list and find the key
    // keys in list are sorted by key length, shortest first
    while( next != 0 && next->key != 0 && strcmp( key_in, next->key ) > 0 )
    {
        last = next;
        next = next->next;
    }

    // did we find it?
    if( next != 0 && next->key != 0 && strcmp( key_in, next->key ) == 0 )
    {
        // we find it
        if(value) *value = next->value;
        if(key_out) *key_out = next->key;

        // we must test are we at the list head?
        if( next == hashtable->table[ bin ] )
        {
            // We're at the start of the linked list in this bin.
            hashtable->table[ bin ] = next->next;
            return_to_pool(hashtable, next);
        }
        else
        {
            // We're at the end of the linked list in this bin.
            // or
            // We're in the middle of the list (because the list is sorted).
            last->next = next->next;
            return_to_pool(hashtable, next);
        }

        return HASHTABLE_OK;
    }

    // Nope, could't find it.
    return HASHTABLE_NOTFOUND;
}


// function for walking through all entries in hash
// callback function fcn is called for every entry. If it returns 0
// walk will continue; if it returns != 0 walk will stop
void hashtable_walk(hashtable_t *hashtable, int (*fcn)(const hashentry_t*))
{
    int i;

    for(i = 0; i < hashtable->table_size; i++)
    {
        hashentry_t* e = hashtable->table[ i ];
        while(e)
        {
            // do something with e->key and e->value
            if(fcn(e)) break;
            e = e->next;
        }
        if(e) break;
    }
}


#ifdef HASHTABLE_PRINT
#include <stdio.h>

void hashtable_print(hashtable_t *hashtable)
{
    int i;

    for(i = 0; i < hashtable->table_size; i++)
    {
        hashentry_t* e = hashtable->table[ i ];
        while(e)
        {
            // do something with e->key and e->value
            printf("[%05d] key=%s, val=%s\n", i, e->key, (char*)e->value);
            e = e->next;
        }
    }
}
#endif // HASHTABLE_PRINT



// copy all entries from old hash into new hash.
// new_hash must be initialized before this call.
// after this call memory occupied by this hash table may be destroyed.
// It means old_hash object, hashentry_t* table array and hashentry_t pool
// can be deleted or reused.
// This function is usefull whan you ran out of hash entries and you
// don't want to just add pool because it will increase load factor.
// Downside is that this function will rehash each entry from old hash.
void hashtable_copy(hashtable_t *old_hash, hashtable_t* new_hash)
{
    int i;

    for(i = 0; i < old_hash->table_size; i++)
    {
        hashentry_t* e = old_hash->table[ i ];
        while(e)
        {
            hashtable_set(new_hash, e->key, e->value);
            e = e->next;
        }
    }
}


// add new hashentry_t array 'pool' of size 'pool_size' to hash pool
// 'hash->pool'. 'hash->pool_size' is incremented by 'pool_size'.
// this function is usefull when you run out of hash entries but you don't want
// to make a new hash and rehash everything. Downside is that load factor will
// increase because table size is still the same.
void hashtable_add2pool(hashtable_t* hash, hashentry_t* pool, int pool_size)
{
    int i;

    // make pool as linked list
    for( i = 0; i < pool_size - 1; i++ )
    {
        pool[i].next = &pool[i + 1];
    }

    // last element in pool list must have .next = hash->pool
    // because new pool is prepended to old pool
    pool[i].next = hash->pool;
    hash->pool = pool;
    hash->pool_size += pool_size;
}
