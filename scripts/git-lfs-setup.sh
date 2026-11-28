#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
git lfs version
git lfs install --local
for pattern in '*.img' '*.iso' '*.ext4' '*.raw' '*.qcow2' '*.vmdk' '*.vdi' '*.rom'; do
 git lfs track "$pattern"
done
git check-attr filter -- build/SwampBoot-GPT.img
git lfs status
printf '%s\n' 'No commits or history were changed. Stage files and review git lfs ls-files before pushing.'
