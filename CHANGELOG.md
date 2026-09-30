# Changelog

형식은 [Keep a Changelog](https://keepachangelog.com/ko/1.1.0/)를 따른다.

## [0.1] - 2026-10-18 (중간 제출)

### Added

- 공통 인터페이스 `PathAlgorithm`과 방식 표 — BFS · Dijkstra · A*
- 측정: 확장 칸 · 힙/큐 삽입 · open 최대 · 추가 메모리 · 경로 비용, 경로 검증(`pathValid`)
- 지도 5종(빈 지도 · 무작위 벽 · 미로 · 지형 비용 · 도로망), minstd 씨앗 고정
- 실험: 지도별(n = 513), n 배가(65 ~ 2,049), 확장 칸 그림(`--visit`), 지도 출력(`--map`)
- 유닛 테스트 40개 (Bellman-Ford 식 정답 계산기와 교차 검증), `make asan`
- 그래프 도구(`tools/`), 중간 발표 슬라이드(`slides/midterm.pdf`)

### Removed

- 템플릿의 예제(버블 정렬)와 Python 구현
