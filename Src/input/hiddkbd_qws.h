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



#ifndef QHIDDKBD_QWS_H
#define QHIDDKBD_QWS_H

#include "Common.h"

#include <QWSKeyboardHandler>
#include <QWSMouseHandler>
#include <qsocketnotifier.h>
#include <map>

#include <nyx/nyx_client.h>

#define MAX_HIDD_EVENTS 100
#define CENTER_BUTTON 	232
#define EV_GESTURE 0x06

// Same as default kernel values
#define REPEAT_DELAY	250
#define REPEAT_PERIOD	33

#define DEV_TYPE_KEYBOARD          0x00
#define DEV_TYPE_MOUSE             0x01
#define DEV_TYPE_JOYSTICK          0x02


typedef enum
{
    BACK = 0,
    MENU,
    QUICK_LAUNCH,
    LAUNCHER,
    NEXT,
    PREV,
    FLICK,
    DOWN,
    HOME,
    HOME_LEFT,
    HOME_RIGHT,
    NUM_GESTURES
} GestureType_t;

enum {
  F1    = 0x276C,         /* Function keys */
  F2    = 0x276D,
  F3    = 0x276E,
  F4    = 0x276F,
  F5    = 0x2770,
  F6    = 0x2771,
  F7    = 0x2772,
  F8    = 0x2773,
  F9    = 0x2774,
  F10   = 0x2775,
  KEY_SYM    = 0xf6,
  KEY_ORANGE = 0x64
};



class QWSHiddKbdHandlerPrivate;

class QWSHiddKbdHandler: public QWSKeyboardHandler {
	friend class QWSHiddKbdHandlerPrivate;
public:

	explicit QWSHiddKbdHandler(const QString & = QString(), const QString & = QString());
	~QWSHiddKbdHandler();

protected:
	QWSHiddKbdHandlerPrivate *d;
};

class QWSHiddKbdHandlerPrivate: public QObject
{
Q_OBJECT
public:
	QWSHiddKbdHandlerPrivate(QWSHiddKbdHandler *h, const QString &);
	~QWSHiddKbdHandlerPrivate();

	int setupHiddSocket(const char* path);

private:

	class DeviceInfo {
	public:
		DeviceInfo(QWSKeyboardHandler* handler)
			: m_type(DEV_TYPE_KEYBOARD)
			, m_kbdHandler(handler)
		{}

		DeviceInfo(QWSMouseHandler* handler)
			: m_type(DEV_TYPE_MOUSE)
			, m_mouseHandler(handler)
		{}

		~DeviceInfo()
		{
			switch (m_type) {
			case DEV_TYPE_KEYBOARD: delete m_kbdHandler; break;
			case DEV_TYPE_MOUSE: delete m_mouseHandler; break;
			}
		}

	private:
		int m_type;
		union {
			QWSKeyboardHandler* m_kbdHandler;
			QWSMouseHandler* m_mouseHandler;
		};
	};

    /*!
        \brief Maps the NYX special keycodes to Qt Key codes

        WEB OS adds new keycodes to Qt's corelib/global/qnamespace.h. This function will
        map keycodes devlivered by NYX to those new codes.
    */
    Qt::Key mapNyxKeyCode(nyx_keys_custom_key_t code);

	int m_keyFd;
	QSocketNotifier *m_keyNotifier;
	QWSHiddKbdHandler *handler;
	nyx_device_handle_t m_nyxKeysHandle;

	int m_inputDevFd;
	QSocketNotifier *m_inputDevNotifier;
	nyx_device_handle_t m_nyxInputDevHandle;

	std::map<int, DeviceInfo*> m_deviceIdMap;

	int m_curDeviceId;
	int m_curDeviceCountry;

private Q_SLOTS:
	void readKeyData();
	void readInputDevData();
};


#endif /* QHIDDKBD_QWS_H */
