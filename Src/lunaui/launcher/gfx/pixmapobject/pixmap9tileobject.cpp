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




#include "pixmap9tileobject.h"
#include "dimensionsglobal.h"

#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QVector>

//public:

Pixmap9TileObject::Pixmap9TileObject()
{
}

Pixmap9TileObject::Pixmap9TileObject(const quint32 width,const quint32 height,
						const QString& imageFilename,
            			const QVector<QRect>& sliceCoordinates,
            			PixmapTilingStyle::Enum sideTilingStyle,
            			PixmapTilingStyle::Enum centerTilingStyle,
            			const char * format, Qt::ImageConversionFlags flags)
: PixmapObject(imageFilename,format,flags)
, m_sourceRects(sliceCoordinates)
, m_sideTilingStyle(sideTilingStyle)
, m_centerTilingStyle(centerTilingStyle)
{

	if (valid() == false)
	{
		return;		//base class failed to load pixmap
	}
	if (m_sourceRects.size() < 9)
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
Pixmap9TileObject::Pixmap9TileObject( const quint32 width, const quint32 height,
							const QString& imageFilename,
							const quint32 topIn,const quint32 bottomIn,
							const quint32 leftIn,const quint32 rightIn,
	            			PixmapTilingStyle::Enum sideTilingStyle,
	            			PixmapTilingStyle::Enum centerTilingStyle,
	            			const char * format, Qt::ImageConversionFlags flags)
: PixmapObject(imageFilename,format,flags)
, m_sideTilingStyle(sideTilingStyle)
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
				topIn,bottomIn,leftIn,rightIn,m_sourceRects,m_inCoords);

	createDestinationRectangles();
}

//virtual
Pixmap9TileObject::~Pixmap9TileObject()
{
}

//virtual
QSize Pixmap9TileObject::size() const
{
	return m_geom.size().toSize();
}

//virtual
int Pixmap9TileObject::width() const
{
	return m_geom.size().toSize().width();
}

//virtual
int Pixmap9TileObject::height() const
{
	return m_geom.size().toSize().height();
}

//virtual
QSizeF Pixmap9TileObject::sizeF() const
{
	return m_geom.size();
}

#include <QDebug>

//virtual
void Pixmap9TileObject::paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
{
	//draw the pieces of the pixmap in the appropriate locations
	for (int i=0;i<9;++i)
	{
//		//qDebug() << "index " << i << ": srcRect: " <<m_sourceRects[i]<< " , destRect: " << m_destRects[i];
		painter->drawPixmap(m_destRects[i].translated(targetOriginInPainterCS),*pm,m_sourceRects[i]);
	}
}

//virtual
bool Pixmap9TileObject::resize(const QSize& size)
{
	if (size == m_destinationSizeRequested)
	{
		return true;		//duplicate resize
	}
	return (resize(size.width(),size.height()));
}

//virtual
bool Pixmap9TileObject::resize(const quint32 w,const quint32 h)
{
	if ((w == 0) || (h == 0))
	{
		return false;
	}
	m_destinationSizeRequested = QSize(w,h);
	makeSlices((quint32)(pm->width()),
						(quint32)(pm->height()),
						m_inCoords.top(),m_inCoords.bottom(),
						m_inCoords.left(),m_inCoords.right(),
						m_sourceRects,m_inCoords);
	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//virtual
bool Pixmap9TileObject::retile(const quint32 topIn,const quint32 bottomIn,
						const quint32 leftIn,const quint32 rightIn,
				PixmapTilingStyle::Enum sideTilingStyle,
				PixmapTilingStyle::Enum centerTilingStyle)
{

	if (valid() == false)
	{
		return false;			//base pm is not valid, so dimensions will be impossible to get
	}

	makeSlices((quint32)(pm->width()),
					(quint32)(pm->height()),
					topIn,bottomIn,leftIn,rightIn,m_sourceRects,m_inCoords);

	//TODO: in theory, should trap a false return and nuke the pm (invalidate the pmo)
	return createDestinationRectangles();
}

//virtual
bool Pixmap9TileObject::retile(const QVector<QRect>& sliceCoordinates,
			PixmapTilingStyle::Enum sideTilingStyle,
			PixmapTilingStyle::Enum centerTilingStyle)
{
	if (sliceCoordinates.size() < 9)
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
bool Pixmap9TileObject::makeSlices(const quint32 width,const quint32 height,
		const quint32 topin,const quint32 bottomin,
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

    const quint32 centerWidth = width - (r_inCoords.left()+r_inCoords.right());
    const quint32 centerHeight = height - (r_inCoords.top()+r_inCoords.bottom());

    r_slices = QVector<QRect>(9);

    r_slices[0] = QRect(0,0,
    		r_inCoords.left(),r_inCoords.top());
    r_slices[2] = QRect(width-r_inCoords.right(),0,
    		r_inCoords.right(),r_inCoords.top());
    r_slices[1] = QRect(r_inCoords.left(),0,
              centerWidth,r_inCoords.top());
    r_slices[6] = QRect(0,height-r_inCoords.bottom(),
    		r_inCoords.left(),r_inCoords.bottom());
    r_slices[8] = QRect(width-r_inCoords.right(),height-r_inCoords.bottom(),
    		r_inCoords.right(),r_inCoords.bottom());
    r_slices[7] = QRect(r_inCoords.left(),height-r_inCoords.bottom(),
              centerWidth,r_inCoords.bottom());
    r_slices[3] = QRect(0,r_inCoords.top(),
    		r_inCoords.left(),centerHeight);
    r_slices[5] = QRect(width-r_inCoords.right(),r_inCoords.top(),
    		r_inCoords.right(),centerHeight);
    r_slices[4] = QRect(r_inCoords.left(),r_inCoords.top(),
              centerWidth,centerHeight);

    ////qDebug() << r_slices;
    return true;
}

//protected:

bool Pixmap9TileObject::createDestinationRectangles()
{
	 m_geom = QRectF();
	 m_boundingRect = QRectF();
	 m_destRects = QVector<QRectF>(9);
    //the corners are the same width and height wise, in both the src and dest
    m_destRects[0] = QRectF(0,0,
             m_sourceRects[0].width(),m_sourceRects[0].height());
    m_destRects[2] = QRectF(m_destinationSizeRequested.width()-m_sourceRects[2].width(),0.0,
                m_sourceRects[2].width(),m_sourceRects[2].height());
    m_destRects[6] = QRectF(0.0,m_destinationSizeRequested.height()-m_sourceRects[6].height(),
                m_sourceRects[6].width(),m_sourceRects[6].height());
    m_destRects[8] = QRectF(m_destinationSizeRequested.width()-m_sourceRects[8].width(),
    			m_destinationSizeRequested.height()-m_sourceRects[8].height(),
                m_sourceRects[8].width(),m_sourceRects[8].height());

    m_inCoords.setLeft(m_sourceRects[0].width());
    m_inCoords.setRight(m_sourceRects[2].width());
    m_inCoords.setTop(m_sourceRects[0].height());
    m_inCoords.setBottom(m_sourceRects[6].height());

    qreal centerWidth = m_destRects[2].x()-(m_destRects[0].x()+m_destRects[0].width());
    qreal centerHeight = m_destRects[6].y()-(m_destRects[0].y()+m_sourceRects[0].height());

    //validity check: the bottom [8]-[6] must match up
    if (m_destRects[8].x()-(m_destRects[6].x()+m_destRects[6].width()) != centerWidth)
    return false;
    //...and the side [8]-[2] must match up
    if (m_destRects[8].y()-(m_destRects[2].y()+m_destRects[2].height()) != centerHeight)
    return false;

    //the top/bottom edges
    m_destRects[1] = QRectF(m_destRects[0].x()+m_destRects[0].width(),
                m_destRects[0].y(),
                centerWidth,m_destRects[0].height());
    m_destRects[7] = QRectF(m_destRects[6].x()+m_destRects[6].width(),
                m_destRects[6].y(),
                centerWidth,m_destRects[6].height());
    //the side edges
    m_destRects[3] = QRectF(m_destRects[0].x(),m_destRects[0].y()+m_destRects[0].height(),
                m_destRects[0].width(),centerHeight);
    m_destRects[5] = QRectF(m_destRects[2].x(),m_destRects[2].y()+m_destRects[2].height(),
                m_destRects[2].width(),centerHeight);
    //the center
    m_destRects[4] = QRectF(m_destRects[3].x()+m_destRects[3].width(),m_destRects[3].y(),
                centerWidth,centerHeight);

    for (int i=0;i<9;++i)
    {
    	m_geom |= m_destRects[i];
    }
    m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
    return true;
}
