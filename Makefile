# 빌드와 테스트를 한 단어로 돌리기 위한 Makefile. (과제 1 저장소의 것을 고쳐 씀)
# 컨테이너 안에서 실행한다 (docker compose exec lab bash).
#
#   make run     비교 표 출력
#   make test    유닛 테스트
#   make charts  측정 → report/ 아래에 CSV와 그림(SVG, PNG)을 다시 만든다
#   make asan    AddressSanitizer · UBSan을 붙여 테스트
#   make debug   디버그 심볼을 넣어 빌드 (VS Code의 F5가 쓴다)
#   make clean   빌드 산출물 정리
#
# 실행 파일은 `*.out`으로 만든다. .gitignore가 그것만 걸러낸다.

CC ?= gcc
CFLAGS ?= -std=c17 -Wall -Wextra -O2
DEBUGFLAGS ?= -std=c17 -Wall -Wextra -g -O0

.PHONY: all run run-c test test-c charts asan debug clean

all: test

run: run-c

run-c: src/main.out
	@./src/main.out

test: test-c

test-c: tests/test_path.out
	@./tests/test_path.out

# 그래프는 표준 모듈만 쓰는 tools/plot.py가 SVG · PNG로 직접 찍는다 (외부 라이브러리 없음).
charts: src/main.out
	@python3 tools/plot.py

debug: src/main.debug.out

# 파일 하나를 그 자리에서 빌드한다. 같은 폴더의 .c를 함께 링크한다.
# **한 폴더에 main은 하나만** 둔다.
%.out: %.c
	$(CC) $(CFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c)

%.debug.out: %.c
	$(CC) $(DEBUGFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c)

# 탐색 구현이 파일마다 하나씩이라 여기에도 나열한다. 새 방식을 넣으면 이 줄도 본다.
PATH_SRC = src/grid.c src/bfs.c src/dijkstra.c src/astar.c src/bench.c
PATH_HDR = src/grid.h src/pathctx.h src/bench.h

src/main.out: src/main.c $(PATH_SRC) $(PATH_HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ src/main.c $(PATH_SRC)

tests/test_path.out: tests/test_path.c $(PATH_SRC) $(PATH_HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_path.c $(PATH_SRC)

tests/test_path.asan.out: tests/test_path.c $(PATH_SRC) $(PATH_HDR)
	$(CC) -std=c17 -Wall -Wextra -g -O1 -fsanitize=address,undefined \
	    -fno-sanitize-recover=all -Isrc -o $@ tests/test_path.c $(PATH_SRC)

asan: tests/test_path.asan.out
	@./tests/test_path.asan.out

clean:
	rm -f src/*.out tests/*.out
