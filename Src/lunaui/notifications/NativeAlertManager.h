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




#ifndef NATIVEALERTMANAGER_H
#define NATIVEALERTMANAGER_H

#include "Common.h"

#include <lunaservice.h>
#include <string>
#include "VolumeControlAlertWindow.h"
#include "Timer.h"

class NativeAlertManager
{
public:

	static NativeAlertManager* instance();
	~NativeAlertManager();

	bool init();

	static bool cbAudioControlsChanged(LSHandle* lshandle, LSMessage *msg,void *user_data);
	static bool cbAudioDServiceState(LSHandle* lshandle, LSMessage *message,void *user_data);
	
	void actOnChanged(const std::string& scenarioName,int volume,bool ringer);
	bool volumeControlAlertWindowTimer();

	void closeWindow(Window* win);

	/*
	void dbg_common();
	void dbg_volumeUp();
	void dbg_volumeDown();
	void dbg_ringerOn();
	void dbg_ringerOff();
	void dbg_phoneMode();
	void dbg_ringerMode();
	void dbg_mediaMode();
	*/
	
private:

	NativeAlertManager();
	static NativeAlertManager * s_inst;
	
	static bool isPhoneScenario(const std::string& scenarioName);
	static bool isMediaScenario(const std::string& scenarioName);
	static bool isRingtoneScenario(const std::string& scenarioName);
	static bool isSystemScenario(const std::string& scenarioName);
	static bool isVolumeEvent(const std::string& eventName);
	static bool isVolumeRelatedChangedEvent(const std::string& changedEventName);
	static bool shouldIgnoreAction(const std::string& actionName);
	
private:

	LSHandle* m_service;
	
	VolumeControlAlertWindow * m_win;
	// timer for removing the VCAWin
	Timer<NativeAlertManager> * m_pVCAWindowTimer;
	
};

#endif
