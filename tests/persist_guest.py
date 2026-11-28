"""Exercise the two native transports using only disposable, private QEMU images."""
import argparse
import os
import select
import struct
import subprocess
import tempfile
import time
import zlib
from pathlib import Path
from ovmf_config import drives

SIZE = 4 * 1024 * 1024
SERIAL = "SWAMP38-DISPOSABLE"
LABEL = b"SWAMPOS-DISPOSABLE-V1".ljust(512, b"\0")
MOUNT = ("persist-mount", "PERSIST_MOUNTED")
UNMOUNT = ("persist-umount", "PERSIST_UNMOUNTED")


def record(generation, message):
    body = struct.pack("<8sQH490s", b"SWPNOTE1", generation, len(message), message)
    return body + struct.pack("<I", zlib.crc32(body))


def arguments(disk, controller, var_store, opt_in, iommu, serial):
    optical = "if=ide,media=cdrom,format=raw,readonly=on,file=build/Farkuad-Live.iso"
    args = ["qemu-system-x86_64", "-machine", "pc-q35-8.2,smm=on", "-cpu", "qemu64", "-m", "128M",
            *drives(str(var_store), explicit_secure_boot=True), "-drive", optical, "-boot", "d",
            "-drive", f"if=none,id=fixture,format=raw,file={disk},cache=writeback",
            "-nic", "none", "-display", "none", "-serial", "stdio", "-monitor", "none", "-no-reboot"]
    if controller == "nvme":
        args += ["-device", f"nvme,addr=5,drive=fixture,serial={serial}"]
    else:
        args += ["-device", "ich9-ahci,id=lab,addr=5", "-device", f"ide-hd,bus=lab.0,drive=fixture,serial={serial}"]
    if iommu:
        args += ["-device", "intel-iommu"]
    if opt_in:
        args += ["-fw_cfg", "name=opt/swamp/disposable-storage,string=1"]
    return args


def guest(disk, controller, scenario, commands, *, opt_in=True, iommu=True, serial=SERIAL):
    log = bytearray()
    args = arguments(disk, controller, disk.parent/"vars.fd", opt_in, iommu, serial)
    with subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=0) as vm:
        def wait(marker, start=0):
            limit = time.monotonic() + 45
            while marker not in log[start:]:
                if vm.poll() is not None or time.monotonic() >= limit:
                    raise RuntimeError(f"{scenario}: missing {marker!r}, {log[-500:]!r}")
                if select.select([vm.stdout], [], [], .25)[0]:
                    log.extend(os.read(vm.stdout.fileno(), 4096))
        try:
            wait(b"Autoboot in")
            vm.stdin.write(b"1")
            wait(b"swamp> ")
            assert b"BOOT_OK" in log and b"STORAGE_TEST_OK" in log
            for cmd, expected in commands:
                start = len(log)
                vm.stdin.write(cmd.encode() + b"\n")
                wait(b"swamp> ", start)
                result = log[start:].replace(b"\r", b"").split(b"\n")[1:]
                assert expected.encode() in result, (controller, scenario, cmd, result)
        finally:
            vm.kill()
            vm.wait()
            Path(f"build/storage-{controller}-{scenario}-serial.log").write_bytes(log)
    print("PASS", controller, scenario, flush=True)


def run(controller):
    with tempfile.TemporaryDirectory() as td:
        disk = Path(td, "scratch.raw")
        original = LABEL + record(1, b"seed") + bytes(512) + b"\xa5" * (SIZE-1536)
        disk.write_bytes(original)
        read = lambda value: ("cat /persist/note", "PERSIST_NOTE " + value)
        write = lambda value: ("write /persist/note " + value, "PERSIST_COMMITTED")
        guest(disk, controller, "write", [("cat /persist/note", "PERSIST_NOT_MOUNTED"), MOUNT,
              ("ls /persist", "note"), read("seed"), write("first"), write("survived cold boot"), read("survived cold boot")])
        committed = disk.read_bytes()
        assert committed == LABEL + record(3,b"survived cold boot") + record(2,b"first") + original[1536:]
        guest(disk, controller, "cold-boot", [MOUNT, read("survived cold boot"), UNMOUNT, MOUNT, read("survived cold boot")])
        assert disk.read_bytes() == committed
        torn = bytearray(committed)
        torn[600:680] = b"X"*80
        disk.write_bytes(torn)
        guest(disk, controller, "torn-record", [MOUNT, read("first"), write("recovered")])
        assert disk.read_bytes() == LABEL + record(3,b"recovered") + record(2,b"first") + original[1536:]
        for name, header, reason, options in [
            ("no-opt-in", LABEL, "Q35 Live opt-in required", {"opt_in": False}),
            ("no-iommu", LABEL, "VT-d unavailable or in use", {"iommu": False}),
            ("wrong-serial", LABEL, "identify or queue setup failed", {"serial": "UNRELATED"}),
            ("wrong-label", bytes(512), "invalid label or records", {}),
        ]:
            before = header + committed[512:]
            disk.write_bytes(before)
            guest(disk, controller, name, [("persist-mount", "PERSIST_DENIED: " + reason),
                  ("write /persist/note blocked", "PERSIST_NOT_MOUNTED")], **options)
            assert disk.read_bytes() == before
        print("PASS", controller, "full-image comparison after commits, recovery and four refusals", flush=True)


def desktop(controller):
    import stat
    path = Path("build", f"storage-demo-{controller}.raw")
    try:
        with path.open("xb") as image:
            image.write(LABEL + record(1, b"seed"))
            image.truncate(SIZE)
    except FileExistsError:
        pass
    fd = os.open(path, os.O_RDWR | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size != SIZE or os.pread(fd,512,0) != LABEL:
            raise SystemExit(f"Invalid demo volume: {path}")
        with tempfile.TemporaryDirectory() as td:
            args = arguments(Path(f"/proc/self/fd/{fd}"),controller,Path(td,"vars.fd"),True,True,SERIAL)
            args[args.index("-display")+1] = "gtk"
            subprocess.run(args,pass_fds=(fd,),check=True)
    finally:
        os.close(fd)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("controller", nargs="?", choices=("ahci", "nvme"))
    parser.add_argument("--interactive",action="store_true")
    options = parser.parse_args()
    choice = options.controller
    if options.interactive:
        desktop(choice or "nvme")
        raise SystemExit(0)
    for kind in ([choice] if choice else ["ahci","nvme"]):
        run(kind)
