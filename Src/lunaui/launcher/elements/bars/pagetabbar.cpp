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




#include "pagetabbar.h"
#include "dimensionslauncher.h"
#include "dimensionsglobal.h"
#include "pixmapobject.h"
#include "pixmap9tileobject.h"
#include "pixmap3vtileobject.h"
#include "pixmaploader.h"
#include "gfxsettings.h"
#include "layoutsettings.h"
#include "pagetab.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QString>
#include <QDebug>
#include <QSizeF>

#include "QEvent"
#include <QGesture>
#include <QGestureEvent>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#include "Settings.h"

#define TAB_NORMAL_BACKGROUND_FILEPATH 	QString("tab-bg.png")
#define TAB_SELECTED_BACKGROUND_FILEPATH QString("tab-selected-bg.png")
#define TAB_HIGHLIGHTED_BACKGROUND_FILEPATH QString("tab-highlight.png")
#define TAB_SHADOW_BACKGROUND_FILEPATH QString("tab-shadow.png")
#define TAB_VDIV_FILEPATH QString("tab-divider.png")

uint qHash(const QPointer<PageTab>& p)
{
	if (p)
	{
		return qHash(p->uid().toString());
	}
	return 0;
}

QFont PageTabBar::s_tabLabelFont = QFont();

QFont PageTabBar::staticLabelFontForTabs()
{
	//TODO: specify a proper font
	static bool fontInitialized = false;
	if (!fontInitialized)
	{
		s_tabLabelFont = QFont(QString::fromStdString(Settings::LunaSettings()->fontQuicklaunch));
		quint32 fontSize = qBound((quint32)2,LayoutSettings::settings()->tabBarTabFontSizePx,(quint32)100);
		s_tabLabelFont.setPixelSize(fontSize);
		s_tabLabelFont.setBold(LayoutSettings::settings()->tabBarTabFontEmbolden);
	}
	return s_tabLabelFont;
}

PageTabBar::PageTabBar(const QRectF& pageTabBarGeometry,LauncherObject * p_belongsTo)
: ThingPaintable(pageTabBarGeometry)
, m_maxTabWidth(150)
, m_interactionsBlocked(false)
, m_qp_currentUIOwner(p_belongsTo)
, m_qp_backgroundPmo(0)
, m_qp_backgroundShadowPmo(0)
{
	resetTabSpaces();
	if (m_qp_currentUIOwner)
		this->setParentItem(m_qp_currentUIOwner);

	grabGesture((Qt::GestureType) SysMgrGestureFlick);

	m_backgroundGeom = m_geom;
	m_backgroundShadowGeom = m_geom;

}

//virtual
PageTabBar::~PageTabBar()
{
}

PixmapObject * PageTabBar::setBackground(PixmapObject * p_newBackground)
{
	PixmapObject * p = m_qp_backgroundPmo;
	m_qp_backgroundPmo = p_newBackground;
	if (m_qp_backgroundPmo)
	{
		//TODO: PIXEL-ALIGN
		m_qp_backgroundPmo->resize(m_geom.size().toSize());
	}
	return p;
}

//virtual
PixmapObject * PageTabBar::setBackgroundShadow(PixmapObject * p_new)
{
	PixmapObject * p = m_qp_backgroundShadowPmo;
	m_qp_backgroundShadowPmo = p_new;
	if (m_qp_backgroundShadowPmo)
	{
		m_qp_backgroundShadowPmo->resize(m_backgroundShadowGeom.size().toSize());
	}
	update();
	return p;
}

//static
QSize PageTabBar::PageTabSizeFromLauncherSize(quint32 launcherWidth,quint32 launcherHeight)
{

	if (LayoutSettings::settings()->tabBarUseAbsoluteSize)
	{
		return QSize(
				launcherWidth,
				qMin(launcherHeight,(quint32)(LayoutSettings::settings()->tabBarHeightAbsolute))
		);
	}

	QSize r = QSize(
			qBound((quint32)2,
					(quint32)DimensionsGlobal::roundDown((qreal)launcherWidth * LayoutSettings::settings()->tabBarSizePctLauncherRelative.width()),
					(quint32)launcherWidth),
			qBound((quint32)2,
					(quint32)DimensionsGlobal::roundDown((qreal)launcherHeight * LayoutSettings::settings()->tabBarSizePctLauncherRelative.height()),
					(quint32)launcherHeight)
	);

	//make evenly divisible (multiple of 2)
	r.setWidth(r.width() - (r.width() % 2));
	r.setHeight(r.height() - (r.height() % 2));
	return r;
}

void PageTabBar::slotAddTab(const QString& labelString,Page * p_refersToPage)
{
	if (labelString.isEmpty())
		return;

	//TODO: DETERMINE THE SIZE OF THE STRING NAME, and use that as tab size, and not just Max size
	PageTab * pPageTab = new PageTab(DimensionsGlobal::realRectAroundRealPoint(QSizeF(newTabMaxSize())),
									labelString,this,p_refersToPage);

/*
 * #define TAB_NORMAL_BACKGROUND_FILEPATH 	QString("tab-bg.png")
#define TAB_SELECTED_BACKGROUND_FILEPATH QString("tab-selected-bg.png")
#define TAB_HIGHLIGHTED_BACKGROUND_FILEPATH QString("tab-highlight.png")
#define TAB_VDIV_FILEPATH QString("tab-divider.png")

 */

	//quickLoadNineTiled: specify in-coordinates in top,bottom,left,right order
	Pixmap9TileObject * pNormalBgPmo = PixmapObjectLoader::instance()->quickLoadNineTiled(
			QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + TAB_NORMAL_BACKGROUND_FILEPATH),
			20,20,4,4
	);
	Pixmap9TileObject * pSelectedBgPmo = PixmapObjectLoader::instance()->quickLoadNineTiled(
			QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + TAB_SELECTED_BACKGROUND_FILEPATH),
			20,20,20,20
	);
	Pixmap9TileObject * pHighlightedBgPmo = PixmapObjectLoader::instance()->quickLoadNineTiled(
			QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + TAB_HIGHLIGHTED_BACKGROUND_FILEPATH),
			20,20,20,20
	);

	Pixmap3VTileObject * pVdiv = PixmapObjectLoader::instance()->quickLoadThreeVertTiled(
				QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + TAB_VDIV_FILEPATH),
				20,20
	);

	pPageTab->setBackgrounds(pNormalBgPmo,pSelectedBgPmo,pHighlightedBgPmo);
	pPageTab->slotSetDisplayMode(PageTabDisplayMode::Normal);

	//determine the placement of the new tab...
	//get the unused tab space rect in terms of the actual screen position
	pPageTab->setPos(m_unusedTabSpace.topLeft()-pPageTab->geometry().topLeft());

	if (pVdiv)
	{
		//TODO: PMO-MANAGE: it's ok to share for now
		pPageTab->setRightVerticalDivider(pVdiv);
		pPageTab->setLeftVerticalDivider(pVdiv);
	}

	//figure out if it's the first or last tab and adjust the divider visibility

	if (m_pageTabs.empty())
	{
		pPageTab->setLeftVerticalDividerVisible(false);
		pPageTab->setTabIndex(0);
	}
	else
	{
		//remove the previous last tab's right divider
		m_pageTabs[m_pageTabs.size()-1]->setRightVerticalDividerVisible(false);
		m_pageTabs[m_pageTabs.size()-1]->update();
		//and set this new tab's left one. It cannot be added in a selected state anyways (which would required a left tab invis.)
		pPageTab->setLeftVerticalDividerVisible(true);
		pPageTab->setTabIndex(m_pageTabs.size());	//this will be it's new index, when it gets appended
	}

	//this is the new last tab, so it needs a right divider visible
	pPageTab->setRightVerticalDividerVisible(true);

	//	//adjust the unused space and the used space - THE TWO REGIONS MUST BE CONTIGUOUS (FLUSH AGAINST EACH OTHER)!
	m_usedTabSpace.adjust(0.0,
							0.0,
							pPageTab->geometry().width(),
							0.0 );
	recalculateUnusedSpace();
	m_pageTabs << QPointer<PageTab>(pPageTab);

	connect(pPageTab,SIGNAL(signalActivatedTap()),
			this,SLOT(slotTabActivatedTap()));
	connect(pPageTab,SIGNAL(signalActivatedTapAndHold()),
			this,SLOT(slotTabActivatedTapAndHold()));
	connect(pPageTab,SIGNAL(signalSlotHighlighted()),
			this,SLOT(slotHighlighted()));		///careful, don't get circular here... (keep straight what is an "action" slot and what is a "alert" slot)

	connect(this,SIGNAL(signalAllTabHighlightsOff()),
			pPageTab,SLOT(slotUnHighlight()));

	connect(this,SIGNAL(signalInteractionsBlocked()),
			pPageTab,SLOT(slotInteractionsBlocked())
			);
	connect(this,SIGNAL(signalInteractionsRestored()),
			pPageTab,SLOT(slotInteractionsAllowed())
			);

}

void PageTabBar::slotUnHighlightAll()   //outside -> Tab bar
{
	//just fwd signal for now, but leave this function here for convenient hook point
	Q_EMIT signalAllTabHighlightsOff();
}

Page * PageTabBar::testForIntersectOnPageTab(const QPointF& sceneCoordinate,bool highlight)
{
	QPointF localCoordinate = mapFromScene(sceneCoordinate);
	qDebug() << __PRETTY_FUNCTION__ << ": localCoordinate: " << localCoordinate;
	for (QList<QPointer<PageTab> >::iterator it = m_pageTabs.begin();
			it != m_pageTabs.end();++it)
	{
		if (*it)
		{
			if ((*it)->testForIntersect(localCoordinate,highlight))
			{
				return (*it)->relatedPage();
			}
			else if (highlight) 	//more than just flagging that a tab should be highlighted, this bool param signifies if changes should be made at all to the tabs
									// including UNhighlighting
			{
				(*it)->slotUnHighlight();
			}
		}
	}

	return 0;
}

//virtual
Page * PageTabBar::testForIntersectOnPageTab(const QPointF& sceneCoordinate,PageTab ** r_p_pageTab,bool highlight)
{
	QPointF localCoordinate = mapFromScene(sceneCoordinate);
	for (QList<QPointer<PageTab> >::iterator it = m_pageTabs.begin();
			it != m_pageTabs.end();++it)
	{
		if (*it)
		{
			if ((*it)->testForIntersect(localCoordinate,highlight))
			{
				if (r_p_pageTab)
				{
					*r_p_pageTab = (*it);
				}
				return (*it)->relatedPage();
			}
			else if (highlight) 	//more than just flagging that a tab should be highlighted, this bool param signifies if changes should be made at all to the tabs
				// including UNhighlighting
			{
				(*it)->slotUnHighlight();
			}
		}
	}

	return 0;
}

//virtual
void PageTabBar::highlightTabForPage(Page * p_page)
{
	PageTab * pTab = this->tabByPage(p_page);
	if (pTab)
	{
		pTab->slotHighlight();
	}
}

//virtual
void PageTabBar::unHighlightTabForPage(Page * p_page)
{
	PageTab * pTab = this->tabByPage(p_page);
	if (pTab)
	{
		pTab->slotUnHighlight();
	}
}

//virtual
void PageTabBar::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	if (m_qp_backgroundPmo)
	{
		m_qp_backgroundPmo->paint(painter,m_backgroundGeom.topLeft());
	}
	if (m_qp_backgroundShadowPmo)
	{
		m_qp_backgroundShadowPmo->paint(painter,m_backgroundShadowGeom.topLeft());
	}

//	QPen sp = painter->pen();
//	painter->setPen(Qt::magenta);
//	painter->drawRect(m_geom);
////	painter->drawEllipse(QPointF(0,0),1.0,1.0);
//	painter->setPen(sp);
}

//virtual
void PageTabBar::paintOffscreen(QPainter *painter)
{

}

//virtual
bool PageTabBar::resize(quint32 newWidth,quint32 newHeight)
{
	//TODO: make consistent; some subclasses are using quint,quint as the impl. and others the qsize impl
	return resize(QSize(newWidth,newHeight));
}

//virtual
bool PageTabBar::resize(const QSize& s)
{
	m_geom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(s));
	m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);

	m_backgroundGeom = m_geom;
	m_backgroundShadowGeom = m_geom.adjusted(0.0,0.0,
												0.0,2.0);
	if (m_qp_backgroundPmo)
	{
		m_qp_backgroundPmo->resize(m_backgroundGeom.size().toSize());
	}
	if (m_qp_backgroundShadowPmo)
	{
		m_qp_backgroundShadowPmo->resize(m_backgroundShadowGeom.size().toSize());
	}

	if (m_pageTabs.size() == 0)
		return true;		//nothing to do

	resetTabSpaces();

	QSizeF tabMaxSize = QSizeF(newTabMaxSize());

	//TODO: TEMP - equal sized tabs, all left compressed (aligned left, no space between)
	for (PageTabConstIterator it = m_pageTabs.constBegin(); it != m_pageTabs.constEnd();++it)
	{
		if (!(*it))
		{
			continue;
		}
		(*it)->resize(tabMaxSize.toSize());
	}
	relayoutExistingTabs();
	//WARN: this should be the greatest of m_geom, m_backgroundShadowGeom, and m_backgroundGeom
	ThingPaintable::recomputeBoundingRect(m_backgroundShadowGeom);
	update();
	return true;
}

//virtual
void PageTabBar::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();
}

//virtual
void PageTabBar::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();
//	qreal moveByX = event->pos().x() - event->lastPos().x();
//	qreal moveByY = event->pos().y() - event->lastPos().y();
//	if (abs(moveByX) < abs(moveByY))
//		event->accept();
//	else if (!m_qp_currentUIOwner.isNull())
//	{
//		m_qp_currentUIOwner->takeMouse(this);
//		event->ignore();
//		return;
//	}
//	else
//	{
//		event->ignore();
//		return;
//	}
}

//virtual
void PageTabBar::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();
}

//virtual
void PageTabBar::slotTabActivatedTap()
{
	PageTab * pTab = qobject_cast<PageTab *>(sender());
	if (pTab == NULL)
	{
		return;
	}
	Q_EMIT signalTabForPageActivatedTap(pTab->relatedPage());
}

//virtual
void PageTabBar::slotTabActivatedTapAndHold()
{
	PageTab * pTab = qobject_cast<PageTab *>(sender());
	if (pTab == NULL)
	{
		return;
	}
	Q_EMIT signalTabForPageActivatedTapAndHold(pTab->relatedPage());
}

//virtual
void PageTabBar::slotHighlighted()
{
	//highlighting is an exclusive thing...only 1 tab can be at a time...so un-h the others
	for (QList<QPointer<PageTab> >::iterator it = m_pageTabs.begin();
			it != m_pageTabs.end();++it)
	{
		if ((*it) && (*it != sender()))
		{
			(*it)->slotUnHighlight();
		}
	}
}

//virtual
void PageTabBar::slotLauncherBlockedInteractions()
{
	m_interactionsBlocked = true;
	Q_EMIT signalInteractionsBlocked();
}

//virtual
void PageTabBar::slotLauncherAllowedInteractions()
{
	m_interactionsBlocked = false;
	Q_EMIT signalInteractionsRestored();
}

/////////////////////// GESTURE, TOUCH, OTHER INPUT EVENTS ////////////////////////////

bool PageTabBar::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture * g = 0;
//		QGesture* g = ge->gesture(Qt::TapGesture);
//		if (g) {
//			QTapGesture* tap = static_cast<QTapGesture*>(g);
//			if (tap->state() == Qt::GestureFinished) {
//				tapGestureEvent(tap);
//			}
//			return true;
//		}
//		g = ge->gesture(Qt::TapAndHoldGesture);
//		if (g) {
//			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
//			if (hold->state() == Qt::GestureFinished) {
//				tapAndHoldGestureEvent(hold);
//			}
//			return true;
//		}
		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g) {
			FlickGesture* flick = static_cast<FlickGesture*>(g);
			if (flick->state() == Qt::GestureFinished) {
				flickGesture(flick);
			}
			return true;
		}
	}
	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool PageTabBar::swipeGesture(QSwipeGesture *swipeEvent)
{
	return true;
}

//virtual
bool PageTabBar::flickGesture(FlickGesture *flickEvent)
{
	return true;
}

//virtual
void PageTabBar::relayoutExistingTabs(const qreal toWidth,const qreal toHeight)
{
	if (m_pageTabs.size() == 0)
			return;		//nothing to do

	resetTabSpaces();

	for (PageTabConstIterator it = m_pageTabs.constBegin(); it != m_pageTabs.constEnd();++it)
	{
		if (!(*it))
		{
			continue;
		}

		//get the unused tab space rect in terms of the actual screen position
		(*it)->setPos(m_unusedTabSpace.topLeft()-(*it)->geometry().topLeft());
		//	//adjust the unused space and the used space - THE TWO REGIONS MUST BE CONTIGUOUS (FLUSH AGAINST EACH OTHER)!
		m_usedTabSpace.adjust(0.0,
				0.0,
				(*it)->geometry().width(),
				0.0 );
		recalculateUnusedSpace();
	}
}

//virtual
QList<QPointer<PageTab> > PageTabBar::tabs()
{
	return m_pageTabs;
}

//virtual
PageTab * PageTabBar::tabByPage(Page * p_page)
{
	for (QList<QPointer<PageTab> >::iterator it = m_pageTabs.begin();
			it != m_pageTabs.end();++it)
	{
		if (*it)
		{
			Page * pTabpage = (*it)->relatedPage();
			if ((pTabpage == p_page) && (p_page))
			{
				return (*it);
			}
		}
	}
	return 0;
}

//virtual
void PageTabBar::relayoutExistingTabs()
{
	/*
	 * same as the toWidth,toHeight flavor, but uses toWidth,toHeight = m_geom
	 *
	 */

	relayoutExistingTabs(m_geom.width(),m_geom.height());

}

//virtual
void PageTabBar::resetTabSpaces()
{
	m_unusedTabSpace = m_geom;
	m_usedTabSpace =  QRectF(m_geom.left(),m_geom.top(),0.0,m_geom.height());
}

//virtual
void PageTabBar::recalculateUnusedSpace()
{
	if (!m_geom.contains(m_usedTabSpace))
	{
		//something is really wrong
		return;
	}
	m_unusedTabSpace.setTopLeft(m_usedTabSpace.topRight());
	m_unusedTabSpace.setHeight(m_usedTabSpace.height());
	m_unusedTabSpace.setWidth(m_geom.right()-m_usedTabSpace.right());
}

QSize PageTabBar::newTabMaxSize()
{
	if (m_pageTabs.isEmpty())
	{
		return QSize(qMin(m_maxTabWidth,(quint32)DimensionsGlobal::roundDown(m_unusedTabSpace.width()))
					,(quint32)DimensionsGlobal::roundDown(m_unusedTabSpace.height()));
	}
	quint32 maxWidthPerTabGeom = (quint32)DimensionsGlobal::roundDown(m_geom.width()/(qreal)(m_pageTabs.size()));
	quint32 maxHeightPerTabGeom = (quint32)DimensionsGlobal::roundDown(m_unusedTabSpace.height());
	return QSize(qMin((qMin(maxWidthPerTabGeom,m_maxTabWidth)),(quint32)DimensionsGlobal::roundDown(m_unusedTabSpace.width()))
			,(quint32)DimensionsGlobal::roundDown(m_unusedTabSpace.height()));

}
