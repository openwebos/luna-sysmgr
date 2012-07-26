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




#include "thing.h"
#include <QString>
#include <QRectF>

#ifndef THINGPAINTABLE_H_
#define THINGPAINTABLE_H_

class ThingPaintable : public Thing
{
	Q_OBJECT
	Q_PROPERTY(QString uistate READ readUiState WRITE writeUiState RESET resetUiState NOTIFY signalUiStateChanged)

	//this property is needed because QGraphicsObject doesn't provide for position changed signals, and
	// during icon animations, I need to know to update-paint the layout and scroller within Page
	// (see code for Page, ScrollableObject, and IconLayout)
	// Even though OpenGL the way this system uses it repaints the whole screen on each update, Qt itself doesn't
	// have that assumption. On a regular setPos() on a QGraphicsObject, only that object and anything under it will
	// be marked for paint updates...but this isn't correct necessarily because the scroller and layout will partially
	// paint based on a "viewport" area that the scroller set. So the paint() needs to go through the scroller,
	// which means I need a way to signal that sequence...
	Q_PROPERTY(QPointF animatePosition READ pos WRITE setPos NOTIFY signalPositionChanged)

public:

	//used to identify ThingPaintables when they are in a list returned by QGraphicsView/QGraphicsScene. Since QGraphicsItems
	//	are not QObject, I cannot use qobject_casts as I would normally to determine type. Thus, I'll use QGI's setData()/data()
	//	with this key to determine
	static int MagicTypePropertyKey;
	static int MagicTypePropertyValue;
	static bool isItemThingPaintable(QGraphicsItem * p_qgitem);
	//this is a convenience, a-la qobject_cast...same semantics
	static ThingPaintable * thingpaintable_cast(QGraphicsItem * p_qgitem);

	ThingPaintable(const QRectF& geom);
	virtual ~ThingPaintable();

	// object properties
	virtual QString readUiState() const { return m_uiState; }
	virtual void writeUiState(const QString& s) { m_uiState = s; }
	virtual void resetUiState() { m_uiState = QString(""); }

	//from Thing
	virtual QRectF geometry() const { return m_geom; }

	//this
	virtual QRectF positionRelativeGeometry() const;
	virtual QRectF untranslateFromPosition(const QRectF& rect) const;
	virtual QRectF boundingRect() const { return m_boundingRect; }

	virtual bool resize(const QSize& newSize);
	virtual bool resize(quint32 newWidth,quint32 newHeight);

	//these two are required...
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0) = 0;
	virtual void paintOffscreen(QPainter *painter) = 0;

	//these are optional
	virtual void paint(QPainter *painter, const QRectF& sourceRect);
	virtual void paint(QPainter *painter, const QRectF& sourceRect,qint32 renderOpt);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt);

	virtual void paintOffscreen(QPainter *painter,const QRect& sourceRect,const QPoint& targetOrigin);
	virtual void paintOffscreen(QPainter *painter,const QRect& sourceRect,const QRect& targetRect);

Q_SIGNALS:

// params:
	//[0] old geom
	//[1] new geom
	void signalGeometryChanged(const QRectF&,const QRectF&);
	void signalGeometryChanged();

	void signalUiStateChanged();
	void signalPositionChanged();

protected:

	ThingPaintable(const QUuid& specificUid,const QRectF& geom);

	virtual void   	recomputeBoundingRect();
	virtual void	recomputeBoundingRect(const QRectF& virtualGeom);

protected:

	QRectF	m_geom;
	QRectF	m_boundingRect;

	// object properties
	QString m_uiState;
};

uint qHash(const ThingPaintable& t);
uint qHash(const QPointer<ThingPaintable>& qpt);

#endif /* THINGPAINTABLE_H_ */
