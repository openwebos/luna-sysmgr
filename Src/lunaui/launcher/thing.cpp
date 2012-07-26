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




#include "thing.h"
#include "dimensionsmain.h"
#include <QString>

#include <QGraphicsSceneMouseEvent>

#include <QDebug>

//virtual
RedirectContext::~RedirectContext()
{
	if (m_dbg_onDestruct)
	{
		qDebug() << "RedirectContext" << this << " getting destroyed";
	}
}

QDebug& operator<<(QDebug dbg,const TouchRegister& reg)
{
	dbg << "\"Register\":{\"touchId\":" << reg.touchId
			<< ", \"triggerType\":" << reg.triggerType
			<< ", \"valid\":" << reg.valid
			<< ", \"redirecting\":" << reg.redirecting
			<< ", \"pause\":" << reg.pause
			<< ", \"pContext\":" << (int)(reg.pRedirectContext)
			<< "}";
	return dbg.nospace();
}

Thing::Thing()
: m_qp_takerOwner(0)
{
	m_uid = QUuid::createUuid();
}

Thing::Thing(const QUuid& specificUid)
: m_qp_takerOwner(0)
{
	//this is in general a bad thing, and the reason why uuids don't have operator=. HOWEVER,
	// in certain situations it's ok to do this, such as if an object will live on in a frozen state
	// (on disk, etc)
	if (specificUid.isNull())
	{
		//can never allow an invalid one
		m_uid = QUuid::createUuid();
	}
	else
	{
		m_uid = specificUid;
	}
}

//virtual
Thing::~Thing()
{
}

//virtual
QUuid Thing::uid() const
{
	return m_uid;
}

//virtual
bool Thing::offer(Thing * p_offer,Thing * p_offeringThing)
{
	return false;
}

//called to transfer this Thing's belongs-to to another Thing
// e.g. icons between pages, etc
// returns false if rejected
//virtual
bool Thing::take(Thing * p_takerThing)
{
	m_qp_takerOwner = p_takerThing;
	return true;
}

//called by the Thing that I own, to tell me some other Thing is trying to take it
// I can return "false" to prevent the abduction
//virtual
bool Thing::taking(Thing * p_victimThing, Thing * p_takerThing)
{
	return true;
}

//called by the Thing that I used to own, to tell me some other Thing took it
//virtual
void Thing::taken(Thing * p_takenThing,Thing * p_takerThing)
{
}

//virtual
uint Thing::hashValue() const
{
	//compiler issues - revisit this...
	return qHash(m_uid.toString());
}

//friend
uint qHash(const Thing& t)
{
	return t.hashValue();
}

uint qHash(const QPointer<Thing>& qpt)
{
	if (qpt)
	{
		return qHash(*(qpt.data()));
	}
	return 0;
}

//virtual
void Thing::slotEnableIconAutoRepaint()
{

}
//virtual
void Thing::slotDisableIconAutoRepaint()
{

}

//virtual
void Thing::touchTrackedPointStarted(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{

}

//virtual
void Thing::touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& amountMoved,const QPointF& initialPosition)
{

}

//virtual
void Thing::touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& amountMoved,const QPointF& initialPosition)
{

}

//virtual
void Thing::redirectTouchPrepare(Thing * p_sourceThing,const TouchRegister& touchRegister,int contextHint)
{

}

//virtual
void Thing::redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& amountMoved,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	//do nothing
}
//virtual
void Thing::redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& amountMoved,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	//do nothing
}


//ALL YOUR MOUSE EVENT HANDLING ARE BELONG TO US!
//(WE SET UP US, THE TOUCH POINT BOMB!)
//
//Dimensions/Launcher3 uses touch points predominantly (hopefully exclusively, ultimately). But to keep mouse events, which get generated always, from
//affecting "underneath" items (like card windows) which still use mouse events, every interact-able item in Launcher3 will devour mouse events.
//Since every such item in Launcher3 is (hopefully!) a Thing subclass, this should suffice...

//TEMPORARILY COMMENTED; OVERLAY WIN M. IS GOING TO BLOCK PROPAGATING INSTEAD

////virtual
//void Thing::mousePressEvent(QGraphicsSceneMouseEvent *event)
//{
//	qDebug() << __FUNCTION__ << ": class: " << this->metaObject()->className();
//	event->ignore();
////	event->accept();
//}
//
////virtual
//void Thing::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
//{
////	event->accept();
//}
//
////virtual
//void Thing::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
//{
////	event->accept();
//}
