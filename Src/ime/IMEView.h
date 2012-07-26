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




#ifndef IMEVIEW_H
#define IMEVIEW_H

#include <QGraphicsObject>

#include "IMEDataInterface.h"
#include <stdint.h>

class QTapGesture;
class ScreenEdgeFlickGesture;

class IMEView : public QGraphicsObject
{
	Q_OBJECT

public:
	IMEView(QGraphicsItem* parent = 0);

	void attach(IMEDataInterface* imeDataInterface);

	void setBoundingRect(const QRectF& r);
	QRectF boundingRect() const;

	void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

public Q_SLOTS:
	void invalidateRect(const QRect &  rect);

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual bool sceneEvent(QEvent* event);
	
	void touchBegin(QTouchEvent* te);
	void touchUpdate(QTouchEvent* te);
	void touchEnd(QTouchEvent* te);

	void tapEvent(QTapGesture* tap);
	void screenEdgeFlickEvent(ScreenEdgeFlickGesture* gesture);

public:
	bool acceptPoint(const QPointF& pt);

private:
	QRectF m_bounds;
	IMEDataInterface* m_imeDataInterface;
	bool m_acceptingInput;
	uint64_t m_lastTouchBegin;
};

#endif

