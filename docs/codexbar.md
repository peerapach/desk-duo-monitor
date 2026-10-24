# CodexBar → Desk Duo

Use Python 3.9+ and its standard library; the CodexBar CLI is not required. The hook example uses `/usr/local/bin/python3`. Replace it with the absolute path of Python on your machine.

CodexBar sends a `usage_updated` JSON event on stdin to the executable configured in Settings → Hooks. It invokes the executable and arguments directly, without a shell. CodexBar documents a 600-second event rate limit per provider/account/window. Behavior may vary by version; check Settings → Hooks and the [External event hooks documentation](https://github.com/steipete/CodexBar/blob/main/docs/configuration.md#external-event-hooks) for your installed version.

Event usage percentages are fractions from `0–1`; the sender converts them to `usedPercent` in `0–100`. `updatedAt` comes from the event's `timestamp`, normalized to UTC, rather than the delivery time. `primary` and `secondary` are included only when the event supplies a usage percentage for that window. Zero is a valid value; missing windows are not invented.

Validate the example without network access:

```sh
python3 bridge/codexbar_sender.py --dry-run < examples/usage-event.json
```

The normalized JSON matches the [usage response fixture](../examples/usage-response.json). Both the event and response contain simulated data.

## Private configuration

Copy `examples/bridge-config.example.json` outside the repository, for example to `~/.config/desk-duo/bridge.local.json`. Set directory permissions to `700` and file permissions to `600`, then replace the sample endpoint and token with your device settings:

```sh
install -d -m 700 ~/.config/desk-duo
install -m 600 examples/bridge-config.example.json ~/.config/desk-duo/bridge.local.json
```

Do not commit `bridge.local.json` or put real tokens in hook arguments, source, or logs. The example is not a configured connection to a deployed device.

In CodexBar Settings → Hooks, merge only the `hooks` object from the [hook configuration fragment](../examples/codexbar-hooks.example.json) into your existing configuration. Replace placeholders with absolute paths to Python, the script, and `bridge.local.json`. Preserve `version`, `providers`, and other CodexBar settings. The example enables separate Claude and Codex rules; it does not modify your local configuration automatically.

CodexBar runs the executable directly. `arguments` is an argument array, and JSON arrives on stdin. The sender uses HTTP `POST /api/usage`, `Content-Type: application/json`, and `Authorization: Bearer …`. It rejects redirects and treats HTTP 2xx as success. Failed deliveries are not retried automatically. See the [protocol](protocol.md) for field definitions.

After installing the firmware, test a POST using the normalized fixture. Replace the host and token with your device's settings:

```sh
curl --fail-with-body --request POST \
  --header 'Content-Type: application/json' \
  --header 'Authorization: Bearer REPLACE_WITH_DEVICE_TOKEN' \
  --data-binary @examples/usage-response.json \
  http://192.168.4.1/api/usage
```

The sender and fixtures do not fetch weather or stocks on the Mac. The ESP32 uses its own configured sources, such as Open-Meteo. The target is ESP32-2432S028R / ESP32-D0WD-V3 rev 3.1, with 4 MiB flash, no PSRAM, LCD on SPI2, and touch on SPI3. Verify portrait orientation, colors, and touch corner mapping on physical hardware.
