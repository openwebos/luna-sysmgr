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




#ifndef PIXMAPLOADER_H_
#define PIXMAPLOADER_H_

#include "pixmapfilmstripobject.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QList>
#include <QPixmap>
#include <QRect>

namespace PixmapObjectType
{
	enum Enum
	{
		Flat,
		ThreeTileH,
		ThreeTileV,
		NineTile
	};
}

namespace PixmapObjectTilingType
{
	enum Enum
	{
		Scale,
		Repeat
	};
}

class PixmapObject;
class Pixmap9TileObject;
class Pixmap3HTileObject;
class Pixmap3VTileObject;

class PixmapObjectLoader : public QObject
{
	Q_OBJECT
public:

	static PixmapObjectLoader * instance();		//not a strict singleton class; supports private loaders in addition to a master one
												// it isn't guarded though through the usual singleton pattern mechanisms so it can be nuked...beware
	PixmapObjectLoader();
	virtual ~PixmapObjectLoader();

	virtual PixmapObject * quickLoad(const QString & fileName, const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,QObject * p_setOwner=0);
	virtual PixmapObject * quickLoad(const QString & fileName, const QSize& size, bool limitOnly = true, const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,QObject * p_setOwner=0);

	//quickLoadNineTiled: uses the Scale tiling method for edge and center tiles
	virtual Pixmap9TileObject * quickLoadNineTiled(const QString & fileName,const quint32 topIn,const quint32 bottomIn,
																			const quint32 leftIn,const quint32 rightIn,
													const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,QObject * p_setOwner=0);

	virtual Pixmap3HTileObject * quickLoadThreeHorizTiled(const QString& filename,const quint32 topIn,const quint32 bottomIn,
													const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,QObject * p_setOwner=0);
	virtual Pixmap3VTileObject * quickLoadThreeVertTiled(const QString& filename,const quint32 leftIn,const quint32 rightIn,
														const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,QObject * p_setOwner=0);

	virtual QList<PixmapObject *> loadMulti(const QList<QRect>& coordinateRects, const QString& fileName ,
											const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,QObject * p_setOwner=0);

	virtual PixmapFilmstripObject * quickLoadFilmstrip(const QSize& frameSize,quint32 numFrames,FrameDirection::Enum direction,
			const QString & fileName,
			const QPoint& startOffset = QPoint(0,0),
			const char * format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor,QObject * p_setOwner=0);


protected:

	static QPointer<PixmapObjectLoader> s_qp_instance;
};

#endif /* PIXMAPLOADER_H_ */
