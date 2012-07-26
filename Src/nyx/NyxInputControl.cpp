/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */




#include "NyxInputControl.h"
#include <glib.h>

NyxInputControl::NyxInputControl(nyx_device_type_t type, nyx_device_id_t id) : m_handle(0)
{
    nyx_error_t error = nyx_device_open(type, id, &m_handle);
    if ((error != NYX_ERROR_NONE) || (m_handle == NULL))
    {
        g_critical("Failed to open NYX device %d: %d", type, error);
    }
}

NyxInputControl::~NyxInputControl()
{
    if (m_handle)
    {
        nyx_error_t error = nyx_device_close(m_handle);
        if (error != NYX_ERROR_NONE)
            g_critical("Unable to release m_handle");
    }
}

bool NyxInputControl::on()
{
    if (m_handle)
    {
        nyx_error_t error = NYX_ERROR_NONE;
        error = nyx_device_set_operating_mode(m_handle, NYX_OPERATING_MODE_ON);
        return (error == NYX_ERROR_NONE || error == NYX_ERROR_NOT_IMPLEMENTED);
    }
    return true;
}

bool NyxInputControl::off()
{
    if (m_handle)
    {
        nyx_error_t error = NYX_ERROR_NONE;
        error = nyx_device_set_operating_mode(m_handle, NYX_OPERATING_MODE_OFF);
        return (error == NYX_ERROR_NONE || error == NYX_ERROR_NOT_IMPLEMENTED);
    }
    return true;
}

bool NyxInputControl::setRate(nyx_report_rate_t rate)
{
    if (m_handle)
    {
        nyx_error_t error = NYX_ERROR_NONE;
        error = nyx_device_set_report_rate(m_handle, rate);
        return (error == NYX_ERROR_NONE || error == NYX_ERROR_NOT_IMPLEMENTED);
    }
    return true;
}
