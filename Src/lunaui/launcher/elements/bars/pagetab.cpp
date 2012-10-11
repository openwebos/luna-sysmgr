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




#include "pagetab.h"
#include "page.h"
#include "pagetabbar.h"
#include "pixmapobject.h"
#include "pixmap9tileobject.h"
#include "gfxsettings.h"
#include "layoutsettings.h"
#include "Localization.h"
#include "QtUtils.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QString>
#include <QTextOption>

#include "QEvent"
#include <QGesture>
#include <QGestureEvent>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#include <QDebug>

const char * PageTab::PropertyNameIndex = "tab_index";

PageTab::PageTab(const QRectF& pageTabGeometry,const QString& label,
				 PageTabBar * p_belongsTo,Page * p_refersToPage)
: ThingPaintable(pageTabGeometry)
, m_qp_currentTabBarOwner(p_belongsTo)
, m_qp_relatedToPage(p_refersToPage)
, m_qp_backgroundShadow(0)
, m_currentMode(PageTabDisplayMode::INVALID)
, m_savedMode(PageTabDisplayMode::INVALID)
, m_interactionsBlocked(false)
, m_currentBackgroundPmo(0)
, m_showLeftDivider(false)
, m_showRightDivider(false)
, m_leftDividerPmo(0)
, m_rightDividerPmo(0)
, m_tabLabel(label)
{
	m_textFont = PageTabBar::staticLabelFontForTabs();
	m_selectedColor = LayoutSettings::settings()->tabBarSelectedTabFontColor;
	m_unselectedColor = LayoutSettings::settings()->tabBarUnSelectedTabFontColor;
	recalculateLabelBoundsForCurrentGeom();
	redoLabelTextLayout();
	recalculateLabelPosition();

	if (m_qp_currentTabBarOwner)
		this->setParentItem(m_qp_currentTabBarOwner);

	grabGesture(Qt::TapGesture);
	initSignalSlotConnections();

	m_backgroundGeom = m_geom;
	m_backgroundShadowGeom = m_backgroundGeom.adjusted(0.0,0.0,		//(left,up,right,down)
											0.0,2.0);
	ThingPaintable::recomputeBoundingRect(m_backgroundShadowGeom);
}

//virtual
PageTab::~PageTab()
{
}

Page * PageTab::relatedPage() const
{
	return m_qp_relatedToPage;
}

//virtual
quint32 PageTab::tabIndex() const
{
	return ((quint32)(property(PageTab::PropertyNameIndex)).toInt());
}

//virtual
void PageTab::setTabIndex(quint32 i)
{
	setProperty(PageTab::PropertyNameIndex,i);
}

//virtual
void PageTab::initObjects()
{

}

//virtual
void PageTab::initSignalSlotConnections()
{
	if (m_qp_relatedToPage)
	{
		connect(m_qp_relatedToPage,SIGNAL(signalPageActive()),
				this,SLOT(slotRelatedPageActive()));
		connect(m_qp_relatedToPage,SIGNAL(signalPageDeactivated()),
				this,SLOT(slotRelatedPageDeactivated()));
	}
}


//virtual
PixmapObject * PageTab::setBackground(PixmapObject * p_new,PageTabDisplayMode::Enum mode,bool makeCurrent)
{
	if (mode == PageTabDisplayMode::INVALID)
	{
		return 0;
	}
	PixmapObject * p = m_modeBackgrounds[mode];
	m_modeBackgrounds[mode] = QPointer<PixmapObject>(p_new);
	if (p_new)
	{
		//TODO: PIXEL-ALIGN
		p_new->resize(m_backgroundGeom.size().toSize());
	}

	if ((makeCurrent) && (p_new))
	{
		m_currentMode = mode;
		m_currentBackgroundPmo = p_new;
		update();
	}
	return p;
}

//virtual
QList<PixmapObject *> PageTab::setBackgrounds(PixmapObject * p_newNormal,
												PixmapObject * p_newSelected,
												PixmapObject * p_newHighlighted)
{
	QList<PixmapObject *> rlist;
	PixmapObject * p = m_modeBackgrounds[PageTabDisplayMode::Normal];
	rlist << p;
	m_modeBackgrounds[PageTabDisplayMode::Normal] = p_newNormal;
	if (p_newNormal)
	{
		//TODO: PIXEL-ALIGN
		p_newNormal->resize(m_backgroundGeom.size().toSize());
	}

	p = m_modeBackgrounds[PageTabDisplayMode::Selected];
	rlist << p;
	m_modeBackgrounds[PageTabDisplayMode::Selected] = p_newSelected;
	if (p_newSelected)
	{
		//TODO: PIXEL-ALIGN
		p_newSelected->resize(m_backgroundGeom.size().toSize());
	}
	p = m_modeBackgrounds[PageTabDisplayMode::Highlighted];
	rlist << p;
	m_modeBackgrounds[PageTabDisplayMode::Highlighted] = p_newHighlighted;
	if (p_newHighlighted)
	{
		//TODO: PIXEL-ALIGN
		p_newHighlighted->resize(m_backgroundGeom.size().toSize());
	}
	return rlist;
}

//virtual
PixmapObject * PageTab::setBackgroundShadow(PixmapObject * p_new)
{
	PixmapObject * p = m_qp_backgroundShadow;
	m_qp_backgroundShadow = p_new;
	if (m_qp_backgroundShadow)
	{
		m_qp_backgroundShadow->resize(m_backgroundShadowGeom.size().toSize());
	}
	update();
	return p;
}

//virtual
PixmapObject * PageTab::setLeftVerticalDivider(PixmapObject * p_new)
{
	PixmapObject * p = m_leftDividerPmo;
	m_leftDividerPmo = p_new;
	//TODO: PIXEL-ALIGN
	if (p_new)
	{
		p_new->resize(m_geom.size().toSize());
		m_leftDividerPosPntCS = m_geom.topLeft();
	}
	update();
	return p;
}

//virtual
PixmapObject * PageTab::setRightVerticalDivider(PixmapObject * p_new)
{
	PixmapObject * p = m_rightDividerPmo;
	m_rightDividerPmo = p_new;
	if (p_new)
	{
		p_new->resize(m_geom.size().toSize());
		m_rightDividerPosPntCS = m_geom.topRight()-QPointF((qreal)(p_new->size().width()),0);
	}
	update();
	return p;
}

//virtual
void PageTab::setLeftVerticalDividerVisible(bool visible)
{
	m_showLeftDivider = visible;
}

//virtual
void PageTab::setRightVerticalDividerVisible(bool visible)
{
	m_showRightDivider = visible;
}

//virtual
bool PageTab::resize(const QSize& s)
{
	m_geom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(s));
	m_backgroundGeom = m_geom;
	m_backgroundShadowGeom = m_backgroundGeom.adjusted(0.0,0.0,		//(left,up,right,down)
												0.0,2.0);
	recalculateLabelBoundsForCurrentGeom();
	redoLabelTextLayout();
	recalculateLabelPosition();
	QSize bgSize = m_backgroundGeom.size().toSize();
	for (QMap<qint32,QPointer<PixmapObject> >::iterator it = m_modeBackgrounds.begin();
			it != m_modeBackgrounds.end();++it)
	{
		if (it.value())
		{
			it.value()->resize(bgSize);
		}
	}
	if (m_qp_backgroundShadow)
	{
		m_qp_backgroundShadow->resize(m_backgroundShadowGeom.size().toSize());
	}

	if (m_leftDividerPmo)
	{
		m_leftDividerPmo->resize(s);
		//reposition it
		m_leftDividerPosPntCS = m_geom.topLeft();
	}
	if (m_rightDividerPmo)
	{
		m_rightDividerPmo->resize(s);
		//reposition it
		m_rightDividerPosPntCS = m_geom.topRight()-QPointF((qreal)(m_rightDividerPmo->size().width()),0);
	}

	//WARN: this should be the greatest of m_geom, m_backgroundShadowGeom, and m_backgroundGeom
	ThingPaintable::recomputeBoundingRect(m_backgroundShadowGeom);
	update();
	return true;
}

//virtual
bool PageTab::resize(quint32 newWidth,quint32 newHeight)
{
	return resize(QSize(newWidth,newHeight));
}

//virtual
void PageTab::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	if (m_currentBackgroundPmo)
	{
		m_currentBackgroundPmo->paint(painter,m_backgroundGeom.topLeft());
	}
	if (m_qp_backgroundShadow)
	{
		m_qp_backgroundShadow->paint(painter,m_backgroundShadowGeom.topLeft());
	}
	if (m_leftDividerPmo && m_showLeftDivider)
	{
		m_leftDividerPmo->paint(painter,m_leftDividerPosPntCS);
	}
	if (m_rightDividerPmo && m_showRightDivider)
	{
		m_rightDividerPmo->paint(painter,m_rightDividerPosPntCS);
	}

//	QPen sp = painter->pen();
//	painter->setPen(Qt::yellow);
//	painter->drawRect(m_geom);
//	painter->drawEllipse(QPointF(0,0),1.0,1.0);
//	painter->drawLine(0.0,m_geom.top(),0.0,m_geom.bottom());
//	painter->drawLine(m_geom.left(),0.0,m_geom.right(),0.0);

//	painter->drawRect(m_labelGeom);
//	painter->setPen(sp);

	QPen sp = painter->pen();
	painter->setPen(m_currentMode == PageTabDisplayMode::Normal ? m_unselectedColor : m_selectedColor);
	m_textLayoutObject.draw(painter,m_labelPosPntCS);
	painter->setPen(sp);
}

//virtual
void PageTab::paintOffscreen(QPainter *painter)
{

}

//virtual
void PageTab::slotSetDisplayMode(PageTabDisplayMode::Enum mode)
{
	//an explicit set mode cancels any saved modes
	m_savedMode = PageTabDisplayMode::INVALID;
	if ((mode == PageTabDisplayMode::INVALID) || (mode == m_currentMode))
	{
		return;
	}
	if (mode == PageTabDisplayMode::Selected)
	{
		setZValue(1.0);
	}
	else if (m_currentMode == PageTabDisplayMode::Selected)
	{
		setZValue(0.0);
	}

	if (m_currentMode == PageTabDisplayMode::Highlighted)
	{
		m_savedMode = mode;
	}
	else
	{
		m_currentMode = mode;
		m_currentBackgroundPmo = m_modeBackgrounds[mode];
	}

	//if the mode is selected, then remove the left divider
	//else, show it
	if (tabIndex() > 0)
	{
		m_showLeftDivider = (mode != PageTabDisplayMode::Selected);
	}
	update();
}

//virtual
void PageTab::slotHighlight()
{
	if ((m_currentMode == PageTabDisplayMode::INVALID)
			|| (m_currentMode == PageTabDisplayMode::Highlighted))
	{
		//nothing to do
		Q_EMIT signalSlotHighlighted();
		return;
	}

	m_savedMode = m_currentMode;
	m_currentMode = PageTabDisplayMode::Highlighted;
	m_currentBackgroundPmo = m_modeBackgrounds[PageTabDisplayMode::Highlighted];

	update();
}

//virtual
void PageTab::slotUnHighlight()
{
	if ((m_savedMode != PageTabDisplayMode::INVALID)
		&& (m_currentMode == PageTabDisplayMode::Highlighted))
	{
		//restore current mode
		m_currentMode = m_savedMode;
		m_currentBackgroundPmo = m_modeBackgrounds[m_savedMode];
		update();
	}
}

//virtual
void PageTab::slotRelatedPageActive()
{
	slotSetDisplayMode(PageTabDisplayMode::Selected);
}

//virtual
void PageTab::slotRelatedPageDeactivated()
{
	slotSetDisplayMode(PageTabDisplayMode::Normal);
}

//virtual
void PageTab::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	slotHighlight();

	event->accept();
}

//virtual
void PageTab::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if ((m_currentMode == PageTabDisplayMode::Highlighted) &&
		!boundingRect().contains(event->pos()))
		slotUnHighlight();

	event->accept();
}

//virtual
void PageTab::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	slotUnHighlight();
	event->accept();
}

//virtual
void PageTab::recalculateLabelBoundsForCurrentGeom()
{
	//the label geom is the next smallest even integer in height and width from m_geom.
	// if that int is 0 in either case, then it's == m_geom's w or h
	quint32 gw = DimensionsGlobal::roundDown(m_geom.width());
	quint32 gh = DimensionsGlobal::roundDown(m_geom.height());
	QSize s = QSize(
				( gw < 2 ? gw : ( gw - (gw % 2))),
				( gh < 2 ? gh : ( gh - (gh % 2))));

	m_labelMaxGeom = DimensionsGlobal::realRectAroundRealPoint(s).toRect();
}

//virtual
void	PageTab::redoLabelTextLayout()
{
	//TODO: somewhat wasteful. If there is no label, should just exit early and leave a layout that will be left unrendered by paint()
	m_textLayoutObject.clearLayout();;
	//TODO: Need a real fix later instead of localizing the labels at runtime
	QString m_tabLabelLocalized = fromStdUtf8(LOCALIZED(m_tabLabel.toStdString()));
	m_textLayoutObject.setText(m_tabLabelLocalized);

//	int fontSize = qBound(4,(int)((qreal)(m_labelMaxGeom.height())*0.5),24) -2;
//	fontSize = fontSize - (fontSize % 2);
//	m_textFont.setPixelSize(fontSize);
	m_textLayoutObject.setFont(m_textFont);
	QTextOption textOpts;
	textOpts.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	textOpts.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	m_textLayoutObject.setTextOption(textOpts);

	QFontMetrics textFontMetrics(m_textFont);
	int leading = textFontMetrics.leading();
	int rise = textFontMetrics.ascent();
	qreal height = 0;

	m_textLayoutObject.beginLayout();
	while (height < m_labelMaxGeom.height()) {
		QTextLine line = m_textLayoutObject.createLine();
		if (!line.isValid())
			break;
		line.setLineWidth(m_labelMaxGeom.width());
		if (m_textLayoutObject.lineCount() > 1)
		{
			height += leading;
		}
		line.setPosition(QPointF(0, height));
		height += line.height();
	}
	height = qMin((quint32)DimensionsGlobal::roundUp(height),(quint32)m_labelMaxGeom.height());
	height = DimensionsGlobal::roundDown(height) - (DimensionsGlobal::roundDown(height) % 2);	//force to an even #
	m_textLayoutObject.endLayout();
	//TODO: PIXEL-ALIGN
	m_labelGeom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(m_textLayoutObject.boundingRect().width(),height)).toAlignedRect();
}

//virtual
void	PageTab::recalculateLabelPosition()
{
	//TODO: MYSTERY! don't know why it needs the 2.0 bump in Y. investigate the geom creation in redoLabelTextLayout()
	m_labelPosICS = QPointF(0,LayoutSettings::settings()->tabTextVerticalPosAdjust);
	m_labelPosPntCS = (m_labelGeom.topLeft() + m_labelPosICS).toPoint();
}

////////////////	/////////////////////			//////////////////////////////////
///////////////////////// GESTURES ///////////////////////////////////////////////////
////////////////	/////////////////////			//////////////////////////////////

bool PageTab::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g) {
			QTapGesture* tap = static_cast<QTapGesture*>(g);
			if (tap->state() == Qt::GestureFinished) {
				tapGesture(tap,ge);
			}
			return true;
		}
		g = ge->gesture(Qt::TapAndHoldGesture);
		if (g) {
			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
			if (hold->state() == Qt::GestureFinished) {
				tapAndHoldGesture(hold,ge);
			}
			return true;
		}
//		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
//		if (g) {
//			FlickGesture* flick = static_cast<FlickGesture*>(g);
//			if (flick->state() == Qt::GestureFinished) {
//				flickGesture(flick,ge);
//			}
//			return true;
//		}
	}
	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool PageTab::flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent)
{
	return true;
}

//virtual
bool PageTab::tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent)
{
	if (m_interactionsBlocked)
	{
		return true;
	}
	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapHoldEvent->hotSpot()));

	Q_EMIT signalActivatedTapAndHold();

	return true;
}
//virtual
bool PageTab::tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent)
{
//	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapEvent->hotSpot()));

	if (m_interactionsBlocked)
	{
		return true;
	}
	Q_EMIT signalActivatedTap();

//	//TODO: DEBUG: TEST:
//	//toggle through the different background modes
//	switch (m_currentMode)
//	{
//		case PageTabDisplayMode::Normal:
//			slotSetDisplayMode(PageTabDisplayMode::Selected);
//		break;
//		case PageTabDisplayMode::Selected:
//			slotSetDisplayMode(PageTabDisplayMode::Highlighted);
//		break;
//		case PageTabDisplayMode::Highlighted:
//			slotSetDisplayMode(PageTabDisplayMode::Normal);
//		break;
//		default:
//			slotSetDisplayMode(PageTabDisplayMode::Normal);
//		break;
//	}
	return true;
}

//virtual
void PageTab::slotInteractionsBlocked()
{
	m_interactionsBlocked = true;
}

//virtual
void PageTab::slotInteractionsAllowed()
{
	m_interactionsBlocked = false;
}

bool PageTab::testForIntersect(const QPointF& tabBarPositionICS,bool highlight)
{
	if (positionRelativeGeometry().contains(tabBarPositionICS))
	{
		if (highlight)
		{
			slotHighlight();
		}

		update();
		return true;
	}
	return false;
}

//virtual
void PageTab::syntheticTap()
{
	//keep in line with tapGesture()
	Q_EMIT signalActivatedTap();
}
