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

#include <sstream>

#include "Preferences.h"
#include "Settings.h"

#include <sqlite3.h>
#include <cjson/json.h>

#include "HostBase.h"
#include "Logging.h"
#include "MutexLocker.h"
#include "WindowServer.h"
#include "JSONUtils.h"
#include "VirtualKeyboardPreferences.h"
#include <QLocale>
#include "Localization.h"

static const char* s_defaultLocale = "en_us";
static const char* s_defaultLocaleRegion = "us";
static const char* s_defaultPhoneRegion = "us";
static const char* s_prefsDbPath = "/var/luna/preferences/systemprefs.db";

static const char* s_logChannel = "Preferences";

#ifdef HAVE_QPA			
extern "C" void setAdvancedGestures(int);
#endif

Preferences* Preferences::instance()
{
	static Preferences* s_prefs = 0;
	if (!s_prefs)
		s_prefs = new Preferences();

	return s_prefs;
}

Preferences::Preferences()
	: m_currentTimeFormat("HH12")
	, m_showAlertsWhenLocked(true)
	, m_ledThrobberEnabled(true)
	, m_playFeedbackSounds(true)
	, m_sysUiNoHomeButtonMode(true)
	, m_sysUiEnableNextPrevGestures(false)
	, m_lockTimeout(0)
	, m_lsHandle(0)
	, m_imeEnabled(false)
	, m_pinyinEnabled(false)
	, m_pinyinPassthrough(false)
	, m_hwrEnabled(false)
	, m_roamingIndicator("")
	, m_hideWANAlert(false)
	, m_dualRSSI(false)
	, m_airplaneMode(false)
	, m_wifiOn(false)
	, m_bluetoothOn(false)
	, m_show3GForEvdo(false)
	, m_enableVoiceDial(false)
    , m_rotationLock(OrientationEvent::Orientation_Invalid)
	, m_muteOn(false)
	, m_enableALS(true)
{
	init();
	registerService();
}

Preferences::~Preferences()
{
}

std::string Preferences::locale() const
{
	MutexLocker locker(&m_mutex);
	return m_locale;
}

std::string Preferences::localeRegion() const
{
	MutexLocker locker(&m_mutex);
	return m_localeRegion;
}

std::string Preferences::phoneRegion() const
{
	MutexLocker locker(&m_mutex);
	return m_phoneRegion;
}

std::string Preferences::timeFormat() const
{
	MutexLocker locker(&m_mutex);
	return m_currentTimeFormat;	
}

uint32_t Preferences::lockTimeout() const
{
	MutexLocker locker(&m_mutex);
	return m_lockTimeout;
}

bool Preferences::airplaneMode() const
{
	MutexLocker locker(&m_mutex);
	return m_airplaneMode;
}

bool Preferences::setAirplaneMode(bool on)
{
	m_airplaneMode = on;

	pbnjson::JValue pref = pbnjson::Object();
	pref.put("airplaneMode", m_airplaneMode);

	LSError error;
	LSErrorInit(&error);

	std::string	valueStr = jsonToString(pref);

	bool ret = LSCall(Preferences::instance()->m_lsHandle,
				 "palm://com.palm.systemservice/setPreferences", valueStr.c_str(),
				 NULL, NULL, NULL, &error);
	if (!ret) {
		g_warning("%s: Failed setting 'airplaneMode' to '%d' (%s)",
				   __FUNCTION__, m_airplaneMode, error.message);
		LSErrorFree(&error);
		return false;
	}
	return true;
}

bool Preferences::wifiState() const
{
	MutexLocker locker(&m_mutex);
	return m_wifiOn;
}

bool Preferences::saveWifiState(bool on)
{
	m_wifiOn = on;

	pbnjson::JValue pref = pbnjson::Object();
	pref.put("wifiRadio", m_wifiOn);

	LSError error;
	LSErrorInit(&error);

	std::string	valueStr = jsonToString(pref);

	bool ret = LSCall(Preferences::instance()->m_lsHandle,
				 "palm://com.palm.systemservice/setPreferences", valueStr.c_str(),
				 NULL, NULL, NULL, &error);
	if (!ret) {
		g_warning("%s: Failed setting 'wifiRadio' to '%d' (%s)",
				   __FUNCTION__, m_wifiOn, error.message);
		LSErrorFree(&error);
		return false;
	}
	return true;
}

bool Preferences::bluetoothState() const
{
	MutexLocker locker(&m_mutex);
	return m_bluetoothOn;
}

bool Preferences::saveBluetoothState(bool on)
{
	m_bluetoothOn = on;

	pbnjson::JValue pref = pbnjson::Object();
	pref.put("bluetoothRadio", m_bluetoothOn);

	LSError error;
	LSErrorInit(&error);

	std::string	valueStr = jsonToString(pref);

	bool ret = LSCall(Preferences::instance()->m_lsHandle,
				 "palm://com.palm.systemservice/setPreferences", valueStr.c_str(),
				 NULL, NULL, NULL, &error);
	if (!ret) {
		g_warning("%s: Failed setting 'wifiRadio' to '%d' (%s)",
				   __FUNCTION__, m_bluetoothOn, error.message);
		LSErrorFree(&error);
		return false;
	}
	return true;
}


OrientationEvent::Orientation Preferences::rotationLock() const
{
	MutexLocker locker(&m_mutex);
	return m_rotationLock;
}

bool Preferences::setRotationLockPref(OrientationEvent::Orientation lockedOrientation)
{
	m_rotationLock = lockedOrientation;

	pbnjson::JValue pref = pbnjson::Object();
	pref.put("rotationLock", (int)m_rotationLock);

	LSError error;
	LSErrorInit(&error);

	std::string	valueStr = jsonToString(pref);

	bool ret = LSCall(Preferences::instance()->m_lsHandle,
				 "palm://com.palm.systemservice/setPreferences", valueStr.c_str(),
				 NULL, NULL, NULL, &error);
	if (!ret) {
		g_warning("%s: Failed setting 'rotationLock' to '%d' (%s)",
				   __FUNCTION__, m_rotationLock, error.message);
		LSErrorFree(&error);
		return false;
	}
	return true;
}

bool Preferences::isMuteOn() const
{
	MutexLocker locker(&m_mutex);
	return m_muteOn;
}

bool Preferences::isAlsEnabled() const
{
	MutexLocker locker(&m_mutex);
	return m_enableALS;
}

bool Preferences::setMuteSoundPref(bool mute)
{
	m_muteOn = mute;

	pbnjson::JValue pref = pbnjson::Object();
	pref.put("muteSound", m_muteOn);

	LSError error;
	LSErrorInit(&error);

	std::string	valueStr = jsonToString(pref);

	bool ret = LSCall(Preferences::instance()->m_lsHandle,
				 "palm://com.palm.systemservice/setPreferences", valueStr.c_str(),
				 NULL, NULL, NULL, &error);
	if (!ret) {
		g_warning("%s: Failed setting 'muteSound' to '%d' (%s)",
				   __FUNCTION__, m_muteOn, error.message);
		LSErrorFree(&error);
		return false;
	}
	return true;
}

uint32_t Preferences::roundLockTimeout(uint32_t unrounded)
{
	// clamp inactivity to platform supported values
	if (unrounded < 30)
		return 0;
	else if (unrounded < 60)
		return 30;
	else if (unrounded < 120)
		return 60;
	else if (unrounded < 180)
		return 120;
	else if (unrounded < 300)
		return 180;
	else if (unrounded < 600)
		return 300;
	else if (unrounded < 1800)
		return 600;
	else if (unrounded < 9999)
		return 1800;
	return 0;
}

void Preferences::setLockTimeout(uint32_t timeout)
{
	MutexLocker locker(&m_mutex);

	if (m_lockTimeout == timeout)
		return;

	sqlite3* prefsDb = 0;
	if (sqlite3_open(s_prefsDbPath, &prefsDb) == SQLITE_OK) {

		std::stringstream value;
		value << timeout;
		char* queryStr = sqlite3_mprintf("INSERT INTO Preferences VALUES (%Q, %Q)",
										 "lockTimeout", value.str().c_str());

		sqlite3_exec(prefsDb, queryStr, NULL, NULL, NULL);
		sqlite3_free(queryStr);

		if (prefsDb)
			sqlite3_close(prefsDb);
	}

	m_lockTimeout = timeout;
}

void Preferences::init()
{
	m_locale = s_defaultLocale;
	m_localeRegion = s_defaultLocaleRegion;
	m_phoneRegion = s_defaultPhoneRegion;

	// We open the preferences database and read the locale setting.
	// avoid waiting for the system-service to come up
	// and we waiting synchronously to get the locale value

	sqlite3* prefsDb = 0;
	sqlite3_stmt* statement = 0;
	const char* tail = 0;
	json_object* label = 0;
	json_object* json = 0;
	json_object* subobj = 0;
	std::string localeCountryCode;
	
	int ret = sqlite3_open(s_prefsDbPath, &prefsDb);
	if (ret) {
		luna_critical(s_logChannel, "Failed to open preferences db");
		goto Done;
	}

	// immediately read lock timeout
	ret = sqlite3_prepare(prefsDb, "SELECT * FROM Preferences WHERE KEY='lockTimeout'",
						  -1, &statement, &tail);

	if (ret) {
		luna_critical(s_logChannel, "Failed to prepare query");
		goto Done;
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ROW) {

		m_lockTimeout = static_cast<uint32_t>( sqlite3_column_int(statement, 1) );
	}
	else {
		setLockTimeout(m_lockTimeout);
	}

	sqlite3_finalize(statement);

	// immediately read locale
	ret = sqlite3_prepare(prefsDb, "SELECT * FROM Preferences WHERE KEY='locale'",
						  -1, &statement, &tail);
	if (ret) {
		luna_critical(s_logChannel, "Failed to prepare query");
		goto Done;
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ROW) {

		std::string languageCode, countryCode;

		const char* val = (const char*) sqlite3_column_text(statement, 1);
		if (!val)
			goto Done;

		label = 0;		
		json = json_tokener_parse(val);
		if (!json || is_error(json))
			goto Done;

		label = json_object_object_get(json, "languageCode");
		if (!label || is_error(label))
			goto Done;
		languageCode = json_object_get_string(label);

		label = json_object_object_get(json, "countryCode");
		if (!label || is_error(label))
			goto Done;
		countryCode = json_object_get_string(label);

		localeCountryCode = countryCode;

		m_locale = languageCode + "_" + countryCode;

		subobj = json_object_object_get(json, "phoneRegion");

		if (subobj && !is_error(subobj)){
			label = json_object_object_get(subobj, "countryCode");

			if (label && !is_error(label)){
				m_phoneRegion = json_object_get_string(label);
			}
		}

		json_object_put(json);
		json = 0;
	}

	sqlite3_finalize(statement);
	
	// immediately read regon
	ret = sqlite3_prepare(prefsDb, "SELECT * FROM Preferences WHERE KEY='region'",
						  -1, &statement, &tail);
	if (ret) {
		luna_critical(s_logChannel, "Failed to prepare query");
		goto Done;
	}

	ret = sqlite3_step(statement);
	if (ret == SQLITE_ROW) {

		const char* val = (const char*) sqlite3_column_text(statement, 1);
		if (!val)
			goto Done;

		label = 0;		
		json = json_tokener_parse(val);
		if (!json || is_error(json))
			goto Done;

		label = json_object_object_get(json, "countryCode");
		if (!label || is_error(label))
			goto Done;
		m_localeRegion = json_object_get_string(label);

		json_object_put(json);
		json = 0;
	}
	
Done:

	QLocale myLocale (m_locale.c_str());
	g_message ("%s: setting locale country %d language %d", __PRETTY_FUNCTION__,
			myLocale.country(), myLocale.language());
	QLocale::setDefault (myLocale);

	// locale region defaults to locale country code
	if (m_localeRegion.empty())
		m_localeRegion = localeCountryCode;

	if (m_phoneRegion.empty())
		m_phoneRegion = m_localeRegion;

	if (json && !is_error(json))
		json_object_put(json);

	if (statement)
		sqlite3_finalize(statement);

	if (prefsDb)
		sqlite3_close(prefsDb);    
}

void Preferences::registerService()
{
	bool ret;
	LSError error;
	LSErrorInit(&error);

	ret = LSRegister(NULL, &m_lsHandle, &error);
	if (!ret) {
		g_critical("Failed to register handler: %s", error.message);
		LSErrorFree(&error);
		exit(1);
	}

	ret = LSGmainAttach(m_lsHandle, HostBase::instance()->mainLoop(), &error);
	if (!ret) {
		g_critical("Failed to attach service to main loop: %s", error.message);
		LSErrorFree(&error);
		exit(1);
	}

	ret = LSCall(m_lsHandle, "palm://com.palm.lunabus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.systemservice\"}",
				 serverConnectCallback, this, &m_serverStatusToken, &error);
	if (!ret) {
		g_critical("Failed in calling palm://com.palm.lunabus/signal/registerServerStatus: %s",
					  error.message);
		LSErrorFree(&error);
		exit(1);
	}
	
}

bool Preferences::serverConnectCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
	if (!message)
		return true;

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return true;
	
	json_object* label = 0;
	json_object* json = 0;
	bool connected = false;
	Preferences * prefObjPtr;
	
	label = 0;		
	json = json_tokener_parse(payload);
	if (!json || is_error(json))
		goto Done;

	label = json_object_object_get(json, "connected");
	if (!label || is_error(label))
		goto Done;
	connected = json_object_get_boolean(label);

	prefObjPtr = (Preferences *)ctx;
	
	if (connected) {

		// We are connected to the systemservice. call and get the preference values
		
		bool ret = false;
		LSError error;
		LSErrorInit(&error);
		
		ret = LSCall(Preferences::instance()->m_lsHandle,
					 "palm://com.palm.systemservice/getPreferences", "{\"subscribe\":true, \"keys\": [ \"locale\" ]}",
					 getPreferencesCallback, prefObjPtr, NULL, &error);
		if (!ret) {
			luna_critical(s_logChannel, "Failed in calling palm://com.palm.systemservice/getPreferences: %s",
						  error.message);
			LSErrorFree(&error);
		}

		ret = LSCall(Preferences::instance()->m_lsHandle,
					 "palm://com.palm.systemservice/getPreferences",
					 "{\"subscribe\":true, \"keys\": [ \"ringtone\",\
													   \"alerttone\", \
													   \"notificationtone\", \
													   \"region\", \
													   \"timeFormat\", \
													   \"wallpaper\" , \
													   \"dockwallpaper\" , \
													   \"systemSounds\" , \
													   \"showAlertsWhenLocked\", \
													   \"BlinkNotifications\", \
													   \"imeEnabled\", \
													   \"imeType\", \
													   \"sysUiNoHomeButtonMode\", \
													   \"sysUiEnableNextPrevGestures\", \
													   \"airplaneMode\", \
													   \"hideWANAlert\", \
													   \"roamingIndicator\", \
													   \"dualRSSI\", \
													   \"lockTimeout\",\
													   \"rotationLock\",\
													   \"muteSound\",\
													   \"" PALM_VIRTUAL_KEYBOARD_PREFS "\",\
													   \"" PALM_VIRTUAL_KEYBOARD_SETTINGS "\",\
					 	 	 	 	 	 	 	 	   \"enableVoiceCommand\",\
													   \"enableALS\" ]}",
					 getPreferencesCallback, prefObjPtr, NULL, &error);
		if (!ret) {
			g_critical("%s: Failed in calling palm://com.palm.systemservice/getPreferences: %s",
					   __PRETTY_FUNCTION__, error.message);
			LSErrorFree(&error);
		}
	}

Done:

	if (json && !is_error(json))
		json_object_put(json);

	return true;
}

bool Preferences::getPreferencesCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
	if (!message)
		return true;
	
	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return true;

	json_object* label = 0;
	json_object* root_label = 0;
	json_object* json = 0;
	json_object* value = 0;
	json_object* subobject = 0;

	const char* languageCode = 0;
	const char* countryCode = 0;
	const char* phoneRegion = 0;
	std::string newLocale;
	std::string newLocaleRegion;
	std::string newPhoneRegion;

	const char* imeType = 0;

	Preferences * prefObjPtr = (Preferences *)ctx;
	label = 0;		
	json = json_tokener_parse(payload);
	if (!json || is_error(json))
		goto Done;
		
	g_message("Preferences::getPreferencesCallback(): toString -> [%s]\n", payload);
	
	label = json_object_object_get(json, "imeEnabled");
	if (label && !is_error(label)) {

		if (prefObjPtr)
			prefObjPtr->m_imeEnabled = json_object_get_boolean(label);
	}

	label = json_object_object_get(json, "imeType");
	if (label && !is_error(label)) {

        imeType = json_object_get_string(label);

		if (prefObjPtr)
        {
            if (!strcmp (imeType, "pinyin"))
            {
    			prefObjPtr->m_pinyinEnabled = true;
    			prefObjPtr->m_hwrEnabled = false;
            }
            else if (!strcmp (imeType, "hwr"))
            {
    			prefObjPtr->m_pinyinEnabled = false;
    			prefObjPtr->m_hwrEnabled = true;
            }
            else // Unknown!
            {
    			prefObjPtr->m_pinyinEnabled = false;
    			prefObjPtr->m_hwrEnabled = false;
            }
        }
	}

	value = json_object_object_get(json, "locale");
	if ((value) && (!is_error(value))) {
		
		label = json_object_object_get(value, "languageCode");
		if ((label) && (!is_error(label))) {
			languageCode = json_object_get_string(label);
		}

		label = json_object_object_get(value, "countryCode");
		if ((label) && (!is_error(label))) {
			countryCode = json_object_get_string(label);
		}

		subobject = json_object_object_get(value, "phoneRegion");
		if ((subobject) && (!is_error(subobject))) {
			label = json_object_object_get(subobject, "countryCode");
			if ((label) && (!is_error(label))) {
				newPhoneRegion = json_object_get_string(label);
			}
		}

		newLocale = languageCode;
		newLocale += "_";
		newLocale += countryCode;

		if ((newLocale != prefObjPtr->m_locale && newLocale != "_") ||
			(!newPhoneRegion.empty() && newPhoneRegion != prefObjPtr->m_phoneRegion )) {

			prefObjPtr->m_mutex.lock();
			if (newLocale != "_") {
				prefObjPtr->m_locale = newLocale;
			}
			if (!newPhoneRegion.empty()){
				prefObjPtr->m_phoneRegion =  newPhoneRegion;
			}
			prefObjPtr->m_mutex.unlock();

			// Don't shutdown sysmgr for locale changes in minimal mode
			if (Settings::LunaSettings()->uiType != Settings::UI_MINIMAL) {

				VirtualKeyboardPreferences::instance().localeChanged();

				g_warning("locale changed: %s (%s). shutting down sysmgr",
						  newLocale.empty() ? "" : newLocale.c_str(),
						  prefObjPtr->m_locale.empty() ? "" : prefObjPtr->m_locale.c_str());

				// Locale has changed. SysMgr needs to be restarted
				exit(0);
			}
			else
			{   // first use: set the default keyboard layout based on the new locale
				VirtualKeyboardPreferences::instance().applyFirstUseSettings();

				// reload localization strings to get updated translations of non-cached strings
				Localization::instance()->loadLocalizedStrings();
			}
		}
	}

	value = json_object_object_get(json, "region");
	if (value && !is_error(value)) {

		label = json_object_object_get(value, "countryCode");
		if (label && !is_error(label))
			newLocaleRegion = json_object_get_string(label);

		if (!newLocaleRegion.empty() && newLocaleRegion != prefObjPtr->m_localeRegion) {

			// Don't shutdown sysmgr for locale changes in minimal mode
			if (Settings::LunaSettings()->uiType != Settings::UI_MINIMAL) {
						
				g_warning("region changed: %s (%s). shutting down sysmgr",
										  newLocaleRegion.empty() ? "" : newLocaleRegion.c_str(),
										  prefObjPtr->m_localeRegion.empty() ? "" : prefObjPtr->m_localeRegion.c_str());

				prefObjPtr->m_mutex.lock();
				prefObjPtr->m_localeRegion = newLocaleRegion;
				prefObjPtr->m_mutex.unlock();


			
				// Region has changed. SysMgr needs to be restarted
				exit(0);
			}
		}
	}
	
	root_label = json_object_object_get(json, "ringtone");
	if ((root_label) && (!is_error(root_label))) {
		
		label = json_object_object_get(root_label,"fullPath");
		if ((label) && (!is_error(label))) {
			if (prefObjPtr)
				prefObjPtr->m_currentRingtoneFile = json_object_get_string(label);
		}
	}
	
	root_label = json_object_object_get(json, "alerttone");
	if ((root_label) && (!is_error(root_label))) {

		label = json_object_object_get(root_label,"fullPath");
		if ((label) && (!is_error(label))) {
			if (prefObjPtr)
				prefObjPtr->m_currentAlerttoneFile = json_object_get_string(label);
		}
	}

	root_label = json_object_object_get(json, "notificationtone");
	if ((root_label) && (!is_error(root_label))) {

		label = json_object_object_get(root_label,"fullPath");
		if ((label) && (!is_error(label))) {
			if (prefObjPtr)
				prefObjPtr->m_currentNotificationtoneFile = json_object_get_string(label);
		}
	}
		
	label = json_object_object_get(json, "timeFormat");
	if (label && !is_error(label)) {
		if (prefObjPtr) {
			MutexLocker locker(&prefObjPtr->m_mutex);
			prefObjPtr->m_currentTimeFormat = json_object_get_string(label);
			Q_EMIT prefObjPtr->signalTimeFormatChanged(prefObjPtr->m_currentTimeFormat.c_str());
		}
	}

	label = json_object_object_get(json, "wallpaper");
	if (label && !is_error(label)) {

		label = json_object_object_get(label, "wallpaperFile");
		if (label && !is_error(label)) {
			if (prefObjPtr)
				Q_EMIT prefObjPtr->signalWallPaperChanged(json_object_get_string(label));
		}
	}

	label = json_object_object_get(json, "dockwallpaper");
	if (label && !is_error(label)) {

		label = json_object_object_get(label, "wallpaperFile");
		if (label && !is_error(label)) {
			if (prefObjPtr)
				Q_EMIT prefObjPtr->signalDockModeWallPaperChanged(json_object_get_string(label));
		}
	}

	label = json_object_object_get(json, "showAlertsWhenLocked");
	if (label && !is_error(label)) {
		
		if (prefObjPtr)
			prefObjPtr->m_showAlertsWhenLocked = json_object_get_boolean(label);
	}

	label = json_object_object_get(json, "BlinkNotifications");
	if (label && !is_error(label)) {
		
		if (prefObjPtr)
			prefObjPtr->m_ledThrobberEnabled = json_object_get_boolean(label);
	}

	label = json_object_object_get(json, "systemSounds");
	if (label && !is_error(label)) {

		if (prefObjPtr)
			prefObjPtr->m_playFeedbackSounds = json_object_get_boolean(label);
	}

	label = json_object_object_get(json, "sysUiNoHomeButtonMode");
	if (label && !is_error(label)) {

		if (prefObjPtr)
			prefObjPtr->m_sysUiNoHomeButtonMode = json_object_get_boolean(label);
	}

	label = json_object_object_get(json, "sysUiEnableNextPrevGestures");
	if (label && !is_error(label)) {

		if (prefObjPtr) {
			prefObjPtr->m_sysUiEnableNextPrevGestures = json_object_get_boolean(label);
#ifdef HAVE_QPA
			setAdvancedGestures(prefObjPtr->m_sysUiEnableNextPrevGestures ? 1 : 0);
#endif
		}
	}

	label = json_object_object_get(json, "lockTimeout");
	if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {

		if (prefObjPtr) {
			prefObjPtr->m_lockTimeout = json_object_get_int(label);
			Q_EMIT prefObjPtr->signalSetLockTimeout(prefObjPtr->m_lockTimeout);
		}
	}

	label = json_object_object_get(json, "dualRSSI");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if (prefObjPtr) {
			prefObjPtr->m_dualRSSI = json_object_get_boolean(label);
			if(prefObjPtr->m_dualRSSI) {
				prefObjPtr->m_show3GForEvdo = true;
				Q_EMIT prefObjPtr->signalDualRssiEnabled();
			}
		}
	}

	label = json_object_object_get(json, "hideWANAlert");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if (prefObjPtr) {
			prefObjPtr->m_hideWANAlert = json_object_get_boolean(label);
		}
	}

	label = json_object_object_get(json, "roamingIndicator");
	if (label && !is_error(label)) {
		if (prefObjPtr) {
			prefObjPtr->m_roamingIndicator = json_object_get_string(label);
			Q_EMIT prefObjPtr->signalRoamingIndicatorChanged();
			if(prefObjPtr->m_roamingIndicator == "triangle") {
				prefObjPtr->m_show3GForEvdo = true;
			}
		}
	}

	label = json_object_object_get(json, "airplaneMode");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if (prefObjPtr) {
			MutexLocker locker(&prefObjPtr->m_mutex);
			prefObjPtr->m_airplaneMode = json_object_get_boolean(label);
			Q_EMIT prefObjPtr->signalAirplaneModeChanged(prefObjPtr->m_airplaneMode);
		}
	}

	label = json_object_object_get(json, "wifiRadio");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if (prefObjPtr) {
			MutexLocker locker(&prefObjPtr->m_mutex);
			prefObjPtr->m_wifiOn = json_object_get_boolean(label);
		}
	}

	label = json_object_object_get(json, "bluetoothRadio");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if (prefObjPtr) {
			MutexLocker locker(&prefObjPtr->m_mutex);
			prefObjPtr->m_bluetoothOn = json_object_get_boolean(label);
		}
	}

	label = json_object_object_get(json, "rotationLock");
	if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
		if (prefObjPtr) {
			MutexLocker locker(&prefObjPtr->m_mutex);
            prefObjPtr->m_rotationLock = (OrientationEvent::Orientation)json_object_get_int(label);
			Q_EMIT prefObjPtr->signalRotationLockChanged(prefObjPtr->m_rotationLock);
		}
	}

	label = json_object_object_get(json, "muteSound");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if (prefObjPtr) {
			MutexLocker locker(&prefObjPtr->m_mutex);
			prefObjPtr->m_muteOn = json_object_get_boolean(label);
			Q_EMIT prefObjPtr->signalMuteSoundChanged(prefObjPtr->m_muteOn);
		}
	}

	label = json_object_object_get(json, PALM_VIRTUAL_KEYBOARD_SETTINGS);
	if ((label) && (!is_error(label)))
		VirtualKeyboardPreferences::instance().virtualKeyboardSettingsChanged(json_object_get_string(label));

	label = json_object_object_get(json, PALM_VIRTUAL_KEYBOARD_PREFS);
	if ((label) && (!is_error(label)))
		VirtualKeyboardPreferences::instance().virtualKeyboardPreferencesChanged(json_object_get_string(label));

	label = json_object_object_get(json, "enableVoiceCommand");
	if (label && !is_error(label)) {

		if (prefObjPtr)
		{
			prefObjPtr->m_enableVoiceDial = json_object_get_boolean(label);
			Q_EMIT prefObjPtr->signalVoiceDialSettingChanged(prefObjPtr->m_enableVoiceDial);
		}
	}


	label = json_object_object_get(json, "enableALS");
	if (label && !is_error(label)) {
		if (prefObjPtr) 
		{
			prefObjPtr->m_enableALS = json_object_get_boolean(label);
			Q_EMIT prefObjPtr->signalAlsEnabled (prefObjPtr->m_enableALS);
		}
	}
Done:

	if (json && !is_error(json))
		json_object_put(json);

	return true;
}

bool Preferences::setStringPreference(const char * keyName, const char * value)
{
	pbnjson::JValue pref = pbnjson::Object();
	pref.put(keyName, value);

	LSError error;
	LSErrorInit(&error);

	std::string	valueStr = jsonToString(pref);
	//g_debug("Preferences::setStringPreference: '%s' = '%s'", keyName, valueStr.c_str());

	bool ret = LSCall(Preferences::instance()->m_lsHandle,
				 "palm://com.palm.systemservice/setPreferences", valueStr.c_str(),
				 NULL, NULL, NULL, &error);
	if (!ret) {
		g_warning("%s: Failed setting '%s' to '%s' (%s)",
				   __FUNCTION__, keyName, value, error.message);
		LSErrorFree(&error);
		return false;
	}
	return true;
}
