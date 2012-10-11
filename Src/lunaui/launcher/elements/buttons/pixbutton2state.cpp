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
#include "pixbutton2state.h"
#include "pixmapobject.h"
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

//public:

PixButton2State::PixButton2State(const QString& label,PixmapObject * p_pixStateNormal,PixmapObject * p_pixStateActive)
: PixButton(QRectF(0,0,1,1))
, m_qp_pixNormal(p_pixStateNormal)
, m_qp_pixActive(p_pixStateActive)
, m_qp_currentlyRenderingPmo(0)
, m_stateActive(false)
, m_p_buttonFSM(0)
, m_p_stateNormal(0)
, m_p_stateActive(0)
, m_valid(true)
{

	m_label = label;
	PixButton2State::commonCtor();
}

//virtual
void PixButton2State::commonCtor()
{
	QRectF g1;
	QRectF g2;

	if ((!m_qp_pixNormal) && (!m_qp_pixActive))
	{
		m_valid = false;
		//prevent it from being painted, and avoid costly if-checks in paint()
		setFlag(ItemHasNoContents,true);
	}
	else {
		setFlag(ItemHasNoContents,false);
		if (m_qp_pixNormal)
		{
			g1 = DimensionsGlobal::realRectAroundRealPoint(m_qp_pixNormal->size());
		}
		if (m_qp_pixActive)
		{
			g2 = DimensionsGlobal::realRectAroundRealPoint(m_qp_pixActive->size());
		}

		ThingPaintable::resize(g1.unite(g2).size().toSize());
	}
	m_qp_currentlyRenderingPmo = m_qp_pixNormal;

	connect(this,SIGNAL(signalFirstContact()),
			this,SIGNAL(signalFSMActivate()));
	connect(this,SIGNAL(signalRelease()),
			this,SIGNAL(signalFSMDeactivate()));
	connect(this,SIGNAL(signalLastRelease()),
			this,SIGNAL(signalFSMDeactivate()));

	setLabel(m_label);
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
bool PixButton2State::tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent)
{
	Q_EMIT signalContact();
	return true;
}
//virtual
bool PixButton2State::tapGesture(QTapGesture *tapEvent)
{
	Q_EMIT signalContact();
	return true;
}
//virtual
void PixButton2State::setupFSM()
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
	m_p_stateActive->addTransition(this, SIGNAL(signalActivated()),m_p_stateNormal);
	m_p_stateActive->addTransition(this, SIGNAL(signalCancel()),m_p_stateNormal);
	//  active SIDE-EFFECTS
	connect(transition,SIGNAL(triggered()),this,SIGNAL(signalActivated()));	/// <---- this one alerts the clients to the action!!


	m_p_buttonFSM->setInitialState(m_p_stateNormal);
}

//protected Q_SLOTS:

//virtual
PixButton2State::~PixButton2State()
{
}

//virtual
void PixButton2State::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
//	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,7);
	//TODO: the pmo could have been deleted at any time. Tie up some signals to catch deletions and set the item flag
	m_qp_currentlyRenderingPmo->paint(painter,m_geom.topLeft());
	if (!m_label.isEmpty())
	{
		QPen sp = painter->pen();
		painter->setPen(m_selectedColor);
		m_textLayoutObject.draw(painter,m_labelPosPntCS);
		painter->setPen(sp);
	}
}

//virtual
void PixButton2State::paintOffscreen(QPainter *painter)
{
	//TODO: IMPLEMENT
}

//virtual
bool PixButton2State::valid()
{
	return m_valid;
}

//virtual
bool PixButton2State::stateActive() const
{
	return m_stateActive;
}

//virtual
void PixButton2State::setStateActive(bool v)
{
	m_stateActive = v;
	m_qp_currentlyRenderingPmo = ( v ? m_qp_pixActive : m_qp_pixNormal );
	if (m_qp_currentlyRenderingPmo)
	{
		setFlag(ItemHasNoContents,false);
	}
	else
	{
		setFlag(ItemHasNoContents,true);
	}
	update();
}
