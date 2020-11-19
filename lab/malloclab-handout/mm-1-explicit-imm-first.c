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

#define GET_NEXT_FREE(bp) (*(unsigned int*)(p))
#define GET_PREV_FREE(bp) (*(unsigned int*)(((char *)p) + 1))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void *best_fit(size_t asize);
static void place(void *bp, size_t size);
static void deferred_coalesce();
static int is_free_head(void *bp);
static int is_free_tail(void *bp)
static char *headptr = 0;
static char *freehead = 0; 
static char *freetail = 0; 


struct explicit_list {
    unsigned int hdr;
    struct explicit_list* pred;
    struct explicit_list* succ;
    unsigned int ftr;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((headptr = mem_sbrk(4*WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(headptr, 0);
    PUT(headptr+1*WSIZE, PACK(DSIZE, 1));
    PUT(headptr+2*WSIZE, PACK(DSIZE, 1));
    PUT(headptr+3*WSIZE, PACK(0, 1));
    headptr += (2*WSIZE);
    freehead = headptr;
    freetail = headptr;
    PUT(freehead, headptr);    // initially, there is only one free block,  
    PUT(freehead+1*WSIZE, headptr);         // so the next and prev is it self
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }

    return 0;
}

/*
 * extend_heap - use to extend the capacity of the heap
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words % 2)? (words+1)*WSIZE:words*WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0));
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
    if ((bp = first_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    } 
    // else {
    //     deferred_coalesce();    
    //     if ((bp = first_fit(asize)) != NULL) {
    //         place(bp, asize);
    //         return bp;
    //     }
    // }
    
    // no fit free block, get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

static void *first_fit(size_t asize)
{
    void *bp;
    for (bp = freehead; GET_SIZE(HDRP(bp)) > 0; bp = GET_NEXT_FREE(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
            return bp;
        }
    }
}

static void *best_fit(size_t asize)
{
    void *bp;
    void *target = NULL;
    size_t min = 0;
    size_t size;
    for (bp = freehead; GET_SIZE(HDRP(bp)) > 0; bp = GET_NEXT_FREE(bp)) {
        size = GET_SIZE(HDRP(bp));
        if (!GET_ALLOC(HDRP(bp)) && size >= asize) {
            if ((min == 0) || (size < min)) {
                target = bp;
                min = size;
            }
        }
    }
    return target;
}

static void place(void *bp, size_t size)
{
    size_t csize = GET_SIZE(HDRP(bp));
    // 2*DSIZE-> Minimum block is 2*DSIZE = 16 byte, so if its larger then can 
    // be devided
    char *prev;
    char *next;
    if ((csize - size) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));    
        if (is_free_head(bp) && is_free_tail(bp)) {
            freehead = NEXT_BLKP(bp);
            freetail = NEXT_BLKP(bp);
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(csize-size, 0));
            PUT(FTRP(bp), PACK(csize-size, 0));
        } 
        else if (is_free_head(bp)) {
            freehead = NEXT_BLKP(bp);
            next = GET_NEXT_FREE(bp);
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(csize-size, 0));
            PUT(FTRP(bp), PACK(csize-size, 0));
            connect(bp, next);
        } 
        else if (is_free_tail(bp)) {
            freetail = NEXT_BLKP(bp);
            prev = GET_PREV_FREE(bp);
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(csize-size, 0));
            PUT(FTRP(bp), PACK(csize-size, 0));
            connect(prev, bp);
        }
        else {
            prev = GET_PREV_FREE(bp);
            next = GET_NEXT_FREE(bp);
            bp = NEXT_BLKP(bp);     // this is the remained free block
            PUT(HDRP(bp), PACK(csize-size, 0));
            PUT(FTRP(bp), PACK(csize-size, 0));    
            connect(prev, bp);    
            connect(bp, next);
        }
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        if (is_free_head(bp) && is_free_tail(bp)) {
            freehead = NULL;
            freetail = NULL;
        } 
        else if (is_free_head(bp)) {
            bp = GET_NEXT_FREE(bp);   // is this make sense?
            freehead = bp;
            PUT((char *)bp+1*WSIZE, bp);
        } 
        else if (is_free_tail(bp)) {
            bp = GET_PREV_FREE(bp);
            freetail = bp;
            PUT((char *)bp, bp);
        }
        else {
            prev = GET_PREV_FREE(bp);
            next = GET_NEXT_FREE(bp);  
            connect(prev, next);    
        }
    }
} 

static void connect(void *prev, void *next)
{
    PUT((char *)prev+1*WSIZE, next);
    PUT((char *)next, prev);
}


static int is_free_head(void *bp)
{
    return (bp == freehead)? 1:0;
}

static int is_free_tail(void *bp)
{
    return (bp == freetail)? 1:0;
}












