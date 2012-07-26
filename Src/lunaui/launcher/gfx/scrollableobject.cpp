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




#include "scrollableobject.h"
#include "dimensionsglobal.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

ScrollableObject::ScrollableObject(const QRectF& geometry)
:  ThingPaintable(geometry)
, m_inOverscroll(false)
, m_overscrollVal(0)
, m_overscrollBottomStart(0)
, m_maxSourceRectHeight(0)
, m_maxSourceRectWidth(0)
{
	m_screenGeom = DimensionsGlobal::realRectToPixelCompatibleForm(m_geom).toRect();
	setFlag(QGraphicsItem::ItemHasNoContents,true);	//disabled by default to prevent paint() of invalid content
}

//virtual
ScrollableObject::~ScrollableObject()
{
}

//virtual
QRect ScrollableObject::screenGeometry() const
{
	return m_screenGeom;
}

//virtual
qint32 ScrollableObject::rawScrollValue() const
{
	return 0;
}

//virtual
qint32 ScrollableObject::scrollValueNeededToEscapeOverscroll()
{
	return 0;
}

//virtual
quint32 ScrollableObject::scrollAmountUntilTopOverscroll()
{
	return 0;
}

//virtual
quint32 ScrollableObject::scrollAmountUntilBottomOverscroll()
{
	return 0;
}

//virtual
void ScrollableObject::enable()
{
	setFlag(QGraphicsItem::ItemHasNoContents,false);
}

//virtual
void ScrollableObject::disable()
{
	setFlag(QGraphicsItem::ItemHasNoContents,true);
}

//virtual
QPointF ScrollableObject::mapToContentSpace(const QPointF& scrollerSpacePointF)
{
	//do nothing
	return QPointF();
}

//virtual
QPointF ScrollableObject::mapToContentSpace(const QPoint& scrollerSpacePoint)
{
	return QPointF();
}

//virtual
QPointF ScrollableObject::mapFromContentSpace(const QPointF& contentSpacePointF)
{
	return QPointF();
}

//virtual
bool ScrollableObject::mapToContentSpace(const QPointF& scrollerSpacePointF,QPointF& r_mappedPointF)
{
	return false;
}

//virtual
bool ScrollableObject::mapToContentSpace(const QPoint& scrollerSpacePoint,QPointF& r_mappedPointF)
{
	return false;
}

//virtual
bool ScrollableObject::mapFromContentSpace(const QPointF& contentSpacePoint,QPointF& r_mappedScrollerSpacePointF)
{
	return false;
}

//virtual
bool ScrollableObject::resize(const QSize& newSize)
{
	ThingPaintable::resize(newSize);
	m_screenGeom = DimensionsGlobal::realRectToPixelCompatibleForm(m_geom).toRect();
	resetToInitialSourceArea();
	resetToInitialTargetArea();
	return true;
}

//virtual
bool ScrollableObject::resize(quint32 w,quint32 h)
{
	return resize(QSize(w,h));
}

//virtual
qint32 ScrollableObject::topLimit() const
{
	return 0;
}

//virtual
qint32 ScrollableObject::bottomLimit()  const
{
	return 0;
}

//virtual
void ScrollableObject::resetToInitialSourceArea()
{
	//do nothing
}

//virtual
void ScrollableObject::resetToInitialTargetArea()
{
	//do nothing
}

//virtual
void ScrollableObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
//	//qDebug() << __PRETTY_FUNCTION__;
	event->ignore();
}
//virtual
void ScrollableObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
//	//qDebug() << __PRETTY_FUNCTION__;
	event->ignore();
}
//virtual
void ScrollableObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
//	//qDebug() << __PRETTY_FUNCTION__;
	event->ignore();
}

//virtual
void ScrollableObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	//do nothing
}

//virtual
void ScrollableObject::paintOffscreen(QPainter *painter)
{
	//do nothing
}
