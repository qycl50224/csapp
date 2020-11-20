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

// #define DEBUG 1 


static void *extend_heap(size_t dwords);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void *best_fit(size_t asize);
static void place(void *bp, size_t asize);
static void deferred_coalesce();
static void put_to_first(void *bp);
static void remove_from_free_list(void *bp);
int mm_check(char *function);
static char *headptr = 0;
static char *root = 0; 



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((headptr = mem_sbrk(6*WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(headptr, 0);
    PUT(headptr+1*WSIZE, 0);
    PUT(headptr+2*WSIZE, 0);
    PUT(headptr+3*WSIZE, PACK(DSIZE, 1));
    PUT(headptr+4*WSIZE, PACK(DSIZE, 1));
    PUT(headptr+5*WSIZE, PACK(0, 1));
    root = headptr + 1*WSIZE;
    headptr += (4*WSIZE);
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
    if ((bp = extend_heap(extendsize/DSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    return bp;
}

static void *first_fit(size_t asize)
{
    void *bp;
    for (bp = GET(root); bp != NULL; bp = GET(NEXT_FREE(bp))) {
        if (GET_SIZE(HDRP(bp)) >= asize) {
            return bp;
        }
    }
    return NULL;

}

static void *best_fit(size_t asize)
{
    void *bp;
    void *target = NULL;
    size_t min = 0;
    size_t size;
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
    // deferred coalesce or immediate coalesce
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
    char *next = GET(root); 
    if (next != NULL) {
        PUT(PREV_FREE(next), bp);
    }
    PUT(NEXT_FREE(bp), next);
    PUT(root, bp);
}

 /*
  * the helper for coalesce
  */
static void remove_from_free_list(void *bp)
{
    char *prev;
    char *next;
    prev = GET(PREV_FREE(bp)); // to get address
    next = GET(NEXT_FREE(bp));
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
}


// void *mm_realloc(void *ptr, size_t size)
// {
//     if(ptr == NULL){
//         return mm_malloc(size);
//     }
//     if(size == 0){
//         mm_free(ptr);
//         return NULL;
//     }

//     size_t asize;
//     if(size <= DSIZE) asize  = 2 * DSIZE;
//     else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/DSIZE);

//     size_t oldsize = GET_SIZE(HDRP(ptr));
//     if(oldsize == asize) return ptr;
//     else if(oldsize > asize){
//         PUT(HDRP(ptr), PACK(asize, 1));
//         PUT(FTRP(ptr), PACK(asize, 1));
//         PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize - asize, 0));
//         PUT(FTRP(NEXT_BLKP(ptr)), PACK(oldsize - asize, 0));
//         coalesce(NEXT_BLKP(ptr));
//         return ptr;
//     }
//     else{ // the case that asize > oldsize which means lacking of space, so we 
//           // consider coalesce the next one if the next is free block 
//         size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
//         // here its a simulation of place()
//         if(!next_alloc && GET_SIZE(HDRP(NEXT_BLKP(ptr))) + oldsize >= asize) {
//             remove_from_free_list(NEXT_BLKP(ptr));
//             size_t last = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + oldsize - asize;
//             PUT(HDRP(ptr), PACK(asize, 1));
//             PUT(FTRP(ptr), PACK(asize, 1));
//             if(last >= 2*DSIZE){ // don't forget to divide if possible
//                 PUT(HDRP(NEXT_BLKP(ptr)), PACK(last, 0));
//                 PUT(FTRP(NEXT_BLKP(ptr)), PACK(last, 0));

//                 put_to_first(NEXT_BLKP(ptr));
//             }
//             return ptr;
//         }
//         else{ // concat the next free block is still imcompatible, then allocate
//             char *newptr = mm_malloc(asize);
//             if(newptr == NULL) return NULL;
//             memcpy(newptr, ptr, oldsize - DSIZE);
//             mm_free(ptr);
//             return newptr;
//         }
//     }
// }
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


int mm_check(char *function)
{
    printf("---cur function:%s empty blocks:\n",function);
    char *tmpP = GET(root);
    int count_empty_block = 0;
    while(tmpP != NULL)
    {
        count_empty_block++;
        printf("address：%x size:%d \n",tmpP,GET_SIZE(HDRP(tmpP)));
        tmpP = GET(NEXT_FREE(tmpP));
    }
    printf("empty_block num: %d\n",count_empty_block);
}
