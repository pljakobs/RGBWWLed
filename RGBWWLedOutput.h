/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */
#ifndef RGBWWLedOutput_h
#define RGBWWLedOutput_h
#include "RGBWWLed.h"
/*
#ifdef ARCH_ESP32
    #include <Esp32HardwarePwm.h>
#endif
*/
#ifdef RGBWW_USE_ESP_HWPWM

/*
 *  Use PWM Code from espressif sdk
 *  Provides a more stable pwm implementation compared to arduino esp
 *  framework
 */

class PWMOutput {

public:
#ifdef ARCH_ESP32
    PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, uint16_t freq = 2000);
    PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, const Esp32HardwarePwm::Config& config);
    PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, const RGBWWPwmConfig& config);
#else 
    PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, uint16_t freq = 200);
#endif
    ~PWMOutput();

    void setRed(int value, bool update = true);
    int getRed();
    void setGreen(int value, bool update = true);
    int getGreen();
    void setBlue(int value, bool update = true);
    int getBlue();
    void setWarmWhite(int value, bool update = true);
    int getWarmWhite();
    void setColdWhite(int value, bool update = true);
    int getColdWhite();
    void setOutput(int red, int green, int blue, int warmwhite, int coldwhite);

    int getChannel(int chan);
    void setChannel(int channel, int duty, bool update = true);

    #ifdef ARCH_ESP32
    void fadeChannel(int chan, int duty_16bit, uint32_t fade_ms, bool queue = false);
    bool fadeChannelWithCycleLimit(int chan, int duty_16bit, uint32_t fade_ms, uint32_t maxCycles, bool queue = false);
    void resetFadeQueue(int chan);
    bool isFadingChannel(int chan);
    bool hasPendingCallbacks() const {
        return (_pPwm[0] != nullptr && _pPwm[0]->hasPendingCallbacks()) ||
               (_pPwm[1] != nullptr && _pPwm[1]->hasPendingCallbacks());
    }
#endif

private:
    static constexpr int UNUSED_CHANNEL = -1;
    int parseDuty(int duty);
#ifdef ARCH_ESP32
    float _dutyRangeFactor[2] = {0.0f, 0.0f};
    uint8_t _channelBank[RGBWW_CHANNELS::NUM_CHANNELS] = {};
    Esp32HardwarePwm* _pPwm[2] = {nullptr, nullptr};
#else
    float _dutyRangeFactor = 0.0f;
    HardwarePWM* _pPwm;
#endif
    int _channelMap[RGBWW_CHANNELS::NUM_CHANNELS] = {
        UNUSED_CHANNEL, UNUSED_CHANNEL, UNUSED_CHANNEL, UNUSED_CHANNEL, UNUSED_CHANNEL
    };
};

#else

/*
 * If not using pwm implementation from espressif esp sdk
 * we fallback to the standard arduino pwm implementation
 *
 */

class PWMOutput
{

public:
    PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, uint16_t freq = 200);

    void setFrequency(int freq);
    int getFrequency();
    void setRed(int value, bool update = true);
    int getRed();
    void setGreen(int value, bool update = true);
    int getGreen();
    void setBlue(int value, bool update = true);
    int getBlue();
    void setWarmWhite(int value, bool update = true);
    int getWarmWhite();
    void setColdWhite(int value, bool update = true);
    int getColdWhite();
    void setOutput(int red, int green, int blue, int warmwhite, int coldwhite);

private:
    static constexpr int UNUSED_PIN = -1;
    int _freq;
    int _pins[RGBWW_CHANNELS::NUM_CHANNELS];
    int _duty[RGBWW_CHANNELS::NUM_CHANNELS];
    int _maxduty;
    int parseDuty(int duty);

};
#endif //RGBWW_USE_ESP_HWPWM

#endif //RGBWWLedOutput_h
