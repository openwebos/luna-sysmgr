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



#include <QtTest/QtTest>

#include <QApplication>
#include <QDesktopWidget>
#include <QGesture>
#include <QWidget>
#include <QWSServer>

#include <linux/input.h>

#include <HidLib.h>
#include "hiddtp_qws.h"

Q_IMPORT_PLUGIN (hiddtp)
Q_IMPORT_PLUGIN (hiddkbd)

// FIXME: Hacked in here
pid_t bootAnimPid;
int   bootAnimPipeFd=-1, sysmgrPipeFd=-1;
int   WebAppMgrPipeFd=-1, IpcServerPipeFd=-1;
char  msgOkToContinue = 0xAB;

// -------------------------------------------------------------------------

static bool s_sawTapGestureStart = false;
static bool s_sawTapGestureFinish = false;
static bool s_sawTapGestureUpdated = false;
static bool s_sawTapGestureCanceled = false;

class GestureWidget : public QWidget
{
public:

	GestureWidget(QWidget* parent) : QWidget(parent) {

		setAttribute(Qt::WA_AcceptTouchEvents);
		grabGesture(Qt::TapGesture);
		grabGesture(Qt::TapAndHoldGesture);
		grabGesture(Qt::PinchGesture);
	};

private:

	bool event(QEvent* event) {

		if (event->type() == QEvent::Gesture) {

			if (QGesture* t = static_cast<QGestureEvent*>(event)->gesture(Qt::TapGesture)) {
				QTapGesture* tap = static_cast<QTapGesture*>(t);
				switch (tap->state()) {
				case Qt::GestureStarted:
					s_sawTapGestureStart = true;
					qDebug() << "Tap GestureStarted";
					break;
				case Qt::GestureUpdated:
					s_sawTapGestureUpdated = true;
					qDebug() << "Tap GestureUpdated";
					break;
				case Qt::GestureFinished:
					s_sawTapGestureFinish = true;
					qDebug() << "Tap GestureFinished";
					break;
				case Qt::GestureCanceled:
					s_sawTapGestureCanceled = true;
					qDebug() << "Tap GestureCanceled";
				default:
					Q_ASSERT(false);
				}
			}

			return true;
		}

		return QWidget::event(event);
	}
};

static GestureWidget* s_widget = 0;

// -------------------------------------------------------------------------

class InputTouch : public QObject
{
	Q_OBJECT

private:

	void sendTouchDown(int x, int y);
	void sendTouchUpdate(int x, int y);
	void sendTouchUp(int x, int y);
	void sendTwoTouchDown(int x1, int y1, int x2, int y2, int code1=0, int code2=1);
	void sendTwoTouchUpdate(int x1, int y1, int x2, int y2, int code1=0, int code2=1);
	void sendTwoTouchUp(int x1, int y1, int x2, int y2, int code1=0, int code2=1);

private Q_SLOTS:
	
	void initTestCase();
	void testTap();
	void testNoTapWithTwoFingers();
	void testTapAfterTwoTouchDowns();
};

void InputTouch::initTestCase()
{
	::setenv("QT_PLUGIN_PATH", "/usr/plugins", 1);

    QWSServer::setDefaultMouse("HiddTp");
    QWSServer::setDefaultKeyboard("HiddKbd");

	QDesktopWidget* desktopWidget = QApplication::desktop();

	QWidget* topWidget = new QWidget;
	topWidget->setFixedSize(desktopWidget->screenGeometry().size());
	
	s_widget = new GestureWidget(topWidget);
	s_widget->setFixedSize(desktopWidget->screenGeometry().size());

	topWidget->show();

	qDebug() << "Created widget: " << s_widget;

	QApplication::setActiveWindow(topWidget);
	QTest::qWaitForWindowShown(s_widget);
}

void InputTouch::sendTouchDown(int x, int y)
{
	const int numEvents = 5;
	struct input_event inputEvents[numEvents];

	struct input_event* ev = 0;

	ev = &inputEvents[0];
	ev->type = EV_FINGERID;
	ev->value = 0;
	ev->code = 0;

	ev = &inputEvents[1];
	ev->type = EV_KEY;
	ev->value = 1;	
	ev->code = BTN_TOUCH;

	ev = &inputEvents[2];
	ev->type = EV_ABS;
	ev->value = x;
	ev->code = ABS_X;

	ev = &inputEvents[3];
	ev->type = EV_ABS;
	ev->value = y;
	ev->code = ABS_Y;

	ev = &inputEvents[4];
	ev->type = EV_SYN;

	QWSHiddTpHandler* handler = static_cast<QWSHiddTpHandler*>(QWSServer::mouseHandler());
	handler->pumpHiddData(inputEvents, numEvents);
}

void InputTouch::sendTouchUpdate(int x, int y)
{
	const int numEvents = 4;
	struct input_event inputEvents[numEvents];

	struct input_event* ev = 0;

	ev = &inputEvents[0];
	ev->type = EV_FINGERID;
	ev->value = 0;
	ev->code = 0;

	ev = &inputEvents[1];
	ev->type = EV_ABS;
	ev->value = x;
	ev->code = ABS_X;

	ev = &inputEvents[2];
	ev->type = EV_ABS;
	ev->value = y;
	ev->code = ABS_Y;

	ev = &inputEvents[3];
	ev->type = EV_SYN;

	QWSHiddTpHandler* handler = static_cast<QWSHiddTpHandler*>(QWSServer::mouseHandler());
	handler->pumpHiddData(inputEvents, numEvents);
}

void InputTouch::sendTouchUp(int x, int y)
{
	const int numEvents = 5;
	struct input_event inputEvents[numEvents];

	struct input_event* ev = 0;

	ev = &inputEvents[0];
	ev->type = EV_FINGERID;
	ev->value = 0;
	ev->code = 0;

	ev = &inputEvents[1];
	ev->type = EV_KEY;
	ev->value = 0;
	ev->code = BTN_TOUCH;

	ev = &inputEvents[2];
	ev->type = EV_ABS;
	ev->value = x;
	ev->code = ABS_X;

	ev = &inputEvents[3];
	ev->type = EV_ABS;
	ev->value = y;
	ev->code = ABS_Y;

	ev = &inputEvents[4];
	ev->type = EV_SYN;	

	QWSHiddTpHandler* handler = static_cast<QWSHiddTpHandler*>(QWSServer::mouseHandler());
	handler->pumpHiddData(inputEvents, numEvents);
}

void InputTouch::sendTwoTouchDown(int x1, int y1, int x2, int y2, int code1, int code2)
{
	const int numEvents = 9;
	struct input_event inputEvents[numEvents];

	struct input_event* ev = 0;

	ev = &inputEvents[0];
	ev->type = EV_FINGERID;
	ev->value = code1;
	ev->code = code1;

	ev = &inputEvents[1];
	ev->type = EV_KEY;
	ev->value = 1;	
	ev->code = BTN_TOUCH;

	ev = &inputEvents[2];
	ev->type = EV_ABS;
	ev->value = x1;
	ev->code = ABS_X;

	ev = &inputEvents[3];
	ev->type = EV_ABS;
	ev->value = y1;
	ev->code = ABS_Y;

	
	ev = &inputEvents[4];
	ev->type = EV_FINGERID;
	ev->value = code2;
	ev->code = code2;

	ev = &inputEvents[5];
	ev->type = EV_KEY;
	ev->value = 1;	
	ev->code = BTN_TOUCH;

	ev = &inputEvents[6];
	ev->type = EV_ABS;
	ev->value = x2;
	ev->code = ABS_X;

	ev = &inputEvents[7];
	ev->type = EV_ABS;
	ev->value = y2;
	ev->code = ABS_Y;
	
	ev = &inputEvents[8];
	ev->type = EV_SYN;

	QWSHiddTpHandler* handler = static_cast<QWSHiddTpHandler*>(QWSServer::mouseHandler());
	handler->pumpHiddData(inputEvents, numEvents);
}

void InputTouch::sendTwoTouchUp(int x1, int y1, int x2, int y2, int code1, int code2)
{
	const int numEvents = 9;
	struct input_event inputEvents[numEvents];

	struct input_event* ev = 0;

	ev = &inputEvents[0];
	ev->type = EV_FINGERID;
	ev->value = code1;
	ev->code = code1;

	ev = &inputEvents[1];
	ev->type = EV_KEY;
	ev->value = 0;	
	ev->code = BTN_TOUCH;

	ev = &inputEvents[2];
	ev->type = EV_ABS;
	ev->value = x1;
	ev->code = ABS_X;

	ev = &inputEvents[3];
	ev->type = EV_ABS;
	ev->value = y1;
	ev->code = ABS_Y;

	
	ev = &inputEvents[4];
	ev->type = EV_FINGERID;
	ev->value = code2;
	ev->code = code2;

	ev = &inputEvents[5];
	ev->type = EV_KEY;
	ev->value = 0;	
	ev->code = BTN_TOUCH;

	ev = &inputEvents[6];
	ev->type = EV_ABS;
	ev->value = x2;
	ev->code = ABS_X;

	ev = &inputEvents[7];
	ev->type = EV_ABS;
	ev->value = y2;
	ev->code = ABS_Y;
	
	ev = &inputEvents[8];
	ev->type = EV_SYN;

	QWSHiddTpHandler* handler = static_cast<QWSHiddTpHandler*>(QWSServer::mouseHandler());
	handler->pumpHiddData(inputEvents, numEvents);
}

void InputTouch::sendTwoTouchUpdate(int x1, int y1, int x2, int y2, int code1, int code2)
{
	const int numEvents = 7;
	struct input_event inputEvents[numEvents];

	struct input_event* ev = 0;

	ev = &inputEvents[0];
	ev->type = EV_FINGERID;
	ev->value = code1;
	ev->code = code1;

	ev = &inputEvents[1];
	ev->type = EV_ABS;
	ev->value = x1;
	ev->code = ABS_X;

	ev = &inputEvents[2];
	ev->type = EV_ABS;
	ev->value = y1;
	ev->code = ABS_Y;

	
	ev = &inputEvents[3];
	ev->type = EV_FINGERID;
	ev->value = code2;
	ev->code = code2;

	ev = &inputEvents[4];
	ev->type = EV_ABS;
	ev->value = x2;
	ev->code = ABS_X;

	ev = &inputEvents[5];
	ev->type = EV_ABS;
	ev->value = y2;
	ev->code = ABS_Y;
	
	ev = &inputEvents[6];
	ev->type = EV_SYN;

	QWSHiddTpHandler* handler = static_cast<QWSHiddTpHandler*>(QWSServer::mouseHandler());
	handler->pumpHiddData(inputEvents, numEvents);
}


void InputTouch::testTap()
{
	s_sawTapGestureStart = false;
	s_sawTapGestureFinish = false;

	sendTouchDown(100, 100);
	QVERIFY(s_sawTapGestureStart);
	sendTouchUp(100, 100);
	QVERIFY(s_sawTapGestureFinish);
}

void InputTouch::testNoTapWithTwoFingers()
{
	s_sawTapGestureStart = false;
	s_sawTapGestureFinish = false;
	s_sawTapGestureCanceled = false;
	s_sawTapGestureFinish = false;

	sendTwoTouchDown(10, 10, 100, 100);
	QVERIFY(s_sawTapGestureStart);
	sendTwoTouchUpdate(11, 11, 101, 101);
	QVERIFY(s_sawTapGestureCanceled);
	sendTouchUpdate(12, 12); // Deliberately miss a touch in the frame
	sendTwoTouchUp(10, 10, 100, 100);

	QVERIFY(!s_sawTapGestureFinish);
}

void InputTouch::testTapAfterTwoTouchDowns()
{
	// NOV-119261: This test cases tries to reproduce what is seen in the bug
	// "Tap" stopped working - all other gestures continued working
	
	s_sawTapGestureStart = false;
	s_sawTapGestureFinish = false;
	s_sawTapGestureCanceled = false;
	s_sawTapGestureFinish = false;

	sendTwoTouchDown(10, 10, 100, 100, 11, 12);
	QVERIFY(s_sawTapGestureStart);
	sendTwoTouchUpdate(11, 11, 101, 101, 11, 12);
	
	sendTwoTouchDown(10, 10, 100, 100, 13, 14);
	
	// Tap 1. this will just cancel

	s_sawTapGestureStart = false;
	s_sawTapGestureFinish = false;
	sendTouchDown(100, 100);
	QVERIFY(s_sawTapGestureCanceled);
	sendTouchUp(100, 100);

	// Tap 2. This should work
	
	s_sawTapGestureStart = false;
	s_sawTapGestureFinish = false;

	sendTouchDown(100, 100);
	QVERIFY(s_sawTapGestureStart);
	sendTouchUp(100, 100);
	QVERIFY(s_sawTapGestureFinish);

	// Tap 3. This should work
	
	s_sawTapGestureStart = false;
	s_sawTapGestureFinish = false;

	sendTouchDown(100, 100);
	QVERIFY(s_sawTapGestureStart);
	sendTouchUp(100, 100);
	QVERIFY(s_sawTapGestureFinish);
}

QTEST_MAIN(InputTouch) 
#include "sysmgrtst_InputTouch.moc"
