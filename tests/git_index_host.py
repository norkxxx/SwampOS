from pathlib import Path
import subprocess, tempfile, sys
script=Path('scripts/check-git-index.py').resolve()
with tempfile.TemporaryDirectory() as d:
    root=Path(d)
    def git(*args): return subprocess.run(['git',*args],cwd=root,check=True,capture_output=True)
    def check(code):
        p=subprocess.run([sys.executable,str(script)],cwd=root,capture_output=True,text=True)
        assert p.returncode==code,(p.stdout,p.stderr)
        return p.stdout+p.stderr
    git('init','-q'); (root/'source.c').write_text('int x;');git('add','source.c');check(0)
    (root/'test.img').write_bytes(b'not a pointer');git('add','test.img');assert 'missing staged LFS attribute' in check(1)
    (root/'.gitattributes').write_text('*.img filter=lfs diff=lfs merge=lfs -text\n');git('add','.gitattributes');assert 'not a canonical' in check(1)
    (root/'test.img').write_bytes(b'version https://git-lfs.github.com/spec/v1\noid sha256:'+b'a'*64+b'\nsize 174063616\n')
    # Use update-index to test the canonical pointer without a local LFS dependency.
    oid=git('hash-object','-w','--no-filters','test.img').stdout.strip().decode();git('update-index','--cacheinfo',f'100644,{oid},test.img');check(0)
print('PASS: Git staged raw image rejection, missing attribute rejection, canonical LFS pointer acceptance (local fixture)')
