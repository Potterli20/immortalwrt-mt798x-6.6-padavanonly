#!/usr/bin/env python3
"""Regression checks distilled from the MedyMa 6.6 private-driver fixes.

Three failure modes were found and fixed on that tree; none of them involves
hardware or a running kernel, so all three can be checked on the host against
this branch's own packages:

1. hidden kconfig bool gates with no default/select that silently resolve to n
   (and cannot be rescued by pinning them in a defconfig fragment);
2. package patches that no longer apply to their vendored source (the 6.6
   mt_wifi_cmn failure: a fix had been folded into src/ but the redundant
   patch was left behind, producing "Hunk #1 FAILED" at prepare time);
3. host selftests that drift from the production functions they extract.

The patch check replays the ordered patch series against a pristine copy of
the tarball in dl/, using `patch --dry-run`, so it never touches build_dir/.
"""
import argparse
import json
import re
import subprocess
import tarfile
import tempfile
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source-tree', type=Path,
               default=Path(__file__).resolve().parents[2])
p.add_argument('--skip-patch-replay', action='store_true',
               help='only run the kconfig/selftest checks (no tarball extract)')
a = p.parse_args()
root = a.source_tree.resolve()
checks = 0
failures = []


def check(ok, label):
    global checks
    checks += 1
    if not ok:
        failures.append(label)


# --- 1) hidden bool gates -------------------------------------------------
hidden = []
pkg_root = root / 'package/mtk'
configs = list(pkg_root.rglob('Config*.in'))
for f in configs:
    txt = f.read_text()
    for m in re.finditer(
            r'^config (\w+)\s*\n((?:\t.*\n|.*\n)*?)'
            r'(?=^config |^menu|^endmenu|^choice|^endchoice|\Z)', txt, re.M):
        name, body = m.group(1), m.group(2)
        if re.search(r'^\s*bool\s+"', body, re.M):      # visible
            continue
        if not re.search(r'^\s*bool\s*$', body, re.M):  # not a bool
            continue
        if re.search(r'^\s*default ', body, re.M):
            continue
        selected = any(re.search(r'^\s*select ' + name + r'\b', c.read_text(),
                                 re.M) for c in configs)
        if selected:
            continue
        referenced = any(('CONFIG_' + name) in g.read_text()
                         for g in list(pkg_root.rglob('Makefile')) +
                         list(pkg_root.rglob('*.mk')))
        if referenced:
            hidden.append((name, str(f)))
check(not hidden, 'no unguarded hidden bool gates: ' + repr(hidden))

# --- 2) ordered patch replay against pristine tarballs -------------------
replay = []
if not a.skip_patch_replay:
    for mk in sorted((root / 'package/mtk/drivers').glob('*/Makefile')):
        text = mk.read_text()
        src = re.search(r'^PKG_SOURCE:=(\S+)', text, re.M)
        if not src:
            continue
        patches = sorted((mk.parent / 'patches').glob('*.patch'))
        if not patches:
            continue
        tarball = root / 'dl' / src.group(1)
        if not tarball.exists():
            replay.append((mk.parent.name, 'tarball missing: ' + str(tarball)))
            continue
        with tempfile.TemporaryDirectory(prefix='drv-replay-') as tmp:
            t = Path(tmp)
            with tarfile.open(tarball) as tf:
                tf.extractall(t, filter='data')
            # tarballs unpack into a single top-level directory
            entries = [e for e in t.iterdir() if e.is_dir()]
            base = entries[0] if len(entries) == 1 else t
            # OpenWrt applies patches in filename order; a later patch may
            # legitimately target lines introduced by an earlier one. Replay
            # the whole series onto ONE pristine copy (real apply, not
            # --dry-run, so the next patch sees the accumulated state).
            failed = []
            for patch in patches:
                r = subprocess.run(['patch', '-p1', '--batch', '--forward',
                                    '-i', str(patch)], cwd=base,
                                   capture_output=True, text=True)
                if r.returncode != 0:
                    lines = (r.stdout + r.stderr).strip().splitlines()
                    failed.append(patch.name + ': ' +
                                  (lines[-1] if lines else 'rc=%d' % r.returncode))
            for e in failed:
                replay.append((mk.parent.name, e))
            if not failed:
                replay.append((mk.parent.name,
                               'OK %d patches' % len(patches)))
for name, result in replay:
    check(result.startswith('OK'), name + ': ' + result)

# --- 3) host selftests present and syntactically valid -------------------
tests = root / 'scripts/tests'
py = sorted(tests.glob('*.py'))
sh = sorted(tests.glob('*.sh'))
check(len(py) >= 1, 'host selftest scripts present')
for f in py:
    r = subprocess.run(['python3', '-m', 'py_compile', str(f)],
                       capture_output=True)
    check(r.returncode == 0, 'py_compile ' + f.name)
for f in sh:
    r = subprocess.run(['sh', '-n', str(f)], capture_output=True)
    check(r.returncode == 0, 'sh -n ' + f.name)

# A selftest that extracts production functions by name must not drift from
# the renamed/removed symbol (the 6.6 mtk-npu-sync-selftest failure: it kept
# looking for mtk_npu_flow_tnl_offload after the refactor to flow_prepare).
# Scan every selftest for `function(<source>, '<symbol>')` calls and, when the
# source resolves to a path under the tree, assert the symbol is still there.
funcall = re.compile(r"function\(\s*[^,]+,\s*['\"](\w+)['\"]\s*\)")
production_files = [p for p in (root / 'package').rglob('*')
                    if p.is_file() and p.suffix in ('.c', '.h', '.lua')]
prod_text = {}
for f in py:
    txt = f.read_text()
    for sym in funcall.findall(txt):
        found = False
        for c in production_files:
            body = prod_text.get(c)
            if body is None:
                body = c.read_text(errors='ignore')
                prod_text[c] = body
            if re.search(r'\b' + re.escape(sym) + r'\s*\(', body):
                found = True
                break
        check(found, 'selftest symbol ' + sym + ' still exists in production')
cache = tests / '__pycache__'
if cache.is_dir():
    import shutil
    shutil.rmtree(cache)

print(json.dumps({'checks': checks, 'failures': len(failures),
                  'failed': failures,
                  'patch_replay': replay,
                  'scope': 'kconfig gate wiring, ordered patch applicability '
                  'against pristine tarballs, host selftest syntax; no '
                  'hardware or kernel build'}, ensure_ascii=False))
raise SystemExit(1 if failures else 0)
