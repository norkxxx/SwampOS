"""BIOS kernel + independent public GPT/EXT4 volume; no physical devices."""
import os,select,shutil,subprocess,time
from pathlib import Path
q=shutil.which('qemu-system-x86_64')
if not q:raise SystemExit('NOT RUN: QEMU missing')
cmd=[q,'-machine','pc','-cpu','qemu64','-m','128M','-drive','if=ide,index=0,format=raw,file=build/SwampOS.img,snapshot=on','-drive','if=ide,index=2,format=raw,file=build/StorageLab-GPT.img,snapshot=on','-nic','none','-display','none','-serial','stdio','-monitor','none','-no-reboot','-no-shutdown']
log=bytearray()
with subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) as p:
 def until(marker,start=0):
  end=time.monotonic()+30
  while marker not in log[start:]:
   if time.monotonic()>end:raise RuntimeError('missing '+repr(marker))
   if select.select([p.stdout],[],[],.2)[0]:
    b=os.read(p.stdout.fileno(),4096)
    if not b:raise RuntimeError('QEMU stopped')
    log.extend(b)
   if any(x in log for x in (b'_FAIL',b' FAIL',b'KERNEL_TRAP')):raise RuntimeError('kernel test failure')
 try:
  until(b'swamp> ');assert b'BOOT_OK' in log and b'STORAGE_NOT_MOUNTED' in log
  for command,marker in ((b'gpt',b'Partition LBA'),(b'mount-gpt 1',b'GPT EXT4 mounted read-only.'),(b'cat /welcome.txt',b'NOT encrypted.'),(b'run-console /bin/hello',b'VAULT_APP_HELLO'),(b'umount',b'Volume unmounted.'),(b'cat /welcome.txt',b'File unavailable')):
   start=len(log);p.stdin.write(command+b'\n');p.stdin.flush();until(marker,start);until(b'swamp> ',start)
  print('PASS: BIOS guest GPT partition, explicit read-only mount, external ELF, unmount')
 finally:
  p.kill();p.wait();Path('build/storage-lab.log').write_bytes(log)
