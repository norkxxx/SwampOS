"""QEMU guest integration gate. Local TCP socket carries raw Ethernet only; no bridge/NAT."""
import os, select, shutil, socket, struct, subprocess, time
from pathlib import Path
qemu=shutil.which('qemu-system-x86_64')
if not qemu: raise SystemExit('NOT RUN: qemu-system-x86_64 is not installed')
mac=bytes.fromhex('525400123456'); peer=bytes.fromhex('020000000002'); src=bytes([192,0,2,2]); dst=bytes([192,0,2,1])
def csum(b):
    if len(b)%2: b+=b'\0'
    s=sum(struct.unpack('!%dH'%(len(b)//2),b))
    while s>>16: s=(s&65535)+(s>>16)
    return (~s)&65535
def arp(): return b'\xff'*6+peer+b'\x08\x06'+struct.pack('!HHBBH',1,0x800,6,4,1)+peer+src+b'\0'*6+dst+b'\0'*18
def icmp():
    body=struct.pack('!BBHHH',8,0,0,17,1)+b'SwampOS connectivity'
    body=body[:2]+struct.pack('!H',csum(body))+body[4:]
    ip=struct.pack('!BBHHHBBH4s4s',0x45,0,20+len(body),0,0,64,1,0,src,dst)
    ip=ip[:10]+struct.pack('!H',csum(ip))+ip[12:]
    return (mac+peer+b'\x08\x00'+ip+body).ljust(60,b'\0')
def recvn(sock,n):
    b=bytearray()
    while len(b)<n:
        d=sock.recv(n-len(b))
        if not d: raise RuntimeError('network peer disconnected')
        b.extend(d)
    return bytes(b)
def exchange(sock,frame):
    sock.sendall(struct.pack('!I',len(frame))+frame)
    n=struct.unpack('!I',recvn(sock,4))[0]
    if not 14<=n<=1514: raise RuntimeError('unexpected frame length')
    return recvn(sock,n)
log=bytearray()
with socket.socket() as server:
    server.bind(('127.0.0.1',0));server.listen(1);server.settimeout(15)
    port=server.getsockname()[1]
    args=[qemu,'-machine','pc','-cpu','qemu64','-m','128M','-drive','if=ide,index=0,format=raw,file=build/SwampOS.img,snapshot=on','-netdev',f'socket,id=lab,connect=127.0.0.1:{port}','-device','virtio-net-pci,netdev=lab,disable-modern=on,mac=52:54:00:12:34:56','-display','none','-serial','stdio','-monitor','none','-no-reboot','-no-shutdown']
    with subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) as p:
        def until(marker,start=0,timeout=35):
            end=time.monotonic()+timeout
            while marker not in log[start:]:
                if time.monotonic()>end: raise RuntimeError(f'timeout: {marker!r}')
                if select.select([p.stdout],[],[],0.1)[0]:
                    b=os.read(p.stdout.fileno(),4096)
                    if not b: raise RuntimeError('QEMU stopped')
                    log.extend(b)
                if any(x in log for x in (b'_FAIL',b'KERNEL_TRAP',b'NETLAB_UNAVAILABLE',b'NETLAB_DEVICE_ERROR')): raise RuntimeError('guest failure')
        try:
            with server.accept()[0] as link:
                link.settimeout(5);until(b'swamp> ');assert b'CONNECTIVITY_TEST_OK' in log
                start=len(log);p.stdin.write(b'netlab\n');p.stdin.flush();until(b'NETLAB_READY',start)
                a=exchange(link,arp());assert a[:14]==peer+mac+b'\x08\x06' and a[20:22]==b'\0\x02' and a[22:32]==mac+dst and a[32:42]==peer+src
                b=exchange(link,icmp());assert b[:14]==peer+mac+b'\x08\x00' and b[26:34]==dst+src and not csum(b[14:34]) and b[34]==0 and not csum(b[34:14+struct.unpack('!H',b[16:18])[0]])
                until(b'NETLAB_STOP replies=2',start);until(b'swamp> ',start)
                print('PASS: isolated QEMU virtio-net ARP/ICMP round-trip and explicit shutdown')
        finally:
            p.kill();p.wait();Path('build/network-lab.log').write_bytes(log)
