/**
 *******************************************************************************
 * @file NyxLedControl.h
 *
 * Copyright (c) 2010 Hewlett-Packard Development Company, L.P.
 * All rights reserved.
 *
 * @brief
 *
 *******************************************************************************
 */

#ifndef _NYXLEDCONTROL_H_
#define _NYXLEDCONTROL_H_

#include "Common.h"
#include "LedControl.h"

#include <nyx/nyx_client.h>

class NyxLedControl: public LedControl {
public:
    NyxLedControl(nyx_device_id_t id);
    virtual ~NyxLedControl();
    virtual bool setBrightness(int keypadBrightness, int displayBrightness, LedControlCallback callback, void* ctx);

    LedControlCallback m_callback;
    void* m_ctx;

private:
    nyx_device_handle_t m_handle;
};

#endif /* _NYXINPUTCONTROL_H_ */
