/**
 * @file
 * 
 * Device-specific functionality for unknown types of ARM-based devices
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
 * Device-specific functionality for unknown types of ARM-based devices
 * 
 * Device details:
 * - Basically the point of this is that just about all we know is that it's based on an ARM processor.
 */
class HostArmUnknown : public HostArm
{
public:
	/**
	 * Constructs an unknown ARM type device host
	 */
	HostArmUnknown();
	
	/**
	 * Destroys an unknown ARM type device host
	 */
	virtual ~HostArmUnknown();
	
	/**
	 * @copybrief HostArm::hardwareName()
	 * 
	 * @return				Returns the string "Unknown hardware type".
	 */
	virtual const char* hardwareName() const;
};

HostArmUnknown::HostArmUnknown()
{
}

HostArmUnknown::~HostArmUnknown()
{
}

const char* HostArmUnknown::hardwareName() const
{
	return "Unknown hardware type";
}
