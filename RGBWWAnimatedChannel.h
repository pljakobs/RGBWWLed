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

    /**
     * @retval true animation finished
     * @retval false
     */
    bool process();

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

    int getValue() const;
    void setValue(const AbsOrRelValue& val);

    bool pushAnimation(RGBWWLedAnimation* pAnim, QueuePolicy queuePolicy);

    void pauseAnimation();
    void continueAnimation();
    void setPwmChannelIndex(int idx) { _pwmChannelIndex = idx; }

private:
    RGBWWLed* _rgbled;
    int _value = 0;
    bool _cancelAnimation = false;
    bool _clearAnimationQueue = false;
    bool _isAnimationActive = false;
    bool _isAnimationPaused = false;

    RGBWWLedAnimation* _currentAnimation = nullptr;
    RGBWWLedAnimationQ* _animationQ = nullptr;
    int _pwmChannelIndex = -1;
    bool _hwFadeActive = false;
    int _fadeTarget = 0;
    int _fadeStartDuty = 0;
    int _fadeStepsTotal = 0;
    int _fadeStepsDone = 0;
    uint32_t _fadeTotalMs = 0;
    static constexpr uint32_t FADE_STEP_MS = 500;

    //helpers
    void notifyAnimationFinished(bool requeued);
    void cleanupCurrentAnimation();
    void cleanupAnimationQ();
    void requeueCurrentAnimation();
};
