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
    "Team1",
    /* First member's full name */
    "Ji Yong Park",
    /* First member's email address */
    "jypark@krafton.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) /* 메모리 할당 및 Align하기 */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 새로 만든 매크로 */
#define CHUNKSIZE (1<<12)  // 4096 bytes

#define PACK(size, alloc) ((size | alloc)) /* 메모리 사이즈와 점유 여부 - 하위 3비트는 비고, 최하위에 OR연산으로 점유 여부 표시 */
#define GET_SIZE(p) (*(unsigned int *) (p) & ~0x7U) /* 사이즈만 추출 (하위 3비트 소거) - not 0x7과 AND 연산 */
#define GET_ALLOC(p) (*(unsigned int *)(p) & 0x1U) /* 점유 여부만 추출 (최하위 1비트 추출) - 0x1과 AND 연산 */
#define HDRP(ptr) ((char *)(ptr) - 4) /* 헤더 찾기 (힙 탐색) | Header (4byte) | payload | */
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr))) /* ptr에서 다음 블록 payload 포인터 */
#define FTRP(ptr) ((char *) NEXT_BLKP(ptr) - 8) /* 현재 블록 footer 주소 */
#define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE(HDRP(ptr) - 4)) /* 이전 블록 payload 포인터 */
#define PUT(p, val) (*(unsigned int *)(p) = (val)) /* p 주소에 값을 쓰기 */


static char *heap_listp;  // 전역변수

/* forward declaration */
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
static void *extend_heap(size_t size);
static void *coalesce(void *ptr);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 처음에 16 byte 할당하기 */
    heap_listp = mem_sbrk(16);

    /* 메모리 할당이 불가능할 때 */
    // if (heap_listp == (void *)-1)
    //     return -1;

    // heap_listp + 0  → padding
    PUT((char *)(heap_listp),  PACK(0,0)); // padding
    // heap_listp + 4  → prologue header
    PUT((char *)(heap_listp) + 4,  PACK(8, 1));  // prologue header
    // heap_listp + 8  → prologue footer
    PUT((char *)(heap_listp) + 8,  PACK(8, 1));  // prologue footer
    // heap_listp + 12 → epilogue header
    PUT((char *)(heap_listp) + 12, PACK(0, 1)); // 크기 0, allocated

    heap_listp += 8;  // prologue payload 위치로 이동

    if (extend_heap(CHUNKSIZE) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    /*
    1. size가 0이면 → return NULL
    2. 빈 블록 찾기 (first fit 탐색)
        → 찾으면 → 그 블록에 배치
        → 못 찾으면 → 힙 늘려서 새 블록 만들기
    3. 배치된 블록 포인터 반환
    */
    if (size == 0){
        return NULL;
    }

    size_t asize = ALIGN(size + 8);  // 실제 블록 크기
    void *bp = find_fit(asize);

    if(bp == NULL){
        bp = extend_heap(asize);
    }

    if(bp == NULL){
        return NULL;
    }

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
    PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
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

// 1. 빈 블록 탐색
static void *find_fit(size_t size) 
{ 
    for (char *bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    // 빈 블록 찾기
     if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= size){
        return bp;
     }   
    }

    return NULL;

}

// 2. 블록 배치 (header/footer 업데이트)
static void place(void *bp, size_t size) 
{   
    // split 함수 구현
    size_t old_size = GET_SIZE(HDRP(bp));

    if (GET_SIZE(HDRP(bp)) - size >= 16){
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));
    } else {
        PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
        PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    }

}

// 3. 힙 확장
static void *extend_heap(size_t size)
{   
    // size_t new_size = ALIGN(size) + 8;
    size_t newsize = (size < CHUNKSIZE) ? CHUNKSIZE : size;
    char *raw = (char *)mem_sbrk(newsize);
    // char *raw = (char *)mem_sbrk(size);

    if (raw == (void *)-1)
        return NULL;

    char *bp = raw;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT((char *)(FTRP(bp)) + 4, PACK(0, 1)); // 크기 0, allocated

    return bp;

}

// 4. free - 4가지 경우 - coalesce 하기
static void *coalesce(void *ptr)
{

    // 1. 양쪽이 다 점유되어있는 경우 
    if(GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && GET_ALLOC(HDRP(PREV_BLKP(ptr)))){
        return ptr;
    }

    size_t curr_size = GET_SIZE(HDRP(ptr));
    size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(ptr)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));


    // 2. 한쪽만 점유되어있는 경우 (왼쪽, 이전)
    if(GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && !GET_ALLOC(HDRP(PREV_BLKP(ptr)))){
        size_t merge_size = curr_size + prev_size;
        PUT(HDRP(PREV_BLKP(ptr)), PACK(merge_size, 0));
        PUT(FTRP(ptr), PACK(merge_size, 0));
        return PREV_BLKP(ptr);
    }
    // 3. 한쪽만 점유되어있는 경우 (오른쪽, 다음)
    if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && GET_ALLOC(HDRP(PREV_BLKP(ptr)))){
        size_t merge_size = curr_size + next_size;
        PUT(HDRP(ptr), PACK(merge_size, 0));
        PUT(FTRP(ptr), PACK(merge_size, 0));
        return ptr;
    }

    // 4. 양쪽 다 free인 경우.
    size_t merge_size = curr_size + prev_size + next_size;
    PUT(HDRP(PREV_BLKP(ptr)), PACK(merge_size, 0));
    PUT(FTRP(PREV_BLKP(ptr)), PACK(merge_size, 0));
    return PREV_BLKP(ptr);
}