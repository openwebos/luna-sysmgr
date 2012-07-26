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




#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "Common.h"

#include <string>
#include <map>

#include <lunaservice.h>

#include <Qt>
#include <QEvent>

#include "Timer.h"

QT_BEGIN_NAMESPACE
class QKeyEvent;
QT_END_NAMESPACE

struct json_object;

class InputManager
{

public:
	InputManager();
	virtual ~InputManager();
	
	virtual bool handleEvent(QEvent* event);
	
	static bool audioKeyServiceCallback(LSHandle* handle, LSMessage* msg, void* userData);
	static bool mediaKeyServiceCallback(LSHandle* handle, LSMessage* msg, void* userData);
	static bool switchesStatusCallback(LSHandle* handle, LSMessage* msg, void* userData);
#if 0
	static bool switchesRingerCallback(LSHandle* handle, LSMessage* msg, void* userData);
	static bool switchesSliderCallback(LSHandle* handle, LSMessage* msg, void* userData);
#endif
	static bool headsetStatusCallback(LSHandle* handle, LSMessage* msg, void* userData);

private:
	void startService();
	void stopService();

	bool isSwitch(Qt::Key key);
	bool isHeadsetKey(Qt::Key key);
	bool isAudioKey(Qt::Key key);
	bool isMediaKey(Qt::Key key);
    bool isBluetoothKey(Qt::Key key);
	bool isPublicCategory(const char* category);

	static bool activityStartCallback(LSHandle* handle, LSMessage* msg, void* userData);

	json_object* createKeyJson(const char* key, const char* value);
	json_object* createKeyJson(const char* key, QEvent::Type type);

	void postKeyToSubscribers(QKeyEvent* event, const char* category, const char* keyString, const char* keyValue = NULL);
	static bool processSubscription(LSHandle* handle, LSMessage* msg, void* userData);
	bool processKeyState(LSHandle* handle, LSMessage* msg, void* userData);

    void handleBluetoothKey(const QKeyEvent* event);

	QEvent::Type getKeyState(Qt::Key key);
	bool setKeyState(Qt::Key key, QEvent::Type state);
	bool keyToString(Qt::Key key, const char** string);
	Qt::Key stringToKey(const char* string);
	
	LSPalmService* m_palmService;
	LSHandle* m_service;           // private bus
	LSHandle* m_publicService;     // public bus

	// Switches
	QEvent::Type m_ringerState;
	QEvent::Type m_sliderState;

	enum HeadsetType {
		HeadsetInvalid = -1,
		Headset,
		HeadsetMic
	};

	// Headset and headset with mic states
	QEvent::Type m_headsetState;
	HeadsetType m_headsetType;
	
	enum HeadsetButtonState {
		Start,
		SinglePressOrHold,
		Hold,
		PotentialDoublePress,
		DoublePressOrHold,
	};

	void headsetStateMachine(QEvent* e);
	bool headsetBtnTimerCallback(void);
	HeadsetButtonState m_headsetBtnState;
	Timer<InputManager> m_headsetBtnTimer;

	class CategoryMapEntry {
		public:
			CategoryMapEntry()
			{
				m_isPublic = false;	
			}

			CategoryMapEntry(const std::string& category, bool isPublic)
			{
				m_category = category;
				m_isPublic = isPublic;
			}

			bool isPublic(void)
			{
				return m_isPublic;
			}

		private:
			std::string m_category;
			bool m_isPublic;
	};

	std::map<std::string, CategoryMapEntry*> m_categoryMap;
};


#endif /* INPUTMANAGER_H */
