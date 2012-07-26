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




#include "dimensionsglobal.h"
#include "WindowServer.h"
#include "WindowServerLuna.h"
#include <QGraphicsScene>
#include <QState>
#include <QStateMachine>

namespace DimensionsGlobal
{

qreal roundDownF(qreal v)
{
	return qFloor(v);
}
qint32 roundDown(qreal v)
{
	return (qint32)qFloor(v);
}

qreal roundUpF(qreal v)
{
	return qCeil(v);
}
qint32 roundUp(qreal v)
{
	return (qint32)qCeil(v);
}

bool isZeroF(qreal v)
{
	//(all your zerof are belong to us)
	return qFuzzyCompare(v+1.0,1.0);
}

static qint32 emask = (~0)-1;
static quint32 uemask = (~0)-1;
qint32 Even(qint32 v)
{
	return v & emask;
}

quint32 Even(quint32 v)
{
	return v & uemask;
}

//approximateDistance() is implemented from a fast hypotenuse algorithm
//ATTRIBUTION: Alan W. Paeth, published in "Graphics Gems", Academic Press Professional, Inc. San Diego, CA, USA ,(c) 1990
quint32 approximateDistance(const QPoint& p1,const QPoint& p2)
{
	qint32 x;
	qint32 y;
	qint32 min;
	if ((x = p2.x() - p1.x()) < 0)
	{
		x = -x;
	}
	if ((y = p2.y() - p1.y()) < 0)
	{
		y = -y;
	}
	min = (x > y ? y : x);
	return (quint32)(x+y - (min >> 1));
}

//this one is just convenience...probably can be rewritten a bit better
quint32	approximateDistance(const QPointF& pf1,const QPointF& pf2)
{
	QPoint p1 = pf1.toPoint();
	QPoint p2 = pf2.toPoint();
	qint32 x;
	qint32 y;
	qint32 min;
	if ((x = p2.x() - p1.x()) < 0)
	{
		x = -x;
	}
	if ((y = p2.y() - p1.y()) < 0)
	{
		y = -y;
	}
	min = (x > y ? y : x);
	return (quint32)(x+y - (min >> 1));
}

qint32 realAsPixelPosition(const qreal v)
{
	return (qint32)qFloor(v);
}

qint32 realAsPixelSize(const qreal v)
{
	return (qint32)qCeil(v);
}

QRectF realRectToPixelCompatibleForm(const QRectF& rect)
{
	return QRectF(	(qreal)realAsPixelPosition(rect.x()),
					(qreal)realAsPixelPosition(rect.y()),
					(qreal)realAsPixelSize(rect.width()),
					(qreal)realAsPixelSize(rect.height()));

}

QPoint realPointAsPixelPosition(const QPointF& p)
{
	return (QPoint(realAsPixelPosition(p.x()),realAsPixelPosition(p.y())));
}

QPoint realPointAsPixelSize(const QPointF& p)
{
	return (QPoint(realAsPixelSize(p.x()),realAsPixelSize(p.y())));
}

QPointF positionFromSizeF(const QSizeF& s)
{
	return positionFromSize(QSize(realAsPixelPosition(s.width()),realAsPixelPosition(s.height())));
}

QPointF positionFromSize(const QSize& s)
{
	return QPointF((qreal)(s.width()),(qreal)(s.height()));
}

QPointF positionFromFraction(const QPointF& basePoint,
							const QSizeF& distance,const qreal xFractional, const qreal yFractional)
{
	if (qFuzzyCompare(xFractional+1.0,1.0) || qFuzzyCompare(yFractional+1.0,1.0))
		return basePoint;		//0 divisor
	return (basePoint + realPointAsPixelSize(QPointF(distance.width()/xFractional,distance.height()/yFractional)));
}

QPointF reoriginTopLeftToCenter(const QPointF& origin,const QSizeF& size)
{
	return realPointAsPixelPosition(origin+QPointF(realAsPixelSize(size.width()/2.0),realAsPixelSize(size.height()/2.0)));
}

QPointF reoriginCenterTopLeft(const QPointF& origin,const QSizeF& size)
{
	return realPointAsPixelPosition(origin-QPointF(realAsPixelSize(size.width()/2.0),realAsPixelSize(size.height()/2.0)));
}

QPointF realPointOnRectF(const QRectF& rect,RectPointName::Enum pointName)
{
	switch (pointName)
	{
	case RectPointName::TopLeft:
		return rect.topLeft();
	case RectPointName::TopRight:
		return rect.topRight();
	case RectPointName::BottomLeft:
		return rect.bottomLeft();
	case RectPointName::BottomRight:
		return rect.bottomRight();
	default:
	case RectPointName::Center:
		return rect.center();
	case RectPointName::TopCenter:
		return QPointF(rect.center().x(),rect.top());
	case RectPointName::RightCenter:
		return QPointF(rect.right(),rect.center().y());
	case RectPointName::BottomCenter:
		return QPointF(rect.center().x(),rect.bottom());
	case RectPointName::LeftCenter:
		return QPointF(rect.left(),rect.center().y());
	}
	return QPointF();	//keep compilers happy
}

QRectF moveRectFcopy(const QRectF& rect,const QPointF& moveToPoint,RectPointName::Enum referencePointName)
{
	QRectF r;
	moveRectF(r,moveToPoint,referencePointName);
	return r;
}

QRectF& moveRectF(QRectF& rect,const QPointF& moveToPoint,RectPointName::Enum referencePointName)
{
	/*
	 * TopLeft = RectCornerName::TopLeft,
		TopRight = RectCornerName::TopRight,
		BottomLeft = RectCornerName::BottomLeft,
		BottomRight = RectCornerName::BottomRight,
		Center,
		TopCenter,
		RightCenter,
		BottomCenter,
		LeftCenter

	 */
	qreal hw,hh;
	switch (referencePointName)
	{
	case RectPointName::TopLeft:
		rect.moveTopLeft(moveToPoint);
		break;
	case RectPointName::TopRight:
		rect.moveTopRight(moveToPoint);
		break;
	case RectPointName::BottomLeft:
		rect.moveBottomLeft(moveToPoint);
		break;
	case RectPointName::BottomRight:
		rect.moveBottomRight(moveToPoint);
		break;
	default:
	case RectPointName::Center:
		rect.moveCenter(moveToPoint);
		break;
	case RectPointName::TopCenter:
		hw = roundDownF(rect.width()/2.0);
		rect.moveTopLeft(moveToPoint-QPointF(hw,0.0));
		break;
	case RectPointName::RightCenter:
		hh = roundDownF(rect.height()/2.0);
		rect.moveTopLeft(moveToPoint+QPoint(roundDownF(rect.width()),hh));
		break;
	case RectPointName::BottomCenter:
		hw = roundDownF(rect.width()/2.0);
		rect.moveTopLeft(moveToPoint+QPointF(hw,roundDownF(rect.height())));
		break;
	case RectPointName::LeftCenter:
		hh = roundDownF(rect.height()/2.0);
		rect.moveTopLeft(moveToPoint+QPoint(0.0,hh));
		break;
	}
	return rect;
}

QRectF relativeGeom(const QRectF& geometry,const QPointF& position)
{
	return geometry.translated(position);
}

//rounds-down, to be logically equivalent to AsPixelPosition (yes, it's a little weird due to 'size' in the name)
QSizeF fractionalSizeF(const QSizeF& size,const qreal wFractional, const qreal hFractional)
{
	if (isZeroF(wFractional) || isZeroF(hFractional))
		return size;
	return QSizeF(roundDownF(size.width()/wFractional),roundDownF(size.height()/hFractional));
}

QSize fractionalSize(const QSizeF& size,const qreal wFractional, const qreal hFractional)
{
	return (fractionalSizeF(size,wFractional,hFractional).toSize());
}

qreal fractionalSizeF(qreal v,qreal fractional)
{
	return roundDownF(v/fractional);
}

quint32 fractionalSize(quint32 v,qreal fractional)
{
	return (quint32)roundDown((qreal)v/fractional);
}

qreal fractionalSizeF(qreal v,quint32 fractional)
{
	return roundDownF(v/(qreal)fractional);
}

quint32 fractionalSize(quint32 v,quint32 fractional)
{
	return (quint32)roundDown((qreal)v/(qreal)fractional);
}

//use this to get consistent bounding rectangles around a center point
QRectF realRectAroundRealPoint(const QPointF& centerPt,const QSizeF& rectSize)
{
	QPointF topleft = QPointF(centerPt)
					- QPointF(realPointAsPixelPosition(QPointF(rectSize.width()/2.0,rectSize.height()/2.0)));
	return QRectF(topleft,rectSize);
}

QRectF realRectAroundRealPoint(const QSizeF& rectSize)
{
	QPointF topleft = -QPointF(realPointAsPixelPosition(QPointF(rectSize.width()/2.0,rectSize.height()/2.0)));
	return QRectF(topleft,rectSize);
}

QRectF realRectAroundRealPoint(const QSize& rectSize)
{
	return realRectAroundRealPoint(QSizeF(rectSize));
}

//use this for "smaller squares into bigger square" class of problems
quint32 howManyFitHorizontally(const quint32 cellWidth,const quint32 areaWidth)
{
	if (cellWidth == 0)
	{
		return 0;
	}
	return (quint32)roundDown((qreal)areaWidth / (qreal)cellWidth);
}

quint32 howManyFitHorizontally(const qreal cellWidthF,const qreal areaWidthF)
{
	if (isZeroF(cellWidthF))
	{
		return 0;
	}
	return (quint32)roundDown(areaWidthF / cellWidthF);
}

quint32 howManyFitHorizontally(const QSize& cellSize,const QSize& areaSize)
{
	return howManyFitHorizontally((quint32)areaSize.width(),(quint32)cellSize.width());
}

quint32 howManyFitHorizontally(const QSizeF& cellSizeF,const QSizeF& areaSizeF)
{
	return howManyFitHorizontally(areaSizeF.width(),cellSizeF.width());
}

quint32 maxCellWidth(const quint32 howManyCells,const quint32 areaWidth)
{
	return (quint32)roundDown((qreal)areaWidth / (qreal)howManyCells);
}

quint32 maxCellWidth(const quint32 howManyCells,const qreal areaWidthF)
{
	return (quint32)roundDown(areaWidthF / (qreal)howManyCells);
}
quint32 maxCellWidth(const quint32 howManyCells,const QSize& areaSize)
{
	return maxCellWidth(howManyCells,(quint32)areaSize.width());
}
quint32 maxCellWidth(const quint32 howManyCells,const QSizeF& areaSizeF)
{
	return maxCellWidth(howManyCells,areaSizeF.width());
}

quint32 indentForCenteredDimensions(const quint32 outerDimension,const quint32 innerDimension)
{
	if (outerDimension < innerDimension)
		return 0;

	return roundUp(((qreal)(outerDimension-innerDimension))/2.0);
}

qreal 	indentForCenteredDimensions(const qreal outerDimensionF,const qreal innerDimensionF)
{
	if (outerDimensionF < innerDimensionF)
		return 0;

	return (outerDimensionF-innerDimensionF)/2.0;
}

QPointF centerRectInRect(const QRectF& outer,const QRectF& inner)
{
	//unpredictable if rects have negative width or height
	if ((outer.width() < inner.width()) || (outer.height() < inner.height()))
	{
		return QPointF(0,0);
	}
	return QPointF(indentForCenteredDimensions(outer.width(),inner.width()),
			indentForCenteredDimensions(outer.height(),inner.height()));

}
QPoint	centerRectInRectPixelAlign(const QRectF& outer,const QRectF& inner)
{
	return realPointAsPixelPosition(QPointF(indentForCenteredDimensions(outer.width(),inner.width()),
				indentForCenteredDimensions(outer.height(),inner.height())));
}

QPoint	centerRectInRectPixelAlign(const QRect& outer,const QRect& inner)
{
	return QPoint(indentForCenteredDimensions((quint32)outer.width(),(quint32)inner.width()),
			indentForCenteredDimensions((quint32)outer.height(),(quint32)inner.height()));
}

QPointF mapFromGlobalCS(const QPointF& globalPointF)
{
	WindowServer * pWs = WindowServer::instance();
	if (!pWs)
	{
		return QPointF();
	}
	return QPointF(pWs->mapFromGlobal(globalPointF.toPoint()));
}

QGraphicsScene * globalGraphicsScene()
{
	WindowServer * pWs = WindowServer::instance();
	if (!pWs)
	{
		return 0;
	}
	return pWs->scene();
}

QState* createFSMState(const QString& stateName,QState * parent)
{
	QState *result = new QState(parent);
	result->setObjectName(stateName);
	return result;
}

} //end namespace

int qHash(const QUuid& q)
{
	return qHash(q.toString());
}
