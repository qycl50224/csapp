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

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))





static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t size);
int mm_check();


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *headptr;
    if ((headptr = mem_sbrk(4*WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(headptr, 0);
    PUT(headptr+WSIZE, PACK(DSIZE, 1));
    PUT(headptr+2*WSIZE, PACK(DSIZE, 1));
    PUT(headptr+3*WSIZE, PACK(0, 1));
    headptr += (2*WSIZE);
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    printf("======================\n");

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;       /* Adjusted block size */
    size_t extendsize;  /* Amount to extend heap if no fit */
    char *bp;

    printf("aaaaaaaaaaaaaaaaaaaaaaa\n");
    if (size == 0)
        return NULL;
    // adjust block size to include overhead and alignment reqs
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else 
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);


    printf("ccccccccccccc\n");
    /* Search the free list fot a fit */
    if ((bp = find_fit(asize)) != NULL) {
        printf("ddddddddddd\n");
        place(bp, asize);
        printf("eeeeeee\n");
        return bp;
    }
    printf("bbbbbbbbbbbbbbbbbbbbbbbb\n");
    // no fit free block, get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    printf("aaaaaaaaaaaaaaaaaaaaaaa\n");
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(size), PACK(size, 0));
    PUT(FTRP(size), PACK(size, 0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/*
 * mm_check - Heap Consistency Checker 
 */
int mm_check()
{
    
}


/*
 * extend_heap - use to extend the capacity of the heap
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // size = (words % 2)? (words+1)*WSIZE:words*WSIZE;
    size = ALIGN(words*WSIZE);
    printf("words:%d  size:%d\n", words, size );
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
 
    return coalesce(bp);
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
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}


static void *find_fit(size_t asize)
{
    void* bp;
    
    for (bp = mem_heap_lo()+2*DSIZE; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }

    return NULL;
}

static void place(void *bp, size_t size)
{
    size_t csize = GET_SIZE(HDRP(bp));

    printf("1111\n");
    if ((csize - size) >= 2*DSIZE) {
        printf("hdrp:%p  ftrp:%p\n", HDRP(bp), FTRP(bp));
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        // printf("22222\n");
        printf("HDRP:%p bp:%p getsize:%d csize:%d  size:%d\n", HDRP(bp), bp, GET_SIZE(HDRP(bp)), csize, size);
        // printf("hdrp:%p  ftrp:%p\n", HDRP(bp), FTRP(bp));
        bp = NEXT_BLKP(bp);
        // printf("3333333\n");
        PUT(HDRP(bp), PACK(csize-size, 0));
        // printf("5555\n");
        printf("HDRP:%p bp:%p getsize:%d csize:%d  size:%d\n", HDRP(bp), bp, GET_SIZE(HDRP(bp)), csize, size);
        // printf("hdrp:%p  ftrp:%p\n", HDRP(bp), FTRP(bp));
        PUT(FTRP(bp), PACK(csize-size, 0));
        // printf("666\n");
    } else {
        // printf("444444444\n");
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
} 








