#!/usr/bin/env python3
"""
Estimate cyclomatic complexity for C functions in navit/plugin.
CC = 1 + (if + while + for + switch + case + && count + || count + ternary).
Reports functions with estimated CC > 10.
"""
import re
import sys
from pathlib import Path

# Match function definition: return_type name(...) { or static return_type name(...) {
FUNC_RE = re.compile(
    r"^(?:static\s+)?(?:inline\s+)?"
    r"(?:void|int|char|unsigned|size_t|double|float|struct\s+\w+|enum\s+\w+|gboolean|GList\s*\*|[\w\s\*]+)\s+"
    r"(\w+)\s*\([^)]*\)\s*\{",
    re.MULTILINE,
)

# Decision point patterns (each adds +1 to CC)
DP_IF = re.compile(r"\bif\s*\(")
DP_WHILE = re.compile(r"\bwhile\s*\(")
DP_FOR = re.compile(r"\bfor\s*\(")
DP_SWITCH = re.compile(r"\bswitch\s*\(")
DP_CASE = re.compile(r"\bcase\s+")
DP_AND = re.compile(r"&&")
DP_OR = re.compile(r"\|\|")
DP_TERNARY = re.compile(r"\?[^:]*:")


def count_decision_points(text):
    n = 0
    n += len(DP_IF.findall(text))
    n += len(DP_WHILE.findall(text))
    n += len(DP_FOR.findall(text))
    n += len(DP_SWITCH.findall(text))
    n += len(DP_CASE.findall(text))
    n += len(DP_AND.findall(text))
    n += len(DP_OR.findall(text))
    n += len(DP_TERNARY.findall(text))
    return n


def find_matching_brace(s, start):
    """Find closing } for { at start. Respects nested {} and string literals."""
    i = start + 1
    depth = 1
    in_string = None
    escape = False
    while i < len(s) and depth > 0:
        c = s[i]
        if escape:
            escape = False
            i += 1
            continue
        if c == "\\" and in_string:
            escape = True
            i += 1
            continue
        if in_string:
            if c == in_string:
                in_string = None
            i += 1
            continue
        if c in '"\'':
            in_string = c
            i += 1
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        i += 1
    return i - 1 if depth == 0 else -1


def analyze_file(path, threshold=10):
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        return [("ERROR", str(path), 0, str(e))]
    results = []
    for m in FUNC_RE.finditer(text):
        name = m.group(1)
        start = m.end() - 1  # position of {
        end = find_matching_brace(text, start)
        if end < 0:
            continue
        body = text[start : end + 1]
        dp = count_decision_points(body)
        cc = 1 + dp
        if cc > threshold:
            if name in ("if", "else", "while", "for", "switch", "case"):
                continue
            line_no = text[: m.start()].count("\n") + 1
            results.append((path, name, cc, line_no))
    return results


def main():
    plugin_dir = Path(__file__).resolve().parent.parent / "navit" / "plugin"
    if not plugin_dir.is_dir():
        print("Plugin dir not found:", plugin_dir, file=sys.stderr)
        sys.exit(1)
    threshold = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    exclude_dirs = {"tests"}
    all_results = []
    for c_file in sorted(plugin_dir.rglob("*.c")):
        if any(p in c_file.parts for p in exclude_dirs):
            continue
        all_results.extend(analyze_file(c_file, threshold))
    rel = plugin_dir.parent.parent
    for path, name, cc, line_no in sorted(all_results, key=lambda x: (-x[2], str(x[0]))):
        try:
            rel_path = path.relative_to(rel)
        except ValueError:
            rel_path = path
        print(f"{rel_path}:{line_no}: {name}  CC={cc}")
    if all_results:
        print(f"\nTotal: {len(all_results)} function(s) with cyclomatic complexity > {threshold}")
        sys.exit(1)
    print("All plugin functions at or below threshold", threshold)
    sys.exit(0)


if __name__ == "__main__":
    main()
