#!/usr/bin/env python3
"""Check staged blobs for GitHub's raw blob limit and canonical LFS pointers.
Does not inspect historical commits or contact GitHub; run before committing.
"""
import re
import subprocess
import sys
from pathlib import PurePosixPath

LIMIT = 100 * 1024 * 1024
LFS_SUFFIXES = {'.img', '.iso', '.ext4', '.raw', '.qcow2', '.vmdk', '.vdi', '.rom'}
POINTER = re.compile(rb'version https://git-lfs.github.com/spec/v1\noid sha256:[0-9a-f]{64}\nsize (0|[1-9][0-9]*)\n')

def git(*args, data=None):
    return subprocess.check_output(['git', *args], input=data)

def check():
    entries = []
    errors = []
    for row in git('ls-files', '-s', '-z').split(b'\0'):
        if not row:
            continue
        metadata, path = row.split(b'\t', 1)
        mode, oid, stage = metadata.split()
        if stage != b'0':
            errors.append(f'unmerged path: {path!r}')
        elif mode != b'160000':
            entries.append((oid, path))
    if entries:
        oids = list(dict.fromkeys(oid for oid, _ in entries))
        sizes = git('cat-file', '--batch-check=%(objectname) %(objecttype) %(objectsize)', data=b'\n'.join(oids)+b'\n')
        by_oid = {parts[0]: int(parts[2]) for line in sizes.splitlines() if len(parts := line.split()) == 3 and parts[1] == b'blob'}
        attrs = git('check-attr', '--cached', '-z', 'filter', '--stdin', data=b'\0'.join(path for _, path in entries)+b'\0').split(b'\0')
        filters = {attrs[i]: attrs[i+2] for i in range(0, len(attrs)-1, 3)}
        for oid, path in entries:
            name = path.decode('utf-8', 'surrogateescape')
            size = by_oid.get(oid)
            if size is None:
                errors.append(f'missing index blob: {name!r}'); continue
            if size > LIMIT:
                errors.append(f'raw Git blob exceeds 100 MiB: {name!r} ({size} bytes)')
            if PurePosixPath(name).suffix.lower() in LFS_SUFFIXES or filters.get(path) == b'lfs':
                if filters.get(path) != b'lfs':
                    errors.append(f'missing staged LFS attribute: {name!r}')
                if size > 1024 or not POINTER.fullmatch(git('cat-file', 'blob', oid.decode('ascii'))):
                    errors.append(f'not a canonical staged LFS pointer: {name!r}')
    for error in errors:
        print('FAIL:', error, file=sys.stderr)
    if errors:
        return 1
    print(f'PASS: {len(entries)} staged paths; raw size and LFS pointer checks (index only)')
    return 0

if __name__ == '__main__':
    try:
        raise SystemExit(check())
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f'Index validation failed: {exc}', file=sys.stderr)
        raise SystemExit(2)
