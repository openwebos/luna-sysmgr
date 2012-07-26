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




#ifndef DOCKMODEWINDOW_H
#define DOCKMODEWINDOW_H

#include "Common.h"

#include "Event.h"
#include "CardWindow.h"
#include "Timer.h"
#include "DockModeWindowManager.h"
#include <QGraphicsRotation>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QList>
#include <QPainterPath>

class PIpcMessage;
class FlickGesture;
class DockModeLaunchPoint;


class DockModeWindow : public CardWindow
{
	Q_OBJECT
	Q_PROPERTY(qreal pulseOpacity READ pulseOpacity WRITE setPulseOpacity)
public:

	DockModeWindow(Window::Type type, HostWindowData* data, IpcClientHost* clientHost=0);
	DockModeWindow(Window::Type type, const QPixmap& pixmap);
	virtual ~DockModeWindow();
	
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	
	void setRotationAxis(bool x, bool y, bool z);
	void setRotationAngle(double angle);
	
	virtual void setMaximized(bool enable);

	virtual void setPrepareAddedToWindowManager();
	virtual void setAddedToWindowManager();

protected:
	bool m_loadingOverlayEnabled;

	qreal pulseOpacity() const { return m_pulseOpacity; }
	void setPulseOpacity(qreal opacity);
	qreal m_pulseOpacity;

	QPixmap m_icon;

	QSequentialAnimationGroup m_pulseAnimation;
	static unsigned int s_dockGlowRefCount;
	static QPixmap* s_dockGlow;

	QPropertyAnimation m_fadeAnimation;
	QGraphicsRotation  m_rotation;
	QList<QGraphicsTransform *> m_transformList;
	bool m_nativeApp;

};

#endif /* DOCKMODEWINDOW_H */
