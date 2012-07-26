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




#ifndef PIXMAP9TILEOBJECT_H_
#define PIXMAP9TILEOBJECT_H_

#include "pixmapobject.h"
#include <QRectF>
#include <QObject>

class QPainter;

namespace PixmapTilingStyle
{
	enum Enum
	{
		Scale,
		Repeat
	};
}

class Pixmap9TileObject : public PixmapObject
{
	Q_OBJECT

public:

	Pixmap9TileObject();
	Pixmap9TileObject(const quint32 width, const quint32 height,
						const QString& imageFilename,
						const QVector<QRect>& sliceCoordinates,
            			PixmapTilingStyle::Enum sideTilingStyle = PixmapTilingStyle::Scale,
            			PixmapTilingStyle::Enum centerTilingStyle = PixmapTilingStyle::Scale,
            			const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);

	//this is the more friendly variant
	Pixmap9TileObject( const quint32 width, const quint32 height,
							const QString& imageFilename,
							const quint32 topIn,const quint32 bottomIn,
							const quint32 leftIn,const quint32 rightIn,
	            			PixmapTilingStyle::Enum sideTilingStyle = PixmapTilingStyle::Scale,
	            			PixmapTilingStyle::Enum centerTilingStyle = PixmapTilingStyle::Scale,
	            			const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);

	virtual ~Pixmap9TileObject();

	virtual QSize size() const;
	virtual int width() const;
	virtual int height() const;
	virtual QSizeF sizeF() const;

	//isSquare() from PixmapObject intentionally omitted because it's used for determining operations on the underlying pixmap,
	//	not the rendered version of the pixmaptileobject
	//sizeOf() from PixmapObject intentionally omitted, since the underlying mem size of the pixmap is not affected by
	//			how the tiles are defined

	//WARNING: no guard on validity on undelying pixmap! check in caller if unsure
	virtual void paint(QPainter * painter,const QPointF& targetOriginInPainterCS);

	virtual bool resize(const QSize& size);
	virtual bool resize(const quint32 w,const quint32 h);

	virtual bool retile(const quint32 topIn,const quint32 bottomIn,
						const quint32 leftIn,const quint32 rightIn,
				PixmapTilingStyle::Enum sideTilingStyle = PixmapTilingStyle::Scale,
				PixmapTilingStyle::Enum centerTilingStyle = PixmapTilingStyle::Scale);

	virtual bool retile(const QVector<QRect>& sliceCoordinates,
			PixmapTilingStyle::Enum sideTilingStyle = PixmapTilingStyle::Scale,
			PixmapTilingStyle::Enum centerTilingStyle = PixmapTilingStyle::Scale);

	static bool makeSlices(const quint32 width, const quint32 height,
			const quint32 topIn,const quint32 bottomIn,
			const quint32 leftIn,const quint32 rightIn,
			QVector<QRect>&   r_slices,
			QRect& r_inCoords);

	friend class PixPager;
	friend class PixPagerPage;
	friend class PixPagerAtlasPage;

protected:

	bool createDestinationRectangles();
	QSize	m_destinationSizeRequested;		//how large this needs to paint itself.
											//geom and boundingrect are computed from this
	QRect	m_inCoords;				//using QRect as a convenient way to store topIn,leftIn..etc passed in
									//this is used by resize() so it can reuse the original "in" coords
									//
	QRectF  m_geom;				//calculated automatically from the slice coordinate setup
	QRectF  m_boundingRect;		//derived from m_geom

	 /*
	   [0][1][2]
	   [3][4][5]
	   [6][7][8]
	 */

	QVector<QRect>  m_sourceRects;
	QVector<QRectF>  m_destRects;

	PixmapTilingStyle::Enum m_sideTilingStyle;
	PixmapTilingStyle::Enum m_centerTilingStyle;

};

#endif /* PIXMAP9TILEOBJECT_H_ */
