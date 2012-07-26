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

#include <stdlib.h>
#include <sys/types.h>

#include "HidLib.h"
#include "HostArm.h"


class HostArmZepfloyd : public HostArm
{
public:
	HostArmZepfloyd();
	virtual ~HostArmZepfloyd();
	
	Event::Key lookupKeyCode(uint16_t keyType, uint16_t keyCode, int32_t keyValue);
	
	virtual unsigned short translateKeyWithMeta( unsigned short key, bool withShift, bool withAlt );

	virtual const char* hardwareName() const;
	
private:
	void setupInput(void);
	void shutdownInput(void);
	
	static const int m_kMinX = 280;
	static const int m_kMaxX = 3780;
	static const int m_kMinY = 368;
	static const int m_kMaxY = 3780;

	static const KeyMapType m_keyMap[KEY_SPACE+2];

	static const int m_keyAlt = 0xF6;
	static const int m_keyOpt = 0x64;
	static const int m_keyHardPhone = 0xE7;
	static const int m_keyHardPower = 0x6B;
	static const int m_keyHardHome = 0x3B;
	static const int m_keyHardOk = 0x3C;
	static const int m_keyHardCalendar = 0x3D;
	static const int m_keyHardMail = 0x3E;
	static const int m_keySoftLeft = 0xF5;
	static const int m_keySoftRight = 0x8B;
	static const int m_keyRockerUp = 0x67;
	static const int m_keyRockerDown = 0x6C;
	static const int m_keyRockerLeft = 0x69;
	static const int m_keyRockerRight = 0x6A;
	static const int m_keyRockerCenter = 0xE8;
};

const KeyMapType HostArmZepfloyd::m_keyMap[] =
{
	{ 0, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_RESERVED	0
	{ 1, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_ESC		1
	{ 2, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_1			2
	{ 3, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_2			3
	{ 4, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_3			4
	{ 5, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_4			5
	{ 6, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_5			6
	{ 7, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_6			7
	{ 8, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_7			8
	{ 9, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_8			9
	{ 10, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_9			10
	{ 11, Event::Key_0, 		Event::Key_0, 			Event::Key_0 }, // KEY_0			11
	{ 12, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_MINUS		12
	{ 13, Event::Key_Null,      Event::Key_Null, 		Event::Key_Null }, // KEY_EQUAL		13
	{ 14, Event::Key_Backspace, Event::Key_Backspace,	Event::Key_Backspace }, // KEY_BACKSPACE		14
	{ 15, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_TAB			15
	{ 16, Event::Key_q, 		Event::Key_Q, 			Event::Key_Slash }, // KEY_Q			16
	{ 17, Event::Key_w, 		Event::Key_W, 			Event::Key_Plus }, // KEY_W			17
	{ 18, Event::Key_e, 		Event::Key_E, 			Event::Key_1 }, // KEY_E			18
	{ 19, Event::Key_r,  		Event::Key_R, 			Event::Key_2 }, // KEY_R			19
	{ 20, Event::Key_t,  		Event::Key_T, 			Event::Key_3 }, // KEY_T			20
	{ 21, Event::Key_y,  		Event::Key_Y, 			Event::Key_LeftParen }, // KEY_Y			21
	{ 22, Event::Key_u,  		Event::Key_U, 			Event::Key_RightParen }, // KEY_U			22
	{ 23, Event::Key_i,  		Event::Key_I, 			Event::Key_At }, // KEY_I			23
	{ 24, Event::Key_o,  		Event::Key_O, 			Event::Key_DoubleQuote }, // KEY_O			24
	{ 25, Event::Key_p,  		Event::Key_P, 			Event::Key_Brightness }, // KEY_P			25
	{ 26, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_LEFTBRACE		26
	{ 27, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_RIGHTBRACE		27
	{ 28, Event::Key_Return,  	Event::Key_Return, 		Event::Key_Return }, // KEY_ENTER		28
	{ 29, Event::Key_Null,  	Event::Key_Null, 		Event::Key_Null }, // KEY_LEFTCTRL		29
	{ 30, Event::Key_a, 		Event::Key_A, 			Event::Key_Ampersand }, // KEY_A			30
	{ 31, Event::Key_s, 		Event::Key_S, 			Event::Key_Minus }, // KEY_S			31
	{ 32, Event::Key_d, 		Event::Key_D, 			Event::Key_4 }, // KEY_D			32
	{ 33, Event::Key_f, 		Event::Key_F, 			Event::Key_5 }, // KEY_F			33
	{ 34, Event::Key_g, 		Event::Key_G, 			Event::Key_6 }, // KEY_G			34
	{ 35, Event::Key_h, 		Event::Key_H, 			Event::Key_DollarSign }, // KEY_H			35
	{ 36, Event::Key_j, 		Event::Key_J, 			Event::Key_Exclamation }, // KEY_J			36
	{ 37, Event::Key_k, 		Event::Key_K, 			Event::Key_SemiColon }, // KEY_K			37
	{ 38, Event::Key_l, 		Event::Key_L, 			Event::Key_SingleQuote }, // KEY_L			38
	{ 39, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_SEMICOLON		39
	{ 40, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_APOSTROPHE		40
	{ 41, Event::Key_Null, 		Event::Key_Null, 		Event::Key_Null }, // KEY_GRAVE		41
	{ 42, Event::Key_Shift,		Event::Key_Shift, 		Event::Key_Shift }, // KEY_LEFTSHIFT		42
	{ 43, Event::Key_Null,  	Event::Key_Null, 		Event::Key_Null }, // KEY_BACKSLASH		43
	{ 44, Event::Key_z,  		Event::Key_Z, 			Event::Key_Asterik }, // KEY_Z			44
	{ 45, Event::Key_x,  		Event::Key_X, 			Event::Key_7 }, // KEY_X			45
	{ 46, Event::Key_c,  		Event::Key_C, 			Event::Key_8 }, // KEY_C			46
	{ 47, Event::Key_v,  		Event::Key_V, 			Event::Key_9 }, // KEY_V			47
	{ 48, Event::Key_b,         Event::Key_B, 			Event::Key_PoundSign }, // KEY_B			48
	{ 49, Event::Key_n,         Event::Key_N, 			Event::Key_Question }, // KEY_N			49
	{ 50, Event::Key_m,         Event::Key_M, 			Event::Key_Comma }, // KEY_M			50
	{ 51, Event::Key_Null,      Event::Key_Null, 		Event::Key_Null }, // KEY_COMMA		51
	{ 52, Event::Key_Period,    Event::Key_Period, 		Event::Key_Period }, // KEY_DOT			52
	{ 53, Event::Key_Null,  	Event::Key_Null, 		Event::Key_Null }, // KEY_SLASH		53
	{ 54, Event::Key_Null,  	Event::Key_Null, 		Event::Key_Null }, // KEY_RIGHTSHIFT		54
	{ 55, Event::Key_Null,  	Event::Key_Null, 		Event::Key_Null }, // KEY_KPASTERISK		55
	{ 56, Event::Key_Null,  	Event::Key_Null, 		Event::Key_Null }, // KEY_LEFTALT		56
	{ 57, Event::Key_Space, 	Event::Key_Space, 		Event::Key_Space }, // KEY_SPACE		57
	
	{ LAST_KEY, Event::Key_Null, Event::Key_Null, Event::Key_Null }
};

HostArmZepfloyd::HostArmZepfloyd()
{
}

HostArmZepfloyd::~HostArmZepfloyd()
{
	shutdownInput();
}

unsigned short HostArmZepfloyd::translateKeyWithMeta( unsigned short key, bool withShift, bool withAlt )
{
	int i=0;
	while( (int)LAST_KEY != m_keyMap[i].devicekey )
	{
		if( m_keyMap[i].normal == key )
		{
			if( withShift ) 
				return m_keyMap[i].shift;
			else
				return m_keyMap[i].opt; 
		}
		i++;
	}
	
	return key;
}


Event::Key HostArmZepfloyd::lookupKeyCode(uint16_t keyType, uint16_t keyCode, int32_t keyValue)
{
	if (keyType != EV_KEY) {
		return Event::Key_Invalid;
	}

	// Check for shift, Option or Alt Key Down
	switch (keyCode) {
	case KEY_LEFTSHIFT:
		m_shiftKeyDown = (keyValue != 0);
		break;
	case m_keyAlt:
		m_altKeyDown = (keyValue != 0);
		break;
	case m_keyOpt:
		m_optKeyDown = (keyValue != 0);
		break;
	}

	if (keyCode > 0 && keyCode <= KEY_SPACE) {
		if (m_shiftKeyDown)
			return (Event::Key) m_keyMap[keyCode].shift;
		else if (m_optKeyDown)
			return (Event::Key) m_keyMap[keyCode].opt;
		else
			return (Event::Key) m_keyMap[keyCode].normal;
	} else {
		switch (keyCode) {
		case (m_keyAlt):
			return Event::Key_Alt;
		case (m_keyOpt):
			return Event::Key_Option;
		case (m_keyHardPhone):
			return Event::Key_HardPhone;
		case (m_keyHardPower):
			return Event::Key_HardPower;
		case (m_keyHardHome):
			return Event::Key_CoreNavi_Launcher;
		case (m_keyHardOk):
			return Event::Key_HardOk;
		case (m_keyHardCalendar):
			return Event::Key_HardCalendar;
//		case (m_keyHardMail):
//			return Event::key_CoreNavi_QuickLaunch;
		case (m_keySoftLeft):
			return Event::Key_CoreNavi_Back;
		case (m_keySoftRight):
			return Event::Key_CoreNavi_Menu;
		case (m_keyRockerLeft):
			return Event::Key_CoreNavi_Previous;
		case (m_keyRockerRight):
			return Event::Key_CoreNavi_Next;
		case (m_keyRockerUp):
			return Event::Key_Up;
		case (m_keyRockerDown):
			return Event::Key_Down;
		case (m_keyRockerCenter):
			return Event::Key_CoreNavi_Home;
		default:
			return Event::Key_Invalid;
		}
	}
	return Event::Key_Invalid;
}

void HostArmZepfloyd::setupInput(void)
{
	// Pen/Key IO Channels
	HidPluginSettings_t* pluginSettings = NULL;
	int numPlugins = 0;

	if (kPmErrorNone != HidAllocPluginSettings(HID_DEFAULT_XML_FILE, &pluginSettings, &numPlugins)) {
		luna_critical(HOSTARM_LOG, "HidAllocPluginSettings failed");
		return;
	}

	if (kPmErrorNone != HidInitPluginTransport(HID_KEYPAD, pluginSettings, numPlugins, &m_hidKeyHandle)) {
		luna_critical(HOSTARM_LOG, "HidInitPluginTransport: Unable to initialize keypad");
	} else {
		int keyFd = HidHandleGetFd(m_hidKeyHandle);
		m_keyIoChannel = g_io_channel_unix_new(keyFd);
		m_keyIoSource = g_io_create_watch(m_keyIoChannel, G_IO_IN);
		g_source_set_callback(m_keyIoSource, (GSourceFunc) keyEventCallbackWrapper, this, NULL);
		g_source_attach(m_keyIoSource, m_mainCtxt);
	}
	
	if (kPmErrorNone != HidInitPluginTransport(HID_TOUCHPANEL, pluginSettings, numPlugins, &m_hidPenHandle)) {
		luna_critical(HOSTARM_LOG, "HidInitPluginTransport: Unable to initialize touchpanel");
	} else {
		int penFd = HidHandleGetFd(m_hidPenHandle);
		m_penIoChannel = g_io_channel_unix_new(penFd);
		m_penIoSource = g_io_create_watch(m_penIoChannel, G_IO_IN);
		g_source_set_callback(m_penIoSource, (GSourceFunc) penEventCallbackWrapper, this, NULL);
		g_source_attach(m_penIoSource, m_mainCtxt);
	}

	HidFreePluginSettings(&pluginSettings, numPlugins);
}

void HostArmZepfloyd::shutdownInput(void)
{
	if (m_hidKeyHandle) HidDestroyPluginTransport(&m_hidKeyHandle);
	if (m_hidPenHandle) HidDestroyPluginTransport(&m_hidPenHandle);
}

const char* HostArmZepfloyd::hardwareName() const
{
    return "ZepFloyd";
}
