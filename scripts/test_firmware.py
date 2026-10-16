#!/usr/bin/env python3
"""Build and run the portable firmware core tests with the cached ESP-IDF cJSON."""

from __future__ import annotations

import os
import pathlib
import shlex
import shutil
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]


def find_sdk() -> pathlib.Path:
    candidates = []
    core_dir = os.environ.get("PLATFORMIO_CORE_DIR")
    if core_dir:
        candidates.append(pathlib.Path(core_dir) / "packages/framework-espidf/components/json/cJSON")
    candidates.append(ROOT / ".platformio/packages/framework-espidf/components/json/cJSON")
    for candidate in candidates:
        if (candidate / "cJSON.c").is_file() and (candidate / "cJSON.h").is_file():
            return candidate
    raise SystemExit("SDK cJSON was not found; install the project ESP-IDF platform with PlatformIO first")


def compiler(name: str) -> list[str]:
    configured = os.environ.get("CC" if name == "cc" else "CXX")
    command = shlex.split(configured) if configured else [name]
    if not command or shutil.which(command[0]) is None:
        raise SystemExit(f"required compiler is unavailable: {configured or name}")
    return command


def run(command: list[str]) -> None:
    print("+ " + shlex.join(command), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> int:
    sdk = find_sdk()
    output = ROOT / ".platformio/native-tests"
    output.mkdir(parents=True, exist_ok=True)
    cjson_object = output / "cJSON.o"
    test_binary = output / "firmware_core_test"
    run(compiler("cc") + ["-std=c99", "-O0", "-g", "-I", str(sdk), "-c", str(sdk / "cJSON.c"), "-o", str(cjson_object)])
    run(compiler("c++") + ["-std=c++17", "-O0", "-g", "-Wall", "-Wextra", "-Werror", "-I", str(sdk),
                           str(ROOT / "firmware/main/core.cpp"), str(ROOT / "tests/firmware_core_test.cpp"),
                           str(cjson_object), "-o", str(test_binary)])
    run([str(test_binary), str(ROOT)])
    policy = output / "brightness_policy_test"
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(sdk),
                           str(ROOT / "firmware/main/core.cpp"), str(ROOT / "firmware/main/brightness_policy.cpp"),
                           str(ROOT / "tests/brightness_policy_test.cpp"), str(cjson_object), "-o", str(policy)])
    run([str(policy)])
    diagnostics = output / "weather_diagnostics_test"
    diagnostic_sources = [str(ROOT / "firmware/main/weather_diagnostics.cpp")]
    run(compiler("c++") + ["-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror", "-I", str(sdk)] + diagnostic_sources +
        [str(ROOT / "tests/weather_diagnostics_test.cpp"), str(cjson_object), "-o", str(diagnostics)])
    run([str(diagnostics)])
    transport = output / "weather_transport_diagnostics_test"
    transport_sources = [str(ROOT / "firmware/main/weather_transport_diagnostics.cpp")]
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "tests/weather_sdk")] + transport_sources +
        [str(ROOT / "firmware/main/weather_diagnostics.cpp"), "-I", str(sdk), str(ROOT / "tests/weather_transport_diagnostics_test.cpp"), str(cjson_object), "-o", str(transport)])
    run([str(transport)])
    connect = output / "weather_connect_test"
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "tests/weather_sdk"),
                           "-I", str(sdk), str(ROOT / "firmware/main/core.cpp"), str(ROOT / "firmware/main/weather_connect.cpp"),
                           str(ROOT / "firmware/main/weather_transport_diagnostics.cpp"),
                           str(ROOT / "firmware/main/weather_diagnostics.cpp"),
                           str(ROOT / "tests/weather_connect_test.cpp"), str(cjson_object), "-o", str(connect)])
    run([str(connect)])
    backlight = output / "backlight_test"
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "tests/backlight_sdk"),
                           str(ROOT / "firmware/main/backlight.cpp"), str(ROOT / "firmware/main/brightness_policy.cpp"), str(ROOT / "tests/backlight_test.cpp"), "-o", str(backlight)])
    run([str(backlight)])
    runtime = output / "runtime_display_test"
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "tests/runtime_sdk"), "-I", str(sdk),
                           str(ROOT / "firmware/main/core.cpp"), str(ROOT / "firmware/main/runtime.cpp"),
                           str(ROOT / "tests/runtime_display_test.cpp"), str(cjson_object), "-o", str(runtime)])
    run([str(runtime)])
    touch = output / "touch_diagnostics_test"
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(sdk),
                           str(ROOT / "tests/touch_diagnostics_test.cpp"), str(cjson_object), "-o", str(touch)])
    run([str(touch)])
    ota = output / "ota_test"
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(sdk), str(ROOT / "firmware/main/core.cpp"), str(ROOT / "tests/ota_test.cpp"), str(cjson_object), "-o", str(ota)])
    run([str(ota)])
    routes = output / "ota_routes_test"
    run(compiler("c++") + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "tests/ota_sdk"), "-I", str(sdk),
                           str(ROOT / "firmware/main/core.cpp"), str(ROOT / "firmware/main/ota.cpp"), str(ROOT / "firmware/main/portal.cpp"), str(ROOT / "firmware/main/brightness_policy.cpp"),
                           str(ROOT / "tests/ota_routes_test.cpp"), str(cjson_object), "-o", str(routes)])
    run([str(routes)])
    run([str(routes), "--idle-capacity"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
