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




#include "dimensionsglobal.h"
#include "colorroundrectbutton.h"
#include "debugglobal.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QString>
#include <QSignalTransition>
#include <QDebug>

#include <QEvent>
#include <QGesture>
#include <QGestureEvent>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#define	DEFAULT_SIZE	QSize(1.618*30,30)

ColorRoundRectButton::ColorRoundRectButton(const QSize& encompassingRectSize,const QString& label,QColor normalColor)
: LabeledButton(QRectF(0,0,1,1))
, m_normalColor(normalColor)
, m_currentColor(normalColor)
, m_stateActive(false)
, m_p_buttonFSM(0)
, m_touchCount(0)
, m_valid(true)
{

	m_label = label;
	//darken the normal color to make the active one
	m_activeColor = m_normalColor.darker(300);
	ColorRoundRectButton::commonCtor(encompassingRectSize);
}

ColorRoundRectButton::ColorRoundRectButton(const QSize& encompassingRectSize,const QString& label,
											QColor normalColor,QColor activeColor)
: LabeledButton(QRectF(0,0,1,1))
, m_normalColor(normalColor)
, m_activeColor(activeColor)
, m_currentColor(normalColor)
, m_stateActive(false)
, m_p_buttonFSM(0)
, m_touchCount(0)
, m_valid(true)
{

	m_label = label;
	ColorRoundRectButton::commonCtor(encompassingRectSize);
}

//virtual
void ColorRoundRectButton::commonCtor(QSize requestedSize)
{
	setAcceptTouchEvents(true);
	grabGesture(Qt::TapGesture);
	setLabel(m_label);
	if ( (!requestedSize.isValid()) || (requestedSize.width() < 2) || (requestedSize.height() < 2))
	{
		LabeledButton::resize(DEFAULT_SIZE);
	}
	else
	{
		LabeledButton::resize(requestedSize);
	}

	m_xRndFactor = (quint32)(m_geom.width())>>3;
	m_yRndFactor = (quint32)(m_geom.height())>>3;
	connect(this,SIGNAL(signalFirstContact()),
			this,SIGNAL(signalFSMActivate()));
	connect(this,SIGNAL(signalContact()),
			this,SIGNAL(signalFSMActivate()));
	connect(this,SIGNAL(signalRelease()),
			this,SIGNAL(signalFSMDeactivate()));
	connect(this,SIGNAL(signalLastRelease()),
			this,SIGNAL(signalFSMDeactivate()));

	m_valid = true;
	setupFSM();
	m_p_buttonFSM->start();
}

//helper
static QState* createState(QString name, QState *parent=0)
{
	QState *result = new QState(parent);
	result->setObjectName(name);
	return result;
}

//virtual
bool ColorRoundRectButton::sceneEvent(QEvent * event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture * g = 0;
		g = ge->gesture(Qt::TapGesture);
		if (g) {
			QTapGesture* tap = static_cast<QTapGesture*>(g);
			if (tap->state() == Qt::GestureFinished) {
				tapGesture(tap);
			}
			return true;
		}
		g = ge->gesture(Qt::TapAndHoldGesture);
		if (g) {
			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
			if (hold->state() == Qt::GestureFinished) {
				tapAndHoldGesture(hold);
			}
			return true;
		}
	}
	else if (event->type() == QEvent::TouchBegin)
	{
		return touchStartEvent(static_cast<QTouchEvent *>(event));
	}
	else if (event->type() == QEvent::TouchUpdate)
	{
		return touchUpdateEvent(static_cast<QTouchEvent *>(event));
	}
	else if (event->type() == QEvent::TouchEnd)
	{
		return touchEndEvent(static_cast<QTouchEvent *>(event));
	}
	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool ColorRoundRectButton::touchStartEvent(QTouchEvent *event)
{
	++m_touchCount;
	if (m_touchCount == 1)
	{
		Q_EMIT signalFirstContact();
	}
	else
	{
		Q_EMIT signalContact();
	}
	return true;
}
//virtual
bool ColorRoundRectButton::touchUpdateEvent(QTouchEvent *event)
{
	return true;
}
//virtual
bool ColorRoundRectButton::touchEndEvent(QTouchEvent *event)
{
	--m_touchCount;
	if (m_touchCount <= 0)
	{
		m_touchCount = 0;
		Q_EMIT signalLastRelease();
	}
	else
	{
		Q_EMIT signalRelease();
	}
	return true;
}

//virtual
void ColorRoundRectButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();	//take the event so it doesn't go elsewhere
}

//virtual
void ColorRoundRectButton::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{

}

//virtual
void ColorRoundRectButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
}

//virtual
bool ColorRoundRectButton::tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent)
{
	Q_EMIT signalContact();
	return true;
}
//virtual
bool ColorRoundRectButton::tapGesture(QTapGesture *tapEvent)
{
	Q_EMIT signalContact();
	return true;
}
//virtual
void ColorRoundRectButton::setupFSM()
{
	m_p_buttonFSM = new QStateMachine(this);
	m_p_buttonFSM->setObjectName("fsm");

	m_p_stateNormal        = createState("normal",m_p_buttonFSM);
	m_p_stateActive     = createState("active",m_p_buttonFSM);

	// ------------------- STATE: normal -----------------------------------
	//	normal PROPERTIES
	m_p_stateNormal->assignProperty(this,"stateActive", false);
	//  normal TRANSITIONS
	m_p_stateNormal->addTransition(this,SIGNAL(signalFSMActivate()),m_p_stateActive);
	//  normal SIDE-EFFECTS

	// ------------------- STATE: active ------------------------------------
	// active PROPERTIES
	m_p_stateActive->assignProperty(this,"stateActive", true);
	// active TRANSITIONS
	QSignalTransition * transition = m_p_stateActive->addTransition(this, SIGNAL(signalFSMDeactivate()),m_p_stateNormal);
	//  active SIDE-EFFECTS
	connect(transition,SIGNAL(triggered()),this,SIGNAL(signalActivated()));	/// <---- this one alerts the clients to the action!!

//	m_p_buttonFSM->addState(m_p_stateNormal);
//	m_p_buttonFSM->addState(m_p_stateActive);

	m_p_buttonFSM->setInitialState(m_p_stateNormal);
}

//protected Q_SLOTS:

//virtual
ColorRoundRectButton::~ColorRoundRectButton()
{
}

//virtual
void ColorRoundRectButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
//	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,7);
	QBrush sb = painter->brush();
	painter->setBrush(m_currentColor);
	painter->drawRoundRect(m_geom,m_xRndFactor,m_yRndFactor);
	painter->setBrush(sb);
	QPen sp = painter->pen();
	painter->setPen(m_selectedColor);
	m_textLayoutObject.draw(painter,m_labelPosPntCS);
	painter->setPen(sp);
}

//virtual
void ColorRoundRectButton::paintOffscreen(QPainter *painter)
{
}

//virtual
bool ColorRoundRectButton::valid()
{
	return m_valid;
}

//virtual
bool ColorRoundRectButton::stateActive() const
{
	return m_stateActive;
}

//virtual
void ColorRoundRectButton::setStateActive(bool v)
{
	m_stateActive = v;
	m_currentColor = ( v ? m_activeColor : m_normalColor );
	update();
}
