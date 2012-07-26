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



#include "pixmapobject.h"
#include <QPainter>
#include <QPointF>
#include <QTransform>

PixmapObject::PixmapObject()
: m_destroyEmitted(false)
{
	m_uid = QUuid::createUuid();
	pm = new QPixmap();
}

PixmapObject::PixmapObject( int width, int height)
: m_destroyEmitted(false)
{
	m_uid = QUuid::createUuid();
	pm = new QPixmap(width,height);
}

PixmapObject::PixmapObject( const QString & fileName, const char * format, Qt::ImageConversionFlags flags)
: m_destroyEmitted(false)
{
	m_uid = QUuid::createUuid();
	pm = new QPixmap(fileName,format,flags);
}

PixmapObject::PixmapObject ( const QString& fileName, const QSize& desiredSize, bool limitOnly, const char * format, Qt::ImageConversionFlags flags )
: m_destroyEmitted(false)
{
	m_uid = QUuid::createUuid();
	QImage img(fileName,format);
	if (img.isNull())
	{
		pm = new QPixmap();
		return;
	}

	QSize s(
			(limitOnly ? qMin(desiredSize.width(),img.width()) : desiredSize.width()),
			(limitOnly ? qMin(desiredSize.height(),img.height()) : desiredSize.height())
			);

	pm = new QPixmap(QPixmap::fromImage(img.scaled(s,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)));
}

PixmapObject::PixmapObject (QPixmap * p_pixmap)
: m_destroyEmitted(false)
{
	m_uid = QUuid::createUuid();
	pm = p_pixmap;
}

PixmapObject::PixmapObject(const QUuid& specificUid)
: pm(0)
, m_destroyEmitted(0)
{
	if (specificUid.isNull())
	{
		//can never allow an invalid one
		m_uid = QUuid::createUuid();
	}
	else
	{
		m_uid = specificUid;
	}
}

//TODO: fix the signalling of destroyed. This is too error-prone
//virtual
PixmapObject::~PixmapObject()
{
	//this signal is sensitive to where it is placed, since upstream receivers might access things in this object
	//due to this signal
	if (!m_destroyEmitted)
	{
		Q_EMIT signalObjectDestroyed();
	}
	if (pm)
	{
		delete pm;
	}

}

//virtual
bool PixmapObject::valid() const
{
	if (!pm)
	{
		return false;
	}
	if (pm->isNull())
	{
		return false;
	}
	return true;
}
//virtual
QUuid PixmapObject::id() const
{
	return m_uid;
}
//virtual
void PixmapObject::setId(const QUuid& uid)
{
	m_uid = uid;
}
//virtual
void PixmapObject::setId(const QString& quuid_as_string)
{
	QUuid tst(quuid_as_string);
	if (tst.isNull())
		return;			//no change, invalid
	m_uid = tst;
}

//virtual
bool PixmapObject::resize(quint32 width,quint32 height)
{
	return false;
}

//virtual
bool PixmapObject::resize(const QSize& size)
{
	return false;
}

//convenience passthrough for QPixmap::size()
QSize PixmapObject::size() const
{
	return pm->size();
}

//virtual
int PixmapObject::width() const
{
	return pm->width();
}

//virtual
int PixmapObject::height() const
{
	return pm->height();
}

//virtual
QSizeF PixmapObject::sizeF() const
{
	return QSizeF(pm->size());
}

//virtual
QSize PixmapObject::nativeSize() const
{
	return pm->size();
}
//virtual
int PixmapObject::nativeWidth() const
{
	return pm->width();
}
//virtual
int PixmapObject::nativeHeight() const
{
	return pm->height();
}

//virtual
QSizeF PixmapObject::nativeSizeF() const
{
	return QSizeF(pm->size());
}

//virtual
////// both sizeof functions should return the same value for the same pixmap
quint64 PixmapObject::sizeOf() const
{
	if (!pm)
		return 0;
	return (pm->size().height()*pm->size().width()*pm->depth());
}
//virtual
bool PixmapObject::isSquare() const
{
	return (pm->size().height() == pm->size().width());
}

//virtual
void PixmapObject::fill(const QColor& c)
{
	if (pm)
	{
		pm->fill(c);
	}
}

//virtual
void PixmapObject::paint(QPainter * painter)
{
	painter->drawPixmap(0,0,*pm);
}

//virtual
void PixmapObject::paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
{
	painter->drawPixmap(targetOriginInPainterCS,*pm);
}

//virtual
void PixmapObject::paint(QPainter * painter,const QRectF& targetRectInPainterCS)
{
	painter->drawPixmap(targetRectInPainterCS,*pm,pm->rect());
}

//virtual
void PixmapObject::paint(QPainter * painter,const QRect& targetRectInPainterCS,
										  const QRect& sourceRect)
{
	painter->drawPixmap(targetRectInPainterCS,*pm,sourceRect);
}

//virtual
QPoint PixmapObject::translatePaintTargetPointToPixmapPoint(const QPoint& point,const QRect& sourceRect,const QRect& destRect)
{
	QTransform tran = QTransform().scale( ((qreal)sourceRect.width()) / (qreal)(destRect.width()) , ((qreal)sourceRect.height()) / (qreal)(destRect.height()))
									.translate( (qreal)(destRect.center().x() - sourceRect.center().x()) , (qreal)(destRect.center().y() - sourceRect.center().y()) );

	return tran.map(point);
}

//virtual
QVector<qint32> PixmapObject::translatePaintTargetPointToPixmapPointEx(const QPoint& point,const QRect& sourceRect,const QRect& destRect)
{
	QTransform tran = QTransform().scale( ((qreal)sourceRect.width()) / (qreal)(destRect.width()) , ((qreal)sourceRect.height()) / (qreal)(destRect.height()))
										.translate( (qreal)(destRect.center().x() - sourceRect.center().x()) , (qreal)(destRect.center().y() - sourceRect.center().y()) );

	QPoint translatedPt = tran.map(point);
	return (QVector<qint32>() << translatedPt.x() << translatedPt.y());
}


//static
quint64 PixmapObject::sizeOfPixmap(const QPixmap * p_pixmap)
{
	if (!p_pixmap)
		return 0;
	return (p_pixmap->size().height()*p_pixmap->size().width()*p_pixmap->depth());
}

//static
quint64 PixmapObject::sizeOfPixmap(quint32 width,quint32 height)
{
	return (width*height*4);			//current used bitdepth = 32 bits (4b)
}
