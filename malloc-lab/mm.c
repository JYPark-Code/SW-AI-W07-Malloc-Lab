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
#include <stdint.h>
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
#define CHUNKSIZE (1 << 12) // 4096 bytes
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size | alloc))                       /* 메모리 사이즈와 점유 여부 - 하위 3비트는 비고, 최하위에 OR연산으로 점유 여부 표시 */
#define GET_SIZE(p) (*(unsigned int *)(p) & ~0x7U)               /* 사이즈만 추출 (하위 3비트 소거) - not 0x7과 AND 연산 */
#define GET_ALLOC(p) (*(unsigned int *)(p) & 0x1U)               /* 점유 여부만 추출 (최하위 1비트 추출) - 0x1과 AND 연산 */
#define HDRP(ptr) ((char *)(ptr) - 4)                            /* 헤더 찾기 (힙 탐색) | Header (4byte) | payload | */
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)))     /* ptr에서 다음 블록 payload 포인터 */
#define FTRP(ptr) ((char *)NEXT_BLKP(ptr) - 8)                   /* 현재 블록 footer 주소 */
// #define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE(HDRP(ptr) - 4)) /* 이전 블록 payload 포인터 -> footer 제거 후 함수로 변환 */
#define PUT(p, val) (*(unsigned int *)(p) = (val))               /* p 주소에 값을 쓰기 */

/* explicit 위한 매크로 */
#define PREV_FREE(p) (*(char **)(p))                                /* free의 prev* 값 : p에서 char* 포인터 읽기 **/
#define NEXT_FREE(p) (*(char **)((char *)(p) + 8))                  /* free의 next* 값 : p+8에서 char* 포인터 읽기 */
#define SET_PREV_FREE(p, val) (*(char **)(p) = (val))               /* free의 prev값 SET */
#define SET_NEXT_FREE(p, val) (*(char **)((char *)(p) + 8) = (val)) /* free의 next값 SET */

/* footer 제거 이후, 적용할 매크로*/
#define GET_PREV_ALLOC(p) (*(unsigned int *)(p) & 0X2U) /* 이전 블록이 점유됬는지? */
#define SET_PREV_ALLOC(p) (*(unsigned int *)(p) |= 0X2U)  /* 이전 블록이 점유 됬다고 SET(값 저장) */
#define CLEAR_PREV_ALLOC(p) (*(unsigned int *)(p) &= ~0X2U) /* 이전 블록을 초기화함 CLEAR(값 저장) */

/* 방법 선택 - 하나만 주석 해제 */
// #define EXPLICIT
#define SEGLIST
/* 없으면 implicit (현재) */

/* fit 전략 선택 */
#define BEST_FIT
/* 없으면 first fit (현재) */

/* 전역변수 */
static char *heap_listp;
#ifdef EXPLICIT
static char *free_listp; // explicit free list 시작점
#endif
#ifdef SEGLIST
static char *seg_list_0; // 1  ~ 31
static char *seg_list_1; // 32 ~ 63
static char *seg_list_2; // 64   ~ 127
static char *seg_list_3; // 128  ~ 255
static char *seg_list_4; // 256  ~ 511
static char *seg_list_5; // 512  ~ 1023
static char *seg_list_6; // 1024 ~ 2047
static char *seg_list_7; // 2048 ~ 4095
static char *seg_list_8; // 4096 ~
#endif

/* forward declaration */
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
static void *extend_heap(size_t size);
static void *coalesce(void *ptr);
static void *_coalesce_blocks(void *ptr);
static void insert_free(void *bp, char **bucket);
static void remove_free(void *bp, char **bucket);
static int _get_bucket_index(size_t size);
static char **_get_bucket(int index);
static void *_prev_blkp(void *ptr);

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
    PUT((char *)(heap_listp), PACK(0, 0)); // padding
    // heap_listp + 4  → prologue header
    PUT((char *)(heap_listp) + 4, PACK(8, 1)); // prologue header
    // heap_listp + 8  → prologue footer
    PUT((char *)(heap_listp) + 8, PACK(8, 1)); // prologue footer
    // heap_listp + 12 → epilogue header
    PUT((char *)(heap_listp) + 12, PACK(0, 1)); // 크기 0, allocated

    heap_listp += 8; // prologue payload 위치로 이동

#ifdef SEGLIST
    seg_list_0 = NULL;
    seg_list_1 = NULL;
    seg_list_2 = NULL;
    seg_list_3 = NULL;
    seg_list_4 = NULL;
    seg_list_5 = NULL;
    seg_list_6 = NULL;
    seg_list_7 = NULL;
    seg_list_8 = NULL;
#endif

#ifdef EXPLICIT
    free_listp = NULL; // TODO: 초기화
#endif

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
    if (size == 0)
    {
        return NULL;
    }

    size_t asize = ALIGN(size + 8); // 실제 블록 크기
    void *bp = find_fit(asize);

    if (bp == NULL)
    {
        bp = extend_heap(asize);
    }

    if (bp == NULL)
    {
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
    // 케이스 1: size == 0
    if (size == 0){
        mm_free(ptr);
        return NULL;
    }
    // 케이스 2: ptr == NULL
    if (ptr == NULL)
    {
        return mm_malloc(size);
    } 
    
    // 그냥 size는 payload의 크기이기에, 전체 블럭의 크기 기준으로 비교 필요
    size_t asize = ALIGN(size + 8); 

    // 케이스 3: 새 크기 <= 현재 블록 크기
    if (asize <= GET_SIZE(HDRP(ptr))) 
    {
        return ptr;
    }
    // 케이스 4: 다음 블록 free이고 합치면 충분
    if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && (GET_SIZE(HDRP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) >= asize)
    {
        char *next_bp = NEXT_BLKP(ptr);
        size_t next_size = GET_SIZE(HDRP(next_bp));
        size_t merged_size = GET_SIZE(HDRP(ptr)) + next_size;

        remove_free(next_bp, _get_bucket(_get_bucket_index(next_size)));
        PUT(HDRP(ptr), PACK(merged_size, 1));
        PUT(FTRP(ptr), PACK(merged_size, 1));
        return ptr;
    }

    // 케이스 5 (else): 기존 naive 코드
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    copySize = GET_SIZE(HDRP(oldptr));
    if (newptr == NULL)
        return NULL;

    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

// 1. 빈 블록 탐색
static void *find_fit(size_t size)
{
#ifdef SEGLIST
#ifdef BEST_FIT
    // seglist best fit (TODO)
    char *best = NULL;
    size_t best_diff = SIZE_MAX; // 2^64 - 1 = 18446744073709551615UL

    for(int i = _get_bucket_index(size); i <= 8; i++) {
        for(char *bp = *_get_bucket(i); bp != NULL; bp = NEXT_FREE(bp)) {
            if (GET_SIZE(HDRP(bp)) >= size) {
                size_t diff = GET_SIZE(HDRP(bp)) - size;
                if (diff < best_diff) {
                    best = bp;
                    best_diff = diff;
                }
            }
        }
    }
    return best;
#else
    // seglist first fit 
    for(int i = _get_bucket_index(size); i <= 8; i++){   
        for(char *bp = *_get_bucket(i); bp != NULL; bp = NEXT_FREE(bp)){
            if (GET_SIZE(HDRP(bp)) >= size)
            {
                return bp;
            }   
        }
    }


#endif
    return NULL;
#elif defined(EXPLICIT)
#ifdef BEST_FIT
    // explicit best fit (TODO)

#else
    // explicit first fit
    for (char *bp = free_listp; bp != NULL; bp = NEXT_FREE(bp))
    {
        if (GET_SIZE(HDRP(bp)) >= size)
        {
            return bp;
        }
    }

#endif
    return NULL;

#else
#ifdef BEST_FIT
    // implicit best fit (TODO)

#else
    // implicit first fit (현재 코드)
    for (char *bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= size)
            return bp;
    }
    return NULL;
#endif
#endif
}

// 2. 블록 배치 (header/footer 업데이트)
static void place(void *bp, size_t size)
{
#ifdef SEGLIST
    // seglist place
    char **curr_bucket = _get_bucket(_get_bucket_index(GET_SIZE(HDRP(bp))));
    remove_free(bp, curr_bucket);
    size_t old_size = GET_SIZE(HDRP(bp));
    if (old_size - size >= 24)
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));

        char **remainder_bucket = _get_bucket(_get_bucket_index(old_size - size));
        insert_free(NEXT_BLKP(bp), remainder_bucket); // 나머지 블록
    }
    else
    {
        PUT(HDRP(bp), PACK(old_size, 1));
        PUT(FTRP(bp), PACK(old_size, 1));
    }


#elif defined(EXPLICIT)
    // explicit place
    /*
    1. remove_free(bp) — free list에서 제거
    2. 헤더/푸터 allocated로 표시
    3. split 조건 확인 (old_size - size >= 24)
    4. split이면 나머지 블록을 insert_free로 free list에 추가
    */
    remove_free(bp, &free_listp);
    size_t old_size = GET_SIZE(HDRP(bp));
    if (old_size - size >= 24)
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));

        insert_free(NEXT_BLKP(bp), &free_listp); // 나머지 블록
    }
    else
    {
        PUT(HDRP(bp), PACK(old_size, 1));
        PUT(FTRP(bp), PACK(old_size, 1));
    }

#else
    // implicit place (현재 코드)
    size_t old_size = GET_SIZE(HDRP(bp));
    if (old_size - size >= 16)
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK((old_size - size), 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(old_size, 1));
        PUT(FTRP(bp), PACK(old_size, 1));
    }
#endif
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
    PUT(HDRP(bp), PACK(newsize, 0));
    PUT(FTRP(bp), PACK(newsize, 0));
    PUT((char *)(FTRP(bp)) + 4, PACK(0, 1)); // 크기 0, allocated

#ifdef SEGLIST
    return coalesce(bp);
#elif defined(EXPLICIT)
    return coalesce(bp);
#else
    return bp;
#endif
}

// 4. free - 4가지 경우 - coalesce 하기
// 실제 블록 병합 로직 (implicit/explicit 공통)
static void *_coalesce_blocks(void *ptr)
{
    // 1. 양쪽이 다 점유되어있는 경우
    if (GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && GET_ALLOC(HDRP(_prev_blkp(ptr))))
        return ptr;

    size_t curr_size = GET_SIZE(HDRP(ptr));
    size_t prev_size = GET_SIZE(HDRP(_prev_blkp(ptr)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

    // 2. 한쪽만 점유되어있는 경우 (왼쪽, 이전)
    if (GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && !GET_ALLOC(HDRP(_prev_blkp(ptr))))
    {
        size_t merge_size = curr_size + prev_size;
        PUT(HDRP(_prev_blkp(ptr)), PACK(merge_size, 0));
        PUT(FTRP(ptr), PACK(merge_size, 0));
        return _prev_blkp(ptr);
    }

    // 3. 한쪽만 점유되어있는 경우 (오른쪽, 다음)
    if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && GET_ALLOC(HDRP(_prev_blkp(ptr))))
    {
        size_t merge_size = curr_size + next_size;
        PUT(HDRP(ptr), PACK(merge_size, 0));
        PUT(FTRP(ptr), PACK(merge_size, 0));
        return ptr;
    }

    // 4. 양쪽 다 free인 경우.
    size_t merge_size = curr_size + prev_size + next_size;
    PUT(HDRP(_prev_blkp(ptr)), PACK(merge_size, 0));
    PUT(FTRP(_prev_blkp(ptr)), PACK(merge_size, 0));
    return _prev_blkp(ptr);
}

// 5. coalesce 함수 분리
static void *coalesce(void *ptr)
{
#ifdef SEGLIST
    // seglist coalesce
    // 병합 전 각 블록의 버킷
    char **prev_bucket = _get_bucket(_get_bucket_index(GET_SIZE(HDRP(_prev_blkp(ptr)))));
    char **curr_bucket = _get_bucket(_get_bucket_index(GET_SIZE(HDRP(ptr))));
    char **next_bucket = _get_bucket(_get_bucket_index(GET_SIZE(HDRP(NEXT_BLKP(ptr)))));

    /* 케이스 1 (양쪽 allocated):
    1. insert_free(ptr)
    2. _coalesce_blocks(ptr) → 사실 병합 없음, ptr 그대로 반환
    */

    unsigned int prev_get_alloc = GET_ALLOC(HDRP(_prev_blkp(ptr)));
    unsigned int next_get_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));

    if (prev_get_alloc && next_get_alloc)
    {
        insert_free(ptr, curr_bucket);
        return _coalesce_blocks(ptr);
    }

    /* 케이스 2 (이전만 free):
    1. remove_free(_prev_blkp(ptr))
    2. _coalesce_blocks(ptr) → _prev_blkp(ptr) 반환
    3. insert_free(결과)
    */
    if (next_get_alloc)
    {
        remove_free(_prev_blkp(ptr), prev_bucket);
        void *result = _coalesce_blocks(ptr);
        char **result_bucket = _get_bucket(_get_bucket_index(GET_SIZE(HDRP(result))));
        insert_free(result, result_bucket);
        return result;
    }

    /*
    케이스 3 (다음만 free):
    1. remove_free(NEXT_BLKP(ptr))
    2. _coalesce_blocks(ptr) → ptr 반환
    3. insert_free(결과)
    */
    if (prev_get_alloc)
    {
        remove_free(NEXT_BLKP(ptr), next_bucket);
        void *result = _coalesce_blocks(ptr);
        char **result_bucket = _get_bucket(_get_bucket_index(GET_SIZE(HDRP(result))));
        insert_free(result, result_bucket);
        return result;
    }

    /*
    케이스 4 (양쪽 free):
    1. remove_free(_prev_blkp(ptr))
    2. remove_free(NEXT_BLKP(ptr))
    3. _coalesce_blocks(ptr) → _prev_blkp(ptr) 반환
    4. insert_free(결과)
    */
    else
    {
        remove_free(_prev_blkp(ptr), prev_bucket);
        remove_free(NEXT_BLKP(ptr), next_bucket);
        void *result = _coalesce_blocks(ptr);
        char **result_bucket = _get_bucket(_get_bucket_index(GET_SIZE(HDRP(result))));
        insert_free(result, result_bucket);
        return result;
    }

#elif defined(EXPLICIT)
    // explicit coalesce  - free list 관리 + _coalesce_blocks 호출
    /* 여기도 케이스 4가지로 병합 */

    /* 케이스 1 (양쪽 allocated):
    1. insert_free(ptr)
    2. _coalesce_blocks(ptr) → 사실 병합 없음, ptr 그대로 반환
    */

    unsigned int prev_get_alloc = GET_ALLOC(HDRP(_prev_blkp(ptr)));
    unsigned int next_get_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));

    if (prev_get_alloc && next_get_alloc)
    {
        insert_free(ptr, &free_listp);
        return _coalesce_blocks(ptr);
    }

    /* 케이스 2 (이전만 free):
    1. remove_free(_prev_blkp(ptr))
    2. _coalesce_blocks(ptr) → _prev_blkp(ptr) 반환
    3. insert_free(결과)
    */
    if (next_get_alloc)
    {
        remove_free(_prev_blkp(ptr), &free_listp);
        void *result = _coalesce_blocks(ptr);
        insert_free(result, &free_listp);
        return result;
    }

    /*
    케이스 3 (다음만 free):
    1. remove_free(NEXT_BLKP(ptr))
    2. _coalesce_blocks(ptr) → ptr 반환
    3. insert_free(결과)
    */
    if (prev_get_alloc)
    {
        remove_free(NEXT_BLKP(ptr), &free_listp);
        void *result = _coalesce_blocks(ptr);
        insert_free(result, &free_listp);
        return result;
    }

    /*
    케이스 4 (양쪽 free):
    1. remove_free(_prev_blkp(ptr))
    2. remove_free(NEXT_BLKP(ptr))
    3. _coalesce_blocks(ptr) → _prev_blkp(ptr) 반환
    4. insert_free(결과)
    */
    else
    {
        remove_free(_prev_blkp(ptr), &free_listp);
        remove_free(NEXT_BLKP(ptr), &free_listp);
        void *result = _coalesce_blocks(ptr);
        insert_free(result, &free_listp);
        return result;
    }

#else
    return _coalesce_blocks(ptr);
#endif
}

// 6. explicit coalesce를 위해 함수 분리
/* free list에 삽입 */
static void insert_free(void *bp, char **bucket)
{
    char *curr = *bucket;
    char *prev = NULL;

    // 현재 블록 주소 < 삽입할 블록의 주소를 판별한다.
    while (curr != NULL && curr < (char *)bp)
    {
        prev = curr;
        curr = NEXT_FREE(curr);
    }
    // 루프 끝나면:
    // prev = bp 앞에 올 블록 (없으면 NULL)
    if (prev)
    {
        SET_NEXT_FREE(prev, bp); // A.next = bp
        SET_PREV_FREE(bp, prev); // bp.prev = A
    }
    else
    {
        SET_PREV_FREE(bp, NULL); // bp.prev = NULL
        *bucket = bp;            // 리스트 시작점 = bp
    }

    if (curr)
    {
        SET_NEXT_FREE(bp, curr); // bp.next = B
        SET_PREV_FREE(curr, bp); // B.prev = bp
    }
    else
    {
        SET_NEXT_FREE(bp, NULL); // bp.next = NULL
    }
}

/* free list에서 제거 */
static void remove_free(void *bp, char **bucket)
{
    /* coalesce와 똑같이 4가지 경우.
    1. prev 있고 next 있음 → 중간 제거
    2. prev 없고 next 있음 → 맨 앞 제거
    3. prev 있고 next 없음 → 맨 뒤 제거
    4. prev 없고 next 없음 → 유일한 블록 제거
    */

    // 1. prev 있고 next 있음 → 중간 제거
    if (PREV_FREE(bp) && NEXT_FREE(bp))
    {
        char *A = PREV_FREE(bp);
        char *B = NEXT_FREE(bp);

        SET_NEXT_FREE(A, B);
        SET_PREV_FREE(B, A);
    }

    // 2. prev 없고 next 있음 → 맨 앞 제거
    if (!PREV_FREE(bp) && NEXT_FREE(bp))
    {
        char *B = NEXT_FREE(bp);
        SET_PREV_FREE(B, NULL);
        *bucket = B;
    }

    // 3. prev 있고 next 없음 → 맨 뒤 제거
    if (PREV_FREE(bp) && !NEXT_FREE(bp))
    {
        char *A = PREV_FREE(bp);
        SET_NEXT_FREE(A, NULL);
    }

    // 4. prev 없고 next 없음 → 유일한 블록 제거
    if (!PREV_FREE(bp) && !NEXT_FREE(bp))
    {
        *bucket = NULL;
    }
}

/* 7. SegList를 위한 함수*/
// 버킷 인덱스 반환 (GET)
static int _get_bucket_index(size_t size)
{
    // size에 따라 0~8 반환
    int index = 0;

    if (size <= 32)
    {
        return index;
    }
    else
    {
        size = size >> 5;
        while (size > 0)
        {
            size = (size >> 1);
            index += 1;
        }

        if (index > 8)
        {
            return 8;
        }

        return index;
    }
}

/* 8. 인덱스를 받아서로 포인터를 반환하는 함수*/
static char **_get_bucket(int index)
{
    switch (index)
    {
    case 0:
        return &seg_list_0;
    case 1:
        return &seg_list_1;
    case 2:
        return &seg_list_2;
    case 3:
        return &seg_list_3;
    case 4:
        return &seg_list_4;
    case 5:
        return &seg_list_5;
    case 6:
        return &seg_list_6;
    case 7:
        return &seg_list_7;
    case 8:
        return &seg_list_8;
    default:
        return &seg_list_8;
    }
}

/* footer 제거를 위해 만드는 함수 */
// 9. _prev_blkp : 함수로 리팩토링
static void *_prev_blkp(void *ptr) {
    if (GET_PREV_ALLOC(HDRP(ptr))) {
        // 이전 블록이 allocated → footer 없음 → ??? (coalesce가 필요없으므로 호출이 안됨.)
        return NULL;
    } else {
        // 이전 블록이 free → footer에서 크기 읽기
        return (char *)(ptr) - GET_SIZE(HDRP(ptr) - 4);
    }
}