"""Strict bare-metal integration gate; never interprets static strings as runtime results."""
import os, select, shutil, subprocess, time
from pathlib import Path
qemu=shutil.which('qemu-system-x86_64')
if not qemu:
    raise SystemExit('NOT RUN: qemu-system-x86_64 is not installed')
cmd=[qemu,'-machine','pc','-cpu','qemu64','-m','128M','-drive','format=raw,file=build/SwampOS.img','-display','none','-serial','stdio','-monitor','none','-no-reboot','-no-shutdown']
log=bytearray()
with subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) as p:
    def until(marker, start=0, timeout=25):
        deadline=time.monotonic()+timeout
        while marker not in log[start:]:
            if time.monotonic()>=deadline: raise RuntimeError(f'timeout: {marker!r}')
            if select.select([p.stdout],[],[],0.1)[0]:
                b=os.read(p.stdout.fileno(),4096)
                if not b: raise RuntimeError('QEMU stopped before expected marker')
                log.extend(b)
            if any(x in log for x in (b'_FAIL',b' FAIL',b'KERNEL_TRAP')): raise RuntimeError('guest self-test failed')
    try:
        until(b'swamp> ')
        for marker in (b'VM_TEST_OK',b'USER_TEST_OK',b'CORE_TEST_OK',b'BOOT_OK',*[f'[user] {s} PASS'.encode() for s in ('syscalls','default deny','kernel access','code write','NX stack','port I/O','guard page','FPU disabled','watchdog')]):
            assert marker in log,marker
        for command,marker in ((b'tasks',b'Task A: 5 Task B: 5'),(b'vmtest',b'VM_TEST_OK'),(b'sandbox',b'USER_TEST_OK'),(b'pmm',b'Free frames:'),(b'ticks',b'swamp> ')):
            start=len(log);p.stdin.write(command+b'\n');p.stdin.flush();until(marker,start);until(b'swamp> ',start)
        print('PASS: QEMU boot, memory, CPL3 fault tests, watchdog, scheduler, shell')
    finally:
        p.kill();p.wait();Path('build/qemu.log').write_bytes(log)
