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




#ifndef CARDHOSTWINDOW_H
#define CARDHOSTWINDOW_H

#include "Common.h"

#include "CardWindow.h"

#include "AnimationEquations.h"
#include "Timer.h"

class PIpcBuffer;
class PIpcChannel;
class IpcClientHost;

class CardHostWindow : public CardWindow
{
Q_OBJECT

public:

	CardHostWindow(Window::Type type, HostWindowData* data, IpcClientHost* clientHost);
	virtual ~CardHostWindow();

	virtual void resizeEvent(int w, int h);
	virtual void resizeEventSync(int w, int h);
	virtual void resizeWindowBufferEvent(int w, int h, QRect windowScreenBounds, bool forceSync);
	virtual void focusEvent(bool enable);
	virtual void aboutToFocusEvent(bool enable);

	virtual bool isHost() const { return true; }
	virtual bool supportsPause() const { return true; }
	virtual void pause();
    bool paused() const { return m_paused; }

protected:

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	virtual void paintBase(QPainter* painter, bool maximized);
	virtual void paintOverlay(QPainter* painter, bool maximized);

	virtual void fullScreenEnabled(bool enabled);

	virtual bool touchEvent(QTouchEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	virtual void refreshAdjustmentAngle();
	virtual void onSetAppFixedOrientation(int orientation, bool isPortrait);

    virtual void initializeRoundedCornerStage();

private:

	virtual void displayOff();
	virtual void displayDimmed();

	void pauseCard();
	void resumeCard();

	bool rotateTimerTicked();

	bool isOrientationPortrait(Event::Orientation orient);

	bool m_paused;
	bool m_pausedDueToDisplayOff;
	bool m_penDownInPlayButton;
	bool m_penInPlayButton;

	bool m_inRotateAnim;
	int m_rotateAngleStart;
	int m_rotateAngleTarget;
	PValue m_rotateScaleStart;
	PValue m_rotateScaleTarget;
	uint32_t m_rotateTimerStart;
	AnimationEquation m_rotateEquation;
	Timer<CardHostWindow> m_rotateAnimTimer;

};

#endif /* CARDHOSTWINDOW_H */
