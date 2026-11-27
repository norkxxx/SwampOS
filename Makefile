CC = gcc
CFLAGS = -m64 -mno-red-zone -mgeneral-regs-only -fno-unwind-tables -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -Wall -Wextra -Werror -O2
SRC := $(wildcard kernel/*.c) $(wildcard kernel/*.S)
OBJ := $(patsubst kernel/%.c,build/%.o,$(filter %.c,$(SRC))) $(patsubst kernel/%.S,build/%.o,$(filter %.S,$(SRC)))
.PHONY: all run test runtime clean
all: build/SwampOS.img
build:
	mkdir -p build
build/boot.bin: boot/boot.S | build
	as --32 $< -o build/boot.o
	ld -m elf_i386 -Ttext 0x7c00 --oformat binary build/boot.o -o $@
build/%.o: kernel/%.c $(wildcard kernel/*.h) | build
	$(CC) $(CFLAGS) -c $< -o $@
build/%.o: kernel/%.S | build
	$(CC) $(CFLAGS) -c $< -o $@
build/kernel.elf: $(OBJ) linker.ld
	ld -m elf_x86_64 -T linker.ld $(OBJ) -o $@
build/kernel.bin: build/kernel.elf
	objcopy -O binary $< $@
build/SwampOS.img: build/boot.bin build/kernel.bin scripts/image.py
	python3 scripts/image.py
run: all
	qemu-system-x86_64 -m 128M -drive format=raw,file=build/SwampOS.img -serial stdio
test: all
	python3 tests/test_image.py
	$(CC) -O2 -Wall -Wextra -Werror -mno-red-zone -Ikernel tests/task_host.c kernel/task.c kernel/task_switch.S -o build/task_host
	./build/task_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -Ikernel tests/pmm_host.c kernel/pmm.c kernel/ranges.c kernel/policy.c -o build/pmm_host
	./build/pmm_host
	$(CC) -O2 -Wall -Wextra -Werror -fsanitize=undefined -DVM_HOST -Ikernel tests/vm_host.c kernel/vm.c kernel/pmm.c kernel/ranges.c -o build/vm_host
	./build/vm_host
runtime: all
	python3 tests/boot.py
clean:
	rm -rf build
