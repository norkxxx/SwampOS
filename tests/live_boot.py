"""OVMF USB or optical boot -> custom kernel -> RAM EXT4 -> native app -> desktop pixels.
No network, no physical disks; a failure is not a successful runtime result.
"""
import argparse, json, os, select, shutil, subprocess, tempfile, time
from pathlib import Path
from ovmf_config import drives
from live_pointer_guest import check_pointer
q=shutil.which('qemu-system-x86_64')
if not q:raise SystemExit('NOT RUN: QEMU missing')
parser=argparse.ArgumentParser()
parser.add_argument('--optical',action='store_true')
parser.add_argument('--profile',choices=('desktop','classic','recovery','iommu'),default='desktop')
args=parser.parse_args()
optical=args.optical
profile=args.profile
mode=('optical' if optical else 'usb')+('' if profile=='desktop' else '-'+profile)
media = ['-drive','if=ide,media=cdrom,format=raw,readonly=on,file=build/Farkuad-Live.iso','-boot','d'] if optical else ['-device','qemu-xhci','-drive','if=none,id=stick,format=raw,file=build/Farkuad-Live.iso,snapshot=on','-device','usb-storage,drive=stick,bootindex=1']
with tempfile.TemporaryDirectory() as td:
 qmp_path=str(Path(td,'qmp'))
 for suffix in ('.in','.out'):os.mkfifo(qmp_path+suffix)
 cmd=[q,'-machine','q35,smm=on','-cpu','qemu64','-m','256M',*drives(f'farkuad-{mode}-vars.fd',explicit_secure_boot=True),*media,'-nic','none','-display','none','-serial','stdio','-monitor','none','-qmp',f'pipe:{qmp_path}','-no-reboot','-no-shutdown']
 if profile=='iommu':
  cmd[cmd.index('-machine')+1]='pc-q35-8.2,smm=on'
  cmd[cmd.index('-m')+1]='128M'
  cmd.extend(['-device','intel-iommu'])
 log=bytearray()
 with subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) as p:
  def wait(marker,start=0):
   end=time.monotonic()+80
   while marker not in log[start:]:
    if time.monotonic()>end:raise RuntimeError('missing '+repr(marker))
    if select.select([p.stdout],[],[],.2)[0]:
     b=os.read(p.stdout.fileno(),4096)
     if not b:raise RuntimeError('guest stopped')
     log.extend(b)
    if any(x in log for x in (b'_FAIL',b'KERNEL_TRAP',b'LIVE_DESKTOP_UNAVAILABLE')):raise RuntimeError('guest self-test failed')
  def command(c,marker):
   start=len(log);p.stdin.write(c+b'\n');p.stdin.flush();wait(marker,start);wait(b'swamp> ',start)
  try:
   if profile in ('classic','recovery'):
    wait(b'Autoboot in')
    p.stdin.write(b'2' if profile=='classic' else b'3');p.stdin.flush()
   wait(b'swamp> ')
   if profile=='recovery':
    assert b'LIVE_SERIAL_RECOVERY' in log and b'LIVE_DESKTOP_READY' not in log
    command(b'version',b'Awakens / x86_64')
    print('PASS: UEFI serial recovery selection and kernel shell')
    raise SystemExit(0)
   for marker in (b'LIVE_DESKTOP_READY',b'VM_TEST_OK',b'USER_TEST_OK',b'STORAGE_TEST_OK',b'BOOT_OK'):assert marker in log,marker
   if profile=='iommu':
    for name in (b'xhci',b'xhci-fw',b'xhci-probe',b'iommu'):
     command(name,b'swamp> ')
    for marker in (b'[PCI revalidated]',b'xHCI CAPABILITIES',b'DMAR scope matched'):
     assert marker in log,marker
    print('PASS: Q35 xHCI PCI/firmware match, MMIO capability read and VT-d scope match')
    raise SystemExit(0)
   command(b'usbtest',b'USB_HID_TEST_OK');command(b'usb',b'Inventory only; no USB transfers');command(b'storage',b'EXT4 mounted: read-only RAM volume');command(b'run /bin/gui-demo',b'App result: 13');command(b'gui-demo',b'GUI_DEMO_OK');command(b'gui-async-test',b'GUI_ASYNC_TEST_OK');command(b'ls /bin',b'overlord-demo');command(b'run-console /bin/sdk-demo',b'SWAMP_SDK_HELLO');command(b'gpt',b'Physical disk access disabled in Live mode.');command(b'ata-info',b'Physical disk access disabled in Live mode.');command(b'run-resource-console /bin/integration-demo ext:/welcome.txt',b'App result: 0');command(b'run-resource-console /bin/resource-scan ext:/pattern.bin',b'RESOURCE_SCAN_OK bytes=0x00002328');command(b'rammkdir docs',b'OK');command(b'ramimport /welcome.txt docs/welcome.txt',b'RAMIMPORT_OK');command(b'rammv docs archive',b'OK');command(b'ramcat archive/welcome.txt',b'SwampOS Vault');command(b'ramappend archive/welcome.txt tail',b'OK');command(b'ramcat archive/welcome.txt',b'tail');command(b'ramwrite test.txt hello-swamp',b'OK');command(b'run-resource-console /bin/integration-demo ram:test.txt',b'App result: 0');command(b'run-resource /bin/integration-demo ram:test.txt',b'App result: 25');command(b'ramcp test.txt copy.txt',b'OK');command(b'rammv copy.txt final.txt',b'OK');command(b'ramcat final.txt',b'hello-swamp');command(b'ramrm final.txt',b'OK');command(b'ramcat final.txt',b'File not found')
   with open(qmp_path+'.in','wb',buffering=0) as send,open(qmp_path+'.out','rb',buffering=0) as receive:
    def reply():
     if not select.select([receive],[],[],15)[0]:raise RuntimeError('QMP reply timeout')
     return json.loads(receive.readline())
    reply()
    def rpc(name,args=None):
     send.write(json.dumps({'execute':name,**({'arguments':args} if args else {})}).encode()+b'\n')
     while True:
      r=reply()
      if 'error' in r:raise RuntimeError(r['error'])
      if 'return' in r:return r['return']
    rpc('qmp_capabilities');time.sleep(.5)
    a=Path(f'build/farkuad-{mode}-guest-terminal.ppm').resolve();b=Path(f'build/farkuad-{mode}-guest-notes.ppm').resolve()
    rpc('screendump',{'filename':str(a)});rpc('human-monitor-command',{'command-line':'sendkey tab'});time.sleep(1);rpc('screendump',{'filename':str(b)})
    assert a.read_bytes().startswith(b'P6') and b.read_bytes().startswith(b'P6') and a.read_bytes()!=b.read_bytes()
    assert len(set(a.read_bytes()))>8
    if profile=='classic':
     assert b'PS2_MOUSE_DISABLED_OR_UNAVAILABLE' in log
    else:
     assert b'PS2_MOUSE_ACK' in log
     check_pointer(rpc,td)
   print(f'PASS: {mode} UEFI kernel boot, framebuffer, RAM EXT4, native ELF, physical-disk denial and PS/2 desktop navigation')
  finally:
   p.kill();p.wait();Path(f'build/farkuad-{mode}-guest.log').write_bytes(log)
