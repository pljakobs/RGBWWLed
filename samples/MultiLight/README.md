# RGBWWLed MultiLight Bank Sample

ESP32 classic-only Sming sample for exercising four independent `RGBWWLed` profiles across both LEDC speed-mode banks:

- Low-speed channels 0–4: `RGBWWCW` (5 outputs)
- Low-speed channels 5–7: `RGB` (3 outputs)
- High-speed channels 0–1: `WWCW` (2 outputs)
- High-speed channel 2: `SINGLE` (1 output)

Each bank uses timer 0 at 1 kHz. The sample alternates HSV and raw fades every five seconds and calls `show()` for each instance at the library update rate. It requires a classic ESP32 target with `SOC_LEDC_SUPPORT_HS_MODE`; SoCs without a high-speed bank are intentionally rejected at compile time.

Build with:

```sh
make SMING_ARCH=Esp32 SMING_SOC=esp32
```

The pin choices are illustrative; check the target board's pin availability and wiring before flashing. This sample validates build and integration wiring; verify outputs on hardware.
