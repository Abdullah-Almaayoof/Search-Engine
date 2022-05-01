#include "hashmap.h"


//prototype
struct fnode* hm_find(struct hashmap* hm, char* word, char* document_id);
int printHash(struct hashmap* hm);

/*
function hm_create creates empty hashmap with the 
specified number of buckets.
*/
struct hashmap* hm_create(int num_buckets)
{
    // Create a hashmap structure
    struct hashmap* ret = malloc( sizeof(struct hashmap) );

    if ( ret == NULL )
    {
        perror("malloc map");
        return NULL;
    }

    // Create the buckets, all NULLs
    ret->map = calloc(sizeof(void*), num_buckets);

    if ( ret->map == NULL)
    {
        perror("calloc buckets");
        return NULL;
    }

    // Assign other hashmap values
    ret->num_buckets = num_buckets;
    return ret;
}

/*
Function hm_find_word returns the node supporting a word
or null if the word is not in the hashmap.
*/
struct llnode* hm_find_word( struct hashmap* hm, char* word )
{
    // Hash the keys, and get the bucket index
    int h = hash(hm, word);

    // Walk any linked list in the bucket
    for ( struct llnode* l = hm->map[h]; l != NULL; l = l->next )
    {
        // Ignore mismatched keys
        if ( ! strcmp( word, l->word ) )
        {
            return l;
        }
    }

    return NULL;
}

/*
Function hm_find returns the node supporting a word and file name or 
null if there is no such word or file.
*/
struct fnode* hm_find(struct hashmap* hm, char* word, char* document_id)
{
    struct llnode* l = hm_find_word( hm, word );

    if ( l != NULL )
    {
        for ( struct fnode* f = l->fn; f != NULL; f = f->next )
        {
            if ( strcmp( document_id, f->document_id ) )
            {
                continue;
            }
            
            // Return pointer to the node that was found
            return f;
        }
    }

    // Return null if not found
    return NULL;
}

/*
Function hm_get returns the count of occurrences of a word
in a file or -1 if the word is not in that file
*/
int hm_get(struct hashmap* hm, char* word, char* document_id)
{
    // Find matching node
    struct fnode* f = hm_find( hm, word, document_id );

    //printf("Got here! at hm_get\n");
    
    // Return num_occurrences if found or -1 if not found
    return f == NULL ? -1 : f->num_occurrences; 
}

/*
Function hm_put finds or creates an entry in the hashmap for the 
word and file name, and sets the number of occurrences of that word
in that file. It may update the number of files a word is in.
*/
void hm_put(struct hashmap* hm, char* word, char* document_id, int num_occurrences)
{
    // Find matching node 
    struct fnode* f = hm_find( hm, word, document_id );

    // If no matching node, create a new node and put at the head of the bucket
    if ( f == NULL ) 
    {
        // Get index of bucket
        int h = hash( hm, word );
        struct llnode* l = hm->map[h];

        // Search for the word
        while ( l != NULL )
        {
            if ( ! strcmp( word, l->word ) )
            {
                break;
            }

            l = l->next;
        }

        // Allocate new node
        if ( l == NULL )
        {
            // Make new allocated copy of word 
            l = malloc( sizeof( struct llnode ) );
            l->word = strdup(word);
            l->num_files = 0;
            l->fn = NULL;
            l->next = hm->map[h];
            hm->map[h] = l;
        }

        for ( f = l->fn; f != NULL; f = f->next )
        {
            if ( ! strcmp( document_id, f->document_id ) )
            {
                break;
            }
        }

        if ( f == NULL )
        {
            // Make new allocated copy of document_id
            f = malloc( sizeof( struct fnode ) );
            f->document_id = document_id;
            f->next = l->fn;
            l->fn = f;
            l->num_files++;
        }
    }
    // Update the number of occurrences
    f->num_occurrences = num_occurrences;
}

/*
Function hm_destroy frees all the storage of the hashmap.
*/
void hm_destroy(struct hashmap* hm)
{
    // Cannot destroy a null pointer
    if ( hm == NULL )
    {
        return;
    }

    // Walk through all buckets
    for ( int i = 0; i < hm->num_buckets; i++ )
    {
        // Walk the list in each of the buckets
        for ( struct llnode* l = hm->map[i]; l != NULL; )
        {
            // Free the word copy
            free(l->word);
            // Save the next pointer for walking the list
            struct llnode* t = l->next;

            for ( struct fnode* f = l->fn; f != NULL; )
            {
                struct fnode* t = f->next;
                free(f);
                f = t;
            }
            
            // Free this node
            free(l);
            // Advance to the next node
            l = t;
        }
    }

    // Free the bucket list
    free(hm->map);
    // Free the hashmap structure
    free(hm);
}

/*
Function hash creates a pseudo-random number from the 
characters of a word, using prime number 37 to record
the value and position of every character, then reduces
it by modulus of the bucket count.
*/
int hash(struct hashmap* hm, char* word)
{
    // Initial return value
    unsigned long ret = 0;
    
    // Find the not null characters of word
    while (*word)
    {
        // Fold each character into the return
        // 37 is prime
        ret = ret * 37 + *word;
        word++;
    }

    // Return the index for this hash
    return ret % hm->num_buckets;
}

/*
function printHash prints the contents of the hashmap
in bucket order, one line for each word followed by
a line for each file.
*/
int printHash(struct hashmap* hm)
{
    printf("Printing hashmap:\n");
    printf("\n");
    int count = 1;

    // Iterate through the buckets
    for ( int i = 0; i < hm->num_buckets; i++)
    {
        // Iterate through the words
        for ( struct llnode* l = hm->map[i]; l != NULL; l = l->next )
        {
            printf( "%d: number of bucket: %d, word: %s, file count: %d\n", count++, i, l->word, l->num_files );

            // Iterate through the files of the word
            for ( struct fnode* f = l->fn; f != NULL; f = f->next )
            {
                printf( "\tdocument_id: %s, word count: %d\n", f->document_id, f->num_occurrences );
            }
        }
    }

    return 0;
}