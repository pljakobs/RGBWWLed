/*
 * RGBWWAnimatedChannel.cpp
 *
 *  Created on: 02.04.2017
 *      Author: Robin
 */

#include "RGBWWAnimatedChannel.h"

#include "RGBWWLedAnimation.h"
#include "RGBWWLedAnimationQ.h"


RGBWWAnimatedChannel::RGBWWAnimatedChannel(RGBWWLed* rgbled) : _rgbled(rgbled), _animationQ(new RGBWWLedAnimationQ(RGBWW_ANIMATIONQSIZE)) {
}

RGBWWAnimatedChannel::~RGBWWAnimatedChannel() {
    delete _animationQ;
    if (_currentAnimation != NULL) {
        delete _currentAnimation;
    }
}

void RGBWWAnimatedChannel::getValue(int& value) const {
    value = _value;
}

void RGBWWAnimatedChannel::pushAnimation(RGBWWLedAnimation* pAnim, QueuePolicy queuePolicy) {
	if (queuePolicy == QueuePolicy::Single) {
		cleanupAnimationQ();
		cleanupCurrentAnimation();
	}

	Serial.printf("Adding animation: %d\n", queuePolicy);

	switch(queuePolicy) {
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
	}
}

bool RGBWWAnimatedChannel::process() {
    if (_isAnimationPaused) {
        return false;
    }

    // check if we need to cancel effect
    if (_cancelAnimation) {
        cleanupCurrentAnimation();
    }

    // cleanup Q if we cancel all effects
    if (_clearAnimationQueue) {
        cleanupAnimationQ();
    }

    // Interval has passed
    // check if we need to animate or there is any new animation
    if (!_isAnimationActive) {
        //check if animation otherwise return true
        if (_animationQ->isEmpty()) {
            return true;
        }
        _currentAnimation = _animationQ->pop();
        _isAnimationActive = true;
    }

    const bool finished = _currentAnimation->run();
    _value = _currentAnimation->getAnimValue();
    if (finished) {
        //callback animation finished
        if(_animationcallback != NULL ){
            _animationcallback(_rgbled, _currentAnimation);
        }

        if (_currentAnimation->shouldRequeue())
            requeueCurrentAnimation();
        else
            cleanupCurrentAnimation();
    }

    return false;

}

void RGBWWAnimatedChannel::pauseAnimation() {
    _isAnimationPaused = true;
}

void RGBWWAnimatedChannel::continueAnimation() {
    _isAnimationPaused = false;
}

bool RGBWWAnimatedChannel::isAnimationQFull() {
    return _animationQ->isFull();
}


bool RGBWWAnimatedChannel::isAnimationActive() {
    return _isAnimationActive;
}


void RGBWWAnimatedChannel::skipAnimation(){
    if (_isAnimationActive) {
        _cancelAnimation = true;
    }
}

void RGBWWAnimatedChannel::clearAnimationQueue() {
    _clearAnimationQueue = true;
}


void RGBWWAnimatedChannel::setAnimationCallback( void (*func)(RGBWWLed* led, RGBWWLedAnimation* anim) ) {
    _animationcallback = func;
}

void RGBWWAnimatedChannel::setAnimationSpeed(int speed) {
    if(_currentAnimation != NULL) {
        _currentAnimation->setSpeed(speed);
    }
}

void RGBWWAnimatedChannel::setAnimationBrightness(int brightness){
    if(_currentAnimation != NULL) {
        _currentAnimation->setBrightness(brightness);
    }
}

void RGBWWAnimatedChannel::cleanupCurrentAnimation() {
    if (_currentAnimation == nullptr)
        return;

    _isAnimationActive = false;
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

    Serial.printf("Requeuing...\n");

    _currentAnimation->reset();
    _animationQ->push(_currentAnimation);

    _currentAnimation = NULL;
    _isAnimationActive = false;
    _cancelAnimation = false;
}
