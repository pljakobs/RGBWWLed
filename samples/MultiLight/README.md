# RGBWWLed MultiLight Bank Sample

ESP32 classic-only Sming sample for exercising four independent `RGBWWLed` profiles across both LEDC speed-mode banks. One RGBWW light is split across the banks:

- Low-speed channels 0–2: RGB outputs of the split `RGBWWCW` light
- Low-speed channels 3–5: `RGB` light (3 outputs)
- High-speed channels 0–1: WW/CW outputs of the split `RGBWWCW` light
- High-speed channels 2–3: `WWCW` light (2 outputs)
- High-speed channel 4: `SINGLE` output

Each bank uses timer 0 at 1 kHz. The sample alternates HSV and raw fades every five seconds and calls `show()` for each instance at the library update rate. It requires a classic ESP32 target with `SOC_LEDC_SUPPORT_HS_MODE`; SoCs without a high-speed bank are intentionally rejected at compile time.

Build with:

```sh
make SMING_ARCH=Esp32 SMING_SOC=esp32
```

The pin choices are illustrative; check the target board's pin availability and wiring before flashing. This sample validates build and integration wiring; verify outputs on hardware.
