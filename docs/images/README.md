# README preview images

These images come from the actual native LVGL renderer for Desk Duo firmware 2.2.3, using deterministic test fixtures. They are not AI-generated mockups, physical device photographs, or live data screenshots.

| File | Native renderer output | Size |
| --- | --- | --- |
| `duo.png` | `device-duo-en.ppm` | 240×320 |
| `weather.png` | `device-weather-en.ppm` | 240×320 |
| `stocks.png` | `device-stocks-en.ppm` | 240×320 |
| `desk-duo-preview.png` | The three captures above, without scaling | 784×352 |

The UI source is `firmware/main/ui.cpp`; fixture setup and capture are in `tests/native_ui/preview.cpp`, particularly `fidelityCaptures()`. The fixtures use sample usage, weather, stock prices, and the documentation-only IP address 192.0.2.10. No private configuration or credentials are shown.

To reproduce the source frames, build and run the native UI tests following [the development guide](../development.md). Convert the 240×320 RGB PPM frames to PNG without resizing. The overview places the three PNGs on a dark background with 16px outer padding and 16px gaps. The individual screenshots retain their original pixel values.
