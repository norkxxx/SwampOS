import subprocess
p=subprocess.Popen(['qemu-system-x86_64','-drive','format=raw,file=build/SwampOS.img','-display','none','-serial','stdio','-monitor','none','-no-reboot'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
try:
    out,_=p.communicate(timeout=10)
except subprocess.TimeoutExpired:
    p.kill(); out,_=p.communicate()
assert b'BOOT_OK' in out, out
print('PASS: QEMU boot marker')
