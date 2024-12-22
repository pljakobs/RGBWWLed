#pragma once

#include "RGBWWconst.h"

struct RampTimeOrSpeed {
    enum class Type {
        Speed, Time
    };

    RampTimeOrSpeed() {
    }

    RampTimeOrSpeed(double v) :
            value(v) {
    }

    RampTimeOrSpeed(double v, Type t) :
            value(v), type(t) {
    }

    double value = 0.0; // Speed: percent/degree per second
    Type type = Type::Time;
};

class AbsOrRelValue {
public:
    enum class Type {
        Raw,   // 0 - 1023
        Hue,   // -> 0 - 360
        Percent, // 0 - 100
        Ct // no check
    };

    enum class Mode {
        Absolute, Relative,
    };

    AbsOrRelValue() {
    }

    AbsOrRelValue(String value, Type type = Type::Percent) :
            _type(type) {
        float fval;
        if (value.startsWith("+") || value.startsWith("-")) {
            _mode = Mode::Relative;
            fval = value.substring(1).toFloat();
            if (value.startsWith("-"))
                fval *= -1;
        } else {
            _mode = Mode::Absolute;
            fval = value.toFloat();
        }
        setValueByType(fval);
    }

    AbsOrRelValue(const AbsOrRelValue& o) {
        _value = o.getValue();
        _mode = o.getMode();
        _type = o.getType();
    }

    /**
     * Accepts CHANNEL output range value
     */
    AbsOrRelValue(int value) {
        _value = value;
    }

    /**
     * Accepts CHANNEL output range value
     */
    AbsOrRelValue(int value, Mode mode) :
            _mode(mode) {
        _value = value;
    }

    /**
     * Accepts INPUT range, will be converted to output range value
     */
    AbsOrRelValue(int value, Type type) :
            _type(type) {
        setValueByType(value);
    }

    bool operator==(const AbsOrRelValue& obj) const {
        return (_mode == obj.getMode()) && (this->_value == obj.getValue());
    }

    operator int() const {
        return _value;
    }

    Type getType() const {
        return _type;
    }

    Mode getMode() const {
        return _mode;
    }

    int getValue() const {
        return _value;
    }

    int getFinalValue(int val) const {
        switch (_mode) {
        case Mode::Relative:
            return fixRangeLimits(_value + val);
        case Mode::Absolute:
        default:
            return _value;
        }
    }

private:
    void setValueByType(float value) {
        switch (_type) {
        case Type::Hue:
            value = (value / 360.0) * RGBWW_CALC_HUEWHEELMAX;
            break;
        case Type::Percent:
            value = (value / 100.0) * RGBWW_CALC_MAXVAL;
            break;
        default:
            break;
        }

        _value = static_cast<int>(value + 0.5f);

        if (_mode != Mode::Relative)
            _value = fixRangeLimits(_value);
    }

    int fixRangeLimits(int val) const {
        switch (_type) {
        case Type::Raw:
            val = constrain(val, 0, 1023);
            break;
        case Type::Hue:
            if (val < 0)
                val = RGBWW_CALC_HUEWHEELMAX + val;
            else
                val = val % RGBWW_CALC_HUEWHEELMAX;
            break;
        case Type::Percent:
            val = constrain(val, 0, RGBWW_CALC_MAXVAL);
            break;
        default:
            break;
        }
        return val;
    }

    AbsOrRelValue::Mode _mode = Mode::Absolute;
    Type _type = Type::Percent;
    int _value = 0;
};

template<typename T>
class Optional {
public:
    Optional() :
            _hasValue(false) {
    }

    Optional<T>(T value) :
            _value(value), _hasValue(true) {
    }

    Optional& operator=(const T& obj) {
        _value = obj;
        _hasValue = true;
        return *this;
    }

    const T& getValue() const {
        return _value;
    }

    bool operator==(const Optional<T>& obj) const {
        return (this->hasValue() == obj.hasValue()) && (this->getValue() == obj.getValue());
    }

    bool operator==(const T& obj) const {
        return this->hasValue() && (_value == obj);
    }

    bool operator!=(const T& obj) const {
        return !(*this == obj);
    }

    bool operator<(const T& obj) const {
        return this->hasValue() && (_value < obj);
    }

    bool operator>(const T& obj) const {
        return this->hasValue() && (_value > obj);
    }

    operator const T&() const {
        return _value;
    }

    void clear() {
        _hasValue;
    }

    bool hasValue() const {
        return _hasValue;
    }
private:
    T _value;
    bool _hasValue = false;
};

enum class CtrlChannel {
    None,

    Hue, Sat, Val, ColorTemp,

    Red, Green, Blue, ColdWhite, WarmWhite,
};

inline String ctrlChannelToString(CtrlChannel ch) {
    HashMap<CtrlChannel, String> strMap;
    strMap[CtrlChannel::None] = "None";
    strMap[CtrlChannel::Hue] = "h";
    strMap[CtrlChannel::Sat] = "s";
    strMap[CtrlChannel::Val] = "v";
    strMap[CtrlChannel::ColorTemp] = "ct";

    strMap[CtrlChannel::Red] = "r";
    strMap[CtrlChannel::Green] = "g";
    strMap[CtrlChannel::Blue] = "b";
    strMap[CtrlChannel::ColdWhite] = "cw";
    strMap[CtrlChannel::WarmWhite] = "ww";

    return strMap[ch];
}

struct BresenhamValues {
    int delta, error, count, step;
};

enum class QueuePolicy {
    Invalid, FrontReset, // queue to front and let the original anim run from the beginning afterwards
    Front, // queue to front and the current animation will continue where it was interrupted
    Back,
    Single,
};

