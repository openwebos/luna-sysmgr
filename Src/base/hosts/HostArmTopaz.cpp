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

class HostArmTopaz : public HostArm
{
public:
	HostArmTopaz();
	virtual ~HostArmTopaz();

	virtual const char* hardwareName() const;

	virtual bool homeButtonWakesUpScreen();
	virtual int getNumberOfSwitches() const;
    virtual OrientationEvent* postProcessDeviceOrientation(OrientationEvent* currOrientation);
protected:
	virtual void turboMode(bool enable);
};

HostArmTopaz::HostArmTopaz()
{
}

HostArmTopaz::~HostArmTopaz()
{
}

const char* HostArmTopaz::hardwareName() const
{
    switch (m_hwRev) {
        default:
            return "Topaz -- unknown revision";
    }
}

bool HostArmTopaz::homeButtonWakesUpScreen()
{
	return true;
}

int HostArmTopaz::getNumberOfSwitches() const
{
	// No ringer, No slider, Has Headset, Has Power
	return 2;
}

void HostArmTopaz::turboMode(bool enable)
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

/**
  * For Topaz, the orientation events are rotated by +90 relative to the
  * display orientation. For Topaz Screen up = Left Orientation
  *
  * Following is the actual screen (Frame Buffer) orientation
  *
  *       Left(90)
  *     -----------
  *     |         |
  *     |         |
  *  UP |         | Down (180)
  *  0  |         |
  *     |         |
  *     -----------
  *          o <- home button
  *     -----------
  *    Right(270/-90)
  *-----------------------------------------------------------
  *
  * But, Nyx gives us data 270 rotated clock-wise (-90 anti clock-wise)
  *
  *             Up
  *         -----------
  *         |         |
  *         |         |
  *  Right  |         | Left
  *         |         |
  *         |         |
  *         -----------
  *             o <- home button
  *         -----------
  *            Down
  */
OrientationEvent* HostArmTopaz::postProcessDeviceOrientation(OrientationEvent* currOrientation)
{
    if (currOrientation)
    {
        OrientationEvent::Orientation topazEvent = OrientationEvent::Orientation_Invalid;

        switch(currOrientation->orientation())
        {
            case OrientationEvent::Orientation_Left:    {topazEvent = OrientationEvent::Orientation_Up;     break;}
            case OrientationEvent::Orientation_Down:    {topazEvent = OrientationEvent::Orientation_Left;   break;}
            case OrientationEvent::Orientation_Right:   {topazEvent = OrientationEvent::Orientation_Down;   break;}
            case OrientationEvent::Orientation_Up:      {topazEvent = OrientationEvent::Orientation_Right;  break;}
            default:                                    {topazEvent = currOrientation->orientation();       break;}
        }

        return (new OrientationEvent(topazEvent, 0, 0));
    }

    return 0;
}
