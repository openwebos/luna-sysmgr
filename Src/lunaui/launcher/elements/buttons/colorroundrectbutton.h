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




#ifndef COLORROUNDRECTBUTTON_H_
#define COLORROUNDRECTBUTTON_H_

#include "labeledbutton.h"
#include <QColor>
#include <QStateMachine>
#include <QState>

class QGraphicsSceneMouseEvent;
class QGesture;
class QGestureEvent;
class QTouchEvent;
class QPanGesture;
class QSwipeGesture;
class QPinchGesture;
class QTapAndHoldGesture;
class QTapGesture;
class FlickGesture;
class PixmapObject;

class ColorRoundRectButton : public LabeledButton
{
	Q_OBJECT
	Q_PROPERTY(bool stateActive READ stateActive WRITE setStateActive NOTIFY signalStateActiveChanged)

public:
	ColorRoundRectButton(const QSize& encompassingRectSize,const QString& label,QColor normalColor);
	ColorRoundRectButton(const QSize& encompassingRectSize,const QString& label,QColor normalColor,QColor activeColor);
	virtual ~ColorRoundRectButton();
	virtual void commonCtor(QSize requestedSize);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter * painter);
	//TODO: need other paintOffscreen flavors, e.g. PixmapHugeObject

	virtual bool valid();

	virtual bool stateActive() const;
	virtual void setStateActive(bool v);

Q_SIGNALS:

	void signalFirstContact();
	void signalContact();
	void signalRelease();
	void signalLastRelease();
	void signalActivated();

	void signalStateActiveChanged();
	void signalFSMActivate();
	void signalFSMDeactivate();

protected:

	virtual bool touchStartEvent(QTouchEvent *event);
	virtual bool touchUpdateEvent(QTouchEvent *event);
	virtual bool touchEndEvent(QTouchEvent *event);

	virtual bool sceneEvent(QEvent * event);

	//overriding these to suppress them from generating signals
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent);
	virtual bool tapGesture(QTapGesture *tapEvent);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	virtual void setupFSM();

protected:

	QColor					m_normalColor;
	QColor					m_activeColor;
	QColor					m_currentColor;

	int				m_xRndFactor;
	int				m_yRndFactor;

	bool			m_stateActive;
	QStateMachine *	m_p_buttonFSM;
	qint32 					m_touchCount;
	QState * m_p_stateNormal;
	QState * m_p_stateActive;
	bool m_valid;
};

#endif /* COLORROUNDRECTBUTTON_H_ */
