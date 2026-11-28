#!/usr/bin/env python3
"""Compare a checkpoint to the original ZIP without extracting large baseline images."""
import hashlib, sys, zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
exclude={'TREE.txt','DIFF-REPORT.md'}
with zipfile.ZipFile(sys.argv[1]) as z:
    old={str(Path(i.filename).relative_to(Path(i.filename).parts[0])):i for i in z.infolist() if not i.is_dir()}
    new={p.relative_to(root).as_posix():p for p in root.rglob('*') if p.is_file() and '.git' not in p.parts and '__pycache__' not in p.parts}
    oldsrc={k:v for k,v in old.items() if not k.startswith('build/') and k not in exclude}
    newsrc={k:v for k,v in new.items() if not k.startswith('build/') and k not in exclude}
    removed=sorted(oldsrc.keys()-newsrc.keys()); added=sorted(newsrc.keys()-oldsrc.keys())
    changed=sorted(k for k in oldsrc.keys()&newsrc.keys() if hashlib.sha256(z.read(oldsrc[k])).digest()!=hashlib.sha256(newsrc[k].read_bytes()).digest())
    report=['# Checkpoint delta against Farkuad 0.13.1','',f'Baseline source/docs paths: {len(oldsrc)}; checkpoint: {len(newsrc)}. Build products and generated reports excluded.','',f'Added: {len(added)}; changed: {len(changed)}; removed: {len(removed)}.','']
    for title,paths in [('Added',added),('Changed',changed),('Removed',removed)]:
        report+=['## '+title]+(['- `'+p+'`' for p in paths] or ['None'])+['']
    report+=['Path retention is not a functional equivalence proof; see TEST-REPORT.md.']
    (root/'DIFF-REPORT.md').write_text('\n'.join(report)+'\n')
    if removed:raise SystemExit('Missing baseline paths: '+', '.join(removed))
    print(f'PASS: {len(added)} added, {len(changed)} changed, no removed source/doc paths')
