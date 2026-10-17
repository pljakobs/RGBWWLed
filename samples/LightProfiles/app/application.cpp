#include <SmingCore.h>
#include <RGBWWLed.h>

#ifndef RGBWW_SAMPLE_PROFILE
#define RGBWW_SAMPLE_PROFILE 0
#endif

static constexpr LightType lightType = static_cast<LightType>(RGBWW_SAMPLE_PROFILE);
static constexpr RGBWW_CHANNELS singleChannel = RGBWW_CHANNELS::GREEN;

#if defined(ARCH_ESP32)
static int pins[RGBWW_CHANNELS::NUM_CHANNELS] = {13, 12, 14, 27, 26};
#else
static int pins[RGBWW_CHANNELS::NUM_CHANNELS] = {14, 12, 13, 5, 4};
#endif

static RGBWWLed led;
static SimpleTimer showTimer;
static SimpleTimer commandTimer;
static bool useHsv = true;

static void configurePins()
{
    switch (lightType) {
    case LightType::RGBWWCW:
        break;
    case LightType::RGBWW:
        pins[RGBWW_CHANNELS::CW] = -1;
        break;
    case LightType::RGBCW:
        pins[RGBWW_CHANNELS::WW] = -1;
        break;
    case LightType::RGB:
        pins[RGBWW_CHANNELS::WW] = -1;
        pins[RGBWW_CHANNELS::CW] = -1;
        break;
    case LightType::WWCW:
        pins[RGBWW_CHANNELS::RED] = -1;
        pins[RGBWW_CHANNELS::GREEN] = -1;
        pins[RGBWW_CHANNELS::BLUE] = -1;
        break;
    case LightType::WW:
        pins[RGBWW_CHANNELS::RED] = -1;
        pins[RGBWW_CHANNELS::GREEN] = -1;
        pins[RGBWW_CHANNELS::BLUE] = -1;
        pins[RGBWW_CHANNELS::CW] = -1;
        break;
    case LightType::CW:
        pins[RGBWW_CHANNELS::RED] = -1;
        pins[RGBWW_CHANNELS::GREEN] = -1;
        pins[RGBWW_CHANNELS::BLUE] = -1;
        pins[RGBWW_CHANNELS::WW] = -1;
        break;
    case LightType::SINGLE:
        pins[RGBWW_CHANNELS::RED] = -1;
        pins[RGBWW_CHANNELS::BLUE] = -1;
        pins[RGBWW_CHANNELS::WW] = -1;
        pins[RGBWW_CHANNELS::CW] = -1;
        break;
    }
}

static void setHsvTarget()
{
    RequestHSVCT color;
    color.h = AbsOrRelValue(210, AbsOrRelValue::Type::Hue);
    color.s = AbsOrRelValue(85, AbsOrRelValue::Type::Percent);
    color.v = AbsOrRelValue(65, AbsOrRelValue::Type::Percent);
    color.ct = AbsOrRelValue(4200, AbsOrRelValue::Type::Ct);
    led.fadeHSV(color, RampTimeOrSpeed(3000), QueuePolicy::Single);
}

static void setRawTarget()
{
    RequestChannelOutput output;
    output.r = AbsOrRelValue(800, AbsOrRelValue::Type::Raw);
    output.g = AbsOrRelValue(250, AbsOrRelValue::Type::Raw);
    output.b = AbsOrRelValue(100, AbsOrRelValue::Type::Raw);
    output.ww = AbsOrRelValue(600, AbsOrRelValue::Type::Raw);
    output.cw = AbsOrRelValue(300, AbsOrRelValue::Type::Raw);
    led.fadeRAW(output, RampTimeOrSpeed(3000), QueuePolicy::Single);
}

static void updateLed()
{
    led.show();
}

static void changeTarget()
{
    useHsv = !useHsv;
    if (useHsv) {
        setHsvTarget();
    } else {
        setRawTarget();
    }
}

void init()
{
    Serial.begin(SERIAL_BAUD_RATE);
    configurePins();

#if defined(ARCH_ESP32)
    Esp32HardwarePwm::Config config;
    config.timer.frequency = 1000;
    led.init(pins[0], pins[1], pins[2], pins[3], pins[4], lightType, config, singleChannel);
#else
    led.init(pins[0], pins[1], pins[2], pins[3], pins[4], lightType, 1000, singleChannel);
#endif

    setHsvTarget();
    showTimer.initializeMs<RGBWW_MINTIMEDIFF>(updateLed).start();
    commandTimer.initializeMs<5000>(changeTarget).start();
}
