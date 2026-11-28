"""Strict bare-metal integration gate; never interprets static strings as runtime results."""
import os, select, shutil, subprocess, time
from pathlib import Path
qemu=shutil.which('qemu-system-x86_64')
if not qemu:
    raise SystemExit('NOT RUN: qemu-system-x86_64 is not installed')
cmd=[qemu,'-machine','pc','-cpu','qemu64','-m','128M','-drive','if=ide,index=0,format=raw,file=build/SwampOS.img,snapshot=on','-drive','if=ide,index=2,format=raw,file=build/vault.ext4,snapshot=on','-nic','none','-display','none','-serial','stdio','-monitor','none','-no-reboot','-no-shutdown']
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
        for marker in (b'VM_TEST_OK',b'USER_TEST_OK',b'CORE_TEST_OK',b'NET_TEST_OK',b'CONNECTIVITY_TEST_OK',b'AUDIT_TEST_OK',b'SESSION_TEST_OK',b'OVERLORD_TEST_OK',b'STORAGE_TEST_OK',b'BOOT_OK',*[f'[user] {s} PASS'.encode() for s in ('syscalls','default deny','kernel access','code write','NX stack','port I/O','guard page','FPU disabled','watchdog')]):
            assert marker in log,marker
        for command,marker in ((b'tasks',b'Task A: 5 Task B: 5'),(b'vmtest',b'VM_TEST_OK'),(b'sandbox',b'USER_TEST_OK'),(b'pmm',b'Free frames:'),(b'ticks',b'swamp> '),(b'cpu',b'CPU:'),(b'firmware',b'BIOS plaintext lab'),(b'nettest',b'NET_TEST_OK'),(b'ls /bin',b'hello  inode='),(b'cat /welcome.txt',b'NOT encrypted.'),(b'run /bin/hello',b'App result: 13'),(b'run-console /bin/hello',b'App result: 0'),(b'run /bin/sdk-demo',b'App result: 13'),(b'run-console /bin/sdk-demo',b'App result: 0'),(b'run /bin/integration-demo',b'App result: 13'),(b'run-console /bin/integration-demo',b'App result: 13'),(b'run-data /bin/integration-demo',b'App result: 0'),(b'session-test',b'SESSION_TEST_OK'),(b'audit',b'kind=4'),(b'process-test',b'PROCESS_TEST_OK'),(b'scratch-test',b'SCRATCH_TEST_OK'),(b'run /bin/overlord-demo',b'App result: 13'),(b'run-scratch /bin/overlord-demo',b'App result: 0'),(b'run-pair /bin/overlord-demo',b'Group results: 0 0'),(b'run-group n:/bin/hello c:/bin/sdk-demo',b'Group results: 13 0')):
            start=len(log);p.stdin.write(command+b'\n');p.stdin.flush();until(marker,start);until(b'swamp> ',start)
        assert log.count(b'IPC_PING_OK') == 1 and log.count(b'IPC_PONG_OK') == 1
        assert log.count(b'SCRATCH_OK') == 1
        assert log.count(b'SWAMP_SDK_HELLO') == 2
        assert log.count(b'VAULT_APP_HELLO') == 1, 'external app console grant not enforced'
        print('PASS: QEMU boot, memory, CPL3 fault tests, watchdog, scheduler, shell, ATA, EXT4, external ELF and explicit console grant')
    finally:
        p.kill();p.wait();Path('build/qemu.log').write_bytes(log)
