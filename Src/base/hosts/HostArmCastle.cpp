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

#include <stdlib.h>
#include <sys/types.h>
#include <linux/input.h>
#include <string.h>
#include <limits.h>
#include <lunaprefs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "HostArm.h"
#include "Settings.h"

class HostArmCastle : public HostArm
{
public:
	HostArmCastle();
	virtual ~HostArmCastle();

	virtual const char* hardwareName() const;
	
protected:
	virtual void turboMode(bool enable);
};

HostArmCastle::HostArmCastle()
{
}

HostArmCastle::~HostArmCastle()
{
}

const char* HostArmCastle::hardwareName() const
{
    if (m_hwPlatform == HidHardwarePlatformCastlePlus) {
        switch (m_hwRev) {
            case HidHardwareRevisionEVT1:
                return "Castle Plus EVT1";
            case HidHardwareRevisionEVT2:
                return "Castle Plus EVT2";
            case HidHardwareRevisionEVT3:
                return "Castle Plus EVT3";
            case HidHardwareRevisionDVT1:
                return "Castle Plus DVT1";
            case HidHardwareRevisionDVT2:
                return "Castle Plus DVT2";
            case HidHardwareRevisionDVT3:
                return "Castle Plus DVT3";
            default:
                return "Castle Plus -- unknown revision";
        }
    }
    else if (m_hwPlatform == HidHardwarePlatformRoadrunner) {
	    return "Roadrunner";
    }
    else {
        switch (m_hwRev) {
            case HidHardwareRevisionEVT1:
                return "Castle EVT1";
            case HidHardwareRevisionEVT2:
                return "Castle EVT2";
            case HidHardwareRevisionEVT3:
                return "Castle EVT3";
            case HidHardwareRevisionDVT1:
                return "Castle DVT1";
            case HidHardwareRevisionDVT2:
                return "Castle DVT2";
            case HidHardwareRevisionDVT3:
                return "Castle DVT3";
            default:
                return "Castle -- unknown revision";
        }
    }
}

void HostArmCastle::turboMode(bool enable)
{
#if defined(MACHINE_ROADRUNNER)
	return;
#endif

	if (G_UNLIKELY(!Settings::LunaSettings()->allowTurboMode))

		return;
	
	g_warning("Turbo mode %s", enable ? "on" : "off");

	int fd = ::open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", O_WRONLY);
	if (fd < 0) {
		g_critical("%s: Failed to open cpu scaling sys file", __PRETTY_FUNCTION__);
		return;
	}

	::write(fd, enable ? "600000" : "500000", 6);

	::close(fd);
}

