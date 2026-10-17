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
#if defined(ARCH_ESP32)
namespace {
std::vector<uint8_t> collectActivePins(const int* logicalPins, int* channelMap, size_t channelCount)
{
    std::vector<uint8_t> pins;
    pins.reserve(channelCount);
    for(size_t i = 0; i < channelCount; ++i) {
        const int pin = logicalPins[i];
        if(pin < 0 || pin > UINT8_MAX) {
            channelMap[i] = -1;
            continue;
        }
        channelMap[i] = static_cast<int>(pins.size());
        pins.push_back(static_cast<uint8_t>(pin));
    }
    return pins;
}
}

PWMOutput::PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, uint16_t freq)
{
    debug_i("starting PWMoutput (ESP32, with frequency %d)", freq);
    
    // Create a default config and update the frequency
    Esp32HardwarePwm::Config config;
    config.timer.frequency = freq;
    
    // Delegate to the config version
    const int logicalPins[] = { redPin, greenPin, bluePin, wwPin, cwPin };
    auto pins = collectActivePins(logicalPins, _channelMap, RGBWW_CHANNELS::NUM_CHANNELS);
    _pPwm = pins.empty() ? nullptr : new Esp32HardwarePwm(pins, config);
    _dutyRangeFactor = _pPwm != nullptr ? _pPwm->getMaxDuty() / 65535.0f : 0.0f;
    debug_i("max duty %i", _pPwm != nullptr ? _pPwm->getMaxDuty() : 0);
}

PWMOutput::PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, const Esp32HardwarePwm::Config& config)
{
    debug_i("starting PWMoutput (ESP32, custom config)");
    const int logicalPins[] = { redPin, greenPin, bluePin, wwPin, cwPin };
    auto pins = collectActivePins(logicalPins, _channelMap, RGBWW_CHANNELS::NUM_CHANNELS);

    _pPwm = pins.empty() ? nullptr : new Esp32HardwarePwm(pins, config);
    _dutyRangeFactor = _pPwm != nullptr ? _pPwm->getMaxDuty() / 65535.0f : 0.0f;
    debug_i("max duty %i", _pPwm != nullptr ? _pPwm->getMaxDuty() : 0);
}

void PWMOutput::fadeChannel(int chan, int duty_16bit, uint32_t fade_ms) {
    if (chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS || _channelMap[chan] == UNUSED_CHANNEL || _pPwm == nullptr) return;
    const uint32_t scaledDuty = static_cast<uint32_t>(roundf(duty_16bit * _dutyRangeFactor));
    _pPwm->fadeChan(static_cast<uint8_t>(_channelMap[chan]), scaledDuty, fade_ms);
}

bool PWMOutput::isFadingChannel(int chan) {
    return chan >= 0 && chan < RGBWW_CHANNELS::NUM_CHANNELS && _channelMap[chan] != UNUSED_CHANNEL &&
           _pPwm != nullptr && _pPwm->isFadingChan(static_cast<uint8_t>(_channelMap[chan]));
}

#else // ESP8266
PWMOutput::PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, uint16_t freq)
{
    debug_i("starting PWMoutput (ESP8266, with frequency %d)", freq);

	const int logicalPins[] = { redPin, greenPin, bluePin, wwPin, cwPin };
	uint8_t activePins[RGBWW_CHANNELS::NUM_CHANNELS];
	uint8_t activePinCount = 0;
	for(uint8_t i = 0; i < RGBWW_CHANNELS::NUM_CHANNELS; ++i) {
		const int pin = logicalPins[i];
		if(pin < 0 || pin >= 16) {
			_channelMap[i] = UNUSED_CHANNEL;
			continue;
		}
		_channelMap[i] = activePinCount;
		activePins[activePinCount++] = static_cast<uint8_t>(pin);
	}
	_pPwm = activePinCount == 0 ? nullptr : new HardwarePWM(activePins, activePinCount);

    // this period calculation is meant for SDK-PWM or for newPcm when SDK_PWM_PERIOD_COMPAT_MODE is ON
    const int period = int(float(1000) / (float(freq) / float(1000)));
    debug_i("PWM period: %i", period);
    if(_pPwm != nullptr) {
		_pPwm->setPeriod(period);
	}
    _dutyRangeFactor = _pPwm != nullptr ? _pPwm->getMaxDuty() / 65535.0f : 0.0f; // 65535 is the maximum what the linear curve will deliver
    debug_i("max duty %i", _pPwm != nullptr ? _pPwm->getMaxDuty() : 0);
}

#endif

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

    if (_pPwm != nullptr) {
        _pPwm->update();
    }
}
int PWMOutput::getChannel(int chan) {
    if(chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS || _channelMap[chan] == UNUSED_CHANNEL || _pPwm == nullptr) return 0;
    return _pPwm->getDutyChan(static_cast<uint8_t>(_channelMap[chan]));
}

void PWMOutput::setChannel(int chan, int duty, bool update /* = true */) {
    if(chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS || _channelMap[chan] == UNUSED_CHANNEL || _pPwm == nullptr) return;
#if defined(ARCH_ESP32)
    if (isFadingChannel(chan)) return;
#endif
    const uint32 scaledDuty = uint32(roundf(duty * _dutyRangeFactor));
    //if (unsigned(scaledDuty) == _pPwm->getDutyChan(chan))
    //    return;
    //debug_i("setChannel: chan=%d, duty=%d (scaled %d), update=%d", chan, duty, scaledDuty, update);
    _pPwm->setDutyChan(static_cast<uint8_t>(_channelMap[chan]), scaledDuty, update);
}
#else

/*
 * If not using pwm implementation from espressif esp sdk
 * we fallback to the standard arduino pwm implementation
 *
 */
PWMOutput::PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, uint16_t freq /* = 200 */) {

    _pins[RGBWW_CHANNELS::RED] = redPin;
    _pins[RGBWW_CHANNELS::GREEN] = greenPin;
    _pins[RGBWW_CHANNELS::BLUE] = bluePin;
    _pins[RGBWW_CHANNELS::WW] = wwPin;
    _pins[RGBWW_CHANNELS::CW] = cwPin;
    if(redPin >= 0) pinMode(redPin, OUTPUT);
    if(greenPin >= 0) pinMode(greenPin, OUTPUT);
    if(bluePin >= 0) pinMode(bluePin, OUTPUT);
    if(wwPin >= 0) pinMode(wwPin, OUTPUT);
    if(cwPin >= 0) pinMode(cwPin, OUTPUT);
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
    if(_pins[RGBWW_CHANNELS::RED] >= 0) analogWrite(_pins[RGBWW_CHANNELS::RED], value);
}

int PWMOutput::getRed() {
    return _duty[RGBWW_CHANNELS::RED];
}

void PWMOutput::setGreen(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::GREEN] = parseDuty(value);
    if(_pins[RGBWW_CHANNELS::GREEN] >= 0) analogWrite(_pins[RGBWW_CHANNELS::GREEN], value);
}

int PWMOutput::getGreen() {
    return _duty[RGBWW_CHANNELS::GREEN];
}

void PWMOutput::setBlue(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::BLUE] = parseDuty(value);
    if(_pins[RGBWW_CHANNELS::BLUE] >= 0) analogWrite(_pins[RGBWW_CHANNELS::BLUE], value);
}

int PWMOutput::getBlue() {
    return _duty[RGBWW_CHANNELS::BLUE];
}

void PWMOutput::setWarmWhite(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::WW] = parseDuty(value);
    if(_pins[RGBWW_CHANNELS::WW] >= 0) analogWrite(_pins[RGBWW_CHANNELS::WW], value);
}

int PWMOutput::getWarmWhite() {
    return _duty[RGBWW_CHANNELS::WW];
}

void PWMOutput::setColdWhite(int value, bool update /* = true */) {
    _duty[RGBWW_CHANNELS::CW] = parseDuty(value);
    if(_pins[RGBWW_CHANNELS::CW] >= 0) analogWrite(_pins[RGBWW_CHANNELS::CW], value);
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
