"""OVMF GOP integration gate: compare actual guest framebuffer to host renderer."""
import errno,json,os,select,shutil,subprocess,tempfile,time
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
    qmp=Path(td)/'qmp'; shot=Path(td)/'screen.ppm'
    for suffix in ('.in','.out'):os.mkfifo(str(qmp)+suffix)
    cmd=[q,'-machine','q35,smm=on','-m','256M',*drives('gui-lab-vars.fd',explicit_secure_boot=True),'-drive','format=raw,file=build/GuiLab-GPT.img,snapshot=on','-nic','none','-display','none','-vga','std','-serial','none','-monitor','none','-qmp',f'pipe:{qmp}','-no-reboot']
    write = None
    with open('build/gui-lab.log','wb') as log, subprocess.Popen(cmd,stdout=log,stderr=subprocess.STDOUT) as p:
        try:
            end=time.monotonic()+60
            read=os.open(str(qmp)+'.out',os.O_RDONLY|os.O_NONBLOCK)
            with os.fdopen(read,'rb',buffering=0) as receive:
                while True:
                    try:
                        write=os.open(str(qmp)+'.in',os.O_WRONLY|os.O_NONBLOCK)
                        break
                    except OSError as exc:
                        if exc.errno!=errno.ENXIO or p.poll() is not None or time.monotonic()>end:
                            raise RuntimeError('QMP unavailable') from exc
                        time.sleep(.05)
                os.set_blocking(read,True)
                def response():
                    if not select.select([receive],[],[],5)[0]:raise RuntimeError('QMP timeout')
                    line=receive.readline()
                    if not line:raise RuntimeError('QMP closed')
                    return json.loads(line)
                assert 'QMP' in response()
                def command(name,args=None):
                    msg={'execute':name}
                    if args:msg['arguments']=args
                    os.write(write,json.dumps(msg).encode()+b'\n')
                    while True:
                        obj=response()
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
            if write is not None:os.close(write)
            p.kill();p.wait()
