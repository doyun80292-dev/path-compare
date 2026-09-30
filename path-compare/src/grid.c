/* grid.c - 지도, 공통 도구, 힙, 탐색 목록(PATH_ALGORITHMS) */
#include "grid.h"

#include <stdlib.h>

#include "pathctx.h"

/* --- 지도 ------------------------------------------------------------- */

int gridInit(Grid *g, int w, int h) {
    g->w = w;
    g->h = h;
    g->cost = NULL;
    if (w <= 0 || h <= 0) {
        return 0;
    }
    g->cost = (unsigned char *)malloc((size_t)w * (size_t)h);
    if (g->cost == NULL) {
        return 0;
    }
    for (size_t i = 0; i < (size_t)w * (size_t)h; i++) {
        g->cost[i] = 1;
    }
    return 1;
}

void gridFree(Grid *g) {
    free(g->cost);
    g->cost = NULL;
}

int gridMinCost(const Grid *g) {
    int best = CELL_MAX_COST;
    for (size_t i = 0; i < (size_t)g->w * (size_t)g->h; i++) {
        if (g->cost[i] != CELL_WALL && g->cost[i] < best) {
            best = g->cost[i];
        }
    }
    return best;
}

const int DIR_X[4] = {1, 0, -1, 0};
const int DIR_Y[4] = {0, 1, 0, -1};

int gridNeighbor(const Grid *g, int v, int d) {
    int x = v % g->w + DIR_X[d];
    int y = v / g->w + DIR_Y[d];
    if (x < 0 || y < 0 || x >= g->w || y >= g->h) {
        return -1;
    }
    int u = y * g->w + x;
    return g->cost[u] == CELL_WALL ? -1 : u;
}

unsigned char *pathVisitMark = NULL;

/* --- 측정값 · 경로 ------------------------------------------------------ */

void pathStatsReset(PathStats *s) {
    if (s == NULL) {
        return;
    }
    s->expanded = 0;
    s->pushes = 0;
    s->peakOpen = 0;
    s->extraBytes = 0;
    s->cost = -1;
    s->steps = 0;
    s->found = 0;
}

void pathFinish(const Grid *g, const int *parent, int start, int goal, PathStats *stats,
                int *path, size_t *pathLen) {
    long cost = 0;
    size_t steps = 0;
    for (int v = goal; v != start; v = parent[v]) {
        cost += g->cost[v];
        steps++;
    }
    stats->cost = cost;
    stats->steps = steps;
    stats->found = 1;
    if (path != NULL) {
        /* 거꾸로 따라왔으니 뒤에서부터 채운다 */
        size_t i = steps;
        for (int v = goal; ; v = parent[v]) {
            path[i] = v;
            if (v == start) {
                break;
            }
            i--;
        }
    }
    if (pathLen != NULL) {
        *pathLen = steps + 1;
    }
}

int pathValid(const Grid *g, int start, int goal, const int *path, size_t len, long cost) {
    if (len == 0 || path[0] != start || path[len - 1] != goal) {
        return 0;
    }
    long sum = 0;
    for (size_t i = 1; i < len; i++) {
        int a = path[i - 1], b = path[i];
        int dx = abs(a % g->w - b % g->w);
        int dy = abs(a / g->w - b / g->w);
        if (dx + dy != 1 || g->cost[b] == CELL_WALL) {
            return 0;
        }
        sum += g->cost[b];
    }
    return sum == cost;
}

/* --- 힙 ---------------------------------------------------------------- */

/* a가 b보다 먼저 나와야 하면 1 */
static int heapLess(const HeapItem *a, const HeapItem *b) {
    if (a->f != b->f) {
        return a->f < b->f;
    }
    if (a->g != b->g) {
        return a->g > b->g; /* 같은 f면 이미 많이 온 쪽(목표에 가까운 쪽) 먼저 */
    }
    return a->v < b->v; /* 끝까지 같으면 칸 번호로. 결과가 항상 같게 */
}

int heapInit(Heap *h, size_t cap) {
    h->size = 0;
    h->cap = cap < 16 ? 16 : cap;
    h->items = (HeapItem *)malloc(h->cap * sizeof(HeapItem));
    return h->items != NULL;
}

void heapFree(Heap *h) {
    free(h->items);
    h->items = NULL;
    h->size = h->cap = 0;
}

int heapPush(Heap *h, HeapItem it) {
    if (h->size == h->cap) {
        HeapItem *bigger = (HeapItem *)realloc(h->items, 2 * h->cap * sizeof(HeapItem));
        if (bigger == NULL) {
            return 0;
        }
        h->items = bigger;
        h->cap *= 2;
    }
    size_t i = h->size++;
    while (i > 0) { /* siftUp */
        size_t p = (i - 1) / 2;
        if (!heapLess(&it, &h->items[p])) {
            break;
        }
        h->items[i] = h->items[p];
        i = p;
    }
    h->items[i] = it;
    return 1;
}

HeapItem heapPop(Heap *h) {
    HeapItem top = h->items[0];
    HeapItem last = h->items[--h->size];
    size_t i = 0;
    while (1) { /* siftDown: 과제 1 heapSort.c와 같은 모양, 부등호만 반대 */
        size_t c = 2 * i + 1;
        if (c >= h->size) {
            break;
        }
        if (c + 1 < h->size && heapLess(&h->items[c + 1], &h->items[c])) {
            c++;
        }
        if (!heapLess(&h->items[c], &last)) {
            break;
        }
        h->items[i] = h->items[c];
        i = c;
    }
    if (h->size > 0) {
        h->items[i] = last;
    }
    return top;
}

/* --- 난수 --------------------------------------------------------------- */

void minstdSeed(Minstd *m, unsigned long seed) {
    seed %= 2147483647UL;
    m->state = seed == 0 ? 1 : seed;
}

unsigned long minstdNext(Minstd *m) {
    m->state = (unsigned long)((unsigned long long)m->state * 16807ULL % 2147483647ULL);
    return m->state;
}

/* --- 탐색 목록 (추가하면 여기 한 줄 + Makefile의 PATH_SRC) ------------------ */

const PathAlgorithm PATH_ALGORITHMS[] = {
    {"bfs",      "O(V + E)",       "모든 칸 비용이 같을 때", bfsSolve},
    {"dijkstra", "O((V + E) log V)", "비용이 음수가 아니면 항상", dijkstraSolve},
    {"astar",    "O((V + E) log V)", "휴리스틱이 과대평가하지 않으면", astarSolve},
};

const size_t PATH_ALGORITHM_COUNT = sizeof(PATH_ALGORITHMS) / sizeof(PATH_ALGORITHMS[0]);
