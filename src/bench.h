/* bench.h - 지도 만들기, 시간 재기 */
#ifndef BENCH_H
#define BENCH_H

#include <stddef.h>

#include "grid.h"

typedef enum MapKind {
    MAP_OPEN,    /* 벽 없음, 비용 전부 1 */
    MAP_RANDOM,  /* 칸마다 30% 확률로 벽, 나머지 비용 1 */
    MAP_MAZE,    /* 재귀 백트래킹 미로 (길이 하나뿐), 비용 1 */
    MAP_TERRAIN, /* 벽 없음, 칸 비용 1~9 무작위 (진흙·풀밭 같은 지형) */
    MAP_CITY,    /* 벽 없음, 32칸마다 간선도로(1), 8칸마다 골목(3), 나머지 비포장(7) */
    MAP_KIND_COUNT
} MapKind;

const char *mapKindName(MapKind kind); /* 표 출력용 */
const char *mapKindKey(MapKind kind);  /* csv용 */

/* n x n 지도를 만든다. 출발 = 왼쪽 위(0), 도착 = 오른쪽 아래(n*n - 1).
 * 미로는 n이 홀수여야 모든 칸이 이어진다. MAP_RANDOM은 출발-도착이 이어질 때까지
 * 씨앗을 1씩 늘려 다시 만든다. seed가 같으면 항상 같은 지도. 실패하면 0 */
int makeMap(Grid *g, int n, MapKind kind, unsigned long seed);

typedef struct BenchResult {
    const PathAlgorithm *algo;
    double millis;   /* reps회 평균 */
    PathStats stats; /* 마지막 회차 (결정적이라 회차마다 같다) */
    int valid;       /* 경로가 pathValid를 통과했나 */
} BenchResult;

BenchResult benchRun(const PathAlgorithm *algo, const Grid *g, int start, int goal, int reps);

#endif
