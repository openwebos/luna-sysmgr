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




#ifndef EMERGENCYWINDOWMANAGER_H
#define EMERGENCYWINDOWMANAGER_H

#include "Common.h"

#include "WindowManagerBase.h"
#include "GraphicsItemContainer.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QPointer>
#include <QPropertyAnimation>

class EmergencyWindowManager : public WindowManagerBase
{
	Q_OBJECT

public:

	EmergencyWindowManager(int maxWidth, int maxHeight);
	virtual ~EmergencyWindowManager();

	virtual void addWindow(Window* win);
	virtual void removeWindow(Window* win);
	virtual void focusWindow(Window* win);

	bool okToResize();
	void resize(int width, int height);

    virtual bool handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate);

protected:

	// eat up events if the window is active
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);

	virtual bool sceneEvent(QEvent* event);

private Q_SLOTS:

	void slotPositiveSpaceChanged(const QRect& r);
	void slotIncomingPhoneCall();
	void fadeAnimationFinished();
	void slotHomeButtonPressed();
	void slotEmergencyModeWindowFocusChange(bool enable);
	void slotUiRotationCompleted();
private:

	virtual void init();

	void positionCornerWindows();
	void showCornerWindows();
	void hideCornerWindows();

    bool emergencyWindowBeingDeleted() const;
	
private:
	
	Window* m_emergencyWindow;
	QGraphicsPixmapItem* m_corners[4];
	GraphicsItemContainer* m_cornerContainer;
	QRectF m_winRect;
	QPointer<QPropertyAnimation> m_opacityAnimPtr;
};

#endif /* EMERGENCYWINDOWMANAGER_H */
