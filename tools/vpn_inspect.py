#!/usr/bin/env python3
"""Offline URI metadata inspection only. Never resolves DNS, connects, or prints credentials."""
import argparse
import ipaddress
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlsplit, parse_qsl

ALLOWED = {'vless', 'hysteria2', 'hy2', 'trojan', 'tuic'}
MAX_INPUT = 256 * 1024

def inspect_uri(line):
    if not isinstance(line, str) or not 1 <= len(line) <= 8192 or any(ord(c) < 33 or ord(c) > 126 for c in line):
        raise ValueError('invalid URI')
    try:
        u = urlsplit(line)
        if u.scheme not in ALLOWED or not u.hostname or not u.port or not u.username or u.path not in ('', '/'):
            raise ValueError
        host = u.hostname
        try:
            ipaddress.ip_address(host)
        except ValueError:
            if len(host)>253 or not all(re.fullmatch(r'[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?', x) for x in host.rstrip('.').split('.')):
                raise ValueError
        params = parse_qsl(u.query, keep_blank_values=True, strict_parsing=True, max_num_fields=64, encoding='utf-8', errors='strict')
        keys = set()
        for k, v in params:
            if k in keys or any(ord(c)<32 or ord(c)==127 for c in k+v): raise ValueError
            keys.add(k)
            if k.lower() in ('insecure','allowinsecure','skip-cert-verify') and v.lower() not in ('0','false','no'):
                raise ValueError
        # No username, query values, fragment, or raw URI are returned.
        return {'protocol': 'hysteria2' if u.scheme=='hy2' else u.scheme, 'host': host, 'port': u.port, 'status': 'metadata-only'}
    except (ValueError, UnicodeError) as e:
        raise ValueError('unsupported or malformed URI') from None

def inspect_text(text):
    if len(text.encode('utf-8'))>MAX_INPUT: raise ValueError('input too large')
    rows=[]; rejected=0
    for line in text.splitlines():
        if not line.strip(): continue
        try: rows.append(inspect_uri(line.strip()))
        except ValueError: rejected+=1
    return {'profiles': rows, 'rejected': rejected, 'connections_attempted': 0}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('file',type=Path);a=p.parse_args()
    try:
        with a.file.open('rb') as f: data=f.read(MAX_INPUT+1)
        if len(data)>MAX_INPUT: raise ValueError
        result=inspect_text(data.decode('utf-8'))
    except (OSError,ValueError,UnicodeError):
        print('Input rejected',file=sys.stderr);return 2
    print(json.dumps(result,indent=2));return 1 if result['rejected'] else 0
if __name__=='__main__': raise SystemExit(main())
