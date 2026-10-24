# Development

Run commands from the repository root. Building does not require RTK or Codex.

## Build firmware

Use Python 3.9+, Git, and the toolchain managed by PlatformIO:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-build.txt
.venv/bin/python scripts/pio.py run -d firmware -e cyd
.venv/bin/python scripts/package_firmware.py
```

`scripts/pio.py` uses the `.platformio/` cache and applies the system trust store only to the build process. Dependency versions are specified in `firmware/platformio.ini`, `firmware/main/idf_component.yml`, and `firmware/dependencies.lock`.

Packages are written to ignored `artifacts/`. Binaries can be distributed separately through GitHub Releases. See the [firmware guide](../firmware/README.md) for offsets, USB migration, and OTA.

## Tests

Python sender/tool tests and Node portal/endpoint tests:

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
node --test tests/stocks_endpoint.test.cjs tests/portal.test.cjs
```

Portable C++ / SDK-stub tests require a C/C++ compiler and the SDK cJSON source downloaded by the build above:

```sh
.venv/bin/python scripts/test_firmware.py
```

Native UI tests use the actual `ui.cpp` and LVGL with simulated runtime/GPIO. They produce PPM screenshots without a connected device:

```sh
cmake -S tests/native_ui -B .platformio/native-ui
cmake --build .platformio/native-ui --parallel
ctest --test-dir .platformio/native-ui --output-on-failure
```

## Assets

Generated C fonts and RGB565 arrays are source files. Commit them together with their PNG/TTF inputs. See the [asset pipeline](../firmware/assets/README.md) to regenerate artwork. The font exporter uses Pillow, fontTools, and `hb-shape` (HarfBuzz) during authoring only; they are not part of the ESP32 runtime.

## Working files

Keep experiments, comparison images, and local reports in `.worktrees/`, rather than a temporary directory outside the repository. Do not commit `.worktrees/`, `.platformio/`, `.venv/`, `.device-backups/`, `artifacts/`, `node_modules/`, or `*.local.json`. The `graft/` directory is a separate checkout, not part of Desk Duo.

## GitHub Actions and releases

The [Firmware workflow](../.github/workflows/firmware.yml) uses Ubuntu 24.04 and Python 3.12. Relevant changes on `main` and pull requests trigger builds; **Actions → Firmware → Run workflow** starts a manual build. It installs pinned build requirements, builds `cyd`, runs portable C++ and Python tests, and validates the firmware package. Successful builds retain downloadable artifacts for 30 days.

A pushed `v*` tag publishes a GitHub Release only after the build and package checks pass. The tag must exactly match `v` plus `PROJECT_VER` from `firmware/CMakeLists.txt` and the compiled application version. For each release, update the version, commit the source, and push its version tag. Existing releases are not overwritten automatically.

Release downloads include:

- `desk-duo-app.bin`: application-only Wi-Fi OTA image.
- `desk-duo-cyd.bin`: merged initial-install image; do not use it to update a configured board.
- `bootloader.bin`, `partitions.bin`, and `ota_data_initial.bin`: segments for the documented USB migration procedure.
- `desk-duo-cyd.json`, `desk-duo-cyd.sha256`, and `SHA256SUMS`: offsets, provenance, and checksums.
- `notices.zip`: third-party licenses.

The build job has read-only repository access. Only the tag-release job can write releases, using GitHub's built-in token; no additional repository secret is needed. CI builds and packages firmware; it does not flash a device.
