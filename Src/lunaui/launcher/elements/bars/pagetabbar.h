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




///TODO: should be a Thing

#ifndef PAGETABBAR_H_
#define PAGETABBAR_H_

#include <QUuid>
#include <QRectF>
#include <QSize>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QSet>
#include <QFont>

#include "thingpaintable.h"
#include "pixmapobject.h"

class LauncherObject;
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
class PageTab;
class Page;

class PageTabBar : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:
	friend class LauncherObject;

	PageTabBar(const QRectF& pageBarGeometry,LauncherObject * p_belongsTo);
	virtual ~PageTabBar();

	//returns old pixmapobject
	PixmapObject * setBackground(PixmapObject * p_newBackground);
	PixmapObject * setBackgroundShadow(PixmapObject * p_new);

	static QSize PageTabSizeFromLauncherSize(quint32 launcherWidth,quint32 launcherHeight);

	//TODO: temporary; remove when a better system (like styles) goes in place
	static QFont staticLabelFontForTabs();

	//used by the launcher; specifically by the redirectTouch movement handler, while an icon is being moved.
	// it will test the coordinate given (it's in Scene CS) against the space being occupied by the tabs and return the Page ptr
	// of the page whos tab is "hit".
	// if the optional "highlight" parameter is true, then a positive test will also result in the tab getting highlighted
	virtual Page * testForIntersectOnPageTab(const QPointF& sceneCoordinate,bool highlight=false);
	virtual Page * testForIntersectOnPageTab(const QPointF& sceneCoordinate,PageTab ** r_p_pageTab,bool highlight=false);
	virtual void highlightTabForPage(Page * p_page);
	virtual void unHighlightTabForPage(Page * p_page);

Q_SIGNALS:

	void signalTabForPageActivatedTap(Page *);
	void signalTabForPageActivatedTapAndHold(Page *);

	void signalAllTabHighlightsOff();			//Tab bar -> Tabs

	void signalInteractionsBlocked();
	void signalInteractionsRestored();

public Q_SLOTS:

	void slotAddTab(const QString& labelString,Page * p_refersToPage=0);  //outside -> Tab bar
	void slotUnHighlightAll();   //outside -> Tab bar

protected Q_SLOTS:

	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	virtual void slotAnimationFinished() {}

	virtual void slotTabActivatedTap();
	virtual void slotTabActivatedTapAndHold();

	virtual void slotHighlighted();

	virtual void slotLauncherBlockedInteractions();
	virtual void slotLauncherAllowedInteractions();

protected:

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);

	virtual bool resize(quint32 newWidth,quint32 newHeight);
	virtual bool resize(const QSize& s);

	virtual bool touchEvent(QTouchEvent * event) { return true; }
	virtual bool touchStartEvent(QTouchEvent *event) { return true; }
	virtual bool touchUpdateEvent(QTouchEvent *event) { return true; }
	virtual bool touchEndEvent(QTouchEvent *event) { return true; }

	virtual bool sceneEvent(QEvent * event);
	virtual bool gestureEvent(QGestureEvent *gestureEvent) { return true; }
	virtual bool panGesture(QPanGesture *panEvent) { return true; }
	virtual bool swipeGesture(QSwipeGesture *swipeEvent);
	virtual bool flickGesture(FlickGesture *flickEvent);
	virtual bool pinchGesture(QPinchGesture *pinchEvent) { return true; }
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent) { return true; }
	virtual bool tapGesture(QTapGesture *tapEvent) { return true; }
	virtual bool customGesture(QGesture *customGesture) { return true; }

	virtual QList<QPointer<PageTab> > tabs();
	virtual PageTab * tabByPage(Page * p_page);

	virtual void relayoutExistingTabs();
	virtual void relayoutExistingTabs(const qreal toWidth,const qreal toHeight);

	//sets m_geomUnusedTabSpace full allowed tab space, and m_geomUsedTabSpace to empty
	// (used by things like relayout__() )
	virtual void resetTabSpaces();
	// sets m_geomUnusedTabSpace relative to what the current m_geomUsedTabSpace is set to
	virtual void recalculateUnusedSpace();

private:

	QSize	newTabMaxSize();

	QRectF  m_unusedTabSpace;
	QRectF	m_usedTabSpace;
	quint32 m_maxTabWidth;
	QString m_pageTabBarUiState;		//container for ui info the owner needs to store here

	bool 	m_interactionsBlocked;
	QRectF					m_backgroundGeom;
	QRectF					m_backgroundShadowGeom;

	QPointer<LauncherObject> m_qp_currentUIOwner;
	QPointer<PixmapObject> m_qp_backgroundPmo;
	QPointer<PixmapObject> m_qp_backgroundShadowPmo;

	typedef QList<QPointer<PageTab> >::const_iterator PageTabConstIterator;
	typedef QList<QPointer<PageTab> >::iterator PageTabIterator;
	QList<QPointer<PageTab> > m_pageTabs;

	static QFont s_tabLabelFont;

};


#endif /* PAGETABBAR_H_ */
