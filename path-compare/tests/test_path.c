/* test_path.c - 유닛 테스트 (실행: make test)
 * PATH_ALGORITHMS에 있는 방식 전부에 같은 테스트를 돌린다 (과제 1 test_sort.c와 같은 틀) */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "grid.h"
#include "pathctx.h"

static int checks = 0;
static int failures = 0;

static void report(const char *who, const char *name, int ok) {
    checks++;
    if (ok) {
        printf("ok    %-9s %s\n", who, name);
        return;
    }
    failures++;
    printf("FAIL  %-9s %s\n", who, name);
}

/* 문자열로 지도 만들기: '#' 벽, '1'~'9' 비용 */
static int gridFromRows(Grid *g, const char *const rows[], int h) {
    int w = (int)strlen(rows[0]);
    if (!gridInit(g, w, h)) {
        return 0;
    }
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            char c = rows[y][x];
            g->cost[y * w + x] = c == '#' ? CELL_WALL : (unsigned char)(c - '0');
        }
    }
    return 1;
}

/* 정답 계산기: Dijkstra와 상관없이, 더 줄일 게 없을 때까지 모든 칸을 완화 (Bellman-Ford 식).
 * 느리지만 작은 지도에서 최소 비용을 확실히 알려 준다. 못 가면 -1 */
static long bruteMinCost(const Grid *g, int start, int goal) {
    size_t n = (size_t)g->w * (size_t)g->h;
    long *d = (long *)malloc(n * sizeof(long));
    if (d == NULL || g->cost[start] == CELL_WALL || g->cost[goal] == CELL_WALL) {
        free(d);
        return -1;
    }
    for (size_t i = 0; i < n; i++) {
        d[i] = LONG_MAX;
    }
    d[start] = 0;
    for (int changed = 1; changed;) {
        changed = 0;
        for (size_t v = 0; v < n; v++) {
            if (d[v] == LONG_MAX) {
                continue;
            }
            int x = (int)(v % (size_t)g->w), y = (int)(v / (size_t)g->w);
            static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
            for (int k = 0; k < 4; k++) {
                int nx = x + DX[k], ny = y + DY[k];
                if (nx < 0 || ny < 0 || nx >= g->w || ny >= g->h) {
                    continue;
                }
                int u = ny * g->w + nx;
                if (g->cost[u] != CELL_WALL && d[v] + g->cost[u] < d[u]) {
                    d[u] = d[v] + g->cost[u];
                    changed = 1;
                }
            }
        }
    }
    long r = d[goal] == LONG_MAX ? -1 : d[goal];
    free(d);
    return r;
}

/* 한 방식을 돌려 비용 · 경로 검증까지 */
static int solveChecked(const PathAlgorithm *a, const Grid *g, int s, int t, PathStats *st) {
    int *path = (int *)malloc((size_t)g->w * (size_t)g->h * sizeof(int));
    size_t len = 0;
    int ok = path != NULL && a->solve(g, s, t, st, path, &len) &&
             pathValid(g, s, t, path, len, st->cost) && len == st->steps + 1;
    free(path);
    return ok;
}

/* --- 손으로 푼 지도 ------------------------------------------------------ */

static void handCases(const PathAlgorithm *a) {
    Grid g;
    PathStats st;
    {
        static const char *const m[] = {"11111", "11111", "11111", "11111", "11111"};
        gridFromRows(&g, m, 5);
        report(a->name, "5x5 빈 지도: 비용 8, 8걸음",
               solveChecked(a, &g, 0, 24, &st) && st.cost == 8 && st.steps == 8);
        gridFree(&g);
    }
    {
        /* 가운데 벽에 아래쪽 틈 하나: 오른쪽 위로 가려면 돌아가야 한다 */
        static const char *const m[] = {"11#11", "11#11", "11#11", "11111"};
        gridFromRows(&g, m, 4);
        report(a->name, "벽 돌아가기: 비용 10",
               solveChecked(a, &g, 0, 4, &st) && st.cost == 10);
        gridFree(&g);
    }
    {
        static const char *const m[] = {"1#1", "1#1", "1#1"};
        gridFromRows(&g, m, 3);
        report(a->name, "막힌 지도: 못 찾음(found = 0)",
               a->solve(&g, 0, 2, &st, NULL, NULL) == 0 && st.found == 0);
        gridFree(&g);
    }
    {
        static const char *const m[] = {"111", "111"};
        size_t len = 99;
        int path[6];
        gridFromRows(&g, m, 2);
        report(a->name, "출발 = 도착: 비용 0, 경로 1칸",
               a->solve(&g, 4, 4, &st, path, &len) && st.cost == 0 && len == 1 && path[0] == 4);
        gridFree(&g);
    }
    {
        static const char *const m[] = {"#11", "111"};
        gridFromRows(&g, m, 2);
        report(a->name, "출발이 벽이면 못 찾음", a->solve(&g, 0, 5, &st, NULL, NULL) == 0);
        gridFree(&g);
    }
    {
        static const char *const m[] = {"1111111111"};
        gridFromRows(&g, m, 1);
        report(a->name, "1줄 복도: 비용 9",
               solveChecked(a, &g, 0, 9, &st) && st.cost == 9 && st.steps == 9);
        gridFree(&g);
    }
}

/* 비용이 다른 지도: 직진하면 비싼 칸(9)을 밟고, 돌아가면 싸다 */
static void weightedCase(void) {
    static const char *const m[] = {"191", "191", "111"};
    Grid g;
    PathStats b, d, s;
    gridFromRows(&g, m, 3);
    solveChecked(&PATH_ALGORITHMS[0], &g, 0, 2, &b);
    solveChecked(&PATH_ALGORITHMS[1], &g, 0, 2, &d);
    solveChecked(&PATH_ALGORITHMS[2], &g, 0, 2, &s);
    report("bfs", "비용 지도: 걸음 수 최소(2걸음)지만 비용 10", b.steps == 2 && b.cost == 10);
    report("dijkstra", "비용 지도: 돌아가서 비용 6", d.cost == 6 && d.steps == 6);
    report("astar", "비용 지도: 돌아가서 비용 6", s.cost == 6 && s.steps == 6);
    gridFree(&g);
}

/* --- 무작위 지도로 교차 검증 ---------------------------------------------- */

/* 지도 5종 x 크기 5~31 x 씨앗 여러 개:
 *   Dijkstra · A* 비용 = 정답 계산기, 경로 유효
 *   BFS: 비용 1 지도에선 정답과 같고, 비용 지도에선 정답 이상 / 걸음 수는 최소 */
static void crossCheck(void) {
    int okD = 1, okA = 1, okBUnit = 1, okBWeighted = 1, okValid = 1, maps = 0;
    for (int k = 0; k < MAP_KIND_COUNT; k++) {
        for (int n = 5; n <= 31; n += 2) {
            for (unsigned long seed = 1; seed <= 6; seed++) {
                Grid g;
                if (!makeMap(&g, n, (MapKind)k, seed * 101UL)) {
                    okValid = 0;
                    continue;
                }
                maps++;
                int t = n * n - 1;
                long best = bruteMinCost(&g, 0, t);
                PathStats b, d, a;
                okValid &= solveChecked(&PATH_ALGORITHMS[0], &g, 0, t, &b);
                okValid &= solveChecked(&PATH_ALGORITHMS[1], &g, 0, t, &d);
                okValid &= solveChecked(&PATH_ALGORITHMS[2], &g, 0, t, &a);
                okD &= d.cost == best;
                okA &= a.cost == best;
                int unit = (k == MAP_OPEN || k == MAP_RANDOM || k == MAP_MAZE);
                if (unit) {
                    okBUnit &= b.cost == best;
                } else {
                    okBWeighted &= b.cost >= best && b.steps <= d.steps && b.steps <= a.steps;
                }
                gridFree(&g);
            }
        }
    }
    char name[96];
    snprintf(name, sizeof(name), "지도 %d개에서 경로가 모두 유효", maps);
    report("all", name, okValid);
    report("dijkstra", "비용 = 정답 계산기(Bellman-Ford 식)", okD);
    report("astar", "비용 = 정답 계산기 (휴리스틱이 과대평가 안 함)", okA);
    report("bfs", "비용 1 지도에서는 최적", okBUnit);
    report("bfs", "비용 지도에서는 비용 ≥ 최적, 걸음 수 ≤ 다른 방식", okBWeighted);
}

/* --- 방식별 성질 ---------------------------------------------------------- */

static void properties(void) {
    Grid g;
    PathStats st;

    /* 빈 지도에서 A*는 f가 같을 때 g가 큰 쪽을 먼저 꺼내므로 대각선 한 줄만 판다 */
    makeMap(&g, 33, MAP_OPEN, 1UL);
    astarSolve(&g, 0, 33 * 33 - 1, &st, NULL, NULL);
    report("astar", "빈 지도 33x33: 확장 칸 = 경로 칸 수(65)", st.expanded == 65);
    bfsSolve(&g, 0, 33 * 33 - 1, &st, NULL, NULL);
    report("bfs", "추가 메모리 = 칸마다 int 2개", st.extraBytes == 2 * 33 * 33 * sizeof(int));
    gridFree(&g);

    /* 미로는 길이 하나뿐인 나무: 열린 칸 = 방 r^2 + 뚫은 벽 (r^2 - 1) */
    int n = 41, r = (n + 1) / 2, open = 0;
    makeMap(&g, n, MAP_MAZE, 7UL);
    for (int i = 0; i < n * n; i++) {
        open += g.cost[i] != CELL_WALL;
    }
    report("bench", "미로 41x41: 열린 칸 = 2r^2 - 1 (나무 모양)", open == 2 * r * r - 1);
    /* 출발에서 닿는 칸을 직접 세어 본다 (탐색 코드와 따로 짠 스택 flood fill) */
    unsigned char *seen = (unsigned char *)calloc((size_t)n * n, 1);
    int *stack = (int *)malloc((size_t)n * n * sizeof(int));
    int top = 0, reached = 0;
    if (seen != NULL && stack != NULL) {
        stack[top++] = 0;
        seen[0] = 1;
        while (top > 0) {
            int v = stack[--top];
            reached++;
            for (int d = 0; d < 4; d++) {
                int u = gridNeighbor(&g, v, d);
                if (u >= 0 && !seen[u]) {
                    seen[u] = 1;
                    stack[top++] = u;
                }
            }
        }
    }
    report("bench", "미로 41x41: 열린 칸이 전부 출발에서 닿는다", reached == open);
    free(seen);
    free(stack);
    gridFree(&g);

    Grid g2;
    makeMap(&g, 65, MAP_RANDOM, 3UL);
    makeMap(&g2, 65, MAP_RANDOM, 3UL);
    report("bench", "같은 씨앗이면 같은 지도", memcmp(g.cost, g2.cost, 65 * 65) == 0);
    int walls = 0;
    for (int i = 0; i < 65 * 65; i++) {
        walls += g.cost[i] == CELL_WALL;
    }
    report("bench", "무작위 벽 비율 25~35%", walls > 65 * 65 / 4 && walls < 65 * 65 * 35 / 100);
    report("bench", "무작위 벽 지도는 출발-도착이 이어져 있다",
           bfsSolve(&g, 0, 65 * 65 - 1, NULL, NULL, NULL));
    gridFree(&g);
    gridFree(&g2);

    makeMap(&g, 65, MAP_TERRAIN, 5UL);
    int lo = 9, hi = 0;
    for (int i = 0; i < 65 * 65; i++) {
        lo = g.cost[i] < lo ? g.cost[i] : lo;
        hi = g.cost[i] > hi ? g.cost[i] : hi;
    }
    report("bench", "지형 비용은 1~9", lo == 1 && hi == 9 && gridMinCost(&g) == 1);
    gridFree(&g);
}

static void tools(void) {
    static const char *const m[] = {"111", "1#1", "111"};
    Grid g;
    gridFromRows(&g, m, 3);
    int good[] = {0, 1, 2, 5, 8};
    int jump[] = {0, 2, 5, 8};      /* 한 칸 건너뜀 */
    int wall[] = {0, 1, 4, 7, 8};   /* 벽(4)을 지남 */
    report("tools", "pathValid: 올바른 경로 통과", pathValid(&g, 0, 8, good, 5, 4));
    report("tools", "pathValid: 비용이 틀리면 거절", !pathValid(&g, 0, 8, good, 5, 5));
    report("tools", "pathValid: 건너뛰면 거절", !pathValid(&g, 0, 8, jump, 4, 3));
    report("tools", "pathValid: 벽을 지나면 거절", !pathValid(&g, 0, 8, wall, 5, 4));
    gridFree(&g);

    /* 힙: 무작위로 넣고 꺼내면 f 오름차순, f가 같으면 g 내림차순 */
    Heap h;
    Minstd rng;
    int heapOk = heapInit(&h, 4); /* 일부러 작게 잡아 늘어나는 것도 확인 */
    minstdSeed(&rng, 11UL);
    for (int i = 0; heapOk && i < 5000; i++) {
        long f = (long)(minstdNext(&rng) % 100);
        heapOk = heapPush(&h, (HeapItem){f, (long)(minstdNext(&rng) % 50), i});
    }
    HeapItem prev = heapPop(&h);
    int count = 1;
    while (heapOk && h.size > 0) {
        HeapItem it = heapPop(&h);
        heapOk = it.f > prev.f || (it.f == prev.f && it.g <= prev.g);
        prev = it;
        count++;
    }
    report("tools", "힙 5000개: 순서대로 나오고 개수가 맞다", heapOk && count == 5000);
    heapFree(&h);

    minstdSeed(&rng, 1UL);
    unsigned long x1 = minstdNext(&rng), x2 = minstdNext(&rng), x3 = minstdNext(&rng);
    report("tools", "minstd(1) = 16807 282475249 1622650073",
           x1 == 16807UL && x2 == 282475249UL && x3 == 1622650073UL);
}

int main(void) {
    for (size_t k = 0; k < PATH_ALGORITHM_COUNT; k++) {
        handCases(&PATH_ALGORITHMS[k]);
        printf("\n");
    }
    weightedCase();
    crossCheck();
    properties();
    tools();
    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
