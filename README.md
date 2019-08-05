# hashtable
Minimalistic hash table implementation in C. It does not require dynamic memory allocation, so it can be used on MCU.
Hash table is table of void* vals with char* keys. Vals and keys are never copied.

# malloc_hash
mhash is extension to hashtable. Key string is always strdup-ed, integer and pointer values are stored as-is and 
string values are strdup-ed.

