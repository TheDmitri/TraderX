#!/usr/bin/env python3
"""Text widgets clip their text to their own box in game: a box shorter than the
font's cell height cuts the glyphs. Reports every text widget (px height) whose
box is shorter than the cell of its exact-em Nasdara font (dz-font-nh|nb|nbb|nm-N).
  scripts/lint-text-cells.py [--fix]
--fix grows the box to the cell height, keeping its centre (vc), bottom (vb) or top."""
import re, sys, glob, os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")

# Cell heights of the baked exact-em fonts (tallest glyph cell), measured from the .fnt
# files baked by dayz-tools/fontbake (re-measure after re-baking).
CELL = {
    "nh":  {13:19,14:22,15:22,16:23,17:24,18:25,19:27,20:29,22:32,24:34,26:37,29:42,31:45,36:52,41:57,48:69,86:122},
    "nb":  {13:15,14:15,15:17,16:17,17:19,18:19,19:21,20:21,22:23,24:26,26:28,29:32,31:33,36:37,41:44,48:51,86:90},
    "nbb": {13:15,14:15,15:16,16:16,17:20,18:20,19:22,20:22,22:23,24:26,26:27,29:31,31:33,36:38,41:43,48:51,86:90},
    "nm":  {13:16,14:18,15:19,16:20,17:21,18:22,19:24,20:25,22:26,24:29,26:31,29:35,31:37,36:43,41:49,48:57,86:102},
}

def cell(fam, size):
    return CELL[fam][size]

fix = "--fix" in sys.argv
bad = 0
for path in sorted(glob.glob(os.path.join(ROOT, "*", "datasets", "**", "*.html"), recursive=True)):
    if ".bundle." in path or "_fixtures" in path:
        continue
    src = open(path, encoding="utf-8").read()
    out = src
    for m in re.finditer(r'<(span|div)\b[^>]*data-dz="(text|richtext)"[^>]*>', src, re.S):
        tag = m.group(0)
        cls = re.search(r'class="([^"]*)"', tag)
        if not cls:
            continue
        c = cls.group(1)
        f = re.search(r"dz-font-(nh|nb|nbb|nm)-(\d+)", c)
        wh = re.search(r"dz-wh-([\d.]+(?:px)?)-(\d+)px", c)
        if not f or not wh or "dz-wrap" in c:
            continue
        need = cell(f.group(1), int(f.group(2)))
        h = int(wh.group(2))
        if h >= need:
            continue
        bad += 1
        name = re.search(r'data-name="([^"]*)"', tag)
        print(f"{os.path.relpath(path, ROOT)}: {name.group(1) if name else '?'} h={h} < cell {need} ({f.group(0)})")
        if fix:
            nc = c.replace(wh.group(0), f"dz-wh-{wh.group(1)}-{need}px")
            xy = re.search(r"dz-xy-([\d.]+(?:px)?)-(-?\d+)px", nc)
            grow = need - h
            if xy:
                y = int(xy.group(2))
                if "dz-text-vb" in nc or "dz-align-b" in nc:
                    ny = y if "dz-align-b" in nc else y - grow
                elif "dz-text-vc" in nc or "dz-align-y-center" in nc or "dz-align-c" in nc:
                    ny = y if ("dz-align-y-center" in nc or "dz-align-cc" in nc or "dz-align-cl" in nc or "dz-align-cr" in nc) else y - grow // 2
                else:
                    ny = y
                nc = nc.replace(xy.group(0), f"dz-xy-{xy.group(1)}-{ny}px")
            out = out.replace(tag, tag.replace(f'class="{c}"', f'class="{nc}"'), 1)
    if fix and out != src:
        open(path, "w", encoding="utf-8").write(out)
print(f"{bad} text box(es) shorter than their font cell")
