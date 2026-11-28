#!/bin/sh
set -eu
sudo apt-get update
sudo apt-get install -y gcc gcc-multilib binutils make python3 qemu-system-x86 qemu-system-gui libext2fs-dev python3-cryptography clang lld ovmf
