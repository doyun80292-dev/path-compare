/* dijkstra.c - Dijkstra와 A*가 같이 쓰는 최선 우선 탐색 + Dijkstra 입구
 *
 * 힙에서 f가 가장 작은 칸을 꺼내 확정(closed)하고, 이웃의 dist를 줄일 수 있으면 줄여서 힙에 넣는다.
 *   Dijkstra : f = g            (출발부터 지금까지 비용)
 *   A*       : f = g + h        (h = 남은 맨해튼 거리 x 칸 최소 비용, 과대평가하지 않음)
 * 힙에서 원소를 지우거나 줄이는 대신, 새 값을 한 번 더 넣고 꺼낼 때 이미 확정된 칸이면 버린다
 * (lazy deletion). 그래서 pushes가 확장 수보다 많을 수 있다. */
#include <limits.h>
#include <stdlib.h>

#include "grid.h"
#include "pathctx.h"

static long heuristic(const Grid *g, int v, int goal, int hUnit) {
    int dx = abs(v % g->w - goal % g->w);
    int dy = abs(v / g->w - goal / g->w);
    return (long)hUnit * (dx + dy);
}

int bestFirstSearch(const Grid *g, int start, int goal, int hUnit, PathStats *stats, int *path,
                    size_t *pathLen) {
    PathStats local;
    if (stats == NULL) {
        stats = &local;
    }
    pathStatsReset(stats);
    if (pathLen != NULL) {
        *pathLen = 0;
    }
    size_t n = (size_t)g->w * (size_t)g->h;
    if (g->cost[start] == CELL_WALL || g->cost[goal] == CELL_WALL) {
        return 0;
    }

    long *dist = (long *)malloc(n * sizeof(long));
    int *parent = (int *)malloc(n * sizeof(int));
    unsigned char *closed = (unsigned char *)calloc(n, 1);
    Heap open;
    int heapOk = heapInit(&open, 1024);
    if (dist == NULL || parent == NULL || closed == NULL || !heapOk) {
        free(dist);
        free(parent);
        free(closed);
        if (heapOk) {
            heapFree(&open);
        }
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        dist[i] = LONG_MAX;
        parent[i] = -1;
    }

    dist[start] = 0;
    parent[start] = start;
    heapPush(&open, (HeapItem){heuristic(g, start, goal, hUnit), 0, start});
    stats->pushes = 1;
    stats->peakOpen = 1;

    while (open.size > 0) {
        HeapItem it = heapPop(&open);
        int v = it.v;
        if (closed[v]) {
            continue; /* 더 싼 값으로 이미 확정된 칸의 옛 기록 */
        }
        closed[v] = 1;
        stats->expanded++;
        if (pathVisitMark != NULL) {
            pathVisitMark[v] = 1;
        }
        if (v == goal) {
            pathFinish(g, parent, start, goal, stats, path, pathLen);
            break;
        }
        for (int d = 0; d < 4; d++) {
            int u = gridNeighbor(g, v, d);
            if (u < 0 || closed[u]) {
                continue;
            }
            long nd = dist[v] + g->cost[u];
            if (nd < dist[u]) {
                dist[u] = nd;
                parent[u] = v;
                if (!heapPush(&open, (HeapItem){nd + heuristic(g, u, goal, hUnit), nd, u})) {
                    open.size = 0; /* 메모리 부족: 못 찾은 것으로 끝낸다 */
                    break;
                }
                stats->pushes++;
            }
        }
        if (open.size > stats->peakOpen) {
            stats->peakOpen = open.size;
        }
    }
    /* 힙은 줄어들지 않으므로 끝났을 때 cap이 가장 컸을 때의 크기다 */
    stats->extraBytes = n * (sizeof(long) + sizeof(int) + 1) + open.cap * sizeof(HeapItem);
    free(dist);
    free(parent);
    free(closed);
    heapFree(&open);
    return stats->found;
}

int dijkstraSolve(const Grid *g, int start, int goal, PathStats *stats, int *path,
                  size_t *pathLen) {
    return bestFirstSearch(g, start, goal, 0, stats, path, pathLen);
}
