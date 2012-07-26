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




#include "pixmap3htileobject.h"
#include "dimensionsglobal.h"

#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QVector>

//public:

Pixmap3HTileObject::Pixmap3HTileObject()
{
}

Pixmap3HTileObject::Pixmap3HTileObject(const quint32 width,const quint32 height,
						const QString& imageFilename,
            			const QVector<QRect>& sliceCoordinates,
            			PixmapHTilingStyle::Enum centerTilingStyle,
            			const char * format, Qt::ImageConversionFlags flags)
: PixmapObject(imageFilename,format,flags)
, m_sourceRects(sliceCoordinates)
, m_centerTilingStyle(centerTilingStyle)
{

	if (valid() == false)
	{
		return;		//base class failed to load pixmap
	}
	if (m_sourceRects.size() < 3)
	{
		//Yes,yes, violates OOP...
		if (pm)
		{
			delete pm;
			pm = 0;
		}
		return;	//bad bad bad
	}

	m_destinationSizeRequested = QSize((width == 0 ? pm->width() : width),
										pm->height());
	createDestinationRectangles();
}

//this is the more friendly variant
Pixmap3HTileObject::Pixmap3HTileObject( const quint32 width, const quint32 height,
							const QString& imageFilename,
							const quint32 leftIn,const quint32 rightIn,
	            			PixmapHTilingStyle::Enum centerTilingStyle,
	            			const char * format, Qt::ImageConversionFlags flags)
: PixmapObject(imageFilename,format,flags)
, m_centerTilingStyle(centerTilingStyle)
{
	if (valid() == false)
	{
		return;		//base class failed to load pixmap
	}
	m_destinationSizeRequested = QSize((width == 0 ? pm->width() : width),
										pm->height());

	makeSlices((quint32)(pm->width()),
				(quint32)(pm->height()),
				leftIn,rightIn,m_sourceRects,m_inCoords);

	createDestinationRectangles();
}

//virtual
Pixmap3HTileObject::~Pixmap3HTileObject()
{
}

//virtual
QSize Pixmap3HTileObject::size() const
{
	return m_geom.size().toSize();
}

//virtual
int Pixmap3HTileObject::width() const
{
	return m_geom.size().toSize().width();
}

//virtual
int Pixmap3HTileObject::height() const
{
	return m_geom.size().toSize().height();
}

//virtual
QSizeF Pixmap3HTileObject::sizeF() const
{
	return m_geom.size();
}

#include <QDebug>

//virtual
void Pixmap3HTileObject::paint(QPainter * painter)
{
	//draw the pieces of the pixmap in the appropriate locations
	for (int i=0;i<3;++i)
	{
		painter->drawPixmap(m_destRects[i],*pm,m_sourceRects[i]);
	}
}

//virtual
void Pixmap3HTileObject::paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
{
	//draw the pieces of the pixmap in the appropriate locations
	for (int i=0;i<3;++i)
	{
//		//qDebug() << "index " << i << ": srcRect: " <<m_sourceRects[i]<< " , destRect: " << m_destRects[i];
		painter->drawPixmap(m_destRects[i].translated(targetOriginInPainterCS),*pm,m_sourceRects[i]);
	}
}

//virtual
void Pixmap3HTileObject::paint(QPainter * painter,const QRectF& targetRectInPainterCS)
{
	//TODO: fix this! it currently just does the same thing as paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
	// 	what it needs to do is map the target rect into the "tile space" of this tiled pmo, and determine which pieces of the tiles
	//	to paint where in the target rect space.
	// This *could* violate the principle of tiled pmo-s, in that I could potentially have targetRect defined in such a way that
	// it would scale up the resulting tiled pmo, resulting in distortion that I intended to avoid in the first place by
	// making tiled pmos! In those cases, it is better to just retile() this tiled pmo to the new size (target size) desired
	for (int i=0;i<3;++i)
	{
		painter->drawPixmap(m_destRects[i].translated(targetRectInPainterCS.topLeft()),*pm,m_sourceRects[i]);
	}
}

//virtual
void Pixmap3HTileObject::paint(QPainter * painter,const QRect& targetRectInPainterCS,
											  const QRect& sourceRect)
{
	//TODO: unimplemented / IMPLEMENT

	//WARNING: if unimplemented, prevents Pixmap3HTileObject from working with PixmapHugeObject as an output target
}


//virtual
bool Pixmap3HTileObject::resize(const QSize& size)
{
	if (size == m_destinationSizeRequested)
	{
		return true;		//duplicate resize
	}
	return (resize(size.width(),size.height()));
}

//virtual
bool Pixmap3HTileObject::resize(const quint32 w,const quint32 h)
{
	if ((w == 0) || (h == 0))
	{
		return false;
	}
	m_destinationSizeRequested = QSize(w,height());
	makeSlices((quint32)(pm->width()),
						(quint32)(pm->height()),
						m_inCoords.left(),m_inCoords.right(),
						m_sourceRects,m_inCoords);
	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//virtual
bool Pixmap3HTileObject::retile(const quint32 leftIn,const quint32 rightIn,
				PixmapHTilingStyle::Enum centerTilingStyle,
				PixmapHTilingStyle::Enum sideTilingStyle)
{

	if (valid() == false)
	{
		return false;			//base pm is not valid, so dimensions will be impossible to get
	}

	makeSlices((quint32)(pm->width()),
					(quint32)(pm->height()),
					leftIn,rightIn,m_sourceRects,m_inCoords);

	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//virtual
bool Pixmap3HTileObject::retile(const QVector<QRect>& sliceCoordinates,
						PixmapHTilingStyle::Enum centerTilingStyle,
						PixmapHTilingStyle::Enum sideTilingStyle)
{
	if (sliceCoordinates.size() < 3)
	{
		return false;	//bad bad bad
	}

	m_sourceRects = sliceCoordinates;
	m_sideTilingStyle = sideTilingStyle;
	m_centerTilingStyle = centerTilingStyle;

	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//static
bool Pixmap3HTileObject::makeSlices(const quint32 width,const quint32 height,
		const quint32 leftin,const quint32 rightin,
        QVector<QRect>&   r_slices,
        QRect& r_inCoords)
{
    if (leftin + rightin > width)
    {
    	r_inCoords.setLeft(0);
    	r_inCoords.setRight(0);
    }
    else
    {
    	r_inCoords.setLeft((int)leftin);
    	r_inCoords.setRight((int)rightin);
    }

    //top and bottom not used
    r_inCoords.setBottom(0);
    r_inCoords.setTop(0);

    const quint32 centerWidth = width - (r_inCoords.left()+r_inCoords.right());

    r_slices = QVector<QRect>(3);

    r_slices[0] = QRect(0,0,
    		r_inCoords.left(),height);
    r_slices[2] = QRect(width-r_inCoords.right(),0,
    		r_inCoords.right(),height);
    r_slices[1] = QRect(r_inCoords.left(),0,
              centerWidth,height);

    ////qDebug() << r_slices;
    return true;
}

//protected:

bool Pixmap3HTileObject::createDestinationRectangles()
{
	 m_geom = QRectF();
	 m_boundingRect = QRectF();
	 m_destRects = QVector<QRectF>(3);

    m_destRects[0] = QRectF(0,0,
             m_sourceRects[0].width(),m_sourceRects[0].height());
    m_destRects[2] = QRectF(m_destinationSizeRequested.width()-m_sourceRects[2].width(),0.0,
                m_sourceRects[2].width(),m_sourceRects[2].height());

    m_inCoords.setLeft(m_sourceRects[0].width());
    m_inCoords.setRight(m_sourceRects[2].width());
    m_inCoords.setTop(0);
    m_inCoords.setBottom(0);

    qreal centerWidth = m_destRects[2].x()-(m_destRects[0].x()+m_destRects[0].width());

    //the top/bottom edges
    m_destRects[1] = QRectF(m_destRects[0].x()+m_destRects[0].width(),
                m_destRects[0].y(),
                centerWidth,m_destRects[0].height());

    for (int i=0;i<3;++i)
    {
    	m_geom |= m_destRects[i];
    }
    m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
    return true;
}
