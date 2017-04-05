/*
 * RGBWWAnimatedChannel.h
 *
 *  Created on: 02.04.2017
 *      Author: Robin
 */

#pragma once

#include "RGBWWTypes.h"
#include "RGBWWconst.h"

class RGBWWLed;
class RGBWWLedAnimation;
class RGBWWLedAnimationQ;


class RGBWWAnimatedChannel {
public:
    RGBWWAnimatedChannel(RGBWWLed* rgbled);
    virtual ~RGBWWAnimatedChannel();

    bool process();

    /**
     * Set a function as callback when an animation has finished.
     *
     * Example use case would be, to save the current color to flash
     * after an animation has finished to preserve it after a powerloss
     *
     * @param func
     */
    void setAnimationCallback( void (*func)(RGBWWLed* led, RGBWWLedAnimation* anim) );


    /**
     * Check if an animation is currently active
     *
     * @retval true if an animation is currently active
     * @retval false if no animation is active
     */
    bool isAnimationActive();


    /**
     * Check if the AnimationQueue is full
     *
     * @retval true queue is full
     * @retval false queue is not full
     */
    bool isAnimationQFull();

    /**
     * skip the current animation
     *
     */
    void skipAnimation();


    /**
     * Cancel all animations in the queue
     *
     */
    void clearAnimationQueue();


    /**
     * Change the speed of the current running animation
     *
     * @param speed
     */
    void setAnimationSpeed(int speed);


    /**
     * Change the brightness of the current animation
     *
     * @param brightness
     */
    void setAnimationBrightness(int brightness);

    int getValue() const {
        return _value;
    }

    void pushAnimation(RGBWWLedAnimation* pAnim, QueuePolicy queuePolicy);

    void pauseAnimation();
    void continueAnimation();

private:
    RGBWWLed* _rgbled;
    int     _value = 0;
    bool    _cancelAnimation = false;
    bool    _clearAnimationQueue = false;
    bool    _isAnimationActive = false;
    bool    _isAnimationPaused = false;

    RGBWWLedAnimation*  _currentAnimation = nullptr;
    RGBWWLedAnimationQ* _animationQ;

    void (*_animationcallback)(RGBWWLed* led, RGBWWLedAnimation* anim) = nullptr;

    //helpers
    void cleanupCurrentAnimation();
    void cleanupAnimationQ();
    void requeueCurrentAnimation();
};