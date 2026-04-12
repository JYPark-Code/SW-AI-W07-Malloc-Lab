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
// #define NEXT_FIT   // Next Fit
// 둘 다 주석이면 First Fit
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
- [ ] 버킷 설계 (2의 거듭제곱 기준 9개 버킷)
- [ ] insert_free - 버킷 탐색 + 삽입
- [ ] remove_free - 버킷에서 제거
- [ ] find_fit - best fit 탐색
- [ ] place - split + 나머지 블록 적절한 버킷에 삽입

### 추가 최적화
- [ ] Best Fit
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