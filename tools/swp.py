#!/usr/bin/env python3
"""SwampPKG v1: signed, bounded, data-only packages and local host staging store."""
import argparse
import contextlib
import fcntl
import hashlib
import io
import json
import os
import re
import stat
import tempfile
import zipfile
from pathlib import Path
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey, Ed25519PublicKey
from cryptography.exceptions import InvalidSignature

LIMIT = 16 * 1024 * 1024
MAX_FILES = 128
NAME = re.compile(r'[a-z][a-z0-9-]{0,62}\Z')
VERSION = re.compile(r'(0|[1-9][0-9]{0,8})\.(0|[1-9][0-9]{0,8})\.(0|[1-9][0-9]{0,8})\Z')
PART = re.compile(r'[A-Za-z0-9_-][A-Za-z0-9_.-]{0,63}\Z')
HEX = re.compile(r'[0-9a-f]{64}\Z')
DOMAIN = b'SwampPKG manifest v1\0'

def require(ok, message):
    if not ok: raise ValueError(message)

def canonical(obj):
    return json.dumps(obj, sort_keys=True, separators=(',', ':'), ensure_ascii=True, allow_nan=False).encode('ascii')

def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'duplicate JSON key')
        result[key] = value
    return result

def digest(data): return hashlib.sha256(data).hexdigest()

def path_ok(path):
    return isinstance(path, str) and len(path) <= 240 and all(PART.fullmatch(x) for x in path.split('/'))

def bounded_read(path, limit=LIMIT):
    with open(path, 'rb') as f: data = f.read(limit+1)
    require(len(data) <= limit, 'file too large')
    return data

def validate(m):
    require(isinstance(m, dict) and set(m)=={'format','name','version','depends','permissions','files'}, 'manifest fields')
    require(type(m['format']) is int and m['format']==1, 'format')
    require(isinstance(m['name'],str) and NAME.fullmatch(m['name']), 'name')
    require(isinstance(m['version'],str) and VERSION.fullmatch(m['version']), 'version')
    dep=m['depends']; require(isinstance(dep,dict) and len(dep)<=64, 'dependencies')
    for n,v in dep.items(): require(NAME.fullmatch(n) and n!=m['name'] and isinstance(v,str) and VERSION.fullmatch(v),'dependency')
    p=m['permissions']; require(isinstance(p,list) and all(isinstance(x,str) and x in ('console',) for x in p) and len(p)==len(set(p)), 'unsupported permissions')
    files=m['files']; require(isinstance(files,dict) and 0<len(files)<=MAX_FILES, 'file count')
    total=0
    for n,entry in files.items():
        require(path_ok(n) and isinstance(entry,dict) and set(entry)=={'size','sha256'},'file entry')
        require(type(entry['size']) is int and 0<=entry['size']<=LIMIT and isinstance(entry['sha256'],str) and HEX.fullmatch(entry['sha256']), 'file metadata')
        total+=entry['size']
        require(not any('/'.join(n.split('/')[:i]) in files for i in range(1,len(n.split('/')))), 'file/directory conflict')
    require(total<=LIMIT-131072, 'payload limit')

def build(name, version, files, key, depends=None, permissions=None):
    m={'format':1,'name':name,'version':version,'depends':depends or {},'permissions':permissions or [],'files':{n:{'size':len(d),'sha256':digest(d)} for n,d in files.items()}}
    validate(m); raw=canonical(m); sig=key.sign(DOMAIN+raw)
    out=io.BytesIO()
    with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_STORED) as z:
        for n,d in [('manifest.json',raw),('signature.ed25519',sig)]+[('payload/'+n,files[n]) for n in sorted(files)]:
            info=zipfile.ZipInfo(n); info.create_system=3; info.external_attr=(stat.S_IFREG|0o644)<<16; z.writestr(info,d)
    data=out.getvalue(); require(len(data)<=LIMIT,'archive limit'); return data

def verify(data,key):
    require(len(data)<=LIMIT,'archive limit')
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        members=z.infolist(); names=[i.filename for i in members]
        require(len(names)<=MAX_FILES+2 and len(names)==len(set(names)), 'duplicate or excessive members')
        for i in members:
            require(i.orig_filename==i.filename and path_ok(i.filename) and i.compress_type==zipfile.ZIP_STORED and not i.flag_bits&1 and i.file_size==i.compress_size and stat.S_IFMT(i.external_attr>>16)==stat.S_IFREG, 'unsafe archive member')
        require('manifest.json' in names and 'signature.ed25519' in names and z.getinfo('manifest.json').file_size<=65536 and z.getinfo('signature.ed25519').file_size==64,'manifest/signature')
        raw=z.read('manifest.json'); m=json.loads(raw,object_pairs_hook=unique_pairs); validate(m)
        require(raw==canonical(m),'noncanonical manifest')
        key.verify(z.read('signature.ed25519'),DOMAIN+raw)
        require(set(names)=={'manifest.json','signature.ed25519'}|{'payload/'+n for n in m['files']},'unexpected members')
        for n,entry in m['files'].items():
            i=z.getinfo('payload/'+n); require(i.file_size==entry['size'],'size mismatch')
            require(digest(z.read(i))==entry['sha256'],'hash mismatch')
        return m

def atomic(path,data):
    fd,tmp=tempfile.mkstemp(prefix='.tmp-',dir=path.parent)
    try:
        with os.fdopen(fd,'wb') as f: f.write(data); f.flush(); os.fsync(f.fileno())
        os.replace(tmp,path)
        d=os.open(path.parent,os.O_RDONLY|os.O_DIRECTORY)
        try: os.fsync(d)
        finally: os.close(d)
    finally:
        if os.path.exists(tmp): os.unlink(tmp)

def validate_graph(manifests):
    for n,m in manifests.items():
        for dep,v in m['depends'].items(): require(dep in manifests and manifests[dep]['version']==v, f'{n}: missing exact dependency {dep}={v}')
    visiting=set(); done=set()
    def visit(n):
        require(n not in visiting,'dependency cycle')
        if n in done: return
        visiting.add(n)
        for d in manifests[n]['depends']: visit(d)
        visiting.remove(n); done.add(n)
    for n in manifests: visit(n)

class Store:
    """Single-user trusted host directory; no scripts, extraction or execution."""
    def __init__(self,root,key): self.root=Path(root); self.key=key
    @contextlib.contextmanager
    def locked(self):
        self.root.mkdir(mode=0o700,parents=True,exist_ok=True)
        require(not self.root.is_symlink() and self.root.stat().st_uid==os.getuid() and not self.root.stat().st_mode&0o022,'unsafe store directory')
        self.objects=self.root/'objects'; self.objects.mkdir(mode=0o700,exist_ok=True)
        require(not self.objects.is_symlink() and self.objects.stat().st_uid==os.getuid() and not self.objects.stat().st_mode&0o022,'unsafe object directory')
        fd=os.open(self.root/'lock',os.O_CREAT|os.O_RDWR|os.O_NOFOLLOW,0o600)
        with os.fdopen(fd,'rb+') as f:
            fcntl.flock(f,fcntl.LOCK_EX); yield
    def state(self):
        p=self.root/'state.json'
        s=json.loads(bounded_read(p,65536),object_pairs_hook=unique_pairs) if p.exists() else {'active':{},'previous':None}
        require(isinstance(s,dict) and set(s)=={'active','previous'},'state')
        for x in (s['active'],s['previous']):
            if x is None: require(x is s['previous'],'active state'); continue
            require(isinstance(x,dict) and len(x)<=128 and all(NAME.fullmatch(n) and isinstance(v,str) and HEX.fullmatch(v) for n,v in x.items()),'state entries')
        require(isinstance(s['active'],dict),'active state'); return s
    def object(self,h):
        data=bounded_read(self.objects/(h+'.swp')); require(digest(data)==h,'object hash mismatch'); return data,verify(data,self.key)
    def manifests(self,active):
        result={}
        for n,h in active.items():
            _,m=self.object(h); require(m['name']==n,'state name mismatch'); result[n]=m
        validate_graph(result); return result
    def commit(self,s,new):
        require(len(new)<=128,'store capacity'); self.manifests(new)
        if new!=s['active']: atomic(self.root/'state.json',canonical({'active':new,'previous':s['active']}))
    def install(self,packages):
        with self.locked():
            s=self.state(); old=self.manifests(s['active']); new=dict(s['active']); seen=set()
            for data in packages:
                m=verify(data,self.key); n=m['name']; h=digest(data); require(n not in seen,'duplicate package name'); seen.add(n)
                if n in old:
                    require(tuple(map(int,m['version'].split('.'))) >= tuple(map(int,old[n]['version'].split('.'))),'downgrade requires rollback')
                    require(m['version']!=old[n]['version'] or h==new[n],'same version different content')
                p=self.objects/(h+'.swp')
                if not p.exists(): atomic(p,data)
                new[n]=h
            self.commit(s,new)
    def remove(self,name):
        with self.locked():
            s=self.state(); self.manifests(s['active']); new=dict(s['active']); require(name in new,'not installed'); del new[name]; self.commit(s,new)
    def rollback(self):
        with self.locked():
            s=self.state(); self.manifests(s['active']); require(s['previous'] is not None,'no previous snapshot'); self.commit(s,s['previous'])
    def list(self):
        with self.locked(): return self.manifests(self.state()['active'])
    def read(self,name,path):
        with self.locked():
            s=self.state(); self.manifests(s['active']); require(name in s['active'],'not installed'); data,m=self.object(s['active'][name]); require(path in m['files'],'not found')
            with zipfile.ZipFile(io.BytesIO(data)) as z: return z.read('payload/'+path)

def public(path):
    k=serialization.load_pem_public_key(bounded_read(path,8192)); require(isinstance(k,Ed25519PublicKey),'expected Ed25519 public key'); return k

def private(path):
    import getpass
    k=serialization.load_pem_private_key(bounded_read(path,8192),password=getpass.getpass('Signing key passphrase: ').encode()); require(isinstance(k,Ed25519PrivateKey),'expected Ed25519 key'); return k

def newfile(path,data):
    fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
    with os.fdopen(fd,'wb') as f: f.write(data)

def main():
    p=argparse.ArgumentParser(description=__doc__); sub=p.add_subparsers(dest='cmd',required=True)
    k=sub.add_parser('keygen'); k.add_argument('private'); k.add_argument('public')
    b=sub.add_parser('build'); b.add_argument('name'); b.add_argument('version'); b.add_argument('directory'); b.add_argument('key'); b.add_argument('output'); b.add_argument('--depends',action='append',default=[]); b.add_argument('--console',action='store_true')
    v=sub.add_parser('verify'); v.add_argument('package'); v.add_argument('key')
    for cmd in ('install','remove','rollback','list','read'):
        a=sub.add_parser(cmd); a.add_argument('store'); a.add_argument('key')
        if cmd=='install': a.add_argument('packages',nargs='+')
        if cmd in ('remove','read'): a.add_argument('name')
        if cmd=='read': a.add_argument('path'); a.add_argument('output')
    a=p.parse_args()
    if a.cmd=='keygen':
        import getpass
        pw=getpass.getpass('New signing passphrase: ').encode(); require(len(pw)>=12 and pw==getpass.getpass('Repeat: ').encode(),'passphrase mismatch or too short')
        k=Ed25519PrivateKey.generate(); newfile(a.private,k.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.BestAvailableEncryption(pw))); newfile(a.public,k.public_key().public_bytes(serialization.Encoding.PEM,serialization.PublicFormat.SubjectPublicKeyInfo))
    elif a.cmd=='build':
        root=Path(a.directory); require(root.is_dir() and not root.is_symlink(),'input directory'); files={}; total=0
        for x in sorted(root.rglob('*')):
            require(not x.is_symlink(),'symlinks forbidden')
            if x.is_dir(): continue
            require(x.is_file() and len(files)<MAX_FILES,'invalid input'); d=bounded_read(x); total+=len(d); require(total<=LIMIT-131072,'payload limit'); files[x.relative_to(root).as_posix()]=d
        deps=dict(x.split('=',1) for x in a.depends); newfile(a.output,build(a.name,a.version,files,private(a.key),deps,['console'] if a.console else []))
    elif a.cmd=='verify': print(json.dumps(verify(bounded_read(a.package),public(a.key)),indent=2))
    else:
        s=Store(a.store,public(a.key))
        if a.cmd=='install': s.install([bounded_read(x) for x in a.packages])
        elif a.cmd=='remove': s.remove(a.name)
        elif a.cmd=='rollback': s.rollback()
        elif a.cmd=='list': print(json.dumps(s.list(),indent=2))
        elif a.cmd=='read': newfile(a.output,s.read(a.name,a.path))

if __name__=='__main__':
    try: main()
    except (ValueError,TypeError,KeyError,OSError,InvalidSignature,zipfile.BadZipFile,RecursionError) as e: raise SystemExit(f'SwampPKG: rejected: {e or type(e).__name__}')
