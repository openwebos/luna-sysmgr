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




#include "pixmap3vtileobject.h"
#include "dimensionsglobal.h"

#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QVector>

//public:

Pixmap3VTileObject::Pixmap3VTileObject()
{
}

Pixmap3VTileObject::Pixmap3VTileObject(const quint32 width,const quint32 height,
						const QString& imageFilename,
						const QVector<QRect>& sliceCoordinates,
            			PixmapVTilingStyle::Enum centerTilingStyle,
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
										(height == 0 ? pm->height() : height));
	createDestinationRectangles();
}

//this is the more friendly variant
Pixmap3VTileObject::Pixmap3VTileObject( const quint32 width, const quint32 height,
							const QString& imageFilename,
							const quint32 topIn,const quint32 bottomIn,
	            			PixmapVTilingStyle::Enum centerTilingStyle,
	            			const char * format, Qt::ImageConversionFlags flags)
: PixmapObject(imageFilename,format,flags)
, m_centerTilingStyle(centerTilingStyle)
{
	if (valid() == false)
	{
		return;		//base class failed to load pixmap
	}
	m_destinationSizeRequested = QSize((width == 0 ? pm->width() : width),
											(height == 0 ? pm->height() : height));

	makeSlices((quint32)(pm->width()),
				(quint32)(pm->height()),
				topIn,bottomIn,m_sourceRects,m_inCoords);

	createDestinationRectangles();
}

//virtual
Pixmap3VTileObject::~Pixmap3VTileObject()
{
}

//virtual
QSize Pixmap3VTileObject::size() const
{
	return m_geom.size().toSize();
}

//virtual
int Pixmap3VTileObject::width() const
{
	return m_geom.size().toSize().width();
}

//virtual
int Pixmap3VTileObject::height() const
{
	return m_geom.size().toSize().height();
}

//virtual
QSizeF Pixmap3VTileObject::sizeF() const
{
	return m_geom.size();
}

#include <QDebug>

//virtual
void Pixmap3VTileObject::paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
{
	//draw the pieces of the pixmap in the appropriate locations
	for (int i=0;i<3;++i)
	{
//		//qDebug() << "index " << i << ": srcRect: " <<m_sourceRects[i]<< " , destRect: " << m_destRects[i];
		painter->drawPixmap(m_destRects[i].translated(targetOriginInPainterCS),*pm,m_sourceRects[i]);
	}
}

//virtual
bool Pixmap3VTileObject::resize(const QSize& size)
{
	if (size == m_destinationSizeRequested)
	{
		return true;		//duplicate resize
	}
	return (resize(size.width(),size.height()));
}

//virtual
bool Pixmap3VTileObject::resize(const quint32 w,const quint32 h)
{
	if ((w == 0) || (h == 0))
	{
		return false;
	}
	m_destinationSizeRequested = QSize(w,h);
	makeSlices((quint32)(pm->width()),
						(quint32)(pm->height()),
						m_inCoords.top(),m_inCoords.bottom(),
						m_sourceRects,m_inCoords);
	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//virtual
bool Pixmap3VTileObject::retile(const quint32 topIn,const quint32 bottomIn,
				PixmapVTilingStyle::Enum sideTilingStyle,
				PixmapVTilingStyle::Enum centerTilingStyle)
{

	if (valid() == false)
	{
		return false;			//base pm is not valid, so dimensions will be impossible to get
	}

	makeSlices((quint32)(pm->width()),
					(quint32)(pm->height()),
					topIn,bottomIn,m_sourceRects,m_inCoords);

	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//virtual
bool Pixmap3VTileObject::retile(const QVector<QRect>& sliceCoordinates,
			PixmapVTilingStyle::Enum sideTilingStyle,
			PixmapVTilingStyle::Enum centerTilingStyle)
{
	if (sliceCoordinates.size() < 3)
	{
		return false;	//bad bad bad
	}

	m_sourceRects = sliceCoordinates;
	m_centerTilingStyle = centerTilingStyle;

	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//static
bool Pixmap3VTileObject::makeSlices(const quint32 width,const quint32 height,
		const quint32 topin,const quint32 bottomin,
        QVector<QRect>&   r_slices,
        QRect& r_inCoords)
{
    if (topin + bottomin > height)
    {
    	r_inCoords.setTop(0);
    	r_inCoords.setBottom(0);
    }
    else
    {
    	r_inCoords.setTop((int)topin);
    	r_inCoords.setBottom((int)bottomin);
    }

    //left and right not used
    r_inCoords.setLeft(0);
    r_inCoords.setRight(0);

    const quint32 centerHeight = height - (r_inCoords.top()+r_inCoords.bottom());

    r_slices = QVector<QRect>(3);

    r_slices[0] = QRect(0,0,
    		width,r_inCoords.top());
    r_slices[2] = QRect(0,height-r_inCoords.bottom(),
    		width,r_inCoords.bottom());
    r_slices[1] = QRect(0,r_inCoords.top(),
    		width,centerHeight);

    ////qDebug() << r_slices;
    return true;
}

//protected:

bool Pixmap3VTileObject::createDestinationRectangles()
{
	 m_geom = QRectF();
	 m_boundingRect = QRectF();
	 m_destRects = QVector<QRectF>(3);
    //the "caps" (top and bottom pieces) are the same width and height wise, in both the src and dest
    m_destRects[0] = QRectF(0,0,
             m_sourceRects[0].width(),m_sourceRects[0].height());
    m_destRects[2] = QRectF(0.0,m_destinationSizeRequested.height()-m_sourceRects[2].height(),
                m_sourceRects[2].width(),m_sourceRects[2].height());

    m_inCoords.setLeft(0);
    m_inCoords.setRight(0);
    m_inCoords.setTop(m_sourceRects[0].height());
    m_inCoords.setBottom(m_sourceRects[2].height());

    qreal centerHeight = m_destRects[2].y()-(m_destRects[0].y()+m_sourceRects[0].height());

    m_destRects[1] = QRectF(m_destRects[0].x(),m_destRects[0].y()+m_destRects[0].height(),
                m_destRects[0].width(),centerHeight);

    for (int i=0;i<3;++i)
    {
    	m_geom |= m_destRects[i];
    }
    m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
    return true;
}
