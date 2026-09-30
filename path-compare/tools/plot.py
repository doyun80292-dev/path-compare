"""plot.py - 측정 결과(csv)와 그림(svg, png)을 report/ 에 만든다.

    make charts          # 또는
    python3 tools/plot.py

main.out --csv, --visit 출력을 읽어서 씀. 표준 모듈만 사용 (PNG도 zlib로 직접 씀).
과제 1 저장소의 tools/plot.py 틀을 가져와 경로 탐색용으로 고쳐 씀.
"""

import csv
import io
import struct
import subprocess
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import svgchart  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "src" / "main.out"
OUT = ROOT / "report"
ALGOS = ["bfs", "dijkstra", "astar"]
MAPS = ["open", "random", "maze", "terrain", "city"]
MAP_LABEL = {"open": "빈 지도", "random": "무작위 벽", "maze": "미로",
             "terrain": "지형 비용", "city": "도로망"}


def run(args):
    if not BINARY.exists():
        subprocess.run(["make", "src/main.out"], cwd=ROOT, check=True)
    return subprocess.run([str(BINARY), *args], cwd=ROOT, check=True,
                          capture_output=True, text=True).stdout


def numeric(rows):
    for r in rows:
        for k, v in r.items():
            try:
                r[k] = int(v)
            except ValueError:
                try:
                    r[k] = float(v)
                except ValueError:
                    pass
    return rows


# --- PNG (표준 모듈만) -------------------------------------------------------

def write_png(path, pixels, scale=1):
    """pixels: 행 목록, 각 행은 (r, g, b) 목록. scale배로 키워서 저장."""
    h, w = len(pixels), len(pixels[0])
    raw = bytearray()
    for row in pixels:
        line = bytearray()
        for (r, g, b) in row:
            line += bytes((r, g, b)) * scale
        for _ in range(scale):
            raw += b"\x00" + line
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    Path(path).write_bytes(png)


def hex_rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


WALL = hex_rgb("#3b4048")
BLANK = hex_rgb("#fbfbf9")
PATH_INK = hex_rgb("#d1242f")
SEEN = {"bfs": hex_rgb("#a9cce3"), "dijkstra": hex_rgb("#f5d38a"), "astar": hex_rgb("#9fd8c2")}
COST_SHADE = {c: hex_rgb(svgchart.BG) if c == 1 else
              tuple(int(251 - (251 - 150) * (c - 1) / 8) for _ in range(3)) for c in range(1, 10)}


def visit_pictures(kind, n, scale):
    """세 방식이 확장한 칸을 그린다. 반환: {algo: (cost, expanded)}"""
    text = run(["--visit", kind, str(n)])
    info, blocks, cur = {}, {}, None
    for line in text.splitlines():
        if line.startswith("# "):
            _, name, cost, expanded = line.split()
            info[name] = (int(cost), int(expanded))
            cur = blocks.setdefault(name, [])
        else:
            cur.append(line)
    costs = run(["--map", kind, str(n)]).splitlines()
    for name, rows in blocks.items():
        pix = []
        for row, crow in zip(rows, costs):
            line = []
            for c, cc in zip(row, crow):
                if c == "#":
                    line.append(WALL)
                elif c == "*":
                    line.append(PATH_INK)
                else:
                    base = COST_SHADE[int(cc)]
                    # 확장한 칸은 방식 색을 비용 음영에 곱해서 칠한다 (비싼 칸일수록 진하게)
                    line.append(tuple(b * s // 255 for b, s in zip(base, SEEN[name]))
                                if c == "o" else base)
            pix.append(line)
        write_png(OUT / f"visit-{kind}-{name}.png", pix, scale)
    return info


def map_thumbnails(n, scale):
    """지도 5종 미리보기. 벽은 진한 회색, 비용은 진하기로 (1 = 밝음, 9 = 진함)."""
    for kind in MAPS:
        rows = run(["--map", kind, str(n)]).splitlines()
        pix = [[WALL if c == "#" else COST_SHADE[int(c)] for c in row] for row in rows]
        write_png(OUT / f"map-{kind}.png", pix, scale)


def main():
    OUT.mkdir(exist_ok=True)
    made = []

    rows = numeric(list(csv.DictReader(io.StringIO(run(["--csv"])))))
    with open(OUT / "results.csv", "w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    kinds = [r for r in rows if r["scope"] == "kinds"]
    growth = [r for r in rows if r["scope"] == "growth"]

    def pick(rs, algo, **kw):
        return next(r for r in rs if r["algo"] == algo and all(r[k] == v for k, v in kw.items()))

    # 1. 지도별 확장 칸 비율 (n = 513)
    ratio = {a: [pick(kinds, a, map=m)["expanded"] / (513 * 513) * 100 for m in MAPS] for a in ALGOS}
    made.append(svgchart.grouped_bar_chart(
        OUT / "kinds-expanded.svg", "지도별 확장한 칸 (전체 칸 대비 %)",
        "n = 513 (26만 칸) · 로그 축 · 낮을수록 덜 뒤짐",
        [MAP_LABEL[m] for m in MAPS], ratio, "확장 칸 (%)", log_scale=True,
        value_label=lambda v: f"{v:.1f}" if v >= 1 else f"{v:.2f}"))

    # 2. 지도별 시간
    made.append(svgchart.grouped_bar_chart(
        OUT / "kinds-time.svg", "지도별 걸린 시간",
        "n = 513 · 5회 평균 · 로그 축",
        [MAP_LABEL[m] for m in MAPS],
        {a: [pick(kinds, a, map=m)["millis"] for m in MAPS] for a in ALGOS},
        "시간 (ms)", log_scale=True, value_label=svgchart.ms))

    # 3. BFS 경로 비용 / 최소 비용
    sizes = sorted({r["n"] for r in growth})
    made.append(svgchart.line_chart(
        OUT / "bfs-cost-ratio.svg", "BFS가 찾은 경로 비용 ÷ 최소 비용",
        "비용이 칸마다 다른 지도에서만 1보다 크다 · Dijkstra · A*는 전부 1.00",
        sizes, {MAP_LABEL[m]: [pick(growth, "bfs", map=m, n=n)["cost"] /
                               pick(growth, "bfs", map=m, n=n)["optimalCost"] for n in sizes]
                for m in ("terrain", "city")},
        "n (한 변 칸 수)", "비용 비", log_axes=False, log_x=True,
        x_label=lambda v: f"{v}", y_label=lambda v: f"{v:.0f}"))

    # 4. n이 커질 때 시간 (무작위 벽, 도로망)
    for m in ("random", "city"):
        made.append(svgchart.line_chart(
            OUT / f"growth-time-{m}.svg", f"n이 커질 때 걸린 시간 — {MAP_LABEL[m]}",
            "로그-로그 · x = 칸 수(n²) · 기울기 1이면 칸 수에 비례",
            [n * n for n in sizes], {a: [pick(growth, a, map=m, n=n)["millis"] for n in sizes]
                                     for a in ALGOS},
            "칸 수 n²", "시간 (ms)", x_label=svgchart.si))

    # 5. 탐색 영역 그림
    info = {k: visit_pictures(k, 129, 3) for k in ("random", "city", "maze")}
    with open(OUT / "visit.csv", "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["map", "n", "algo", "cost", "expanded"])
        for k, per in info.items():
            for a, (c, e) in per.items():
                w.writerow([k, 129, a, c, e])

    # 6. 지도 미리보기 (65 x 65)
    map_thumbnails(65, 3)

    for p in made:
        print(f"wrote {Path(p).relative_to(ROOT)}")
    print("wrote report/results.csv, report/visit.csv, report/visit-*.png, report/map-*.png")


if __name__ == "__main__":
    main()
