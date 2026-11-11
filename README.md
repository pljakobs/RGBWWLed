# RGBWWLibrary
Library for controlling LEDs via PWM. Supports different color spaces (RGBW, HSV) and color correction.

On ESP32, hardware fade segments are limited by `PWM_MAX_FADE_CYCLES` (default: `1023`). Define this macro at build time to override the limit when porting the hardware-fade path to a PWM backend with different constraints. Segment durations are calculated from each channel's PWM frequency and effective duty resolution; time beyond the safe hardware fade duration is preserved as a hold.
