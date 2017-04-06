/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */
#include "RGBWWLed.h"
#include "RGBWWAnimatedChannel.h"
#include "RGBWWLedColor.h"
#include "RGBWWLedAnimation.h"
#include "RGBWWLedAnimationQ.h"
#include "RGBWWLedOutput.h"



/**************************************************************
 *                setup, init and settings
 **************************************************************/

RGBWWLed::RGBWWLed() {
	_current_color = HSVCT(0, 0, 0);
	_current_output = ChannelOutput(0, 0, 0, 0, 0);

	_pwm_output = nullptr;

	_animChannelsHsv[CtrlChannel::Hue] = new RGBWWAnimatedChannel(this);
	_animChannelsHsv[CtrlChannel::Sat] = new RGBWWAnimatedChannel(this);
	_animChannelsHsv[CtrlChannel::Val] = new RGBWWAnimatedChannel(this);
	_animChannelsHsv[CtrlChannel::ColorTemp] = new RGBWWAnimatedChannel(this);

    _animChannelsRaw[CtrlChannel::Red] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::Green] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::Blue] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::WarmWhite] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::ColdWhite] = new RGBWWAnimatedChannel(this);
}

RGBWWLed::~RGBWWLed() {
	delete _pwm_output;
	_pwm_output = nullptr;
}

void RGBWWLed::init(int redPIN, int greenPIN, int bluePIN, int wwPIN, int cwPIN, int pwmFrequency /* =200 */) {
	_pwm_output = new PWMOutput(redPIN, greenPIN, bluePIN, wwPIN, cwPIN, pwmFrequency);
}


void RGBWWLed::getAnimChannelHsvColor(HSVCT& c) {
    _animChannelsHsv[CtrlChannel::Hue]->getValue(c.hue);
    _animChannelsHsv[CtrlChannel::Sat]->getValue(c.sat);
    _animChannelsHsv[CtrlChannel::Val]->getValue(c.val);
    _animChannelsHsv[CtrlChannel::ColorTemp]->getValue(c.ct);
}

void RGBWWLed::getAnimChannelRawOutput(ChannelOutput& o) {
    _animChannelsRaw[CtrlChannel::Red]->getValue(o.r);
    _animChannelsRaw[CtrlChannel::Green]->getValue(o.g);
    _animChannelsRaw[CtrlChannel::Blue]->getValue(o.b);
    _animChannelsRaw[CtrlChannel::WarmWhite]->getValue(o.cw);
    _animChannelsRaw[CtrlChannel::ColdWhite]->getValue(o.ww);
}

/**************************************************************
 *                     OUTPUT
 **************************************************************/
bool RGBWWLed::show() {
	switch(_mode) {
	case ColorMode::Hsv:
	{
	    for(int i=0; i < _animChannelsHsv.count(); ++i) {
	    	_animChannelsHsv.valueAt(i)->process();
	    }

	    // now build the output value and set
	    HSVCT c;
	    getAnimChannelHsvColor(c);

	    debugRGBW("NEW: h:%d, s:%d, v:%d, ct: %d", c.h, c.s, c.v, c.ct);

	    if (getCurrentColor() != c) {
	    	this->setOutput(c);
	    }
		break;
	}
	case ColorMode::Raw:
	{
	    for(int i=0; i < _animChannelsRaw.count(); ++i) {
	    	_animChannelsRaw.valueAt(i)->process();
	    }

	    // now build the output value and set
	    ChannelOutput o;
	    getAnimChannelRawOutput(o);

	    if (getCurrentOutput() != o) {
	    	this->setOutput(o);
	    }
		break;
	}
	}
}

void RGBWWLed::refresh() {
	setOutput(_current_color);
}

const ChannelOutput& RGBWWLed::getCurrentOutput() const {
	return _current_output;
}


const HSVCT& RGBWWLed::getCurrentColor() const {
	return _current_color;
}

void RGBWWLed::setOutput(HSVCT& outputcolor) {
	RGBWCT rgbwk;
	_current_color = outputcolor;
	colorutils.HSVtoRGB(outputcolor, rgbwk);
	setOutput(rgbwk);

}

void RGBWWLed::setOutput(RGBWCT& outputcolor) {
	ChannelOutput output;
	colorutils.whiteBalance(outputcolor, output);
	setOutput(output);
}

void RGBWWLed::setOutput(ChannelOutput& output) {
	if(_pwm_output != NULL) {
		colorutils.correctBrightness(output);
		_current_output = output;
		debugRGBW("R:%i | G:%i | B:%i | WW:%i | CW:%i", output.r, output.g, output.b, output.ww, output.cw);
		_pwm_output->setOutput(RGBWW_dim_curve[output.r],
							   RGBWW_dim_curve[output.g],
							   RGBWW_dim_curve[output.b],
							   RGBWW_dim_curve[output.ww],
							   RGBWW_dim_curve[output.cw]);
	}
};

void RGBWWLed::setOutputRaw(int& red, int& green, int& blue, int& wwhite, int& cwhite) {
	if(_pwm_output != NULL) {
		_current_output = ChannelOutput(red, green, blue, wwhite, cwhite);
		_pwm_output->setOutput(red, green, blue, wwhite, cwhite);
	}
}

/**************************************************************
 *                 ANIMATION/TRANSITION
 **************************************************************/

void RGBWWLed::blink(int time) {
	Serial.printf("Blink\n");
	if (_mode == ColorMode::Hsv) {
		HSVCT color = getCurrentColor();
		int val = (color.val) > 50 ? 0 : 100;
		_animChannelsHsv[CtrlChannel::Val]->pushAnimation(new AnimSetAndStay(val, time, this, CtrlChannel::Val), QueuePolicy::Front);
	}
	else {
		ChannelOutput out = getCurrentOutput();
		int cwVal = (out.coldwhite) > 50 ? 0 : 100;
		int wwVal = (out.warmwhite) > 50 ? 0 : 100;
		_animChannelsRaw[CtrlChannel::WarmWhite]->pushAnimation(new AnimSetAndStay(wwVal, time, this, CtrlChannel::WarmWhite), QueuePolicy::Front);
		_animChannelsRaw[CtrlChannel::ColdWhite]->pushAnimation(new AnimSetAndStay(cwVal, time, this, CtrlChannel::ColdWhite), QueuePolicy::Front);
	}
}

//// setHSV ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::setHSV(const HSVCT& color, QueuePolicy queuePolicy, bool requeue, const String& name) {
	setHSV(color, 0, queuePolicy, requeue, name);
}

void RGBWWLed::setHSV(const HSVCT& color, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Hsv;

	auto pAnimH = new AnimSetAndStay(color.h, time, this, CtrlChannel::Hue, requeue, name);
	auto pAnimS = new AnimSetAndStay(color.s, time, this, CtrlChannel::Sat, requeue, name);
	auto pAnimV = new AnimSetAndStay(color.v, time, this, CtrlChannel::Val, requeue, name);
	auto pAnimCt = new AnimSetAndStay(color.ct, time, this, CtrlChannel::ColorTemp, requeue, name);

	_animChannelsHsv[CtrlChannel::Hue]->pushAnimation(pAnimH, queuePolicy);
	_animChannelsHsv[CtrlChannel::Sat]->pushAnimation(pAnimS, queuePolicy);
	_animChannelsHsv[CtrlChannel::Val]->pushAnimation(pAnimV, queuePolicy);
	_animChannelsHsv[CtrlChannel::ColorTemp]->pushAnimation(pAnimCt, queuePolicy);
}

//// fadeHSV ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::fadeHSV(const HSVCT& color, int ramp, int direction, bool requeue, const String& name) {
	fadeHSV( color, ramp, direction, QueuePolicy::Single, requeue, name);
}

void RGBWWLed::fadeHSV(const HSVCT& color, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
	fadeHSV( color, ramp, 1, queuePolicy, requeue, name);
}

void RGBWWLed::fadeHSV(const HSVCT& color, int ramp, int direction, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Hsv;

	RGBWWLedAnimation* pAnimH = NULL;
	RGBWWLedAnimation* pAnimS = NULL;
	RGBWWLedAnimation* pAnimV = NULL;
	RGBWWLedAnimation* pAnimCt = NULL;
	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pAnimH = new AnimSetAndStay(color.h, 0, this, CtrlChannel::Hue, requeue, name);
		pAnimS = new AnimSetAndStay(color.s, 0, this, CtrlChannel::Sat, requeue, name);
		pAnimV = new AnimSetAndStay(color.v, 0, this, CtrlChannel::Val, requeue, name);
		pAnimCt = new AnimSetAndStay(color.ct, 0, this, CtrlChannel::ColorTemp, requeue, name);
	}
	else {
		pAnimH = new AnimTransitionCircularHue(color.h, ramp, direction, this, CtrlChannel::Hue, requeue, name);
		pAnimS = new AnimTransition(color.s, ramp, this, CtrlChannel::Sat, requeue, name);
		pAnimV = new AnimTransition(color.v, ramp, this, CtrlChannel::Val, requeue, name);
		pAnimCt = new AnimTransition(color.ct, ramp, this, CtrlChannel::ColorTemp, requeue, name);
	}

	_animChannelsHsv[CtrlChannel::Hue]->pushAnimation(pAnimH, queuePolicy);
	_animChannelsHsv[CtrlChannel::Sat]->pushAnimation(pAnimS, queuePolicy);
	_animChannelsHsv[CtrlChannel::Val]->pushAnimation(pAnimV, queuePolicy);
	_animChannelsHsv[CtrlChannel::ColorTemp]->pushAnimation(pAnimCt, queuePolicy);
}

void RGBWWLed::fadeHSV(const HSVCT& colorFrom, const HSVCT& color, int ramp, int direction /* = 1 */, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Hsv;

	RGBWWLedAnimation* pAnimH = NULL;
	RGBWWLedAnimation* pAnimS = NULL;
	RGBWWLedAnimation* pAnimV = NULL;
	RGBWWLedAnimation* pAnimCt = NULL;
	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pAnimH = new AnimSetAndStay(color.h, 0, this, CtrlChannel::Hue, requeue, name);
		pAnimS = new AnimSetAndStay(color.s, 0, this, CtrlChannel::Sat, requeue, name);
		pAnimV = new AnimSetAndStay(color.v, 0, this, CtrlChannel::Val, requeue, name);
		pAnimCt = new AnimSetAndStay(color.ct, 0, this, CtrlChannel::ColorTemp, requeue, name);
	}
	else {
		pAnimH = new AnimTransitionCircularHue(colorFrom.h, color.h, ramp, direction, this, CtrlChannel::Hue, requeue, name);
		pAnimS = new AnimTransition(colorFrom.s, color.s, ramp, this, CtrlChannel::Sat, requeue, name);
		pAnimV = new AnimTransition(colorFrom.v, color.v, ramp, this, CtrlChannel::Val, requeue, name);
		pAnimCt = new AnimTransition(colorFrom.ct, color.ct, ramp, this, CtrlChannel::ColorTemp, requeue, name);
	}

	_animChannelsHsv[CtrlChannel::Hue]->pushAnimation(pAnimH, queuePolicy);
	_animChannelsHsv[CtrlChannel::Sat]->pushAnimation(pAnimS, queuePolicy);
	_animChannelsHsv[CtrlChannel::Val]->pushAnimation(pAnimV, queuePolicy);
	_animChannelsHsv[CtrlChannel::ColorTemp]->pushAnimation(pAnimCt, queuePolicy);
}

//// setRAW ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::setRAW(const ChannelOutput& output, QueuePolicy queuePolicy, bool requeue, const String& name) {
	setRAW(output, 0, queuePolicy, requeue, name);
}

void RGBWWLed::setRAW(const ChannelOutput& output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Raw;

	auto pAnimR = new AnimSetAndStay(output.r, time, this, CtrlChannel::Red, requeue, name);
	auto pAnimG = new AnimSetAndStay(output.g, time, this, CtrlChannel::Green, requeue, name);
	auto pAnimB = new AnimSetAndStay(output.b, time, this, CtrlChannel::Blue, requeue, name);
	auto pAnimCw = new AnimSetAndStay(output.cw, time, this, CtrlChannel::ColdWhite, requeue, name);
	auto pAnimWw = new AnimSetAndStay(output.ww, time, this, CtrlChannel::WarmWhite, requeue, name);

	_animChannelsRaw[CtrlChannel::Red]->pushAnimation(pAnimR, queuePolicy);
	_animChannelsRaw[CtrlChannel::Green]->pushAnimation(pAnimG, queuePolicy);
	_animChannelsRaw[CtrlChannel::Blue]->pushAnimation(pAnimB, queuePolicy);
	_animChannelsRaw[CtrlChannel::ColdWhite]->pushAnimation(pAnimCw, queuePolicy);
	_animChannelsRaw[CtrlChannel::WarmWhite]->pushAnimation(pAnimWw, queuePolicy);
}

//// fadeRAW ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::fadeRAW(const ChannelOutput& output, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Raw;

	RGBWWLedAnimation* pAnimR = NULL;
	RGBWWLedAnimation* pAnimG = NULL;
	RGBWWLedAnimation* pAnimB = NULL;
	RGBWWLedAnimation* pAnimWw = NULL;
	RGBWWLedAnimation* pAnimCw = NULL;
	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pAnimR = new AnimSetAndStay(output.r, 0, this, CtrlChannel::Red, requeue, name);
		pAnimG = new AnimSetAndStay(output.g, 0, this, CtrlChannel::Green, requeue, name);
		pAnimB = new AnimSetAndStay(output.b, 0, this, CtrlChannel::Blue, requeue, name);
		pAnimWw = new AnimSetAndStay(output.ww, 0, this, CtrlChannel::WarmWhite, requeue, name);
		pAnimCw = new AnimSetAndStay(output.cw, 0, this, CtrlChannel::ColdWhite, requeue, name);
	}
	else {
		pAnimR = new AnimTransition(output.r, ramp, this, CtrlChannel::Red, requeue, name);
		pAnimG = new AnimTransition(output.g, ramp, this, CtrlChannel::Green, requeue, name);
		pAnimB = new AnimTransition(output.b, ramp, this, CtrlChannel::Blue, requeue, name);
		pAnimWw = new AnimTransition(output.ww, ramp, this, CtrlChannel::WarmWhite, requeue, name);
		pAnimCw = new AnimTransition(output.cw, ramp, this, CtrlChannel::ColdWhite, requeue, name);
	}

	_animChannelsHsv[CtrlChannel::Red]->pushAnimation(pAnimR, queuePolicy);
	_animChannelsHsv[CtrlChannel::Green]->pushAnimation(pAnimG, queuePolicy);
	_animChannelsHsv[CtrlChannel::Blue]->pushAnimation(pAnimB, queuePolicy);
	_animChannelsHsv[CtrlChannel::WarmWhite]->pushAnimation(pAnimWw, queuePolicy);
	_animChannelsHsv[CtrlChannel::ColdWhite]->pushAnimation(pAnimCw, queuePolicy);
}

void RGBWWLed::fadeRAW(const ChannelOutput& output_from, const ChannelOutput& output, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Raw;

	RGBWWLedAnimation* pAnimR = NULL;
	RGBWWLedAnimation* pAnimG = NULL;
	RGBWWLedAnimation* pAnimB = NULL;
	RGBWWLedAnimation* pAnimWw = NULL;
	RGBWWLedAnimation* pAnimCw = NULL;
	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pAnimR = new AnimSetAndStay(output.r, 0, this, CtrlChannel::Red, requeue, name);
		pAnimG = new AnimSetAndStay(output.g, 0, this, CtrlChannel::Green, requeue, name);
		pAnimB = new AnimSetAndStay(output.b, 0, this, CtrlChannel::Blue, requeue, name);
		pAnimWw = new AnimSetAndStay(output.ww, 0, this, CtrlChannel::WarmWhite, requeue, name);
		pAnimCw = new AnimSetAndStay(output.cw, 0, this, CtrlChannel::ColdWhite, requeue, name);
	}
	else {
		pAnimR = new AnimTransition(output_from.r, output.r, ramp, this, CtrlChannel::Red, requeue, name);
		pAnimG = new AnimTransition(output_from.g, output.g, ramp, this, CtrlChannel::Green, requeue, name);
		pAnimB = new AnimTransition(output_from.b, output.b, ramp, this, CtrlChannel::Blue, requeue, name);
		pAnimWw = new AnimTransition(output_from.ww, output.ww, ramp, this, CtrlChannel::WarmWhite, requeue, name);
		pAnimCw = new AnimTransition(output_from.cw, output.cw, ramp, this, CtrlChannel::ColdWhite, requeue, name);
	}

	_animChannelsHsv[CtrlChannel::Red]->pushAnimation(pAnimR, queuePolicy);
	_animChannelsHsv[CtrlChannel::Green]->pushAnimation(pAnimG, queuePolicy);
	_animChannelsHsv[CtrlChannel::Blue]->pushAnimation(pAnimB, queuePolicy);
	_animChannelsHsv[CtrlChannel::WarmWhite]->pushAnimation(pAnimWw, queuePolicy);
	_animChannelsHsv[CtrlChannel::ColdWhite]->pushAnimation(pAnimCw, queuePolicy);
}

void RGBWWLed::pushAnimationHsv_AnimSetAndStay(int val, int time, bool requeue, const String& name, const ChannelList& channels) {
	const bool all = channels.size() == 0;
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		const CtrlChannel ch = _animChannelsHsv.keyAt(i);
		if (!all && !channels.contains(ch))
			continue;
		_animChannelsHsv[i]->pushAnimation(new AnimSetAndStay(val, 0, this, ch, requeue, name));
	}
}

void RGBWWLed::clearAnimationQueue(const ChannelList& channels) {
	callForChannels(&RGBWWAnimatedChannel::clearAnimationQueue);
}

void RGBWWLed::skipAnimation(const ChannelList& channels) {
	callForChannels(&RGBWWAnimatedChannel::skipAnimation);
}

void RGBWWLed::pauseAnimation(const ChannelList& channels) {
	callForChannels(&RGBWWAnimatedChannel::pauseAnimation);
}

void RGBWWLed::continueAnimation(const ChannelList& channels) {
	callForChannels(&RGBWWAnimatedChannel::continueAnimation);
}

void RGBWWLed::callForChannels(void (RGBWWAnimatedChannel::*fnc)(), const ChannelList& channels) {
	const bool all = channels.size() == 0;

	RGBWWAnimatedChannel* modes[] = { _animChannelsHsv, _animChannelsRaw };
	for(int j=0; i < 2; ++j) {
		for(int i=0; i < modes[j].count(); ++i) {
			if (!all || !channels.contains(modes[j].keyAt(i)))
				continue;
			(modes[j].valueAt(i)->*fnc)();
		}
	}
}

void RGBWWLed::setAnimationCallback( void (*func)(RGBWWLed* led, RGBWWLedAnimation* anim) ) {
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		_animChannelsHsv.valueAt(i)->setAnimationCallback(func);
	}
	for(int i=0; i < _animChannelsRaw.count(); ++i) {
		_animChannelsRaw.valueAt(i)->setAnimationCallback(func);
	}
}
