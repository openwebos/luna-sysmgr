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




#ifndef SCROLLINGLAYOUTRENDERER_H_
#define SCROLLINGLAYOUTRENDERER_H_

#include "scrollableobject.h"
#include <QPointer>

class IconLayout;
class ScrollingLayoutRenderer : public ScrollableObject
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:
	ScrollingLayoutRenderer(const QRectF& geometry,IconLayout& layout);
	virtual ~ScrollingLayoutRenderer();

	virtual qint32 scrollValue() const;
	virtual void setScrollValue(qint32 v);
	virtual qint32 rawScrollValue() const;
	virtual bool inOverscroll() const;
	virtual qint32 scrollValueNeededToEscapeOverscroll();
	virtual quint32 scrollAmountUntilTopOverscroll();
	virtual quint32 scrollAmountUntilBottomOverscroll();

	virtual qint32 topLimit() const;
	virtual qint32 bottomLimit()  const;

	virtual QPointF mapToContentSpace(const QPointF& scrollerSpacePointF);
	virtual QPointF mapToContentSpace(const QPoint& scrollerSpacePoint);
	virtual QPointF mapFromContentSpace(const QPointF& contentSpacePointF);

	virtual bool mapToContentSpace(const QPointF& scrollerSpacePointF,QPointF& r_mappedPointF);
	virtual bool mapToContentSpace(const QPoint& scrollerSpacePoint,QPointF& r_mappedPointF);
	virtual bool mapFromContentSpace(const QPointF& contentSpacePointF,QPointF& r_mappedScrollerSpacePointF);

	virtual bool resize(quint32 w,quint32 h);

protected Q_SLOTS:

	virtual void slotSourceGeomChanged(const QRectF& newGeom);
	virtual void slotSourceContentSizeChanged(const QSizeF& newContentSize);
	virtual void slotSourceContentSizeChanged(const QSize& newContentSize);

protected:

	virtual void resetToInitialSourceArea();
	virtual void resetToInitialTargetArea();

	//used in the case where the content size is smaller than the screen size
	virtual void setScrollValueSmallContent(qint32);

	virtual void setSourceContentGeom(const QRectF& newContentGeom);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);

	QPointer<IconLayout>	m_qp_layoutObject;
	QSize m_sourceContentSize;			//in pixels, for comparison to screen geom
};

#endif /* SCROLLINGLAYOUTRENDERER_H_ */
