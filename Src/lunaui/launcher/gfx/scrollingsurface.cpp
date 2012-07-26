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




#include "scrollingsurface.h"
#include "pixmapobject.h"
#include "dimensionsglobal.h"
#include <QPainter>
#include <QDebug>

ScrollingSurface::ScrollingSurface(const QRectF& geometry)
: ScrollableObject(geometry)
, m_qp_surfacePmo(0)
{
}

//virtual
ScrollingSurface::~ScrollingSurface()
{
}

void ScrollingSurface::resetTargetRectValid()
{
	m_targetRect = QRect(m_screenGeom.x(),
			m_screenGeom.y(),
			m_maxSourceRectWidth,
			m_maxSourceRectHeight);
}

//TODO: pixmap management - deletion of unused when new ones are set
void ScrollingSurface::setPixmapObject(PixmapObject * p_pmo)
{
	m_qp_surfacePmo = p_pmo;
	if (p_pmo)
	{
		setFlag(QGraphicsItem::ItemHasNoContents,false);
		setSourceContentGeom(DimensionsGlobal::realRectAroundRealPoint(QSizeF(p_pmo->size())));
	}
	else
	{
		setFlag(QGraphicsItem::ItemHasNoContents,true);
		m_sourceRect = QRect(0,0,0,0);
		setSourceContentGeom(QRectF(0,0,0,0));
		//TODO: TEST:  see setSourceContentGeom. clean this up
		m_targetRect = QRect(0,0,0,0);
	}
	update(m_boundingRect);
}

bool ScrollingSurface::testAndtranslateScrollSurfacePointToPixmapPoint(const QPointF& pointF,QPoint& r_translatedPoint)
{
//	QPoint point = pointF.toPoint();
//	if (m_targetRect.contains(point))
//	{
//		//ask the pixmapobject to map this for me
//		r_translatedPoint = m_qp_surfacePmo->translatePaintTargetPointToPixmapPoint(point,m_sourceRect,m_targetRect);
//		//qDebug() <<  __PRETTY_FUNCTION__ << ": point: " << point << " , pixmap point: " << r_translatedPoint;
//		return true;
//	}
//	//qDebug() <<  __PRETTY_FUNCTION__ << ": point: " << point << " is not contained in the target paint rect " << m_targetRect;
	return false;
}

//virtual
qint32 ScrollingSurface::scrollValue() const
{
	if (m_overscrollVal)
		return m_overscrollVal;

	//else not in overscroll, return the top y of the source rect
	return m_sourceRect.top();
}

//virtual
qint32 ScrollingSurface::rawScrollValue() const
{
	return m_sourceRect.top();
}

//virtual
void ScrollingSurface::setScrollValue(qint32 v)
{

	//qDebug() << __FUNCTION__ << " " << v;
	if ((v >= 0) && (v < m_overscrollBottomStart))
	{
		//no overscroll...somewhere in the middle of things
		m_sourceRect.setY(v);
		m_sourceRect.setHeight(m_maxSourceRectHeight);
		resetTargetRectValid();
		m_overscrollVal = 0;
	}
	else if (v < 0)
	{
		//Negative values: set it to overscroll from top.
		//the source rect is shrunk, with the shrinking coming from the bottom of the rect
		m_sourceRect.setY(0);
		m_sourceRect.setHeight(qMax(0,m_screenGeom.height()+v));
		m_targetRect.setY(m_screenGeom.top() + qMin(-v,m_screenGeom.height()));
		m_targetRect.setHeight(m_sourceRect.height());
		m_overscrollVal = v;
	}
	else
	{
		//TODO: PIXEL-ALIGN
		QSize sourceSize = m_sourceGeom.size().toSize();
		//Positive value > m_overscrollBottomStart
		//the source rect is shrunk, with the shrinking coming from the top
		qint32 va = qMin(v,sourceSize.height());
		m_sourceRect.setY(va);
		m_sourceRect.setHeight(sourceSize.height()-va);
		m_targetRect.setY(m_screenGeom.top());
		m_targetRect.setHeight(m_sourceRect.height());
		m_overscrollVal = v;
	}
	update(m_boundingRect);

}

////public Q_SLOTS:

/*
 *
 * These geom change functions are empty on purpose...see .h file
 *
 */

//virtual
void ScrollingSurface::slotSourceGeomChanged(const QRectF& newGeom)
{
}

//virtual
void ScrollingSurface::slotSourceContentSizeChanged(const QSizeF& newContentSize)
{
}

//virtual
void ScrollingSurface::slotSourceContentSizeChanged(const QSize& newContentSize)
{
}

////protected:

//virtual
void ScrollingSurface::setSourceContentGeom(const QRectF& newContentGeom)
{
	m_sourceGeom = newContentGeom;
	//TODO: PIXEL-ALIGN
	QSize sourceSize = m_sourceGeom.size().toSize();
	m_maxSourceRectWidth = qMin(sourceSize.width(),m_screenGeom.width());
	m_maxSourceRectHeight = qMin(sourceSize.height(),m_screenGeom.height());
	m_sourceRect = QRect(0,0,m_maxSourceRectWidth,m_maxSourceRectHeight);
	m_targetRect = QRect(m_screenGeom.x(),
			m_screenGeom.y(),
			m_maxSourceRectWidth,
			m_maxSourceRectHeight);
	m_overscrollBottomStart = qMax(0,sourceSize.height()-m_screenGeom.height());
}

//virtual
void ScrollingSurface::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	//ASSUMES m_qp_surfacePmo is valid; the ItemHasNoContents should have been set otherwise

	QPen sp = painter->pen();
	painter->setPen(Qt::green);
	painter->drawRect(m_screenGeom);
	painter->setPen(sp);

//	painter->drawPixmap(m_screenGeom.x(),m_screenGeom.y(),
//			m_screenGeom.width(),m_screenGeom.height(),
//			(*(*m_qp_surfacePmo)),m_sourceRect.x(),m_sourceRect.y(),
//			m_sourceRect.width(),m_sourceRect.height());

	//COMMENTED TO TEST OUT HUGE PIXMAP

//	painter->drawPixmap(m_targetRect,
//			(*(*m_qp_surfacePmo)),
//			m_sourceRect);

//	m_qp_surfacePmo->paint(painter,m_targetRect,m_sourceRect);
}
