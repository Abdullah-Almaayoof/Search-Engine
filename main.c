#include "hashmap.h"
#include <glob.h>
#include <ctype.h>
#include <math.h>

// Prototypes
void stop_word( struct hashmap* hm, int n);
int training( struct hashmap* hm );
char** read_query( int *len );
void free_query( char** q, int len);
double tf_idf( int tf, int n, int df );
struct ranked_files rank( struct hashmap* hm, char** list, int len, int n );

/*
Function stop_word finds words in the hashmap that are in every file,
and removes them.
*/
void stop_word( struct hashmap* hm, int n)
{
    // Go through all buckets
    for ( int i = 0; i < hm->num_buckets; i++ )         
    {
        // Save a pointer to a pointer to the rest of the list
        struct llnode** hanger = &( hm->map[i] );      

        // Go through all words in the bucket
        for ( struct llnode* node = hm->map[i]; node != NULL; )     
        {
            // Found a stop word
            if ( node->num_files == n )
            {
                free( node->word );

                // Free all files for this word
                for ( struct fnode* fn = node->fn; fn != NULL; )
                {
                    struct fnode* temp = fn->next;
                    free( fn );
                    fn = temp;
                }

                // Save the next value for use after free
                struct llnode* temp = node->next;
                free( node );
                // Snap this word out of the list, advance to the next word
                *hanger = node = temp;
                continue;
            }
            
            // Advance to the next word
            hanger = &( node->next );
            node = node->next;
        }  
    }
}

// Repository of file names
static glob_t gt;

/*
Function training will find every word in every file in folder p5docs 
that matches *.txt, counting the files that sucessfully open and recording
in the hashmap the words, number of files containing the word, names
of files containing the word, and how many times each file contains
the word. Finally, it calls stop_word to remove words in all files
from the hashmap.
*/
int training( struct hashmap* hm )
{
    char word[32];
    FILE * fp;
    int n = 0;
    

    // Find all matching file names
    if ( glob( "p5docs/*.txt", 0, NULL, &gt ) )
    {
        printf("glob failure\n");
        exit(1);
    }

    // For every file name, load all words
    for ( size_t i = 0; i < gt.gl_pathc; i++ )
    {
        if ( NULL == ( fp = fopen( gt.gl_pathv[i], "r" ) ) )
        {
            printf( "Cannot open '%s'\n", gt.gl_pathv[i] );
        }
        else 
        {
            // Read each word in the file
            while ( 1 == fscanf( fp, "%s", word ) )
            {
                // Determine if word and file are in hashmap
                int ct = hm_get( hm, word, gt.gl_pathv[i] + 7);
                // Update/insert file and count for this word
                hm_put( hm, word, gt.gl_pathv[i] + 7, ct > 0 ? ct + 1 : 1 );
            }

            // Close file
            fclose( fp );
            // Only count successful opens
            n++;
        }
    }

    // Remove stop words
    stop_word( hm, n );
    return n;
}

/*
Function read_query will read one line of input of any length into 
an expanding buffer. Then, it will parse the words in the line,
copying them onto an expanding list. It returns the list and 
writes the length by reference.
*/
char** read_query( int *len )
{
    char** list = NULL;
    *len = 0;
    char* line = NULL, *cp, *word = NULL;
    size_t llen = 0, blen = 0;

    // Read a line into an expandable buffer
    do 
    {
        // Expand buffer length
        blen += 256;

        if ( NULL == ( line = realloc( line, blen ) ) )
        {
            perror("realloc query");
            exit(1);
        }

        // Read additional line data to end of old data
        if ( NULL == fgets( line + llen, blen - llen, stdin ) )
        {
            perror("fgets query");
            exit(1);
        }

        // Update current line length
        llen = strlen( line );
      // Stop when buffer ends in line feed
    } while ( line[llen - 1] != '\n' );

    // Parse line extracting words one character at a time
    for ( cp = line; *cp; cp++ )
    {
        // White space is skipped unless it ends a word
        if ( isspace( *cp ) )
        {
            // No word is started, so skip white space
            if ( word == NULL )
            {
                continue;
            }

            // Null terminate the word
            *cp = '\0';
            
            // Expand the word list
            if ( NULL == ( list = realloc( list, ++*len * sizeof(char*) ) ) )
            {
                perror("realloc query list");
                exit(1);
            }

            // Copy the word into the list
            if ( NULL == ( list[*len - 1] = strdup( word ) ) )
            {
                perror("strdup query word");
                exit(1);
            }

            // Start looking for a new word
            word = NULL;
            continue;
        }

        // Found beginning of new word
        if ( word == NULL )
        {
            word = cp;
        }
    }

    // Free the expanded buffer
    free( line );
    return list;
}

/*
Free query words and the array that holds them.
*/
void free_query( char** q, int len)
{
    // Done with list, free it 
    for ( int i = 0; i < len; i++ )
    {
        free( q[i] );
    }

        free( q );
}

/*
Calculate the tf_idf.
*/
double tf_idf( int tf, int n, int df )
{
    //printf("tf_idf: %d %d %d %lf\n", tf, n, df, tf * log( (double)n / df) );
    return tf * log( (double)n / df);
}

// Structure to hold ranked list of files
struct ranked_files
{
    int len;
    char** names;
    double* scores;
};

/* 
Function rank will look up every word in the list, and for every file,
create or update a ranking score. Then sort the list by decreasing score.
Output is a structure of parallel arrays of names and scores, and the 
array length.
*/
struct ranked_files rank( struct hashmap* hm, char** list, int len, int n )
{
    struct ranked_files rf;
    rf.len = gt.gl_pathc;
    rf.names = malloc( rf.len * sizeof( char* ) );
    rf.scores = calloc( rf.len, sizeof( double ) );

    for ( int i = 0; i < rf.len; i++ )
    {
        rf.names[i] = gt.gl_pathv[i] + 7;
    }

    // Examine every word in the query
    for ( int i = 0; i < len; i++ )
    {
        // Find the word in the hashmap
        struct llnode* l = hm_find_word( hm, list[i] );

        // Word is not in the hashmap, skip
        if ( l == NULL)
        {
            //printf("rank miss %s\n", list[i]);
            continue;
        }
        
        // Score every file for this word
        for ( struct fnode* f = l->fn; f != NULL; f = f->next )
        {
            // Search for existing score for this file
            for ( int j = 0; j < rf.len; j++ )
            {
                // Found existing score
                if ( ! strcmp( rf.names[j], f->document_id ) )
                {
                    // Update existing score
                    rf.scores[j] += tf_idf( f->num_occurrences, n, l->num_files );
                    break;
                }
            }

            //printf("rank %s %lf\n", f->document_id, rf.scores[rf.len - 1]);
        }   // End for each file
    }   // End for each word

    // Sort the list descending score
    for ( int i = 1; i < rf.len; i++ )
    {
        for ( int j = 0; j < i; j++ )
        {
            if ( rf.scores[j] < rf.scores[i] )
            {
                // Swap scores
                double d = rf.scores[j];
                rf.scores[j] = rf.scores[i];
                rf.scores[i] = d;
                // Swap names
                char* cp = rf.names[j];
                rf.names[j] = rf.names[i];
                rf.names[i] = cp;
            }
        }
    }

    return rf;
}

int main(void)
{
    FILE * fp;
    int htSize;

    printf("How many buckets?:\n");

    // Check if number of buckets is valid
    if ( 1 != scanf("%d", &htSize) || htSize < 1 )
    {
        printf("Invalid Input\n");
        return 1;
    }

    while ( getchar() != '\n' )
    {
        // Remove rest of bucket line from stdin
    }

    // Create a hashmap
    struct hashmap* hm = hm_create( htSize );
    int n = training( hm );
    //printf( "Loaded %d files\n", n);

    // Abort if no files loaded
    if ( ! n )
    {
        hm_destroy( hm );
        return 0;
    }

    // Open score file for use in all queries
    if ( NULL == (fp = fopen("search_scores.txt", "w") ) )
    {
        perror("fopen search_scores.txt");
        return 1;
    }

    // Loop through multiple queries
    while ( 1 )
    {
        printf("Enter Search String or X to Exit\n");
        // get the list of query words
        int len;
        char** list = read_query( &len );  
        
        // Detect request to exit
        if ( len == 1 && !strcmp( list[0], "X") )
        {
            // Clean up memory and files
            free_query( list, len );
            hm_destroy( hm );
            // Free the glob data
            globfree( &gt );
            fclose( fp );
            return 0;
        }
        
        // Run the query
        struct ranked_files rf = rank( hm, list, len, n );
        free_query( list, len );

        // Report the results to screen and file
        for ( int i = 0; i < rf.len; i++ )
        {
            if ( rf.scores[i] != 0.0 )
            {
                printf( "%s\n", rf.names[i] );
            }

            fprintf( fp, "%s   %lf\n", rf.names[i], rf.scores[i] );
        }

        // Ensure results on disk
        fflush( fp );
        // Release the results
        free( rf.names );
        free( rf.scores );
    }
}