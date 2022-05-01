#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct fnode
{
        char* document_id;
        int num_occurrences;
        struct fnode* next;
};

struct llnode 
{
        char* word;
        int num_files;
        struct fnode* fn;       // File nodes
        struct llnode* next;
};

struct hashmap 
{
        struct llnode** map;
        int num_buckets;
};

struct hashmap* hm_create(int num_buckets);
int hm_get(struct hashmap* hm, char* word, char* document_id);
struct llnode* hm_find_word( struct hashmap* hm, char* word );
void hm_put(struct hashmap* hm, char* word, char* document_id, int num_occurrences);
void hm_remove(struct hashmap* hm, char* word, char* document_id);
void hm_destroy(struct hashmap* hm);
int hash(struct hashmap* hm, char* word);

#endif
