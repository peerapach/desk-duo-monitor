# Task 2 report: CodexBar sender and project integration

## Files

- `bridge/codexbar_sender.py` — standard-library-only event normalization, config validation, bounded stdin handling, dry-run CLI, and one-shot POST transport.
- `tests/test_codexbar_sender.py` — normalization, validation, CLI, and actual loopback HTTP receiver tests.
- `examples/usage-event.json`, `examples/usage-response.json`, `examples/bridge-config.example.json`, and `examples/codexbar-hooks.example.json` — simulated inputs, output, placeholder local config, and a `hooks` merge fragment.
- `docs/protocol.md` — sender and receiver contract.
- `README.md` — Thai setup guide, Sheets links, private config guidance, dry-run and future manual POST commands, app hook cadence, and source-only status.

## RED and GREEN evidence

The first requested RED command was:

```text
rtk proxy python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Initially discovery stopped with `ModuleNotFoundError: No module named 'bridge'`. The test harness was adjusted to report the missing sender contract as test failures while preserving normal behavior assertions once the module exists. The next RED run reported `Ran 27 tests` and `FAILED (failures=27)`; every case failed its setup assertion that `bridge.codexbar_sender` was not implemented.

After implementation, focused verification used:

```text
rtk proxy python3 -m unittest tests.test_codexbar_sender -v
```

It reported `Ran 31 tests` and `OK`. The final full Python run below also passed after the last config-validation adjustment.

## Final verification

- `rtk proxy python3 -m unittest discover -s tests -p 'test_*.py' -v` — **31 passed** (`OK`). This includes real loopback POST checks for path, JSON body, content and bearer headers, 204 success, 500 failure, timeout, and a 302 redirect whose destination received zero requests. Dry-run made no network attempt.
- `rtk proxy node --test tests/stocks_endpoint.test.cjs` — the system PATH had no `node`. The bundled runtime was used without installation: `rtk proxy /Users/peerapach.v/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node --test tests/stocks_endpoint.test.cjs` — **35 passed, 0 failed**.
- Parsed all five example JSON files and ran the sender's `--dry-run` against `examples/usage-event.json`; parsed output matched `examples/usage-response.json` exactly.
- `rtk git diff --cached --check` — clean.

The default sandbox denied local socket binding for the mandatory HTTP fixtures. The test command was rerun with the authorized escalation and bound only to `127.0.0.1`; no remote endpoint was contacted.

## Self-review and concerns

The sender forwards only allowlisted provider usage data, preserves zero, omits absent windows, validates dates and finite numeric values, refuses redirects, bounds request time, performs no retries, and prints coarse sanitized failures without tokens or endpoint URLs. The hook example contains only a `hooks` JSON merge fragment; setup text says to retain the existing CodexBar `version` and `providers` configuration. No app hook, device config, Apps Script deployment, serial port, or physical receiver was changed or contacted.

The receiver firmware and physical ESP32 behavior remain unverified and are subsequent work. The shared worktree also has an unstaged `.gitignore` change for `.device-backups/` and `.platformio/` that was outside Task 2 and excluded from the Task 2 commit.
