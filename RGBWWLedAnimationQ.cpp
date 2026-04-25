/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite strips
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */

#include "RGBWWLedAnimationQ.h"
#include "RGBWWLedAnimation.h"

RGBWWLedAnimationQ::RGBWWLedAnimationQ(int qsize) {
    _size = qsize;
    _count = 0;
    _front = 0;
    _back = 0;
    _q = new RGBWWLedAnimation*[qsize];
}

RGBWWLedAnimationQ::~RGBWWLedAnimationQ() {
    clear();
    delete[] _q;
}

bool RGBWWLedAnimationQ::isEmpty() {
    return _count == 0;
}

bool RGBWWLedAnimationQ::isFull() {
    return _count == _size;
}

bool RGBWWLedAnimationQ::push(RGBWWLedAnimation* animation) {
    if (isFull())
        return false;

    _count++;
    _q[_front] = animation;
    _front = (_front + 1) % _size;
    return true;
}

bool RGBWWLedAnimationQ::pushFront(RGBWWLedAnimation* animation) {
    if (isFull())
        return false;

    ++_count;
    --_back;
    if (_back < 0)
        _back = _size + _back;

    _q[_back] = animation;
    return true;
}

void RGBWWLedAnimationQ::clear() {
    while (!isEmpty()) {
        RGBWWLedAnimation* animation = pop();
        if (animation != NULL) {
            delete animation;
        }
    }
}

RGBWWLedAnimation* RGBWWLedAnimationQ::peek() {
    if (!isEmpty()) {
        return _q[_back];
    }
    return NULL;
}

RGBWWLedAnimation* RGBWWLedAnimationQ::pop() {
    RGBWWLedAnimation* tmpptr;
    if (!isEmpty()) {
        _count--;
        tmpptr = _q[_back];
        _q[_back] = NULL;
        _back = (_back + 1) % _size;
        return tmpptr;
    }
    return NULL;
}
