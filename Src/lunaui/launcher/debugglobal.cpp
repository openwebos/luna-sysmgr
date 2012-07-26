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




#include "debugglobal.h"
#include <QtGlobal>
#include <QPainter>
#include <QTransform>
#include <QFile>
#include <QTouchEvent>

void DimensionsDebugGlobal::dbgPaintBoundingRect(QPainter * painter,const QRectF& rect,qint32 color)
{
	qint32 pens[] =
	{
			Qt::black,	//0
			Qt::white,
			Qt::yellow,
			Qt::red,
			Qt::green,
			Qt::blue,	//5
			Qt::cyan,
			Qt::magenta,
			Qt::darkGray,
			Qt::gray,
			Qt::lightGray,  //10
			Qt::darkRed,
			Qt::darkGreen,
			Qt::darkBlue,
			Qt::darkCyan,
			Qt::darkMagenta, //15
			Qt::darkYellow,
	};

	if (!painter)
	{
		//qDebug() << "__PRETTY_FUNCTION__" << ": null painter passed in";
		return;
	}
	QPen spen = painter->pen();
	qint32 sz = sizeof(pens)/sizeof(qint32);
	qint32 idx = (color < 0 ? qrand() % sz : color % sz);
	painter->setPen((Qt::GlobalColor)(pens[idx]));
	painter->drawRect(rect);
	painter->setPen(spen);
}

void DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(QPainter * painter,const QPointF& translatePainterTo,const QRectF& rect,qint32 color)
{
	QTransform saveTran = painter->transform();
	painter->translate(translatePainterTo);
	dbgPaintBoundingRect(painter,rect,color);
	painter->setTransform(saveTran);
}

QString touchStateToString(Qt::TouchPointState state)
{
	QString s;
	s.append( (state & Qt::TouchPointPressed) ? " pressed" : "");
	s.append( (state & Qt::TouchPointMoved) ? " moved" : "");
	s.append( (state & Qt::TouchPointStationary) ? " stationary" : "");
	return s;
}

QDebug operator<<(QDebug dbg, const QTouchEvent::TouchPoint &s)
{
	dbg.nospace() << "\"TouchPoint\":"
			<< "{ \"id\":" << s.id()
			<< ", \"primary\":\"" << (s.isPrimary() ? "true" : "false") << "\""
			<< ", \"state\":" << touchStateToString(s.state())
			<< ", \"pos\":\"" << s.pos() << "\""
			<< ", \"pressure\":\"" << s.pressure() << "\""
			<< " }";
	return dbg.space();
}

//QDebug operator<<(QDebug dbg, const QList<QTouchEvent::TouchPoint> * s)
//{
//	return dbg.space();
//}
/*
static QFile nulf("/dev/null");

static QDebug nullDebug()
{
	nulf.open(QIODevice::ReadWrite);
	return QDebug(&nulf);
}

static QDebug noDbg = nullDebug();

QDebug myDebug()
{
#ifdef DEBUG_DIUI
	return //qDebug();
#else
	return noDbg;
#endif
}
*/
