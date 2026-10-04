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


FLASH_BYTES = 4 * 1024 * 1024
GEOMETRY = {
    "nvs": (1, 2, 0x9000, 0x16000), "phy_init": (1, 1, 0x1f000, 0x1000),
    "otadata": (1, 0, 0x20000, 0x2000), "ota_0": (0, 16, 0x30000, 0x1e0000),
    "ota_1": (0, 17, 0x210000, 0x1e0000),
}
SEGMENTS = {"bootloader.bin": (0x1000, 0x7000), "partitions.bin": (0x8000, 0x1000),
            "ota_data_initial.bin": (0x20000, 0x2000), "desk-duo-app.bin": (0x30000, 0x1e0000)}


def validate_partitions(table: bytes, app_offset: int, app_size: int) -> int:
    entries = {}
    for pos in range(0, len(table), 32):
        row = table[pos:pos + 32]
        if row == b"\xff" * 32:
            break
        if row[:2] == b"\xeb\xeb":
            if len(row) != 32 or row[16:] != hashlib.md5(table[:pos]).digest():
                raise ValueError("Partition table checksum mismatch")
            continue
        if len(row) != 32 or row[:2] != b"\xaa\x50":
            raise ValueError("Malformed partition table")
        _, kind, subtype, offset, size, raw_name, flags = struct.unpack("<HBBII16sI", row)
        name = raw_name.split(b"\0", 1)[0].decode("ascii")
        if name in entries or flags:
            raise ValueError("Duplicate or unsupported partition")
        entries[name] = (kind, subtype, offset, size)
    if entries != GEOMETRY:
        raise ValueError("Partition geometry must preserve NVS/PHY and the two approved OTA slots")
    if app_offset != 0x30000 or not 0 < app_size <= 0x1e0000:
        raise ValueError("Application offset or size does not fit OTA 0")
    return 0x1e0000


def validate_application(data: bytes) -> None:
    if len(data) < 288 or data[0] != 0xe9 or not 1 <= data[1] <= 16 or data[12:14] != b"\0\0":
        raise ValueError("Expected an ESP32 application image, not a merged installation package")
    if struct.unpack_from("<I", data, 32)[0] != 0xabcd5432 or data[80:89] != b"desk_duo\0":
        raise ValueError("Application project must be desk_duo")
    position, checksum = 24, 0xef
    for segment in range(data[1]):
        if position + 8 > len(data):
            raise ValueError("Truncated image segment header")
        length = struct.unpack_from("<I", data, position + 4)[0]
        position += 8
        if position + length > len(data) or (segment == 0 and length < 256):
            raise ValueError("Truncated image segment")
        for byte in data[position:position + length]:
            checksum ^= byte
        position += length
    end = (position + 16) & ~15
    if end > len(data) or data[end - 1] != checksum:
        raise ValueError("Application checksum mismatch")
    if data[23] == 1:
        if len(data) != end + 32 or hashlib.sha256(data[:end]).digest() != data[end:]:
            raise ValueError("Application digest mismatch")
    elif data[23] != 0 or len(data) != end:
        raise ValueError("Unexpected application trailer")


def verify_segments(directory: pathlib.Path, segments: list) -> list:
    if len(segments) != len(SEGMENTS) or {s["file"] for s in segments} != set(SEGMENTS):
        raise ValueError("Missing or unexpected migration segments")
    checked = []
    for segment in segments:
        name = segment["file"]
        offset, limit = SEGMENTS[name]
        data = (directory / name).read_bytes()
        if int(segment["offset"], 0) != offset or len(data) != segment["size"] or not 0 < len(data) <= limit or digest(data) != segment["sha256"]:
            raise ValueError("Migration segment metadata or hash mismatch")
        start, end = offset & ~4095, (offset + len(data) + 4095) & ~4095
        if start < 0x20000 and end > 0x9000 or end > FLASH_BYTES:
            raise ValueError("Segment erase range would touch retained NVS/PHY")
        checked.append((offset, directory / name, data))
    checked.sort()
    for first, second in zip(checked, checked[1:]):
        if first[0] + len(first[2]) > second[0]:
            raise ValueError("Overlapping segments")
    blobs = {file.name: data for _, file, data in checked}
    validate_partitions(blobs["partitions.bin"], 0x30000, len(blobs["desk-duo-app.bin"]))
    validate_application(blobs["desk-duo-app.bin"])
    if blobs["desk-duo-app.bin"][48:80].split(b"\0", 1)[0] == b"rollback-probe":
        raise ValueError("Rollback probe is test-only and cannot be packaged as a release")
    if blobs["ota_data_initial.bin"] != b"\xff" * 8192:
        raise ValueError("OTA data must be fully erased for initial slot selection")
    if blobs["bootloader.bin"][0] != 0xe9:
        raise ValueError("Invalid bootloader image")
    return checked


def verify_package(directory: pathlib.Path, metadata: dict) -> list:
    if metadata.get("chip") != "esp32" or metadata.get("flashBytes") != FLASH_BYTES or metadata.get("formatVersion") != 2:
        raise ValueError("Expected a verified ESP32 4 MiB OTA migration package")
    checked = verify_segments(directory, metadata["segments"])
    if metadata.get("image") != "desk-duo-cyd.bin":
        raise ValueError("Unexpected merged package name")
    merged = (directory / metadata["image"]).read_bytes()
    if len(merged) != metadata["size"] or digest(merged) != metadata["sha256"]:
        raise ValueError("Merged package hash mismatch")
    end = 0
    for offset, _, data in checked:
        if merged[end:offset] != b"\xff" * (offset - end) or merged[offset:offset + len(data)] != data:
            raise ValueError("Merged bytes or erased padding do not match segments")
        end = offset + len(data)
    if end != len(merged):
        raise ValueError("Unexpected merged trailer")
    return checked


def main() -> int:
    args = json.loads((BUILD / "flasher_args.json").read_text())
    if args["extra_esptool_args"]["chip"] != "esp32" or args["flash_settings"]["flash_size"] != "4MB":
        raise ValueError("Expected the pinned ESP32 4MB build")
    app = (BUILD / "firmware.bin").read_bytes()
    table = (BUILD / "partitions.bin").read_bytes()
    slot_size = validate_partitions(table, int(args["app"]["offset"], 0), len(app))
    validate_application(app)
    for key, expected in (("bootloader", 0x1000), ("partition-table", 0x8000)):
        if int(args[key]["offset"], 0) != expected:
            raise ValueError("Build segment offset mismatch")
    flash_files = {int(offset, 0): file for offset, file in args["flash_files"].items()}
    if set(flash_files) != {0x1000, 0x8000, 0x20000, 0x30000}:
        raise ValueError("Build is missing OTA initialization or contains unexpected flash segments")
    initial = (BUILD / flash_files[0x20000]).read_bytes()
    if initial != b"\xff" * 8192:
        raise ValueError("Build OTA initialization must be erased")
    OUTPUT.mkdir(exist_ok=True)
    data_by_name = {"bootloader.bin": (BUILD / "bootloader.bin").read_bytes(),
                    "partitions.bin": table, "ota_data_initial.bin": initial, "desk-duo-app.bin": app}
    segments = []
    merged = bytearray(b"\xff" * (0x30000 + len(app)))
    for name, data in data_by_name.items():
        offset, _ = SEGMENTS[name]
        (OUTPUT / name).write_bytes(data)
        merged[offset:offset + len(data)] = data
        segments.append({"offset": hex(offset), "file": name, "size": len(data), "sha256": digest(data)})
    (OUTPUT / "desk-duo-cyd.bin").write_bytes(merged)
    metadata = {"formatVersion": 2, "board": "ESP32-2432S028R", "chip": "esp32", "flashBytes": FLASH_BYTES,
                "image": "desk-duo-cyd.bin", "writeOffset": "0x0", "size": len(merged), "sha256": digest(merged),
                "segments": segments, "otaSlotBytes": slot_size, "applicationBytes": len(app),
                "applicationOnly": {"file": "desk-duo-app.bin", "writeOffset": "0x30000"},
                "initialInstallErasesNvs": True,
                "gitCommit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                "sourceDirty": bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True))}
    verify_package(OUTPUT, metadata)
    (OUTPUT / "desk-duo-cyd.json").write_text(json.dumps(metadata, indent=2) + "\n")
    (OUTPUT / "desk-duo-cyd.sha256").write_text(digest(merged) + "  desk-duo-cyd.bin\n")
    notices = OUTPUT / "notices"
    notices.mkdir(exist_ok=True)
    for file in (ROOT / "THIRD_PARTY_NOTICES.md", ROOT / "firmware/main/fonts/Sarabun-OFL.txt",
                 ROOT / "firmware/main/fonts/Montserrat-OFL.txt", ROOT / "firmware/managed_components/lvgl__lvgl/LICENCE.txt"):
        shutil.copyfile(file, notices / file.name)
    for source, name in ((ROOT / ".platformio/packages/framework-espidf/LICENSE", "ESP-IDF-LICENSE.txt"),
                         (ROOT / ".platformio/packages/framework-espidf/components/json/cJSON/LICENSE", "cJSON-LICENSE.txt")):
        shutil.copyfile(source, notices / name)
    print(json.dumps(metadata, indent=2))
    print("Verified segments, hashes, erase ranges, padding, app identity and checksum; no device accessed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
