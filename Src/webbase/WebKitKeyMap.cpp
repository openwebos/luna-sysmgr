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

#include "WebKitKeyMap.h"

#include "qchar.h"

WebKitKeyMap* WebKitKeyMap::m_instance = NULL;

WebKitKeyMap* WebKitKeyMap::instance() {
    if (!m_instance)
	m_instance = new WebKitKeyMap;

    return m_instance;
}


// creating reverse translation table from qt-keys to luna-keys to send to webkit
WebKitKeyMap::WebKitKeyMap()
{
	keyMap [Qt::Key_Escape]		= Key_Escape;
	keyMap [Qt::Key_Tab]		= Key_Tab;
	keyMap [Qt::Key_Backtab]	= Key_Tab;
	keyMap [Qt::Key_Delete]		= Key_Delete;
	keyMap [Qt::Key_Backspace]	= Key_Backspace;
	keyMap [Qt::Key_Return]		= Key_Return;
	keyMap [Qt::Key_Enter]		= Key_Return;
	keyMap [Qt::Key_Insert]		= Key_Insert;
	keyMap [Qt::Key_Delete]		= Key_Delete;
	keyMap [Qt::Key_Pause]		= Key_MediaPause;
	keyMap [Qt::Key_Print]		= Key_Null;
	keyMap [Qt::Key_SysReq]		= Key_Null;
	keyMap [Qt::Key_Clear]		= Key_Null;
	keyMap [Qt::Key_Home]		= Key_Home;
	keyMap [Qt::Key_End]		= Key_End;
	keyMap [Qt::Key_Left]		= Key_Left;
	keyMap [Qt::Key_Up]			= Key_Up;
	keyMap [Qt::Key_Right]		= Key_Right;
	keyMap [Qt::Key_Down]		= Key_Down;
	keyMap [Qt::Key_PageUp]		= Key_PageUp;
	keyMap [Qt::Key_PageDown]	= Key_PageDown;
	keyMap [Qt::Key_Shift]		= Key_Shift;
	keyMap [Qt::Key_Control]	= Key_Ctrl;
	keyMap [Qt::Key_Meta]		= Key_Option; // ???: check
	keyMap [Qt::Key_Alt]		= Key_Alt;
	keyMap [Qt::Key_CapsLock]	= Key_Null;
	keyMap [Qt::Key_NumLock]	= Key_Null;
	keyMap [Qt::Key_ScrollLock]	= Key_Null;

	keyMap [Qt::Key_F1]			= Key_F1;
	keyMap [Qt::Key_F2]			= Key_F2;
	keyMap [Qt::Key_F3]			= Key_F3;
	keyMap [Qt::Key_F4]			= Key_F4;
	keyMap [Qt::Key_F5]			= Key_F5;
	keyMap [Qt::Key_F6]			= Key_F6;
	keyMap [Qt::Key_F7]			= Key_F7;
	keyMap [Qt::Key_F8]			= Key_F8;
	keyMap [Qt::Key_F9]			= Key_F9;
	keyMap [Qt::Key_F10]		= Key_F10;
	keyMap [Qt::Key_F11]		= Key_F11;
	keyMap [Qt::Key_F12]		= Key_F12;

	// this is not right!
//    keyMap [(unsigned int)Qt::Key_currency]	= WebKitKeyInfo (Qt::Key_currency,	Key_EuroSign,	Key_EuroSign);

// PALM -->
    // core navi keys
	keyMap [Qt::Key_Gesture_Key_Range_Start]	= Key_Null;
	keyMap [Qt::Key_CoreNavi_Back]				= Key_CoreNavi_Back;
	keyMap [Qt::Key_CoreNavi_Menu]				= Key_CoreNavi_Menu;
	keyMap [Qt::Key_CoreNavi_QuickLaunch]		= Key_CoreNavi_QuickLaunch;
	keyMap [Qt::Key_CoreNavi_Launcher]			= Key_CoreNavi_Launcher;
	keyMap [Qt::Key_CoreNavi_SwipeDown]			= Key_CoreNavi_Down;
	keyMap [Qt::Key_CoreNavi_Next]				= Key_CoreNavi_Next;
	keyMap [Qt::Key_CoreNavi_Previous]			= Key_CoreNavi_Previous;
	keyMap [Qt::Key_CoreNavi_Home]				= Key_CoreNavi_Home;
	keyMap [Qt::Key_CoreNavi_Meta]				= Key_CoreNavi_Meta;
	keyMap [Qt::Key_Flick]						= Key_Flick;
	keyMap [Qt::Key_Gesture_Key_Range_End]		= Key_Null;

	keyMap [Qt::Key_Slider]						=Key_Slider;
	keyMap [Qt::Key_Optical]					=Key_Optical;
	keyMap [Qt::Key_Ringer]						=Key_Ringer;
	keyMap [Qt::Key_Power]						=Key_HardPower;
	keyMap [Qt::Key_HeadsetButton]				=Key_HeadsetButton;
	keyMap [Qt::Key_Headset]					=Key_Headset;
	keyMap [Qt::Key_HeadsetMic]					=Key_HeadsetMic;
// <-- PALM

}

unsigned short WebKitKeyMap::translateKey (int qtKey)
{
	const std::map<int, unsigned short>::const_iterator it = keyMap.find ((unsigned int)qtKey);

	if (it != keyMap.end())
		return (*it).second;

	if (qtKey >= Qt::Key_Space && qtKey < 0xE000)
		return (unsigned short) qtKey;

	return Key_Null;
}
