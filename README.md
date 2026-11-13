# RGBWWLibrary
Library for controlling LEDs via PWM. Supports different color spaces (RGBW, HSV) and color correction.

## Release branches

The standard development-branch name is `develop`, qualified by release line:

| Branch | Purpose | Firmware consumer |
|---|---|---|
| `0.10/develop` | 0.10.z maintenance, based on `v0.10.1` | Lightinator `5.0/develop`, pinned to `v0.10.1` |
| `0.11/develop` | 0.11.z features, including multi-light/channel accessors and central PWM ownership work | Lightinator `5.1/develop`, currently pinned to `5f86688` |

Keep the release lines separate: do not merge 0.11 into 0.10. Backport only fixes compatible with the 0.10 API and behavior, and release them as 0.10.z. New features belong on 0.11. Firmware submodule pins must be updated explicitly within their intended release line.

The unqualified `devel` and `develop` branches are legacy names, not the targets for new release-line work.

## Controller-owned ESP32 PWM

Use `bool RGBWWLed::init(LightType, const RGBWWPwmChannels&, RGBWW_CHANNELS)` to bind a light to externally owned PWM banks. The five bindings are in RED/GREEN/BLUE/WW/CW order; each contains a bank pointer and its bank-local channel index. Null bindings represent unused channels. Initialization rejects mismatched profiles, invalid banks/indices and duplicate assignments.

The controller must allocate distinct channels to each light and keep the banks alive until every light is destroyed. A light only updates, fades or queries its assigned channels. Destroying it cancels those channels and sets their duty to zero without destroying the banks or touching another light. Frequency, spread spectrum and phase placement belong to the controller, not the light. Existing pin/config constructors remain supported for standalone users and own their compatibility banks.

Run `make -C tests/native test` with `SMING_HOME` pointing to Sming to test borrowed mappings, split banks, duty scaling, channel isolation, reassignment and legacy ownership against the HardwarePwm native mocks. Both fade-stop-capable and classic variants are covered.

On ESP32, hardware fade segments are limited by `PWM_MAX_FADE_CYCLES` (default: `1023`). Define this macro at build time to override the limit when porting the hardware-fade path to a PWM backend with different constraints. Segment durations are calculated from each channel's PWM frequency and effective duty resolution; time beyond the safe hardware fade duration is preserved as a hold.
