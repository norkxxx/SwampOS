"""Exercise kernel-owned, translated xHCI HID input on Q35 after ExitBootServices."""
import argparse
import json
import os
import re
from pathlib import Path
import select
import subprocess
import tempfile
import time

from live_pointer_guest import check_pointer
from ovmf_config import drives

def audit_domain(rpc,kind,revoked=False):
    # Read guest physical memory through the monitor, independently of kernel messages.
    def words(address,count):
        reply=rpc("human-monitor-command",{"command-line":f"xp /{count}gx {address:#x}"})
        return [int(n,16) for line in reply.splitlines() for n in re.findall(r"0x[0-9a-fA-F]+",line.split(":",1)[-1])]
    registers=words(0xfed90000,5)
    assert registers[3]>>63, "translation enable status bit missing"
    root=words(registers[4],512)
    assert len(root)==512
    if revoked:
        assert not any(root)
        print("PASS VT-d root entries revoked, translation still enabled")
        return
    assert sum(bool(x) for x in root)==1 and root[0]&1
    ctx=words(root[0]&~4095,512)
    active=[i for i in range(0,512,2) if ctx[i]&1]
    buses=rpc("query-pci")
    xhci=[d for bus in buses for d in bus["devices"] if d["id"]["vendor"]==0x1b36 and d["id"]["device"]==0x000d]
    assert len(xhci)==1 and xhci[0]["bus"]==0
    target=2*((xhci[0]["slot"]<<3)|xhci[0]["function"])
    assert active==[target] and ctx[target+1]==0x101
    tables=[registers[4],root[0]&~4095,ctx[target]&~4095]
    for _ in range(2):
        level=words(tables[-1],512)
        assert len(level)==512 and level[0]&3==3 and not any(level[1:])
        tables.append(level[0]&~4095)
    pt=words(tables[-1],512)
    pages=[2,2,2,1]+([2,2,1] if kind=="pair" else [])
    expected={16*(i+1)+j for i,n in enumerate(pages) for j in range(n)}
    assert len(pt)==512 and {i for i,p in enumerate(pt) if p}==expected
    targets={pt[i]&~4095 for i in expected}
    assert len(targets)==sum(pages) and not targets.intersection(tables)
    for i in expected:assert pt[i]&4095==3 and pt[i]&~4095!=4096*i
    print(f"PASS VT-d one PCI requester, {len(pages)} sparse windows / {len(targets)} pages, page tables excluded")


def run(kind,interactive=False):
    positive = kind in ("keyboard", "mouse", "pair")
    with tempfile.TemporaryDirectory() as temp:
        prefix = Path(temp, "qmp")
        for suffix in (".in", ".out"):
            os.mkfifo(str(prefix) + suffix)
        log = bytearray()
        device = "usb-mouse" if kind == "mouse" else "usb-kbd"
        args = ["qemu-system-x86_64", "-machine", "q35,smm=on,i8042=off",
                "-cpu", "qemu64", "-m", "128M", *drives(str(Path(temp,"VARS.fd")), explicit_secure_boot=True),
                "-drive", "if=ide,media=cdrom,format=raw,readonly=on,file=build/Farkuad-Live.iso",
                "-boot", "d", "-device", "qemu-xhci,id=hci", "-device", device+",id=hid,bus=hci.0,port=1",
                "-display", "none", "-serial", "stdio", "-monitor", "none", "-nic", "none",
                "-qmp", f"pipe:{prefix}", "-no-reboot", "-no-shutdown"]
        if kind in ("pair","three-devices"):
            args += ["-device","usb-mouse,id=pointer,bus=hci.0,port=2"]
        if kind=="three-devices":
            args += ["-device","usb-kbd,id=extra,bus=hci.0,port=3"]
        if kind != "no-iommu":
            args += ["-device", "intel-iommu"]
        if kind != "no-opt-in":
            args += ["-fw_cfg", "name=opt/swamp/usb-input,string=1"]
        if interactive:
            index=args.index("-qmp")
            del args[index:index+2]
            args[args.index("-display")+1]="gtk"
            subprocess.run(args,check=True)
            return
        with subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, bufsize=0) as guest:
            def wait(marker, start=0):
                deadline = time.monotonic()+65
                while marker not in log[start:]:
                    if time.monotonic() >= deadline or guest.poll() is not None:
                        raise AssertionError(f"missing {marker!r}: {log[-650:]!r}")
                    if select.select([guest.stdout], [], [], .25)[0]:
                        log.extend(os.read(guest.stdout.fileno(), 8192))
                return len(log)
            def serial(data):
                guest.stdin.write(data)
            try:
                wait(b"swamp> ")
                if not positive:
                    start=len(log)
                    serial(b"usb-start\n")
                    expected={"no-opt-in":b"USB_INPUT_DISABLED","no-iommu":b"VT-d domain unavailable",
                              "three-devices":b"attach one or two USB2 HID devices"}
                    wait(expected[kind],start)
                    assert b"_READY\n" not in log[log.find(b"BOOT_OK"):] and b"USB_INPUT_QUARANTINED" not in log
                    print("PASS",kind,"activation refused")
                    return
                ready = b"USB_INPUT_"+(b"KEYBOARD" if kind=="pair" else kind.upper().encode())+b"_READY"
                if kind=="pair": assert b"USB_INPUT_MOUSE_READY" in log, log[-600:]
                assert ready in log, log[-600:]
                assert b"PS2_KEYBOARD_ACK" not in log and b"PS2_MOUSE_ACK" not in log
                with open(str(prefix)+".in","wb",buffering=0) as send, open(str(prefix)+".out","rb",buffering=0) as recv:
                    def receive():
                        assert select.select([recv],[],[],10)[0], "QMP timeout"
                        return json.loads(recv.readline())
                    receive()
                    def rpc(command, arguments=None):
                        send.write((json.dumps({"execute":command,"arguments":arguments or {}})+"\n").encode())
                        while True:
                            reply=receive()
                            if "error" in reply:raise AssertionError(reply)
                            if "return" in reply:return reply["return"]
                    rpc("qmp_capabilities")
                    audit_domain(rpc,kind)
                    if kind in ("keyboard","pair"):
                        for key in ["v","e","r","s","i","o","n","ret"]:
                            rpc("human-monitor-command",{"command-line":"sendkey "+key+" 45"})
                            time.sleep(.13)
                        wait(b"Awakens / x86_64",log.find(b"swamp> "))
                    if kind in ("mouse","pair"):
                        if kind=="pair":rpc("human-monitor-command",{"command-line":"sendkey tab"})
                        else:serial(b"\t")
                        time.sleep(.35)
                        check_pointer(rpc,temp,"USB HID")
                        if kind=="pair":
                            rpc("human-monitor-command",{"command-line":"sendkey esc"})
                            time.sleep(.2)
                        else:serial(b"\x1b")
                    start=len(log)
                    serial(b"usb-status\n")
                    wait(b"USB input active, reports ",start)
                    wait(b"swamp> ",start)
                    start=len(log)
                    serial(b"usb-stop\n")
                    wait(b"USB_INPUT_STOPPED",start)
                    wait(b"swamp> ",start)
                    audit_domain(rpc,kind,revoked=True)
                    start=len(log)
                    serial(b"usb-start\n")
                    wait(ready,start)
                    start=len(log)
                    rpc("device_del",{"id":"hid"})
                    wait(b"USB_INPUT_STOPPED",start)
                print("PASS",kind,"over translated xHCI. Stop, restart and physical-port disconnect in guest verified.")
            finally:
                guest.kill();guest.wait()
                log.extend(guest.stdout.read())
                Path("build",f"usb-{kind}-serial.log").write_bytes(log)

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("profile",choices=("pair","keyboard","mouse","no-iommu","no-opt-in","three-devices"))
    p.add_argument("--interactive",action="store_true",help="open the desktop and forward the terminal to COM1")
    options=p.parse_args()
    if options.interactive and options.profile!="pair":p.error("--interactive requires pair")
    run(options.profile,options.interactive)
