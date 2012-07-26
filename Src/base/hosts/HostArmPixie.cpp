/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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




#include "Common.h"

#include "HostArm.h"

class HostArmPixie : public HostArm
{
public:
	HostArmPixie();
	virtual ~HostArmPixie();

	virtual const char* hardwareName() const;

	virtual int getNumberOfSwitches() const { return 2; }
	virtual void getInitialSwitchStates();
	virtual void wakeUpLcd();
};

HostArmPixie::HostArmPixie()
{
}

HostArmPixie::~HostArmPixie()
{
}

const char* HostArmPixie::hardwareName() const
{
    switch (m_hwRev) {
        case HidHardwareRevisionEVT1:
            return "Pixie EVT1";
        case HidHardwareRevisionEVT2:
            return "Pixie EVT2";
        case HidHardwareRevisionEVT3:
            return "Pixie EVT3";
        case HidHardwareRevisionDVT1:
            return "Pixie DVT1";
        case HidHardwareRevisionDVT2:
            return "Pixie DVT2";
        case HidHardwareRevisionDVT3:
            return "Pixie DVT3";
        case HidHardwareRevisionDVT4:
            return "Pixie DVT4";
        case HidHardwareRevisionA:
            return "Pixie A";
        case HidHardwareRevisionB:
            return "Pixie B";
        case HidHardwareRevisionC:
            return "Pixie C";
        default:
            return "Pixie -- unknown revision";
    }
}

void HostArmPixie::getInitialSwitchStates()
{
	LSError err;
	LSErrorInit(&err);

	if (!LSCall(m_service, HIDD_RINGER_URI, HIDD_GET_STATE, HostArm::switchStateCallback, (void*)SW_RINGER, NULL, &err))
		goto Error;

	if (!LSCall(m_service, HIDD_HEADSET_URI, HIDD_GET_STATE, HostArm::switchStateCallback, (void*)SW_HEADPHONE_INSERT, NULL, &err))
		goto Error;

Error:

	if (LSErrorIsSet(&err)) {
		LSErrorPrint(&err, stderr);
		LSErrorFree(&err);
	}
}

void HostArmPixie::wakeUpLcd()
{
	(void) ::system("echo 1 > /sys/devices/platform/mddi_power.0/state");
}
