"""OVMF GOP integration gate: compare actual guest framebuffer to host renderer."""
import json,shutil,socket,subprocess,tempfile,time
from pathlib import Path
from ovmf_config import drives
q=shutil.which('qemu-system-x86_64')
if not q: raise SystemExit('NOT RUN: QEMU missing')

def ppm(path):
    with open(path,'rb') as f:
        assert f.readline().strip()==b'P6'
        w,h=map(int,f.readline().split()); assert 0<w<=4096 and 0<h<=4096
        assert f.readline().strip()==b'255'; data=f.read(w*h*3+1); assert len(data)==w*h*3
        return w,h,data

expected=ppm('build/gui-preview.ppm'); assert expected[:2]==(640,360)
with tempfile.TemporaryDirectory() as td:
    sock=Path(td)/'qmp'; shot=Path(td)/'screen.ppm'
    cmd=[q,'-machine','q35','-m','256M',*drives('gui-lab-vars.fd'),'-drive','format=raw,file=build/GuiLab-GPT.img,snapshot=on','-nic','none','-display','none','-vga','std','-serial','none','-monitor','none','-qmp',f'unix:{sock},server=on,wait=off','-no-reboot']
    with open('build/gui-lab.log','wb') as log, subprocess.Popen(cmd,stdout=log,stderr=subprocess.STDOUT) as p:
        try:
            end=time.monotonic()+60
            while not sock.exists():
                if p.poll() is not None or time.monotonic()>end: raise RuntimeError('QMP unavailable')
                time.sleep(.1)
            with socket.socket(socket.AF_UNIX) as s:
                s.settimeout(5);s.connect(str(sock));f=s.makefile('rwb')
                assert 'QMP' in json.loads(f.readline())
                def command(name,args=None):
                    msg={'execute':name}
                    if args:msg['arguments']=args
                    f.write(json.dumps(msg).encode()+b'\n');f.flush()
                    while True:
                        line=f.readline()
                        if not line:raise RuntimeError('QMP closed')
                        obj=json.loads(line)
                        if 'error' in obj:raise RuntimeError(str(obj['error']))
                        if 'return' in obj:return obj['return']
                command('qmp_capabilities')
                while time.monotonic()<end:
                    command('screendump',{'filename':str(shot)})
                    w,h,data=ppm(shot)
                    if w>=640 and h>=360:
                        x=(w-640)//2;y=(h-360)//2
                        crop=b''.join(data[3*((y+i)*w+x):3*((y+i)*w+x+640)] for i in range(360))
                        if crop==expected[2]:
                            shutil.copyfile(shot,'build/gui-guest.ppm');print('PASS: OVMF GOP rendered exact host reference pixels');break
                    time.sleep(.5)
                else:raise RuntimeError('GUI framebuffer mismatch or boot timeout')
        finally:
            p.kill();p.wait()
