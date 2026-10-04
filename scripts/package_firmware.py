#!/usr/bin/env python3
"""Merge and verify actual PlatformIO build outputs; never access a serial device."""
from __future__ import annotations

import hashlib
import json
import pathlib
import shutil
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BUILD = ROOT / "firmware/.pio/build/cyd"
OUTPUT = ROOT / "artifacts"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    args = json.loads((BUILD / "flasher_args.json").read_text())
    if args["extra_esptool_args"]["chip"] != "esp32" or args["flash_settings"]["flash_size"] != "4MB":
        raise SystemExit("Expected the pinned ESP32 4MB build")
    # CMake's generated names describe offsets; PlatformIO writes these final names.
    parts = [(int(args[key]["offset"], 0), BUILD / name) for key, name in (
        ("bootloader", "bootloader.bin"), ("partition-table", "partitions.bin"), ("app", "firmware.bin"))]
    parts.sort()
    app_offset = int(args["app"]["offset"], 0)
    table = (BUILD / "partitions.bin").read_bytes()
    factory_size = None
    for pos in range(0, len(table), 32):
        if table[pos:pos+2] != b"\xaa\x50":
            continue
        _, kind, subtype, offset, size, _, _ = struct.unpack("<HBBII16sI", table[pos:pos+32])
        if kind == 0 and subtype == 0:
            if offset != app_offset:
                raise SystemExit("Factory application offset disagrees with build metadata")
            factory_size = size
    if factory_size is None or (BUILD / "firmware.bin").stat().st_size > factory_size:
        raise SystemExit("Application does not fit the built factory partition")
    previous_end = 0
    for offset, file in parts:
        if offset < previous_end:
            raise SystemExit("Overlapping image segments")
        previous_end = offset + file.stat().st_size
    if previous_end > 4 * 1024 * 1024:
        raise SystemExit("Image exceeds physical flash size")
    OUTPUT.mkdir(exist_ok=True)
    merged = OUTPUT / "desk-duo-cyd.bin"
    tool = ROOT / ".platformio/packages/tool-esptoolpy/esptool.py"
    command = [sys.executable, str(tool), "--chip", "esp32", "merge_bin", "--output", str(merged)]
    for flag in ("flash_mode", "flash_freq", "flash_size"):
        command.extend(["--" + flag, args["flash_settings"][flag]])
    for offset, file in parts:
        command.extend([hex(offset), str(file)])
    subprocess.run(command, check=True)
    data = merged.read_bytes()
    if len(data) != previous_end:
        raise SystemExit("Merged size differs from final segment end")
    segments = []
    for offset, file in parts:
        source = file.read_bytes()
        if data[offset:offset+len(source)] != source:
            raise SystemExit("Merged segment does not exactly match the build")
        segments.append({"offset": hex(offset), "file": file.name, "size": len(source), "sha256": digest(source)})
    # Verify erased padding, including the initial-install NVS region.
    end = 0
    for offset, file in parts:
        if data[end:offset] != b"\xff" * (offset - end):
            raise SystemExit("Unexpected bytes between image segments")
        end = offset + file.stat().st_size
    shutil.copyfile(BUILD / "firmware.bin", OUTPUT / "desk-duo-app.bin")
    notices = OUTPUT / "notices"
    notices.mkdir(exist_ok=True)
    for file in (ROOT / "THIRD_PARTY_NOTICES.md", ROOT / "firmware/main/fonts/Sarabun-OFL.txt",
                 ROOT / "firmware/main/fonts/Montserrat-OFL.txt",
                 ROOT / "firmware/managed_components/lvgl__lvgl/LICENCE.txt"):
        shutil.copyfile(file, notices / file.name)
    for source, name in (
        (ROOT / ".platformio/packages/framework-espidf/LICENSE", "ESP-IDF-LICENSE.txt"),
        (ROOT / ".platformio/packages/framework-espidf/components/json/cJSON/LICENSE", "cJSON-LICENSE.txt"),
    ):
        shutil.copyfile(source, notices / name)
    metadata = {"board": "ESP32-2432S028R", "chip": "esp32", "flashBytes": 4194304,
                "image": merged.name, "writeOffset": "0x0", "size": len(data), "sha256": digest(data),
                "segments": segments, "factoryPartitionBytes": factory_size,
                "applicationOnly": {"file": "desk-duo-app.bin", "writeOffset": hex(app_offset)},
                "initialInstallErasesNvs": True,
                "gitCommit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                "sourceDirty": bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True))}
    (OUTPUT / "desk-duo-cyd.json").write_text(json.dumps(metadata, indent=2) + "\n")
    (OUTPUT / "desk-duo-cyd.sha256").write_text(digest(data) + "  " + merged.name + "\n")
    print(json.dumps(metadata, indent=2))
    print("Verified: all segment bytes, offsets, sizes, erased padding and SHA256; no device accessed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
