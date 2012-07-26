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




#ifndef THING_H_
#define THING_H_

#include <QGraphicsObject>
#include <QUuid>
#include <QPointer>
#include <QString>
#include <QList>
#include <QRectF>
#include <QDebug>

class QGraphicsSceneMouseEvent;

namespace	TouchTriggerType
{
	enum Enum
	{
		INVALID,
		Tap,
		TapAndHold,
		Flick
		//add other gesture types here as needed
	};
}


namespace RedirectingType
{
	enum Enum
	{
		INVALID,
		NoRedirect,
		Redirect,
		RedirectTargetMissing
	};
}

class Thing;
class RedirectContext;
class TouchRegister
{
public:
	TouchRegister() : valid(false) {}
	TouchRegister(int _id,TouchTriggerType::Enum _triggerType)
	: touchId(_id) , triggerType(_triggerType) , valid(true) , pause(false) , redirecting(false) , deferredCancel(false), qpRedirectTarget(0) , pRedirectContext(0) {}
	int							touchId;
	TouchTriggerType::Enum		triggerType;
	bool 						valid;				//when reading the 'register', if this is false, then all other values are invalid
	bool 						pause;
	bool						redirecting;
	bool						deferredCancel;
	QPointer<Thing>				qpRedirectTarget;
	RedirectContext	*			pRedirectContext;

	friend QDebug& operator<<(QDebug dbg,const TouchRegister& reg);
};

QDebug& operator<<(QDebug dbg,const TouchRegister& reg);

class RedirectContext : public QObject
{
	Q_OBJECT
public:
	RedirectContext() : m_valid(false) , m_dbg_onDestruct(false) {}
	virtual ~RedirectContext();
	virtual bool isValid() const { return m_valid; }
	bool m_valid;
	bool m_dbg_onDestruct;
};

class Thing : public QGraphicsObject
{
	Q_OBJECT

public:
	Thing();
	virtual ~Thing();
	virtual QUuid uid() const;
	virtual QRectF geometry() const = 0;

	// every "thing" must at least consider a resize, which is why i am making it pure-v.
	// the return value can let the item indicate 'false' if it wants to flag that it can't resize to
	// the new size. The upstream ui elements can act on or ignore that as desired
	virtual bool resize(quint32 newWidth,quint32 newHeight) = 0;

	//called by another Thing on 'this' (me), to offer p_offer for the taking.
	// if this wants it, it will initiate a take() sequence immediately. It is expected that
	// offeringThing will allow the take if it called offer.
	// returns false if this doesn't want the offered thing.
	// returns true at the end of the take sequence (actually, returns the return of take())
	virtual bool offer(Thing * p_offer,Thing * p_offeringThing);

	//called to transfer this (my) Thing's belongs-to to another Thing
	// e.g. icons between pages, etc
	// returns false if rejected
	virtual bool take(Thing * p_takerThing);

	//called by the Thing that I own, to tell me some other Thing is trying to take it
	// I can return "false" to prevent the abduction
	virtual bool taking(Thing * p_victimThing, Thing * p_takerThing);

	//called by the Thing that I used to own, to tell me some other Thing took it
	virtual void taken(Thing * p_takenThing,Thing * p_takerThing);

	virtual uint hashValue() const;
	friend uint qHash(const Thing& t);

	//Since I want to avoid multiple inheriting (c++) at all costs, going to give Thing-s the ability to accept touch events
	//	at least from other Thing-s

	virtual void touchTrackedPointStarted(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void redirectTouchPrepare(Thing * p_sourceThing,const TouchRegister& touchRegister,int contextHint=0);
	virtual void redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);
	virtual void redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);

Q_SIGNALS:

	//provided so that owners of Thing-s can hook onto a signal w/o caring about subclasses
	void signalThingGeometryChanged(const QRectF&);

public Q_SLOTS:

	// TODO: REMOVE: made its way in here for some incorrect reason...
	// These two toggle this item's painting and repainting of itself as a part of the QGraphics View system
	// then AutoRepaint is disabled, the item will only be painted when its paint function is called explicitly
	// By default, the base class impl. does nothing
	virtual void slotEnableIconAutoRepaint();
	virtual void slotDisableIconAutoRepaint();

protected:

	Thing(const QUuid& specificUid);

//	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
//	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
//	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

protected:

	QPointer<Thing>	m_qp_takerOwner;	//the last Thing that performed a successful take() on this Thing

private:

	QUuid 	m_uid;

};

typedef QList<Thing *> ThingList;
typedef ThingList::const_iterator ThingListConstIter;
typedef ThingList::iterator ThingListIter;

uint qHash(const Thing& t);
uint qHash(const QPointer<Thing>& qpt);

#include <QHash>

#endif /* THING_H_ */
