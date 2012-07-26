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




#include "thingpaintable.h"
#include "dimensionsglobal.h"
//public:

//static
int ThingPaintable::MagicTypePropertyKey = 1111;
int ThingPaintable::MagicTypePropertyValue = 2222;
//static
bool ThingPaintable::isItemThingPaintable(QGraphicsItem * p_qgitem)
{
	if (!p_qgitem)
	{
		return false;
	}
	return (p_qgitem->data(MagicTypePropertyKey).toInt() == MagicTypePropertyValue);
}

//static
ThingPaintable * ThingPaintable::thingpaintable_cast(QGraphicsItem * p_qgitem)
{
	if (!p_qgitem)
	{
		return 0;
	}
	if (p_qgitem->data(MagicTypePropertyKey).toInt() == MagicTypePropertyValue)
	{
		//safe to static downcast now, unless of course this little honor-code type system was subverted,
		// in which case, you deserve the crash that's coming, coming right soon!
		return static_cast<ThingPaintable *>(p_qgitem);
	}
	return 0;	//wasn't a ThingPaintable
}

ThingPaintable::ThingPaintable(const QRectF& geom)
: m_geom(geom)
{
	m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
	//mark myself as a thing paintable so the outside-QObject world can find its way home
	//(see statics above)
	setData(MagicTypePropertyKey,MagicTypePropertyValue);
}

ThingPaintable::ThingPaintable(const QUuid& specificUid,const QRectF& geom)
: Thing(specificUid)
, m_geom(geom)
{
	m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
	//mark myself as a thing paintable so the outside-QObject world can find its way home
	//(see statics above)
	setData(MagicTypePropertyKey,MagicTypePropertyValue);
}

//virtual
ThingPaintable::~ThingPaintable()
{
}

//virtual
QRectF ThingPaintable::positionRelativeGeometry() const
{
	return m_geom.translated(pos());
}

//virtual
QRectF ThingPaintable::untranslateFromPosition(const QRectF& rect) const
{
	return rect.translated(-pos());
}

//virtual
void ThingPaintable::recomputeBoundingRect()
{
	prepareGeometryChange();
	m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
}

//virtual
void ThingPaintable::recomputeBoundingRect(const QRectF& virtualGeom)
{
	prepareGeometryChange();
	m_boundingRect = virtualGeom.adjusted(-0.5,-0.5,0.5,0.5);
}

//virtual
bool ThingPaintable::resize(quint32 newWidth,quint32 newHeight)
{
	return resize(QSize(newWidth,newHeight));
}

//virtual
bool ThingPaintable::resize(const QSize& s)
{

	m_geom = DimensionsGlobal::realRectAroundRealPoint(s);
	recomputeBoundingRect();
	//return false to remind of un-reimplemented resize() in subclasses, which is bad thing
	return false;
}

//virtual
void ThingPaintable::paint(QPainter *painter, const QRectF& sourceRect)
{
	//do nothing
}

//virtual
void ThingPaintable::paint(QPainter *painter, const QRectF& sourceRect,qint32 opt)
{
	//do nothing
}

//virtual
void ThingPaintable::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate)
{

}

//virtual
void ThingPaintable::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt)
{

}

//virtual
void ThingPaintable::paintOffscreen(QPainter *painter,const QRect& sourceRect,const QPoint& targetOrigin)
{
	//do nothing
}

//virtual
void ThingPaintable::paintOffscreen(QPainter *painter,const QRect& sourceRect,const QRect& targetRect)
{
	//do nothing
}


uint qHash(const ThingPaintable& t)
{
	return (qHash((Thing&)t));
}

uint qHash(const QPointer<ThingPaintable>& qpt)
{
	return qHash(QPointer<Thing>(qpt));
}
