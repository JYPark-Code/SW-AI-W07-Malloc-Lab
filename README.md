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
- [x] 버킷 설계 (20개 버킷, seg_list_0 ~ seg_list_19)
- [x] _get_bucket_index - if-else 체인으로 크기 기반 버킷 인덱스 계산
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
- [x] 케이스 4-1: 다음 블록 free + 합치면 충분 → 병합 후 반환
- [x] 케이스 4-2: 이전 블록 free + 합치면 충분 → memmove 후 반환
- [x] 케이스 5: naive (malloc + memcpy + free)

### 추가 최적화
- [x] Best Fit (seglist) + 조기 종료 (diff == 0)
- [x] Footer 제거 (allocated 블록) - prev_alloc 비트 활용
- [x] 버킷 세분화 - 9개 → 20개 (24~576 구간 촘촘하게)
- [x] CHUNKSIZE 튜닝 실험 - 4096 유지 (최적값 확인)
- [x] mm_realloc 이전 블록 활용 (케이스 4-2 & 케이스 4-3)

---

## 빌드 및 테스트

```bash
make clean && make

# 전체 trace 테스트
./mdriver -V

# 단일 trace 테스트
./mdriver -V -f traces/binary-bal.rep
./mdriver -V -f traces/realloc-bal.rep
```

---

## 현재 점수

| 방법 | Util | Throughput | 총점 |
|------|------|-----------|------|
| Implicit + First Fit | 75% | 22/40 | 67/100 |
| Explicit + First Fit | 74% | 40/40 | 84/100 |
| Seglist + First Fit | 74% | 40/40 | 85/100 |
| Seglist + First Fit + realloc 최적화 | 79% | 40/40 | 87/100 |
| Seglist + Best Fit + realloc 최적화 | 83% | 40/40 | 90/100 |
| Seglist + Best Fit + realloc 최적화 + Footer 제거 | 84% | 40/40 | 90/100 |
| 위 + 버킷 세분화(20개) + realloc 이전 블록 + best fit 조기 종료 | 86% | 40/40 | 91/100 |

---

## 핵심 개념 정리

### 블록 구조 (Footer 제거 후)
```
Allocated:  [header 4][payload ...]           (footer 없음)
Free:       [header 4][prev* 8][next* 8][...][footer 4]
```

### 헤더 인코딩 (Footer 제거 후)
```
31                3  2  1  0
[   size (29bit)  |  0  p  a]
                        ↑  ↑
                        │  allocated bit (현재 블록)
                        prev_alloc bit (이전 블록 할당 여부)
```

### Boundary Tag
- free 블록만 footer 유지 (coalesce 시 이전 블록 크기 역추적 필요)
- allocated 블록은 footer 제거 → 헤더의 prev_alloc 비트로 대체
- prev_alloc=1이면 이전 블록 footer 없음 → coalesce 불필요

### Segregated Free List 버킷 구조 (세분화 후)
```
seg_list_0:    1 ~   24 바이트
seg_list_1:   25 ~   32 바이트
seg_list_2:   33 ~   48 바이트
seg_list_3:   49 ~   64 바이트
seg_list_4:   65 ~   80 바이트
seg_list_5:   81 ~   96 바이트
seg_list_6:   97 ~  112 바이트
seg_list_7:  113 ~  128 바이트
seg_list_8:  129 ~  192 바이트
seg_list_9:  193 ~  256 바이트
seg_list_10: 257 ~  320 바이트
seg_list_11: 321 ~  384 바이트
seg_list_12: 385 ~  448 바이트
seg_list_13: 449 ~  512 바이트
seg_list_14: 513 ~  576 바이트
seg_list_15: 577 ~  768 바이트
seg_list_16: 769 ~ 1024 바이트
seg_list_17: 1025 ~ 2048 바이트
seg_list_18: 2049 ~ 4096 바이트
seg_list_19: 4097 ~      바이트
```

### mm_realloc 최적화 전략
```
케이스 1: size == 0         → free 후 NULL
케이스 2: ptr == NULL       → malloc
케이스 3: 현재 블록으로 충분 → 그대로 반환
케이스 4-1: 다음 블록 병합  → remove_free + 헤더 업데이트
케이스 4-2: 이전 블록 병합  → remove_free + memmove + 헤더 업데이트
케이스 4-3: 이전, 다음 블록 병합 -> remove_free + memmove + 헤더 업데이트
케이스 5: naive             → malloc + memcpy + free
```