# SW 정글 Week 7 - Malloc Lab

> C로 직접 `malloc`, `free`, `realloc`을 구현하며 동적 메모리 할당기의 원리를 학습합니다.  
> CS:APP(Computer Systems: A Programmer's Perspective) Malloc Lab 기반 프로젝트

---

## 최종 점수

```
Perf index = 52 (util) + 40 (thru) = 92/100
```

| trace | 파일 | util | 비고 |
|-------|------|------|------|
| 0~3 | amptjp, cccp, cp-decl, expr | 99% | 일반 할당 패턴 |
| 4 | coalescing-bal.rep | 66% | ALIGN 특성상 8바이트 낭비 |
| 5~6 | random, random2 | 95%, 94% | 랜덤 패턴 |
| 7 | binary-bal.rep | 60% | 외부 단편화 |
| 8 | binary2-bal.rep | 53% | 외부 단편화 |
| 9 | realloc-bal.rep | 99% | realloc 최적화 효과 |
| 10 | realloc2-bal.rep | 87% | 힙 끝 확장 + 이전 블록 활용 |

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

## 아키텍처 설계

### 블록 구조 진화

**초기 (Implicit)**
```
┌────────┬─────────────────────────┬────────┐
│header 4│       payload           │footer 4│
└────────┴─────────────────────────┴────────┘
```

**Explicit 이후 (free 블록)**
```
┌────────┬────────┬────────┬───────┬────────┐
│header 4│ prev* 8│ next* 8│  ...  │footer 4│
└────────┴────────┴────────┴───────┴────────┘
최소 블록 = 4 + 8 + 8 + 4 = 24바이트
```

**Footer 제거 후 (allocated 블록)**
```
┌────────┬─────────────────────────┐
│header 4│       payload           │  ← footer 없음!
└────────┴─────────────────────────┘
```

### 헤더 인코딩

```
31                           3  2  1  0
┌──────────────────────────┬──┬──┬──┬──┐
│      size (29bit)        │  │  │ p│ a│
└──────────────────────────┴──┴──┴──┴──┘
                                  ↑  ↑
                                  │  └── alloc: 현재 블록 할당 여부
                                  └───── prev_alloc: 이전 블록 할당 여부
```

### Segregated Free List 구조

```
seg_list_0  ──→ [24] ──→ [24] ──→ NULL        (1~24)
seg_list_1  ──→ [32] ──→ NULL                  (25~32)
seg_list_2  ──→ [40] ──→ [48] ──→ NULL        (33~48)
    ...
seg_list_19 ──→ [8192] ──→ NULL                (4097~)

각 버킷 = address-ordered doubly linked list
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
- [x] insert_free / remove_free - 버킷 인자화로 explicit/seglist 공용
- [x] coalesce - 병합 전후 동적 버킷 선택
- [x] find_fit - 버킷 인덱스부터 순회, 없으면 다음 버킷
- [x] place - 크기 기반 버킷 선택 + split

### mm_realloc 최적화
- [x] 케이스 1: size == 0 → mm_free 후 NULL 반환
- [x] 케이스 2: ptr == NULL → mm_malloc 반환
- [x] 케이스 3: 새 크기 <= 현재 블록 크기 → ptr 반환
- [x] 케이스 4-1: 다음 블록 free + 합치면 충분 → 병합 후 반환
- [x] 케이스 4-2: 이전 블록 free + 합치면 충분 → memmove 후 반환
- [x] 케이스 4-2-ext: 이전 free + 힙 끝 + 부족 → 병합하면서 힙 확장
- [x] 케이스 4-3: 양쪽 블록 free + 합치면 충분 → 양쪽 병합
- [x] 케이스 4-3-ext: 이전 free + 힙 끝 + 부족 (양쪽 변형)
- [x] 케이스 4-4: 현재 블록이 힙 끝 → mem_sbrk 직접 확장
- [x] 케이스 5: naive (malloc + memcpy + free)

### 추가 최적화
- [x] Best Fit (seglist) + 조기 종료 (diff == 0)
- [x] Footer 제거 (allocated 블록) - prev_alloc 비트 활용
- [x] 버킷 세분화 - 9개 → 20개 (24~576 구간 촘촘하게)
- [x] CHUNKSIZE 튜닝 실험 - 4096 유지 (최적값 확인)

---

## 점수 이력

| 방법 | Util | Throughput | 총점 |
|------|------|-----------|------|
| Implicit + First Fit | 75% | 22/40 | 67/100 |
| Explicit + First Fit | 74% | 40/40 | 84/100 |
| Seglist + First Fit | 74% | 40/40 | 85/100 |
| Seglist + First Fit + realloc 최적화 | 79% | 40/40 | 87/100 |
| Seglist + Best Fit + realloc 최적화 | 83% | 40/40 | 90/100 |
| + Footer 제거 | 84% | 40/40 | 90/100 |
| + 버킷 세분화(20개) + realloc 고도화 + best fit 조기 종료 | 86% | 40/40 | 91/100 |
| + realloc 힙 끝 확장 (4-4) | 87% | 40/40 | **92/100** |

---

## 최적화 고민 과정

### 접근 방법

```
점수 공식: P = 0.6 * U + 0.4 * min(1, T/Tlibc)

Util(U) 최대 60점 → 메모리 효율
Throughput(T) 최대 40점 → 속도
```

Throughput은 Explicit 단계에서 이미 만점(40점) → 이후 모든 최적화는 **Util 개선**에 집중

### 단계별 의사결정

```
[67점] Implicit First Fit
  ↓ free 블록도 순회하는 비효율
[84점] Explicit First Fit  
  ↓ 하나의 free list, 탐색 비효율
[85점] Seglist First Fit
  ↓ first fit은 큰 블록 낭비
[87점] + realloc 최적화
  ↓ 단편화 문제 지속
[90점] + Best Fit
  ↓ util 개선 여지
[90점] + Footer 제거
  ↓ 버킷 범위가 너무 넓음
[91점] + 버킷 세분화(20개) + realloc 고도화
  ↓ realloc2 trace 힙 끝 패턴 미처리
[92점] + realloc 힙 끝 직접 확장 (4-4)
```

### 시행착오

| 시도 | 결과 | 이유 |
|------|------|------|
| asize +4 (footer 제거 반영) | 실패 | `ALIGN(4095+4) = ALIGN(4095+8) = 4104` — 동일 결과 |
| CHUNKSIZE 2048 | 점수 하락 | trace 8 throughput 절반으로 감소 |
| CHUNKSIZE 8192 | 점수 하락 | util 대폭 감소 |
| free 블록 footer 제거 | 효과 없음 | 최소 블록이 여전히 24바이트 |
| Buddy System | 도입 안 함 | 내부 단편화 증가로 오히려 util 하락 |

---

## 핵심 개념 정리

### coalesce 4가지 케이스

```
케이스 1: [allocated][FREE][allocated]  → 그냥 삽입
케이스 2: [free     ][FREE][allocated]  → 이전 블록과 병합
케이스 3: [allocated][FREE][free     ]  → 다음 블록과 병합
케이스 4: [free     ][FREE][free     ]  → 양쪽 모두 병합
```

### prev_alloc 비트 활용 (Footer 제거)

```
기존: 이전 블록 크기를 알려면 → 이전 블록 footer 읽기
개선: 현재 헤더의 bit1(prev_alloc)으로 판단
  → prev_alloc=1: 이전 블록 allocated → coalesce 불필요
  → prev_alloc=0: 이전 블록 free → footer에서 크기 읽기
```

### mm_realloc 최적화 전략

| 케이스 | 조건 | 동작 |
|--------|------|------|
| 1 | size == 0 | mm_free(ptr) → NULL 반환 |
| 2 | ptr == NULL | mm_malloc(size) 반환 |
| 3 | 현재 블록 크기 충분 | ptr 그대로 반환 |
| 4-1 | 다음 블록 free + 합치면 충분 | remove_free + 헤더 업데이트 → ptr 반환 |
| 4-2 | 이전 블록 free + 합치면 충분 | remove_free + memmove + 헤더 업데이트 → prev_bp 반환 |
| 4-2-ext | 이전 free + 힙 끝 + 부족 | remove_free + mem_sbrk + memmove → prev_bp 반환 |
| 4-3 | 양쪽 free + 합치면 충분 | remove_free x2 + memmove → prev_bp 반환 |
| 4-4 | 현재 블록이 힙 끝 | mem_sbrk 직접 확장 → ptr 반환 |
| 5 | 모두 해당 없음 | malloc + memcpy + free (naive) |

### Segregated Free List 버킷 구조

```
버킷  범위          설계 의도
 0    1~24          최소 블록 전용
 1    25~32         +8 단위
 2~7  33~128        +16 단위 (촘촘)
 8~14 129~576       +64 단위
 15   577~768       +192
 16   769~1024      +256
 17   1025~2048     +1024
 18   2049~4096     +2048
 19   4097~          대형 블록
```

---

## 빌드 및 테스트

```bash
make clean && make

# 전체 trace 테스트
./mdriver -V

# 단일 trace 테스트
./mdriver -V -f traces/binary-bal.rep
./mdriver -V -f traces/realloc-bal.rep
./mdriver -V -f traces/coalescing-bal.rep
```