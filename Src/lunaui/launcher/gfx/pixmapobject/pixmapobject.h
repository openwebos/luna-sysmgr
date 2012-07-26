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




#ifndef PIXMAPOBJECT_H_
#define PIXMAPOBJECT_H_

#include <QObject>
#include <QPixmap>
#include <QUuid>
#include <QSize>

class QPainter;
class QPointF;
class PixPager;
class PixPagerPage;
class PixPagerAtlasPage;
class VirtualCamera;

/*
 * Not a true guarded pointer, nor intended to be. There is no way to guarantee with this impl that QPixmap wrapped herein
 * cannot be destroyed by something external. That isn't the purpose though. PixmapObject is just to tag the QPixmap with an id
 * and to be used like a QPixmap, including destroying *it* directly. This way, the proper signaling can take place for things
 * like the PixPager
 *
 */
class PixmapObject : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString id READ id WRITE setId)

public:

	friend  class VirtualCamera;
	friend  class PixmapObjectLoader;
	PixmapObject();
	PixmapObject ( int width, int height );
	PixmapObject ( const QString & fileName, const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor );
	PixmapObject ( const QString& fileName, const QSize& desiredSize, bool limitOnly = true, const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor );

	//TODO: IMPLEMENT
	virtual PixmapObject * clone() { return 0; }

	virtual ~PixmapObject();

	virtual bool valid() const;

	virtual QUuid id() const;

	virtual void setId(const QUuid& uid);
	virtual void setId(const QString& quuid_as_string);

	//for resizable target size pixmaps (like the tiled variety). Ignored in other case
	// (returns true or false for resized or not)
	virtual bool resize(const quint32 w,const quint32 h);
	virtual bool resize(const QSize& size);

	//TODO: create "rendered___"() versions of some of these...e.g. renderedSize(), renderedWidth()...
	//	it is needed to make subclasses like the tileobject more friendly to use and less ambiguous
	virtual QSize size() const;
	virtual int width() const;
	virtual int height() const;
	virtual QSizeF sizeF() const;
	virtual quint64 sizeOf() const;
	virtual bool isSquare() const;

	virtual QSize nativeSize() const;
	virtual int nativeWidth() const;
	virtual int nativeHeight() const;
	virtual QSizeF nativeSizeF() const;

	virtual void fill(const QColor& c);
	virtual void paint(QPainter * painter);
	virtual void paint(QPainter * painter,const QPointF& targetOriginInPainterCS);
	virtual void paint(QPainter * painter,const QRectF& targetRectInPainterCS);

	/*
	 *
	 * sourceRect:
	 * 		The rectangle source in either PHYSICAL or VIRTUAL space of this PMO!
	 * 		This is an important point...this rect will be specified in the space of (0,0)->size()
	 * 		In the case of the base PixmapObject, this will be 1:1 with the actual pixmap contained. e.g. if the
	 * 		pixmap stored here is 200x300, then the space will be (0,0)-(200,300)
	 * 		However, in the case of complex subclasses which define a virtual space - i.e. their painted sizes are larger
	 * 		then the pixmap size contained and/or they stitch a large picture from many smaller pixmaps - the sourceRect
	 * 		space will be relative to this virtual picture. IT IS VITAL that size(),sizeF(), width(), and height() return
	 * 		sane values, for this reason
	 *
	 */
	virtual void paint(QPainter * painter,const QRect& targetRectInPainterCS,
										  const QRect& sourceRect);

	virtual QPoint translatePaintTargetPointToPixmapPoint(const QPoint& point,const QRect& sourceRect,const QRect& destRect);
	//this variant is needed for pixmapobject variants that need to express the return coordinates with more than just a QPoint
	// 	(e.g. hugepixmap, which will return x,y,index of
	virtual QVector<qint32> translatePaintTargetPointToPixmapPointEx(const QPoint& point,const QRect& sourceRect,const QRect& destRect);

	static quint64 sizeOfPixmap(const QPixmap * p_pixmap);
	//how much would a pixmap of w x h x whatever_bitdepth_we_use
	static quint64 sizeOfPixmap(quint32 width,quint32 height);

	virtual QPixmap* operator->() const
	{ return static_cast<QPixmap*>(const_cast<QPixmap*>(pm)); }
	virtual QPixmap& operator*() const
	{ return *static_cast<QPixmap*>(const_cast<QPixmap*>(pm)); }
	virtual operator QPixmap*() const
	{ return static_cast<QPixmap*>(const_cast<QPixmap*>(pm)); }
	virtual QPixmap* data() const
	{ return static_cast<QPixmap*>(const_cast<QPixmap*>(pm)); }

	friend class PixPager;
	friend class PixPagerPage;
	friend class PixPagerAtlasPage;

Q_SIGNALS:
	void signalObjectDestroyed();

protected:

	//DANGEROUS VARIANT! Assumes ownership of pixmap; must never be a pixmap on the stack!
	PixmapObject (QPixmap * p_pixmap);

	//this is to be used only in very specific circumstances; PixmapJUPOCObject and PixmapJUPOCRefObject use this to transfer the Uid key of a map to the actual object
	PixmapObject(const QUuid& specificUid);

	QUuid m_uid;
	QPixmap * pm;
	bool m_destroyEmitted;		//a workaround for destructors in base classes
									//emitting PixmapObject::signalObjectDestroyed
};

#endif /* PIXMAPOBJECT_H_ */
