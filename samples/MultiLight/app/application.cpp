#include <SmingCore.h>
#include <RGBWWLed.h>
#include <soc/soc_caps.h>

#if !defined(ARCH_ESP32)
#error This sample requires the ESP32 architecture
#endif

#if !SOC_LEDC_SUPPORT_HS_MODE
#error This sample requires an ESP32 SoC with low-speed and high-speed LEDC banks
#endif

static RGBWWLed rgbwwcw;
static RGBWWLed rgb;
static RGBWWLed wwcw;
static RGBWWLed single;
static SimpleTimer showTimer;
static SimpleTimer targetTimer;
static bool useHsv = true;

static Esp32HardwarePwm::Config makeBankConfig(ledc_mode_t speedMode, ledc_channel_t channelStart)
{
    Esp32HardwarePwm::Config config;
    config.channelStart = channelStart;
    config.timer.speed_mode = speedMode;
    config.timer.timer_num = LEDC_TIMER_0;
    config.timer.frequency = 1000;
    config.timer.resolution = LEDC_TIMER_10_BIT;
    return config;
}

static void setHsvTargets()
{
    RequestHSVCT color;
    color.h = AbsOrRelValue(205, AbsOrRelValue::Type::Hue);
    color.s = AbsOrRelValue(80, AbsOrRelValue::Type::Percent);
    color.v = AbsOrRelValue(65, AbsOrRelValue::Type::Percent);
    color.ct = AbsOrRelValue(4200, AbsOrRelValue::Type::Ct);
    const RampTimeOrSpeed ramp(3000);

    rgbwwcw.fadeHSV(color, ramp, QueuePolicy::Single);
    rgb.fadeHSV(color, ramp, QueuePolicy::Single);
    wwcw.fadeHSV(color, ramp, QueuePolicy::Single);
    single.fadeHSV(color, ramp, QueuePolicy::Single);
}

static void setRawTargets()
{
    RequestChannelOutput output;
    output.r = AbsOrRelValue(800, AbsOrRelValue::Type::Raw);
    output.g = AbsOrRelValue(250, AbsOrRelValue::Type::Raw);
    output.b = AbsOrRelValue(100, AbsOrRelValue::Type::Raw);
    output.ww = AbsOrRelValue(600, AbsOrRelValue::Type::Raw);
    output.cw = AbsOrRelValue(300, AbsOrRelValue::Type::Raw);
    const RampTimeOrSpeed ramp(3000);

    rgbwwcw.fadeRAW(output, ramp, QueuePolicy::Single);
    rgb.fadeRAW(output, ramp, QueuePolicy::Single);
    wwcw.fadeRAW(output, ramp, QueuePolicy::Single);
    single.fadeRAW(output, ramp, QueuePolicy::Single);
}

static void updateLeds()
{
    rgbwwcw.show();
    rgb.show();
    wwcw.show();
    single.show();
}

static void changeTargets()
{
    useHsv = !useHsv;
    if (useHsv) {
        setHsvTargets();
    } else {
        setRawTargets();
    }
}

void init()
{
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.systemDebugOutput(true);

    const auto lowSpeed = makeBankConfig(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    const auto highSpeed = makeBankConfig(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    // Low-speed bank: 5-channel RGBWWCW followed by 3-channel RGB (8 total).
    rgbwwcw.init(25, 26, 27, 32, 33, LightType::RGBWWCW, lowSpeed);
    auto rgbConfig = lowSpeed;
    rgbConfig.channelStart = LEDC_CHANNEL_5;
    rgb.init(18, 19, 21, -1, -1, LightType::RGB, rgbConfig);

    // High-speed bank: 2-channel WWCW followed by one green SINGLE output.
    wwcw.init(-1, -1, -1, 22, 23, LightType::WWCW, highSpeed);
    auto singleConfig = highSpeed;
    singleConfig.channelStart = LEDC_CHANNEL_2;
    single.init(-1, 4, -1, -1, -1, LightType::SINGLE, singleConfig, RGBWW_CHANNELS::GREEN);

    setHsvTargets();
    showTimer.initializeMs<RGBWW_MINTIMEDIFF>(updateLeds).start();
    targetTimer.initializeMs<5000>(changeTargets).start();
}
