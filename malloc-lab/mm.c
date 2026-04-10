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
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) /* 메모리 할당 및 Align하기 */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 새로 만든 매크로 */
#define PACK(size, alloc) ((size | alloc)) /* 메모리 사이즈와 점유 여부 - 하위 3비트는 비고, 최하위에 OR연산으로 점유 여부 표시 */
#define GET_SIZE(p) (*(size_t *) (p) & ~0x7) /* 사이즈만 추출 (하위 3비트 소거) - not 0x7과 AND 연산 */
#define GET_ALLOC(p) (*(size_t *)(p) & 0x1) /* 점유 여부만 추출 (최하위 1비트 추출) - 0x1과 AND 연산 */
#define HDRP(ptr) ((char *)(ptr) - 4) /* 헤더 찾기 (힙 탐색) | Header (4byte) | payload | */
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr))) /* ptr에서 다음 블록 payload 포인터 */
#define FTRP(ptr) ((char *) NEXT_BLKP(ptr) - 8) /* 현재 블록 footer 주소 */
#define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE(HDRP(ptr) - 4)) /* 이전 블록 payload 포인터 */
#define PUT(p, val) (*(size_t *)(p) = (val)) /* p 주소에 값을 쓰기 */


static char *heap_listp;  // 전역변수

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 처음에 16 byte 할당하기 */
    heap_listp = mem_sbrk(16);

    /* 메모리 할당이 불가능할 때 */
    if (heap_listp == (void *)-1)
        return -1;

    // heap_listp + 0  → padding
    PUT((char *)(heap_listp),  PACK(0,0)); // padding
    // heap_listp + 4  → prologue header
    PUT((char *)(heap_listp) + 4,  PACK(8, 1));  // prologue header
    // heap_listp + 8  → prologue footer
    PUT((char *)(heap_listp) + 8,  PACK(8, 1));  // prologue footer
    // heap_listp + 12 → epilogue header
    PUT((char *)(heap_listp) + 12, PACK(0, 1)); // 크기 0, allocated

    heap_listp += 8;  // prologue payload 위치로 이동
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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