/**
 * @file
 * 
 * Device-specific functionality for the Mantaray-based devices
 *
 * @author Hewlett-Packard Development Company, L.P.
 * @author tyrok1
 *
 * @section LICENSE
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
 */




#include "Common.h"

#include "HostArm.h"

/**
 * Device-specific functionality for the Mantaray-based devices
 * 
 * Device details:
 * - ARMv7.
 * - No turbo mode.
 * - Switches (3): ringer, slider, and headphones inserted.
 * 
 * @see https://en.wikipedia.org/wiki/Pre_3
 */
class HostArmMantaray : public HostArm
{
public:
	/**
	 * Constructs a Mantaray-based device host
	 */
	HostArmMantaray();
	
	/**
	 * Destroys a Mantaray-based device host
	 */
	virtual ~HostArmMantaray();

	
	/**
	 * @copybrief HostArm::hardwareName()
	 * 
	 * Returns one of the following depending on
	 * hardware model:
	 * 
	 * - Mantaray EVT1
	 * - Mantaray EVT2
	 * - Mantaray EVT3
	 * - Mantaray DVT1
	 * - Mantaray DVT2
	 * - Mantaray DVT3
	 * - Mantaray -- unknown revision
	 * 
	 * @return				Returns a string starting with "Mantaray".  See description for full list.
	 */
	virtual const char* hardwareName() const;
};

HostArmMantaray::HostArmMantaray()
{
}

HostArmMantaray::~HostArmMantaray()
{
}

const char* HostArmMantaray::hardwareName() const
{
	switch (m_hwRev) {
		case HidHardwareRevisionEVT1:
			return "Mantaray EVT1";
		case HidHardwareRevisionEVT2:
			return "Mantaray EVT2";
		case HidHardwareRevisionEVT3:
			return "Mantaray EVT3";
		case HidHardwareRevisionDVT1:
			return "Mantaray DVT1";
		case HidHardwareRevisionDVT2:
			return "Mantaray DVT2";
		case HidHardwareRevisionDVT3:
			return "Mantaray DVT3";
		default:
			return "Mantaray -- unknown revision";
	}
}

