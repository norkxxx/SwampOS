"""Independent OVMF -> kernel guest gate; no synthetic success."""
import os,select,shutil,subprocess,time
from pathlib import Path
from ovmf_config import drives
q=shutil.which('qemu-system-x86_64')
if not q:raise SystemExit('NOT RUN: QEMU missing')
cmd=[q,'-machine','q35,smm=on','-cpu','qemu64','-m','128M',*drives('kernel-lab-vars.fd',explicit_secure_boot=True),'-drive','if=ide,index=0,format=raw,file=build/KernelLab-GPT.img,snapshot=on','-drive','if=ide,index=2,format=raw,file=build/vault.ext4,snapshot=on','-nic','none','-display','none','-serial','stdio','-monitor','none','-no-reboot','-no-shutdown']
log=bytearray()
with subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) as p:
 def wait(marker,start=0):
  end=time.monotonic()+60
  while marker not in log[start:]:
   if time.monotonic()>end:raise RuntimeError('missing '+repr(marker))
   if select.select([p.stdout],[],[],.2)[0]:
    b=os.read(p.stdout.fileno(),4096)
    if not b:raise RuntimeError('QEMU stopped')
    log.extend(b)
   if any(x in log for x in (b'_FAIL',b' FAIL',b'KERNEL_TRAP')):raise RuntimeError('kernel test failure')
 try:
  wait(b'swamp> ')
  for marker in (b'VM_TEST_OK',b'USER_TEST_OK',b'CORE_TEST_OK',b'NET_TEST_OK',b'BOOT_OK'):assert marker in log,marker
  start=len(log);p.stdin.write(b'firmware\n');p.stdin.flush();wait(b'UEFI plaintext kernel lab',start);wait(b'swamp> ',start)
  print('PASS: OVMF handoff, kernel markers, UEFI mode console')
 finally:
  p.kill();p.wait();Path('build/kernel-lab.log').write_bytes(log)
