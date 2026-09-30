/* main.c - 경로 탐색 비교 실행
 *   make run                    비교 표 (n = 257, 지도 5종)
 *   ./src/main.out --csv        지도 모양별 + n 배가 측정을 csv로 (그래프용)
 *   ./src/main.out --visit K N  지도 K(open/random/maze/terrain/city), 크기 N에서
 *                               세 방식이 확장한 칸을 글자 그림으로 (그림용)
 *   ./src/main.out --map K N    지도 K, 크기 N의 칸 비용을 글자로 (# = 벽, 1~9 = 비용)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "grid.h"

#define SEED 20261018UL

static MapKind mapKindFromKey(const char *key);

/* --- 사람이 읽는 표 ---------------------------------------------------- */

#define ROW_FORMAT "%-9s %9.3f %10zu %10zu %9zu %11zu B %8ld %6.3f %5s\n"
#define ROW_HEADER "방식        시간(ms)   확장 칸   힙/큐 넣기  open최대     추가메모리    경로비용  비용비   검증\n"
#define ROW_RULE   "-----------------------------------------------------------------------------------------------\n"

static long optimalCost(const Grid *g, int start, int goal) {
    PathStats s;
    dijkstraSolve(g, start, goal, &s, NULL, NULL);
    return s.found ? s.cost : -1;
}

static void reportTable(void) {
    const int n = 257;
    printf("=== 경로 탐색 비교: BFS · Dijkstra · A* ===\n");
    printf("%d x %d 격자, 4방향 이동, 출발 = 왼쪽 위, 도착 = 오른쪽 아래, 5회 평균\n", n, n);
    printf("비용비 = 찾은 경로 비용 / 최소 비용 (1.000이면 최적)\n");
    for (int k = 0; k < MAP_KIND_COUNT; k++) {
        Grid g;
        if (!makeMap(&g, n, (MapKind)k, SEED)) {
            return;
        }
        int start = 0, goal = n * n - 1;
        long best = optimalCost(&g, start, goal);
        printf("\n[%s]  최소 비용 %ld\n%s%s", mapKindName((MapKind)k), best, ROW_HEADER, ROW_RULE);
        for (size_t a = 0; a < PATH_ALGORITHM_COUNT; a++) {
            BenchResult r = benchRun(&PATH_ALGORITHMS[a], &g, start, goal, 5);
            printf(ROW_FORMAT, r.algo->name, r.millis, r.stats.expanded, r.stats.pushes,
                   r.stats.peakOpen, r.stats.extraBytes, r.stats.cost,
                   (double)r.stats.cost / (double)best, r.valid ? "ok" : "FAIL");
        }
        gridFree(&g);
    }
    printf("\n읽는 법\n");
    printf("  확장 칸   : 꺼내서 이웃을 본 칸 수. 시간보다 기계를 덜 탄다.\n");
    printf("  힙/큐 넣기: Dijkstra·A*는 같은 칸을 여러 번 넣을 수 있다 (lazy deletion).\n");
    printf("  추가메모리: 지도 밖에 malloc한 바이트. 힙은 가장 컸을 때 크기.\n");
    printf("  검증      : 경로가 이어지고 벽을 안 지나고 비용 합이 맞는지 (pathValid).\n");
}

/* --- 기계가 읽는 CSV --------------------------------------------------- */

static void csvRows(const char *scope, MapKind kind, int n, int reps) {
    Grid g;
    if (!makeMap(&g, n, kind, SEED)) {
        return;
    }
    int start = 0, goal = n * n - 1;
    long best = optimalCost(&g, start, goal);
    for (size_t a = 0; a < PATH_ALGORITHM_COUNT; a++) {
        BenchResult r = benchRun(&PATH_ALGORITHMS[a], &g, start, goal, reps);
        printf("%s,%s,%d,%s,%.4f,%zu,%zu,%zu,%zu,%ld,%zu,%ld,%d\n", scope, mapKindKey(kind), n,
               r.algo->name, r.millis, r.stats.expanded, r.stats.pushes, r.stats.peakOpen,
               r.stats.extraBytes, r.stats.cost, r.stats.steps, best, r.valid);
        fflush(stdout);
    }
    gridFree(&g);
}

static void reportCsv(void) {
    static const int SIZES[] = {65, 129, 257, 513, 1025, 2049}; /* 미로 때문에 홀수 */
    printf("scope,map,n,algo,millis,expanded,pushes,peakOpen,extraBytes,cost,steps,"
           "optimalCost,valid\n");
    for (int k = 0; k < MAP_KIND_COUNT; k++) {
        csvRows("kinds", (MapKind)k, 513, 5);
    }
    for (int k = 0; k < MAP_KIND_COUNT; k++) {
        for (size_t s = 0; s < sizeof(SIZES) / sizeof(SIZES[0]); s++) {
            csvRows("growth", (MapKind)k, SIZES[s], 3);
        }
    }
}

/* --- 확장한 칸 그림 ------------------------------------------------------ */

/* 한 방식마다 "# algo cost expanded" 줄 다음 n줄:
 *   # 벽, . 안 본 칸, o 확장한 칸, * 찾은 경로 */
static void reportVisit(const char *key, int n) {
    MapKind kind = mapKindFromKey(key);
    Grid g;
    if (kind == MAP_KIND_COUNT || n < 3 || !makeMap(&g, n, kind, SEED)) {
        fprintf(stderr, "사용법: --visit open|random|maze|terrain|city N\n");
        return;
    }
    size_t cells = (size_t)n * (size_t)n;
    unsigned char *mark = (unsigned char *)calloc(cells, 1);
    int *path = (int *)malloc(cells * sizeof(int));
    if (mark == NULL || path == NULL) {
        free(mark);
        free(path);
        gridFree(&g);
        return;
    }
    for (size_t a = 0; a < PATH_ALGORITHM_COUNT; a++) {
        PathStats s;
        size_t len = 0;
        memset(mark, 0, cells);
        pathVisitMark = mark;
        PATH_ALGORITHMS[a].solve(&g, 0, (int)cells - 1, &s, path, &len);
        pathVisitMark = NULL;
        for (size_t i = 0; i < len; i++) {
            mark[path[i]] = 2;
        }
        printf("# %s %ld %zu\n", PATH_ALGORITHMS[a].name, s.cost, s.expanded);
        for (int y = 0; y < n; y++) {
            for (int x = 0; x < n; x++) {
                int v = y * n + x;
                putchar(g.cost[v] == CELL_WALL ? '#' : mark[v] == 2 ? '*' : mark[v] ? 'o' : '.');
            }
            putchar('\n');
        }
    }
    free(mark);
    free(path);
    gridFree(&g);
}

static MapKind mapKindFromKey(const char *key) {
    for (int k = 0; k < MAP_KIND_COUNT; k++) {
        if (strcmp(mapKindKey((MapKind)k), key) == 0) {
            return (MapKind)k;
        }
    }
    return MAP_KIND_COUNT;
}

static void reportMap(const char *key, int n) {
    MapKind kind = mapKindFromKey(key);
    Grid g;
    if (kind == MAP_KIND_COUNT || n < 3 || !makeMap(&g, n, kind, SEED)) {
        fprintf(stderr, "사용법: --map open|random|maze|terrain|city N\n");
        return;
    }
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            int c = g.cost[y * n + x];
            putchar(c == CELL_WALL ? '#' : '0' + c);
        }
        putchar('\n');
    }
    gridFree(&g);
}

int main(int argc, char **argv) {
    if (argc > 3 && strcmp(argv[1], "--map") == 0) {
        reportMap(argv[2], atoi(argv[3]));
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--csv") == 0) {
        reportCsv();
        return 0;
    }
    if (argc > 3 && strcmp(argv[1], "--visit") == 0) {
        reportVisit(argv[2], atoi(argv[3]));
        return 0;
    }
    reportTable();
    return 0;
}
