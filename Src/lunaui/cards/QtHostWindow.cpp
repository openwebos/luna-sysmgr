/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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

#include "QtHostWindow.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

QtHostWindow::QtHostWindow(Window::Type type, HostWindowData* data, IpcClientHost* clientHost)
	: CardHostWindow(type, data, clientHost)
{
}

QtHostWindow::~QtHostWindow()
{
}

bool QtHostWindow::touchEvent(QTouchEvent* event)
{

	if (!m_focused || m_pendingFocus == PendingFocusFalse) {
		return false;
	}

	if (paused())
		return true;

    if (m_channel) {
        QRectF br = boundingRect();
        m_channel->sendAsyncMessage(new View_TouchEvent(routingId(), SysMgrTouchEvent(event, -br.x(), -br.y())));
    }

    return true;
/*
    if (m_channel) {

		typedef QList<QTouchEvent::TouchPoint> TouchPoints;
		TouchPoints touchPoints = event->touchPoints();

		QPointF topLeft = boundingRect().topLeft();

		for (TouchPoints::const_iterator it = touchPoints.begin();
			 it != touchPoints.end(); ++it) {

//			printf("TouchPoint Id: %d, State: %d, pos: %g, %g\n",
//				   (*it).id(), (*it).state(), (*it).pos().x(), (*it).pos().y());

			qreal adjustmentAngle = m_adjustmentAngle;


			int displayWidth = Settings::LunaSettings()->displayWidth;
			int displayHeight = Settings::LunaSettings()->displayHeight;
			QPointF pt = (*it).pos();
			pt -= topLeft;
			int x = pt.x();
			int y = pt.y();

			//		adjustmentAngle, x, y, displayWidth, displayHeight);
			if (adjustmentAngle == 90) { //0,0 in upper right
				int tmp = y;
				y = x;
				x = tmp;

				y = displayHeight-y;
			}
			else if (adjustmentAngle == -90) { //0,0 in lower left

				int tmp = y;
				y = x;
				x = tmp;

				x = displayWidth-x;
			} else if (adjustmentAngle == 0) { //0,0 in upper left
				//Do nothing
			}
			else if (adjustmentAngle == 180) { //0,0 in lower right / screen dimensions flip
				x = displayWidth-x;
				y = displayHeight-y;
			}


			switch ((*it).state()) {
			case Qt::TouchPointPressed: {

				Event evt;
				evt.type = Event::PenDown;
				evt.id = (*it).id();
				evt.x = x;
				evt.y = y;
				evt.z = 0;
				evt.key = Event::Key_Null;
				evt.button = Event::Left;
				evt.modifiers = Event::modifiersFromQt(event->modifiers());
				evt.time = Time::curTimeMs();
				evt.clickCount = 1;

				m_channel->sendAsyncMessage (new View_InputEvent(routingId(),
																 SysMgrEventWrapper(&evt)));

				break;				
			}
			case Qt::TouchPointMoved: {

				Event evt;
				evt.type = Event::PenMove;
				evt.id = (*it).id();
				evt.x = x;
				evt.y = y;
				evt.z = 0;
				evt.key = Event::Key_Null;
				evt.button = Event::Left;
				evt.modifiers = Event::modifiersFromQt(event->modifiers());
				evt.time = Time::curTimeMs();
				evt.clickCount = 0;

				m_channel->sendAsyncMessage (new View_InputEvent(routingId(),
																 SysMgrEventWrapper(&evt)));

				break;
			}
			case Qt::TouchPointReleased: {

				Event evt;
				evt.type = Event::PenUp;
				evt.id = (*it).id();
				evt.x = x;
				evt.y = y;
				evt.z = 0;
				evt.key = Event::Key_Null;
				evt.button = Event::Left;
				evt.modifiers = Event::modifiersFromQt(event->modifiers());
				evt.time = Time::curTimeMs();
				evt.clickCount = 0;

				m_channel->sendAsyncMessage (new View_InputEvent(routingId(),
																 SysMgrEventWrapper(&evt)));

				break;
			}
			default:
				continue;
			}								
		}
    }

	return true;
*/
}

