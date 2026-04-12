# SW 정글 Week 7 - Malloc Lab

CS:APP Malloc Lab 구현 프로젝트입니다.  
C로 직접 `malloc`, `free`, `realloc`을 구현하며 동적 메모리 할당기의 원리를 학습합니다.

---

## 구현 방법 선택

`mm.c` 상단의 매크로 주석을 해제해 방법을 선택합니다:

```c
// #define EXPLICIT   // Explicit Free List
// #define SEGLIST    // Segregated Free List
// 둘 다 주석이면 Implicit Free List

// #define BEST_FIT   // Best Fit
// 주석이면 First Fit
```

---

## 구현 진행 상황

### Implicit Free List
- [x] 기본 블록 구조 (header / payload / footer)
- [x] 경계 태그 (boundary tag) - PACK, GET_SIZE, GET_ALLOC
- [x] mm_init - prologue / epilogue 힙 초기화
- [x] mm_malloc - first fit 탐색 + extend_heap
- [x] mm_free - alloc 비트 해제
- [x] coalesce - 인접 free 블록 4가지 케이스 병합
- [x] place - split 구현 (최소 블록 16바이트)

### Explicit Free List (Doubly Linked List)
- [x] free 블록 내 prev/next 포인터 저장 (payload 공간 재활용)
- [x] PREV_FREE / NEXT_FREE / SET_PREV_FREE / SET_NEXT_FREE 매크로
- [x] insert_free - address-ordered 방식 삽입
- [x] remove_free - 4가지 케이스 제거
- [x] coalesce - free list 관리 + 블록 병합
- [x] find_fit - free_listp 순회
- [x] place - remove_free + split + insert_free (최소 블록 24바이트)

### Segregated Free List
- [x] 버킷 설계 (2의 거듭제곱 기준 9개 버킷, seg_list_0 ~ seg_list_8)
- [x] _get_bucket_index - 크기 기반 버킷 인덱스 계산
- [x] _get_bucket - 인덱스로 버킷 포인터 반환
- [x] insert_free - 버킷 인자화로 explicit/seglist 공용
- [x] remove_free - 버킷 인자화로 explicit/seglist 공용
- [x] coalesce - 병합 전후 동적 버킷 선택
- [x] find_fit - 버킷 인덱스부터 순회, 없으면 다음 버킷
- [x] place - 크기 기반 버킷 선택 + split

### mm_realloc 최적화
- [x] 케이스 1: size == 0 → mm_free 후 NULL 반환
- [x] 케이스 2: ptr == NULL → mm_malloc 반환
- [x] 케이스 3: 새 크기 <= 현재 블록 크기 → ptr 반환
- [x] 케이스 4: 다음 블록 free + 합치면 충분 → 병합 후 반환

### 추가 최적화
- [ ] Best Fit (seglist)
- [ ] Footer 제거 (allocated 블록)

---

## 빌드 및 테스트

```bash
make clean && make

# 전체 trace 테스트
./mdriver -V

# 단일 trace 테스트
./mdriver -V -f short1-bal.rep
./mdriver -V -f short2-bal.rep
```

---

## 현재 점수

| 방법 | Util | Throughput | 총점 |
|------|------|-----------|------|
| Implicit + First Fit | 75% | 22/40 | 67/100 |
| Explicit + First Fit | 74% | 40/40 | 84/100 |
| Seglist + First Fit | 74% | 40/40 | 85/100 |
| Seglist + First Fit + realloc 최적화 | 79% | 40/40 | 87/100 |

---

## 핵심 개념 정리

### 블록 구조
```
Allocated:  [header 4][payload ...][footer 4]
Free:       [header 4][prev* 8][next* 8][...][footer 4]
```

### 헤더/푸터 인코딩
```
31                3  2  1  0
[   size (29bit)  |  0  0  a]
                           ↑ allocated bit
```

### Boundary Tag
- header와 footer에 동일한 값(size | alloc) 저장
- 이전 블록 크기를 O(1)로 역추적 가능 → coalesce 효율화

### Segregated Free List 버킷 구조
```
seg_list_0:    1 ~   31 바이트
seg_list_1:   32 ~   63 바이트
seg_list_2:   64 ~  127 바이트
seg_list_3:  128 ~  255 바이트
seg_list_4:  256 ~  511 바이트
seg_list_5:  512 ~ 1023 바이트
seg_list_6: 1024 ~ 2047 바이트
seg_list_7: 2048 ~ 4095 바이트
seg_list_8: 4096 ~      바이트
```