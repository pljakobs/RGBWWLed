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

## Shared ESP32 animation worker

After initialization, call `light.startService()` to opt into automatic servicing.
All participating lights share one library-owned FreeRTOS task, without another
50 Hz ESP timer or per-light tasks. The task is unpinned and works on single-core
ESP32 variants. It parks when the last light detaches; its 6 KiB stack is retained
for reuse. `stopService()`, reinitialization and destruction detach a light under
the same recursive mutex used for commands and rendering.
Registration, stop, reinitialization and destruction belong to the Sming
thread, like the notification callbacks. Commands and value-snapshot reads
are synchronized against the animation task.
An experimental `RGBWW_SERVICE_CORE` compile definition pins the worker to an
enabled CPU; the default remains unpinned. Pinning does not make application
callbacks thread-safe: they must still execute on the Sming dispatcher.

```cpp
if(!light.startService(onUpdate, context)) {
    // Report failure to the application; automatic servicing did not start.
}
```

The optional `void onUpdate(void*)` callback runs on the Sming event loop, not
the animation task. Register it on one light when collecting controller-wide
state. Notifications share a single pending dispatcher callback: delayed
dispatch coalesces updates without gating animation advancement. Animation completion
hooks also run on the event loop, no earlier than the completed frame boundary.
Named completions coalesce by name while dispatch is delayed. Detaching removes
pending notifications, and dispatch validates registrations against destruction
or reinitialization.

The nominal interval is 20 ms. `setServiceIntervalUs()` permits clock steering
(1,000 through 1,000,000 microseconds). Frame positions use monotonic time: a late
wake advances directly over missed logical steps, including queue boundaries,
without replaying physical frames. New commands and resumed animations do not
inherit time missed before their acceptance. Pause freezes that channel's
logical progression. Catch-up is bounded to one queue capacity plus one
animation boundary per pass; extreme overload involving short requeued entries
drops remaining stale steps with a warning rather than monopolizing the task.

For transition frames, RGBWWLed computes HSV conversion, white balance,
brightness correction and the dim curve before submitting short physical PWM
microfades. A segment uses the time left in its frame, less one PWM-cycle margin.
Expired frames and ordinary set/blink frames use immediate duties. Classic
ESP32 cannot abort an active hardware fade, so only the newest replacement is
retained behind the current short segment; a frame backlog cannot grow. A stop
does not interrupt the already submitted microfade.

`show()` remains available. While automatic servicing is enabled it only
consumes the completion flag; it never advances a second time.
`takeServiceSteps()` consumes the logical frame count, including skipped frames,
for application clock reporting. State getters return locked **value snapshots**
of the latest calculated target, not references into worker-owned state or
instantaneous hardware duty. Do not retain a pointer to a getter result.
Opt-in `FADE_TIMING_DIAGNOSTICS=1` builds must provide `rgbwwLedTimingTrace`
as declared in `RGBWWLedService.h`. Its non-blocking hook reports service
entry/exit, mutex wait, selected-frame lateness/rendering, and callback
enqueue/dispatch; it may run on either thread and must not perform I/O.
Use `RGBWWLed::Guard` around external `colorutils` changes or direct PWM access,
and keep that scope short (never across networking or persistence).

Without `startService()`, manual servicing retains its existing cadence and RAW
hardware-fade path. A manual hardware fade must finish before automatic
servicing can be enabled; otherwise `startService()` logs an error and fails.
ESP8266 and Host do not enable this worker.

The native suite also exercises sixteen lights sharing one animation task,
missed frames, queue boundaries, pause/resume, physical duty correction,
bounded classic replacements, concurrent commands, delayed/rejected dispatch,
teardown and manual-mode compatibility on both LEDC variants.
