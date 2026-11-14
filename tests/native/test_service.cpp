#include <SmingCore.h>
#include <array>
#include <iostream>
#include <memory>
#include <Wiring/WHashMap.h>
#define private public
#include "../../RGBWWLed.h"
#include "../../RGBWWLedService.h"
#include <FadeWorker.h>
#undef private

#if FADE_TIMING_DIAGNOSTICS
static std::vector<RGBWWServiceTrace> traceEvents;
void rgbwwLedTimingTrace(RGBWWServiceTrace event, const RGBWWLed*, uint32_t, uint32_t)
{
    traceEvents.push_back(event);
}
#endif

struct Light : RGBWWLed {
    unsigned completions = 0;
    std::vector<String> names;
    std::vector<bool> requeued;
    void onAnimationFinished(const String& name, bool repeat) override {
        if(isServiceRunning()) assert(Mock::locks == 0);
        ++completions;
        names.push_back(name);
        requeued.push_back(repeat);
    }
};

static void dispatch()
{
    while(!Mock::callbacks.empty()) Mock::dispatch();
}

static void completeFrames()
{
    for(auto* light : RGBWWLedService::lights) {
        for(const auto& binding : light->getPwmOutput()->getChannels()) {
            if(binding.bank != nullptr && binding.bank->isFadingChan(binding.channel)) {
                // Tests bind channel index to hardware channel index in each bank.
                const int mode = binding.bank->timer_.speed_mode;
                Mock::complete(mode, binding.channel);
            }
        }
    }
    FadeWorker::service();
}

static RequestChannelOutput raw(int value)
{
    RequestChannelOutput output;
    output.ww = AbsOrRelValue(value);
    return output;
}

static void testSharedWorker()
{
    Mock::now = 100000;
    std::vector<uint8_t> pins{1, 2, 3, 4, 5, 6, 7, 8};
    Esp32HardwarePwm::Config config;
    config.timer.frequency = 4000;
    Esp32HardwarePwm first(pins, config);
    config.timer.speed_mode = 1;
    Esp32HardwarePwm second(pins, config);
    std::array<std::unique_ptr<Light>, 16> lights;
    unsigned updates = 0;
    for(unsigned i = 0; i < lights.size(); ++i) {
        lights[i] = std::make_unique<Light>();
        RGBWWPwmChannels bindings{};
        bindings[RGBWW_CHANNELS::WW] = {i < 8 ? &first : &second, uint8_t(i % 8)};
        assert(lights[i]->init(LightType::SINGLE, bindings));
        assert(lights[i]->fadeRAW(raw(1023), RampTimeOrSpeed(3000), QueuePolicy::Single, false, "fade"));
        assert(lights[i]->startService(i == 0 ? +[](void* arg) { ++*static_cast<unsigned*>(arg); } : nullptr, &updates));
    }
    assert(Mock::tasks == 2 && Mock::timers == 1); // PWM owns the only ESP timer.
    RGBWWLedService::service();
#if FADE_TIMING_DIAGNOSTICS
    assert(traceEvents.front() == RGBWWServiceTrace::Begin);
    assert(traceEvents.back() == RGBWWServiceTrace::End);
    assert(std::count(traceEvents.begin(), traceEvents.end(), RGBWWServiceTrace::Rendered) == 16);
    assert(std::count(traceEvents.begin(), traceEvents.end(), RGBWWServiceTrace::Queued) == 1);
#endif
    assert(Mock::starts == 16);
    const int firstStep = lights[0]->getCurrentOutput().ww;
    assert(firstStep > 0 && firstStep < 20);
    for(auto& light : lights) {
        assert(!light->show());
        assert(light->getCurrentOutput().ww == firstStep);
        assert(light->completions == 0);
    }
    dispatch();
    assert(updates == 1);
    // Missing a frame advances directly to the third target, not a second frame.
    completeFrames();
    Mock::now = 140000;
    RGBWWLedService::service();
    assert(lights[0]->takeServiceSteps() == 3);
    assert(lights[0]->getCurrentOutput().ww > firstStep * 2);
    // A blocked event loop does not stop any of the sixteen fades.
    for(int64_t now = 160000; now <= 3080000; now += 20000) {
        completeFrames();
        Mock::now = now;
        RGBWWLedService::service();
        assert(Mock::callbacks.size() <= 1);
    }
    assert(lights[0]->getCurrentOutput().ww == 1023);
    assert(lights[0]->completions == 0);
    completeFrames();
    Mock::now = 3100000;
    RGBWWLedService::service();
    dispatch();
    for(auto& light : lights) {
        assert(light->completions == 1);
        assert(light->show());
        assert(!light->show());
    }
    // Queued updates must not dereference a removed light.
    Mock::now = 3120000;
    RGBWWLedService::service();
    lights[0].reset();
    dispatch();
    for(auto& light : lights) light.reset();
    assert(RGBWWLedService::lights.empty());
    assert(RGBWWLedService::service() == 0);
    dispatch();
}

static void testQueueAndPause()
{
    Mock::now = 4000000;
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm bank(pins);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    Light light;
    assert(light.init(LightType::SINGLE, bindings));
    assert(light.setRAW(raw(100), 40, QueuePolicy::Single, false, "hold"));
    assert(light.fadeRAW(raw(500), RampTimeOrSpeed(80), QueuePolicy::Back, false, "ramp"));
    assert(light.startService());
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 100);
    Mock::now = 4080000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 400); // two hold + three ramp steps
    dispatch();
    light.pauseAnimation();
    const auto paused = light.getCurrentOutput().ww;
    completeFrames();
    Mock::now = 4180000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == paused);
    light.continueAnimation();
    Mock::now = 4200000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 500);
    completeFrames();
    Mock::now = 4220000;
    RGBWWLedService::service();
    dispatch();
    assert(light.completions == 2 && light.names[0] == "hold" && light.names[1] == "ramp");
    light.stopService();
    const auto stopped = light.getCurrentOutput().ww;
    Mock::now = 4500000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == stopped);
    assert(light.startService());
    RGBWWLedService::service();
    light.stopService();
    dispatch();
}

static void testAdvanceEquivalence()
{
    RGBWWLed light;
    for(int target : {1, 7, 37, 1023}) {
        for(int count : {1, 3, 10, 31}) {
            AnimTransition sequential(AbsOrRelValue(1023), AbsOrRelValue(target),
                RampTimeOrSpeed(620), &light, CtrlChannel::Val);
            AnimTransition skipped(AbsOrRelValue(1023), AbsOrRelValue(target),
                RampTimeOrSpeed(620), &light, CtrlChannel::Val);
            bool finished = false;
            for(int i = 0; i < count; ++i) finished = sequential.run();
            uint32_t steps = count;
            assert(skipped.advance(steps) == finished);
            assert(steps == 0 && skipped.getAnimValue() == sequential.getAnimValue());
        }
    }
    AnimTransitionCircularHue sequential(AbsOrRelValue(6000), AbsOrRelValue(100),
        RampTimeOrSpeed(200), 1, &light, CtrlChannel::Hue);
    AnimTransitionCircularHue skipped(AbsOrRelValue(6000), AbsOrRelValue(100),
        RampTimeOrSpeed(200), 1, &light, CtrlChannel::Hue);
    for(int i = 0; i < 7; ++i) sequential.run();
    uint32_t steps = 7;
    assert(!skipped.advance(steps));
    assert(skipped.getAnimValue() == sequential.getAnimValue());
}

static void testPhysicalFrames()
{
    Mock::now = 5000000;
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm::Config config;
    config.timer.frequency = 4000;
    Esp32HardwarePwm bank(pins, config);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    Light light;
    assert(light.init(LightType::SINGLE, bindings));
    light.colorutils.setBrightnessCorrection(100, 100, 100, 50, 100);
    RequestHSVCT hsv;
    hsv.v = AbsOrRelValue(1023);
    assert(light.fadeHSV(hsv, RampTimeOrSpeed(200), QueuePolicy::Single));
    assert(light.startService());
    RGBWWLedService::service();
    completeFrames();
    Mock::now = 5080000;
    RGBWWLedService::service();
    const auto color = light.getCurrentColor();
    const auto output = light.getCurrentOutput();
    assert(color.v > 500 && color.v < 520);
    assert(output.ww > 240 && output.ww < 260);
    const uint32_t expected = std::lround(light.getDimCurveValue(output.ww) * (bank.getMaxDuty() / 65535.0f));
    assert(Mock::target[0][0] == expected);
    assert(bank.fadeQueues_[0].workerDurationMs <= 19);
    // Replacing an active microfade never accumulates a frame backlog.
    for(int i = 0; i < 20; ++i) {
        light.getPwmOutput()->setFrame({0, 0, 0, i % 2 ? 30000 : 40000, 0}, 0x08, Mock::now + 20000);
        assert(bank.getQueueEntries(0) <= 1);
    }
    completeFrames();
    Mock::now += 20000;
    completeFrames();
    light.getPwmOutput()->setFrame({0, 0, 0, 65535, 0}, 0x08, Mock::now - 1000);
    assert(bank.getDutyChan(0) == bank.getMaxDuty());
    light.stopService();
    dispatch();
}

static void testLateCommandsAndDispatch()
{
    Mock::now = 6000000;
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm bank(pins);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    auto light = std::make_unique<Light>();
    assert(light->init(LightType::SINGLE, bindings));
    assert(light->startService(+[](void* argument) {
        static_cast<std::unique_ptr<Light>*>(argument)->reset();
    }, &light));
    RGBWWLedService::service();
    // Replace a command after a long blocked pass: don't count time before acceptance.
    Mock::now = 6599000;
    assert(light->fadeRAW(raw(1000), RampTimeOrSpeed(200)));
    Mock::now = 6600000;
    RGBWWLedService::service();
    assert(light->getCurrentOutput().ww == 100);
    dispatch();
    assert(!light && RGBWWLedService::lights.empty());
    // Failed posting is retried, without losing the completed animation.
    Light retry;
    assert(retry.init(LightType::SINGLE, bindings));
    assert(retry.setRAW(raw(200), QueuePolicy::Single, false, "instant"));
    assert(retry.startService());
    RGBWWLedService::service();
    Mock::rejectPosts = true;
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(Mock::callbacks.empty() && retry.completions == 0);
    Mock::rejectPosts = false;
    Mock::now += 20000;
    RGBWWLedService::service();
    dispatch();
    assert(retry.completions == 1);
    assert(!retry.setServiceIntervalUs(0));
    assert(retry.setServiceIntervalUs(10000));
    retry.stopService();
    RGBWWLed uninitialized;
    assert(!uninitialized.startService());
}

static void testConcurrentCommands()
{
    Mock::now = 7000000;
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm bank(pins);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    Light light;
    assert(light.init(LightType::SINGLE, bindings));
    assert(light.startService());
    std::thread render([] {
        for(int i = 0; i < 250; ++i) {
            Mock::now += 20000;
            RGBWWLedService::service();
        }
    });
    for(int i = 0; i < 250; ++i) {
        assert(light.fadeRAW(raw(i % 1024), RampTimeOrSpeed(200)));
        const auto snapshot = light.getCurrentOutput();
        assert(snapshot.ww >= 0 && snapshot.ww <= 1023);
        RGBWWLed::Guard guard;
        light.colorutils.setBrightnessCorrection(100, 100, 100, 100, 100);
    }
    render.join();
    assert(Mock::callbacks.size() <= 1);
    light.stopService();
    dispatch();
}

static void testManualCompatibility()
{
    Mock::now = 13000000;
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm bank(pins);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    Light light;
    assert(light.init(LightType::SINGLE, bindings));
    ChannelOutput initial(0, 0, 0, 500, 0);
    light.setOutput(initial);
    assert(light.fadeRAW(raw(0), RampTimeOrSpeed(100)));
    assert(!light.show());
    assert(light.getCurrentOutput().ww == 400);
    assert(bank.isFadingChan(0));
    assert(!light.startService());
    Mock::complete(0, 0);
    FadeWorker::service();
    assert(light.show());
    assert(light.getCurrentOutput().ww == 0);
    assert(light.startService());
    light.stopService();
    dispatch();
}

static void testModeSwitchBaseline()
{
    Mock::now = 13500000;
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm bank(pins);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    Light light;
    assert(light.init(LightType::SINGLE, bindings));
    assert(light.startService());
    assert(light.setRAW(raw(200)));
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 200);
    RequestHSVCT black;
    black.v = AbsOrRelValue(0);
    assert(light.setHSV(black));
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 0);
    assert(light.fadeRAW(raw(200), RampTimeOrSpeed(200)));
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 20);
    light.stopService();
    dispatch();
}

static void testInterruptionAndRequeue()
{
    Mock::now = 14000000;
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm bank(pins);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    Light light;
    assert(light.init(LightType::SINGLE, bindings));
    assert(light.setRAW(raw(100)));
    assert(light.startService());
    RGBWWLedService::service();
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(light.fadeRAW(raw(500), RampTimeOrSpeed(200), QueuePolicy::Single, false, "ramp"));
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 140);
    completeFrames();
    assert(light.setRAW(raw(900), 40, QueuePolicy::FrontReset, false, "interrupt"));
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 900);
    Mock::now += 20000;
    RGBWWLedService::service();
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(light.getCurrentOutput().ww == 860);
    completeFrames();
    light.skipAnimation();
    Mock::now += 20000;
    RGBWWLedService::service();
    assert(light.setRAW(raw(0), 40, QueuePolicy::Single, true, "repeat"));
    Mock::now += 20000;
    RGBWWLedService::service();
    Mock::now += 100000;
    RGBWWLedService::service();
    Mock::now += 20000;
    RGBWWLedService::service();
    dispatch();
    assert(light.names.back() == "repeat" && light.requeued.back());
    light.clearAnimationQueue();
    light.skipAnimation();
    Mock::now += 20000;
    RGBWWLedService::service();
    Mock::now += 20000;
    RGBWWLedService::service();
    dispatch();
    assert(light.names.back() == "repeat" && !light.requeued.back());
    light.stopService();
}

static void testLinearRawOutput()
{
    Mock::now = 16000000;
    Esp32HardwarePwm::Config config;
    config.timer.resolution = static_cast<ledc_timer_bit_t>(11);
    std::vector<uint8_t> pins{1};
    Esp32HardwarePwm bank(pins, config);
    RGBWWPwmChannels bindings{};
    bindings[RGBWW_CHANNELS::WW] = {&bank, 0};
    Light light;
    assert(light.init(LightType::SINGLE, bindings));
    light.colorutils.setBrightnessCorrection(50, 50, 50, 50, 50);
    assert(light.startService());
    const std::array<std::array<int, 2>, 7> samples{{
        {0, 0}, {1, 2}, {5, 10}, {7, 14}, {8, 16}, {511, 1022}, {1023, 2047}
    }};
    for(const auto& sample : samples) {
        const int value = sample[0];
        assert(light.setRAW(raw(value)));
        Mock::now += 20000;
        RGBWWLedService::service();
        assert(light.getCurrentOutput().ww == value);
        assert(bank.getDutyChan(0) == unsigned(sample[1]));
        assert(light.getPwmDutyForChannel(RGBWW_CHANNELS::WW, value) == light.linearPwmDuty(value));
    }
    assert(light.linearPwmDuty(-1) == 0);
    assert(light.linearPwmDuty(1024) == 65535);
    assert(light.fadeRAW(raw(0), RampTimeOrSpeed(200)));
    Mock::now += 20000;
    RGBWWLedService::service();
    auto value = light.getCurrentOutput().ww;
    assert(value > 0 && value < 1023);
    assert(Mock::target[0][0] == unsigned(std::lround(
        light.linearPwmDuty(value) * (bank.getMaxDuty() / 65535.0f))));
    completeFrames();
    RequestHSVCT hsv;
    hsv.v = AbsOrRelValue(512);
    assert(light.setHSV(hsv));
    Mock::now += 20000;
    RGBWWLedService::service();
    ChannelOutput corrected(0, 0, 0, 512, 0);
    light.colorutils.correctBrightness(corrected);
    assert(light.getCurrentOutput().ww == corrected.ww);
    assert(bank.getDutyChan(0) == unsigned(std::lround(
        light.getDimCurveValue(corrected.ww) * (bank.getMaxDuty() / 65535.0f))));
    light.stopService();
    dispatch();
}

int main()
{
    testAdvanceEquivalence();
    testSharedWorker();
    testQueueAndPause();
    testPhysicalFrames();
    testLateCommandsAndDispatch();
    testConcurrentCommands();
    testManualCompatibility();
    testModeSwitchBaseline();
    testInterruptionAndRequeue();
    testLinearRawOutput();
    std::cout << "shared animation worker, frame skipping, queue, pause and notification tests passed\n";
}
