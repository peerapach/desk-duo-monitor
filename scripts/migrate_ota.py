#!/usr/bin/env python3
"""Dry-run-first, settings-preserving migration. Never write the merged image."""
import argparse
import datetime
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

from package_firmware import ROOT, FLASH_BYTES, digest, verify_package


def execute(command):
    # Tool diagnostics are not needed in successful output; never print backup data.
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError("esptool operation failed; no subsequent operation was attempted")
    return result.stdout


def main(argv=None, run=execute):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=pathlib.Path, default=ROOT / 'artifacts')
    parser.add_argument('--port')
    parser.add_argument('--flash', action='store_true')
    parser.add_argument('--skip-backup', action='store_true', help='Explicitly skip all backup reads (user opt-out)')
    parser.add_argument('--backup-dir', type=pathlib.Path, default=ROOT / '.device-backups')
    options = parser.parse_args(argv)
    if options.flash and not options.port:
        parser.error('--flash requires --port')
    metadata = json.loads((options.package / 'desk-duo-cyd.json').read_text())
    segments = verify_package(options.package, metadata)
    print('Verified segmented migration; NVS 0x9000–0x1efff and PHY 0x1f000–0x1ffff retained.')
    for offset, file, data in segments:
        print(f'  {offset:#08x} {file.name}: {len(data)} bytes')
    print('Backup: explicitly skipped' if options.skip_backup else 'Backup: fresh private 4 MiB read before any write')
    if not options.flash:
        print('DRY RUN: no device accessed. Add --port PORT --flash to migrate.')
        return 0
    tool = ROOT / '.platformio/packages/tool-esptoolpy/esptool.py'
    base = [sys.executable, str(tool), '--chip', 'esp32', '--port', options.port]
    identity = run(base + ['chip_id'])
    flash = run(base + ['flash_id'])
    if not re.search(r'Chip is ESP32(?:\b|-)', identity) or not re.search(r'Detected flash size:\s*4MB\b', flash):
        raise ValueError('Physical chip/flash must be ESP32 with detected 4MB flash')
    if not options.skip_backup:
        options.backup_dir.mkdir(parents=True, exist_ok=True, mode=0o700)
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ-')
        private = pathlib.Path(tempfile.mkdtemp(prefix=stamp, dir=options.backup_dir))
        os.chmod(private, 0o700)
        backup = private / 'flash.bin'
        descriptor = os.open(backup, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        os.close(descriptor)
        run(base + ['read_flash', '0x0', hex(FLASH_BYTES), str(backup)])
        data = backup.read_bytes()
        if len(data) != FLASH_BYTES:
            raise ValueError('Backup is incomplete; refusing migration')
        recovery = {'chip': 'esp32', 'flashBytes': FLASH_BYTES, 'file': backup.name,
                    'sha256': digest(data), 'writeOffset': '0x0', 'createdUtc': stamp,
                    'recovery': 'Restore this exact complete backup only when intentionally recovering the previous full flash layout.'}
        file = private / 'recovery.json'
        with file.open('x') as handle:
            json.dump(recovery, handle, indent=2)
        os.chmod(file, 0o600)
        print(f'Fresh verified backup saved privately: {private}')
    # Revalidate immediately before writing. A changed package cannot slip past
    # the time spent checking hardware / reading the backup.
    segments = verify_package(options.package, metadata)
    command = base + ['write_flash', '--flash_mode', 'dio', '--flash_freq', '40m', '--flash_size', '4MB']
    for offset, file, _ in segments:
        command += [hex(offset), str(file)]
    run(command)  # esptool verifies each written segment's hash before returning.
    print('Segmented migration verified by esptool. Check local boot, settings and firmware status.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
