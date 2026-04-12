/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */

#pragma once
#include "RGBWWTypes.h"
#include "RGBWWLedColor.h"

class RGBWWLed;
class RGBWWLedAnimation;

/**
 * Abstract class representing the interface for animations
 *
 */
class RGBWWLedAnimation {
public:
    enum class Type {
        Undefined, SetAndStay, Transition, Blink,
    };

    RGBWWLedAnimation(RGBWWLed const * rgbled, CtrlChannel ch, Type type, bool requeue = false, const String& name = "");

    virtual ~RGBWWLedAnimation() {
    }
    ;

    /**
     * Processing method, will be called from main loop
     *
     * @return status of the animation
     * @retval true     the animation is finished
     * @retval false    the animation is not finished yet
     */
    virtual bool run() {
        return true;
    }
    ;

    virtual const char* toString() {
        return "<empty>";
    }

    /**
     * Generic interface method for changing a variable
     * representing the speed of the current active animation
     *
     * @param newspeed
     */
    virtual void setSpeed(int /* newspeed */) {
    }
    ;

    /**
     * Generic interface method for changing a variable
     * representing the brightness of the current active animation
     *
     * @param newbrightness
     */
    virtual void setBrightness(int /* newbrightness */) {
    }
    ;

    /**
     * Interface to reset the animation so it can run from the beginning
     *
     */
    virtual void reset() {
    }
    ;

    bool shouldRequeue() const {
        return _requeue;
    }

    const String& getName() const {
        return _name;
    }

    int getAnimValue() const {
        return _value;
    }

    Type getAnimType() const {
        return _type;
    }

protected:
    int getBaseValue() const;

    RGBWWLed const * _rgbled = nullptr;
    CtrlChannel _ctrlChannel = CtrlChannel::None;
    const bool _requeue = false;
    const String _name;
    int _value = 0;
    Type _type = Type::Undefined;
};

class AnimSetAndStay: public RGBWWLedAnimation {
public:
    AnimSetAndStay(const AbsOrRelValue& endVal, int onTime, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;
    virtual void reset() override;

private:
    virtual bool init();

    int _currentstep = 0;
    int _steps = 0;
    AbsOrRelValue _initEndVal;
};

class AnimTransition: public RGBWWLedAnimation {
public:
    AnimTransition(const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name =
            "");
    AnimTransition(const AbsOrRelValue& from, const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue =
            false, const String& name = "");

    virtual bool run() override;
    virtual void reset() override;

    int getFinalVal() const { return _finalval; }
    uint32_t getDurationMs() const { return static_cast<uint32_t>(_steps) * RGBWW_MINTIMEDIFF; }

protected:
    int bresenham(BresenhamValues& values, int& dx, int& base, int& current);

    virtual bool init();

    int _baseval = 0;
    int _currentval = 0;
    int _finalval = 0;
    bool _hasfromval = false;
    int _currentstep = 0;
    int _steps = 0;
    BresenhamValues _bresenham;
    AbsOrRelValue _initEndVal;
    AbsOrRelValue _initStartVal;
    RampTimeOrSpeed _ramp;
};

class AnimTransitionCircularHue: public AnimTransition {
public:
    AnimTransitionCircularHue(const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int direction, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue =
            false, const String& name = "");
    AnimTransitionCircularHue(const AbsOrRelValue& startVal, const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int direction, RGBWWLed const * rgbled,
            CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;

private:
    virtual bool init() override;

    int _direction = 0;
};

class AnimBlink: public RGBWWLedAnimation {
public:
    AnimBlink(int blinkTime, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;
    virtual void reset() override;

private:
    virtual bool init();

    int _currentstep = 0;
    int _steps = 0;

protected:
    int _prevvalue = 0;
};
