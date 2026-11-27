#!/bin/sh
set -eu
sudo apt-get update
sudo apt-get install -y gcc gcc-multilib binutils make python3 qemu-system-x86
