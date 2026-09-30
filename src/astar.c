/* astar.c - A* 탐색 (수업에서 안 배운 방식, 새로 공부함)
 *
 * Dijkstra에 "목표까지 적어도 이만큼은 남았다"는 추정 h를 더해 목표 쪽 칸을 먼저 꺼낸다.
 * h(v) = (|x - gx| + |y - gy|) x (지도에서 가장 싼 칸 비용)
 *   - 4방향 이동이라 맨해튼 거리보다 적게 움직여서는 못 간다
 *   - 한 걸음에 적어도 최소 비용은 든다
 *   => 실제 남은 비용 이하(과대평가 안 함, admissible)이고, 한 칸 옮길 때 h가 최대 최소 비용만큼만
 *      줄어드므로 consistent. 그래서 Dijkstra처럼 한 번 꺼낸 칸은 다시 안 봐도 최적이다.
 * 몸통은 dijkstra.c의 bestFirstSearch와 같고 hUnit만 다르다. */
#include "grid.h"
#include "pathctx.h"

int astarSolve(const Grid *g, int start, int goal, PathStats *stats, int *path, size_t *pathLen) {
    return bestFirstSearch(g, start, goal, gridMinCost(g), stats, path, pathLen);
}
