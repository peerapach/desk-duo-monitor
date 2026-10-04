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

## Review round 1/5 fixes

The review's three findings are covered by these regressions:

- `NormalizeEventTests.test_normalizes_integer_zero_and_one_usage_fractions` exercises integer `0` and `1` with Python 3.9.6 as well as Python 3.12.8.
- `NormalizeEventTests.test_timestamp_underflow_is_a_sanitized_cli_error` runs the actual CLI with `0001-01-01T00:00:00+07:00` and checks exit 2, coarse stderr, and empty stdout.
- `DeliveryTests.test_timeout_is_an_overall_deadline_and_stops_slow_header_transport` uses a real loopback responder that emits one response-header byte every 15 ms. It checks delivery fails under a 0.6-second deadline, the response does not finish, the peer and handler close, and no transport worker remains.
- `DeliveryTests.test_sigterm_cancels_cli_and_closes_worker_transport` starts the real CLI, sends SIGTERM after the request arrives, and checks exit 143 plus peer and handler closure before the slow response finishes.

### RED evidence

Integer and date regressions were run against the installed Python 3.9.6:

```text
rtk proxy /usr/bin/python3 -m unittest tests.test_codexbar_sender.NormalizeEventTests.test_normalizes_integer_zero_and_one_usage_fractions tests.test_codexbar_sender.NormalizeEventTests.test_timestamp_underflow_is_a_sanitized_cli_error -v
```

Observed: **FAILED (failures=3)**. Both integer subtests reported `AttributeError: 'int' object has no attribute 'is_integer'`. The CLI date case returned 1 instead of 2 and printed `OverflowError: date value out of range`.

The initial total-deadline regression was run against a slow loopback HTTP fixture:

```text
rtk proxy /usr/local/bin/python3 -m unittest tests.test_codexbar_sender.DeliveryTests.test_timeout_is_an_overall_deadline_and_stops_slow_header_transport -v
```

Observed: **FAILED** with `a slow but progressing response must time out`; the sender returned success only after the slow response completed (the test took 3.016 seconds with a 0.6-second configured timeout).

The cancellation regression was run before installing CLI cleanup handling:

```text
rtk proxy /usr/local/bin/python3 -m unittest tests.test_codexbar_sender.DeliveryTests.test_sigterm_cancels_cli_and_closes_worker_transport -v
```

Observed: **FAILED**, `return_code` was `-15` instead of `143`; default SIGTERM killed the CLI before it could reap its spawned transport worker.

### GREEN evidence

After the fixes, the two transport lifecycle regressions passed together:

```text
rtk proxy /usr/local/bin/python3 -m unittest tests.test_codexbar_sender.DeliveryTests.test_timeout_is_an_overall_deadline_and_stops_slow_header_transport tests.test_codexbar_sender.DeliveryTests.test_sigterm_cancels_cli_and_closes_worker_transport -v
```

```text
Ran 2 tests in 1.891s

OK
```

The older interpreter regression checks also passed:

```text
rtk proxy /usr/bin/python3 -m unittest tests.test_codexbar_sender.NormalizeEventTests.test_normalizes_integer_zero_and_one_usage_fractions tests.test_codexbar_sender.NormalizeEventTests.test_timestamp_underflow_is_a_sanitized_cli_error -v
```

```text
Ran 2 tests in 0.108s

OK
```

### Final full-suite output

Python 3.9.6:

```text
rtk proxy /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v

Ran 35 tests in 5.983s

OK
```

Python 3.12.8:

```text
rtk proxy /usr/local/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v

Ran 35 tests in 6.281s

OK
```

Both full runs included normalization, config, CLI, actual loopback POST, slow-header deadline, redirect, timeout, and SIGTERM cleanup cases. All five example JSON files also parsed under Python 3.9, and the Python 3.9 dry-run output matched `examples/usage-response.json` exactly.

Verbose test names and final summaries:

```text
Python 3.9.6
test_accepts_input_at_four_kibibytes (test_codexbar_sender.CliTests) ... ok
test_dry_run_needs_no_config_and_prints_normalized_json_without_network (test_codexbar_sender.CliTests) ... ok
test_rejects_input_over_four_kibibytes (test_codexbar_sender.CliTests) ... ok
test_rejects_invalid_json_and_non_object_json (test_codexbar_sender.CliTests) ... ok
test_returns_two_when_delivery_config_is_missing (test_codexbar_sender.CliTests) ... ok
test_accepts_https_receiver_endpoint (test_codexbar_sender.ConfigTests) ... ok
test_loads_valid_config_and_defaults_timeout_to_five_seconds (test_codexbar_sender.ConfigTests) ... ok
test_rejects_empty_and_control_character_tokens (test_codexbar_sender.ConfigTests) ... ok
test_rejects_invalid_endpoints (test_codexbar_sender.ConfigTests) ... ok
test_rejects_missing_or_malformed_config (test_codexbar_sender.ConfigTests) ... ok
test_rejects_timeouts_outside_finite_point_one_to_thirty_seconds (test_codexbar_sender.ConfigTests) ... ok
test_accepts_http_204_as_success (test_codexbar_sender.DeliveryTests) ... ok
test_cli_returns_one_and_redacts_delivery_failure (test_codexbar_sender.DeliveryTests) ... ok
test_posts_json_to_receiver_with_expected_path_and_bearer_header (test_codexbar_sender.DeliveryTests) ... ok
test_receiver_rejection_returns_sanitized_delivery_error (test_codexbar_sender.DeliveryTests) ... ok
test_redirect_is_rejected_before_destination_receives_bearer_request (test_codexbar_sender.DeliveryTests) ... ok
test_send_usage_validates_direct_config_before_network (test_codexbar_sender.DeliveryTests) ... ok
test_sigterm_cancels_cli_and_closes_worker_transport (test_codexbar_sender.DeliveryTests) ... ok
test_timeout_is_an_overall_deadline_and_stops_slow_header_transport (test_codexbar_sender.DeliveryTests) ... ok
test_timeout_returns_sanitized_delivery_error (test_codexbar_sender.DeliveryTests) ... ok
test_accepts_both_supported_providers (test_codexbar_sender.NormalizeEventTests) ... ok
test_normalizes_fractional_usage_and_timestamp_to_utc_without_identity (test_codexbar_sender.NormalizeEventTests) ... ok
test_normalizes_integer_zero_and_one_usage_fractions (test_codexbar_sender.NormalizeEventTests) ... ok
test_omits_secondary_window_when_secondary_fraction_is_absent (test_codexbar_sender.NormalizeEventTests) ... ok
test_omits_window_fields_when_usage_fraction_is_absent (test_codexbar_sender.NormalizeEventTests) ... ok
test_preserves_event_with_no_usage_windows_as_unknown_data (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_boolean_and_nonfinite_usage_fractions (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_fraction_outside_zero_to_one (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_malformed_source_and_reset_dates (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_non_object_event (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_nonpositive_or_nonfinite_window_minutes (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_reset_and_source_timestamps_without_explicit_timezone (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_unknown_provider (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_wrong_event_name (test_codexbar_sender.NormalizeEventTests) ... ok
test_timestamp_underflow_is_a_sanitized_cli_error (test_codexbar_sender.NormalizeEventTests) ... ok

Ran 35 tests in 5.983s

OK

Python 3.12.8
test_accepts_input_at_four_kibibytes (test_codexbar_sender.CliTests.test_accepts_input_at_four_kibibytes) ... ok
test_dry_run_needs_no_config_and_prints_normalized_json_without_network (test_codexbar_sender.CliTests.test_dry_run_needs_no_config_and_prints_normalized_json_without_network) ... ok
test_rejects_input_over_four_kibibytes (test_codexbar_sender.CliTests.test_rejects_input_over_four_kibibytes) ... ok
test_rejects_invalid_json_and_non_object_json (test_codexbar_sender.CliTests.test_rejects_invalid_json_and_non_object_json) ... ok
test_returns_two_when_delivery_config_is_missing (test_codexbar_sender.CliTests) ... ok
test_accepts_https_receiver_endpoint (test_codexbar_sender.ConfigTests) ... ok
test_loads_valid_config_and_defaults_timeout_to_five_seconds (test_codexbar_sender.ConfigTests) ... ok
test_rejects_empty_and_control_character_tokens (test_codexbar_sender.ConfigTests) ... ok
test_rejects_invalid_endpoints (test_codexbar_sender.ConfigTests) ... ok
test_rejects_missing_or_malformed_config (test_codexbar_sender.ConfigTests) ... ok
test_rejects_timeouts_outside_finite_point_one_to_thirty_seconds (test_codexbar_sender.ConfigTests) ... ok
test_accepts_http_204_as_success (test_codexbar_sender.DeliveryTests) ... ok
test_cli_returns_one_and_redacts_delivery_failure (test_codexbar_sender.DeliveryTests) ... ok
test_posts_json_to_receiver_with_expected_path_and_bearer_header (test_codexbar_sender.DeliveryTests) ... ok
test_receiver_rejection_returns_sanitized_delivery_error (test_codexbar_sender.DeliveryTests) ... ok
test_redirect_is_rejected_before_destination_receives_bearer_request (test_codexbar_sender.DeliveryTests) ... ok
test_send_usage_validates_direct_config_before_network (test_codexbar_sender.DeliveryTests) ... ok
test_sigterm_cancels_cli_and_closes_worker_transport (test_codexbar_sender.DeliveryTests) ... ok
test_timeout_is_an_overall_deadline_and_stops_slow_header_transport (test_codexbar_sender.DeliveryTests) ... ok
test_timeout_returns_sanitized_delivery_error (test_codexbar_sender.DeliveryTests) ... ok
test_accepts_both_supported_providers (test_codexbar_sender.NormalizeEventTests) ... ok
test_normalizes_fractional_usage_and_timestamp_to_utc_without_identity (test_codexbar_sender.NormalizeEventTests) ... ok
test_normalizes_integer_zero_and_one_usage_fractions (test_codexbar_sender.NormalizeEventTests) ... ok
test_omits_secondary_window_when_secondary_fraction_is_absent (test_codexbar_sender.NormalizeEventTests) ... ok
test_omits_window_fields_when_usage_fraction_is_absent (test_codexbar_sender.NormalizeEventTests) ... ok
test_preserves_event_with_no_usage_windows_as_unknown_data (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_boolean_and_nonfinite_usage_fractions (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_fraction_outside_zero_to_one (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_malformed_source_and_reset_dates (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_non_object_event (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_nonpositive_or_nonfinite_window_minutes (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_reset_and_source_timestamps_without_explicit_timezone (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_unknown_provider (test_codexbar_sender.NormalizeEventTests) ... ok
test_rejects_wrong_event_name (test_codexbar_sender.NormalizeEventTests) ... ok
test_timestamp_underflow_is_a_sanitized_cli_error (test_codexbar_sender.NormalizeEventTests) ... ok

Ran 35 tests in 6.281s

OK
```

### Fix review

The sender now performs blocking HTTP work in a `spawn` worker process and starts a monotonic deadline before worker launch. The parent supplies request data over a private multiprocessing pipe after launch; the bearer token is absent from worker process arguments and is never logged. The parent accepts a result only by the deadline, then terminates and reaps the worker on timeout. The CLI entrypoint temporarily handles SIGTERM/SIGINT so cancellation unwinds through that cleanup; it restores the prior handlers before exit. Library calls do not install process-global signal handlers. README now declares Python 3.9+ and the hook example points to the available `/usr/local/bin/python3` (3.12.8); Python 3.9.6 was also exercised.

Only the sender, its tests, README, hook example, and this report are in the Task 2 fix set. The worktree's unrelated `.gitignore` and design-spec edits were left unstaged. The sender still has no live receiver firmware or physical-device verification; loopback fixtures are the only transport targets used.

## Review round 2/5: deadline-safe request transfer

`send_usage` no longer calls blocking `Connection.send()` for the request. It encodes the request as UTF-8 JSON, frames it in the format consumed by `Connection.recv_bytes()`, then writes through a nonblocking descriptor. Each partial write waits with `select` only for the remaining parent monotonic deadline. The parent uses that same clock for both transfer and result acceptance; the worker uses only the configured socket timeout, so it does not compare monotonic readings across processes. If transfer or response exceeds the deadline, the parent closes the pipe and terminates/reaps the worker. The CLI-only signal cleanup from round 1 remains in place.

No new configuration size cap was added: the regression keeps a valid 1 MiB bearer token, and the transfer mechanism itself must stop waiting when the held reader does not drain the pipe. The token is sent through the private pipe after worker launch, never through process arguments or logs. The local-only regression does not connect to its placeholder loopback endpoint.

### RED evidence

The held-reader regression was first run before the nonblocking transfer implementation, with a valid 1 MiB token, a 0.1-second timeout, and a real spawned worker that slept for two seconds without reading its pipe:

```text
rtk proxy /usr/local/bin/python3 -m unittest tests.test_codexbar_sender.DeliveryTests.test_ipc_transfer_obeys_deadline_with_held_reader_and_one_mib_token -v
```

Observed: **FAILED**. The measured sender duration was `2.262213416979648` seconds and failed `request IPC blocked past its monotonic deadline` (`not less than 0.8`). The worker exited and was reaped after closing the pipe.

### GREEN evidence

The final regression uses a 0.5-second budget so the spawned held-reader is confirmed running before the deadline, asserts the transfer phase was entered, and still keeps the same two-second hold and valid 1 MiB token:

```text
rtk proxy /usr/local/bin/python3 -m unittest tests.test_codexbar_sender.DeliveryTests.test_ipc_transfer_obeys_deadline_with_held_reader_and_one_mib_token -v
```

```text
Ran 1 test in 0.568s

OK
```

Real loopback delivery, 204 success, and redirect rejection were also checked after changing the wire framing:

```text
rtk proxy /usr/local/bin/python3 -m unittest tests.test_codexbar_sender.DeliveryTests.test_posts_json_to_receiver_with_expected_path_and_bearer_header tests.test_codexbar_sender.DeliveryTests.test_accepts_http_204_as_success tests.test_codexbar_sender.DeliveryTests.test_redirect_is_rejected_before_destination_receives_bearer_request -v

Ran 3 tests in 2.351s

OK
```

### Final suite output

```text
rtk proxy /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v
Python 3.9.6: Ran 36 tests in 6.752s
OK

rtk proxy /usr/local/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v
Python 3.12.8: Ran 36 tests in 6.543s
OK

rtk proxy /Users/peerapach.v/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node --test tests/stocks_endpoint.test.cjs
ℹ tests 35
ℹ pass 35
ℹ fail 0
```

The Python suites cover the held-reader transfer, slow response headers, successful loopback POST, receiver rejection, redirect destination isolation, timeout, and SIGTERM cleanup. The Node suite remains green. Local HTTP fixtures bind only to `127.0.0.1`; the held-reader IPC test performs no socket request. `rtk git diff --check` was clean before report append.

### Round 2 self-review

The deadline wait no longer surrounds a blocking request write: the request transfer itself is incremental and nonblocking, and the parent checks remaining time before each wait/write and after the final byte. Partial frames are abandoned only as the parent closes the pipe and terminates the worker, so the child cannot continue network activity. Response status IPC remains a single small boolean frame. The deadline path and status check use only the parent process's monotonic clock. The library adds no signal handler or other process-global signal change; cancellation is still handled and restored only by `_cli_entrypoint`. The held-reader test confirms prompt return and zero active child processes. No live service, device, or remote endpoint was contacted.
