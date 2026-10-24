#!/usr/bin/env python3
"""Make compiled .layout files follow the screen width.

The DZML sources are authored 1:1 in pixels on a 1920x1080 design. In game, exact-pixel
geometry is scaled by the screen HEIGHT (like the text), so on a screen that is not 16:9
(16:10 = 1728 reference px wide) fixed-width columns overflow. This pass rewrites the
HORIZONTAL geometry of every compiled layout into fractions of the parent's design width:

  - widgets at least MIN_W px wide get a relative width (hexactsize 0);
  - their left/right offset becomes relative too (hexactpos 0) when it is not a small
    edge inset (< MIN_X px, e.g. a 24 px margin from a right-aligned edge);
  - heights, vertical positions, icons, square buttons and children of Grid/Wrap
    spacers (laid out by the spacer) stay in pixels.

Run after `dzml compile` (scripts/build-layouts.sh calls it).
  scripts/fluid-layouts.py <file.layout>...
"""
import os
import re
import sys

MIN_W = 100   # px: narrower widgets (icons, square buttons) keep a fixed width
MIN_X = 60    # px: smaller offsets are edge insets and stay fixed
SPACERS = ("GridSpacerWidgetClass", "WrapSpacerWidgetClass", "SpacerWidgetClass")


# Components whose <body> keeps the 1920 default although they render inside a narrower
# host: their real design width (the host's width in the 1920 design).
DESIGN_WIDTH = {
    "CheckoutCardView": 456,        # cart rows
    "SearchBarView": 264,           # sidebar search field
    "ItemCardSmallView": 1094,      # list rows of a category section (body says it too)
}
# Relative-root components hosted in a container of unknown design width: left in pixels.
SKIP = {
    "AlertRowView", "VaultActivityRowView", "VaultItemCard", "VaultItemRowView", "VaultOwnerRowView",
    "ComboBoxPrefab", "PaymentResumeView", "PanelShadowView", "CheckoutPresetOptionView",
    "CategoryCardSmallView", "CategoryCardSmallVariantView", "CategoryCardMediumView", "NavBtnTooltip",
    "BeansPromoView",   # fixed 1360x800 card: screenshots keep their 16:10 ratio
}


def design_width(layout_path):
    """Design width of the layout's root from its DZML source (<body style="width:..px">)."""
    base = os.path.splitext(os.path.basename(layout_path))[0]
    if base in DESIGN_WIDTH:
        return float(DESIGN_WIDTH[base])
    html = os.path.splitext(layout_path)[0] + ".html"
    if os.path.exists(html):
        m = re.search(r'<body[^>]*width:\s*(\d+)px', open(html, encoding="utf-8").read())
        if m:
            return float(m.group(1))
    return 1920.0


def fmt(v):
    s = f"{v:.5f}".rstrip("0").rstrip(".")
    return s if s else "0"


class Node:
    def __init__(self, cls, start):
        self.cls = cls
        self.start = start          # line index of "XxxClass name {"
        self.props = {}             # key -> line index
        self.children = []
        self.parent = None


def parse(lines):
    # Brace depth decides nesting: "Xxx name {" opens a node one level deeper, a lone "{"
    # opens its children (or ScriptParams) block, "}" closes whatever is innermost.
    roots, stack = [], []           # stack of (node, depth at which it opened)
    depth = 0
    for i, ln in enumerate(lines):
        m = re.match(r"\s*(\w+Class)\s+\S+\s*\{\s*$", ln)
        if m:
            n = Node(m.group(1), i)
            if stack:
                n.parent = stack[-1][0]
                stack[-1][0].children.append(n)
            else:
                roots.append(n)
            depth += 1
            stack.append((n, depth))
            continue
        if re.search(r"\{\s*$", ln):         # lone "{" or an unnamed block ("ScriptParamsClass {")
            depth += 1
            continue
        if re.match(r"\s*\}\s*$", ln):
            if stack and stack[-1][1] == depth:
                stack.pop()
            depth -= 1
            continue
        if stack and stack[-1][1] == depth:     # a property of the node itself
            k = re.match(r"\s*(position|size|hexactpos|hexactsize|halign|Columns|Margin|Padding)\b", ln)
            if k:
                stack[-1][0].props[k.group(1)] = i
    return roots


def val(lines, node, key, idx=None):
    if key not in node.props:
        return None
    parts = lines[node.props[key]].split()
    vals = parts[1:]
    if idx is None:
        return vals
    return vals[idx] if idx < len(vals) else None


def set_val(lines, node, key, idx, new):
    i = node.props[key]
    indent = re.match(r"\s*", lines[i]).group(0)
    parts = lines[i].split()
    parts[1 + idx] = new
    lines[i] = indent + " ".join(parts)


def set_flag(lines, node, key, new):
    i = node.props[key]
    indent = re.match(r"\s*", lines[i]).group(0)
    lines[i] = f"{indent}{key} {new}"


def width_px(lines, node, parent_w):
    s = val(lines, node, "size", 0)
    if s is None:
        return parent_w
    w = float(s)
    return w if val(lines, node, "hexactsize", 0) == "1" else parent_w * w


def cell_width(lines, spacer, spacer_w):
    if spacer.cls != "GridSpacerWidgetClass":
        return spacer_w
    cols = max(1, int(float((val(lines, spacer, "Columns", 0) or "1"))))
    # DayZ: "Margin" insets the content on every side, "Padding" is the gap between children
    inset = float(val(lines, spacer, "Margin", 0) or 0)
    gap = float(val(lines, spacer, "Padding", 0) or 2)
    return max(1.0, (spacer_w - 2 * inset - (cols - 1) * gap) / cols)


def walk(lines, node, parent_w, parent_is_spacer, changed):
    own_w = width_px(lines, node, parent_w)
    if not parent_is_spacer and parent_w > 0:
        if val(lines, node, "hexactsize", 0) == "1" and own_w >= MIN_W:
            set_val(lines, node, "size", 0, fmt(own_w / parent_w))
            set_flag(lines, node, "hexactsize", "0")
            changed[0] += 1
            # its horizontal offset follows, unless it is a small edge inset
            if val(lines, node, "hexactpos", 0) == "1":
                x = float(val(lines, node, "position", 0) or 0)
                if abs(x) >= MIN_X:
                    set_val(lines, node, "position", 0, fmt(x / parent_w))
                    set_flag(lines, node, "hexactpos", "0")
        elif val(lines, node, "hexactpos", 0) == "1":
            # fixed-width widget (icon, square button): keep its size, scale a large offset
            x = float(val(lines, node, "position", 0) or 0)
            if abs(x) >= MIN_X and "position" in node.props:
                set_val(lines, node, "position", 0, fmt(x / parent_w))
                set_flag(lines, node, "hexactpos", "0")
                changed[0] += 1
    is_spacer = node.cls in SPACERS
    child_w = cell_width(lines, node, own_w) if is_spacer else own_w
    for c in node.children:
        walk(lines, c, child_w, is_spacer, changed)


def process(path):
    if os.path.splitext(os.path.basename(path))[0] in SKIP:
        return 0
    lines = open(path, encoding="utf-8").read().split("\n")
    roots = parse(lines)
    changed = [0]
    W = design_width(path)
    for r in roots:
        # the root keeps its own sizing (it fills its host); its children follow its width
        own = width_px(lines, r, W)
        is_spacer = r.cls in SPACERS
        for c in r.children:
            walk(lines, c, cell_width(lines, r, own) if is_spacer else own, is_spacer, changed)
    open(path, "w", encoding="utf-8").write("\n".join(lines))
    return changed[0]


if __name__ == "__main__":
    total = 0
    for p in sys.argv[1:]:
        total += process(p)
    print(f"fluid: {total} widgets made width-relative in {len(sys.argv) - 1} layouts")
