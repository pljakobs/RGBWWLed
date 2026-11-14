/**
 * @author  Peter Jakobs http://github.com/pljakobs
 */

/*
 * RGBWWAnimatedChannel.cpp
 *
 *  Created on: 02.04.2017
 *      Author: Robin
 */

#include "RGBWWAnimatedChannel.h"

#include "RGBWWLed.h"
#include "RGBWWLedAnimation.h"
#include "RGBWWLedAnimationQ.h"
#include "RGBWWLedOutput.h"

#include <algorithm>

RGBWWAnimatedChannel::RGBWWAnimatedChannel(RGBWWLed* rgbled) :
        _rgbled(rgbled), _animationQ(new RGBWWLedAnimationQ(RGBWW_ANIMATIONQSIZE)) {
}

RGBWWAnimatedChannel::~RGBWWAnimatedChannel() {
    delete _animationQ;
    if (_currentAnimation != NULL) {
        delete _currentAnimation;
    }
}

int RGBWWAnimatedChannel::getValue() const {
    return _value;
}

void RGBWWAnimatedChannel::setValue(const AbsOrRelValue& val) {
    _value = val.getFinalValue(_value);
}

// Takes ownership of pAnim: it is deleted if it cannot be queued.
bool RGBWWAnimatedChannel::pushAnimation(RGBWWLedAnimation* pAnim, QueuePolicy queuePolicy) {
    if (pAnim == nullptr) {
        debug_e("RGBWWAnimatedChannel::pushAnimation: animation allocation failed");
        return false;
    }

    // do not blink while blinking
    if (_currentAnimation != nullptr && (queuePolicy == QueuePolicy::Single || queuePolicy == QueuePolicy::Front || queuePolicy == QueuePolicy::FrontReset)
            && (pAnim->getAnimType() == RGBWWLedAnimation::Type::Blink && _currentAnimation->getAnimType() == RGBWWLedAnimation::Type::Blink)) {
        debug_w("Ignored blink commmand cause already blink running!");
        delete pAnim;
        return false;
    }

    if (queuePolicy == QueuePolicy::Single) {
        cleanupAnimationQ();
        cleanupCurrentAnimation();
    }

    if (queuePolicy != QueuePolicy::Back)
        continueAnimation();

    if (_animationQ->isFull()) {
        delete pAnim;
        return false;
    }

#ifdef ARCH_ESP32
    if(queuePolicy != QueuePolicy::Back || (!_isAnimationActive && _animationQ->isEmpty())) {
        _notBeforeUs = esp_timer_get_time();
    }
#endif
    switch (queuePolicy) {
    case QueuePolicy::Back:
    case QueuePolicy::Single:
        _animationQ->push(pAnim);
        break;
    case QueuePolicy::Front:
    case QueuePolicy::FrontReset:
        if (_currentAnimation != nullptr) {
            if (queuePolicy == QueuePolicy::FrontReset)
                _currentAnimation->reset();
            _animationQ->pushFront(_currentAnimation);
            _currentAnimation = NULL;
        }
        _animationQ->pushFront(pAnim);
        _isAnimationActive = false;
        _cancelAnimation = false;
        break;
    default:
        debug_w(
                "RGBWWAnimatedChannel::pushAnimation: Unknown queue policy: %d\n",
                queuePolicy);
        delete pAnim;
        return false;
    }

    return true;
}

void RGBWWAnimatedChannel::notifyAnimationFinished(bool requeued) {
    _rgbled->animationFinished(_currentAnimation->getName(), requeued);
}

bool RGBWWAnimatedChannel::process(uint32_t steps, uint32_t intervalUs, int64_t frameEndUs) {
    _transitionFrame = false;
    if (_isAnimationPaused) {
        return false;
    }

#ifdef ARCH_ESP32
    if(_notBeforeUs != 0 && frameEndUs != 0 && intervalUs != 0) {
        const uint32_t available = std::max<int64_t>(1, (frameEndUs - _notBeforeUs) / intervalUs);
        steps = std::min(steps, available);
        _notBeforeUs = 0;
    }
    if (_hwFadeActive) {
        PWMOutput* pwm = _rgbled->getPwmOutput();
        if (_cancelAnimation && !_fadeCancelPending) {
            if (pwm != nullptr) pwm->resetFadeQueue(_pwmChannelIndex);
            _fadeCancelPending = true;
            _cancelAnimation = false;
        }
        if (pwm == nullptr || !pwm->isFadingChannel(_pwmChannelIndex)) {
            if (_fadeCancelPending) {
                cleanupCurrentAnimation();
                return true;
            }
            _hwFadeActive = false;
            _value = _fadeTarget;
            if (_currentAnimation->shouldRequeue()) {
                notifyAnimationFinished(true);
                requeueCurrentAnimation();
            } else {
                cleanupCurrentAnimation();
            }
            return true;
        }
        const uint32_t elapsed = std::min<uint32_t>(millis() - _fadeStartMs, _fadeTotalMs);
        _value = _fadeStartDuty + int(int64_t(_fadeTarget - _fadeStartDuty) * elapsed / _fadeTotalMs);
        return false;
    }
#endif

    // check if we need to cancel effect
    if (_cancelAnimation) {
        cleanupCurrentAnimation();
    }

    // cleanup Q if we cancel all effects
    if (_clearAnimationQueue) {
        cleanupAnimationQ();
    }

    bool anyFinished = false;
    unsigned boundaries = 0;
    while(steps != 0) {
        if (!_isAnimationActive) {
            if (_animationQ->isEmpty()) break;
            _currentAnimation = _animationQ->pop();
            _currentAnimation->setBaseValue(_value);
            _isAnimationActive = true;
        }
        _transitionFrame = _currentAnimation->getAnimType() == RGBWWLedAnimation::Type::Transition;
        const bool finished = _currentAnimation->advance(steps);
        _value = _currentAnimation->getAnimValue();
#ifdef ARCH_ESP32
        if(!finished && !_rgbled->isServiceRunning() && !_hwFadeFailed && _pwmChannelIndex >= 0 &&
           _currentAnimation->getAnimType() == RGBWWLedAnimation::Type::Transition) {
            auto* transition = static_cast<AnimTransition*>(_currentAnimation);
            auto* pwm = _rgbled->getPwmOutput();
            if(pwm != nullptr && transition->getDurationMs() != 0) {
                _fadeTarget = transition->getFinalVal();
                _fadeStartDuty = _value;
                _fadeTotalMs = transition->getDurationMs();
                _fadeStartMs = millis();
                _fadeCancelPending = false;
                _hwFadeActive = pwm->fadeChannelWithCycleLimit(_pwmChannelIndex,
                    _rgbled->getPwmDutyForChannel(_pwmChannelIndex, _fadeTarget), _fadeTotalMs, PWM_MAX_FADE_CYCLES);
                if(!_hwFadeActive) {
                    _hwFadeFailed = true;
                    debug_e("RGBWWAnimatedChannel: failed to enqueue cycle-limited hardware fade");
                }
                return anyFinished;
            }
        }
#endif
        if(!finished) break;
        anyFinished = true;
        if (_currentAnimation->shouldRequeue()) {
            notifyAnimationFinished(true);
            requeueCurrentAnimation();
        } else
            cleanupCurrentAnimation();
        if(++boundaries >= RGBWW_ANIMATIONQSIZE + 1 && steps != 0) {
            debug_w("RGBWWLed: catch-up crossed too many queue entries; dropping %u stale steps", unsigned(steps));
            break;
        }
    }
    return anyFinished;
}

void RGBWWAnimatedChannel::pauseAnimation() {
    _isAnimationPaused = true;
}

void RGBWWAnimatedChannel::continueAnimation() {
#ifdef ARCH_ESP32
    if(_isAnimationPaused) _notBeforeUs = esp_timer_get_time();
#endif
    _isAnimationPaused = false;
}

bool RGBWWAnimatedChannel::isAnimationQFull() {
    return _animationQ->isFull();
}

bool RGBWWAnimatedChannel::isAnimationActive() {
    return _isAnimationActive;
}

void RGBWWAnimatedChannel::skipAnimation() {
    if (_isAnimationActive) {
        _cancelAnimation = true;
    }
}

void RGBWWAnimatedChannel::clearAnimationQueue() {
    _clearAnimationQueue = true;
}

void RGBWWAnimatedChannel::setAnimationSpeed(int speed) {
    if (_currentAnimation != NULL) {
        _currentAnimation->setSpeed(speed);
    }
}

void RGBWWAnimatedChannel::setAnimationBrightness(int brightness) {
    if (_currentAnimation != NULL) {
        _currentAnimation->setBrightness(brightness);
    }
}

void RGBWWAnimatedChannel::cleanupCurrentAnimation() {
    if (_currentAnimation == nullptr)
        return;

    notifyAnimationFinished(false);

    _isAnimationActive = false;
#ifdef ARCH_ESP32
    _hwFadeActive = false;
    _fadeCancelPending = false;
    _hwFadeFailed = false;
#endif
    delete _currentAnimation;
    _currentAnimation = NULL;
    _cancelAnimation = false;
}

void RGBWWAnimatedChannel::cleanupAnimationQ() {
    _animationQ->clear();
    _clearAnimationQueue = false;
}

void RGBWWAnimatedChannel::requeueCurrentAnimation() {
    if (_currentAnimation == nullptr)
        return;

    debug_d("Requeuing...\n");

    _currentAnimation->reset();
    _animationQ->push(_currentAnimation);

    _currentAnimation = NULL;
    _isAnimationActive = false;
    _cancelAnimation = false;
}
