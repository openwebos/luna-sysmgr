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




#include "BtDeviceClass.h"

namespace BtDeviceClass {


	bool isCODInMask(unsigned int cod, unsigned int mask)
	{
		if (mask == 0)
			return true;

		// Match the service mask exactly
		unsigned int c = cod & COD_SERVICE_MASK;
		unsigned int m = mask & COD_SERVICE_MASK;

		if (c != 0 && m != 0 && (c & m) != m)
			return false;

		// Match the major class exactly
		c = cod & COD_MAJOR_MASK;
		m = mask & COD_MAJOR_MASK;
		if (c != 0 && m != 0 && (c & m) != m)
			return false;

		// Match the minor class
		c = cod & COD_MINOR_MASK;
		m = mask & COD_MINOR_MASK;
		if (c != 0 && m != 0 && (c & m) != c)
			return false;

		return true;
	}

	bool isAudioDevice(unsigned int cod)
	{
	//	if ((cod & COD_MAJOR_AUDIO) == COD_MAJOR_AUDIO)
	//		return true;
		return false;
	}

	bool isHFGSupported(unsigned int cod)
	{
		return isCODInMask(cod, HFG_MASK);
	}

	bool isPhone(unsigned int cod)
	{
		return ((( cod & COD_MAJOR_PHONE ) == COD_MAJOR_PHONE) && ((cod & COD_TELEPHONY) == COD_TELEPHONY));
	}

	bool isA2DPSupported(unsigned int cod)
	{
		return isCODInMask(cod, A2DP_MASK);
	}

	bool isComputerOrPhone(unsigned int cod) {
		return isCODInMask(cod,COD_MAJOR_COMPUTER) || isCODInMask(cod,COD_MAJOR_PHONE);
	}

}

