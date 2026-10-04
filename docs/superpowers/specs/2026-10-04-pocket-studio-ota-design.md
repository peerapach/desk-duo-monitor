# Pocket Studio redesign and Wi-Fi firmware updates

Status: approved for implementation on 2026-10-04. User selected Pocket Studio, requested local Wi-Fi OTA, reviewed the linked design, and approved the corrected canonical Claude concept with “แบบนี้ละถูกแล้ว”. No new firmware has been flashed at design approval.

## User requirements

- Portrait 240×320 CYD ESP32-2432S028R, pure black background, approachable and polished.
- Keep Claude above Codex, with clear usage and reset times. Replace crude geometric mascots with expressive original artwork.
- Real bottom navigation for Duo, Weather, Stocks, Setup; visible LAN IP.
- Add firmware updates over Wi-Fi.
- Mac runs CodexBar hooks and sends only Claude/Codex usage directly to ESP32.
- Raspberry Pi can separately supply weather, stocks/Google Sheet data, and appropriately licensed map images. It must not become a dependency for usage or local firmware upload.
- No Outlook calendar or appointment integration; the user explicitly declined it.

## Evidence from existing repository

- Current partition table: NVS at 0x9000 size 0x16000; PHY at 0x1f000 size 0x1000; a single factory application at 0x20000 size 0x3e0000.
- There are no OTA slots or OTA data partition. An application-only upload cannot establish the proposed dual-slot layout.
- Current packaged application is 1,429,360 bytes; physical flash is 4 MiB.
- Existing setup portal uses a random per-device Basic admin credential. Usage has a separate bearer token.
- Existing endpoints provide JSON weather/stocks. An image URL currently is not sufficient to display a map; fetch, decode, bounded memory, cache, and fallback behavior would need additional implementation.

## Revised art direction

Use the selected Pocket Studio hierarchy with copper Claude and ivory/mint Codex. The user rejected the rounded terracotta creature because it did not resemble Claude. Preserve the recognizable blocky Claude silhouette: a single stepped rectangular orange body, two square black eyes, short side protrusions, and four rectangular legs. Add restrained highlights and shading without changing its identity. Keep the ceramic terminal-faced Codex robot. See the corrected concept `artifacts/design-reference/pocket-studio-claude-corrected.png`, which supersedes `pocket-studio-sculpted-concept.png`.

The silhouette reference is the mascot inside the TamaClaude repository's `docs/images/tamaclaude-logo.png`, saved locally as `artifacts/design-reference/tamaclaude-mascot-reference.png`. Reference source: https://github.com/thaitop/tamaclaude.

This image is an art-direction reference generated with the built-in imagegen tool, not a functional screenshot. Its radar is illustrative art, not actual Windy imagery or live data. Final typography, spacing, map data, and animation must be verified at actual 240×320 size.

Characters should be pre-rendered bounded sprite frames stored in flash, rather than rendering 3D on the device. Propose up to eight 64×64 RGB565 frames per character (131,072 bytes total before any extra metadata), with subtle breathing, blinking, and a short tap response. These are decorative states and must not imply that a Claude/Codex session is working or idle. Measure final firmware size before fixing the asset budget.

Keep usage values and reset dates as native text. Page transitions are short; no constant full-screen effects. Stocks and weather prioritize readable data. The visible IP and a genuine Setup navigation control address the existing affordance problem.

## OTA approach

Recommend a manual local web upload first: open Setup in a browser, authenticate as the existing admin, select a Desk Duo application `.bin`, review its version, and upload. Show upload progress, verification, restart, and reconnect states. Do not require a cloud account or Pi update server. A Pi-hosted release feed can be a separate later enhancement.

Use ESP-IDF native application OTA, writing the inactive slot and selecting it for boot only after validation. Enable bootloader rollback, then explicitly accept a new image only after local initialization checks pass. Internet access and successful connection to a particular Wi-Fi network must not be boot health requirements; the protected setup AP must remain recoverable.

Proposed 4 MiB partition geometry, to validate against actual build output:

| Partition | Offset | Size |
| --- | --- | --- |
| NVS | 0x9000 | 0x16000 |
| PHY | 0x1f000 | 0x1000 |
| OTA data | 0x20000 | 0x2000 |
| OTA slot 0 | 0x30000 | 0x1e0000 |
| OTA slot 1 | 0x210000 | 0x1e0000 |

Each slot is 1,966,080 bytes. Both slots end below the physical flash boundary and retain the current NVS and PHY ranges. Assets and application must fit one slot; the packaging script must verify all offsets, bounds, and image sizes rather than assuming a factory partition.

The one-time migration updates the bootloader, partition table, OTA data, and initial application through USB, after a fresh device backup. Do not erase or overwrite NVS. Current usage token, admin credential, Wi-Fi, endpoint settings, and touch calibration must survive and be checked after reboot. Subsequent Wi-Fi uploads accept application images only; reject merged full-flash packages.

## Upload and recovery constraints

- Require the existing admin credential. A usage bearer token must not authorize firmware replacement.
- Reject cross-origin browser uploads; use an explicit same-origin upload header without permissive CORS.
- Bound file size by the inactive slot, stream rather than buffering the whole firmware, permit only one upload, and abort incomplete or timed-out transfers.
- Validate image header, chip compatibility, application project identity, and ESP-IDF image integrity before selecting the boot image. Do not describe checksums or project-name checks as cryptographic publisher authentication.
- On interrupted upload, retain the current boot selection. On an unconfirmed new image that crashes or resets, boot the previous confirmed OTA image.
- UI must remain responsive during upload. Concurrency design must prevent settings writes/reboots from racing the OTA commit; interactions during maintenance need explicit behavior.
- No fuse changes, credential replacement, cloud exposure, or remote auto-update in this scope.

## Verification required before delivery

- Inspect every native UI page at real resolution, longest reset strings, missing/stale data, and reduced-motion behavior in the preview.
- Build the actual target and verify slot capacity and packaging metadata; existing core/bridge behavior must remain intact.
- Exercise upload authentication, oversized/invalid/wrong-project image rejection, interrupted upload, and rollback state handling.
- On hardware, preserve settings through USB migration, complete one Wi-Fi update, and verify reconnect and usage hooks.
- Verify rollback using a controlled failing test image with USB recovery available. Do not claim hardware rollback verified from compilation or a mock test.
- Follow the existing work-with-luna review workflow, including a final independent Luna max review for production changes.

Reference: ESP-IDF v5.5.3 OTA documentation, https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-reference/system/ota.html.

## Scope sequence

1. Agree on revised Pocket Studio art and local OTA behavior.
2. Implement UI/assets and dual-slot OTA, review, build, then migrate and test the connected board.
3. Design and implement Pi data/image endpoints separately once the map provider, its permitted image use, and the Pi environment are settled. Preserve Mac-only usage hooks throughout.
