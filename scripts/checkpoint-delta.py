#!/usr/bin/env python3
"""Compare current source/docs with a previous cumulative SwampOS ZIP."""
import hashlib
import sys
import zipfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {"TREE.txt", "DIFF-REPORT.md"}

def source_path(name):
    p = PurePosixPath(name)
    if len(p.parts) < 2 or p.parts[0] != "SwampOS-Farkuad":
        return None
    rel = PurePosixPath(*p.parts[1:])
    if rel.parts[0] == "build" or "__pycache__" in rel.parts or rel.name in EXCLUDED:
        return None
    return rel.as_posix()

def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: python3 scripts/checkpoint-delta.py PREVIOUS.zip")
    base = Path(sys.argv[1])
    if not base.is_file():
        raise SystemExit(f"base archive not found: {base}")
    with zipfile.ZipFile(base) as z:
        old = {}
        for item in z.infolist():
            if item.is_dir():
                continue
            rel = source_path(item.filename)
            if rel:
                old[rel] = item
        new = {}
        for path in ROOT.rglob("*"):
            if not path.is_file() or ".git" in path.parts or "__pycache__" in path.parts:
                continue
            rel = path.relative_to(ROOT).as_posix()
            if rel.startswith("build/") or rel in EXCLUDED:
                continue
            new[rel] = path
        old_names, new_names = set(old), set(new)
        added = sorted(new_names - old_names)
        removed = sorted(old_names - new_names)
        changed = sorted(name for name in old_names & new_names
                         if hashlib.sha256(z.read(old[name])).digest() != hashlib.sha256(new[name].read_bytes()).digest())
    lines = [
        f"# Incremental source delta against {base.name}", "",
        f"Previous source/docs paths: {len(old)}; current source/docs paths: {len(new)}.",
        f"Added: {len(added)}; changed: {len(changed)}; removed: {len(removed)}.",
        "", "## Added", *([f"- `{x}`" for x in added] or ["None"]),
        "", "## Changed", *([f"- `{x}`" for x in changed] or ["None"]),
        "", "## Removed", *([f"- `{x}`" for x in removed] or ["None"]),
        "", "Path retention does not prove functional equivalence. See TEST-REPORT.md.",
    ]
    (ROOT / "DIFF-REPORT.md").write_text("\n".join(lines) + "\n")
    print(f"PASS: {len(added)} added, {len(changed)} changed, {len(removed)} removed")
    if removed:
        raise SystemExit("removed source paths detected")

if __name__ == "__main__":
    main()
