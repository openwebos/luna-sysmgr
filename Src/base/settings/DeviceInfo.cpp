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




#include "Common.h"

#include "DeviceInfo.h"

#include "HostBase.h"
#include "Localization.h"
#include "Settings.h"
#include "Window.h"

#if defined(HAS_LUNA_PREF)
#include <lunaprefs.h>
#endif

#include <cjson/json.h>

static DeviceInfo* s_instance = 0;
static const int kTouchableHeight = 48;

DeviceInfo* DeviceInfo::instance()
{
	if (G_UNLIKELY(s_instance == 0))
		new DeviceInfo;

	return s_instance;
}

DeviceInfo::DeviceInfo()
{
	s_instance = this;

	gatherInfo();
}

DeviceInfo::~DeviceInfo()
{
    s_instance = 0;
}

std::string DeviceInfo::jsonString() const
{
    return m_jsonString;
}

static bool getLunaPrefSystemValue(const char* key, std::string& value)
{
#if defined(HAS_LUNA_PREF)

	char* str = 0;
	if (LP_ERR_NONE == LPSystemCopyStringValue(key, &str) && str) {
		value = str;
		g_free((gchar*) str);
		return true;
	}	
#endif

	value = LOCALIZED("Unknown");
	return false;
}


void DeviceInfo::gatherInfo()
{
    // Display information --------------------------------------------------------
	const HostInfo& hostInfo = HostBase::instance()->getInfo();
	m_screenDensity = 1.0f;
	m_hardwareScreenWidth = hostInfo.displayWidth;
	m_hardwareScreenHeight = hostInfo.displayHeight;

	m_screenWidth = (int) (m_hardwareScreenWidth / m_screenDensity);
	m_screenHeight = (int) (m_hardwareScreenHeight / m_screenDensity);

	Settings* settings = Settings::LunaSettings();

	int maxNegativeSpaceHeight = (int) (settings->maximumNegativeSpaceHeightRatio *
										m_screenHeight);
		
	m_maximumCardWidth = m_screenWidth;
	m_maximumCardHeight = m_screenHeight - settings->positiveSpaceTopPadding;

	m_minimumCardWidth = m_screenWidth;
	m_minimumCardHeight = m_screenHeight - settings->positiveSpaceTopPadding -
						  maxNegativeSpaceHeight;

	m_touchableRows = (m_screenHeight
					   - settings->positiveSpaceTopPadding
					   - settings->positiveSpaceBottomPadding) / kTouchableHeight;

	// Platform Info --------------------------------------------------------------

	getLunaPrefSystemValue("com.palm.properties.ProdSN", m_serialNumber);
	getLunaPrefSystemValue("com.palm.properties.DMCARRIER", m_carrierName);

	// If getLunaPrefSystemValue fails, such as luna-prefs not yet built for the platform, return
        // a sane version number for applications
        if(!getLunaPrefSystemValue("com.palm.properties.version", m_platformVersion))
        {
            // This should only be returned if the luna-prefs package is not working, or
            // the luna-prefs system version has not been set.
            m_platformVersion = "3.5.0";
        }
        else if (m_platformVersion.find("Palm webOS ", 0) != std::string::npos)
        {
            m_platformVersion = m_platformVersion.substr(11);
        }
        else if (m_platformVersion.find("HP webOS ", 0) != std::string::npos)
        {
            m_platformVersion = m_platformVersion.substr(9);
        } 
        else if (m_platformVersion.find("Open webOS ", 0) != std::string::npos)
        {
            m_platformVersion = m_platformVersion.substr(11);
        }

        std::string platformVersion = m_platformVersion;

        size_t npos1 = 0, npos2 = 0;
        npos1 = platformVersion.find_first_of ('.');
        if (npos1 != std::string::npos && npos1 <= platformVersion.size() - 1)
            npos2 = platformVersion.find_first_of ('.', npos1 + 1);
        if (npos1 == std::string::npos || npos2 == std::string::npos)  {
            m_platformVersionMajor = m_platformVersionMinor = m_platformVersionDot = -1;
        }
        else {
            m_platformVersionMajor = atoi ((platformVersion.substr (0, npos1)).c_str());
            m_platformVersionMinor = atoi ((platformVersion.substr (npos1+1, npos2)).c_str());
            m_platformVersionDot = atoi ((platformVersion.substr (npos2+1)).c_str());
        }

	// WIFI and bluetooth ---------------------------------------------------------

	std::string dummy;

	if (getLunaPrefSystemValue("com.palm.properties.WIFIoADDR", dummy))
		m_wifiAvailable = true;
	else
		m_wifiAvailable = false;

	if (getLunaPrefSystemValue("com.palm.properties.BToADDR", dummy))
		m_bluetoothAvailable = true;
	else
		m_bluetoothAvailable = false;


	// RadioType and carrier availability -----------------------------------------
    gchar* buffer;
    gsize sz;
    int token = 0;

    if( !g_file_get_contents( "/dev/tokens/RadioType", &buffer, &sz, 0 ) ) {
    	g_warning("RadioType token not found!");
    	m_radioType = 0;
    } else {
        token = atoi(buffer);

        g_free(buffer);
        m_radioType = token;
    }

/*	
	// Storage --------------------------------------------------------------------

	getLunaPrefSystemValue("com.palm.properties.storageCapacity", dummy);
	if (!dummy.empty())
		m_storageTotal = strtod(dummy.c_str(), NULL);
	else
		m_storageTotal = 0.0;
*/

	// Keyboard configration -----------------------------------------------------

	std::string hwName = HostBase::instance()->hardwareName();
        if (hwName == "Desktop") {
            m_modelName = "Desktop";
            m_modelNameAscii = "Desktop";
            m_keyboardAvailable = true;
            m_keyboardSlider = false;
            m_coreNaviButton = true;
            m_keyboardType = "QWERTY";
            m_swappableBattery = false;
        } else {
            if (!getLunaPrefSystemValue("com.palm.properties.deviceNameShortBranded", m_modelName))
                m_modelName = "webOS smartphone";

		if (!getLunaPrefSystemValue("com.palm.properties.deviceNameShort", m_modelNameAscii))
			m_modelNameAscii = "webOS smartphone";
		
		m_keyboardSlider = false;
		m_coreNaviButton = false;
        m_keyboardAvailable = false;
		m_swappableBattery = false;

		// Castle
#if defined(MACHINE_CASTLE)		
		if (hwName.find("Castle Plus", 0) != std::string::npos
				|| hwName.find ("Roadrunner", 0) != std::string::npos) {

			m_keyboardSlider = true;
			m_coreNaviButton = false;
			m_swappableBattery = true;
		}
		else if (hwName.find("Castle", 0) != std::string::npos) {

			m_keyboardSlider = true;
			m_coreNaviButton = true;
			m_swappableBattery = true;
		}
#endif

#if defined(MACHINE_WINDSOR)
		if (hwName.find("Windsor", 0) != std::string::npos) {
			
			m_keyboardSlider = true;
			m_coreNaviButton = false;
			m_swappableBattery = true;
		}
#endif

#if defined(MACHINE_BROADWAY)
		if (hwName.find("Broadway", 0) != std::string::npos) {
			
			m_keyboardSlider = true;
			m_coreNaviButton = false;
			m_swappableBattery = false;
		}
#endif
		
#if defined(MACHINE_PIXIE)
		if (hwName.find("Pixie", 0) != std::string::npos) {

			m_keyboardSlider = false;
			m_coreNaviButton = false;
			m_swappableBattery = true;
		}
#endif		

		if (getLunaPrefSystemValue("com.palm.properties.KEYoBRD", dummy)) {

			m_keyboardAvailable = true;
			
			if (dummy == "z")
				m_keyboardType = "QWERTY";
			else if (dummy == "w")
				m_keyboardType = "AZERTY";
			else if (dummy == "y")
				m_keyboardType = "QWERTZ";
			else if (dummy == "w1")
				m_keyboardType = "AZERTY_FR";
			else if (dummy == "y1")
				m_keyboardType = "QWERTZ_DE";
			else
				m_keyboardType = LOCALIZED("Unknown");
		}
        else {
            m_keyboardAvailable = false;
            m_keyboardType = LOCALIZED("Unknown");
        }
	}

	// Compose json string from the parameters  -------------------------------

	json_object* json = json_object_new_object();

	json_object_object_add(json, (char*) "modelName", json_object_new_string(m_modelName.c_str()));
	json_object_object_add(json, (char*) "modelNameAscii", json_object_new_string(m_modelNameAscii.c_str()));
	json_object_object_add(json, (char*) "platformVersion", json_object_new_string(m_platformVersion.c_str()));
	json_object_object_add(json, (char*) "platformVersionMajor", json_object_new_int (m_platformVersionMajor));
	json_object_object_add(json, (char*) "platformVersionMinor", json_object_new_int (m_platformVersionMinor));
	json_object_object_add(json, (char*) "platformVersionDot", json_object_new_int (m_platformVersionDot));
	json_object_object_add(json, (char*) "carrierName", json_object_new_string(m_carrierName.c_str()));
	json_object_object_add(json, (char*) "serialNumber", json_object_new_string(m_serialNumber.c_str()));

	json_object_object_add(json, (char*) "screenWidth", json_object_new_int(m_screenWidth));
	json_object_object_add(json, (char*) "screenHeight", json_object_new_int(m_screenHeight));

	json_object_object_add(json, (char*) "minimumCardWidth", json_object_new_int(m_minimumCardWidth));
	json_object_object_add(json, (char*) "minimumCardHeight", json_object_new_int(m_minimumCardHeight));
	json_object_object_add(json, (char*) "maximumCardWidth", json_object_new_int(m_maximumCardWidth));
	json_object_object_add(json, (char*) "maximumCardHeight", json_object_new_int(m_maximumCardHeight));

	json_object_object_add(json, (char*) "touchableRows", json_object_new_int(m_touchableRows));
	
	json_object_object_add(json, (char*) "keyboardAvailable", json_object_new_boolean(m_keyboardAvailable));
	json_object_object_add(json, (char*) "keyboardSlider", json_object_new_boolean(m_keyboardSlider));
	json_object_object_add(json, (char*) "keyboardType", json_object_new_string(m_keyboardType.c_str()));

	json_object_object_add(json, (char*) "wifiAvailable", json_object_new_boolean(m_wifiAvailable));
	json_object_object_add(json, (char*) "bluetoothAvailable", json_object_new_boolean(m_bluetoothAvailable));

	json_object_object_add(json, (char*) "carrierAvailable", json_object_new_boolean(carrierAvailable()));

	json_object_object_add(json, (char*) "coreNaviButton", json_object_new_boolean(m_coreNaviButton));
	
	json_object_object_add(json, (char*) "swappableBattery", json_object_new_boolean(m_swappableBattery));
	json_object_object_add(json, (char*) "dockModeEnabled", json_object_new_boolean(true));

	m_jsonString = json_object_to_json_string(json);

	json_object_put(json);
}

bool DeviceInfo::keyboardSlider() const
{
	return m_keyboardSlider;    
}

bool DeviceInfo::coreNaviButton() const
{
	return m_coreNaviButton;    
}

unsigned int DeviceInfo::platformVersionMajor() const
{
	return m_platformVersionMajor;    
}

unsigned int DeviceInfo::platformVersionMinor() const
{
	return m_platformVersionMinor;
}

unsigned int DeviceInfo::platformVersionDot() const
{
	return m_platformVersionDot;
}
