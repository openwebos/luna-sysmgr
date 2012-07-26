/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

class HostArmOpal : public HostArm
{
public:
	HostArmOpal();
	virtual ~HostArmOpal();

	virtual const char* hardwareName() const;

	virtual bool homeButtonWakesUpScreen();
	virtual int getNumberOfSwitches() const;

protected:
	virtual void turboMode(bool enable);
};

HostArmOpal::HostArmOpal()
{
}

HostArmOpal::~HostArmOpal()
{
}

const char* HostArmOpal::hardwareName() const
{
    switch (m_hwRev) {
        default:
            return "Opal -- unknown revision";
    }
}

bool HostArmOpal::homeButtonWakesUpScreen()
{
	return true;
}

int HostArmOpal::getNumberOfSwitches() const
{
	// No ringer, No slider, Has Headset, Has Power
	return 2;
}

void HostArmOpal::turboMode(bool enable)
{
    if (G_UNLIKELY(!Settings::LunaSettings()->allowTurboMode))
        return;

    g_warning("Turbo mode %s", enable ? "on" : "off");

    int fd = ::open("/sys/devices/system/cpu/cpufreq/ondemandtcl/up_threshold", O_WRONLY);
    if (fd < 0) {
        g_critical("%s: Failed to open cpu up_threshold sys file", __PRETTY_FUNCTION__);
        return;
    }

    ::write(fd, enable ? "40" : "95", 2);

    ::close(fd);
}
