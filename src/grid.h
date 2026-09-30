/* grid.h - 격자 지도와 경로 탐색 3가지(BFS, Dijkstra, A*)를 같은 방식으로 부르기 위한 헤더
 * 과제 1(sort-compare-hw1)의 구조체 + 함수 포인터 방식을 그대로 가져옴 */
#ifndef GRID_H
#define GRID_H

#include <stddef.h>

/* 칸 비용: 0 = 벽(못 지나감), 1~9 = 그 칸에 들어갈 때 드는 비용 */
#define CELL_WALL 0
#define CELL_MAX_COST 9

typedef struct Grid {
    int w, h;
    unsigned char *cost; /* w*h칸, 칸 번호 = y*w + x */
} Grid;

int gridInit(Grid *g, int w, int h);   /* 전부 비용 1로 채움. 실패하면 0 */
void gridFree(Grid *g);
int gridMinCost(const Grid *g);        /* 벽 아닌 칸 중 최소 비용 (A* 휴리스틱에 씀) */

/* 탐색 한 번 할 때 세는 값들 */
typedef struct PathStats {
    size_t expanded;   /* 꺼내서 이웃을 본 칸 수 (= 확장 노드) */
    size_t pushes;     /* 큐/힙에 넣은 횟수 */
    size_t peakOpen;   /* 큐/힙이 가장 컸을 때 원소 수 */
    size_t extraBytes; /* 탐색에 따로 malloc한 메모리 (지도 자체는 뺌) */
    long cost;         /* 찾은 경로의 비용 합 (출발 칸 비용은 안 셈) */
    size_t steps;      /* 찾은 경로의 이동 횟수 */
    int found;         /* 경로를 찾았으면 1 */
} PathStats;

/* 경로를 찾으면 path에 출발→도착 칸 번호를 넣고 칸 수를 *pathLen에 적는다.
 * path는 NULL이어도 되고, 넣으려면 w*h칸이 있어야 한다. */
typedef int (*PathSolve)(const Grid *g, int start, int goal, PathStats *stats,
                         int *path, size_t *pathLen);

typedef struct PathAlgorithm {
    const char *name;
    const char *timeComplexity;
    const char *optimalWhen; /* 최단(최소 비용) 경로를 보장하는 조건 */
    PathSolve solve;
} PathAlgorithm;

int bfsSolve(const Grid *g, int start, int goal, PathStats *stats, int *path, size_t *pathLen);
int dijkstraSolve(const Grid *g, int start, int goal, PathStats *stats, int *path,
                  size_t *pathLen);
int astarSolve(const Grid *g, int start, int goal, PathStats *stats, int *path, size_t *pathLen);

extern const PathAlgorithm PATH_ALGORITHMS[];
extern const size_t PATH_ALGORITHM_COUNT;

void pathStatsReset(PathStats *s);

/* 실험용 (과제 1의 quickSortPivot처럼 전역으로 둠): NULL이 아니면 확장한 칸에 1을 적는다.
 * 세 방식이 지도의 어디를 뒤졌는지 그림으로 그릴 때만 쓴다 */
extern unsigned char *pathVisitMark;

/* 경로가 올바른지: 이웃끼리 이어지고, 벽이 없고, 비용 합이 cost와 같은지 */
int pathValid(const Grid *g, int start, int goal, const int *path, size_t len, long cost);

/* 과제 1에서 쓴 minstd 난수 (수업 topic-04) */
typedef struct Minstd {
    unsigned long state;
} Minstd;

void minstdSeed(Minstd *m, unsigned long seed);
unsigned long minstdNext(Minstd *m);

#endif
