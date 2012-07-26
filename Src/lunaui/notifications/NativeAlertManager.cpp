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

#include "NativeAlertManager.h"

#include <list>
#include <map>
#include <string>
#include <glib.h>
#include <cjson/json.h>

#include "DashboardWindowManager.h"
#include "HostBase.h"
#include "Logging.h"
#include "Utils.h"
#include "WindowServer.h"

/*
// Names of the sub-scenario
#define SCENARIO_BACK_SPEAKER	"back_speaker"
#define SCENARIO_FRONT_SPEAKER	"front_speaker"
#define SCENARIO_HEADSET		"headset"
#define SCENARIO_HEADSET_MIC	"headset_mic"
#define SCENARIO_A2DP			"a2dp"
#define SCENARIO_WIRELESS		"wireless"
#define SCENARIO_BLUETOOTH_SCO	"bluetooth_sco"
#define SCENARIO_TTY_FULL		"tty_full"
#define SCENARIO_TTY_HCO		"tty_hco"
#define SCENARIO_TTY_VCO		"tty_vco"
 */

static const char * phone_scenarios[] =
{ "phone_front_speaker" , "phone_back_speaker" , "phone_headset" , "phone_headset_mic" , "phone_bluetooth_sco"};
//static 
bool NativeAlertManager::isPhoneScenario(const std::string& scenarioName)
{
	for (unsigned int i=0;i<sizeof(phone_scenarios)/sizeof(char *);++i)
	{
		if (scenarioName == phone_scenarios[i])
			return true;
	}
	return false;
}

static const char * media_scenarios[] =
{ "media_back_speaker" , "media_a2dp" , "media_headset" , "media_headset_mic" };
//static 
bool NativeAlertManager::isMediaScenario(const std::string& scenarioName)
{
	for (unsigned int i=0;i<sizeof(media_scenarios)/sizeof(char *);++i)
	{
		if (scenarioName == media_scenarios[i])
			return true;
	}
	return false;
}

static const char * ringtone_scenarios[] =
{ "ringtone_default" };
//static 
bool NativeAlertManager::isRingtoneScenario(const std::string& scenarioName)
{
	for (unsigned int i=0;i<sizeof(ringtone_scenarios)/sizeof(char *);++i)
	{
		if (scenarioName == ringtone_scenarios[i])
			return true;
	}
	return false;
}

static const char * system_scenarios[] =
{ "system_default" };
//static
bool NativeAlertManager::isSystemScenario(const std::string& scenarioName)
{
	for (unsigned int i=0;i<sizeof(system_scenarios)/sizeof(char *);++i)
	{
		if (scenarioName == system_scenarios[i])
			return true;
	}
	return false;
}


static const char * events_volume[] = 
{ "event" , "minVolume" , "maxVolume" ,"currentlyMuted" };
//static
bool NativeAlertManager::isVolumeEvent(const std::string& eventName)
{
	for (unsigned int i=0;i<sizeof(events_volume)/sizeof(char *);++i)
	{
		if (eventName == events_volume[i])
			return true;
	}
	return false;
}

static const char * changed_events[] =
{ "volume" , "ringer switch" };
//static 
bool NativeAlertManager::isVolumeRelatedChangedEvent(const std::string& changedEventName)
{
	// Yeah, I know the "changed" naming is a little funky, but that's what audiod announces
	for (unsigned int i=0;i<sizeof(changed_events)/sizeof(char *);++i)
	{
		if (changedEventName == changed_events[i])
			return true;
	}
	return false;
}

static const char * ignore_actions[] =
{ "requested" };
//static
bool NativeAlertManager::shouldIgnoreAction(const std::string& actionName)
{
	// Yeah, I know the "changed" naming is a little funky, but that's what audiod announces
	for (unsigned int i=0;i<sizeof(ignore_actions)/sizeof(char *);++i)
	{
		if (actionName == ignore_actions[i])
			return true;
	}
	return false;
}

NativeAlertManager * NativeAlertManager::s_inst = NULL;
//static 
NativeAlertManager* NativeAlertManager::instance()
{
	if (s_inst == NULL)
	{
		s_inst = new NativeAlertManager();
		s_inst->init();
	}
	
	return s_inst;
}

bool NativeAlertManager::init()
{
	bool result;
	int seq=0;
	LSError lsError;
	LSErrorInit(&lsError);

	GMainLoop *mainLoop = HostBase::instance()->mainLoop();
	result = LSRegister("com.palm.nativealertmanager", &m_service, &lsError);
	if (!result)
		goto Done_init;
	else
		seq=1;
	
	result = LSGmainAttach(m_service, mainLoop, &lsError);
	if (!result)
		goto Done_init;
	else
		seq=2;
	
	// register for audiod status
	result = LSCall(m_service,"palm://com.palm.bus/signal/registerServerStatus",
	    		"{\"serviceName\":\"com.palm.audio\", \"subscribe\":true}",
	    		cbAudioDServiceState,this,NULL, &lsError);
	   
	if (!result)
		goto Done_init;
	else
		seq=3;
	
Done_init:

	if (seq)
		return false;			//TODO: better cleanup...for now, the single caller, instance() will return NULL, which should signal a need to exit() to the caller of instance()
								// (don't want to hardcode that decision way down here though)
	return true;
}

//static 
bool NativeAlertManager::cbAudioControlsChanged(LSHandle* lshandle, LSMessage *msg,void *user_data)
{
	NativeAlertManager * inst = instance();
	if (inst == NULL)
		return false;
		
	// {"returnValue":true,"action":"changed","changed":["active"],"scenario":"phone_front_speaker","volume":56,"active":false,"ringer switch":true,"muted":false,"slider":true}
	
	LSError lsError;
	LSErrorInit(&lsError);

	json_object * root = 0;
	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;

	root = json_tokener_parse(str);
	if ((root == NULL) || (is_error(root))) {
		root = NULL;
		return true;
	}

	std::string scenarioName;
	bool gotVolume=false;
	int volume = -1;
	bool gotRingerSwitch=false;
	bool ringerSwitch=false;
	
	//check the action...if it's ignore-able, then just exit
	std::string action;
	extractFromJson(root,"action",action);
	if (shouldIgnoreAction(action))
		return true;
	
	extractFromJson(root,"scenario",scenarioName);
	json_object * label = JsonGetObject(root,"volume");
	if (label != NULL)
	{
		volume = json_object_get_int(label);
		gotVolume = true;
	}
	label = JsonGetObject(root,"ringer switch");
	if (label != NULL)
	{
		ringerSwitch = json_object_get_boolean(label);
		gotRingerSwitch = true;
	}
	
	//check the event type(s), and gate calling the changed function so that the dialog doesn't pop up pointlessly for stuff like opening the slider
	std::string eventName;
	label = JsonGetObject(root,"event");
	if (label)
	{
		str = json_object_get_string(label);
		if (str)
			eventName = std::string(str);
		else
			eventName = "";
	}
	
	label = JsonGetObject(root,"changed");
	if (label)
	{
		if (json_object_is_type(label,json_type_array))
		{
			bool hasVolumeChange = false;
			bool hasScenarioChange = false;
			
			for (int listIdx=0;listIdx<json_object_array_length(label);++listIdx) {
				json_object * changedJobj = json_object_array_get_idx(label,listIdx);
				if ((changedJobj == NULL) || (is_error(changedJobj)))
					continue;
				str = json_object_get_string(changedJobj);
				if (!str)
					continue;

				std::string s(str);
				if (isVolumeRelatedChangedEvent(s))
				{
					hasVolumeChange = true;
				}
				if ((isVolumeEvent(s)) && (isVolumeEvent(eventName)))
				{
					hasVolumeChange = true;
				}
				if (s == "scenario")
				{
					hasScenarioChange = true;
				}				
			}

			if (hasVolumeChange && !hasScenarioChange) {
				inst->actOnChanged(scenarioName,volume,ringerSwitch);	
			}
		}
	}

	json_object_put(root);
	
	return true;
}

//static 
bool NativeAlertManager::cbAudioDServiceState(LSHandle* lshandle, LSMessage *message,void *user_data)
{
	
	LSError lsError;
	LSErrorInit(&lsError);
	
	json_object * root = 0;
	const char* str = LSMessageGetPayload(message);
	if( !str )
		return false;
	
	root = json_tokener_parse(str);
	if ((root == NULL) || (is_error(root))) {
		root = NULL;
		return true;
	}

	NativeAlertManager * inst = instance();
	if (inst == NULL) {
		json_object_put(root);
		return false;
	}
	
	json_object * label = JsonGetObject(root,"connected");
	if (label != NULL)
	{
		if (json_object_get_boolean(label) == true) 
		{
			//the audiod service is connected...
			if (LSCall(inst->m_service,"palm://com.palm.audio/media/status",
					"{\"subscribe\":true}",
					NativeAlertManager::cbAudioControlsChanged,NULL,NULL, &lsError) == false) 
			{
				g_warning("%s call to audio/media/status failed",__FUNCTION__);
				LSErrorFree(&lsError);
				goto Done_cbAudioDServiceState;
			} 
			if (LSCall(inst->m_service,"palm://com.palm.audio/phone/status",
					"{\"subscribe\":true}",
					NativeAlertManager::cbAudioControlsChanged,NULL,NULL, &lsError) == false) 
			{
				g_warning("%s call to audio/phone/status failed",__FUNCTION__);
				LSErrorFree(&lsError);
				goto Done_cbAudioDServiceState;
			} 
			if (LSCall(inst->m_service,"palm://com.palm.audio/ringtone/status",
					"{\"subscribe\":true}",
					NativeAlertManager::cbAudioControlsChanged,NULL,NULL, &lsError) == false) 
			{
				g_warning("%s call to audio/ringtone/status failed",__FUNCTION__);
				LSErrorFree(&lsError);
				goto Done_cbAudioDServiceState;
			} 
			if (LSCall(inst->m_service,"palm://com.palm.audio/system/status",
					"{\"subscribe\":true}",
					NativeAlertManager::cbAudioControlsChanged,NULL,NULL, &lsError) == false)
			{
				g_warning("%s call to audio/system/status failed",__FUNCTION__);
				LSErrorFree(&lsError);
				goto Done_cbAudioDServiceState;
			}
		}
	}
	else 
	{
		g_warning("%s called with a message that didn't include 'connected' field; message follows: %s",__FUNCTION__,str);
	}
	
Done_cbAudioDServiceState:

	json_object_put(root);	
	return true;
}

void NativeAlertManager::actOnChanged(const std::string& scenarioName,int volume,bool ringerSwitch)
{
	//TODO: probably could be optimized a bit...is___Scenario functions shouldn't be called more than once

	int alertWidth = Settings::LunaSettings()->tabletUi ?
					 DashboardWindowManager::sTabletUiWidth() :
					 HostBase::instance()->getInfo().displayWidth;
	
	if ((ringerSwitch == false) && (isRingtoneScenario(scenarioName) || isSystemScenario(scenarioName)))
	{
		if (!(m_win))
		{
			m_win = 
				new VolumeControlAlertWindow(alertWidth,48,true,VolumeControlAlertWindow::VolumeControl_MuteBell,volume);
			WindowServer::instance()->addWindow(m_win);
		}
		else
		{
			m_win->setDrawType(VolumeControlAlertWindow::VolumeControl_MuteBell);
		}
	}
	else if (isPhoneScenario(scenarioName))
	{
		if (!(m_win))
		{
			m_win = 
				new VolumeControlAlertWindow(alertWidth,48,true,VolumeControlAlertWindow::VolumeControl_Phone,volume);
			WindowServer::instance()->addWindow(m_win);
		}
		else
		{
			m_win->setDrawType(VolumeControlAlertWindow::VolumeControl_Phone);
			m_win->setVolumeLevel(volume);
		}
	}
	else if (isRingtoneScenario(scenarioName))
	{
		if (!(m_win))
		{
			m_win = 
				new VolumeControlAlertWindow(alertWidth,48,true,VolumeControlAlertWindow::VolumeControl_Ringtone,volume);
			WindowServer::instance()->addWindow(m_win);
		}
		else
		{
			m_win->setDrawType(VolumeControlAlertWindow::VolumeControl_Ringtone);
			m_win->setVolumeLevel(volume);
		}
	}
	else if (isSystemScenario(scenarioName))
	{
		if (!(m_win))
		{
			m_win =
				new VolumeControlAlertWindow(alertWidth,48,true,VolumeControlAlertWindow::VolumeControl_Ringtone,volume);
			WindowServer::instance()->addWindow(m_win);
		}
		else
		{
			m_win->setDrawType(VolumeControlAlertWindow::VolumeControl_Ringtone);
			m_win->setVolumeLevel(volume);
		}
	}
	else if (isMediaScenario(scenarioName))
	{
		if (!(m_win))
		{
			m_win = 
				new VolumeControlAlertWindow(alertWidth,48,true,VolumeControlAlertWindow::VolumeControl_Media,volume);
			WindowServer::instance()->addWindow(m_win);
		}
		else
		{
			m_win->setDrawType(VolumeControlAlertWindow::VolumeControl_Media);
			m_win->setVolumeLevel(volume);
		}
	}

	if (!m_pVCAWindowTimer)
		m_pVCAWindowTimer = new Timer<NativeAlertManager>(HostBase::instance()->masterTimer(), this, &NativeAlertManager::volumeControlAlertWindowTimer);
	
	if (m_win)
		m_pVCAWindowTimer->start(3000,true);
}

bool NativeAlertManager::volumeControlAlertWindowTimer()
{
	if (m_win)
	{
		WindowServer::instance()->removeWindow(m_win);			//DashboardWindowManager will actually delete win object. If this ever changes, it has to be dealt with here
																// WARNING: close() in VolumeControlAlertWindow does a remove as well, which will delete m_win from underneath me here
	}
	m_win = 0;
	return true;
}

NativeAlertManager::NativeAlertManager()
: m_service(0)
, m_win(0)
, m_pVCAWindowTimer(0)
{
}

NativeAlertManager::~NativeAlertManager()
{
}

void NativeAlertManager::closeWindow(Window* win)
{
	if (G_UNLIKELY(win == 0))
		return;
	
	if (G_LIKELY(m_win == win))
		m_win = 0;

	WindowServer::instance()->removeWindow(win);
}


////---------------------------------------------- DEBUG ---------------------------------------------------------------

/*
#define clamp(x,low,high) 		((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))

void NativeAlertManager::dbg_common()
{
	if (m_lastRingerSwitchSetting == -1)
		m_lastRingerSwitchSetting = 1;
	if (m_lastPhoneVolume < 0)
		m_lastPhoneVolume = 0;
	if (m_lastMediaVolume < 0)
		m_lastMediaVolume = 0;
	if (m_lastRingtoneVolume < 0)
		m_lastRingtoneVolume = 0;
	
	m_dbgCurrentScenario = "phone_front_speaker";
	m_dbgVolume = 0;
	if (!m_pVCAWindowTimer)
		m_pVCAWindowTimer = new Timer<NativeAlertManager>(HostBase::instance()->masterTimer(), this, &NativeAlertManager::volumeControlAlertWindowTimer);
}

void NativeAlertManager::dbg_volumeUp()
{
	if (m_lastRingerSwitchSetting == -1)
		dbg_common();
	
	m_dbgVolume = clamp(m_dbgVolume+10,0,100);
	actOnChanged(m_dbgCurrentScenario,m_dbgVolume,m_lastRingerSwitchSetting);
}

void NativeAlertManager::dbg_volumeDown()
{
	if (m_lastRingerSwitchSetting == -1)
		dbg_common();
	
	m_dbgVolume = clamp(m_dbgVolume-10,0,100);
	actOnChanged(m_dbgCurrentScenario,m_dbgVolume,m_lastRingerSwitchSetting);
		
}
void NativeAlertManager::dbg_ringerOn()
{
	if (m_lastRingerSwitchSetting == -1)
		dbg_common();
	m_lastRingerSwitchSetting = 1;
	actOnChanged(m_dbgCurrentScenario,m_dbgVolume,m_lastRingerSwitchSetting);
}
void NativeAlertManager::dbg_ringerOff()
{
	if (m_lastRingerSwitchSetting == -1)
		dbg_common();
	
	m_lastRingerSwitchSetting = 0;
	actOnChanged(m_dbgCurrentScenario,m_dbgVolume,m_lastRingerSwitchSetting);
}
void NativeAlertManager::dbg_phoneMode()
{
	if (m_lastRingerSwitchSetting == -1)
		dbg_common();
	m_dbgCurrentScenario = "phone_front_speaker";
	actOnChanged(m_dbgCurrentScenario,m_dbgVolume,m_lastRingerSwitchSetting);
}
void NativeAlertManager::dbg_ringerMode()
{
	if (m_lastRingerSwitchSetting == -1)
		dbg_common();
	m_dbgCurrentScenario = "ringtone_default";
	actOnChanged(m_dbgCurrentScenario,m_dbgVolume,m_lastRingerSwitchSetting);
}
void NativeAlertManager::dbg_mediaMode()
{
	if (m_lastRingerSwitchSetting == -1)
		dbg_common();
	m_dbgCurrentScenario = "media_back_speaker";
	actOnChanged(m_dbgCurrentScenario,m_dbgVolume,m_lastRingerSwitchSetting);
}
*/
