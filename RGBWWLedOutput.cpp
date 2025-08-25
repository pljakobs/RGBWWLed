/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */
#include "RGBWWLed.h"
#include "RGBWWLedOutput.h"

#ifdef RGBWW_USE_ESP_HWPWM
/*
 *  Use PWM Code from espressif sdk
 *  Provides a more stable pwm implementation compared to arduino esp
 *  framework
 */

PWMOutput::PWMOutput(uint8_t redPin, uint8_t greenPin, uint8_t bluePin, uint8_t wwPin, uint8_t cwPin, uint16_t freq /* = 200 */) {
    debug_i("starting PWMoutput");
    #if ARCH_ESP32
    std::vector<uint8_t> pins = { redPin, greenPin, bluePin, wwPin, cwPin };

    Esp32HwPwmConfig pwmConfig;
    pwmConfig.channelStart = LEDC_CHANNEL_0;
    pwmConfig.spreadSpectrum.mode = SpreadSpectrumMode::ON;
    pwmConfig.spreadSpectrum.WidthPercent = 15; // 15% modulation
    pwmConfig.spreadSpectrum.Subsampling = 1;
    pwmConfig.spreadSpectrum.StepsizeHz = 5;
    pwmConfig.phaseShift.mode = PhaseShiftMode::AUTO;
    pwmConfig.timer.timer_num = LEDC_TIMER_0;
    pwmConfig.timer.frequency= 4000;

    _pPwm = new Esp32HardwarePwm(pins, pwmConfig);
    #else
    uint8_t pins[] = { redPin, greenPin, bluePin, wwPin, cwPin };
    _pPwm = new HardwarePWM(pins, sizeof(pins));

    // this period calculation is meant for SDK-PWM or for newPcm when SDK_PWM_PERIOD_COMPAT_MODE is ON
    const int period = int(float(1000) / (float(freq) / float(1000)));
    debug_i("PWM period: %i", period);
    _pPwm->setPeriod(period);
    #endif
    _dutyRangeFactor = _pPwm->getMaxDuty() / 65535.0f; // 65535 is the maximum what the linear curve will deliver
    debug_i("max duty %i", _pPwm->getMaxDuty());
    

}

PWMOutput::~PWMOutput() {
    delete _pPwm;
    _pPwm = nullptr;
}

void PWMOutput::setRed(int duty, bool update /* = true */) {
    setChannel(RGBWW_CHANNELS::RED, duty, update);
}

int PWMOutput::getRed() {
    return getChannel(RGBWW_CHANNELS::RED);
}

void PWMOutput::setGreen(int duty, bool update /* = true */) {
    setChannel(RGBWW_CHANNELS::GREEN, duty, update);
}

int PWMOutput::getGreen() {
    return getChannel(RGBWW_CHANNELS::GREEN);
}

void PWMOutput::setBlue(int duty, bool update /* = true */) {
    setChannel(RGBWW_CHANNELS::BLUE, duty, update);
}

int PWMOutput::getBlue() {
    return getChannel(RGBWW_CHANNELS::BLUE);
}

void PWMOutput::setWarmWhite(int duty, bool update /* = true */) {
    setChannel(RGBWW_CHANNELS::WW, duty, update);
}

int PWMOutput::getWarmWhite() {
    return getChannel(RGBWW_CHANNELS::WW);
}

void PWMOutput::setColdWhite(int duty, bool update /* = true */) {
    setChannel(RGBWW_CHANNELS::CW, duty, update);
}
int PWMOutput::getColdWhite() {
    return getChannel(RGBWW_CHANNELS::CW);
}

void PWMOutput::setOutput(int red, int green, int blue, int warmwhite, int coldwhite) {
    setRed(red, false);
    setGreen(green, false);
    setBlue(blue, false);
    setWarmWhite(warmwhite, false);
    setColdWhite(coldwhite, false);

    _pPwm->update();
}
int PWMOutput::getChannel(int chan) {
    return _pPwm->getDutyChan(chan);
}

void PWMOutput::setChannel(int chan, int duty, bool update /* = true */) {
    const uint32 scaledDuty = uint32(roundf(duty * _dutyRangeFactor));
    //if (unsigned(scaledDuty) == _pPwm->getDutyChan(chan))
    //    return;
    //debug_i("setChannel: chan=%d, duty=%d (scaled %d), update=%d", chan, duty, scaledDuty, update);
    _pPwm->setDutyChan(chan, scaledDuty, update);
}
#else

/*
 * If not using pwm implementation from espressif esp sdk
 * we fallback to the standard arduino pwm implementation
 *
 */
PWMOutput::PWMOutput(uint8_t redPin, uint8_t greenPin, uint8_t bluePin, uint8_t wwPin, uint8_t cwPin, uint16_t freq /* = 200 */) {

    _pins[RGBWW_CHANNELS::RED] = redPin;
    _pins[RGBWW_CHANNELS::GREEN] = greenPin;
    _pins[RGBWW_CHANNELS::BLUE] = bluePin;
    _pins[RGBWW_CHANNELS::WW] = wwPin;
    _pins[RGBWW_CHANNELS::CW] = cwPin;
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    pinMode(wwPin, OUTPUT);
    pinMode(cwPin, OUTPUT);
    setFrequency(freq);
    _maxduty = RGBWW_ARDUINO_MAXDUTY;

}

void PWMOutput::setFrequency(int freq) {
    _freq = freq;
    analogWriteFreq(freq);
}

int PWMOutput::getFrequency() {
    return _freq;
}

void PWMOutput::setRed(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::RED] = parseDuty(value);
    analogWrite(_pins[RGBWW_CHANNELS::RED], value);
}

int PWMOutput::getRed() {
    return _duty[RGBWW_CHANNELS::RED];
}

void PWMOutput::setGreen(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::GREEN] = parseDuty(value);
    analogWrite(_pins[RGBWW_CHANNELS::GREEN], value);
}

int PWMOutput::getGreen() {
    return _duty[RGBWW_CHANNELS::GREEN];
}

void PWMOutput::setBlue(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::BLUE] = parseDuty(value);
    analogWrite(_pins[RGBWW_CHANNELS::BLUE], value);
}

int PWMOutput::getBlue() {
    return _duty[RGBWW_CHANNELS::BLUE];
}

void PWMOutput::setWarmWhite(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::WW] = parseDuty(value);
    analogWrite(_pins[RGBWW_CHANNELS::WW], value);
}

int PWMOutput::getWarmWhite() {
    return _duty[RGBWW_CHANNELS::WW];
}

void PWMOutput::setColdWhite(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::CW] = parseDuty(value);
    analogWrite(_pins[RGBWW_CHANNELS::CW], value);
}

int PWMOutput::getColdWhite() {
    return _duty[RGBWW_CHANNELS::CW];
}

void PWMOutput::setOutput(int red, int green, int blue, int warmwhite, int coldwhite) {
    setRed(red);
    setGreen(green);
    setBlue(blue);
    setWarmWhite(warmwhite);
    setColdWhite(coldwhite);
}

int PWMOutput::parseDuty(int duty) {
    return (duty * _maxduty) / RGBWW_CALC_WIDTH;
}
#endif //RGBWW_USE_ESP_HWPWM
