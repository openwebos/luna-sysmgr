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

class HostArmMantaray : public HostArm
{
public:
	HostArmMantaray();
	virtual ~HostArmMantaray();

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

