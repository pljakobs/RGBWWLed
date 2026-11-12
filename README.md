# RGBWWLibrary
Library for controlling LEDs via PWM. Supports different color spaces (RGBW, HSV) and color correction.

## Development branch

The current 0.11 development line is `devel`. The standard branch name going forward is `develop`; this repository retains `devel` until an explicit branch-name migration. The existing `develop` branch is legacy and is not the current development target.

On ESP32, hardware fade segments are limited by `PWM_MAX_FADE_CYCLES` (default: `1023`). Define this macro at build time to override the limit when porting the hardware-fade path to a PWM backend with different constraints. Segment durations are calculated from each channel's PWM frequency and effective duty resolution; time beyond the safe hardware fade duration is preserved as a hold.
