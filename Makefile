CC = gcc
CFLAGS = -m64 -mno-red-zone -mgeneral-regs-only -fno-unwind-tables -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -Wall -Wextra -Werror -O2
SRC := $(wildcard kernel/*.c) $(wildcard kernel/*.S)
OBJ := $(patsubst kernel/%.c,build/%.o,$(filter %.c,$(SRC))) $(patsubst kernel/%.S,build/%.o,$(filter %.S,$(SRC))) build/live-surface.o build/window.o build/compositor.o build/ramfs-store.o build/desktop-tools.o
.PHONY: all run test runtime clean
all: build/SwampOS.img build/vault.ext4 uefi gpt-image
build:
	mkdir -p build
build/boot.bin: boot/boot.S boot/layout.inc | build
	as --32 $< -o build/boot.o
	ld -m elf_i386 -Ttext 0x7c00 --oformat binary build/boot.o -o $@
build/%.o: kernel/%.c $(wildcard kernel/*.h) | build
	$(CC) $(CFLAGS) -c $< -o $@
build/%.o: kernel/%.S $(wildcard kernel/*.h) | build
	$(CC) $(CFLAGS) -c $< -o $@
build/kernel.elf: $(OBJ) linker.ld
	ld -m elf_x86_64 -T linker.ld $(OBJ) -o $@
build/kernel.bin: build/kernel.elf
	objcopy -O binary $< $@
build/main.o build/storage.o build/vm.o build/live.o build/user.o build/elf.o build/net.o build/transport.o build/ext4.o build/ramfs_shell.o build/virtio_net.o build/hardware.o build/core.o build/pmm.o build/pci.o build/device.o build/acpi.o build/process.o build/lineedit.o: CFLAGS += -Os
build/SwampOS.img: build/boot.bin build/kernel.bin scripts/image.py scripts/boot_layout.py boot/layout.inc
	python3 scripts/image.py
build/hello.elf: userspace/hello.S userspace/app.ld | build
	as --64 userspace/hello.S -o build/app.o
	ld -m elf_x86_64 -T userspace/app.ld build/app.o -o $@
build/mkvolume: tools/mkvolume.c | build
	$(CC) -O2 -Wall -Wextra -Werror $< -lext2fs -lcom_err -o $@
build/vault.ext4: build/mkvolume build/hello.elf build/sdk-demo.elf build/integration-demo.elf build/overlord-demo.elf build/gui-demo.elf build/resource-scan.elf
	./build/mkvolume $@ build/hello.elf build/sdk-demo.elf build/integration-demo.elf build/overlord-demo.elf build/gui-demo.elf build/resource-scan.elf
run: all
	qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M -drive if=ide,index=0,format=raw,file=build/SwampOS.img -drive if=ide,index=2,format=raw,file=build/vault.ext4,snapshot=on -nic none -serial stdio
test: all boot-host-test
	python3 tests/test_image.py
	$(CC) -O2 -Wall -Wextra -Werror -mno-red-zone -Ikernel tests/task_host.c kernel/task.c kernel/task_switch.S -o build/task_host
	./build/task_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -Ikernel tests/pmm_host.c kernel/pmm.c kernel/ranges.c kernel/policy.c -o build/pmm_host
	./build/pmm_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -DVM_HOST -Ikernel tests/vm_host.c kernel/vm.c kernel/pmm.c kernel/ranges.c -o build/vm_host
	./build/vm_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -Ikernel tests/storage_host.c kernel/block.c kernel/ext4.c kernel/elf.c -o build/storage_host
	./build/storage_host
	python3 tests/vault_host.py
runtime: all
	python3 tests/boot.py
clean:
	rm -rf build

# UEFI firmware front end (separate from the existing BIOS kernel).
EFI_CC ?= clang
EFI_LD ?= lld-link
EFI_FLAGS = --target=x86_64-pc-windows-msvc -O2 -ffreestanding -fshort-wchar -fno-stack-protector -mno-stack-arg-probe -mno-red-zone -Wall -Wextra -Werror
EFI_LINK = /subsystem:efi_application /entry:efi_main /nodefaultlib /timestamp:0 /fixed:no /dynamicbase
.PHONY: uefi gpt-image boot-host-test uefi-runtime
uefi: build/BOOTX64.EFI build/BOOTLAB.EFI build/LOGIN.EFI
build/efi-path.obj: boot/uefi/path.c boot/uefi/path.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/efi-manager.obj: boot/uefi/manager.c boot/uefi/efi.h boot/uefi/path.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/efi-lab.obj: boot/uefi/manager.c boot/uefi/efi.h boot/uefi/path.h | build
	$(EFI_CC) $(EFI_FLAGS) -DLAB_MODE=1 -c $< -o $@
build/efi-login.obj: boot/uefi/login.c boot/uefi/efi.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/BOOTX64.EFI: build/efi-manager.obj build/efi-path.obj
	$(EFI_LD) $(EFI_LINK) /out:$@ $^
build/BOOTLAB.EFI: build/efi-lab.obj build/efi-path.obj
	$(EFI_LD) $(EFI_LINK) /out:$@ $^
build/LOGIN.EFI: build/efi-login.obj
	$(EFI_LD) $(EFI_LINK) /out:$@ $^
gpt-image: build/SwampBoot-GPT.img
build/SwampBoot-GPT.img: build/BOOTLAB.EFI build/LOGIN.EFI tools/gptdisk.py tools/vault.py
	rm -f $@.tmp
	python3 tools/gptdisk.py $@.tmp build/BOOTLAB.EFI build/LOGIN.EFI
	mv $@.tmp $@
boot-host-test: gpt-image uefi build/vault.ext4
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -Ikernel tests/gpt_host.c kernel/gpt.c kernel/block.c -o build/gpt_host
	./build/gpt_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fshort-wchar -Iboot/uefi tests/efi_host.c boot/uefi/manager.c boot/uefi/path.c -o build/efi_host
	./build/efi_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fshort-wchar -Iboot/uefi -DLAB_MODE=1 tests/efi_host.c boot/uefi/manager.c boot/uefi/path.c -o build/efi_lab_host
	./build/efi_lab_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fshort-wchar -Iboot/uefi tests/login_host.c boot/uefi/login.c -o build/login_host
	./build/login_host
	python3 tests/gpt_image.py
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -DPCI_HOST -Ikernel tests/pci_host.c kernel/pci.c -o build/pci_host
	./build/pci_host
	python3 tests/registry_host.py
	python3 tests/provision_host.py
uefi-runtime: gpt-image
	python3 tests/uefi_boot.py

# Independent lab handoff, never part of the locked production-oriented image.
.PHONY: kernel-lab
kernel-lab: build/KernelLab-GPT.img
build/efi-handoff.obj: boot/uefi/handoff.c boot/uefi/handoff.h kernel/bootinfo.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/efi-kernellab.obj: boot/uefi/kernellab.c boot/uefi/handoff.h boot/uefi/efi.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/efi-jump.obj: boot/uefi/lab_jump.S build/kernel.bin kernel/bootinfo.h
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/KERNELLAB.EFI: build/efi-kernellab.obj build/efi-handoff.obj build/efi-jump.obj
	$(EFI_LD) $(EFI_LINK) /out:$@ $^
build/KernelLab-GPT.img: build/KERNELLAB.EFI build/LOGIN.EFI tools/gptdisk.py tools/vault.py
	rm -f $@.tmp
	python3 tools/gptdisk.py $@.tmp build/KERNELLAB.EFI build/LOGIN.EFI
	mv $@.tmp $@
all: kernel-lab

all: build/StorageLab-GPT.img
build/StorageLab-GPT.img: tools/mkstoragegpt.py tools/gptdisk.py build/vault.ext4
	rm -f $@.tmp
	python3 tools/mkstoragegpt.py $@.tmp build/vault.ext4
	mv $@.tmp $@
.PHONY: hardware-host-test kernel-lab-runtime
hardware-host-test: all
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -DHARDWARE_HOST -DNET_HOST -Ikernel -Iboot/uefi tests/hardware_host.c kernel/acpi.c kernel/keyboard.c kernel/net.c kernel/device.c boot/uefi/handoff.c kernel/block.c kernel/gpt.c kernel/ext4.c kernel/vfs.c -o build/hardware_host
	./build/hardware_host
	python3 tests/hardware_image.py
kernel-lab-runtime: kernel-lab
	python3 tests/kernel_lab_boot.py
test: hardware-host-test
.PHONY: storage-lab-runtime run-storage-lab
storage-lab-runtime: all
	python3 tests/storage_lab_boot.py
run-storage-lab: all
	qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M -drive if=ide,index=0,format=raw,file=build/SwampOS.img,snapshot=on -drive if=ide,index=2,format=raw,file=build/StorageLab-GPT.img,snapshot=on -nic none -serial stdio
.PHONY: checksums
checksums: all
	cd build && sha256sum boot.bin kernel.bin kernel.elf SwampOS.img vault.ext4 hello.elf BOOTX64.EFI BOOTLAB.EFI LOGIN.EFI SwampBoot-GPT.img KERNELLAB.EFI KernelLab-GPT.img StorageLab-GPT.img sdk-demo.elf integration-demo.elf overlord-demo.elf gui-demo.elf resource-scan.elf GUILAB.EFI GuiLab-GPT.img FARKUAD.EFI Farkuad-Live.iso Farkuad-Live.iso.report > SHA256SUMS
	cd build && sha256sum SwampOS.img vault.ext4 SwampBoot-GPT.img KernelLab-GPT.img StorageLab-GPT.img GuiLab-GPT.img Farkuad-Live.iso > IMAGE-SHA256SUMS
	cd build && sha256sum Farkuad-Live.iso > Farkuad-Live.iso.sha256

.PHONY: connectivity-host-test network-lab-runtime run-network-lab
connectivity-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -DNET_HOST -Ikernel tests/connectivity_host.c kernel/net.c kernel/transport.c kernel/swampwall.c kernel/vring.c kernel/vpn_policy.c -o build/connectivity_host
	./build/connectivity_host
	python3 tests/vpn_inspect_host.py
test: connectivity-host-test
network-lab-runtime: all
	python3 tests/network_lab_boot.py
# No NAT, bridge or Internet: only a local socket peer. Run the test target instead for automated traffic.
run-network-lab: all
	qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M -drive if=ide,index=0,format=raw,file=build/SwampOS.img,snapshot=on -netdev socket,id=lab,listen=127.0.0.1:17391 -device virtio-net-pci,netdev=lab,disable-modern=on,mac=52:54:00:12:34:56 -serial stdio


# Ecosystem: independent native SDK binary and firmware graphics laboratory.
build/sdk-demo.o: userspace/sdk_demo.c sdk/include/swamp.h | build
	$(CC) $(CFLAGS) -fno-builtin -Isdk/include -c $< -o $@
build/sdk-demo.elf: build/sdk-demo.o userspace/app.ld
	ld -m elf_x86_64 -T userspace/app.ld $< -o $@
build/efi-guilab.obj: boot/uefi/guilab.c boot/uefi/efi.h gui/surface.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/efi-surface.obj: gui/surface.c gui/surface.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/GUILAB.EFI: build/efi-guilab.obj build/efi-surface.obj
	$(EFI_LD) $(EFI_LINK) /out:$@ $^
build/GuiLab-GPT.img: build/GUILAB.EFI build/LOGIN.EFI tools/gptdisk.py tools/vault.py
	rm -f $@.tmp
	python3 tools/gptdisk.py $@.tmp build/GUILAB.EFI build/LOGIN.EFI
	mv $@.tmp $@
all: build/GuiLab-GPT.img
.PHONY: ecosystem-host-test gui-lab
gui-lab: build/GuiLab-GPT.img

ecosystem-host-test: all
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Igui -Ikernel tests/ecosystem_host.c gui/surface.c kernel/audit.c -o build/ecosystem_host
	./build/ecosystem_host
	python3 tests/ecosystem_host.py

test: ecosystem-host-test
.PHONY: gui-lab-runtime
# Generates the reference pixels before trying the independent OVMF graphics guest.
gui-lab-runtime: ecosystem-host-test
	python3 tests/gui_lab_boot.py

.PHONY: integration-host-test
build/integration-demo.o: userspace/integration_demo.c sdk/include/swamp.h | build
	$(CC) $(CFLAGS) -fno-builtin -Isdk/include -c $< -o $@
build/integration-demo.elf: build/integration-demo.o userspace/app.ld
	ld -m elf_x86_64 -T userspace/app.ld $< -o $@
integration-host-test: all
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Ikernel tests/integration_host.c kernel/session.c kernel/ranges.c kernel/policy.c -o build/integration_host
	./build/integration_host
	python3 tests/integration_host.py
test: integration-host-test

.PHONY: overlord-host-test
build/overlord-demo.o: userspace/overlord_demo.c sdk/include/swamp.h | build
	$(CC) $(CFLAGS) -fno-builtin -Isdk/include -c $< -o $@
build/overlord-demo.elf: build/overlord-demo.o userspace/app-data.ld
	ld -m elf_x86_64 -T userspace/app-data.ld $< -o $@
overlord-host-test: all
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Ikernel tests/overlord_host.c kernel/process.c kernel/scratch.c kernel/policy.c kernel/ranges.c -o build/overlord_host
	./build/overlord_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Ikernel tests/overlord_storage.c kernel/block.c kernel/ext4.c kernel/elf.c -o build/overlord_storage
	./build/overlord_storage
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -ffunction-sections -fdata-sections -Wl,--gc-sections -DUSER_HOST -Ikernel tests/overlord_dispatch_host.c kernel/user.c kernel/process.c kernel/scratch.c kernel/policy.c kernel/ranges.c kernel/session.c kernel/audit.c -o build/overlord_dispatch_host
	./build/overlord_dispatch_host
test: overlord-host-test
.PHONY: git-index-host-test
git-index-host-test:
	python3 tests/git_index_host.py
test: git-index-host-test

# Farkuad: separate unsigned removable-media live image.
build/live-surface.o: gui/surface.c gui/surface.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/live.o: kernel/live.c kernel/live.h gui/surface.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/desktop.o: kernel/desktop.c kernel/live.h gui/surface.h desktop/window.h filesystem/ramfs/ramfs.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/efi-farkuad.obj: boot/uefi/kernellab.c boot/uefi/live_menu.h boot/uefi/handoff.h boot/uefi/pci_resource.h boot/uefi/efi.h kernel/bootinfo.h | build
	$(EFI_CC) $(EFI_FLAGS) -DFARKUAD_LIVE=1 -c $< -o $@
build/efi-live-jump.obj: boot/uefi/live_jump.S boot/uefi/lab_jump.S build/kernel.bin build/vault.ext4 kernel/bootinfo.h
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/efi-pci-resource.obj: boot/uefi/pci_resource.c boot/uefi/pci_resource.h boot/uefi/efi.h kernel/bootinfo.h | build
	$(EFI_CC) $(EFI_FLAGS) -c $< -o $@
build/FARKUAD.EFI: build/efi-farkuad.obj build/efi-handoff.obj build/efi-live-jump.obj build/efi-pci-resource.obj
	$(EFI_LD) $(EFI_LINK) /out:$@ $^
build/Farkuad-Live.iso: build/FARKUAD.EFI tools/mkiso.py tools/gptdisk.py
	python3 tools/mkiso.py $@ build/FARKUAD.EFI
build/Farkuad-Live.iso.report: build/Farkuad-Live.iso build/FARKUAD.EFI build/kernel.bin build/kernel.elf build/vault.ext4 tools/image_report.py
	python3 tools/image_report.py build/Farkuad-Live.iso build/FARKUAD.EFI build/kernel.bin build/kernel.elf build/vault.ext4 --output $@
.PHONY: live live-host-test live-runtime
build/Farkuad-Live.iso.sha256: build/Farkuad-Live.iso
	cd build && sha256sum Farkuad-Live.iso > Farkuad-Live.iso.sha256
live: build/Farkuad-Live.iso build/Farkuad-Live.iso.sha256 build/Farkuad-Live.iso.report
all: live
live-host-test: live
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Ikernel tests/live_host.c kernel/live.c gui/surface.c -o build/live_host
	./build/live_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Ikernel tests/ramdisk_host.c kernel/ramdisk.c kernel/block.c kernel/ext4.c -o build/ramdisk_host
	./build/ramdisk_host
	python3 tests/live_image.py
	python3 tests/mkiso_dynamic_host.py
	python3 tests/mklive_dynamic_host.py
test: live-host-test
live-runtime: live
	python3 tests/live_boot.py

.PHONY: live-optical-runtime
live-optical-runtime: live
	python3 tests/live_boot.py --optical
build/ps2.o: CFLAGS += -Os
.PHONY: input-host-test
input-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -DHARDWARE_HOST -DPS2_MOCK -Ikernel tests/input_host.c kernel/ps2.c kernel/keyboard.c kernel/live.c gui/surface.c -o build/input_host
	./build/input_host
	$(CC) -O2 -Wall -Wextra -Werror -fshort-wchar -Iboot/uefi tests/menu_host.c -o build/menu_host
	./build/menu_host
live-host-test: input-host-test
test: input-host-test

# Awakens development build 01
build/window.o: desktop/window.c desktop/window.h kernel/live.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
.PHONY: window-host-test
window-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/window_host.c desktop/window.c kernel/live.c gui/surface.c applications/system/tools.c filesystem/ramfs/ramfs.c kernel/audit.c -o build/window_host
	./build/window_host
test: window-host-test
live-host-test: window-host-test

# Awakens development build 02: volatile file store
build/ramfs-store.o: filesystem/ramfs/ramfs.c filesystem/ramfs/ramfs.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/ramfs_shell.o build/main.o: filesystem/ramfs/ramfs.h
.PHONY: ramfs-host-test
ramfs-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/ramfs_host.c filesystem/ramfs/ramfs.c kernel/ramfs_shell.c -o build/ramfs_host
	./build/ramfs_host
test: ramfs-host-test
live-host-test: ramfs-host-test
.PHONY: lineedit-host-test bios-layout-host-test
lineedit-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -DHARDWARE_HOST -Ikernel tests/lineedit_host.c kernel/lineedit.c kernel/keyboard.c -o build/lineedit_host
	./build/lineedit_host
bios-layout-host-test: build/boot.bin
	python3 tests/bios_layout_host.py
test: lineedit-host-test bios-layout-host-test
live-host-test: lineedit-host-test

# Awakens build 04 — desktop tools
build/desktop-tools.o: applications/system/tools.c applications/system/tools.h filesystem/ramfs/ramfs.h kernel/audit.h kernel/policy.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/window.o build/desktop.o: applications/system/tools.h filesystem/ramfs/ramfs.h kernel/audit.h
.PHONY: tools-host-test
tools-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/tools_host.c desktop/window.c applications/system/tools.c filesystem/ramfs/ramfs.c kernel/audit.c kernel/live.c gui/surface.c -o build/tools_host
	./build/tools_host
test: tools-host-test
live-host-test: tools-host-test

# Awakens build 05 — hierarchical volatile filesystem
.PHONY: ramtree-host-test
ramtree-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/ramtree_host.c filesystem/ramfs/ramfs.c kernel/ramfs_shell.c -o build/ramtree_host
	./build/ramtree_host
test: ramtree-host-test
live-host-test: ramtree-host-test

# Awakens build 06 — capability-gated native GUI output
build/gui-demo.o: userspace/gui_demo.c sdk/include/swamp.h | build
	$(CC) $(CFLAGS) -fno-builtin -Isdk/include -c $< -o $@
build/gui-demo.elf: build/gui-demo.o userspace/app.ld
	ld -m elf_x86_64 -T userspace/app.ld $< -o $@
.PHONY: gui-client-host-test
gui-client-host-test: all
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -ffunction-sections -fdata-sections -Wl,--gc-sections -DUSER_HOST -Ikernel tests/gui_client_host.c kernel/user.c kernel/process.c kernel/scratch.c kernel/policy.c kernel/ranges.c kernel/session.c kernel/audit.c kernel/ext4.c kernel/block.c kernel/elf.c -o build/gui_client_host
	./build/gui_client_host
test: gui-client-host-test
live-host-test: gui-client-host-test

# Awakens build 07 — read-only hardware inventory, RTC and USB boot HID codecs
OBJ += build/usb-boot-hid.o build/rtc-driver.o
# kernel.elf prerequisite expansion happened above, so explicitly add these.
build/kernel.elf: build/usb-boot-hid.o build/rtc-driver.o
build/usb-boot-hid.o: drivers/usb/hid/boot_hid.c drivers/usb/hid/boot_hid.h kernel/lineedit.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/rtc-driver.o: drivers/rtc/rtc.c drivers/rtc/rtc.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/main.o: drivers/usb/hid/boot_hid.h drivers/rtc/rtc.h
.PHONY: usb-rtc-host-test
usb-rtc-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -DRTC_HOST tests/usb_rtc_host.c drivers/usb/hid/boot_hid.c drivers/rtc/rtc.c kernel/device.c -o build/usb_rtc_host
	./build/usb_rtc_host
test: usb-rtc-host-test
live-host-test: usb-rtc-host-test

# Awakens build 08 — expanded volatile storage and read-only import
OBJ += build/ramfs-import.o
build/kernel.elf: build/ramfs-import.o
build/ramfs-import.o: filesystem/ramfs/import.c filesystem/ramfs/import.h filesystem/ramfs/ramfs.h kernel/ext4.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/storage.o: filesystem/ramfs/import.h filesystem/ramfs/ramfs.h
.PHONY: ramfs-volume-host-test
ramfs-volume-host-test: build/vault.ext4
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/ramfs_volume_host.c filesystem/ramfs/ramfs.c filesystem/ramfs/import.c kernel/ext4.c kernel/block.c -o build/ramfs_volume_host
	./build/ramfs_volume_host
test: ramfs-volume-host-test
live-host-test: ramfs-volume-host-test

# Awakens build 09 — bounded read-only ATA LBA28/LBA48 transport
OBJ += build/ata-pio.o
build/kernel.elf: build/ata-pio.o
build/ata.o: drivers/storage/ata/pio.h
build/ata-pio.o: drivers/storage/ata/pio.c drivers/storage/ata/pio.h kernel/block.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
.PHONY: ata-host-test
ata-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/ata_host.c drivers/storage/ata/pio.c kernel/block.c -o build/ata_host
	./build/ata_host
test: ata-host-test

# Awakens build 10 — explicit per-run resource snapshot
OBJ += build/resource.o
build/kernel.elf: build/resource.o
build/storage.o: security/onion/resource.h
build/resource.o: security/onion/resource.c security/onion/resource.h kernel/session.h filesystem/ramfs/ramfs.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
.PHONY: resource-host-test
resource-host-test: build/vault.ext4
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/resource_host.c security/onion/resource.c filesystem/ramfs/ramfs.c kernel/ext4.c kernel/block.c kernel/session.c -o build/resource_host
	./build/resource_host
test: resource-host-test
live-host-test: resource-host-test

# Awakens build 11 — chunked native resource inspection
build/resource-scan.o: userspace/resource_scan.c sdk/include/swamp.h | build
	$(CC) $(CFLAGS) -fno-builtin -Isdk/include -c $< -o $@
build/resource-scan.elf: build/resource-scan.o userspace/app.ld
	ld -m elf_x86_64 -T userspace/app.ld $< -o $@
.PHONY: resource-scan-host-test
resource-scan-host-test: build/vault.ext4
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Itests/resource_mock -D_start=resource_scan_entry -c userspace/resource_scan.c -o build/resource-scan-host.o
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/resource_scan_host.c build/resource-scan-host.o kernel/session.c kernel/ext4.c kernel/block.c kernel/elf.c -o build/resource_scan_host
	./build/resource_scan_host
test: resource-scan-host-test
live-host-test: resource-scan-host-test

# Awakens build 12 — per-inode capability ceilings
.PHONY: guard-host-test
guard-host-test: build/vault.ext4
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/guard_host.c kernel/guard_ui.c desktop/window.c applications/system/tools.c kernel/live.c gui/surface.c kernel/guard.c kernel/storage.c kernel/persist.c kernel/vfs_note.c kernel/audit.c kernel/ext4.c kernel/elf.c kernel/block.c kernel/gpt.c kernel/vfs.c kernel/ramdisk.c security/onion/resource.c filesystem/ramfs/ramfs.c filesystem/ramfs/import.c -o build/guard_host
	./build/guard_host
test: guard-host-test
live-host-test: guard-host-test

# Awakens build 14 — paused native GUI client
.PHONY: gui-async-host-test
gui-async-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -ffunction-sections -fdata-sections -Wl,--gc-sections -DUSER_HOST -Ikernel tests/gui_async_host.c kernel/user.c kernel/process.c kernel/scratch.c kernel/policy.c kernel/ranges.c kernel/session.c kernel/audit.c -o build/gui_async_host
	./build/gui_async_host
test: gui-async-host-test
live-host-test: gui-async-host-test


# Awakens build 15 — volume-scoped SwampGuard identities (EXT4 UUID + inode).
.PHONY: volume-guard-host-test
volume-guard-host-test: build/vault.ext4
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Ikernel tests/volume_guard_host.c kernel/guard.c kernel/ext4.c kernel/block.c kernel/vfs.c -o build/volume_guard_host
	./build/volume_guard_host
test: volume-guard-host-test
live-host-test: volume-guard-host-test

# Awakens build 16 — xHCI command/event ring transport foundation.
OBJ += build/xhci-ring.o
build/xhci-ring.o: drivers/usb/xhci/ring.c drivers/usb/xhci/ring.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-ring.o
.PHONY: xhci-ring-host-test
xhci-ring-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_ring_host.c drivers/usb/xhci/ring.c -o build/xhci_ring_host
	./build/xhci_ring_host
test: xhci-ring-host-test
live-host-test: xhci-ring-host-test

# Awakens build 17 — bounded xHCI MMIO controller lifecycle (mock-only backend).
OBJ += build/xhci-controller.o
build/xhci-controller.o: drivers/usb/xhci/controller.c drivers/usb/xhci/controller.h drivers/usb/xhci/ring.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-controller.o
.PHONY: xhci-controller-host-test
xhci-controller-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_controller_host.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c -o build/xhci_controller_host
	./build/xhci_controller_host
test: xhci-controller-host-test
live-host-test: xhci-controller-host-test

# Awakens build 18 — addressed-device USB boot-HID enumeration (control callback).
OBJ += build/usb-enumeration.o
build/usb-enumeration.o: drivers/usb/core/enumeration.c drivers/usb/core/enumeration.h drivers/usb/hid/boot_hid.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/usb-enumeration.o
.PHONY: usb-enum-host-test
usb-enum-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/usb_enum_host.c drivers/usb/core/enumeration.c drivers/usb/hid/boot_hid.c -o build/usb_enum_host
	./build/usb_enum_host
test: usb-enum-host-test
live-host-test: usb-enum-host-test

# Awakens build 19 — EP0 control transfer TRBs and completion state.
OBJ += build/xhci-control.o
build/xhci-control.o: drivers/usb/xhci/control.c drivers/usb/xhci/control.h drivers/usb/xhci/ring.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-control.o
.PHONY: xhci-control-host-test
xhci-control-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_control_host.c drivers/usb/xhci/control.c -o build/xhci_control_host
	./build/xhci_control_host
test: xhci-control-host-test
live-host-test: xhci-control-host-test

# Awakens build 20 — bridge USB enumeration and EP0 xHCI transport.
OBJ += build/xhci-control-transport.o
build/xhci-control-transport.o: drivers/usb/xhci/control_transport.c drivers/usb/xhci/control_transport.h drivers/usb/xhci/control.h drivers/usb/xhci/controller.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-control-transport.o
.PHONY: xhci-ep0-host-test
xhci-ep0-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_ep0_host.c drivers/usb/xhci/control_transport.c drivers/usb/xhci/control.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c drivers/usb/core/enumeration.c drivers/usb/hid/boot_hid.c -o build/xhci_ep0_host
	./build/xhci_ep0_host
test: xhci-ep0-host-test
live-host-test: xhci-ep0-host-test

# Awakens build 21 — root-port device contexts and single-slot command lifecycle.
OBJ += build/xhci-device.o
build/xhci-device.o: drivers/usb/xhci/device.c drivers/usb/xhci/device.h drivers/usb/xhci/control_transport.h drivers/usb/xhci/controller.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-device.o
.PHONY: xhci-device-host-test
xhci-device-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_device_host.c drivers/usb/xhci/device.c drivers/usb/xhci/control_transport.c drivers/usb/xhci/control.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c -o build/xhci_device_host
	./build/xhci_device_host
test: xhci-device-host-test
live-host-test: xhci-device-host-test

# Awakens build 22 — narrow USB2 root-port reset with explicit controller ownership.
OBJ += build/xhci-port.o
build/xhci-port.o: drivers/usb/xhci/port.c drivers/usb/xhci/port.h drivers/usb/xhci/controller.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-port.o
.PHONY: xhci-port-host-test
xhci-port-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_port_host.c drivers/usb/xhci/port.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c -o build/xhci_port_host
	./build/xhci_port_host
test: xhci-port-host-test
live-host-test: xhci-port-host-test

# Awakens build 23 — USB2 interrupt IN endpoint context programming.
OBJ += build/xhci-interrupt-context.o
build/xhci-interrupt-context.o: drivers/usb/xhci/interrupt_context.c drivers/usb/xhci/interrupt_context.h drivers/usb/xhci/device.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-interrupt-context.o
.PHONY: xhci-interrupt-context-host-test
xhci-interrupt-context-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_interrupt_context_host.c drivers/usb/xhci/interrupt_context.c drivers/usb/xhci/device.c drivers/usb/xhci/control_transport.c drivers/usb/xhci/control.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c -o build/xhci_interrupt_context_host
	./build/xhci_interrupt_context_host
test: xhci-interrupt-context-host-test
live-host-test: xhci-interrupt-context-host-test

# Awakens build 24 — bounded interrupt IN transfer ring.
OBJ += build/xhci-interrupt.o
build/xhci-interrupt.o: drivers/usb/xhci/interrupt.c drivers/usb/xhci/interrupt.h drivers/usb/xhci/interrupt_context.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-interrupt.o
.PHONY: xhci-interrupt-host-test
xhci-interrupt-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_interrupt_host.c drivers/usb/xhci/interrupt.c -o build/xhci_interrupt_host
	./build/xhci_interrupt_host
test: xhci-interrupt-host-test
live-host-test: xhci-interrupt-host-test

# Awakens build 25 — nonblocking interrupt-IN HID bridge, model only.
OBJ += build/xhci-hid-transport.o
build/xhci-hid-transport.o: drivers/usb/xhci/hid_transport.c drivers/usb/xhci/hid_transport.h drivers/usb/xhci/interrupt.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-hid-transport.o
.PHONY: xhci-hid-host-test
xhci-hid-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_hid_host.c drivers/usb/xhci/hid_transport.c drivers/usb/xhci/interrupt.c drivers/usb/xhci/interrupt_context.c drivers/usb/xhci/device.c drivers/usb/xhci/control_transport.c drivers/usb/xhci/control.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c drivers/usb/hid/boot_hid.c -o build/xhci_hid_host
	./build/xhci_hid_host
test: xhci-hid-host-test
live-host-test: xhci-hid-host-test

# Awakens build 26 — full-speed EP0 Max Packet Size probe and Evaluate Context.
.PHONY: xhci-ep0-probe-host-test
xhci-ep0-probe-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_ep0_probe_host.c drivers/usb/xhci/device.c drivers/usb/xhci/control_transport.c drivers/usb/xhci/control.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c -o build/xhci_ep0_probe_host
	./build/xhci_ep0_probe_host
test: xhci-ep0-probe-host-test
live-host-test: xhci-ep0-probe-host-test

# Awakens build 27 — end-to-end single-interface USB2 HID session.
OBJ += build/xhci-boot-session.o
build/xhci-boot-session.o: drivers/usb/xhci/boot_session.c drivers/usb/xhci/boot_session.h drivers/usb/xhci/hid_transport.h drivers/usb/xhci/port.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-boot-session.o
.PHONY: xhci-boot-session-host-test
xhci-boot-session-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_boot_session_host.c drivers/usb/xhci/boot_session.c drivers/usb/xhci/hid_transport.c drivers/usb/xhci/interrupt.c drivers/usb/xhci/interrupt_context.c drivers/usb/xhci/device.c drivers/usb/xhci/control_transport.c drivers/usb/xhci/control.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c drivers/usb/xhci/port.c drivers/usb/core/enumeration.c drivers/usb/hid/boot_hid.c -o build/xhci_boot_session_host
	./build/xhci_boot_session_host
test: xhci-boot-session-host-test
live-host-test: xhci-boot-session-host-test

# Awakens build 28 — contiguous, aligned, pinned DMA lease allocator.
.PHONY: dma-pool-host-test
dma-pool-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -DVM_HOST -Ikernel tests/dma_pool_host.c kernel/vm.c kernel/pmm.c kernel/ranges.c -o build/dma_pool_host
	./build/dma_pool_host
test: dma-pool-host-test
live-host-test: dma-pool-host-test

# Awakens build 29 — PCI BAR and bounded UC MMIO mapping foundation.
OBJ += build/xhci-pci-resource.o
build/xhci-pci-resource.o: drivers/usb/xhci/pci_resource.c drivers/usb/xhci/pci_resource.h kernel/pci.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-pci-resource.o
.PHONY: mmio-resource-host-test
mmio-resource-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -DVM_HOST -Ikernel tests/mmio_resource_host.c kernel/vm.c kernel/pmm.c kernel/ranges.c drivers/usb/xhci/pci_resource.c -o build/mmio_resource_host
	./build/mmio_resource_host
test: mmio-resource-host-test
live-host-test: mmio-resource-host-test

# Awakens build 30 — explicit xHCI legacy ownership protocol.
OBJ += build/xhci-ownership.o
build/xhci-ownership.o: drivers/usb/xhci/ownership.c drivers/usb/xhci/ownership.h drivers/usb/xhci/controller.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/kernel.elf: build/xhci-ownership.o
.PHONY: xhci-ownership-host-test
xhci-ownership-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_ownership_host.c drivers/usb/xhci/ownership.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c -o build/xhci_ownership_host
	./build/xhci_ownership_host
test: xhci-ownership-host-test
live-host-test: xhci-ownership-host-test

# Awakens Build 32 — read-only runtime xHCI PCI inventory and BAR decode.
OBJ += build/xhci-pci-inventory.o
build/xhci-pci-inventory.o: drivers/usb/xhci/pci_inventory.c drivers/usb/xhci/pci_inventory.h kernel/pci.h kernel/bootinfo.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/hardware.o: drivers/usb/xhci/pci_inventory.h kernel/pci.h kernel/bootinfo.h
build/kernel.elf: build/xhci-pci-inventory.o
.PHONY: xhci-pci-inventory-host-test
xhci-pci-inventory-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Ikernel tests/xhci_pci_inventory_host.c drivers/usb/xhci/pci_inventory.c -o build/xhci_pci_inventory_host
	./build/xhci_pci_inventory_host
test: xhci-pci-inventory-host-test
live-host-test: xhci-pci-inventory-host-test

# Awakens Build 33 — UEFI PCI I/O BAR resource handoff (read-only metadata).
.PHONY: uefi-pci-resource-host-test
uefi-pci-resource-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -fshort-wchar -Iboot/uefi tests/uefi_pci_resource_host.c boot/uefi/pci_resource.c -o build/uefi_pci_resource_host
	./build/uefi_pci_resource_host
test: uefi-pci-resource-host-test
live-host-test: uefi-pci-resource-host-test

# Awakens Build 34 — complete physical-range handoff for MMIO/RAM alias checks.
.PHONY: uefi-handoff-host-test
uefi-handoff-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fshort-wchar -Iboot/uefi tests/uefi_handoff_host.c boot/uefi/handoff.c -o build/uefi_handoff_host
	./build/uefi_handoff_host
test: uefi-handoff-host-test
live-host-test: uefi-handoff-host-test

# Awakens Build 34 — read-only xHCI capability register probe.
.PHONY: xhci-caps-probe-host-test
xhci-caps-probe-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/xhci_caps_probe_host.c drivers/usb/xhci/controller.c drivers/usb/xhci/ring.c -o build/xhci_caps_probe_host
	./build/xhci_caps_probe_host
test: xhci-caps-probe-host-test
live-host-test: xhci-caps-probe-host-test

# Build 34 command depends on the existing xHCI capability reader and VM mapping API.
build/hardware.o: drivers/usb/xhci/controller.h kernel/vm.h
boot-host-test: uefi-handoff-host-test

# Awakens Build 35 — ACPI DMAR/VT-d topology inventory (no IOMMU enable).
.PHONY: acpi-dmar-host-test
acpi-dmar-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/acpi_dmar_host.c kernel/acpi.c -o build/acpi_dmar_host
	./build/acpi_dmar_host
test: acpi-dmar-host-test
live-host-test: acpi-dmar-host-test


# Awakens Build 36: retained desktop scene and cursor presentation.
build/compositor.o: desktop/compositor.c desktop/compositor.h desktop/window.h kernel/live.h gui/surface.h | build
	$(CC) $(CFLAGS) -Os -c $< -o $@
build/desktop.o: desktop/compositor.h
build/window.o: gui/surface.h
.PHONY: compositor-host-test
compositor-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/compositor_host.c desktop/compositor.c desktop/window.c kernel/live.c gui/surface.c applications/system/tools.c filesystem/ramfs/ramfs.c kernel/audit.c -o build/compositor_host
	./build/compositor_host
live-host-test test: compositor-host-test

.PHONY: live-classic-runtime live-recovery-runtime live-iommu-runtime
live-classic-runtime: live
	python3 tests/live_boot.py --optical --profile classic
live-recovery-runtime: live
	python3 tests/live_boot.py --optical --profile recovery
live-iommu-runtime: live
	python3 tests/live_boot.py --profile iommu

# Build 37: opt-in Q35 VT-d and USB boot HID.
$(filter build/xhci-%.o,$(OBJ)) build/usb_input.o: $(wildcard drivers/usb/xhci/*.h) drivers/usb/hid/boot_hid.h drivers/usb/core/enumeration.h
build/usb_input.o build/vtd.o: CFLAGS += -Os
.PHONY: live-usb-input-runtime
live-usb-input-runtime: live
	python3 tests/usb_input_guest.py pair
	python3 tests/usb_input_guest.py keyboard
	python3 tests/usb_input_guest.py mouse
	python3 tests/usb_input_guest.py no-iommu
	python3 tests/usb_input_guest.py no-opt-in
	python3 tests/usb_input_guest.py three-devices

.PHONY: live-usb-desktop
live-usb-desktop: live
	python3 tests/usb_input_guest.py pair --interactive

build/fixture_ahci.o build/fixture_nvme.o build/fixture_disk.o build/persist.o build/vfs_note.o: CFLAGS += -Os
.PHONY: live-storage-runtime
live-storage-runtime: live
	python3 tests/persist_guest.py

.PHONY: note-host-test live-storage-desktop
note-host-test: | build
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/note_host.c kernel/vfs_note.c kernel/block.c -o build/note_host
	./build/note_host
test: note-host-test
STORAGE_CONTROLLER ?= nvme
live-storage-desktop: live
	python3 tests/persist_guest.py $(STORAGE_CONTROLLER) --interactive
