/*
 * CacheHash Copyright 2014 Regents of the University of Michigan
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy 
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

//#include "triemap.h"
#include "cachehash.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <Judy.h>

#define EVICTION_NEEDED 1
#define EVICTION_UNNEC 0

// doubly-linked-list node
typedef struct node {
    struct node *next;
    struct node *prev;
    void *key;
    size_t keylen;
    void *data;
} node_t;


//TrieMap *tm = NewTrieMap();

// data structure that contains the enterity of a linked list
// we typedef this as cachehash in cachehash.h s.t. external interface is clean
struct cachehash_s {
    Pvoid_t judy;
    void *malloced;
    node_t *start;
    node_t *end;
    node_t *curr_end;
    size_t maxsize;
    size_t currsize;
    cachehash_process_cb *evict_cb;
};

cachehash* cachehash_init(size_t maxitems, cachehash_process_cb *cb)
{
    assert(maxitems > 0);
    cachehash *retv = malloc(sizeof(cachehash));
    assert(retv);
    memset(retv, 0, sizeof(cachehash));
    // allocate nodes all at once to avoid fragmented memory
    node_t *nodes = calloc(maxitems, sizeof(node_t));
    retv->malloced = nodes;
    assert(nodes);
    retv->start = nodes;
    retv->curr_end = nodes;
    retv->end = &nodes[maxitems-1];
    retv->maxsize = maxitems;
    retv->currsize = 0;
    // initial node
    nodes[0].next = &nodes[1];
    // middle nodes
    for (unsigned int i=1; i < maxitems - 1; i++) {
        nodes[i].prev = &nodes[i-1];
        nodes[i].next = &nodes[i+1];
    }
    // last node
    nodes[maxitems-1].prev = &nodes[maxitems-2]; return retv;
}

void cachehash_set_evict_cb(cachehash *ch, cachehash_process_cb *cb)
{
    ch->evict_cb = cb;
}

// is the hashcache full?
static inline int eviction_needed(cachehash *ch)
{
    assert(ch);
    return ch->currsize == ch->maxsize;
}

// completely evict the LRU object
// does not cb to user w/ object
static inline void* evict(cachehash *ch)
{

    
    assert(ch);
    node_t *last = ch->end;
    // remove item from judy array
    int rc;
    JHSD(rc, ch->judy, last->key, last->keylen);
    // we should never end up with something in the linked list
    // that's not in the judy array.
    assert(rc);
    // reset linked list node
    void *retv = last->data;
    free(last->key);
    last->key = NULL;
    last->keylen = 0;

    free(last->data);
    last->data = NULL;
    ch->currsize--;
    ch->curr_end = ch->end;
    return retv;
}

static inline void use(cachehash *ch, node_t *n)
{
    
    assert(ch);
    assert(n);
    //if first node, nothing to do and return
    if (n == ch->start) {
        return;
    }
    // remove from current spot in linked list
    node_t *prev = n->prev;
    n->prev->next = n->next;
    // if last node then no next, but must update LL
    if (n->next) {
        n->next->prev = prev;
    } else {
        ch->end = prev;
    }
    // front of list
    n->next = ch->start;
    ch->start->prev = n;
    ch->start = n;
    n->prev = NULL;
}
static inline void unuse(cachehash *ch, node_t *n)
{
    printf("HERERERERERE222\n");
        //node_t *last = ch->end;
        assert(ch);
        assert(n);
        //if first node, nothing to do and return
        // remove from current spot in linked list
        ch->currsize--;

	printf("HERERERERERE333\n");
        //reset clear n
        //free(n->data);

        int rc;
        JHSD(rc, ch->judy, n->key, n->keylen);
        if (rc) {
                printf("        UNUSED!!!!EXIST!!! = %s\n",n->key);
        }else{
                printf("        UNUSED!!!!ERROR NOT EXIST!!!!!!!\n");
                //cachehash_debug_dump(ch);
        }
        assert(rc);

        if (n == ch->end) {
                //                printf("ENDING!!!!!!!!!!!\n");
                free(n->data);
                n->data = NULL;
                free(n->key);
                n->key = NULL;
                n->keylen = 0;
                return;
        }

        if(ch->curr_end == n){
                ch->curr_end = ch->curr_end->next;
                ch->curr_end->prev = n->prev;
        }

        if (n == ch->start) {
                //move start to 2nd in linked list
                ch->start = n->next;

                free(n->data);
                n->data = NULL;
                free(n->key);
                n->key = NULL;
                n->keylen = 0;

                n->next = NULL;
                n->prev = ch->end;


                ch->end->next = n;
                ch->end = n;
                return;
        }



        free(n->data);
        n->data = NULL;
        free(n->key);
        n->key = NULL;
        n->keylen = 0;

        if (ch->curr_end == ch->start) {
                ch->curr_end == ch->end;

        }

        n->next->prev = n->prev;
        n->prev->next = n->next;

        n->next = NULL;
        n->prev = ch->end;
        ch->end->next = n;
        ch->end = n;




}




static inline node_t* judy_get(cachehash *ch, void *key, size_t keylen)
{
    assert(ch);
    assert(key);
    assert(keylen);
    Word_t *v_;
    JHSG(v_, ch->judy, key, keylen);
    if (!v_) {
        return NULL;
    }
    return (node_t*) *v_;
}


void* cachehash_has(cachehash *ch, const void *key, size_t keylen)
{
    assert(ch);
    assert(key);
    assert(keylen);
    node_t *n = judy_get(ch, key, keylen);
    if (n) {
        return n->data;
    } else {
        return NULL;
    }
}

void* cachehash_get(cachehash *ch, const void *key, size_t keylen)
{
    assert(ch);
    assert(key);
    assert(keylen);

    node_t *n = judy_get(ch, key, keylen);
    if (n) {
        use(ch, n);
        return n->data;
    } else {
        return NULL;
    }
}

void* cachehash_evict_if_full(cachehash *ch)
{
    assert(ch);
    if (eviction_needed(ch) == EVICTION_UNNEC) {
        return NULL;
    }
    return evict(ch);
}

void cachehash_put(cachehash *ch, const void *key, size_t keylen, void *value, size_t valuelen)
{
    assert(ch);
    assert(key);
    assert(keylen);

    void *evicted = cachehash_evict_if_full(ch);
    if (evicted && ch->evict_cb) {
       ch->evict_cb(evicted); 
       ch->curr_end = ch->end;
    }
    // create new node

    node_t *n;
    void *newkey = malloc(keylen+1);
    n = ch->curr_end;
    memcpy(newkey, key, keylen+1);

    void *newvalue = malloc(valuelen+1);
    memcpy(newvalue, value, valuelen+1);

    if (n->key) {
	    free(n->key);
	    n->key = NULL;
    }

    n->key = newkey;
    n->keylen = keylen;
    if (n->data) {
	    free(n->data);
	    n->data = NULL;
    }
    n->data = newvalue;
    //n->prev = ch->curr_end->prev;
    //n->next = ch->curr_end->next;

    if(ch->curr_end != ch->end){ 
        ch->curr_end = ch->curr_end->next;  
        ch->curr_end->prev = n;
    }


    use(ch, n);
    ch->currsize++;
    // add to judy array
    Word_t *v_;
    JHSI(v_, ch->judy, key, keylen);
    // key should not already be in hash table
    assert(!*v_);
    *v_ = (Word_t) n;

//    free(newkey);
//    free(newvalue);
}



//void cachehash_replace(cachehash *ch, void *key, int keylen, char *value, int valuelen)
void cachehash_replace(cachehash *ch, const void *key, size_t keylen, void *value, size_t valuelen)
{


        Word_t *v_;
        printf("\tREPLACE!!!! = %s -> %d\n", (void*) key, strlen(key));
        //      pthread_mutex_lock(&lock);
//        pthread_mutex_lock(&lock);
        JHSG(v_, ch->judy, key, keylen);
        if (!v_) {
                cachehash_put(ch, key, keylen, value, valuelen);
                //    pthread_mutex_unlock(&lock);
                printf("        JUDY DEL!!!!NOT EXIST?!\n");
        }else{ 
                printf("        JUDY DEL!!!!EXIST!!!\n");
//                int rc;
                //    pthread_mutex_lock(&lock);
                //                JHSD(rc, ch->judy, key, keylen);
                //                assert(rc);
                printf("        !!!!JJJJJ!!!\n");
                node_t *n = ch->start;
                do {
                        if (strcmp(n->key,key) == 0) {
                                printf("\tMATCHED!!! : %s -> %s\n", (void*) n->key, (void*) key);

                                //                              free(n->key);
                                //                              n->key = NULL;
                                //                              n->keylen = 0;

                                //                              free(n->data);
                                //                              n->data = NULL;
                                unuse(ch, n);
                                //cachehash_debug_dump(ch);
                                break;
                                //                        }else{
                                //                                printf("\tNOTTTTMATCHED!!! : %s -> %s\n", (void*) n->key, (void*) key);
                }
                n = n->next;
                } while (n);
                cachehash_put(ch, key, keylen, value, valuelen);
        }

        cachehash_debug_dump(ch);

/*

//        printf("key = %s ",key);
//        pthread_mutex_lock(&lock);
        Word_t *v_;
        JHSG(v_, ch->judy, key, keylen);
        if (!v_) {
//                pthread_mutex_unlock(&lock);
                //              printf("\tINSERT1 = %s : %x -> %d, %d -> %x -> %d\n", key, (void*) key, strlen(key), sizeof(key), (void*) value, sizeof(*value));
                cachehash_put(ch, key, keylen, value);
        }else{  
//                pthread_mutex_unlock(&lock);
                //              printf("\tINSERT2 = %x -> %d -> %d -> %x\n", (void*) key, sizeof(key), (void*) value, ((node_t*) *v_)->key);
                //node_t *nx = judy_get(ch, key, strlen(key));
                //usez(ch, n, key, value);
                //cachehash_get(ch, key);
                cachehash_del(ch, key, keylen);
                //cachehash_put(ch, key, keylen, value);
                //cachehash_put(ch, key, sizeof(*key), value, sizeof(*value));
        }
        cachehash_debug_dump(ch);
*/
}

void cachehash_del(cachehash *ch, const void *key, size_t keylen)
{
        Word_t *v_;
        //      printf("\tDELETE!!!! = %s -> %d\n", (void*) key, strlen(key));
        //      pthread_mutex_lock(&lock);
//        pthread_mutex_lock(&lock);
        JHSG(v_, ch->judy, key, keylen);
        if (!v_) {
                //    pthread_mutex_unlock(&lock);
                //                printf("        DEL!!!!NOT EXIST?!\n");
        }else{  
                //                printf("        DEL!!!!EXIST!!!\n");
//                int rc;
                //    pthread_mutex_lock(&lock);
                //                JHSD(rc, ch->judy, key, keylen);
                //                assert(rc);
                //                printf("        !!!!JJJJJ!!!\n");
                node_t *n = ch->start;
                do {
                        if (strcmp(n->key,key) == 0) {
                                //                                printf("\tMATCHED!!! : %s -> %s\n", (void*) n->key, (void*) key);

                                //                              free(n->key);
                                //                              n->key = NULL;
                                //                              n->keylen = 0;

                                //                              free(n->data);
                                //                              n->data = NULL;
                                unuse(ch, n);
                                //cachehash_debug_dump(ch);
                                break;
                                //                        }else{
                                //                                printf("\tNOTTTTMATCHED!!! : %s -> %s\n", (void*) n->key, (void*) key);
                }
                n = n->next;
                } while (n);
        }
        //      pthread_mutex_unlock(&lock);
//        pthread_mutex_unlock(&lock);
}



// print out entire state.
void cachehash_debug_dump(cachehash *ch)
{
    printf("Statistics:\n");
    printf("\tcurrent size: %lu\n", ch->currsize);
    printf("\tmaximum size: %lu\n", ch->maxsize);
    printf("\n");
    printf("Linked List:\n");
    size_t i = 0;
    node_t *n = ch->start;

    do {
	    if (n->key == ch->curr_end->key) {
		    printf("THIS IS CURR END KEY ");
	    }else{  
		    printf("\t\t");
	    }

	    if (n->key == ch->end->key) {
		    printf("THIS IS ENDING KEY ");
		    //}else{
		    //   printf("\t\t");
    }

    if (n->key) {
	    printf("\t\t%lu: %s -> %s\n", i++, (char*) n->key, (char*) n->data);
    } else {
	    printf("\t\t%lu: EMPTY\n", i++);
    }

    n = n->next;
    } while (n);
}

void cachehash_free(cachehash *ch, cachehash_process_cb *cb)
{
	assert(ch);
	int rc;
	JHSFA(rc, ch->judy);
	node_t *n = ch->start;
	do {
		if (n->key) {
			free(n->key);
			if (cb) {
				cb(n->data);
			}
		}
		n = n->next;
	} while(n);
	free(ch->malloced);
	free(ch);
} 

void cachehash_iter(cachehash *ch, cachehash_process_cb *cb)
{
	node_t *n = ch->start;
	do {
		if (n->key) {
			cb(n->data);
		} else {
			break;
		}
		n = n->next;
	} while (n);
}

