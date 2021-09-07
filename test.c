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

#include  <stdlib.h>
#include <time.h>
#include <unistd.h>


cachehash *ch = NULL;



char *gen_rdm_bytestream(int num_bytes)
{
  char *stream = malloc(num_bytes);
  int i;

  int randlength = rand()%num_bytes+1;
  for (i = 0; i < randlength; i++)
  {
    stream[i] = rand()%32+34;
    //printf("i = %d,",i);
  }

  stream[randlength] = '\0';
//  printf("dDDDDDDD = %s\n",stream);
  return stream;
}


void *randstring(int length) {
    char *string = "abcdefghijklmnopqrstuvwxyz";
    size_t stringLen = 36;
    char *randomString;

    randomString = malloc(sizeof(char) * (length +1));
    if (!randomString) {
	    printf("WHAHT??!??!?!?!!?!?!\n\n\n");
        return (char*)0;
    }
    unsigned int key = 0;
    for (int n = 0;n < length;n++) {
        key = rand() % stringLen;
        randomString[n] = string[key];
    }
    randomString[length] = '\0';

    if (strlen(randomString) != 1) {
	    printf("DAMNMMMM??!??!?!?!!?!?! = %d\n\n", strlen(randomString));
    }
    return randomString;
}


int seqnum = 100000;
int seqnum1 = 0;

char* seqstring() {
	char* str;
	asprintf(&str, "%i", seqnum--);
	if (seqnum == 0) {
		seqnum = 100000;
	}
	//free(str);
	return str;
}

char* seqstr() {
	char* str;
	asprintf(&str, "%i", seqnum1++);
	//free(str);
	return str;
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
	cachehash_put(ch, key, strlen(key), value, strlen(value));
	cachehash_debug_dump(ch);
	key = "test-key-2";
	value = "test-value-2";
	cachehash_put(ch, key, strlen(key), value, strlen(value));
	cachehash_debug_dump(ch);
}

void testPutAndHas(void)
{
	ch = cachehash_init(5, NULL);
	char *key = "test-key";
	char *value = "test-value";
	cachehash_put(ch, key, strlen(key), value, strlen(value));
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
	cachehash_put(ch, key, strlen(key), value, strlen(value));
	cachehash_debug_dump(ch);
	printf("value of has is %s\n", cachehash_has(ch, key, strlen(key)));
	assert(cachehash_has(ch, key, strlen(key)) == value);
	cachehash_free(ch, NULL);

}

const char* C_Whitespaces = " \t\n\r\f\v";
char* rtrim(char* s)
{
	char* r = s, * end;
	size_t len = strlen(s) - 1;
	for (end = s+len;
			strstr(C_Whitespaces, r+len) && end != s;
			end = s+len , *(r + (len--)) = 0);
	return r;
}

char* getline(void) {
	char *line = malloc(100), *linep = line;
	size_t lenmax = 100, len = lenmax;
	int c;

	if(line == NULL)
		return NULL;

	for(;;) {
		c = fgetc(stdin);
		if(c == EOF)
			break;

		if(--len == 0) {
			len = lenmax;
			char * linen = realloc(linep, lenmax *= 2);

			if(linen == NULL) {
				free(linep);
				return NULL;
			}
			line = linen + (line - linep);
			linep = linen;
		}

		if((*line++ = c) == '\n')
			break;
	}
	*line = '\0';
	rtrim(linep);
	return linep;
}


int main(void)
{

	char *keya;
	char *valuea;

	clock_t start_time = clock();

	int i;
	char *data;

	ch = cachehash_init(6, NULL);


	for(i=0; i<800000; i++){
		//keya = randstring(1);
		//keya = seqstring();
		//keya = gen_rdm_bytestream(2);
		keya = getline();

		//void *keya = randstring(2);
		//valuea = seqstr();
		valuea = seqstr();
//		valuea = seqstring();

//		printf("value of has is %d. %s = cachehash = %s = %d\n", i, keya, cachehash_has(ch, keya, strlen(keya)), strlen(keya));

//		printf("test = %s = %s\n",keya,valuea);

//		printf("test\n\n",keya,valuea);



		cachehash_replace(ch, keya, strlen(keya), valuea, strlen(valuea));
//		cachehash_debug_dump(ch);
		//              putK(ch, keya, valuea);
		/*
		   data = cachehash_get(ch, keya, sizeof(keya));


		   printf("value of has is %s = %s\n", cachehash_has(ch, keya, sizeof(keya)), data);
		   assert(cachehash_has(ch, keya, sizeof(keya)) == valuea);


		   keya = gen_rdm_bytestream(2);
		   cachehash_del(ch, keya, sizeof(keya));

		   cachehash_debug_dump(ch);
		   */
		free(keya);
		free(valuea);

	}

	printf("=================================\n");
	cachehash_debug_dump(ch);
	for(i=0;i<100000;i++){
		keya = getline();
		data = cachehash_get(ch, keya, strlen(keya));
		printf("value of has is %s = %s = %s\n", keya, cachehash_has(ch, keya, sizeof(keya)), data);

	}




	double elapsed_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
	printf("Done in %f seconds\n", elapsed_time);


	/*
	   testCreation();
	   printf("--\n");
	   testPut();
	   printf("--\n");
	   testPutAndHas();
	   */
}
