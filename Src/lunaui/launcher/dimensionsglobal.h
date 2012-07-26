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




#ifndef DIMENSIONSGLOBAL_H_
#define DIMENSIONSGLOBAL_H_

#include <qglobal.h>
#include <qmath.h>
#include <QRectF>
#include <QPoint>
#include <QPointF>
#include <QUuid>
#include <QHash>
#include <QVariantMap>

class QStateMachine;
class QState;
class QGraphicsScene;

namespace DimensionsGlobal
{

namespace RectCornerName
{
 	 enum Enum
 	 {
 		 TopLeft,
 		 TopRight,
 		 BottomLeft,
 		 BottomRight
 	 };
}
namespace RectPointName
{
	enum Enum
	{
		TopLeft = RectCornerName::TopLeft,
		TopRight = RectCornerName::TopRight,
		BottomLeft = RectCornerName::BottomLeft,
		BottomRight = RectCornerName::BottomRight,
		Center,
		TopCenter,
		RightCenter,
		BottomCenter,
		LeftCenter
	};
}

qreal roundDownF(qreal v);
qint32 roundDown(qreal v);
qreal roundUpF(qreal v);
qint32 roundUp(qreal v);
bool isZeroF(qreal v);

qint32 Even(qint32 v);
quint32 Even(quint32 v);

quint32 approximateDistance(const QPoint& p1,const QPoint& p2);
quint32	approximateDistance(const QPointF& pf1,const QPointF& pf2);

qint32 realAsPixelPosition(const qreal v);
qint32 realAsPixelSize(const qreal v);
QPoint realPointAsPixelPosition(const QPointF& p);
QPoint realPointAsPixelSize(const QPointF& p);
QRectF realRectToPixelCompatibleForm(const QRectF& rect);

QPointF positionFromSizeF(const QSizeF& s);
QPointF positionFromSize(const QSize& s);
QPointF positionFromFraction(const QPointF& basePoint,
							const QSizeF& distance,const qreal xFractional, const qreal yFractional);
//returns the point that is the center, if "origin" passed in is the assumed top-left
// this is essentially a convenience function of positionFromFraction()
QPointF reoriginTopLeftToCenter(const QPointF& origin,const QSizeF& size);
//and this one is the opposite
QPointF reoriginCenterTopLeft(const QPointF& origin,const QSizeF& size);

//REPOSITIONING HELPERS - unlike most of the other functions here, these DO NOT "fix" real values
//	to be consistent with screen/pixel coordinates.
QPointF realPointOnRectF(const QRectF&,RectPointName::Enum pointName);
QRectF moveRectFcopy(const QRectF& rect,const QPointF& moveToPoint,RectPointName::Enum referencePointName);
QRectF& moveRectF(QRectF& rect,const QPointF& moveToPoint,RectPointName::Enum referencePointName);
QRectF relativeGeom(const QRectF& geometry,const QPointF& position);

//divides the size by the "fractionals" (divisors). Just like everything else here, use this for
// consistent calculations throughout w.r.t. rounding
QSizeF fractionalSizeF(const QSizeF& size,const qreal wFractional, const qreal hFractional);
QSize fractionalSize(const QSizeF& size,const qreal wFractional, const qreal hFractional);

qreal fractionalSizeF(qreal v,qreal fractional);
quint32 fractionalSize(quint32 v,qreal fractional);
qreal fractionalSizeF(qreal v,quint32 fractional);
quint32 fractionalSize(quint32 v,quint32 fractional);

//use this to get consistent bounding rectangles around a center point
QRectF realRectAroundRealPoint(const QPointF& centerPt,const QSizeF& rectSize);
QRectF realRectAroundRealPoint(const QSizeF& rectSize);		//assumes center is (0.0,0.0)
QRectF realRectAroundRealPoint(const QSize& rectSize);		//assumes center is (0.0,0.0)

//use this for "smaller squares into bigger square" class of problems
quint32 howManyFitHorizontally(const quint32 cellWidth,const quint32 areaWidth);
quint32 howManyFitHorizontally(const qreal cellWidthF,const qreal areaWidthF);
quint32 howManyFitHorizontally(const QSize& cellSize,const QSize& areaSize);
quint32 howManyFitHorizontally(const QSizeF& cellSizeF,const QSizeF& areaSizeF);

quint32 maxCellWidth(const quint32 howManyCells,const quint32 areaWidth);
quint32 maxCellWidth(const quint32 howManyCells,const qreal areaWidthF);
quint32 maxCellWidth(const quint32 howManyCells,const QSize& areaSize);
quint32 maxCellWidth(const quint32 howManyCells,const QSizeF& areaSizeF);

quint32 indentForCenteredDimensions(const quint32 outerDimension,const quint32 innerDimension);
qreal 	indentForCenteredDimensions(const qreal outerDimensionF,const qreal innerDimensionF);

//these don't actually have to be rects since it uses only the sizes and returns a point *offset* (not absolute point)
// from the outer rect's top left corner to the inner rect's top left corner in order to get them centered
// the fact that the functions take QRect and not QSize is a convenience
QPointF centerRectInRect(const QRectF& outer,const QRectF& inner);
QPoint	centerRectInRectPixelAlign(const QRectF& outer,const QRectF& inner);
QPoint	centerRectInRectPixelAlign(const QRect& outer,const QRect& inner);

//this one is needed for gestures, since they seem to be coming in within a "global" coordinate space.
// (Don't know why we haven't fixed this since it seems useless and error-prone for it to not be in at least
//	the scene cs)

QPointF mapFromGlobalCS(const QPointF& globalPointF);

//Access to the graphics scene...since this will be kind of fuzzy, depending on how the rest
// of sysmgr changes, i'm making a single access point to it here, so that the rest of Dimensions
// can just rely on it being correct, and there is only 1 place to modify when changes do occur.
// it CAN return NULL in cases when the scene isn't available (should be rare/hopefully never),
// so check for that in the caller and NEVER hold the reference for later
QGraphicsScene * globalGraphicsScene();

// FSM setup

QState * createFSMState(const QString& stateName,QState * parent = 0);

//VISUAL-HACKS:
typedef QVariantMap VHDescriptor;

}

int qHash(const QUuid&);

#endif /* DIMENSIONSGLOBAL_H_ */
