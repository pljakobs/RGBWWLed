/**
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 * @version 0.7.3
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 * This Library provides Methods for controlling LED RGB, WarmWhite and
 * ColdWhite Led(stripes) via PWM. The Library can either use the standard
 * ARDUINO PWM Implementation or if available the espressif ESP8266 SDK
 * Pwm.
 *
 * The Library can handle different color modes (RGB, RGB+WarmWhite,
 * RGB+ColdWhite, RGB+WarmWhite+ColdWhite) and offeres different calculation
 * models for outputting HSV colors
 *
 * The Library offers various adjustments (i.e. maximum brightness of each
 * channel, adjusting HSV basecolors etc..)
 *
 * It also provides a simple interface for creating Animations and
 * queuing several animations/colors to a complex animation
 */

#pragma once

#include <Arduino.h>
#include <Wiring/WHashMap.h>
#include "RGBWWTypes.h"
#include "RGBWWconst.h"
#include "RGBWWLedColor.h"
#include "RGBWWLedAnimation.h"
#include "RGBWWLedOutput.h"

class RGBWWLedAnimation;
class RGBWWLedAnimationQ;
class RGBWWColorUtils;
class PWMOutput;
class RGBWWAnimatedChannel;

/**
 *
 */
class RGBWWLed {
public:
    enum class ColorMode {
        Hsv, Raw,
    };

    RGBWWLed();
    ~RGBWWLed();

    typedef Vector<CtrlChannel> ChannelList;

    /**
     * Initialize the the LED Controller
     *
     * @param redPIN	int representing the MC pin for the red channel
     * @param greenPIN  int representing the MC pin for the green channel
     * @param bluePIN   int representing the MC pin for the blue channel
     * @param wwPIN     int representing the MC pin for the warm white channel
     * @param cwPIN		int representing the MC pin for the cold white channel
     * @param pwmFrequency (default 200)
     */
    void init(int redPIN, int greenPIN, int bluePIN, int wwPIN, int cwPIN, int pwmFrequency = 200);
    #ifdef ARCH_ESP32
    void init(int redPIN, int greenPIN, int bluePIN, int wwPIN, int cwPIN,  const Esp32HwPwmConfig& config);
    #endif
    /**
     * Main function for processing animations/color output
     * Use this in your loop()
     *
     *
     * @retval true 	not updating
     * @retval false 	updates applied
     */
    bool show();

    /**
     * Refreshs the current output.
     * Usefull when changing brightness, white or color correction
     *
     */
    void refresh();

    /**
     * Set Output to given HSVK color.
     * Converts HSVK into seperate color channels (r,g,b,w)
     * and applies brightness and white correction
     *
     * @param HSVK&	outputcolor
     */
    void setOutput(HSVCT& color);

    /**
     * Sets the output of the Controller to the given RGBWK
     * while applying brightness and white correction
     *
     * @param RGBWK& outputcolor
     */
    void setOutput(RGBWCT& color);

    /**
     * Sets the output of the controller to the specified
     * channel values (with internal brightness correction)
     */
    void setOutput(ChannelOutput& output);

    /**
     * Directly set the PWM values without color correction or white balance.
     * Assumes the values are in the range of [0, PWMMAXVAL].
     *
     * @param int&	red
     * @param int&	green
     * @param int&	blue
     * @param int&	wwhite
     * @param int&	cwhite
     */
    void setOutputRaw(int& red, int& green, int& blue, int& cwhite, int& wwhite);

    /**
     * Returns an HSVK object representing the current color
     *
     * @return HSVK current color
     */
    const HSVCT& getCurrentColor() const;

    /**
     * Returns the current values for each channel
     *
     * #return ChannelOutput current value of all channels
     */
    const ChannelOutput& getCurrentOutput() const;

    /**
     * 	Output specified color
     *  (until a new color is set)
     *
     * @param color
     * @param queue
     */
    bool setHSV(const RequestHSVCT& color, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");

    /**
     * Output color for time x
     * if time x has passed it will continue with the next
     * color/transition in the animat queue
     *
     * @param color
     * @param time
     * @param queue
     */
    bool setHSV(const RequestHSVCT& color, int time, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");

    /**
     * Fade to specified HSVK color
     *
     * @param color 	new color
     * @param time		duration of transition in ms
     * @param direction direction of transition (0= long/ 1=short)
     */
    bool fadeHSV(const RequestHSVCT& color, const RampTimeOrSpeed& change, int direction, bool requeue = false, const String& name = "");

    /**
     * Fade to specified HSVK color
     *
     * @param color 	new color
     * @param time		duration of transition in ms
     * @param queue		directly execute fade or queue it
     */
    bool fadeHSV(const RequestHSVCT& color, const RampTimeOrSpeed& change, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false,
            const String& name = "");

    /**
     * Fade to specified HSVK color
     *
     * @param color 	new color
     * @param time		duration of transition in ms
     * @param direction direction of transition (0= long/ 1=short)
     * @param queue		directly execute fade or queue it
     */
    bool fadeHSV(const RequestHSVCT& color, const RampTimeOrSpeed& change, int direction = 1, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue =
            false, const String& name = "");

    /**
     * Fade from one color (colorFrom) to another color
     *
     * @param colorFrom starting color of transition
     * @param color 	ending color of transition
     * @param time		duration of transition in ms
     * @param direction direction of transition (0= long/ 1=short)
     * @param queue		directly execute fade or queue it
     */
    bool fadeHSV(const RequestHSVCT& colorFrom, const RequestHSVCT& color, const RampTimeOrSpeed& change, int direction = 1, QueuePolicy queuePolicy =
            QueuePolicy::Single, bool requeue = false, const String& name = "");

    //TODO: add documentation
    /**
     *
     * @param output
     */
    bool setRAW(const RequestChannelOutput& output, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");

    //TODO: add documentation
    /**
     *
     * @param output
     * @param time
     * @param queue
     */
    bool setRAW(const RequestChannelOutput& output, int time, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");

    //TODO: add documentation
    /**
     *
     * @param output
     * @param time
     * @param queue
     */
    bool fadeRAW(const RequestChannelOutput& output, const RampTimeOrSpeed& change, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false,
            const String& name = "");

    //TODO: add documentation
    /**
     *
     * @param output_from
     * @param output
     * @param time
     * @param queue
     */
    bool fadeRAW(const RequestChannelOutput& output_from, const RequestChannelOutput& output, const RampTimeOrSpeed& change, QueuePolicy queuePolicy =
            QueuePolicy::Single, bool requeue = false, const String& name = "");

    void colorDirectHSV(const RequestHSVCT& output);
    void colorDirectRAW(const RequestChannelOutput& output);

    void blink(const ChannelList& channels = ChannelList(), int time = 100, QueuePolicy queuePolicy = QueuePolicy::Front, bool requeue = false,
            const String& name = "");

    //colorutils
    RGBWWColorUtils colorutils;

    void pauseAnimation(const ChannelList& channels = ChannelList());
    void continueAnimation(const ChannelList& channels = ChannelList());

    void clearAnimationQueue(const ChannelList& channels = ChannelList());
    void skipAnimation(const ChannelList& channels = ChannelList());

    virtual void onAnimationFinished(const String& name, bool requeued);

    ColorMode getMode() const {
        return _mode;
    }

    #if RGBWW_PWMRESOLUTION == 256
    // use bytes
        int getDimCurveValue(int val) {
            if (val < 0||val>=(pow(2,RGBWW_CALC_DEPTH)))
                return 0;
            return pgm_read_byte(&RGBWW_dim_curve[val]);
        }
    #endif
    #if RGBWW_PWMRESOLUTION == 65536
        // use words
        int getDimCurveValue(int val) {
            if (val < 0 || val>=(pow(2,RGBWW_CALC_DEPTH)))
                return 0;
            return pgm_read_word(&RGBWW_dim_curve[val]);
        }
    #endif
private:
    typedef HashMap<CtrlChannel, RGBWWAnimatedChannel*> ChannelGroup;

    bool pushAnimSetAndStay(const Optional<AbsOrRelValue>& val, int time, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name);
    bool pushAnimTransition(const Optional<AbsOrRelValue>& val, const RampTimeOrSpeed& ramp, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue,
            const String& name);
    bool pushAnimTransition(const AbsOrRelValue& from, const Optional<AbsOrRelValue>& val, const RampTimeOrSpeed& ramp, QueuePolicy queuePolicy, CtrlChannel ch,
            bool requeue, const String& name);
    bool pushAnimTransitionCircularHue(const Optional<AbsOrRelValue>& val, const RampTimeOrSpeed& ramp, int direction, QueuePolicy queuePolicy, CtrlChannel ch,
            bool requeue, const String& name);
    bool pushAnimTransitionCircularHue(const AbsOrRelValue& from, const Optional<AbsOrRelValue>& val, const RampTimeOrSpeed& ramp, int direction,
            QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name);

    /**
     * Dispatches an animation to the apropriate channel group. Will not dispatch if ch is not part of the channel list
     */
    bool dispatchAnimation(RGBWWLedAnimation* pAnim, CtrlChannel ch, QueuePolicy queuePolicy, const ChannelList& channels = ChannelList());

    bool processChannelGroup(const ChannelGroup& cg);
    void getAnimChannelHsvColor(HSVCT& c);
    void getAnimChannelRawOutput(ChannelOutput& o);
    void callForChannels(const ChannelGroup& group, void (RGBWWAnimatedChannel::*fnc)(), const ChannelList& channels = ChannelList());

    ChannelOutput _current_output;
    HSVCT _current_color;

    PWMOutput* _pwm_output;

    ChannelGroup _animChannelsHsv;
    ChannelGroup _animChannelsRaw;

protected:
    ColorMode _mode = ColorMode::Hsv;
};
