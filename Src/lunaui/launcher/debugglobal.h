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




#ifndef DEBUGGLOBAL_H_
#define DEBUGGLOBAL_H_

#include <QRect>
#include <QRectF>
#include <QPoint>
#include <QPointF>
#include <QDebug>
#include <QTouchEvent>
#include <QList>

class QPainter;
namespace DimensionsDebugGlobal
{

void dbgPaintBoundingRect(QPainter * painter,const QRectF& rect,qint32 color=0);
void dbgPaintBoundingRectWithTranslate(QPainter * painter,const QPointF& translatePainterTo,const QRectF& rect,qint32 color=0);

}

QDebug operator<<(QDebug dbg, const QTouchEvent::TouchPoint& s);
//QDebug operator<<(QDebug dbg, const QList<QTouchEvent::TouchPoint> * s);

QDebug myDebug();

#endif /* DEBUGGLOBAL_H_ */
