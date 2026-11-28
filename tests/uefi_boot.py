"""OVMF runtime gate. Captures console output; no synthetic success."""
import shutil,os,subprocess,select,time
from pathlib import Path
from ovmf_config import drives
q=shutil.which('qemu-system-x86_64')
if not q:raise SystemExit('NOT RUN: QEMU missing')
cmd=[q,'-machine','q35','-m','256M',*drives('locked-vars.fd'),'-drive','format=raw,file=build/SwampBoot-GPT.img,snapshot=on','-nic','none','-nographic','-monitor','none','-no-reboot']
log=bytearray()
with subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,stdin=subprocess.DEVNULL) as p:
    try:
        end=time.monotonic()+60
        while time.monotonic()<end:
            if select.select([p.stdout],[],[],.2)[0]:
                b=os.read(p.stdout.fileno(),4096)
                if not b:break
                log.extend(b)
            if all(x in log for x in (b'LOGIN_CHAINLOAD',b'SWAMPLOGIN_LOCKED',b'RETURNED LOCKED')):print('PASS: OVMF manager -> separate login -> locked return');break
        else:raise RuntimeError('OVMF expected console markers absent')
        if not all(x in log for x in (b'LOGIN_CHAINLOAD',b'SWAMPLOGIN_LOCKED',b'RETURNED LOCKED')):raise RuntimeError('OVMF stopped before markers')
    finally:
        p.kill();p.wait();Path('build/ovmf.log').write_bytes(log)
