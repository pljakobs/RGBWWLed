#include "../../RGBWWLedOutput.h"
#include <algorithm>
#include <array>
#define private public
#include <FadeWorker.h>
#undef private
#include <iostream>
#include <memory>

static void drain()
{
	for(int i = 0; i < 100; ++i) {
		FadeWorker::service();
		if(Mock::callbacks.empty()) return;
		Mock::dispatch();
	}
	assert(false);
}

static void testFullBoard()
{
	Esp32HardwarePwm::Config config;
	std::vector<uint8_t> firstPins{27, 26, 25, 23, 22, 21, 19, 18};
	std::vector<uint8_t> secondPins{17, 14, 13, 12, 5, 4, 2};
	Esp32HardwarePwm first(firstPins, config);
	config.timer.speed_mode = 1;
	Esp32HardwarePwm second(secondPins, config);
	assert(FadeWorker::instanceCount() == 2 && Mock::tasks == 1);
	for(uint8_t i = 0; i < first.getPinCount(); ++i) assert(first.getDutyChan(i) == 0);
	for(uint8_t i = 0; i < second.getPinCount(); ++i) assert(second.getDutyChan(i) == 0);
	std::array<std::unique_ptr<PWMOutput>, 15> lights;
	for(size_t i = 0; i < lights.size(); ++i) {
		RGBWWPwmChannels bindings{};
		bindings[RGBWW_CHANNELS::WW] = {i < 8 ? &first : &second, uint8_t(i % 8)};
		lights[i] = std::make_unique<PWMOutput>(bindings);
		assert(lights[i]->isInitialized());
		lights[i]->setWarmWhite(32768);
		assert(FadeWorker::instanceCount() == 2);
	}
	assert(lights[1]->fadeChannelWithCycleLimit(RGBWW_CHANNELS::WW, 65535, 100, 1023));
	lights[0].reset();
	lights[8].reset();
	assert(first.getDutyChan(0) == 0 && second.getDutyChan(0) == 0);
	assert(lights[1]->isFadingChannel(RGBWW_CHANNELS::WW));
	assert(second.getDutyChan(1) != 0 && FadeWorker::instanceCount() == 2);
	for(auto& light : lights) light.reset();
	// Even with no logical lights, all board banks remain alive at zero.
	assert(FadeWorker::instanceCount() == 2);
	for(uint8_t i = 0; i < first.getPinCount(); ++i) assert(first.getDutyChan(i) == 0);
	for(uint8_t i = 0; i < second.getPinCount(); ++i) assert(second.getDutyChan(i) == 0);
}

int main()
{
	testFullBoard();
	drain();
	Esp32HardwarePwm::Config config;
	config.timer.resolution = LEDC_TIMER_12_BIT;
	std::vector<uint8_t> pins{27, 26, 25, 23, 22, 21, 19, 18};
	Esp32HardwarePwm bank(pins, config);
	auto secondaryConfig = config;
	secondaryConfig.timer.speed_mode = 1;
	secondaryConfig.timer.resolution = LEDC_TIMER_10_BIT;
	std::vector<uint8_t> secondaryPins{17, 16};
	Esp32HardwarePwm secondary(secondaryPins, secondaryConfig);
	assert(bank.isInitialized() && secondary.isInitialized());
	assert(FadeWorker::instanceCount() == 2);

	RGBWWPwmChannels bindings{};
	bindings[RGBWW_CHANNELS::WW] = {&bank, 6};
	PWMOutput existing(bindings);
	assert(existing.isInitialized());
	existing.setWarmWhite(32768);
	assert(bank.getDutyChan(6) == 2048);
	assert(bank.fadeChan(6, 3000, 10));
	const auto starts = Mock::starts;
	{
		RGBWWPwmChannels split{};
		split[RGBWW_CHANNELS::RED] = {&bank, 7};
		split[RGBWW_CHANNELS::CW] = {&secondary, 0};
		PWMOutput view(split);
		assert(view.isInitialized() && FadeWorker::instanceCount() == 2);
		// Noncontiguous bank-local indices and per-bank duty scaling are preserved.
		view.setOutput(65535, 0, 0, 0, 32768);
		assert(bank.getDutyChan(7) == 4095);
		assert(secondary.getDutyChan(0) == 512);
		assert(Mock::starts == starts && existing.isFadingChannel(RGBWW_CHANNELS::WW));
		// A light's flush cannot latch another light's staged duty.
		assert(secondary.setDutyChan(1, 100, false));
		const auto updates = Mock::dutyUpdates;
		view.setOutput(65535, 0, 0, 0, 32768);
		assert(Mock::dutyUpdates == updates);
		assert(secondary.updateChan(1));
		assert(Mock::dutyUpdates == updates + 1);
		assert(view.fadeChannelWithCycleLimit(RGBWW_CHANNELS::RED, 0, 100, 1023));
		Mock::complete(0, 6);
		assert(existing.hasPendingCallbacks() && !view.hasPendingCallbacks());
		drain();
	}
	assert(FadeWorker::instanceCount() == 2 && bank.isInitialized());
	assert(bank.getDutyChan(7) == 0 && secondary.getDutyChan(0) == 0);
	assert(secondary.getDutyChan(1) == 100 && bank.getDutyChan(6) == 3000);
	RGBWWPwmChannels reassigned{};
	reassigned[RGBWW_CHANNELS::GREEN] = {&bank, 7};
	{
		PWMOutput replacement(reassigned);
		replacement.setGreen(65535);
		drain();
		assert(bank.getDutyChan(7) == 4095 && bank.getDutyChan(6) == 3000);
	}
	auto bad = bindings;
	bad[RGBWW_CHANNELS::RED] = bad[RGBWW_CHANNELS::WW];
	{
		PWMOutput duplicate(bad);
		assert(!duplicate.isInitialized());
	}
	assert(bank.getDutyChan(6) == 3000);
	bad = {};
	bad[RGBWW_CHANNELS::RED] = {&bank, 8};
	PWMOutput invalid(bad);
	assert(!invalid.isInitialized());
	PWMOutput empty(RGBWWPwmChannels{});
	assert(!empty.isInitialized());
	// Retained pin/config constructors still own and release compatibility banks.
	const auto count = FadeWorker::instanceCount();
	{
		auto ownedConfig = config;
		ownedConfig.channelStart = 2;
		ownedConfig.timer.speed_mode = 1;
		ownedConfig.timer.timer_num = 1;
		PWMOutput legacy(4, -1, -1, -1, 5, ownedConfig);
		assert(legacy.isInitialized() && FadeWorker::instanceCount() == count + 1);
		legacy.setOutput(65535, 0, 0, 0, 32768);
		assert(legacy.getRed() == 4095 && legacy.getColdWhite() == 2048);
	}
	assert(FadeWorker::instanceCount() == count);
	std::cout << "borrowed PWM mapping, isolation, lifetime and compatibility tests passed\n";
}
