/*
 * CacheHash Copyright 2014 Regents of the University of Michigan
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#include <string.h>
#include "cachehash.h"
#include <assert.h>
#include <stdio.h>

cachehash *ch = NULL;

void *randstring(int length) {
    char *string = "abcdefghijklmnopqrstuvwxyz";
    size_t stringLen = 36;
    char *randomString;

    randomString = malloc(sizeof(char) * (length +1));
    if (!randomString) {
        return (char*)0;
    }
    unsigned int key = 0;
    for (int n = 0;n < length;n++) {
        key = rand() % stringLen;
        randomString[n] = string[key];
    }
    randomString[length] = '\0';
    return randomString;
}


void testCreation(void)
{
	ch = cachehash_init(5, NULL);
	assert(ch);
	cachehash_free(ch, NULL);
}

void testPut(void)
{
	ch = cachehash_init(5, NULL);
	char *key = "test-key";
	char *value = "test-value";
	cachehash_put(ch, key, strlen(key), value);
	cachehash_debug_dump(ch);
	key = "test-key-2";
	value = "test-value-2";
	cachehash_put(ch, key, strlen(key), value);
	cachehash_debug_dump(ch);
}

void testPutAndHas(void)
{
	ch = cachehash_init(5, NULL);
	char *key = "test-key";
	char *value = "test-value";
	cachehash_put(ch, key, strlen(key), value);
	cachehash_debug_dump(ch);
	printf("value of has is %s\n", cachehash_has(ch, key, strlen(key)));
	assert(cachehash_has(ch, key, strlen(key)) == value);
	cachehash_free(ch, NULL);
}

void evict_cb(void *arg)
{
	(void) arg;
}

void testEvict(void)
{
	ch = cachehash_init(5, NULL);
	char *key = "test-key";
	char *value = "test-value";
	cachehash_put(ch, key, strlen(key), value);
	cachehash_debug_dump(ch);
	printf("value of has is %s\n", cachehash_has(ch, key, strlen(key)));
	assert(cachehash_has(ch, key, strlen(key)) == value);
	cachehash_free(ch, NULL);

}


int main(void)
{

	char *keya;
	char *valuea;
	
        clock_t start_time = clock();
        
        int i;
        char *data;

        ch = cachehash_init(5, NULL);
        

        for(i=0; i<TAG_LEN; i++){
                keya = randstring(1);
                //void *keya = randstring(2);
                valuea = randstring(1);
		
		cachehash_replace(ch, keya, strlen(keya), valuea);
//              putK(ch, keya, valuea);
/*
                data = cachehash_get(ch, keya, sizeof(keya));
                cachehash_debug_dump(ch);


                printf("value of has is %s = %s\n", cachehash_has(ch, keya, sizeof(keya)), data);
                assert(cachehash_has(ch, keya, sizeof(keya)) == valuea);


                keya = gen_rdm_bytestream(2);
                cachehash_del(ch, keya, sizeof(keya));

                cachehash_debug_dump(ch);
		*/
                free(keya);
                free(valuea);

	}
	
	double elapsed_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
        printf("Done in %f seconds\n", elapsed_time);

	
	testCreation();
	printf("--\n");
	testPut();
	printf("--\n");
	testPutAndHas();
}
