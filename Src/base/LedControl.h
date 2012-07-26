/**
 *******************************************************************************
 * @file LedControl.h
 *
 * Copyright (c) 2010 Hewlett-Packard Development Company, L.P.
 * All rights reserved.
 *
 * @brief
 *
 *******************************************************************************
 */

#ifndef _LEDCONTROL_H_
#define _LEDCONTROL_H_

typedef void (*LedControlCallback)(void* ctx);


class LedControl
{
public:

    virtual ~LedControl() {};

    virtual bool setBrightness(int keypadBrightness, int displayBrightness,
        LedControlCallback callback, void* ctx) = 0;

};

#endif /* _LEDCONTROL_H_ */
