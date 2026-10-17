COMPONENT_SEARCH_DIRS += $(PROJECT_DIR)/../../..
COMPONENT_INCDIRS += $(PROJECT_DIR)/../..
COMPONENT_SRCDIRS := app
COMPONENT_DEPENDS += RGBWWLed Esp32HardwarePwm

ifneq ($(SMING_ARCH),Esp32)
$(error MultiLight sample requires SMING_ARCH=Esp32 SMING_SOC=esp32)
endif
ifneq ($(SMING_SOC),esp32)
$(error MultiLight sample requires SMING_ARCH=Esp32 SMING_SOC=esp32)
endif
