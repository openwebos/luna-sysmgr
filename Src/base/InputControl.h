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




#ifndef _INPUTCONTROL_H_
#define _INPUTCONTROL_H_

#if !defined(TARGET_DESKTOP)
#include <nyx/nyx_client.h>
#endif

class InputControl
{
public:

    virtual ~InputControl() {};

    virtual bool on() = 0;
    virtual bool off() = 0;

#if !defined(TARGET_DESKTOP)
    virtual bool setRate(nyx_report_rate_t rate) =0;
    virtual nyx_device_handle_t getHandle() = 0;
#else
    virtual void* getHandle() { return 0;}
#endif

};

#endif /* _INPUTCONTROL_H_ */
