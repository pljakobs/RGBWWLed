/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */
#include "RGBWWLedOutput.h"

#ifdef RGBWW_USE_ESP_HWPWM
/*
 *  Use PWM Code from espressif sdk
 *  Provides a more stable pwm implementation compared to arduino esp
 *  framework
 */
#if defined(ARCH_ESP32)
namespace {
RGBWWPwmConfig defaultPwmConfig(uint16_t frequency)
{
    RGBWWPwmConfig config;
    config.primary.timer.frequency = frequency;
    return config;
}

RGBWWPwmConfig singleBankConfig(const Esp32HardwarePwm::Config& pwmConfig)
{
    RGBWWPwmConfig config;
    config.primary = pwmConfig;
    return config;
}

void collectActivePins(const int* logicalPins, int* channelMap, uint8_t* channelBank, size_t channelCount,
                       uint8_t secondaryChannelMask, std::vector<uint8_t>* bankPins)
{
    for(size_t i = 0; i < channelCount; ++i) {
        const int pin = logicalPins[i];
        if(pin < 0 || pin > UINT8_MAX) {
            channelMap[i] = -1;
            continue;
        }
        const uint8_t bank = (secondaryChannelMask & (1U << i)) != 0 ? 1 : 0;
        channelBank[i] = bank;
        channelMap[i] = static_cast<int>(bankPins[bank].size());
        bankPins[bank].push_back(static_cast<uint8_t>(pin));
    }
}
}

PWMOutput::PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, uint16_t freq)
    : PWMOutput(redPin, greenPin, bluePin, wwPin, cwPin, defaultPwmConfig(freq))
{
}

PWMOutput::PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, const Esp32HardwarePwm::Config& config)
    : PWMOutput(redPin, greenPin, bluePin, wwPin, cwPin, singleBankConfig(config))
{
}

PWMOutput::PWMOutput(int redPin, int greenPin, int bluePin, int wwPin, int cwPin, const RGBWWPwmConfig& config)
{
    const int logicalPins[] = { redPin, greenPin, bluePin, wwPin, cwPin };
    std::vector<uint8_t> bankPins[2];
    collectActivePins(logicalPins, _channelMap, _channelBank, RGBWW_CHANNELS::NUM_CHANNELS,
                      config.secondaryChannelMask, bankPins);

    if(!bankPins[0].empty()) {
        _pPwm[0] = new Esp32HardwarePwm(bankPins[0], config.primary);
    }
    if(!bankPins[1].empty()) {
        _pPwm[1] = new Esp32HardwarePwm(bankPins[1], config.secondary);
    }
    _initialized = true;
    for(size_t i = 0; i < _bindings.size(); ++i) {
        if(_channelMap[i] == UNUSED_CHANNEL) continue;
        auto* bank = _pPwm[_channelBank[i]];
        _bindings[i] = {bank, static_cast<uint8_t>(_channelMap[i])};
        _initialized = _initialized && bank != nullptr && bank->isInitialized();
        _dutyRangeFactor[i] = bank != nullptr ? bank->getMaxDuty() / 65535.0f : 0.0f;
    }
}

PWMOutput::PWMOutput(const RGBWWPwmChannels& channels) : _bindings(channels), _borrowed(true)
{
    bool active = false;
    for(size_t i = 0; i < channels.size(); ++i) {
        const auto& binding = channels[i];
        if(binding.bank == nullptr) continue;
        if(!binding.bank->isInitialized() || binding.channel >= binding.bank->getPinCount()) {
            debug_e("PWMOutput: invalid borrowed channel %u", unsigned(i));
            return;
        }
        for(size_t j = 0; j < i; ++j) {
            if(channels[j].bank == binding.bank && channels[j].channel == binding.channel) {
                debug_e("PWMOutput: duplicate borrowed channel %u", unsigned(i));
                return;
            }
        }
        active = true;
        _dutyRangeFactor[i] = binding.bank->getMaxDuty() / 65535.0f;
    }
    if(!active) {
        debug_e("PWMOutput: no borrowed channels");
        return;
    }
    _initialized = true;
}

bool PWMOutput::hasPendingCallbacks() const
{
    if(!_initialized) return false;
    for(const auto& binding : _bindings) {
        if(binding.bank != nullptr && binding.bank->hasPendingCallbacks(binding.channel)) return true;
    }
    return false;
}

void PWMOutput::fadeChannel(int chan, int duty_16bit, uint32_t fade_ms, bool queue) {
    if(!_initialized || chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS) return;
    const auto& binding = _bindings[chan];
    if(binding.bank == nullptr) return;
    const uint32_t scaledDuty = static_cast<uint32_t>(roundf(duty_16bit * _dutyRangeFactor[chan]));
    if(!binding.bank->fadeChan(binding.channel, scaledDuty, fade_ms, queue)) {
        debug_e("PWMOutput: fade failed on channel %d", chan);
    }
}

bool PWMOutput::fadeChannelWithCycleLimit(int chan, int duty_16bit, uint32_t fade_ms, uint32_t maxCycles, bool queue) {
    if(!_initialized || chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS) return false;
    const auto& binding = _bindings[chan];
    if(binding.bank == nullptr) return false;
    const uint32_t scaledDuty = static_cast<uint32_t>(roundf(duty_16bit * _dutyRangeFactor[chan]));
    return binding.bank->fadeChanWithCycleLimit(binding.channel, scaledDuty, fade_ms, maxCycles, queue);
}

void PWMOutput::resetFadeQueue(int chan) {
    if(!_initialized || chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS) return;
    const auto& binding = _bindings[chan];
    if(binding.bank != nullptr) {
        binding.bank->resetQueue(binding.channel);
    }
}

bool PWMOutput::isFadingChannel(int chan) {
    if(!_initialized || chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS) return false;
    const auto& binding = _bindings[chan];
    return binding.bank != nullptr && binding.bank->isFadingChan(binding.channel);
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
#if defined(ARCH_ESP32)
    if(_borrowed && _initialized) {
        for(const auto& binding : _bindings) {
            if(binding.bank == nullptr) continue;
            binding.bank->resetQueue(binding.channel);
            if(!binding.bank->setDutyChan(binding.channel, 0)) {
                debug_e("PWMOutput: failed to detach channel %u", unsigned(binding.channel));
            }
        }
    }
    delete _pPwm[0];
    delete _pPwm[1];
    _pPwm[0] = nullptr;
    _pPwm[1] = nullptr;
#else
    delete _pPwm;
    _pPwm = nullptr;
#endif
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

#if defined(ARCH_ESP32)
    if(_initialized) for(const auto& binding : _bindings) {
        if(binding.bank != nullptr) binding.bank->updateChan(binding.channel);
    }
#else
    if(_pPwm != nullptr) _pPwm->update();
#endif
}
int PWMOutput::getChannel(int chan) {
    if(chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS) return 0;
#if defined(ARCH_ESP32)
    const auto& binding = _bindings[chan];
    return _initialized && binding.bank != nullptr ? binding.bank->getDutyChan(binding.channel) : 0;
#else
    if(_channelMap[chan] == UNUSED_CHANNEL) return 0;
    return _pPwm != nullptr ? _pPwm->getDutyChan(static_cast<uint8_t>(_channelMap[chan])) : 0;
#endif
}

void PWMOutput::setChannel(int chan, int duty, bool update /* = true */) {
    if(chan < 0 || chan >= RGBWW_CHANNELS::NUM_CHANNELS) return;
#if defined(ARCH_ESP32)
    const auto& binding = _bindings[chan];
    if(!_initialized || binding.bank == nullptr) return;
    if (isFadingChannel(chan)) return;
#else
    if(_channelMap[chan] == UNUSED_CHANNEL || _pPwm == nullptr) return;
#endif
#if defined(ARCH_ESP32)
    const uint32 scaledDuty = uint32(roundf(duty * _dutyRangeFactor[chan]));
    if(!binding.bank->setDutyChan(binding.channel, scaledDuty, update)) {
        debug_e("PWMOutput: duty write failed on channel %d", chan);
    }
#else
    const uint32 scaledDuty = uint32(roundf(duty * _dutyRangeFactor));
    //if (unsigned(scaledDuty) == _pPwm->getDutyChan(chan))
    //    return;
    //debug_i("setChannel: chan=%d, duty=%d (scaled %d), update=%d", chan, duty, scaledDuty, update);
    _pPwm->setDutyChan(static_cast<uint8_t>(_channelMap[chan]), scaledDuty, update);
#endif
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
