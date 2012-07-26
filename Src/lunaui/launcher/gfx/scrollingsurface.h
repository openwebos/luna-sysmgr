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




#ifndef SCROLLINGSURFACE_H_
#define SCROLLINGSURFACE_H_

#include "scrollableobject.h"
#include <QRectF>
#include <QRect>
#include <QPointer>

class PixmapObject;
class QPainter;
class QGraphicsSceneMouseEvent;

class ScrollingSurface : public ScrollableObject
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:

	ScrollingSurface(const QRectF& geometry);
	virtual ~ScrollingSurface();

	void setPixmapObject(PixmapObject * p_pmo);

	bool testAndtranslateScrollSurfacePointToPixmapPoint(const QPointF& point,QPoint& r_translatedPoint);

	virtual qint32 scrollValue() const;
	virtual void setScrollValue(qint32 v);
	virtual qint32 rawScrollValue() const;

public Q_SLOTS:

	//this one ignores content geom changes because it's bound to the pixmap.
	//TODO: revisit this idea; perhaps some tricky cool things can be done by messing
	// with the geom despite a fixed pixmap size
	virtual void slotSourceGeomChanged(const QRectF& newGeom);
	virtual void slotSourceContentSizeChanged(const QSizeF& newContentSize);
	virtual void slotSourceContentSizeChanged(const QSize& newContentSize);

protected:

	virtual void resetTargetRectValid();

	virtual void setSourceContentGeom(const QRectF& newContentGeom);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);

	QPointer<PixmapObject> m_qp_surfacePmo;
};

#endif /* SCROLLINGSURFACE_H_ */
