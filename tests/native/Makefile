SMING_HOME ?= /opt/sming/Sming
HWPWM_DIR ?= $(SMING_HOME)/Libraries/Esp32HardwarePwm
CXX ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -pthread -g
CPPFLAGS := -DARCH_ESP32 -DSMING_VERSION=1 -Istubs -I$(HWPWM_DIR)/tests/native/stubs -I$(HWPWM_DIR)/src/include -I$(HWPWM_DIR)/src
SOURCES := test_output.cpp ../../RGBWWLedOutput.cpp $(HWPWM_DIR)/src/Esp32HardwarePwm.cpp $(HWPWM_DIR)/src/FadeWorker.cpp
SERVICE_SOURCES := test_service.cpp ../../RGBWWLed.cpp ../../RGBWWLedService.cpp ../../RGBWWAnimatedChannel.cpp \
	../../RGBWWLedAnimation.cpp ../../RGBWWLedAnimationQ.cpp ../../RGBWWLedColor.cpp $(filter-out test_output.cpp,$(SOURCES))
SERVICE_HEADERS := $(wildcard ../../*.h)

.PHONY: test clean
test: test_output test_output_classic test_service test_service_classic
	./test_output
	./test_output_classic
	./test_service
	./test_service_classic

test_output: $(SOURCES) ../../RGBWWLedOutput.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@

test_output_classic: $(SOURCES) ../../RGBWWLedOutput.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DSOC_LEDC_SUPPORT_FADE_STOP=0 $(SOURCES) -o $@

test_service: $(SERVICE_SOURCES) $(SERVICE_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SERVICE_SOURCES) -o $@

test_service_classic: $(SERVICE_SOURCES) $(SERVICE_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DSOC_LEDC_SUPPORT_FADE_STOP=0 -DFADE_TIMING_DIAGNOSTICS=1 $(SERVICE_SOURCES) -o $@

clean:
	rm -f test_output test_output_classic test_service test_service_classic
