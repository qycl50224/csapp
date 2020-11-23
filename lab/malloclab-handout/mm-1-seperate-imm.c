#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};


#define ALIGNMENT 8

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* preprocessor macros of pointer arithmetic */
#define MAX(x,y) ((x)>(y)? (x):(y))

/* Pack and write a word at address p */
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))


#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NEXT_FREE(bp) ((char *)bp)
#define PREV_FREE(bp) ((char *)bp + 1*WSIZE)

static void *extend_heap(size_t dwords);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void *best_fit(size_t asize);
static void place(void *bp, size_t asize);
static void deferred_coalesce();
static void put_to_first(void *bp);
static void remove_from_free_list(void *bp);
static char* get_idx_root(size_t asize);
static char *headptr = 0;
static char *listhead = 0;
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((headptr = mem_sbrk(14*WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(headptr, 0);
    PUT(headptr+1*WSIZE, 0);   // [0-16]  0~2 dword
    PUT(headptr+2*WSIZE, 0);   // (16-32]  2~4 dword
    PUT(headptr+3*WSIZE, 0);   // (32-64]   4~8 dword
    PUT(headptr+4*WSIZE, 0);   // (64-128]   ...
    PUT(headptr+5*WSIZE, 0);   // (128-256]
    PUT(headptr+6*WSIZE, 0);   // (256-512]
    PUT(headptr+7*WSIZE, 0);   // (512-1024]
    PUT(headptr+8*WSIZE, 0);   // (1024-2048]
    PUT(headptr+9*WSIZE, 0);   // (2048-4096]
    PUT(headptr+10*WSIZE, 0);  // >=4096
    PUT(headptr+11*WSIZE, PACK(DSIZE, 1));
    PUT(headptr+12*WSIZE, PACK(DSIZE, 1));
    PUT(headptr+13*WSIZE, PACK(0, 1));

    listhead = headptr;
    headptr += (12*WSIZE);
    if (extend_heap(CHUNKSIZE/DSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/*
 * extend_heap - use to extend the capacity of the heap
 */
static void *extend_heap(size_t dwords)
{
    char *bp;
    size_t size;
    size = (dwords % 2)? (dwords+1)*DSIZE:dwords*DSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0));
    PUT(NEXT_FREE(bp), 0); // initialize
    PUT(PREV_FREE(bp), 0);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // the end block
    return coalesce(bp);
}

void *mm_malloc(size_t size)
{
    size_t asize;       /* Adjusted block size */
    size_t extendsize;  /* Amount to extend heap if no fit */
    void *bp;

    if (size == 0)
        return NULL;
    // adjust block size to include overhead and alignment reqs
    asize = (size <= DSIZE)?(2*DSIZE):(DSIZE*((size +(DSIZE)+(DSIZE-1))/DSIZE));
    /* Search the free list fot a fit */
    // printf("1\n");
    if ((bp = first_fit(asize)) != NULL) { // got 84 for first_fit and 65 for best_fit
        // printf("==========\n");
        place(bp, asize);
        // printf("----------\n");
        return bp;
    } 
    
    // no fit free block, get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/DSIZE)) == NULL)
        return NULL;
    // printf("||||||||||||||||||||\n");
    place(bp, asize);
    // printf("heap size %d\n", mem_heapsize());

    return bp;
}

static void *first_fit(size_t asize)
{
    void *bp;
    char *root;
    root = get_idx_root(asize);
    while (root != (headptr-WSIZE)) {
        // GET(root)->the first free, NEXT_FREE(bp) is an address 
        for (bp = GET(root); bp != NULL; bp = GET(NEXT_FREE(bp))) {
            if (GET_SIZE(HDRP(bp)) >= asize) {
                // printf("1\n");
                // printf("find the free block in %d\n", i);
                return bp;
            }
        }    
        root += WSIZE;
    }
    
    return NULL;

}

static void *best_fit(size_t asize)
{
    void *bp;
    void *target = NULL;
    char *root;
    size_t min = 0;
    size_t size;
    int i;
    root = get_idx_root(asize);
    for (bp = GET(root); bp != NULL; bp = GET(NEXT_FREE(bp))) {
        size = GET_SIZE(HDRP(bp));
        if (size >= asize) {
            if ((min == 0) || (size < min)) {
                target = bp;
                min = size;
            }
        }
    }
    return target;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    // 2*DSIZE-> Minimum block is 2*DSIZE = 16 byte, so if its larger then can 
    // be devided
    if ((csize - asize) >= (2*DSIZE)) {
        remove_from_free_list(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));    
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        PUT(NEXT_FREE(bp), 0);
        PUT(PREV_FREE(bp), 0);
        coalesce(bp);
    } else {
        remove_from_free_list(bp);
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
} 

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == 0) return; 
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    PUT(NEXT_FREE(ptr), 0);
    PUT(PREV_FREE(ptr), 0);
    // immediate coalesce
    coalesce(ptr);  // a vital important step

}

/*
 * coalesce - use to coalesce adjecant free block
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {  /* case 1 */
    }
    else if (prev_alloc && !next_alloc) {  /* case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_from_free_list(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {   /* case 3 */
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        remove_from_free_list(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {                              /* case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_from_free_list(PREV_BLKP(bp));
        remove_from_free_list(NEXT_BLKP(bp));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    put_to_first(bp);
    return bp;
}

 /*
  * the helper for coalesce
  */
static void put_to_first(void *bp)
{
    size_t asize = GET_SIZE(HDRP(bp));
    char *root;
    char *next;
    root = get_idx_root(asize);
    next = GET(root); // indirect access
    if (next != NULL) {
        PUT(PREV_FREE(next), bp);
    }
    PUT(NEXT_FREE(bp), next);
    PUT(PREV_FREE(bp), 0);
    PUT(root, bp);
}

 /*
  * the helper for coalesce
  */
static void remove_from_free_list(void *bp)
{
    char *prev;
    char *next;
    char *root;
    size_t asize = GET_SIZE(HDRP(bp));
    prev = GET(PREV_FREE(bp)); // to get address
    next = GET(NEXT_FREE(bp));
    root = get_idx_root(asize);       
    if (prev == NULL) {
        if (next != NULL) {
            PUT(PREV_FREE(next), 0);
        }
        PUT(root, next);
    } else {
        if (next != NULL) {
            PUT(PREV_FREE(next), prev);
        }
        PUT(NEXT_FREE(prev), next);
    }
    PUT(NEXT_FREE(bp), 0);
    PUT(PREV_FREE(bp), 0);
    // printf("removed from %d\n", i);
}

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    size_t asize;
    if (ptr == NULL) {
        newptr = mm_malloc(size);
        return newptr;
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }
    // cuz we need to align every block so the size must be aligned
    asize = (size <= DSIZE)?(DSIZE*2):(DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE));
    // check if the block is allocated by mm_malloc or mm_realloc
    // differen block 
    newptr = mm_malloc(asize);
    if (newptr == NULL) 
      return NULL;
    copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (asize < copySize)
      copySize = asize;
    memcpy(newptr, oldptr, copySize);
    // place(newptr, asize); // don't forget to fix the header and footer
    mm_free(oldptr);
    return newptr;    
}

static char* get_idx_root(size_t asize)
{
    int i;
    if (asize <= 2*DSIZE) {
        i = 1;
    } else if (asize <= 4*DSIZE) {
        i = 2;
    } else if (asize <= 8*DSIZE) {
        i = 3;
    } else if (asize <= 16*DSIZE) {
        i = 4;
    } else if (asize <= 32*DSIZE) {
        i = 5;
    } else if (asize <= 64*DSIZE) {
        i = 6;
    } else if (asize <= 128*DSIZE) {
        i = 7;
    } else if (asize <= 256*DSIZE) {
        i = 8;
    } else if (asize <= 512*DSIZE) {
        i = 9;    
    } else {
        i = 10;
    }   
    return listhead+(i*WSIZE);
}