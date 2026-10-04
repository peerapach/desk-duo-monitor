#!/usr/bin/env python3
"""Run PlatformIO with system trust scoped to this build command and its children."""

from __future__ import annotations

import os
import pathlib
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]


def main() -> int:
    try:
        import truststore
    except ImportError as error:
        raise SystemExit("Build dependencies are missing; install requirements-build.txt in .venv") from error

    truststore.inject_into_ssl()
    package_root = pathlib.Path(truststore.__file__).resolve().parent.parent
    cli = ROOT / ".venv/bin/platformio"
    if not cli.is_file():
        raise SystemExit("Project PlatformIO is missing; install requirements-build.txt in .venv")

    environment = os.environ.copy()
    inherited_path = environment.get("PYTHONPATH", "")
    scoped_paths = [str(ROOT / "scripts/build_trust"), str(package_root)]
    if inherited_path:
        scoped_paths.append(inherited_path)
    environment["PYTHONPATH"] = os.pathsep.join(scoped_paths)
    environment["DESKDUO_BUILD_TRUSTSTORE"] = "1"
    environment["PLATFORMIO_CORE_DIR"] = str(ROOT / ".platformio")
    return subprocess.run([str(cli), *sys.argv[1:]], cwd=ROOT, env=environment, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
