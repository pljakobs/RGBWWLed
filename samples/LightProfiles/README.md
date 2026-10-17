# RGBWWLed Light Profiles Sample

Small standalone Sming app for exercising type-specific animation queues and sparse physical-pin mappings without building Lightinator.

`RGBWW_SAMPLE_PROFILE` selects a `LightType` by enum value:

| Value | Profile |
|---:|---|
| 0 | `RGBWWCW` |
| 1 | `RGBWW` |
| 2 | `RGBCW` |
| 3 | `RGB` |
| 4 | `WWCW` |
| 5 | `WW` |
| 6 | `CW` |
| 7 | `SINGLE` |

Build for the default five-channel profile:

```sh
make SMING_ARCH=Esp8266
make SMING_ARCH=Esp32
```

Build another profile by passing its numeric value:

```sh
make SMING_ARCH=Esp32 RGBWW_SAMPLE_PROFILE=4
```

The app alternates HSV and raw fades every five seconds and calls `show()` at the library update frequency. Select GPIOs appropriate for the board before flashing; the example pin choices are only defaults.
