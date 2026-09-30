#include "bench.h"

#include <stdlib.h>
#include <time.h>

const char *mapKindName(MapKind kind) {
    switch (kind) {
        case MAP_OPEN:    return "빈 지도";
        case MAP_RANDOM:  return "무작위 벽";
        case MAP_MAZE:    return "미로";
        case MAP_TERRAIN: return "지형 비용";
        case MAP_CITY:    return "도로망";
        default:          return "?";
    }
}

const char *mapKindKey(MapKind kind) {
    switch (kind) {
        case MAP_OPEN:    return "open";
        case MAP_RANDOM:  return "random";
        case MAP_MAZE:    return "maze";
        case MAP_TERRAIN: return "terrain";
        case MAP_CITY:    return "city";
        default:          return "unknown";
    }
}

static const int MAZE_DX[4] = {2, 0, -2, 0};
static const int MAZE_DY[4] = {0, 2, 0, -2};

/* 미로: 짝수 좌표 칸을 방으로 보고, 방 사이 벽을 무너뜨리며 깊이 우선으로 판다.
 * 재귀 대신 명시적 스택 (n = 2049면 재귀 깊이가 100만을 넘을 수 있음) */
static int carveMaze(Grid *g, Minstd *rng) {
    int n = g->w;
    int rooms = (n + 1) / 2;
    size_t total = (size_t)rooms * (size_t)rooms;
    int *stack = (int *)malloc(total * sizeof(int));
    if (stack == NULL) {
        return 0;
    }
    for (size_t i = 0; i < (size_t)n * (size_t)n; i++) {
        g->cost[i] = CELL_WALL;
    }
    size_t top = 0;
    stack[top++] = 0;
    g->cost[0] = 1;
    while (top > 0) {
        int v = stack[top - 1];
        int x = v % n, y = v / n;
        int cand[4], k = 0;
        for (int d = 0; d < 4; d++) {
            int nx = x + MAZE_DX[d], ny = y + MAZE_DY[d];
            if (nx >= 0 && ny >= 0 && nx < n && ny < n && g->cost[ny * n + nx] == CELL_WALL) {
                cand[k++] = d;
            }
        }
        if (k == 0) {
            top--;
            continue;
        }
        int d = cand[minstdNext(rng) % (unsigned long)k];
        int nx = x + MAZE_DX[d], ny = y + MAZE_DY[d];
        g->cost[(y + MAZE_DY[d] / 2) * n + (x + MAZE_DX[d] / 2)] = 1; /* 사이 벽 */
        g->cost[ny * n + nx] = 1;
        stack[top++] = ny * n + nx;
    }
    free(stack);
    return 1;
}

int makeMap(Grid *g, int n, MapKind kind, unsigned long seed) {
    if (!gridInit(g, n, n)) {
        return 0;
    }
    size_t cells = (size_t)n * (size_t)n;
    int start = 0, goal = (int)cells - 1;
    Minstd rng;
    minstdSeed(&rng, seed);

    switch (kind) {
        case MAP_OPEN:
            break;
        case MAP_RANDOM:
            for (int tries = 0; ; tries++) {
                for (size_t i = 0; i < cells; i++) {
                    g->cost[i] = (minstdNext(&rng) % 100 < 30) ? CELL_WALL : 1;
                }
                g->cost[start] = g->cost[goal] = 1;
                if (bfsSolve(g, start, goal, NULL, NULL, NULL) || tries > 1000) {
                    break;
                }
                minstdSeed(&rng, seed + (unsigned long)tries + 1);
            }
            break;
        case MAP_MAZE:
            if (!carveMaze(g, &rng)) {
                gridFree(g);
                return 0;
            }
            break;
        case MAP_TERRAIN:
            for (size_t i = 0; i < cells; i++) {
                g->cost[i] = (unsigned char)(1 + minstdNext(&rng) % CELL_MAX_COST);
            }
            break;
        case MAP_CITY:
            for (int y = 0; y < n; y++) {
                for (int x = 0; x < n; x++) {
                    unsigned char c = 7;                        /* 비포장 */
                    if (x % 8 == 4 || y % 8 == 4) {
                        c = 3;                                  /* 골목 */
                    }
                    if (x % 32 == 16 || y % 32 == 16) {
                        c = 1;                                  /* 간선도로 */
                    }
                    g->cost[y * n + x] = c;
                }
            }
            break;
        default:
            break;
    }
    return 1;
}

BenchResult benchRun(const PathAlgorithm *algo, const Grid *g, int start, int goal, int reps) {
    BenchResult r;
    r.algo = algo;
    r.millis = 0.0;
    r.valid = 0;
    pathStatsReset(&r.stats);
    if (reps < 1) {
        reps = 1;
    }
    size_t cells = (size_t)g->w * (size_t)g->h;
    int *path = (int *)malloc(cells * sizeof(int));
    if (path == NULL) {
        return r;
    }
    size_t len = 0;
    clock_t spent = 0;
    for (int t = 0; t < reps; t++) {
        clock_t begin = clock();
        algo->solve(g, start, goal, &r.stats, path, &len);
        spent += clock() - begin;
    }
    r.millis = (double)spent * 1000.0 / CLOCKS_PER_SEC / reps;
    r.valid = r.stats.found && pathValid(g, start, goal, path, len, r.stats.cost);
    free(path);
    return r;
}
