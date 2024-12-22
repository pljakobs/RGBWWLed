/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */

#pragma once

class RGBWWLedAnimation;

/**
 * A simple queue implementation
 *
 */
class RGBWWLedAnimationQ {
public:
    RGBWWLedAnimationQ(int qsize);
    ~RGBWWLedAnimationQ();

    /**
     * Check if the queue is empty or not
     *
     * @return	bool
     * @retval	true	queue is empty
     * @retval	false	queue is not empty
     */
    bool isEmpty();

    /**
     * Check if the queue is full
     *
     * @return	bool
     * @retval	true	queue is full
     * @retval	false	queue is not full
     */
    bool isFull();

    /**
     * Add an animation to the queue
     *
     * @param 	animation
     * @return	if the object was inserted successfully
     * @retval 	true 	successfully inserted object
     * @retval	false	failed to insert objec
     */
    bool push(RGBWWLedAnimation* animation);

    bool pushFront(RGBWWLedAnimation* animation);

    /**
     * Empty queue and delete all objects stored
     */
    void clear();

    /**
     * Returns first animation object pointer but keeps it in the queue
     *
     * @return RGBWWLedAnimation*
     */
    RGBWWLedAnimation* peek();

    /**
     *	Returns first animation object pointer and removes it from queue
     *
     * @return RGBWWLedAnimation*
     */
    RGBWWLedAnimation* pop();

private:
    int _size, _count, _front, _back;
    RGBWWLedAnimation** _q;

};

