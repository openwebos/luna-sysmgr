/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include "Common.h"
#include "Window.h"

#include <string>

class DeviceInfo
{
public:

	static DeviceInfo* instance();
	~DeviceInfo();

	std::string jsonString() const;

	bool keyboardSlider() const;
	bool coreNaviButton() const;
	bool wifiAvailable() const { return m_wifiAvailable; }
	bool bluetoothAvailable() const { return m_bluetoothAvailable; }
	bool carrierAvailable() const {return (m_radioType != 0);}
	bool compassAvailable() const { return false; } // FIXME
	bool accelerometerAvailable() const { return true; } // FIXME
	bool dockModeEnabled() const { return true; }
    bool keyboardAvailable() const { return m_keyboardAvailable; }

	const std::string& platformVersion() const { return m_platformVersion; }
	unsigned int platformVersionMajor() const;
	unsigned int platformVersionMinor() const;
	unsigned int platformVersionDot() const;

private:

	DeviceInfo();

	void gatherInfo();

private:

	std::string m_jsonString;

	int m_screenWidth;
	int m_screenHeight;

	int m_hardwareScreenWidth;
	int m_hardwareScreenHeight;
	float m_screenDensity;

	int m_minimumCardWidth;
	int m_minimumCardHeight;
	int m_maximumCardWidth;
	int m_maximumCardHeight;

	int m_touchableRows;
	
	std::string m_modelName;
	std::string m_modelNameAscii;
	std::string m_platformVersion;

	// platform versions are <major>.<minor>.<dot>
	unsigned int m_platformVersionMajor;
	unsigned int m_platformVersionMinor;
	unsigned int m_platformVersionDot;

	std::string m_carrierName;
	std::string m_serialNumber;

	// double m_storageTotal;
	
	bool m_keyboardAvailable;
	bool m_keyboardSlider;
	std::string m_keyboardType;

	bool m_wifiAvailable;
	bool m_bluetoothAvailable;

	bool m_coreNaviButton;
	
	bool m_swappableBattery;
	int m_radioType;
};

#endif /* DEVICEINFO_H */
