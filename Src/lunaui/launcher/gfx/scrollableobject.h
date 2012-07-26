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




#include "thingpaintable.h"
#include <QRectF>
#include <QRect>

class QPainter;
class QGraphicsSceneMouseEvent;

#ifndef SCROLLABLEOBJECT_H_
#define SCROLLABLEOBJECT_H_

class ScrollableObject : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	Q_PROPERTY(qint32 scroll READ scrollValue WRITE setScrollValue)

public:

	ScrollableObject(const QRectF& geometry);
	virtual ~ScrollableObject();

	virtual QRect screenGeometry() const;

	virtual qint32 scrollValue() const = 0;
	virtual void setScrollValue(qint32 v) = 0;
	virtual qint32 rawScrollValue() const;
	virtual qint32 scrollValueNeededToEscapeOverscroll();
	virtual quint32 scrollAmountUntilTopOverscroll();
	virtual quint32 scrollAmountUntilBottomOverscroll();
	virtual void enable();
	virtual void disable();

	virtual QPointF mapToContentSpace(const QPointF& scrollerSpacePointF);
	virtual QPointF mapToContentSpace(const QPoint& scrollerSpacePoint);
	virtual QPointF mapFromContentSpace(const QPointF& contentSpacePointF);

	virtual bool mapToContentSpace(const QPointF& scrollerSpacePointF,QPointF& r_mappedPointF);
	virtual bool mapToContentSpace(const QPoint& scrollerSpacePoint,QPointF& r_mappedPointF);
	virtual bool mapFromContentSpace(const QPointF& contentSpacePoint,QPointF& r_mappedScrollerSpacePointF);

	virtual bool resize(const QSize& newSize);
	virtual bool resize(quint32 w,quint32 h);

	virtual qint32 topLimit() const;
	virtual qint32 bottomLimit()  const;

	virtual bool   isInOverscroll() { return m_inOverscroll; }
	virtual qint32 overscrollAmount() { return m_overscrollVal; }

public Q_SLOTS:

	//these all do the same thing, but are in different flavors to make it easier for a wide variety of sources to
	//	set the proper m_sourceGeom. Owners only need to connect to one of them, and subclasses must implement all of them
	//	which also serves as a reminder that they need
	//		to set content geom correctly during initialization, etc.
	virtual void slotSourceGeomChanged(const QRectF& newGeom) = 0;
	virtual void slotSourceContentSizeChanged(const QSizeF& newContentSize) = 0;
	virtual void slotSourceContentSizeChanged(const QSize& newContentSize) = 0;

protected:

	virtual void resetToInitialSourceArea();
	virtual void resetToInitialTargetArea();

	virtual void setSourceContentGeom(const QRectF& newContentGeom) = 0;

	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);

	/*
	 * m_geom:	the geometry, running from -w/2,-h/2 -> w/2,h/2  (floats)
	 *
	 * m_screenGeom: a version of m_geom that is rounded correctly to
	 * 					pixel coordinates (ints), but same neg->pos coord space
	 *
	 * m_boundingRect: m_geom with a bit of padding on the edges in accordance with QG'view rules
	 *
	 * m_sourceRect:  the coordinates FROM which to paint from the "content". It is in the CONTENT's
	 * 					coordinate space. So, e.g. for a pixmap content source (like ScrollingSurface),
	 * 					the coordinate space will be from 0,0 -> m_sourceContentSize
	 * 					For sources that are in a logical item space more like regular g'items,
	 * 					or layouts, etc, the cs will be from (-w_of_item/2,-h_of_item/2),[size of item]
	 *
	 * m_targetRect:
	 * 				the coordinates INTO which to paint in the painter provided to paint(). It should be in the
	 * 				painter's cs; Since the paint() function is being called by the QG'view system automatically,
	 * 				the origin will have been translated to the item's origin by the QGV sys
	 */
	QRect  m_screenGeom;
	QRectF  m_sourceRect;
	QRect  m_targetRect;
	bool	m_inOverscroll;
	qint32 m_overscrollVal;
	qint32 m_overscrollBottomStart;
	qint32 m_maxSourceRectHeight;
	qint32 m_maxSourceRectWidth;

	QRectF	m_sourceGeom;				//from which the sourceRect is taken; to be set by subclasses

};
#endif /* SCROLLABLEOBJECT_H_ */
