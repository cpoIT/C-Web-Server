#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "cache.h"

/**
 * Allocate a cache entry
 */
struct cache_entry *alloc_entry(char *path, char *content_type, void *content, int content_length)
{
    struct cache_entry * ce = malloc(sizeof *ce); // = sizeof (struct cache_entry)

    // Deep Copy
    // Fleeting Pointer: use malloc & strcpy (not just: ce->path, path)
    ce->path = malloc(strlen(path)+1); // + for '\0'
    strcpy(ce->path, path);

    ce->content_type = malloc(strlen(content_type)+1);
    strcpy(ce->content_type, content_type);

    // content is type void, but we have content_length to compensate
    ce->content = malloc(content_length);
    memcpy(ce->content, content, content_length);

    ce->content_length = content_length;

    ce->prev = ce->next = NULL;
    
    return ce;
}

/**
 * Deallocate a cache entry
 */
void free_entry(struct cache_entry *entry)
{
    free(entry->path);
    free(entry->content_type);
    free(entry->content);
    free(entry);
}

/**
 * Insert a cache entry at the head of the linked list
 */
void dllist_insert_head(struct cache *cache, struct cache_entry *ce)
{
    // Insert at the head of the list
    if (cache->head == NULL) {
        cache->head = cache->tail = ce;
        ce->prev = ce->next = NULL;
    } else {
        cache->head->prev = ce;
        ce->next = cache->head;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Move a cache entry to the head of the list
 */
void dllist_move_to_head(struct cache *cache, struct cache_entry *ce)
{
    if (ce != cache->head) {
        if (ce == cache->tail) {
            // We're the tail
            cache->tail = ce->prev;
            cache->tail->next = NULL;

        } else {
            // We're neither the head nor the tail
            ce->prev->next = ce->next;
            ce->next->prev = ce->prev;
        }

        ce->next = cache->head;
        cache->head->prev = ce;
        ce->prev = NULL;
        cache->head = ce;
    }
}


/**
 * Removes the tail from the list and returns it
 * 
 * NOTE: does not deallocate the tail
 */
struct cache_entry *dllist_remove_tail(struct cache *cache)
{
    struct cache_entry *oldtail = cache->tail;

    cache->tail = oldtail->prev;
    cache->tail->next = NULL;

    cache->cur_size--;

    return oldtail;
}

/**
 * Create a new cache
 * 
 * max_size: maximum number of entries in the cache
 * hashsize: hashtable size (0 for default)
 */
struct cache *cache_create(int max_size, int hashsize)
{

    struct cache *cache = malloc(sizeof *cache);

    cache->index = hashtable_create(hashsize, NULL); // NULL for default hash function
    cache->head = cache->tail = NULL; //Doubly-Linked List
    cache->max_size = max_size;    // Max entries
    cache->cur_size = 0;    // Curr No. of entries
    return cache;
}

void cache_free(struct cache *cache)
{
    struct cache_entry *cur_entry = cache->head;

    hashtable_destroy(cache->index);

    while (cur_entry != NULL) {
        struct cache_entry *next_entry = cur_entry->next;

        free_entry(cur_entry);

        cur_entry = next_entry;
    }

    free(cache);
}

/**
 * Store an entry in the cache
 *
 * This will also remove the least-recently-used items as necessary.
 * 
 * NOTE: doesn't check for duplicate cache entries
 */
void cache_put(struct cache *cache, char *path, char *content_type, void *content, int content_length)
{

// Allocate a new cache entry with the passed parameters.
// Insert the entry at the head of the doubly-linked list.
// Store the entry in the hashtable as well, indexed by the entry's `path`.
// Increment the current size of the cache.
// If the cache size is greater than the max size:
//    - Remove the cache entry at the tail of the linked list.
//    - Remove that same entry from the hashtable, using the entry's `path` and the `hashtable_delete` function.
//    - Free the cache entry.
//    - Ensure the size counter for the number of entries in the cache is correct. (included in dllist_remove_tail(cache);)

struct cache_entry *new_cache_entry = alloc_entry(path, content_type, content, content_length);

dllist_insert_head(cache, new_cache_entry);
hashtable_put(cache->index, path, new_cache_entry);
cache->cur_size++;
if(cache->cur_size > cache->max_size) {
    struct cache_entry *oldtail_entry = dllist_remove_tail(cache);
    hashtable_delete(cache->index, oldtail_entry->path); // ht, key
    free_entry(oldtail_entry);
    // cache->cur_size--;
    }
}

/**
 * Retrieve an entry from the cache
 */
struct cache_entry *cache_get(struct cache *cache, char *path)
{

// Goal: Attempt to find the cache entry pointer by `path` in the hash table.
// hashtable_get(struct hashtable *ht, char *key)   
// If not found, return `NULL`.
    if (hashtable_get(cache->index, path) == NULL) {
        return NULL;
    } else {
// Move the cache entry to the head of the doubly-linked list.
// Return the cache entry pointer.
    dllist_move_to_head(cache, hashtable_get(cache->index, path));
    return cache->head;
   }

}
