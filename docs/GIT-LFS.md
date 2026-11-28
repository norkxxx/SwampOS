# Git LFS release workflow

The repository tracks disk images using LFS. Build intermediates, test executables, private keys, logs and other generated files remain ignored. Full release ZIPs include all artifacts and logs. LFS is not installed automatically by these scripts; install Git LFS first, then run `bash scripts/git-lfs-setup.sh` or `powershell -File scripts/git-lfs-setup.ps1`.

For an existing clean Git branch with images previously tracked as ordinary files, use `git lfs migrate import` on the intended **unpublished** commits; tracking patterns alone do not rewrite history. Create a backup ref first. Do not rewrite shared branches without coordination. Review `git lfs ls-files`, `git lfs fsck` and the staged diff before pushing. If prior build artifacts were already tracked, `git rm -r --cached --ignore-unmatch build/` followed by `git add -A` re-stages only the intended build files under these ignore rules.

The full release `build/SHA256SUMS` lists 20 compiled/image artifacts; the tracked `build/IMAGE-SHA256SUMS` lists only seven disk images including Farkuad-Live.iso. CI checks out LFS data, then cleans and rebuilds before testing. LFS transfer consumes account quotas; consider attaching compressed release ZIPs instead for public distribution.

Local validation checks Git attribute and ignore rules. Git LFS executable, server upload, and remote CI execution were not available locally.

## Overlord index preflight
After `git add -A`, run `python3 scripts/check-git-index.py` and `git lfs fsck`. The script rejects raw Git blobs above 100 MiB and tracked image files without canonical staged LFS pointers. It examines the index only, not older commits or remote quota; use `git lfs migrate import` if an unpublished branch already contains oversized historical blobs.

The primary Live distribution is now `build/Farkuad-Live.iso` (LFS tracked). The old `build/Farkuad-Live.img` is ignored; remove it from the index if it was previously committed.
