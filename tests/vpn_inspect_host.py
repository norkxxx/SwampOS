import sys,json,random
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from vpn_inspect import inspect_uri,inspect_text
s='vless://SECRET-USER@vpn.example:443?security=reality&pbk=SECRET-KEY#SECRET-LABEL'
r=inspect_uri(s);assert r=={'protocol':'vless','host':'vpn.example','port':443,'status':'metadata-only'} and 'SECRET' not in json.dumps(r)
assert inspect_uri('hy2://SECRET@[2001:db8::1]:443?insecure=0')['protocol']=='hysteria2'
for bad in ['https://x@example.com:443','vless://example.com:443','trojan://p@example.com:0','hy2://p@example.com:443?insecure=1','hy2://p@example.com:443?k=1&k=2','tuic://p@-x.example:443','vless://p@x:65536','vless://p@x:443/a','vless://p@x:443?k=%0a']:
    try: inspect_uri(bad)
    except ValueError: pass
    else: raise AssertionError('invalid URI accepted')
assert inspect_text(s+'\ninvalid')['rejected']==1
rng=random.Random(9)
for i in range(10000):
    pos=rng.randrange(len(s));line=s[:pos]+chr(rng.randrange(32,127))+s[pos+1:]
    try: row=inspect_uri(line)
    except ValueError: continue
    assert set(row)=={'protocol','host','port','status'} and 1<=row['port']<=65535
print('PASS: offline VPN URI metadata, secret field omission, insecure flag rejection, 10000 mutations')
