#pragma once

#ifdef ARCH_ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <vector>

class RGBWWLed;

#if FADE_TIMING_DIAGNOSTICS
enum class RGBWWServiceTrace : uint8_t {
    Begin = 20, End, FrameDue, Rendered, Queued, DispatchBegin, DispatchEnd, QueueRejected
};
void rgbwwLedTimingTrace(RGBWWServiceTrace event, const RGBWWLed* light, uint32_t value, uint32_t detail);
#endif

class RGBWWLedService {
public:
    static void lock();
    static void unlock();
    static bool attach(RGBWWLed& light);
    static void detach(RGBWWLed& light);
    static void wake();

private:
    static void task(void*);
    static void dispatch(uint32_t);
    static int64_t service();
    static SemaphoreHandle_t mutex;
    static TaskHandle_t worker;
    static std::vector<RGBWWLed*> lights;
    static uint32_t nextId;
    static bool dispatchQueued;
};
#endif
