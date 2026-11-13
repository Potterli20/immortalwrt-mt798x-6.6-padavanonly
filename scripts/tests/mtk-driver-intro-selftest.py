#!/usr/bin/env python3
"""Gate the 8-step private-driver introduction flow with host-side checks.

Verifies the template package against the rules recorded in
driver-reuse-assessment.md section 4.1/4.2. No kernel build is executed here;
the real compile/packaging checks are done by the build steps themselves.
"""
import argparse
import json
import re
import tempfile
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source-tree', type=Path,
               default=Path(__file__).resolve().parents[2])
p.add_argument('--package', default='package/mtk/drivers/mtk-driver-template')
a = p.parse_args()
root = a.source_tree.resolve()
pkg = root / a.package
checks = 0
failures = []


def check(ok, label):
    global checks
    checks += 1
    if not ok:
        failures.append(label)


mk = (pkg / 'Makefile').read_text()
cfg = (pkg / 'Config.in').read_text()
srcmk = (pkg / 'src/Makefile').read_text()
csrc = (pkg / 'src/mtk-driver-template.c').read_text()

# Step 3: package metadata must be complete and opt-in.
check('include $(INCLUDE_DIR)/kernel.mk' in mk, 'kernel.mk included')
check(re.search(r'^define KernelPackage/mtk-driver-template$', mk, re.M) is not None,
      'KernelPackage defined')
check(re.search(r'^  FILES:=\$\(PKG_BUILD_DIR\)/mtk-driver-template\.ko$', mk, re.M) is not None,
      'FILES points at the real .ko')
check(re.search(r'^  DEFAULT:=n$', mk, re.M) is not None, 'opt-in by default')
check('AUTOLOAD:=' in mk, 'AUTOLOAD declared')
check('source "$(SOURCE)/Config.in"' in mk, 'Config.in sourced')

# Step 3/4: every symbol that changes the build must be in PKG_CONFIG_DEPENDS
# and EXTRA_KCONFIG, and the Config.in symbol must be visible (no hidden gate).
check('PKG_CONFIG_DEPENDS:=CONFIG_MTK_DRIVER_TEMPLATE_PROBE' in mk,
      'build-affecting symbol hashed into configured stamp')
check('CONFIG_MTK_DRIVER_TEMPLATE_PROBE=$(CONFIG_MTK_DRIVER_TEMPLATE_PROBE)' in mk,
      'symbol injected via EXTRA_KCONFIG')
check(re.search(r'^config MTK_DRIVER_TEMPLATE_PROBE\s*\n\tbool "', cfg, re.M) is not None,
      'Config.in symbol is visible (prompt)')
check(re.search(r'^\tdefault y$', cfg, re.M) is not None, 'Config.in has a default')

# Step 5: external module build uses the kernel's M= and the .c guards on the
# injected symbol.
check('M="$(PKG_BUILD_DIR)"' in mk, 'external module build uses M=')
check('obj-m += mtk-driver-template.o' in srcmk, 'obj-m declared')
check('#ifdef CONFIG_MTK_DRIVER_TEMPLATE_PROBE' in csrc,
      'source consumes the injected symbol')
check('MODULE_LICENSE("GPL")' in csrc, 'module license declared')
check('module_init(' in csrc and 'module_exit(' in csrc,
      'init/exit hooks present')

# Step 4: solve the real Config.in with the tree's kconfig tool. A promptless
# bool is silently discarded at defconfig time, so pinning it cannot replace a
# default in Config.in. Here the symbol HAS a prompt, so a hand pin must hold.
conf = root / 'scripts/config/conf'
with tempfile.TemporaryDirectory(prefix='mtk-intro-') as tmp:
    if not conf.exists():
        check(False, 'scripts/config/conf present')
    else:
        t = Path(tmp)
        # External symbols referenced by the Config.in fragment.
        ext = ('config PACKAGE_kmod-mtk-driver-template\n'
               '\tbool "kmod-mtk-driver-template"\n\tdefault n\n\n')
        (t / 'Kconfig').write_text(ext + cfg)
        for label, deffile_content in (
                ('default', 'CONFIG_PACKAGE_kmod-mtk-driver-template=y\n'),
                ('pinned_off', 'CONFIG_PACKAGE_kmod-mtk-driver-template=y\n'
                 '# CONFIG_MTK_DRIVER_TEMPLATE_PROBE is not set\n')):
            d = t / (label + '.defconfig')
            out = t / (label + '.config')
            d.write_text(deffile_content)
            r = subprocess.run([str(conf), '--silent', '--defconfig=' + str(d),
                                '-w', str(out), str(t / 'Kconfig')],
                               capture_output=True, text=True)
            solved = out.read_text() if out.exists() else ''
            expect_y = label == 'default'
            got_y = 'CONFIG_MTK_DRIVER_TEMPLATE_PROBE=y' in solved
            check(r.returncode == 0 and got_y == expect_y,
                  'kconfig solve ' + label + ' (expect probe=' + str(expect_y) + ')')

print(json.dumps({'checks': checks, 'failures': len(failures),
                  'failed': failures, 'scope': 'package metadata, kconfig gate '
                  'solving and source contract; no MMIO or hardware behaviour'},
                 ensure_ascii=False))
raise SystemExit(1 if failures else 0)
