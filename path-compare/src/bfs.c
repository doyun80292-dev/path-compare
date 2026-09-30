/* bfs.c - 너비 우선 탐색 (수업에서 배운 큐 기반 BFS)
 * 칸 비용을 보지 않고 "몇 번 움직였나"만 센다. 그래서 비용이 모두 같으면 최단이지만,
 * 비용이 다르면 이동 횟수가 가장 적은 경로를 줄 뿐 비용이 가장 적은 경로는 아닐 수 있다. */
#include <stdlib.h>

#include "grid.h"
#include "pathctx.h"

int bfsSolve(const Grid *g, int start, int goal, PathStats *stats, int *path, size_t *pathLen) {
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

    int *parent = (int *)malloc(n * sizeof(int)); /* -1 = 아직 안 봄. 방문 표시를 겸함 */
    int *queue = (int *)malloc(n * sizeof(int));  /* 칸마다 한 번만 들어가므로 n칸이면 충분 */
    if (parent == NULL || queue == NULL) {
        free(parent);
        free(queue);
        return 0;
    }
    stats->extraBytes = 2 * n * sizeof(int);
    for (size_t i = 0; i < n; i++) {
        parent[i] = -1;
    }

    size_t head = 0, tail = 0;
    parent[start] = start;
    queue[tail++] = start;
    stats->pushes = 1;
    stats->peakOpen = 1;

    while (head < tail) {
        int v = queue[head++];
        stats->expanded++;
        if (pathVisitMark != NULL) {
            pathVisitMark[v] = 1;
        }
        if (v == goal) { /* Dijkstra · A*와 맞추려고 "꺼낼 때" 끝낸다 */
            pathFinish(g, parent, start, goal, stats, path, pathLen);
            break;
        }
        for (int d = 0; d < 4; d++) {
            int u = gridNeighbor(g, v, d);
            if (u < 0 || parent[u] != -1) {
                continue;
            }
            parent[u] = v;
            queue[tail++] = u;
            stats->pushes++;
        }
        if (tail - head > stats->peakOpen) {
            stats->peakOpen = tail - head;
        }
    }
    free(parent);
    free(queue);
    return stats->found;
}
