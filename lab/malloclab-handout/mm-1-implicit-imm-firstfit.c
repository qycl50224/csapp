/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
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

/* single word (4) or double word (8) alignment */
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

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((HDRP(bp)))) 
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))





static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t size);

static char *headptr = 0;
int mm_check();


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
    // size = ALIGN(words*WSIZE);
    // printf("words:%d  size:%d\n", words, size );
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
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
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    // no fit free block, get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
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

    coalesce(ptr);  // a vital important step
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * method 1: create a new block to substitude the old block, so the cost is
 * expensive, which means its not very fast
 */
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
    if (GET_SIZE(HDRP(ptr)) == GET_SIZE(FTRP(ptr))) {
        // differen block 
        newptr = mm_malloc(asize);
        if (newptr == NULL) 
          return NULL;
        copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
        if (asize < copySize)
          copySize = asize;
        memcpy(newptr, oldptr, copySize);
        place(newptr, asize); // don't forget to fix the header and footer
        mm_free(oldptr);
        return newptr;    
    } else {
        return NULL;
    } 
}

void *mm_realloc(void *ptr, size_t size)
{
    if(ptr == NULL){
        return mm_malloc(size);
    }
    if(size == 0){
        mm_free(ptr);
        return NULL;
    }

    size_t asize;
    if(size <= DSIZE) asize  = 2 * DSIZE;
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/DSIZE);

    size_t oldsize = GET_SIZE(HDRP(ptr));
    if(oldsize == asize) return ptr;
    else if(oldsize > asize){
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize - asize, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(oldsize - asize, 0));
        coalesce(NEXT_BLKP(ptr));
        return ptr;
    }
    else{ // the case that asize > oldsize which means lacking of space, so we 
          // consider coalesce the next one if the next is free block 
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        // here its a simulation of place()
        if(!next_alloc && GET_SIZE(HDRP(NEXT_BLKP(ptr))) + oldsize >= asize) {
            size_t last = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + oldsize - asize;
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            if(last >= DSIZE){ // don't forget to divide if possible
                PUT(HDRP(NEXT_BLKP(ptr)), PACK(last, 0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(last, 0));
            }
            return ptr;
        }
        else{ // concat the next free block is still imcompatible, then allocate
            char *newptr = mm_malloc(asize);
            if(newptr == NULL) return NULL;
            memcpy(newptr, ptr, oldsize - DSIZE);
            mm_free(ptr);
            return newptr;
        }
    }
}
/*
 * mm_check - Heap Consistency Checker 
 */
// int mm_check()
// {
    // void* bp;
    // for (bp = mem_heap_lo()+2*DSIZE; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
    //         return bp;
    //     }
    // }
    // return NULL;   
// }


/*
 * coalesce - use to coalesce adjecant free block
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {  /* case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) {  /* case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {   /* case 3 */
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                              /* case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}


static void *find_fit(size_t asize)
{
    void* bp;
    // headptr -> the first block 
    for (bp = headptr; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t size)
{
    size_t csize = GET_SIZE(HDRP(bp));
    // 2*DSIZE-> Minimum block is 2*DSIZE = 16 byte, so if its larger then can 
    // be devided
    if ((csize - size) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-size, 0));
        PUT(FTRP(bp), PACK(csize-size, 0));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
} 