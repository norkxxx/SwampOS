#!/usr/bin/env python3
"""Export a verified, dependency-checked native binary from a host SWP store.
Does not install into SwampOS, grant capabilities, or execute the payload.
"""
import argparse
import compat_inspect
import swp
from cryptography.exceptions import InvalidSignature
from zipfile import BadZipFile

def export(store, name, member, output):
    swp.require(member.startswith('bin/'), 'only bin/ payloads can be staged')
    data = store.read(name, member)
    swp.require(compat_inspect.inspect(data).get('native_profile_candidate') is True, 'not a native ELF candidate')
    swp.newfile(output, data)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('store','public_key','package','member','output'): p.add_argument(arg)
    a=p.parse_args()
    export(swp.Store(a.store,swp.public(a.public_key)),a.package,a.member,a.output)
    print('Verified host export only; guest loader revalidates ELF and operator grants capabilities.')

if __name__=='__main__':
    try: main()
    except (ValueError,TypeError,KeyError,OSError,InvalidSignature,BadZipFile) as exc: raise SystemExit(f'Stage rejected: {exc}')
