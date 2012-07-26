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




#include "scrollinglayoutrenderer.h"
#include "iconlayout.h"
#include "dimensionsglobal.h"
#include "renderopts.h"
#include "operationalsettings.h"
#include <QPainter>
#include <QDebug>

//public:

ScrollingLayoutRenderer::ScrollingLayoutRenderer(const QRectF& geometry,IconLayout& layout)
: ScrollableObject(geometry)
, m_qp_layoutObject(&layout)
{
	setSourceContentGeom(layout.geometry());
	setFlag(QGraphicsItem::ItemHasNoContents,false);
}

//virtual
ScrollingLayoutRenderer::~ScrollingLayoutRenderer()
{

}

//virtual
qint32 ScrollingLayoutRenderer::scrollValue() const
{
	if (m_inOverscroll)
	{
		return m_overscrollVal;
	}

	//the initial position is v=0, and it's when the top edge of the source content is flush with the top
	// edge of the scrollable window ("screen")
	return (DimensionsGlobal::roundDown(m_sourceRect.top()-m_sourceGeom.top()));
}

//virtual
void ScrollingLayoutRenderer::setScrollValue(qint32 v)
{
	if (m_sourceContentSize.height() <= m_screenGeom.height())
	{
		return setScrollValueSmallContent(v);
	}

	if (v < 0)
	{
//		qDebug() << "(TOP-Over) v = " << v;
		//top overscroll condition
		// the source (and target) rects both shrink from the bottom, and the target rect keeps its top() moving downward
		// (where downward is +y)

		//the source rect
		m_sourceRect.setTopLeft(m_sourceGeom.topLeft());
		m_sourceRect.setHeight(qMax(0,m_maxSourceRectHeight+v));
		m_sourceRect.setWidth((qreal)m_maxSourceRectWidth);

		//TODO: PIXEL-ALIGN
		QSize sourceRectPixelSize = m_sourceRect.toRect().size();
		//the target rect. It's the same size as the source rect, with its bottom() aligned with the screen's bottom
		m_targetRect.setTopLeft(m_screenGeom.bottomLeft()-QPoint(0,sourceRectPixelSize.height()));
		m_targetRect.setHeight(sourceRectPixelSize.height());
		m_targetRect.setWidth(sourceRectPixelSize.width());

		m_inOverscroll = true;
		m_overscrollVal = v;
	}
	else if (v > m_sourceGeom.height() - (qreal)m_screenGeom.height())  // v >  (content height) - (screen height)
	{
//		qDebug() << "(BOTTOM-Over) v = " << v;
		//bottom overscroll condition
		// the source (and target) rects both shrink from the top, and the target rect keeps its bottom() moving upward
		// (where upward is -y)

		qint32 dy = v - (m_sourceGeom.height() - (qreal)m_screenGeom.height());

		//the source rect
		m_sourceRect.setTopLeft(QPointF(m_sourceGeom.bottomLeft()-QPointF(0,m_screenGeom.height()-dy)));
		m_sourceRect.setHeight(qMax<qreal>(0.0, m_screenGeom.height()-dy));
		m_sourceRect.setWidth((qreal)m_maxSourceRectWidth);

//		//qDebug() << "bottom of source rect is y = " << m_sourceRect.bottom()
//				<< " , bottom of content is y = " << this->m_sourceGeom.bottom();
		//TODO: PIXEL-ALIGN
		QSize sourceRectPixelSize = m_sourceRect.toRect().size();
		//the target rect. It's the same size as the source rect, with its top() aligned with the screen's top
		m_targetRect.setTopLeft(m_screenGeom.topLeft());
		m_targetRect.setHeight(sourceRectPixelSize.height());
		m_targetRect.setWidth(sourceRectPixelSize.width());

		m_inOverscroll = true;
		m_overscrollVal = v;
	}
	else
	{
//		qDebug() << "(NORMAL) v = " << v;
		//normal scroll
		//...source rect size = screen , target rect size
		m_sourceRect.setTop(m_sourceGeom.top()+(qreal)v);
		m_sourceRect.setHeight((qreal)m_maxSourceRectHeight);
		resetToInitialTargetArea();

		m_inOverscroll = false;
		m_overscrollVal = 0;
	}
	update(m_boundingRect);
}

//virtual
void ScrollingLayoutRenderer::setScrollValueSmallContent(qint32 v)
{
	// "small content" is when the source height is smaller than the screen height. This case is always in overscroll
	// in this case, m_sourceContentSize.height() == m_maxSourceRectHeight
	if (v < 0)
	{
//		//qDebug() << "(svc TOP-Over) v = " << v;
		//top overscroll condition
		// the source (and target) rects both shrink from the bottom, and the target rect keeps its top() moving downward
		// (where downward is +y)

		//the source rect
		m_sourceRect.setTopLeft(m_sourceGeom.topLeft());
		m_sourceRect.setHeight(qMin(m_maxSourceRectHeight,m_screenGeom.height()+v));  //v is neg, so this == - abs(v)
		m_sourceRect.setWidth((qreal)m_maxSourceRectWidth);

		//TODO: PIXEL-ALIGN
		QSize sourceRectPixelSize = m_sourceRect.toRect().size();
		//the target rect. It's the same size as the source rect, with its bottom() aligned with the screen's bottom
		m_targetRect.setTopLeft(m_screenGeom.topLeft()+QPoint(0,qMin(-v,m_screenGeom.height())));
		m_targetRect.setHeight(sourceRectPixelSize.height());
		m_targetRect.setWidth(sourceRectPixelSize.width());

		m_overscrollVal = v;
	}
	else
	{
//		//qDebug() << "(svc BOTTOM-Over) v = " << v;
		//bottom overscroll condition
		// the source (and target) rects both shrink from the top, and the target rect keeps its bottom() moving upward
		// (where upward is -y)

		//the source rect
		m_sourceRect.setTopLeft(m_sourceGeom.topLeft()+QPointF(0,qMin(v,m_sourceContentSize.height())));
		m_sourceRect.setHeight(qMax(0,m_sourceContentSize.height()-v));
		m_sourceRect.setWidth((qreal)m_maxSourceRectWidth);

		//TODO: PIXEL-ALIGN
		QSize sourceRectPixelSize = m_sourceRect.toRect().size();
		//the target rect. It's the same size as the source rect, with its top() aligned with the screen's top
		m_targetRect.setTopLeft(m_screenGeom.topLeft());
		m_targetRect.setHeight(sourceRectPixelSize.height());
		m_targetRect.setWidth(sourceRectPixelSize.width());

		m_overscrollVal = v;
	}

	m_inOverscroll = true;
	update(m_boundingRect);
}

//virtual
qint32 ScrollingLayoutRenderer::rawScrollValue() const
{
	return DimensionsGlobal::roundDown(m_sourceRect.top());
}

//virtual
bool ScrollingLayoutRenderer::inOverscroll() const
{
	//do nothing
	return m_inOverscroll;
}

//virtual
qint32 ScrollingLayoutRenderer::scrollValueNeededToEscapeOverscroll()
{
	if (!m_inOverscroll)
	{
		return 0;
	}
	if (m_overscrollVal < 0)
	{
		return -m_overscrollVal;
	}

	//else in positive, i.e. bottom- overscroll

	//if the content is "small", then just bring it to 0
	if (m_sourceContentSize.height() <= m_screenGeom.height())
	{
		return -m_overscrollVal;
	}

	//else, bring the bottom edges into alignment
	//TODO: PIXEL-ALIGN - rounding may miss the final 1px of scroll or may scroll too far by 1px
	return -(m_overscrollVal-(m_sourceGeom.height() - (qreal)m_screenGeom.height()));
}

//virtual
quint32 ScrollingLayoutRenderer::scrollAmountUntilTopOverscroll()
{
//	qDebug() << "scroll v = " << scrollValue() << " , raw scroll v = " << rawScrollValue() << " , top lim = " << topLimit();
	if (scrollValue() <= topLimit())
	{
		return 0;	//already in top overscroll
	}
	return (quint32)qAbs(scrollValue());
}

//virtual
quint32 ScrollingLayoutRenderer::scrollAmountUntilBottomOverscroll()
{
//	qDebug() << "scroll v = " << scrollValue() << " , raw scroll v = " << rawScrollValue() << " , bottom lim = " << bottomLimit();
	if (scrollValue() >= bottomLimit())
	{
		return 0;	//already in bottom overscroll
	}
	return (quint32)qAbs(bottomLimit() - scrollValue());
}

//virtual
qint32 ScrollingLayoutRenderer::topLimit() const
{
	return 0;
}

//virtual
qint32 ScrollingLayoutRenderer::bottomLimit()  const
{
	if (m_sourceContentSize.height() <= m_screenGeom.height())
	{
		return 0;
	}
	return (qint32)(m_sourceGeom.height() - (qreal)m_screenGeom.height());
}

//virtual
QPointF ScrollingLayoutRenderer::mapToContentSpace(const QPointF& scrollerSpacePointF)
{
	//the point must be in the target rect, or else it didn't come from anywhere in the
	//source rect
	if (!m_targetRect.contains(scrollerSpacePointF.toPoint()))
	{
		return QPointF();
	}

	//for this renderer, the source and target rects are simply translation maps (no scale or shear)
	// and the coordinate systems for both rects go in the same directions and are "positive"
	// (+x to the right, +y to the bottom)

	return QPointF((scrollerSpacePointF-m_targetRect.topLeft())+m_sourceRect.topLeft());

}

//virtual
QPointF ScrollingLayoutRenderer::mapToContentSpace(const QPoint& scrollerSpacePoint)
{
	return mapToContentSpace(QPointF(scrollerSpacePoint));
}
//TRANSFORMER: allowing extrapolate
//virtual
QPointF ScrollingLayoutRenderer::mapFromContentSpace(const QPointF& contentSpacePointF)
{
	//the point must be in the source rect, or else it's not in the target rect
//	if (!m_sourceRect.contains(contentSpacePointF))
//	{
//		return QPointF();
//	}

	//for this renderer, the source and target rects are simply translation maps (no scale or shear)
	// and the coordinate systems for both rects go in the same directions and are "positive"
	// (+x to the right, +y to the bottom)
	return QPointF((contentSpacePointF-m_sourceRect.topLeft())+m_targetRect.topLeft());
}

//virtual
bool ScrollingLayoutRenderer::mapToContentSpace(const QPointF& scrollerSpacePointF,QPointF& r_mappedPointF)
{
	//the point must be in the target rect, or else it didn't come from anywhere in the
	//source rect
	if (!m_targetRect.contains(scrollerSpacePointF.toPoint()))
	{
		return false;
	}

	//for this renderer, the source and target rects are simply translation maps (no scale or shear)
	// and the coordinate systems for both rects go in the same directions and are "positive"
	// (+x to the right, +y to the bottom)

	r_mappedPointF = QPointF((scrollerSpacePointF-m_targetRect.topLeft())+m_sourceRect.topLeft());
	return true;
}

//virtual
bool ScrollingLayoutRenderer::mapToContentSpace(const QPoint& scrollerSpacePoint,QPointF& r_mappedPointF)
{
	return mapToContentSpace(QPointF(scrollerSpacePoint),r_mappedPointF);
}

//virtual
bool ScrollingLayoutRenderer::mapFromContentSpace(const QPointF& contentSpacePointF,QPointF& r_mappedScrollerSpacePointF)
{
	//the point must be in the source rect, or else it's not in the target rect
	if (!m_sourceRect.contains(contentSpacePointF))
	{
		return false;
	}

	//for this renderer, the source and target rects are simply translation maps (no scale or shear)
	// and the coordinate systems for both rects go in the same directions and are "positive"
	// (+x to the right, +y to the bottom)
	r_mappedScrollerSpacePointF = QPointF((contentSpacePointF-m_sourceRect.topLeft())+m_targetRect.topLeft());
	return true;
}

//virtual
bool ScrollingLayoutRenderer::resize(quint32 w,quint32 h)
{
	//TODO: return codes
	(void)ScrollableObject::resize(w,h);
	m_maxSourceRectWidth = qMin(m_screenGeom.width(),m_sourceContentSize.width());
	m_maxSourceRectHeight = qMin(m_screenGeom.height(),m_sourceContentSize.height());
	resetToInitialSourceArea();
	resetToInitialTargetArea();
	if (m_sourceContentSize.height() <= m_screenGeom.height())
	{
		m_inOverscroll = true;
		m_overscrollVal = 0;
	} else {
		m_inOverscroll = false;
		m_overscrollVal = 0;
	}
	return true;
}

/////protected Q_SLOTS:

//virtual
void ScrollingLayoutRenderer::slotSourceGeomChanged(const QRectF& newGeom)
{
	setSourceContentGeom(newGeom);
}

//virtual
void ScrollingLayoutRenderer::slotSourceContentSizeChanged(const QSizeF& newContentSize)
{
	setSourceContentGeom(DimensionsGlobal::realRectAroundRealPoint(newContentSize));
}

//virtual
void ScrollingLayoutRenderer::slotSourceContentSizeChanged(const QSize& newContentSize)
{
	setSourceContentGeom(DimensionsGlobal::realRectAroundRealPoint(newContentSize));
}

/////protected:

//virtual
void ScrollingLayoutRenderer::resetToInitialSourceArea()
{
	//for this type of content, the source rect starts at m_sourceGeom.topLeft
	m_sourceRect = QRectF(m_sourceGeom.topLeft(),QSizeF((qreal)m_maxSourceRectWidth,(qreal)m_maxSourceRectHeight));
}

//virtual
void ScrollingLayoutRenderer::resetToInitialTargetArea()
{
	m_targetRect = QRect(m_screenGeom.topLeft(),QSize(m_maxSourceRectWidth,m_maxSourceRectHeight));
}

//virtual
void ScrollingLayoutRenderer::setSourceContentGeom(const QRectF& newContentGeom)
{
	m_sourceGeom = newContentGeom;
	//TODO: PIXEL-ALIGN
	m_sourceContentSize = m_sourceGeom.toAlignedRect().size();
	m_maxSourceRectWidth = qMin(m_screenGeom.width(),m_sourceContentSize.width());
	m_maxSourceRectHeight = qMin(m_screenGeom.height(),m_sourceContentSize.height());
	resetToInitialSourceArea();
	resetToInitialTargetArea();
	if (m_sourceContentSize.height() <= m_screenGeom.height())
	{
		m_inOverscroll = true;
		m_overscrollVal = 0;
	} else {
		m_inOverscroll = false;
		m_overscrollVal = 0;
	}
}

//virtual
void ScrollingLayoutRenderer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	/*
	 *
	 * Because this is a realtime rendered item and not a pre-rendered pixmap to just display in pieces, like
	 * ScrollingSurface, this is essentially a paint of a piece (rectangle) of the source content layout into a target
	 * rectangluar region of this painter. Practically, it will always be a 1:1 scale between source and target rects
	 * (i.e. no stretch or compress as I'm rendering/painting the content), and there shouldn't be any clipping since
	 * the IconLayout's paint___() functions for these partial paints should just not render things that would be clipped anyways
	 *
	 */
	QTransform saveTran = painter->transform();
	painter->translate(m_targetRect.topLeft());			//TODO: IMPROVE: necessary to trick the layout paints into the right place
														// better way would be to pass in the origin to which the painters
														// in there should be calculating paint coords
	if (OperationalSettings::settings()->useStagedRendering)
	{
		//Staged rendering paints in the following order:
		// Icon and Frame
		// decorators
		// Labels
		// Horiz. Div pix line
		// Horiz. Div label
		m_qp_layoutObject->paint(painter,QRectF(m_sourceRect),IconRenderStage::Icon | IconRenderStage::IconFrame);
		m_qp_layoutObject->paint(painter,QRectF(m_sourceRect),IconRenderStage::Decorators);
		m_qp_layoutObject->paint(painter,QRectF(m_sourceRect),IconRenderStage::Label);
		m_qp_layoutObject->paint(painter,QRectF(m_sourceRect),IconRenderStage::LAST * (2 << LabeledDivRenderStage::DivPix));		//these are all constants at compile time. Hopefully the compiler is smart enough to resolve it then
		m_qp_layoutObject->paint(painter,QRectF(m_sourceRect),IconRenderStage::LAST * (2 << LabeledDivRenderStage::Label));
	}
	else
	{
		m_qp_layoutObject->paint(painter,QRectF(m_sourceRect));
	}
	painter->setTransform(saveTran);
//	qDebug() << __FUNCTION__ << "painter org: " << m_targetRect.center()
//			<< " targetRect: " << m_targetRect << " , sourceRect: " << m_sourceRect;
//	painter->drawRect(this->boundingRect());
}
