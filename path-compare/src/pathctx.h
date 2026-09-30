/* pathctx.h - 탐색 코드끼리만 쓰는 도구 (과제 1의 sortctx.h 역할)
 * 이웃 순서, 경로 복원, 힙을 한 곳에 둬서 세 방식이 같은 조건으로 세어지게 한다 */
#ifndef PATHCTX_H
#define PATHCTX_H

#include <stddef.h>

#include "grid.h"

/* 이웃 순서 고정: 오른쪽, 아래, 왼쪽, 위. 순서가 바뀌면 같은 비용 경로 중 고르는 것이 달라진다 */
extern const int DIR_X[4];
extern const int DIR_Y[4];

/* 칸 v에서 방향 d로 갈 수 있으면 이웃 번호, 아니면 -1 (지도 밖이거나 벽) */
int gridNeighbor(const Grid *g, int v, int d);

/* parent를 따라 goal에서 start까지 거슬러 올라가 cost, steps, path를 채운다 */
void pathFinish(const Grid *g, const int *parent, int start, int goal, PathStats *stats,
                int *path, size_t *pathLen);

/* --- 이진 최소 힙 (과제 1의 힙 정렬 siftDown을 최소 힙으로 뒤집어 씀) --- */

typedef struct HeapItem {
    long f; /* 우선순위: Dijkstra는 g, A*는 g + h */
    long g; /* 출발부터 실제 비용. f가 같으면 g가 큰 것(목표에 가까운 것) 먼저 */
    int v;  /* 칸 번호 */
} HeapItem;

typedef struct Heap {
    HeapItem *items;
    size_t size;
    size_t cap;
} Heap;

int heapInit(Heap *h, size_t cap);
void heapFree(Heap *h);
int heapPush(Heap *h, HeapItem it); /* 모자라면 2배로 늘림. 실패하면 0 */
HeapItem heapPop(Heap *h);          /* size > 0일 때만 부른다 */

/* Dijkstra와 A*의 공통 몸통. hUnit = 0이면 Dijkstra, 칸 최소 비용이면 A*.
 * 두 방식의 차이가 우선순위에 h를 더하느냐 한 가지뿐이라는 걸 코드로 보이려고 합쳤다 */
int bestFirstSearch(const Grid *g, int start, int goal, int hUnit, PathStats *stats, int *path,
                    size_t *pathLen);

#endif
