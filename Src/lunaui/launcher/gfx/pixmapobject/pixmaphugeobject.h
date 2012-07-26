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




#ifndef PIXMAPHUGEOBJECT_H_
#define PIXMAPHUGEOBJECT_H_

#include "pixmapobject.h"
#include <QVector>
#include <QList>
#include <QRect>
#include <QDebug>

class VirtualCamera;
class DemoGoggles;
class PixmapHugeObject : public PixmapObject
{
	Q_OBJECT

public:

	friend class VirtualCamera;
	friend class DemoGoggles;
	class FragmentedPaintCoordinate
	{
	public:
		FragmentedPaintCoordinate()
		: pixmapIndex(0)
		{
		}
		FragmentedPaintCoordinate(int index,const QRect& src,const QRect& dst)
		: pixmapIndex(index)
		, sourceRect(src)
		, targetRect(dst)
		{
		}
		//which "tile" in the pixmap list
		int pixmapIndex;
		//the coordinates of the rectangle to paint from in the absolute coordinate space
		QRect sourceRect;
		//coordinates in the painter cs of the pixmap
		QRect targetRect;

		friend QDebug operator<<(QDebug dbg, const FragmentedPaintCoordinate &c);
	};

	PixmapHugeObject();
	PixmapHugeObject ( int width, int height );
	PixmapHugeObject ( const QString & fileName, const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor );
	PixmapHugeObject ( const QList<QString>& fileNames, const QSize& fileLayout,const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor );
	//this one copies the QPixmaps in the list, so they can be anything in the caller
	PixmapHugeObject ( const QList<QPixmap>& pixmaps, const QSize& layout);

	virtual ~PixmapHugeObject();

	virtual bool valid() const;

	virtual QSize size() const;
	virtual int width() const;
	virtual int height() const;
	virtual QSizeF sizeF() const;
	virtual quint64 sizeOf() const;
	virtual bool isSquare() const;

	/*
	 * The paint() functions are OUTPUT functions...they paint the contents of this "huge pixmap" onto whatever painter is passed in
	 *
	 * For INPUT functions (i.e. painting ONTO this huge pixmap), each item that wants to be able to render offscreen to a PixmapHugeObject will
	 *	have to implement a paint(PixmapHugeObject * p,...).  Ideally, this would be done by making a PixmapHuge class derived from QPixmap, and a PixmapHugeEngine
	 *	derived from QPaintEngine, and then wrapping PixmapHugeObject around it just as PixmapObject wraps over QPixmap
	 *
	 *	...however I don't have time to do this at the moment so this is a shortcut approach
	 */

	virtual void paint(QPainter * painter);
	virtual void paint(QPainter * painter,const QPointF& targetOriginInPainterCS);
	virtual void paint(QPainter * painter,const QRectF& targetRectInPainterCS);
	virtual void paint(QPainter * painter,const QRect& targetRectInPainterCS,
											  const QRect& sourceRect);

	virtual QPoint translatePaintTargetPointToPixmapPoint(const QPoint& point,const QRect& sourceRect,const QRect& destRect);
	// vec[0] = point.x in hugespace , [1] = point.y in hugespace , [2] = point.x on local pixmap , [3] = point.y on local pixmap , [4] = index of local pixmap
	virtual QVector<qint32> translatePaintTargetPointToPixmapPointEx(const QPoint& point,const QRect& sourceRect,const QRect& destRect);

	//these return a 3-vector: [0] = pixmap index (in m_pixmaps vector), [1],[2] x,y coords in painter CS inside that pixmap
	QVector<int> hugeSpaceCoordinatesOfPoint(const QPoint& pointInAbsolutePainterCS);
	QVector<int> hugeSpaceCoordinatesOfPoint(const int x,const int y);
	QVector<FragmentedPaintCoordinate> paintCoordinates(const QRect& rectInAbsolutePainterCS);

	virtual QPixmap * pixAt(const quint32 index) const;

	virtual QPixmap* operator->() const;
	virtual QPixmap& operator*() const;
	virtual operator QPixmap*() const;
	virtual QPixmap* data() const;

	friend class PixPager;
	friend class PixPagerPage;
	friend class PixPagerAtlasPage;

	static QSize pixmapToArraySize(const QSize& pixmapSize,const QSize& maxSinglePixmapSize);
	//this variant returns the size of the "edge pixmaps" (on the right and bottom edges) since these
	// are the "leftover" size of what wouldn't fit into a whole maxSinglePixmapSize
	static QSize pixmapToArraySize(const QSize& pixmapSize,const QSize& maxSinglePixmapSize,QSize& r_rightAndBottomEdgeSizes);

	void dbg_save(const QString& baseName);

	friend QDebug operator<<(QDebug dbg, const PixmapHugeObject& c);
protected:

	//DANGEROUS VARIANT! Assumes ownership of pixmaps; must never be pixmaps on the stack!
	PixmapHugeObject (const QVector<QPixmap *> pixmaps,const QSize& layout);

	virtual void allocatePixmaps(const QSize& arraySize,const QSize& normalSize,const QSize& leftoverSize);
	virtual void copyToPixmaps(const QImage& sourceImage);

	QSize m_arraySize;			//array config..e.g. [3][4] (3 by 4)...QSize.w = columns, QSize.h = rows
	QVector<QRect> m_pixmapRectCoords;	//(precomputed divisions from 0,0->m_hugePixmapSize to make paint() faster)
	QSize m_hugePixmapSize;		//the total size of the pixmap if you stitched together all the pixmaps
	QVector<QPixmap *> m_pixmaps;	//using QVector for the property of having pixmap ptrs adjacent in memory
	bool m_valid;				//(precomputed validity to make valid() faster)

};

QDebug operator<<(QDebug dbg, const PixmapHugeObject& c);
QDebug operator<<(QDebug dbg, const PixmapHugeObject::FragmentedPaintCoordinate &c);

#endif /* PIXMAPHUGEOBJECT_H_ */
