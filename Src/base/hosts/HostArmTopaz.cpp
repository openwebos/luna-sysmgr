/**
 * @file
 * 
 * Device-specific functionality for the Topaz devices
 *
 * @author Hewlett-Packard Development Company, L.P.
 * @author tyrok1
 *
 * @section LICENSE
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
 */




#include "Common.h"

#include "HostArm.h"

/**
 * Device-specific functionality for the Topaz devices
 * 
 * Device details:
 * - ARMv7.
 * - Turbo mode from 40% to 95% of speed.
 * - Switches (2): headphones inserted and power.
 * - Raw orientation readings are rotated +90 degrees from correct orientation before being translated.
 * - Home button wakes up the screen.
 * 
 * @see https://en.wikipedia.org/wiki/HP_TouchPad
 */
class HostArmTopaz : public HostArm
{
public:
	/**
	 * Constructs a Topaz device host
	 */
	HostArmTopaz();
	
	/**
	 * Destroys a Topaz device host
	 */
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
  * Translates an orientation sensor reading to the actual correct value
  * 
  * For Topaz, the orientation events are rotated by +90 relative to the
  * display orientation. For Topaz Screen up = Left Orientation
  *
  * Following is the actual screen (Frame Buffer) orientation
  *
  * <pre>
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
  * </pre>
  *-----------------------------------------------------------
  *
  * But, Nyx gives us data 270 rotated clock-wise (-90 anti clock-wise)
  *
  * <pre>
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
  * </pre>
  * 
  * @param	curOrientation			Raw sensor event.
  * @return					Translated (correct) sensor reading event.
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
