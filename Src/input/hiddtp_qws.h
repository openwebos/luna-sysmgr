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



#ifndef QHIDDTP_QWS_H
#define QHIDDTP_QWS_H

#include "Common.h"

#include <QtGui/qmouse_qws.h>
#include <QList>
#include <QTouchEvent>

#include <stdint.h>
#include <stdio.h>
#include <glib.h>

#include "FlickGesture.h"
#include "ScreenEdgeFlickGesture.h"

#include <nyx/nyx_client.h>

#define EV_GESTURE 0x06

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

class QWSHiddTpHandlerPrivate;

class QWSHiddTpHandler: public QWSMouseHandler {
	friend class QWSHiddTpHandlerPrivate;
public:

	explicit QWSHiddTpHandler(const QString & = QString(), const QString & = QString());
	~QWSHiddTpHandler();

	void suspend();
	void resume();
	void pumpHiddData(struct input_event* inputEvents, int numEvents);
	void startRecording();
	void stopRecording();
	
protected:
	QWSHiddTpHandlerPrivate *d;
};

class QWSHiddTpHandlerPrivate: public QObject {
Q_OBJECT
public:
	QWSHiddTpHandlerPrivate(QWSHiddTpHandler *h, const QString &);
	~QWSHiddTpHandlerPrivate();

	void suspend();
	void resume();
	int setupHiddSocket(const char* path);
	void parseHiddData(struct input_event* inputEvents, int numEvents);

	void startRecording();
	void stopRecording();

	void enableScreenEdgeGesture();
	
private:
	Qt::Key lookupGesture(uint16_t value);

	QWSHiddTpHandler *handler;
	FlickGesture* flickGesture;
	ScreenEdgeFlickGesture* m_screenEdgeFlickGesture;

	enum State {
	    FingerDown, FingerUp, FingerMove, FingerNoMove, Invalid = -1
	};

	struct HiddTouch {
		uint64_t hiddId;
		int id;
		struct timeval time;
		int x;
		int y;
		int xDown;
		int yDown;
		int xVelocity;
		int yVelocity;
		int gestureKey;

		State state;
		bool isPrimary;
		bool isMetaTouch;
		bool seenInScan;

		HiddTouch() {
		    reset();
		}

		void reset() {
		    hiddId = 0;
		    id = -1;
		    x = 0;
		    y = 0;
			xDown = 0;
			yDown = 0;
		    time.tv_sec = 0;
		    time.tv_usec = 0;
		    state = Invalid;
		    isPrimary = false;
		    gestureKey = Qt::Key_unknown;
		    xVelocity = 0;
		    yVelocity = 0;
		    isMetaTouch = false;
			seenInScan = false;
		}
	};

	QList<HiddTouch> m_touches;
	QPoint m_lastTouchDown;
	QPoint m_mousePress;
	uint32_t m_mousePressTime;

    	/* fine-tuning  support for NYX */
	nyx_device_handle_t m_nyxPenHandle;
	int m_penFd;

	int m_deviceWidth;
	int m_deviceHeight;
	int m_metaActiveTouchesCount;
	bool m_sendPenCancel;
	QPoint m_penCancelPoint;

	bool m_isSuspended;
	FILE* m_recordFile;
	bool m_enableScreenEdgeGesture;

	bool updateTouchEvents(QList<HiddTouch>& hiddTouches);

	void addNewTouch(HiddTouch& touch);
	bool updateOldTouch(HiddTouch& touch);
	void generateTouchEvent();
	void removeReleasedTouches();

	inline int squareDistance (const HiddTouch& p1, const HiddTouch& p2) {
	    int dx = p1.x - p2.x;
	    int dy = p1.y - p2.y;
	    
	    return dx*dx + dy*dy;
	}

	bool updateTouch (struct Touch touch);

	static gboolean ioCallback(GIOChannel* channel, GIOCondition condition, gpointer arg);
										 
private Q_SLOTS:
	void readHiddData();
};


#endif /* QHIDDTP_QWS_H */
