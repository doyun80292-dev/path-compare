# CLAUDE.md

고급알고리즘 개인프로젝트 (경로 탐색 비교: BFS · Dijkstra · A*) 저장소. AI 도구로 이 저장소를 다룰 때 참고할 규칙.

## 구조

```plaintext
src/    grid.h · pathctx.h · grid.c · bfs.c · dijkstra.c · astar.c · bench.* · main.c
tests/  test_path.c
tools/  plot.py · svgchart.py
report/ 측정값(csv) · 그래프(svg) · 그림(png)
slides/ 중간 발표 PDF
```

## 규칙

- 외부 라이브러리 안 씀. C는 표준 라이브러리만, Python은 표준 모듈만.
- 실행 파일은 `*.out`으로 만든다 (`.gitignore`가 걸러냄).
- `-Wall -Wextra` 경고 없이 빌드돼야 한다.
- 커밋 전에 `make test` 통과 확인.
- 방식을 추가하면 `src/grid.c`의 `PATH_ALGORITHMS`와 `Makefile`의 `PATH_SRC`에 같이 추가.
- 세 방식의 비교 조건(이웃 순서, "도착 칸을 꺼낼 때 종료")을 바꾸지 않는다.
- 발표 · 보고서 숫자는 `make charts`로 만든 `report/*.csv`와 맞아야 한다.

## 실행

```sh
docker compose up -d
docker compose exec lab bash
make test
make run
```
