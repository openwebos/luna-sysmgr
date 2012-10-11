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




#include "page.h"
#include "pagemovement.h"
#include "dimensionsmain.h"
#include "dimensionslauncher.h"
#include "dimensionsglobal.h"
#include "debugglobal.h"
#include "pixmapobject.h"
#include "gfxsettings.h"
#include "layoutsettings.h"
#include "dynamicssettings.h"
#include "layoutitem.h"
#include "icon.h"
#include "iconlayout.h"
#include "propertysettingsignaltransition.h"
#include "Settings.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QDebug>

#include <QEvent>
#include <QTouchEvent>
#include <QGesture>
#include <QGestureEvent>
#include <QPropertyAnimation>
#include <QAnimationGroup>
#include <QParallelAnimationGroup>

#include <QState>
#include <QStateMachine>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#include "Localization.h"

//TEST:////////////// test ////////////////
#define PAGE_ICONFRAME_FILEPATH	QString("icon-bg-small-light.png")
#define PAGE_ICON_FILEPATH		QString("icon-maps.png")
/////////////////// end

#define PAGE_TOP_SHADOW_PIX_FILEPATH QString("tab-shadow.png")
#define PAGE_BOTTOM_SHADOW_PIX_FILEPATH QString("quicklaunch-shadow.png")

const char * Page::PageNamePropertyName = "pagename";
const char * Page::PageIndexPropertyName = "pageuiindex";
const char * Page::PageDesignatorPropertyName = "pagedesignator";

QString Page::PageDesignator_AllAlphabetic = QString("all");
QString Page::PageDesignator_Favorites = QString("favorites");

uint qHash(const QPointer<Page>& p)
{
	if (p)
	{
		return qHash(p->uid().toString());
	}
	return 0;
}

Page::Page(const QRectF& pageGeometry,LauncherObject * p_belongsTo)
: ThingPaintable(pageGeometry)
, m_pageUiIndex(0)
, m_pageActive(false)
, m_qp_currentUIOwner(p_belongsTo)
, m_qp_backgroundPmo(0)
, m_p_scroll(0)
, m_qp_ensembleAnimation(0)
, m_scrollDirectionLock(PageScrollDirectionLock::None)
, m_scrollLockTouchId(0)
, m_qp_iconLayout(0)
, m_p_touchFSM(0)
, m_p_scrollDelayFSM(0)
, m_interactionsBlocked(false)
{
	if (m_qp_currentUIOwner)
		this->setParentItem(m_qp_currentUIOwner);

	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture(Qt::TapAndHoldGesture);
	grabGesture(Qt::TapGesture);

//	m_normalKineticFriction = scrollPhysicsControl.coeffKineticFriction();
//	scrollPhysicsControl.setAutocount(0.1);
//	connect(&m_scrollPhysicsTimer,SIGNAL(timeout()),
//			&scrollPhysicsControl,SLOT(slotTimetic()));
//	connect(&scrollPhysicsControl,SIGNAL(signalRecomputed(qreal,qreal,qreal,qreal)),
//			this,SLOT(slotScrollingPhysicsUpdate(qreal,qreal,qreal,qreal)));

	m_topBorderShadowPixmap = QPixmap(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory
			+ PAGE_TOP_SHADOW_PIX_FILEPATH);
	m_bottomBorderShadowPixmap = QPixmap(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory
			+ PAGE_BOTTOM_SHADOW_PIX_FILEPATH);

	m_topBorderShadowArea = areaTopEdgeShadow();
	m_bottomBorderShadowArea = areaBottomEdgeShadow();

	m_topBorderShadowBrush = QBrush(m_topBorderShadowPixmap);
	m_bottomBorderShadowBrush = QBrush(m_bottomBorderShadowPixmap);

	connect(&m_failmaticScroller,SIGNAL(scrolled(qreal)),
			this,SLOT(slotKineticScrollerSpew(qreal)));

	setAcceptTouchEvents(true);
	setupTouchFSM();
	startTouchFSM();

	setupScrollDelayFSM();
	startScrollDelayFSM();

}

Page::Page(const QUuid& specificUid,const QRectF& pageGeometry,LauncherObject * p_belongsTo)
: ThingPaintable(specificUid,pageGeometry)
, m_pageActive(false)
, m_qp_currentUIOwner(p_belongsTo)
, m_qp_backgroundPmo(0)
, m_p_scroll(0)
, m_qp_ensembleAnimation(0)
, m_scrollDirectionLock(PageScrollDirectionLock::None)
, m_qp_iconLayout(0)
, m_p_touchFSM(0)
, m_p_scrollDelayFSM(0)
, m_interactionsBlocked(false)
{
	if (m_qp_currentUIOwner)
		this->setParentItem(m_qp_currentUIOwner);

	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture(Qt::TapAndHoldGesture);
	grabGesture(Qt::TapGesture);

	m_topBorderShadowPixmap = QPixmap(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory
			+ PAGE_TOP_SHADOW_PIX_FILEPATH);
	m_bottomBorderShadowPixmap = QPixmap(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory
			+ PAGE_BOTTOM_SHADOW_PIX_FILEPATH);

	m_topBorderShadowArea = areaTopEdgeShadow();
	m_bottomBorderShadowArea = areaBottomEdgeShadow();

	m_topBorderShadowBrush = QBrush(m_topBorderShadowPixmap);
	m_bottomBorderShadowBrush = QBrush(m_bottomBorderShadowPixmap);

	connect(&m_failmaticScroller,SIGNAL(scrolled(qreal)),
			this,SLOT(slotKineticScrollerSpew(qreal)));

	setAcceptTouchEvents(true);
	setupTouchFSM();
	startTouchFSM();

	setupScrollDelayFSM();
	startScrollDelayFSM();

}

//virtual
Page::~Page()
{
	if (!m_qp_backgroundPmo)
	{
		delete m_qp_backgroundPmo;
	}
}

void Page::setPageName(const QString& v)
{
	m_pageName = v;
	setObjectName(v);
}

//static
QSize Page::PageSizeFromLauncherSize(quint32 launcherWidth,quint32 launcherHeight)
{

	QSize r = QSize(
			qBound((quint32)2,
					(quint32)DimensionsGlobal::roundDown((qreal)launcherWidth * LayoutSettings::settings()->pageSizePctLauncherRelative.width()),
					(quint32)launcherWidth),
			qBound((quint32)2,
					(quint32)DimensionsGlobal::roundDown((qreal)launcherHeight * LayoutSettings::settings()->pageSizePctLauncherRelative.height()),
					(quint32)launcherHeight)
	);

	//make evenly divisible (multiple of 2)
	r.setWidth(r.width() - (r.width() % 2));
	r.setHeight(r.height() - (r.height() % 2));
	return r;

}

//virtual
bool Page::resize(quint32 w, quint32 h) 	// called by the ui owner when the ui itself resizes
{
	//stop physics
//	m_scrollPhysicsTimer.stop();
//	scrollPhysicsControl.deactivate();

	//force stop any animation running
	delete m_qp_ensembleAnimation;

	//compute the new geometry
	m_geom = DimensionsGlobal::realRectAroundRealPoint(QSize(w,h));
	m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);

	if (m_qp_backgroundPmo)
	{
		m_qp_backgroundPmo->resize(w,h);
	}

	m_topBorderShadowArea = areaTopEdgeShadow();
	m_bottomBorderShadowArea = areaBottomEdgeShadow();

	prepareGeometryChange();
	return true;
}

//virtual
bool Page::take(Thing * p_takerThing)
{
	return true;
}

//virtual
bool Page::taking(Thing * p_victimThing, Thing * p_takerThing)
{
	return true;
}

//virtual
void Page::taken(Thing * p_takenThing,Thing * p_takerThing)
{
}

//virtual
bool Page::canAcceptIcons() const
{
	return false;
}

//virtual
bool Page::acceptIncomingIcon(IconBase * p_newIcon)
{
	return false;
}

//virtual
bool Page::releaseTransferredIcon(IconBase * p_transferredIcon)
{
	return false;
}

//virtual
bool Page::layoutFromItemList(const ThingList& items)
{
	return false;
}

//virtual
bool Page::layoutFromItemList(const IconList& items)
{
	return false;
}

//virtual
bool Page::setIconLayout(IconLayout * p_iconLayout)
{
	if (p_iconLayout == m_qp_iconLayout.data())
	{
		return true;
	}

	if (m_qp_iconLayout)
	{
		delete m_qp_iconLayout;
	}
	m_qp_iconLayout = p_iconLayout;
	return true;
}

//virtual
IconLayout * Page::currentIconLayout() const
{
	return m_qp_iconLayout;
}

IconBase * Page::iconAtLayoutCoordinate(const QPointF& layoutCoord,QPointF * r_p_intraIconCoord)
{
	QPointF pt;
	if (m_p_scroll)
	{
		if (!m_p_scroll->mapToContentSpace(layoutCoord,pt))
		{
			return 0;
		}
	}
	else
	{
		//no scroller, point is 1:1 to layout
		// TODO: consider offsets, etc
		pt = layoutCoord;
	}

	IconCell * p_iconCell = m_qp_iconLayout->iconCellAtLayoutCoordinate(pt);
	if (p_iconCell)
	{
		if (r_p_intraIconCoord)
		{
			*r_p_intraIconCoord = pt - p_iconCell->relativeGeometry().center();
		}
		if (p_iconCell->m_qp_icon)
		{
			return p_iconCell->m_qp_icon;
		}
		else
		{
			return 0;
		}
	}
	return 0;
}

//virtual
QPointF Page::pageCoordinateFromLayoutCoordinate(const QPointF& layoutCoord)
{
	QPointF pt;
	if (m_p_scroll)
	{
		return mapFromItem(m_p_scroll,m_p_scroll->mapFromContentSpace(layoutCoord));
	}
	//no scroller, point is 1:1 to layout
	// TODO: consider offsets, etc
	return layoutCoord;
}

//virtual
QPointF	Page::layoutCoordinateFromPageCoordinate(const QPointF& pageCoordinate)
{

	QPointF pt;
	if (m_p_scroll)
	{
		if (!m_p_scroll->mapToContentSpace(mapToItem(m_p_scroll,pageCoordinate),pt))
		{
			return pageCoordinate;
		}
		return pt;
	}
	else
	{
		//no scroller, point is 1:1 to layout
		// TODO: consider offsets, etc
		return pageCoordinate;
	}
	return QPointF();	//never reached

}

//virtual
QRectF Page::areaScroller() const
{
	if (m_p_scroll)
	{
		return m_p_scroll->positionRelativeGeometry();
	}
	return QRectF();
}

//virtual
QRectF Page::areaTopEdgeShadow() const
{
	if (!m_topBorderShadowPixmap.isNull())
	{
		if (LayoutSettings::settings()->openglHatesOddNumbers && (qAbs((qint32)m_geom.topLeft().y()) % 2))
		{
			return QRectF(m_geom.topLeft()-QPointF(0.0,1.0),
										QSizeF((qreal)(m_geom.width()),(qreal)(m_topBorderShadowPixmap.height())));
		}
		else
		{
			return QRectF(m_geom.topLeft(),
										QSizeF((qreal)(m_geom.width()),(qreal)(m_topBorderShadowPixmap.height())));
		}
	}
	return QRectF();
}

//virtual
QRectF Page::areaBottomEdgeShadow() const
{
	if (!m_bottomBorderShadowPixmap.isNull())
	{
		if (LayoutSettings::settings()->openglHatesOddNumbers && (qAbs((qint32)m_geom.bottomLeft().y()) % 2))
		{
			return QRectF(m_geom.bottomLeft()-QPointF(0,1.0)-QPointF(0,(qreal)m_bottomBorderShadowPixmap.height()),
					QSizeF(m_geom.width(),(qreal)m_bottomBorderShadowPixmap.height()));
		}
		else
		{
			return QRectF(m_geom.bottomLeft()-QPointF(0,(qreal)m_bottomBorderShadowPixmap.height()),
					QSize(m_geom.width(),m_bottomBorderShadowPixmap.height()));
		}
	}
	return QRectF();
}

QRectF Page::areaLeftBorder() const
{
	int borderWidth = LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.width();
	if (borderWidth > geometry().width()/2)
	{
		borderWidth = geometry().width()/2;
	}

	return QRectF(geometry().topLeft(),QSize(borderWidth,geometry().size().toSize().height()));
}

QRectF Page::areaRightBorder() const
{
	int borderWidth = LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.height();		//it's not actually "height". see layoutsettings.cpp
	if (borderWidth > geometry().width()/2)
	{
		borderWidth = geometry().width()/2;
	}

	return QRectF(geometry().topRight()-QPointF((qreal)borderWidth,0.0),
					QSize(borderWidth,geometry().size().toSize().height()));
}

QRectF Page::areaTopBorder() const
{
	int borderHeight = LayoutSettings::settings()->pageVerticalBorderActivationAreaSizePx.width();		//it's not actually "width"....
	if (borderHeight > geometry().height()/2)
	{
		borderHeight = geometry().height()/2;
	}

	return QRectF(geometry().topLeft(),
						QSize(geometry().size().toSize().width(),borderHeight));
}

QRectF Page::areaBottomBorder() const
{
	int borderHeight = LayoutSettings::settings()->pageVerticalBorderActivationAreaSizePx.height();
	if (borderHeight > geometry().height()/2)
	{
		borderHeight = geometry().height()/2;
	}

	return QRectF(geometry().bottomLeft()-QPointF(0.0,(qreal)borderHeight),
						QSize(geometry().size().toSize().width(),borderHeight));
}

QRectF Page::areaCenter() const
{
	//see comments above (re: "height" and "width")
	int borderWidth = LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.width() + LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.height();
	if (borderWidth > geometry().width()/2)
	{
		borderWidth = geometry().width()/2;
	}
	int borderHeight = LayoutSettings::settings()->pageVerticalBorderActivationAreaSizePx.height() + LayoutSettings::settings()->pageVerticalBorderActivationAreaSizePx.width();
	if (borderHeight > geometry().height()/2)
	{
		borderHeight = geometry().height()/2;
	}
	return QRectF(geometry().topLeft()+QPointF((qreal)borderWidth,(qreal)borderHeight),
					QSize(geometry().width() - 2*borderWidth,geometry().height() - 2*borderHeight));
}

PageAreas::Enum Page::classifyPageLocation(const QPointF& pageCoordinate) const
{
	//TODO: could be optimized
	//Since the areas could overlap, there is a precedence order...top/bottom are preferred over sides

	QRectF borderArea = areaCenter();
	if (borderArea.contains(pageCoordinate))
	{
		return PageAreas::Content;
	}

	borderArea = areaTopBorder();
	if (borderArea.contains(pageCoordinate))
	{
		return PageAreas::TopBorder;
	}
	borderArea = areaBottomBorder();
	if (borderArea.contains(pageCoordinate))
	{
		return PageAreas::BottomBorder;
	}

	borderArea = areaLeftBorder();
	if (borderArea.contains(pageCoordinate))
	{
		return PageAreas::LeftBorder;
	}
	borderArea = areaRightBorder();
	if (borderArea.contains(pageCoordinate))
	{
		return PageAreas::RightBorder;
	}
	return PageAreas::INVALID;
}

//virtual
void Page::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	if (m_qp_backgroundPmo)
		painter->drawPixmap(m_geom,*(*m_qp_backgroundPmo),(*m_qp_backgroundPmo)->rect());

	//TODO: this is actually wrong. It paints the shadow "under" the icons because the scroller/layout is a child of this QGItem so it
	//		gets painted on top of any painting here.
	//	Possible ways to fix:
	//	1. create a "shadow" QGItem, make it a child of this, and stack it on top of the scroller
	//	2. manually paint the shadows as done here, but paint it in the scroller (or as part of painting the layout)
	paintShadows(painter);
//	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,2);
}

//virtual
void Page::paintOffscreen(QPainter *painter)
{
	//TODO: implement
}

//virtual
void Page::paintShadows(QPainter * painter)
{
	//not sure if this is necessary
	QPoint sbo = painter->brushOrigin();
	if (!m_topBorderShadowPixmap.isNull())
	{
		painter->setBrushOrigin(m_topBorderShadowArea.topLeft());
		painter->fillRect(m_topBorderShadowArea,
				 m_topBorderShadowBrush);
//		DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_topBorderShadowArea,7);
	}
	if (!m_bottomBorderShadowPixmap.isNull())
	{
		painter->setBrushOrigin(m_bottomBorderShadowArea.topLeft());
		painter->fillRect(m_bottomBorderShadowArea,
				m_bottomBorderShadowBrush);
//		DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_bottomBorderShadowArea,7);
	}
	painter->setBrushOrigin(sbo);
	//DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom.adjusted(2.0,2.0,-2.0,-2.0),7);
}

/////////////////////// MOUSE, GESTURE, TOUCH, OTHER INPUT EVENTS ////////////////////////////

bool Page::sceneEvent(QEvent* event)
{

	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {

		if (!stopAnimationEnsemble())
		{
			//can't stop the animation, so bail here - SILENTLY CONSUME EVENT.
			// I don't want anyone else handling the event. The user will retry is shortly when the anim completes
			return true;
		}

		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g) {
			QTapGesture* tap = static_cast<QTapGesture*>(g);
			if (tap->state() == Qt::GestureFinished) {
				return tapGesture(tap,ge);
			}
		}
		g = ge->gesture(Qt::TapAndHoldGesture);
		if (g) {
			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
			if (hold->state() == Qt::GestureFinished) {
				return tapAndHoldGesture(hold,ge);
			};
		}
		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g) {
			FlickGesture* flick = static_cast<FlickGesture*>(g);
			if (flick->state() == Qt::GestureFinished) {
				return flickGesture(flick,ge);
			}
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
		qDebug() << __PRETTY_FUNCTION__ << " START";	
		return touchEndEvent(static_cast<QTouchEvent *>(event));
	}

	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool Page::swipeGesture(QSwipeGesture *swipeEvent,QGestureEvent * baseGestureEvent)
{
	return true;
}

////virtual
bool Page::flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent)
{
	//if nothing is tracking, reject
	int mainTouchId;
	if (anyTouchTracking(&mainTouchId))
	{
		//TODO: MULTI-TOUCH: here is where I can get into deep trouble; I've just assumed that the main touch point is part of this gesture. It need not be
		if (!okToUseFlick(mainTouchId))
		{
			return true;
		}
		//else fall through
	}

	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return true;
	}

	if(m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt() != -1 &&
	   !m_touchRegisters.contains(m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt())) {
		// this happens if we were tracking a finger that got removed from teh screen (but other are present).
		// Ignore flick gestures at this point.
		// TODO: add touch it to the FlickGesture events so we can decide based on the ID when to use
		// or drop a flick
		return true;
	}

	//if it was a fall-through, then it's Ok to use a flick with this registered touch point
	// else it's an un-registered flick; it happened after all touch points retired. This theoretically shouldn't be allowed to happen but Qt allows it to...
	// In my mind, a gesture recognizer should make sure a gesture event comes in BEFORE the touch-end event(s) for the touch point(s) that caused the gesture
	// ...but it doesn't seem to in Qt.4.7

	if (m_failmaticScroller.animatingFlick())
	{
		m_failmaticScroller.stopImmediately();
	}

	//if the flick velocity is horizontal, then the main ui needs to handle it; it's a page L/R flick
	// since the awesome gesture event system won't let me reject an event up to my parent when it's
	// already in GestureFinished, i have to resort to this signalling trickery
	// TODO: research Qt docs to see if events can somehow be bounced at GestureFinished
	if (qAbs(flickEvent->velocity().x()) > qAbs(flickEvent->velocity().y()))
	{
		// if we get a horizontal flick after we have locked into vertical movement, just ignore it
		if(m_scrollDirectionLock == PageScrollDirectionLock::Vertical)
			return true;

		Q_EMIT signalRedirectFlick(flickEvent,baseGestureEvent);

		return true;
	}

	if (m_p_scroll && m_scrollDirectionLock == PageScrollDirectionLock::Vertical) // only use flicks in the page if we have  locked into vertical movement
	{
		m_failmaticScroller.setScrollOffset((qreal)m_p_scroll->scrollValue());
		m_failmaticScroller.handleFlick(flickEvent->velocity().y());
	}

	return true;
}

//virtual
bool Page::okToUseFlick(int id)
{
	return testAndSetTriggerOnRegisterCreate(id,TouchTriggerType::INVALID,TouchTriggerType::Flick);
}

//virtual
bool Page::okToUseTap(int id)
{
	return testAndSetTriggerOnRegisterCreate(id,TouchTriggerType::INVALID,TouchTriggerType::Tap);
}

//virtual
bool Page::okToUseTapAndHold(int id)
{
	return testAndSetTriggerOnRegisterCreate(id,TouchTriggerType::INVALID,TouchTriggerType::TapAndHold);
}

//virtual
void Page::slotOkToScroll()
{
	//check and see if more autoscrolling is necessary...this would be called after an autoscroll caused a higher level action, like a page pan,
	//	which then finished and now leaves the Page wondering if it should continue scrolling like before, or simply at the end of an autoscroll delay.
	//TODO: IMPLEMENT
	//   to implement this, I need a notion of where the last touch position occurred...
	// actually more than that: because theoretically there can be multiple touch points, there should
	// be a system to track what (and when) the last special boundary area (e.g. top/bottom border, which causes autoscroll)
	// was (triggered). So if in the meantime while the FSM was delaying, a touch point moved to a different special
	// area, this needs to be reflected...as well as the simple case where the touch point that triggered the scroll that
	// led me here simply moved off of the special area that triggered the scroll

}

//virtual
void Page::slotLauncherCmdEndReorderMode()
{
	//nothing to do in the base; reimplement in pages that can be in reorder modes
}

//virtual
QPointF Page::translatePagePointToScrollSurfacePoint(const QPointF& point,bool clipToGeom)
{
	if ((m_p_scroll) && clipToGeom)
	{
		QPointF mapPt = mapToItem(m_p_scroll,point);
		return (QPointF(qBound(m_p_scroll->geometry().left(),mapPt.x(),m_p_scroll->geometry().right()),
						qBound(m_p_scroll->geometry().top(),mapPt.y(),m_p_scroll->geometry().bottom())));
	}
	else if (m_p_scroll)
	{
		return (mapToItem(m_p_scroll,point));
	}
	return point;
}

//virtual
void Page::slotLauncherBlockedInteractions()
{
	m_interactionsBlocked = true;
// 	clearAndResetAllTouch();
}

//virtual
void Page::slotLauncherAllowedInteractions()
{
	m_interactionsBlocked = false;
//	clearAndResetAllTouch();
}

//virtual
void Page::slotClearTouchRegisters()
{
	//DO NOT CALL THIS ARBITRARILY! It is tied in subtle ways to the touch FSM
	qDebug() << __PRETTY_FUNCTION__ << " END";	
	clearAllTouchRegisters();
}

//virtual
void Page::slotLauncherActivating()
{
}

//virtual
void Page::slotLauncherDeactivating()
{
	clearAllTouchRegisters();
}

//virtual
bool Page::testAndTranslatePagePointToScrollSurfacePoint(const QPointF& point,QPointF& r_translatedPoint)
{
	if (m_p_scroll)
	{
		r_translatedPoint = mapToItem(m_p_scroll,point);
		if (m_p_scroll->geometry().contains(r_translatedPoint))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	r_translatedPoint = point;
	return true;
}

//////////////////////////// ANIMATIONS ///////////////////////////////////////////

void Page::slotAnimationEnsembleFinished()
{
	//walk the animation tree and figure out what finished
	animationFinishedProcessGroup(m_qp_ensembleAnimation);
}

void Page::slotStopAnimationEnsemble(bool * r_result)
{
	if (r_result)
		*r_result = stopAnimationEnsemble();
	else
		(void)stopAnimationEnsemble();
}


bool Page::isAnimationEnsembleRunning() const
{
	if (m_qp_ensembleAnimation)
	{
		if (m_qp_ensembleAnimation->state() != QAbstractAnimation::Stopped)
		{
			return true;
		}
	}
	return false;
}

bool Page::stopAnimationEnsemble()
{
	if (!m_qp_ensembleAnimation.isNull())
	{
		if (m_qp_ensembleAnimation->property("canInterrupt").toBool())
			m_qp_ensembleAnimation->stop();
		else
		{
			return false;
		}
	}
	return true;
}

void Page::slotStartAnimationEnsemble(bool canInterrupt)
{
	if (!m_qp_ensembleAnimation.isNull())
	{
		(void)connect(m_qp_ensembleAnimation,SIGNAL(finished()),SLOT(slotAnimationEnsembleFinished()));
		m_qp_ensembleAnimation->setProperty("canInterrupt",QVariant(canInterrupt));
		m_qp_ensembleAnimation->start(QAnimationGroup::DeleteWhenStopped);
	}
}

void Page::slotAddAnimationToEnsemble(QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType)
{
	if (m_qp_ensembleAnimation.isNull())
		m_qp_ensembleAnimation = new QParallelAnimationGroup();
	slotAddAnimationTo(m_qp_ensembleAnimation,p_anim,animType);

}
void Page::slotAddAnimationTo(QAnimationGroup * p_addToGroup,QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType)
{
	if ((!p_anim) || (!p_addToGroup))
		return;

	p_anim->setProperty("diui_animtype",animType);
	p_addToGroup->addAnimation(p_anim);

}

void Page::animationFinishedProcessGroup(QAnimationGroup * pAnim)
{
	if (!pAnim)
		return;
	for (int i=0;i < pAnim->animationCount();++i)
		animationFinishedProcessAnim(pAnim->animationAt(i));
}

void Page::animationFinishedProcessAnim(QAbstractAnimation * pAnim)
{
	if (!pAnim)
		return;
	if (qobject_cast<QAnimationGroup *>(pAnim))
		return animationFinishedProcessGroup(qobject_cast<QAnimationGroup *>(pAnim));

	QPropertyAnimation * pPropAnim=0;

	DimensionsTypes::AnimationType::Enum animType = (DimensionsTypes::AnimationType::Enum)(pAnim->property("diui_animtype").toInt());
	switch (animType)
	{
	case DimensionsTypes::AnimationType::PageVScroll:
		pPropAnim = qobject_cast<QPropertyAnimation *>(pAnim);
		if (pPropAnim)
		{
			qDebug() << "finished: scroll is now: " << m_p_scroll->scrollValue();
		}
		break;
	default:
		break;
	}
}

//////////////////////////// OTHER ////////////////////////////////////////////////

//virtual
void Page::slotPageOverscrolled()
{
	//bring it back, like the mouse release handler
	return slotUndoOverscroll();
}

//virtual
void Page::slotUndoOverscroll()
{
	if (!stopAnimationEnsemble())
	{
		//can't stop the animation, so bail here - SILENTLY CONSUME EVENT.
		// I don't want anyone else handling the event. The user will retry is shortly when the anim completes
		return;
	}

	if (m_failmaticScroller.animatingFlick())
	{
		//flick is going...so it's pointless to try and undo overscroll
		return;
	}

	// if really in overscroll, bring it back!
	if (m_p_scroll)
	{
		qint32 overscrollCorrection = m_p_scroll->scrollValueNeededToEscapeOverscroll();
		if (overscrollCorrection == 0)
		{
			//none needed
			return;
		}

		//make a prop anim to return the scroll to normal value.
		QPropertyAnimation * pOverscrollAnim = new QPropertyAnimation(m_p_scroll,"scroll");
		pOverscrollAnim->setEndValue(m_p_scroll->scrollValue() + overscrollCorrection);
		slotAddAnimationToEnsemble(pOverscrollAnim,DimensionsTypes::AnimationType::PageVScroll);
		slotStartAnimationEnsemble(false);		//uninterruptible!
	}
}

//virtual
void Page::slotBroadcastPageActivated(QUuid pageUid)
{
	if (pageUid == uid())
	{
		activatePage();
	}
}

//virtual
void Page::slotBroadcastAllPagesDeactivated()
{
	deactivatePage();
}

//virtual
void Page::activatePage()
{
	m_pageActive = true;
	Q_EMIT signalPageActive();
}

//virtual
void Page::deactivatePage()
{
	m_pageActive = false;
	Q_EMIT signalPageDeactivated();
}

//virtual
void Page::slotScrollingPhysicsUpdate(qreal t,qreal d,qreal v,qreal a)
{

//	//triggers on a periodic timer-triggered physics recalc
//	m_p_scroll->setScrollValue(d);
//	if ( (t > scrollPhysicsControl.haltTime()) || (m_p_scroll->scrollValueNeededToEscapeOverscroll()))
//	{
//		m_scrollPhysicsTimer.stop();
//		if (m_p_scroll->scrollValueNeededToEscapeOverscroll())
//		{
//			//no need to emit; it's local
//			slotPageOverscrolled();
//		}
//	}
}

//virtual
void Page::slotKineticScrollerSpew(qreal oldScroll)
{
	m_p_scroll->setScrollValue(m_failmaticScroller.scrollOffset());	//(can you tell i think this is a bad idea?)
}

/////////////////////////// Touch FSM related /////////////////////////////////////////////

//helper
static QState* createState(QString name, QState *parent=0)
{
	QState *result = new QState(parent);
	result->setObjectName(name);
	return result;
}

//statics, see .h file
const char * Page::TouchFSMPropertyName_isTracking = "isTracking";
const char * Page::TouchFSMProperyName_trackedId = "trackedId";
const char * Page::ScrollDelayFSMPropertyName_isScrollOk = "isScrollOk";

//virtual
bool Page::anyTouchTracking(int * r_p_mainTouchId)
{
	// make sure we're still tracking our initial touch point
	if (m_p_touchFSM->property(TouchFSMPropertyName_isTracking).toBool() && (m_touchRegisters.contains(m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt())))
	{
		if (r_p_mainTouchId)
		{
			*r_p_mainTouchId = m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt();
		}
		return true;
	}
	return false;
}

//virtual
bool Page::touchPointTriggerType(int id,TouchTriggerType::Enum& r_type)
{
	QMap<int,TouchRegister>::const_iterator f = m_touchRegisters.constFind(id);
	if (f != m_touchRegisters.constEnd())
	{
		if (f.value().valid)
		{
			r_type = f.value().triggerType;
			return true;
		}
	}
	return false;
}

//virtual
bool Page::testAndSetTriggerOnRegister(int id,TouchTriggerType::Enum conditionV,TouchTriggerType::Enum setV)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if ((f.value().valid) && (f.value().triggerType == conditionV))
		{
			f.value().triggerType = setV;
			return true;
		}
	}
	return false;
}

//virtual
bool Page::testAndSetTriggerOnRegisterCreate(int id,TouchTriggerType::Enum conditionV,TouchTriggerType::Enum setV)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if ((f.value().valid) && (f.value().touchId == id) && (f.value().triggerType == conditionV))		//hmmm, register is keyed on id, which is what find() just found, so is the id == check needed?
		{
			//already set case - test pass
			return true;
		}
		else
		{
			//there is a register and it's got different values than specified, so test is a fail
			return false;
		}
	}
	//nothing there, so test is a pass implicitly, with the values being set
	m_touchRegisters[id] = TouchRegister(id,setV);
	return true;
}

//virtual
RedirectingType::Enum Page::isRedirecting(int id,Thing ** r_pp_redirectTargetThing,RedirectContext ** r_pp_redirContext)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
//		qDebug() << __FUNCTION__ << ": id " << id << " : " << QString(objectName()) << ": register found, valid = " << f.value().valid << " , redirecting = " << f.value().redirecting;
		if ((f.value().valid) && (f.value().redirecting))
		{
			if (r_pp_redirectTargetThing)
			{
				*r_pp_redirectTargetThing = f.value().qpRedirectTarget;
			}
			if (r_pp_redirContext)
			{
				*r_pp_redirContext = f.value().pRedirectContext;
			}
			if (f.value().qpRedirectTarget)
			{
				return RedirectingType::Redirect;
			}
			else
			{
				return RedirectingType::RedirectTargetMissing;
			}
		}
	}
	else
	{
		qDebug() << __FUNCTION__ << ": id " << id << " : " << QString(objectName()) << ": register not found";
	}
	return RedirectingType::NoRedirect;
}

//virtual
bool Page::isDeferCancelled(int id,bool doCancel)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
//		qDebug() << __FUNCTION__ << ": id " << id << " : " << QString(objectName()) << ": register found, valid = " << f.value().valid << " , redirecting = " << f.value().redirecting;
		if ((f.value().valid) && (f.value().deferredCancel))
		{
			if (doCancel)
			{
				f.value().deferredCancel=false;
				f.value().redirecting = false;
				f.value().qpRedirectTarget = 0;
				if (f.value().pRedirectContext)
				{
					delete f.value().pRedirectContext;
					f.value().pRedirectContext = 0;
				}
			}
			return true;
		}
	}
	else
	{
		qDebug() << __FUNCTION__ << ": id " << id << " : " << QString(objectName()) << ": register not found";
	}
	return false;
}

//virtual
bool Page::redirectTo(int id,Thing * p_redirectTargetThing,RedirectContext * p_redirContext)
{
	//if already redirecting somewhere else, then fail.
	//TODO: possibly a way to allow these situations too; it would probably need a way to alert the previous redirectee that
	//		no more redirects are coming
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if (
				( !f.value().valid)
				|| ( (f.value().redirecting) && (f.value().qpRedirectTarget) && (f.value().qpRedirectTarget != p_redirectTargetThing))
			)
		{
			//already redirecting elsewhere, or an invalid register
//			qDebug() << __FUNCTION__ << ": id " << id << " : " << QString(objectName()) << ": redirecting elsewhere/invalid!?";
			return false;
		}
		TouchRegister& reg = f.value();

		reg.redirecting = true;
		reg.qpRedirectTarget = p_redirectTargetThing;
		if (reg.pRedirectContext)
		{
			delete reg.pRedirectContext;
		}
		reg.pRedirectContext = p_redirContext;
//		qDebug() << __FUNCTION__ << ": id " << id << " : " << QString(objectName()) << ": redirecting to " << QString(f.value().qpRedirectTarget->objectName());
		return true;
	}
//	qDebug() << __FUNCTION__ << ": id " << id << " : "  << QString(objectName()) << ": not found";
	return false;	//not found
}

///TODO: unsafe...make it more so.
//virtual
bool Page::changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext)
{
	if (this == p_newRedirectedToThing)
	{
		//self-redirection is a cancel
		qDebug() << __FUNCTION__ << "--------- CANCELLED REDIRECTIONS!!!";
		return cancelRedirection(id);
	}

	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if ( !f.value().valid)
		{
			//invalid register
			return false;
		}
		f.value().redirecting = true;
		f.value().qpRedirectTarget = p_newRedirectedToThing;
		if (f.value().pRedirectContext)
		{
			delete f.value().pRedirectContext;
		}
		f.value().pRedirectContext = p_redirContext;
		return true;
	}
	return false;	//not found
}

//virtual
bool Page::cancelRedirection(int id,bool deferred)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if (deferred)
		{
			//don't explicitly destroy the context. just mark this to be killed later
			f.value().deferredCancel = true;
		}
		else
		{
			f.value().redirecting = false;
			f.value().qpRedirectTarget = 0;
			if (f.value().pRedirectContext)
			{
				delete f.value().pRedirectContext;
				f.value().pRedirectContext = 0;
			}
		}
	}
	return true;	//not found - so it's not redirecting by definition
}

//virtual
void Page::setupTouchFSM()
{

	if (m_p_touchFSM)
	{
		qDebug() << __FUNCTION__ << ": attempting to setup a new touch FSM when one already exists! Bad! ignoring...";
		return;
	}
	m_p_touchFSM = new QStateMachine(this);
	m_p_touchFSM->setObjectName("touchfsm");

	m_p_fsmStateNoTouch           		= createState("touchfsmstate_notouch");
	m_p_fsmStateTouchMotionTracking     = createState("touchfsmstate_motion");
	m_p_fsmStateTouchStationaryTracking = createState("touchfsmstate_stationary");

	// ------------------- STATE: touchfsmstate_notouch -----------------------------------
	//	touchfsmstate_notouch PROPERTIES
	m_p_fsmStateNoTouch->assignProperty(m_p_touchFSM,TouchFSMPropertyName_isTracking, false);
	m_p_fsmStateNoTouch->assignProperty(m_p_touchFSM,TouchFSMProperyName_trackedId, -1);
	connect(m_p_fsmStateNoTouch,SIGNAL(propertiesAssigned()), SLOT(slotClearTouchRegisters()));
	connect(m_p_fsmStateNoTouch, SIGNAL(propertiesAssigned()), SLOT(dbg_slotStatePropertiesAssigned()));

	//  touchfsmstate_notouch TRANSITIONS
	PropertySettingSignalTransition * pTransition =
			new PropertySettingSignalTransition(this,SIGNAL(signalTouchFSMTouchBegin_Trigger(int)),
												m_p_touchFSM,QString(TouchFSMProperyName_trackedId));
	pTransition->setTargetState(m_p_fsmStateTouchMotionTracking);
	m_p_fsmStateNoTouch->addTransition(pTransition);

	// ------------------- STATE: touchfsmstate_motion ------------------------------------
	// touchfsmstate_motion PROPERTIES
	m_p_fsmStateTouchMotionTracking->assignProperty(m_p_touchFSM,TouchFSMPropertyName_isTracking, true);
	connect(m_p_fsmStateTouchMotionTracking, SIGNAL(propertiesAssigned()), SLOT(dbg_slotStatePropertiesAssigned()));

	// touchfsmstate_motion TRANSITIONS
	m_p_fsmStateTouchMotionTracking->addTransition(this, SIGNAL(signalTouchFSMTouchEnd_Trigger()),m_p_fsmStateNoTouch);

	m_p_touchFSM->addState(m_p_fsmStateNoTouch);
	m_p_touchFSM->addState(m_p_fsmStateTouchMotionTracking);
	m_p_touchFSM->addState(m_p_fsmStateTouchStationaryTracking);

	m_p_touchFSM->setInitialState(m_p_fsmStateNoTouch);
}

//virtual
void Page::startTouchFSM()
{
	if (!m_p_touchFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot start; FSM does not exist";
		return;
	}
	if (m_p_touchFSM->isRunning())
	{
		qDebug() << __FUNCTION__ << ": Already running";
		return;
	}
	m_p_touchFSM->setInitialState(m_p_fsmStateNoTouch);
	m_p_touchFSM->start();
}

//virtual
void Page::stopTouchFSM()
{
	if (!m_p_touchFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot stop; FSM does not exist";
		return;
	}
	m_p_touchFSM->stop();
}

////virtual
void Page::clearAllTouchRegisters()
{
	for (QMap<int,TouchRegister>::iterator it = m_touchRegisters.begin();
			it != m_touchRegisters.end();++it)
	{
		TouchRegister& reg = it.value();
		if (reg.pRedirectContext)
		{
			delete reg.pRedirectContext;
			reg.pRedirectContext = 0;
		}
	}
	m_touchRegisters.clear();
}

//virtual
void Page::setupScrollDelayFSM()
{
	if (m_p_scrollDelayFSM)
	{
		qDebug() << __FUNCTION__ << ": attempting to setup a new touch FSM when one already exists! Bad! ignoring...";
		return;
	}
	m_p_scrollDelayFSM = new QStateMachine(this);
	m_p_scrollDelayFSM->setObjectName("scrolldelayfsm");

	m_p_fsmStateScrollOk           		= createState("scroll_ok");
	m_p_fsmStateScrollDelay     = createState("scroll_delay");

	m_scrollFSMTimer.setSingleShot(true);
	m_scrollFSMTimer.setInterval(DynamicsSettings::settings()->pageScrollDelayMs);

	// ------------------- STATE: scroll_ok -----------------------------------
	//	scroll_ok PROPERTIES
	m_p_fsmStateScrollOk->assignProperty(m_p_scrollDelayFSM,ScrollDelayFSMPropertyName_isScrollOk, true);
	//  scroll_ok TRANSITIONS
	m_p_fsmStateScrollOk->addTransition(this,SIGNAL(signalScrollDelayFSM_UserScrolled()),m_p_fsmStateScrollDelay);
	//  scroll_ok SIDE-EFFECTS
	connect(m_p_fsmStateScrollOk, SIGNAL(propertiesAssigned()), SLOT(slotOkToScroll()));
	//TODO: DEBUG:
	connect(m_p_fsmStateScrollOk, SIGNAL(propertiesAssigned()), SLOT(dbg_slotStatePropertiesAssigned()));

	// ------------------- STATE: scroll_delay ------------------------------------
	// scroll_delay PROPERTIES
	m_p_fsmStateScrollDelay->assignProperty(m_p_scrollDelayFSM,ScrollDelayFSMPropertyName_isScrollOk, false);
	connect(m_p_fsmStateScrollDelay, SIGNAL(propertiesAssigned()),&m_scrollFSMTimer,SLOT(start()));
	// scroll_delay TRANSITIONS
	m_p_fsmStateScrollDelay->addTransition(&m_scrollFSMTimer, SIGNAL(timeout()),m_p_fsmStateScrollOk);
	//  scroll_delay SIDE-EFFECTS
	//TODO: DEBUG:
	connect(m_p_fsmStateScrollDelay, SIGNAL(propertiesAssigned()), SLOT(dbg_slotStatePropertiesAssigned()));

	m_p_scrollDelayFSM->addState(m_p_fsmStateScrollOk);
	m_p_scrollDelayFSM->addState(m_p_fsmStateScrollDelay);

	m_p_scrollDelayFSM->setInitialState(m_p_fsmStateScrollOk);
}

//virtual
void Page::startScrollDelayFSM()
{
	if (!m_p_scrollDelayFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot start; FSM does not exist";
		return;
	}
	if (m_p_scrollDelayFSM->isRunning())
	{
		m_p_scrollDelayFSM->stop();
	}
	m_p_scrollDelayFSM->setInitialState(m_p_fsmStateScrollOk);
	m_p_scrollDelayFSM->start();
}

//virtual
void Page::stopScrollDelayFSM()
{
	if (!m_p_scrollDelayFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot stop; FSM does not exist";
		return;
	}
	m_p_scrollDelayFSM->stop();
}

//virtual
void Page::scrollActionTrigger()
{
	Q_EMIT signalScrollDelayFSM_UserScrolled();
}

//virtual
bool Page::okToPerformAutoScroll() const
{
	return m_p_scrollDelayFSM->property(ScrollDelayFSMPropertyName_isScrollOk).toBool();
}

//virtual
bool Page::touchStartEvent(QTouchEvent *event)
{
	//  3-31-2011: just disable multitouch for now
	if (anyTouchTracking())
	{
		return false;
	}

	event->accept();
	if (event->touchPoints().isEmpty())
	{
		return true;
	}

	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return true;
	}

	if (m_failmaticScroller.animatingFlick())
	{
		m_failmaticScroller.stopImmediately();
	}

	//TODO: MULTI-TOUCH:
	//	assume that multiple touch starts can't happen for a single id
	//

	QTouchEvent::TouchPoint touchPoint = event->touchPoints().first();
	int id = touchPoint.id();
	m_touchRegisters[id] = TouchRegister(id,TouchTriggerType::INVALID);	//INVALID marks the register as not having a specific gesture yet, not that the reg itself is invalid
	Q_EMIT signalTouchFSMTouchBegin_Trigger(id);

	//call the touch start function
	//virtual void touchTrackedPointStarted(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	touchTrackedPointStarted(id,touchPoint.scenePos(),touchPoint.lastScenePos(),touchPoint.startScenePos());
	return true;
}

//virtual
bool Page::touchUpdateEvent(QTouchEvent *event)
{
	bool foundTrackedFinger = false;

	event->accept();

	if (!anyTouchTracking())
	{
		//fsm says not tracking, so I don't recognize this... swallow it
		return true;
	}

	//TODO: MULTI-TOUCH
	//dig out the touch point which is being tracked
	QList<QTouchEvent::TouchPoint> points = event->touchPoints();
	for (QList<QTouchEvent::TouchPoint>::iterator it = points.begin();
			it != points.end();++it)
	{
		if (it->id() == m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt())
		{
			if(it->state() == Qt::TouchPointReleased) {
				handleTrackedTouchPointRelease(&(*it));
			} else {
				//found it...
				//if it is being redirected, then keep redirecting it, but if the target got destroyed, then just silently swallow the events
				Thing * pRedirectTarget = 0;
				RedirectContext * pRedirContext = 0;
				RedirectingType::Enum redirType = isRedirecting(it->id(),&pRedirectTarget,&pRedirContext);
				if (redirType == RedirectingType::Redirect)
				{
					if (pRedirContext)
					{
						pRedirectTarget->redirectedTouchTrackedPointMoved(this,it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos(),*pRedirContext);
					}
					else
					{
						RedirectContext nullContext;
						//bypass the stupid qemux86 toolchain issue
						pRedirectTarget->redirectedTouchTrackedPointMoved(this,it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos(),nullContext);
					}
				}
				else if (redirType == RedirectingType::NoRedirect)
				{
					touchTrackedPointMoved(it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos());
				}
				//else the target was destroyed..just silently consume
			}
			break;
		}
	}
	return true;
}
//virtual
bool Page::touchEndEvent(QTouchEvent *event)
{
	event->accept();
	//TODO: MULTI-TOUCH
	//dig out the touch point which is being tracked
	QList<QTouchEvent::TouchPoint> points = event->touchPoints();
	bool handled = false;
	int id = -1;
	RedirectContext * pRedirContext = 0;
	for (QList<QTouchEvent::TouchPoint>::iterator it = points.begin();
			it != points.end();++it)
	{
		if (it->id() == m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt())
		{
			handleTrackedTouchPointRelease(&(*it));
			break;
		}
	}
	Q_EMIT signalTouchFSMTouchEnd_Trigger();
	//remove the register entry

	return true;
}

//virtual
void Page::handleTrackedTouchPointRelease(QTouchEvent::TouchPoint* touchPoint)
{
	//if it is being redirected, then keep redirecting it, but if the target got destroyed, then just silently swallow the events
	Thing * pRedirectTarget = 0;
	RedirectContext * pRedirContext = 0;
	RedirectingType::Enum redirType = isRedirecting(touchPoint->id(),&pRedirectTarget,&pRedirContext);
	if (redirType == RedirectingType::Redirect)
	{
		// if the page is in ovescroll, undo it when redirecting the flick gesture to the launcher for a page switch
		if(m_p_scroll->isInOverscroll()) {
			slotUndoOverscroll();
		}

		if (pRedirContext)
		{
			pRedirectTarget->redirectedTouchTrackedPointReleased(this,touchPoint->id(),touchPoint->scenePos(),touchPoint->lastScenePos(),touchPoint->startScenePos(),*pRedirContext);
		}
		else
		{
			RedirectContext nullContext;
			pRedirectTarget->redirectedTouchTrackedPointReleased(this,touchPoint->id(),touchPoint->scenePos(),touchPoint->lastScenePos(),touchPoint->startScenePos(),nullContext);
		}

		//check for a deferred redirect cancel. If it happened, then change the redirType and handle it locally, below
		if (isDeferCancelled(touchPoint->id(),true))
		{
			redirType = RedirectingType::NoRedirect;
			pRedirContext=0;
		}
	}

	if (redirType == RedirectingType::NoRedirect)
	{
		touchTrackedPointReleased(touchPoint->id(),touchPoint->scenePos(),touchPoint->lastScenePos(),touchPoint->startScenePos());
	}

	if (m_scrollLockTouchId == touchPoint->id())
	{
		m_scrollDirectionLock = PageScrollDirectionLock::None;
	}
	if (pRedirContext)
	{
		delete pRedirContext;
	}
	m_touchRegisters.remove(touchPoint->id());
}

//virtual
void Page::touchTrackedPointStarted(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{
}

///virtual
void Page::touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{
	if (!stopAnimationEnsemble())
	{
		//can't stop the animation, so bail here - SILENTLY CONSUME EVENT.
		// I don't want anyone else handling the event. The user will retry is shortly when the anim completes
		return;
	}

	if (m_failmaticScroller.animatingFlick())
	{
		//just like an animation...
		return;
	}

	//TODO: OPTIMIZE: consider making the local touch point tracking take the event's ICS instead of the scene, and
	// leave the "redirect" versions as scene....this should avoid the extra mapping here back to ICS
	QPointF localPosition = mapFromScene(scenePosition);
	QPointF localAmountMoved = localPosition - mapFromScene(lastScenePosition);
	QPointF localInitialPosition = mapFromScene(initialPosition);

	qreal moveByX = localAmountMoved.x();
	qreal moveByY = localAmountMoved.y();

	// Axis locking
	if (m_scrollDirectionLock == PageScrollDirectionLock::None) {

		int radius = Settings::LunaSettings()->tapRadius;
		QPointF moveSinceDown = localPosition - localInitialPosition;
		m_scrollLockTouchId = id;
		if (qAbs(moveSinceDown.y()) > radius) {
			m_scrollDirectionLock = PageScrollDirectionLock::Vertical;
		}
		else if (qAbs(moveSinceDown.x()) > radius) {
			m_scrollDirectionLock = PageScrollDirectionLock::Horizontal;
		}
		else {
			// Not locked yet
			return;
		}
	}

	if (m_scrollDirectionLock == PageScrollDirectionLock::Horizontal)
	{
		//TODO: LAUNCHER-REF:

		//this is a launcher page horizontal move. it's handled by the LauncherObject itself, so send it there
		if (DimensionsUI::launcher())
		{
			//add redirect
			LauncherPageHPanRedirectContext * pPanRedirectContext = new LauncherPageHPanRedirectContext();
			redirectTo(id,DimensionsUI::launcher(),pPanRedirectContext);
			DimensionsUI::launcher()->redirectedTouchTrackedPointMoved(this,id,scenePosition,lastScenePosition,initialPosition,*pPanRedirectContext);

			if(m_p_scroll->isInOverscroll())
				slotUndoOverscroll();
		}
		return;
	}

	if (m_p_scroll)
	{
		//scroll
		qint32 sv = m_p_scroll->scrollValue();
		//TODO: possibly might need to be able to reverse the notion of "up" and "down" at runtime (-/+ moveBy)

		if(m_p_scroll->isInOverscroll()) // only move buy 1/2 the distance if the page is in overscroll
			moveByY /= 2;
		m_p_scroll->setScrollValue(m_p_scroll->scrollValue()-moveByY);
	}

}

//virtual
void Page::touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{
	slotUndoOverscroll();
}

//TODO: this base version is "hard"-coded to do a fixed scroll. What it really should do it let the layout tell it
// how far to go up or down; usually just enough to expose the next row fully. I'll let this be dealt with in subclasses
//virtual
void Page::autoScrollDown()
{
	//if the FSM says not ok to scroll yet, then don't
	if (!okToPerformAutoScroll())
	{
		return;
	}
	if (!stopAnimationEnsemble())
	{
		//can't stop the animation, so bail here
		return;
	}

	if (m_failmaticScroller.animatingFlick())
	{
		//flick is going...so can't really autoscroll at the moment
		return;
	}

	if (m_p_scroll)
	{
		quint32 sv = qMin<qint32>(DynamicsSettings::settings()->pageScrollAmount,m_p_scroll->scrollAmountUntilBottomOverscroll());
		if (sv > 0)
		{
			QPropertyAnimation * pAutoscrollAnim = new QPropertyAnimation(m_p_scroll,"scroll");
			pAutoscrollAnim->setEndValue(m_p_scroll->scrollValue() + sv);
			pAutoscrollAnim->setDuration(DynamicsSettings::settings()->pageScrollAnimTime);
			slotAddAnimationToEnsemble(pAutoscrollAnim,DimensionsTypes::AnimationType::PageVScroll);
			slotStartAnimationEnsemble(false);		//uninterruptible!

			//Be aware here: scrollActionTrigger will kick in the FSM for scrolling which works by staying in a no-scroll
			// state for a certain amount of time (see DynamicsSettings). This delay should actually be set such that
			// value to set in the settings object = delay desired + duration of this scroll anim
			scrollActionTrigger();
		}
	}

}

//virtual
void Page::autoScrollUp()
{
	//if the FSM says not ok to scroll yet, then don't
	if (!okToPerformAutoScroll())
	{
		return;
	}
	if (!stopAnimationEnsemble())
	{
		//can't stop the animation, so bail here
		return;
	}

	if (m_failmaticScroller.animatingFlick())
	{
		//flick is going...so can't really autoscroll at the moment
		return;
	}

	if (m_p_scroll)
	{
		quint32 sv = qMin<qint32>(DynamicsSettings::settings()->pageScrollAmount,m_p_scroll->scrollAmountUntilTopOverscroll());
		if (sv > 0)
		{
			QPropertyAnimation * pAutoscrollAnim = new QPropertyAnimation(m_p_scroll,"scroll");
			pAutoscrollAnim->setEndValue(m_p_scroll->scrollValue() - sv);
			pAutoscrollAnim->setDuration(DynamicsSettings::settings()->pageScrollAnimTime);
			slotAddAnimationToEnsemble(pAutoscrollAnim,DimensionsTypes::AnimationType::PageVScroll);
			slotStartAnimationEnsemble(false);		//uninterruptible!

			//Be aware here: scrollActionTrigger will kick in the FSM for scrolling which works by staying in a no-scroll
			// state for a certain amount of time (see DynamicsSettings). This delay should actually be set such that
			// value to set in the settings object = delay desired + duration of this scroll anim
			scrollActionTrigger();

		}
	}
}

//virtual
IconBase * Page::removeIconFromPageAddWaitlist(const QUuid& iconUid)
{
	QMap<QUuid,QPointer<IconBase> >::iterator f = m_pendingExternalIconAddWaitlist.find(iconUid);
	if (f != m_pendingExternalIconAddWaitlist.end())
	{
		IconBase * p = *f;
		m_pendingExternalIconAddWaitlist.erase(f);		//you've been healed, my child!
		return p;
	}
	return 0;
}

//virtual
bool Page::isIconInPageAddWaitlist(const QUuid& iconUid)
{
	return m_pendingExternalIconAddWaitlist.contains(iconUid);
}

//virtual
void Page::addIconToPageAddWaitlist(IconBase * p_icon)
{
	if (!p_icon)
	{
		return;
	}
	m_pendingExternalIconAddWaitlist.insert(p_icon->uid(),QPointer<IconBase>(p_icon));
}

//virtual
qint32 Page::addWaitlistHandler()
{
	QMap<QUuid,QPointer<IconBase> > leftovers;
	for (QMap<QUuid,QPointer<IconBase> >::iterator it = m_pendingExternalIconAddWaitlist.begin();
			it != m_pendingExternalIconAddWaitlist.end();++it)
	{
		IconBase *pIcon = *it;
		if (!pIcon)
		{
			continue;		//it's been deleted externally. qpointers!
		}
		if (!addIcon(pIcon))
		{
			//couldn't add it
			leftovers.insert(pIcon->uid(),QPointer<IconBase>(pIcon));
		}
	}
	m_pendingExternalIconAddWaitlist = leftovers;
	return leftovers.size();
}

//virtual
void Page::removeIconFromPageRemoveWaitlist(const QUuid& iconUid)
{
	m_pendingExternalIconRemoveWaitlist.remove(iconUid);
}
//virtual
bool Page::isIconInPageRemoveWaitlist(const QUuid& iconUid)
{
	return (m_pendingExternalIconRemoveWaitlist.contains(iconUid));
}
//virtual
void Page::addIconToPageRemoveWaitlist(IconBase * p_icon)
{
	if (!p_icon)
	{
		return;
	}
	m_pendingExternalIconRemoveWaitlist.insert(p_icon->uid());
}

//virtual
void Page::addIconToPageRemoveWaitlist(const QUuid& iconUid)
{
	m_pendingExternalIconRemoveWaitlist.insert(iconUid);
}

//returns the number of items NOT handled (still waitlisted) ...so 0 would mean total success
//virtual
qint32 Page::removeWaitlistHandler()
{
	QSet<QUuid> leftovers;
	for (QSet<QUuid>::iterator it = m_pendingExternalIconRemoveWaitlist.begin();
			it != m_pendingExternalIconRemoveWaitlist.end();++it)
	{
		if (!removeIcon(*it))
		{
			//couldn't remove it
			leftovers.insert(*it);
		}
	}
	m_pendingExternalIconRemoveWaitlist = leftovers;
	return leftovers.size();
}

////////////////////////////// -- DEBUG -- ///////////////////////////////////////////

#include <QDebug>
//virtual
void Page::dbg_slotPrintLayout()
{
	qDebug() << __PRETTY_FUNCTION__ << ": base class Page; no layout to display";
}

void Page::dbg_slotStatePropertiesAssigned()
{
//	QAbstractState * pState = qobject_cast<QAbstractState *>(sender());
//	if (pState)
//	{
//		qDebug() << __FUNCTION__ << ": FSM currently: " << pState->objectName();
//	}

//	QAbstractState * currentState = *(m_p_touchFSM->configuration().begin());
//	qDebug() << __FUNCTION__ << ": FSM currently: " << currentState->objectName()
//							<< " , " << TouchFSMPropertyName_isTracking << " = " << m_p_touchFSM->property(TouchFSMPropertyName_isTracking).toBool()
//							<< ": currently tracked touchId = " << m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt();
}

