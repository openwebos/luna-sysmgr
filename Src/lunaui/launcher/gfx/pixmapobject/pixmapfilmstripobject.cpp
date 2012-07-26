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




#include "pixmapfilmstripobject.h"
#include <QPainter>

const char * PixmapFilmstripObject::FrameIndexPropertyName = "frameindex";
const char * PixmapFilmstripObject::TotalFramesPropertyName = "totalframes";

PixmapFilmstripObject::PixmapFilmstripObject(const QList<QRect> frameCoordinates,const QString & fileName, const char * format, Qt::ImageConversionFlags flags)
: PixmapObject(fileName,format,flags)
, m_currentFrameIndex(0)
, m_currentFrameRect(QRect())
{
	createFrames(frameCoordinates);
	if (!m_frames.empty())
	{
		m_currentFrameRect = m_frames[0];
	}
}

PixmapFilmstripObject::PixmapFilmstripObject(const QSize& frameSize,quint32 numFrames,FrameDirection::Enum direction,
						const QString& fileName,
						const char * format, Qt::ImageConversionFlags flags,
						const QPoint& startOffset)
: PixmapObject(fileName,format,flags)
, m_currentFrameIndex(0)
, m_currentFrameRect(QRect())
{
	createFrames(frameSize,numFrames,direction,startOffset);
	if (!m_frames.empty())
	{
		m_currentFrameRect = m_frames[0];
	}
}

//virtual
PixmapFilmstripObject::~PixmapFilmstripObject()
{
}

//virtual
QSize PixmapFilmstripObject::size() const
{
	return m_currentFrameRect.size();
}

//virtual
int PixmapFilmstripObject::width() const
{
	return m_currentFrameRect.size().width();
}

//virtual
int PixmapFilmstripObject::height() const
{
	return m_currentFrameRect.size().height();
}

//virtual
QSizeF PixmapFilmstripObject::sizeF() const
{
	return QSizeF(m_currentFrameRect.size());
}

//virtual
void PixmapFilmstripObject::paint(QPainter * painter)
{
	painter->drawPixmap(QPoint(0,0),*pm,m_currentFrameRect);
}

//virtual

void PixmapFilmstripObject::paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
{
	painter->drawPixmap(targetOriginInPainterCS,*pm,m_currentFrameRect);
}

//virtual
void PixmapFilmstripObject::paint(QPainter * painter,const QRectF& targetRectInPainterCS)
{
	painter->drawPixmap(targetRectInPainterCS,*pm,m_currentFrameRect);
}


#include <QDebug>
// (see comments in PixmapObject.h/cpp)
//virtual
void PixmapFilmstripObject::paint(QPainter * painter,const QRect& targetRectInPainterCS,
										  const QRect& sourceRect)
{
	painter->drawPixmap(targetRectInPainterCS,*pm,sourceRect.translated(m_currentFrameRect.topLeft()));
}

//virtual
bool PixmapFilmstripObject::valid() const
{
	if (m_frames.empty())
	{
		return false;
	}
	return PixmapObject::valid();
}

// goes to the next frame. if last frame and rollover=true, then it goes to first frame. Else, it does nothing
//virtual
void PixmapFilmstripObject::nextFrame(bool rollOver)
{
	if (m_frames.empty())
	{
		return;
	}

	m_currentFrameIndex = (rollOver ? (m_currentFrameIndex +1) % m_frames.size() : qMin(m_currentFrameIndex+1,(quint32)(m_frames.size()-1)));
	m_currentFrameRect = m_frames[m_currentFrameIndex];
}

//goes all the way back to the beginning (frame 0)
//virtual
void PixmapFilmstripObject::rewind()
{
	m_currentFrameIndex = 0;
}

//uses saturation mode: index  > framelist.size-1 will = size-1
//virtual
void PixmapFilmstripObject::setFrameIndex(quint32 index)
{
	if (m_frames.empty())
	{
		return;
	}
	m_currentFrameIndex = qMin(index,(quint32)(m_frames.size()-1));
	m_currentFrameRect = m_frames[m_currentFrameIndex];
}

//virtual
quint32 PixmapFilmstripObject::frameIndex() const
{
	return m_currentFrameIndex;
}

//virtual
quint32 PixmapFilmstripObject::totalFrames() const
{
	return m_frames.size();
}

//virtual
QSize PixmapFilmstripObject::minFrame() const
{
	return m_minFrameSize;
}

//virtual
QSize PixmapFilmstripObject::maxFrame() const
{
	return m_maxFrameSize;
}

////protected:

//virtual
void PixmapFilmstripObject::createFrames(const QSize& frameSize,quint32 numFrames,FrameDirection::Enum direction,const QPoint& startOffset)
{
	if ((numFrames == 0) || (direction == FrameDirection::INVALID) || frameSize.isEmpty() || (!frameSize.isValid()))
	{
		return;
	}
	QPoint adder;
	switch (direction)
	{
	case FrameDirection::North:
		adder = QPoint(0,-frameSize.height());
		break;
	case FrameDirection::South:
		adder = QPoint(0,frameSize.height());
		break;
	case FrameDirection::East:
		adder = QPoint(-frameSize.width(),0);
		break;
	case FrameDirection::West:
		adder = QPoint(frameSize.width(),0);
	default:
		break;
	}
	QPoint currentOrigin = startOffset;
	QList<QRect> frameCoordinates;
	for (quint32 i = 0;i<numFrames;++i,currentOrigin+=adder)
	{
		frameCoordinates << QRect(currentOrigin,frameSize);
	}
	createFrames(frameCoordinates);
}

//virtual
void PixmapFilmstripObject::createFrames(const QList<QRect>& frameCoordinates)
{

	m_frames.empty();
	m_minFrameSize = QSize();
	m_maxFrameSize = QSize();
	m_currentFrameIndex = 0;
	m_currentFrameRect = QRect();

	if (frameCoordinates.empty())
	{
		//there are no frames!
		return;
	}
	if (!pm)
	{
		//no source image
		return;
	}
	QRect srcImgRect = QRect(QPoint(0,0),pm->size());
	quint32 minArea = INT_MAX;
	for (int i=0;i<frameCoordinates.size();++i)
	{
		QRect fc = frameCoordinates[i];

		if (!fc.isValid())
		{
			//skip invalid frame
			continue;
		}
		//it needs to be within the src image
		fc = fc.intersected(srcImgRect);
		if (fc.isEmpty())
		{
			//skip frame not at least partially in the src img rect
			continue;
		}
		quint32 a = fc.width() * fc.height();
		if (a < minArea)
		{
			minArea = a;
			m_minFrameSize = fc.size();
		}
		if (fc.width() > m_maxFrameSize.width())
		{
			m_maxFrameSize.setWidth(fc.width());
		}
		if (fc.height() > m_maxFrameSize.height())
		{
			m_maxFrameSize.setHeight(fc.height());
		}

		m_frames << fc;
	}
}
