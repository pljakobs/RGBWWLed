/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *          Peter Jakobs    http://github.com/pljakobs
 *
 * All files of this project are provided under the LGPL v3 license.
 */

#pragma once

#include <SmingCore.h>
#include <pgmspace.h>
#include <stdint.h>
#include <stddef.h>

#ifdef SMING_VERSION
#define RGBWW_USE_ESP_HWPWM
#define RGBWW_PWMRESOLUTION 65536
#define RGBWW_CALC_DEPTH 10
#else
#define RGBWW_PWMRESOLUTION 1023
#define RGBWW_CALC_DEPTH 8
#endif

#define RGBWW_VERSION "0.10.0"
#define RGBWW_CALC_WIDTH int(pow(2, RGBWW_CALC_DEPTH))
#define RGBWW_CALC_MAXVAL int(RGBWW_CALC_WIDTH - 1)
#define RGBWW_CALC_HUEWHEELMAX int(RGBWW_CALC_MAXVAL * 6)

#define RGBWW_UPDATEFREQUENCY 50
#define RGBWW_MINTIMEDIFF  int(1000 / RGBWW_UPDATEFREQUENCY)
#define RGBWW_MINTIMEDIFF_US  RGBWW_MINTIMEDIFF * 1000
#ifdef ARCH_ESP8266
#define RGBWW_ANIMATIONQSIZE 20
#else
#define RGBWW_ANIMATIONQSIZE 100
#endif
#define RGBWW_WARMWHITEKELVIN 2700
#define RGBWW_COLDWHITEKELVIN 6000


// Determine Array Size and Return Type based on configurations
constexpr size_t TableSize = (RGBWW_CALC_DEPTH == 8) ? 256 : 1024;
using TableType = typename std::conditional<(RGBWW_CALC_DEPTH == 8 && RGBWW_PWMRESOLUTION == 256), uint8_t, uint16_t>::type;

// ==========================================
// 1. Core Mathematical Curve Engines
// ==========================================

// CIE 1931 Luminance Math
constexpr TableType calculate_cie_point(int index, double max_input, double max_output) {
    double x = index / max_input;
    double y = (x > 0.08) ? ((x + 0.16) / 1.16) * ((x + 0.16) / 1.16) * ((x + 0.16) / 1.16) 
                          : (x / 9.03296);
    return static_cast<TableType>((y * max_output) + 0.5);
}

// Cubic Power Math (Matches 256-step configurations)
constexpr TableType calculate_cubic_point(int index, double max_input, double max_output) {
    double normalized = index / max_input;
    double curve = normalized * normalized * normalized; 
    return static_cast<TableType>((curve * max_output) + 0.5);
}

// Router function selecting formula based on compilation parameters
constexpr TableType generate_point(int index) {
    constexpr double max_in = TableSize - 1;
    constexpr double max_out = RGBWW_PWMRESOLUTION - 1;

    if (RGBWW_CALC_DEPTH == 10 && RGBWW_PWMRESOLUTION == 65536) {
        return calculate_cie_point(index, max_in, max_out);
    } else {
        return calculate_cubic_point(index, max_in, max_out);
    }
}

// ==========================================
// 2. Compile-time Array Sequence Generator
// ==========================================
template<typename T, size_t... Is>
struct TableBuilder {
    static constexpr T data[sizeof...(Is)] = { generate_point(Is)... };
};

// Out-of-line storage definition required for C++14
template<typename T, size_t... Is>
constexpr T TableBuilder<T, Is...>::data[];

template<typename T, size_t N, typename Indices = std::make_index_sequence<N>>
struct LookupTable;

template<typename T, size_t N, size_t... Is>
struct LookupTable<T, N, std::index_sequence<Is...>> : TableBuilder<T, Is...> {};

// ==========================================
// 3. Final Unified Interface
// ==========================================
// Replaces all 4 hardcoded options with a single, auto-calculating array
constexpr auto RGBWW_dim_curve = LookupTable<TableType, TableSize>::data;

constexpr bool verify_dim_curve() {
    // Test point 1: The absolute floor must always be 0
    static_assert(RGBWW_dim_curve[0] == 0, "Error: First element must be 0");

    // Test point 2: Check milestones based on selected resolution configuration
    if constexpr (RGBWW_CALC_DEPTH == 8 && RGBWW_PWMRESOLUTION == 256) {
        static_assert(RGBWW_dim_curve[28] == 1,   "Error at index 28");
        static_assert(RGBWW_dim_curve[128] == 35, "Error at index 128");
        static_assert(RGBWW_dim_curve[255] == 255, "Error at index 255");
    } 
    else if constexpr (RGBWW_CALC_DEPTH == 10 && RGBWW_PWMRESOLUTION == 65535) {
        static_assert(RGBWW_dim_curve[1] == 7,       "Error at index 1");
        static_assert(RGBWW_dim_curve[512] == 14801, "Error at index 512");
        static_assert(RGBWW_dim_curve[1023] == 65535, "Error at index 1023");
    }

    return true; // Used to trigger evaluation
}

// Forces the compiler to execute the verification routine right now
constexpr bool dummy_verification = verify_dim_curve();