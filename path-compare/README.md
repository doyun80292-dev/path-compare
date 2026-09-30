# 경로 탐색 비교 — BFS · Dijkstra · A*

2026-2 **고급알고리즘**(SIT2001-01) 개인프로젝트. 칸마다 비용이 있는 n × n 격자 지도에서
왼쪽 위 → 오른쪽 아래로 가는 **비용이 가장 작은 경로**를 세 방식으로 찾고,
같은 지도 · 같은 계측기로 속도 · 메모리 · 경로 품질을 비교한다.
[`algorithm-env`](https://github.com/lec-algorithm/algorithm-env) 템플릿에서 시작했고,
과제 1(`doyun80292-dev/sort-compare-hw1`)의 구조를 가져왔다.

- 중간 발표: [`slides/midterm.pdf`](slides/midterm.pdf)
- 측정값 원본: `report/results.csv`, `report/visit.csv` · 그림: `report/*.svg`, `report/*.png`

## 돌려보기

Codespaces(**Code → Codespaces → Create codespace**) 또는 로컬 컨테이너
(`docker compose up -d && docker compose exec lab bash`) 안에서:

| 명령 | 하는 일 |
| --- | --- |
| `make test` | 유닛 테스트 40개 (`40 checks, 0 failures`가 나와야 한다) |
| `make run` | n = 257, 지도 5종 비교 표 |
| `make charts` | 모든 실험을 다시 재서 `report/`의 CSV · SVG · PNG를 새로 쓴다 (약 1분) |
| `make asan` | AddressSanitizer · UBSan을 붙여 테스트 |
| `make clean` | 빌드 산출물 정리 |

직접 볼 수도 있다:

```sh
./src/main.out --csv                 # 지도 모양별(n = 513) + n 배가(65 ~ 2049)
./src/main.out --visit random 129    # 세 방식이 확장한 칸 (# 벽, o 확장, * 경로)
./src/main.out --map city 65         # 지도 칸 비용 (# 벽, 1~9 비용)
```

지도는 minstd 난수(씨앗 20261018)로 만들므로 **확장 칸 · 삽입 수 · 경로 비용은 어느 기계에서나 같다.**
시간(ms)만 기계마다 다르다. 입력 데이터 파일은 따로 없고 프로그램이 매번 같은 지도를 만든다.

## 지도 5종

| 이름 | 내용 | BFS가 최적인가 |
| --- | --- | --- |
| `open` | 벽 없음, 비용 1 | 예 |
| `random` | 칸마다 30% 벽, 비용 1 (출발-도착이 이어지게 씨앗 조정) | 예 |
| `maze` | 재귀 백트래킹 미로 (길이 하나뿐), 비용 1 | 예 |
| `terrain` | 벽 없음, 비용 1~9 무작위 | 아니오 |
| `city` | 32칸마다 간선도로(1), 8칸마다 골목(3), 나머지 비포장(7) | 아니오 |

## 구조

```plaintext
src/
  grid.h · pathctx.h · grid.c    공통 인터페이스(PathAlgorithm) · 힙 · 경로 검증 · 방식 표
  bfs.c                          BFS (큐)
  dijkstra.c                     Dijkstra · A* 공통 몸통(bestFirstSearch) + Dijkstra
  astar.c                        A* (h = 맨해튼 거리 × 최소 칸 비용)
  bench.h · bench.c              지도 만들기 · 시간 재기
  main.c                         비교 표와 실험별 CSV
tests/test_path.c                유닛 테스트 (표준 C만, 정답 계산기와 교차 검증)
tools/plot.py · svgchart.py      CSV → SVG 그래프, PNG 그림 (표준 모듈만)
report/                          측정값 · 그림
slides/                          중간 발표 PDF
```

외부 라이브러리를 쓰지 않는다. 실행 파일은 `*.out`으로 만들어 `.gitignore`가 걸러낸다.

## 출처

- 저장소 뼈대(`Dockerfile`, `compose.yml`, `.devcontainer/`, `.vscode/`, `.gitignore`): `lec-algorithm/algorithm-env` 템플릿
- `tools/svgchart.py`: 과제 1 저장소의 것 (원래 `lec-algorithm/hw1-sample-2026`에서 가져와 고친 것). 계열 이름만 바꿈
- 힙의 siftDown: 과제 1 `heapSort.c`를 최소 힙으로 뒤집어 씀
- minstd 난수: 수업 topic-04
- A*: Hart, Nilsson, Raphael, "A Formal Basis for the Heuristic Determination of Minimum Cost Paths" (1968)
- AI: 코드 · 테스트 작성, 실험 실행, 발표 자료 작성에 Claude(Anthropic)를 사용함. 내역은 최종 보고서에 첨부
