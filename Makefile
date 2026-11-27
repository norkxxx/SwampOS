CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -Wall -Wextra -Werror -O2
.PHONY: all run test clean
all: build/SwampOS.img
build:
	mkdir -p build
build/boot.bin: boot/boot.S | build
	as --32 $< -o build/boot.o
	ld -m elf_i386 -Ttext 0x7c00 --oformat binary build/boot.o -o $@
build/kernel.elf: kernel/entry.S kernel/main.c linker.ld | build
	$(CC) $(CFLAGS) -c kernel/entry.S -o build/entry.o
	$(CC) $(CFLAGS) -c kernel/main.c -o build/main.o
	ld -m elf_i386 -T linker.ld build/entry.o build/main.o -o $@
build/kernel.bin: build/kernel.elf
	objcopy -O binary $< $@
build/SwampOS.img: build/boot.bin build/kernel.bin
	python3 scripts/image.py
run: all
	qemu-system-x86_64 -drive format=raw,file=build/SwampOS.img -serial stdio
test: all
	python3 tests/test_image.py
clean:
	rm -rf build
