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

class HostArmBroadway : public HostArm
{
public:
	HostArmBroadway();
	virtual ~HostArmBroadway();

	virtual const char* hardwareName() const;

	int getNumberOfSwitches() const;
};

HostArmBroadway::HostArmBroadway()
{
}

HostArmBroadway::~HostArmBroadway()
{
}

const char* HostArmBroadway::hardwareName() const
{
	switch (m_hwRev) {
		case HidHardwareRevisionEVT1:
			return "Broadway EVT1";
		case HidHardwareRevisionEVT2:
			return "Broadway EVT2";
		case HidHardwareRevisionEVT3:
			return "Broadway EVT3";
		case HidHardwareRevisionDVT1:
			return "Broadway DVT1";
		case HidHardwareRevisionDVT2:
			return "Broadway DVT2";
		case HidHardwareRevisionDVT3:
			return "Broadway DVT3";
		default:
			return "Broadway -- unknown revision";
	}
}

int HostArmBroadway::getNumberOfSwitches() const
{
    // broadway only has a ringer and a slider, but no jack, so there is no headset switch.
	return 2;
}
