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




#ifndef PIXMAPFILMSTRIPOBJECT_H_
#define PIXMAPFILMSTRIPOBJECT_H_

#include "pixmapobject.h"
#include <QRect>
#include <QRectF>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QList>

class QPainter;

namespace FrameDirection
{
	enum Enum
	{
		INVALID,
		North,
		South,
		East,
		West
	};
}
class PixmapFilmstripObject : public PixmapObject
{
	Q_OBJECT
	Q_PROPERTY(quint32 frameindex READ frameIndex WRITE setFrameIndex)
	Q_PROPERTY(quint32 totalframes READ totalFrames)

public:

	static const char * FrameIndexPropertyName;
	static const char * TotalFramesPropertyName;

	PixmapFilmstripObject(const QList<QRect> frameCoordinates,const QString & fileName,
							const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor );
	PixmapFilmstripObject(const QSize& frameSize,quint32 numFrames,FrameDirection::Enum direction,
							const QString & fileName,
							const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,
							const QPoint& startOffset = QPoint(0,0));
	virtual ~PixmapFilmstripObject();

	virtual QSize size() const;
	virtual int width() const;
	virtual int height() const;
	virtual QSizeF sizeF() const;

	virtual void paint(QPainter * painter);
	virtual void paint(QPainter * painter,const QPointF& targetOriginInPainterCS);
	virtual void paint(QPainter * painter,const QRectF& targetRectInPainterCS);

	// (see comments in PixmapObject.h/cpp)
	virtual void paint(QPainter * painter,const QRect& targetRectInPainterCS,
										  const QRect& sourceRect);

	virtual bool valid() const;

	// goes to the next frame. if last frame and rollover=true, then it goes to first frame. Else, it does nothing
	virtual void nextFrame(bool rollOver=false);

	//goes all the way back to the beginning (frame 0)
	virtual void rewind();

	//uses saturation mode: index  > framelist.size-1 will = size-1
	virtual void setFrameIndex(quint32 index);
	virtual quint32 frameIndex() const;
	virtual quint32 totalFrames() const;

	virtual QSize minFrame() const;
	virtual QSize maxFrame() const;

protected:

	// creates numFrames equal sized (frameSize) frames, optionally offset into the source pix
	virtual void createFrames(const QSize& frameSize,quint32 numFrames,FrameDirection::Enum direction,const QPoint& startOffset = QPoint(0,0));

	// creates arbitrarily sourced frames from the source pix
	virtual void createFrames(const QList<QRect>& frameCoordinates);

protected:

	quint32 m_currentFrameIndex;
	QRect m_currentFrameRect;
	QSize m_minFrameSize;		//the width and height of the single frame with the smallest area
	QSize m_maxFrameSize;		// the maximum width and height of all frames; i.e. the minimum area needed to display every frame fully
	QList<QRect> m_frames;			//in progression order

};

#endif /* PIXMAPFILMSTRIPOBJECT_H_ */
