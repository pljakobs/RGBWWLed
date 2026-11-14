#include "RGBWWLed.h"
#include "RGBWWLedService.h"
#include "RGBWWAnimatedChannel.h"

#ifdef ARCH_ESP32
#include <Platform/System.h>
#include <algorithm>
#include <cassert>
#include <esp_timer.h>

#if FADE_TIMING_DIAGNOSTICS
#define SERVICE_TRACE(event, light, value, detail) \
    rgbwwLedTimingTrace(RGBWWServiceTrace::event, light, value, detail)
#else
#define SERVICE_TRACE(...) ((void)0)
#endif

SemaphoreHandle_t RGBWWLedService::mutex = xSemaphoreCreateRecursiveMutex();
TaskHandle_t RGBWWLedService::worker = nullptr;
std::vector<RGBWWLed*> RGBWWLedService::lights;
uint32_t RGBWWLedService::nextId = 0;
bool RGBWWLedService::dispatchQueued = false;

void RGBWWLedService::lock()
{
    assert(mutex != nullptr);
    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
}

void RGBWWLedService::unlock()
{
    xSemaphoreGiveRecursive(mutex);
}

bool RGBWWLedService::attach(RGBWWLed& light)
{
    RGBWWLed::Guard guard;
    if(light._serviceId != 0) return true;
    if(worker == nullptr) {
#ifdef RGBWW_SERVICE_CORE
        static_assert(RGBWW_SERVICE_CORE >= 0 && RGBWW_SERVICE_CORE < portNUM_PROCESSORS,
                      "RGBWW_SERVICE_CORE must select an enabled CPU");
        const auto created = xTaskCreatePinnedToCore(task, "rgbww-led", 6144, nullptr,
                                                    tskIDLE_PRIORITY + 1, &worker, RGBWW_SERVICE_CORE);
#else
        const auto created = xTaskCreate(task, "rgbww-led", 6144, nullptr, tskIDLE_PRIORITY + 1, &worker);
#endif
        if(created != pdPASS) {
            worker = nullptr;
            debug_e("RGBWWLed: unable to create animation worker");
            return false;
        }
    }
    do { ++nextId; } while(nextId == 0);
    light._serviceId = nextId;
    light._nextFrameUs = lights.empty() ? esp_timer_get_time() : lights.front()->_nextFrameUs;
    lights.push_back(&light);
    wake();
    return true;
}

void RGBWWLedService::detach(RGBWWLed& light)
{
    RGBWWLed::Guard guard;
    lights.erase(std::remove(lights.begin(), lights.end(), &light), lights.end());
    light._serviceId = 0;
    light._serviceCallback = nullptr;
    light._serviceArgument = nullptr;
    wake();
}

void RGBWWLedService::wake()
{
    if(worker != nullptr) xTaskNotifyGive(worker);
}

int64_t RGBWWLedService::service()
{
#if FADE_TIMING_DIAGNOSTICS
    const int64_t started = esp_timer_get_time();
    SERVICE_TRACE(Begin, nullptr, 0, 0);
#endif
    RGBWWLed::Guard guard;
#if FADE_TIMING_DIAGNOSTICS
    const uint32_t lockWait = esp_timer_get_time() - started;
#endif
    int64_t next = 0;
    bool notify = false;
    for(auto* light : lights) {
        const int64_t now = esp_timer_get_time();
        if(now >= light->_nextFrameUs) {
            light->_serviceFinished |= light->_frameFinished;
            light->_frameFinished = false;
            for(unsigned i = 0; i < light->_frameNotifications.count(); ++i) {
                light->_notifications[light->_frameNotifications.keyAt(i)] = light->_frameNotifications.valueAt(i);
            }
            light->_frameNotifications.clear();
            const uint32_t steps = 1 + (now - light->_nextFrameUs) / light->_serviceIntervalUs;
            SERVICE_TRACE(FrameDue, light, now - light->_nextFrameUs, steps);
            light->_nextFrameUs += int64_t(steps) * light->_serviceIntervalUs;
            light->_frameEndUs = light->_nextFrameUs;
#if FADE_TIMING_DIAGNOSTICS
            const int64_t renderStarted = esp_timer_get_time();
#endif
            light->_frameFinished = light->render(steps);
            SERVICE_TRACE(Rendered, light, esp_timer_get_time() - renderStarted,
                          (light->_frameFinished ? 0x80000000u : 0) | steps);
            light->_frameEndUs = 0;
            light->_serviceSteps += steps;
            light->_serviceUpdated = true;
        }
        notify |= light->_notifications.count() != 0 ||
                  (light->_serviceUpdated && light->_serviceCallback != nullptr);
        if(next == 0 || light->_nextFrameUs < next) next = light->_nextFrameUs;
    }
    if(notify && !dispatchQueued) {
        SERVICE_TRACE(Queued, nullptr, 0, 0);
        dispatchQueued = System.queueCallback(dispatch, 0);
        if(!dispatchQueued) {
            SERVICE_TRACE(QueueRejected, nullptr, 0, 0);
            debug_w("RGBWWLed: event queue full; retrying animation notifications");
        }
    }
    SERVICE_TRACE(End, nullptr, lockWait, esp_timer_get_time() - started);
    return next;
}

void RGBWWLedService::task(void*)
{
    for(;;) {
        const int64_t next = service();
        const int64_t remaining = next - esp_timer_get_time();
        // Absolute frame positions survive late wakes; no queue of missed ticks.
        const TickType_t wait = next == 0 ? portMAX_DELAY
            : std::max<TickType_t>(1, pdMS_TO_TICKS(std::max<int64_t>(1, (remaining + 999) / 1000)));
        ulTaskNotifyTake(pdTRUE, wait);
    }
}

void RGBWWLedService::dispatch(uint32_t)
{
#if FADE_TIMING_DIAGNOSTICS
    const int64_t started = esp_timer_get_time();
    SERVICE_TRACE(DispatchBegin, nullptr, 0, 0);
#endif
    std::vector<uint32_t> ids;
    {
        RGBWWLed::Guard guard;
        for(auto* light : lights) ids.push_back(light->_serviceId);
    }
    for(auto id : ids) {
        std::vector<std::pair<String, bool>> notifications;
        RGBWWLed::ServiceCallback callback = nullptr;
        void* argument = nullptr;
        {
            RGBWWLed::Guard guard;
            auto found = std::find_if(lights.begin(), lights.end(),
                [id](RGBWWLed* light) { return light->_serviceId == id; });
            if(found == lights.end()) continue;
            auto* light = *found;
            for(unsigned i = 0; i < light->_notifications.count(); ++i) {
                notifications.emplace_back(light->_notifications.keyAt(i), light->_notifications.valueAt(i));
            }
            light->_notifications.clear();
        }
        for(const auto& notification : notifications) {
            RGBWWLed* light = nullptr;
            {
                RGBWWLed::Guard guard;
                auto found = std::find_if(lights.begin(), lights.end(),
                    [id](RGBWWLed* entry) { return entry->_serviceId == id; });
                if(found != lights.end()) light = *found;
            }
            // Lifecycle operations and dispatch belong to the Sming thread.
            if(light == nullptr) break;
            light->onAnimationFinished(notification.first, notification.second);
        }
        {
            RGBWWLed::Guard guard;
            auto found = std::find_if(lights.begin(), lights.end(),
                [id](RGBWWLed* entry) { return entry->_serviceId == id; });
            if(found == lights.end()) continue;
            auto* light = *found;
            if(light->_serviceUpdated) {
                light->_serviceUpdated = false;
                callback = light->_serviceCallback;
                argument = light->_serviceArgument;
            }
        }
        if(callback != nullptr) callback(argument);
    }
    RGBWWLed::Guard guard;
    dispatchQueued = false;
    SERVICE_TRACE(DispatchEnd, nullptr, esp_timer_get_time() - started, 0);
}
#endif

RGBWWLed::Guard::Guard()
{
#ifdef ARCH_ESP32
    RGBWWLedService::lock();
#endif
}

RGBWWLed::Guard::~Guard()
{
#ifdef ARCH_ESP32
    RGBWWLedService::unlock();
#endif
}

#ifdef ARCH_ESP32
bool RGBWWLed::startService(ServiceCallback callback, void* argument)
{
    Guard guard;
    if(_pwm_output == nullptr || !_pwm_output->isInitialized() || !_animationChannelsConfigured) {
        debug_e("RGBWWLed: cannot service an uninitialized output");
        return false;
    }
    for(unsigned i = 0; i < _ownedAnimationChannelCount; ++i) {
        if(_ownedAnimationChannels[i]->_hwFadeActive) {
            debug_e("RGBWWLed: finish the manual hardware fade before enabling automatic servicing");
            return false;
        }
    }
    _serviceCallback = callback;
    _serviceArgument = argument;
    return RGBWWLedService::attach(*this);
}

void RGBWWLed::stopService()
{
    Guard guard;
    RGBWWLedService::detach(*this);
    _serviceSteps = 0;
    _serviceUpdated = false;
    _serviceFinished = false;
    _frameFinished = false;
    _frameNotifications.clear();
    _notifications.clear();
}

bool RGBWWLed::isServiceRunning() const
{
    Guard guard;
    return _serviceId != 0;
}

bool RGBWWLed::setServiceIntervalUs(uint32_t interval)
{
    Guard guard;
    if(interval < 1000 || interval > 1000000) {
        debug_e("RGBWWLed: invalid service interval %u", unsigned(interval));
        return false;
    }
    _serviceIntervalUs = interval;
    RGBWWLedService::wake();
    return true;
}

uint32_t RGBWWLed::takeServiceSteps()
{
    Guard guard;
    const uint32_t result = _serviceSteps;
    _serviceSteps = 0;
    return result;
}
#endif
