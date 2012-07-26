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




#include "pixmaploader.h"
#include "pixmapobject.h"
#include "pixmap9tileobject.h"
#include "pixmap3htileobject.h"
#include "pixmap3vtileobject.h"
#include "pixmapfilmstripobject.h"

QPointer<PixmapObjectLoader> PixmapObjectLoader::s_qp_instance = 0;

//static
PixmapObjectLoader * PixmapObjectLoader::instance()
{
	if (!s_qp_instance)
	{
		s_qp_instance = new PixmapObjectLoader();
	}
	return s_qp_instance;
}

PixmapObjectLoader::PixmapObjectLoader()
{
}

//virtual
PixmapObjectLoader::~PixmapObjectLoader()
{
}

//virtual
PixmapObject * PixmapObjectLoader::quickLoad(const QString & fileName, const char * format, Qt::ImageConversionFlags flags,QObject * p_setOwner)
{
	PixmapObject * p = new PixmapObject(fileName,format,flags);
	if (!(p->valid()))
	{
		delete p;	//done this way so that there is no issues if ownership policy from loader -> caller change, in cases where the caller
						//would want to delete the invalid pixmap
		return 0;
	}
	p->setParent(p_setOwner);
	return p;
}

//virtual
PixmapObject * PixmapObjectLoader::quickLoad(const QString & fileName, const QSize& size, bool limitOnly, const char * format, Qt::ImageConversionFlags flags,QObject * p_setOwner)
{
	PixmapObject * p = new PixmapObject(fileName,size,limitOnly,format,flags);
	if (!(p->valid()))
	{
		delete p;	//done this way so that there is no issues if ownership policy from loader -> caller change, in cases where the caller
						//would want to delete the invalid pixmap
		return 0;
	}
	p->setParent(p_setOwner);
	return p;
}

//quickLoadNineTiled: uses the Scale tiling method for edge and center tiles
//the initial load just makes a "default tile config". Just call resize() on it later when final size is known
//virtual
Pixmap9TileObject * PixmapObjectLoader::quickLoadNineTiled(const QString & fileName,const quint32 topIn,const quint32 bottomIn,
																					const quint32 leftIn,const quint32 rightIn,
													const char * format, Qt::ImageConversionFlags flags,QObject * p_setOwner)
{
	Pixmap9TileObject * p = new Pixmap9TileObject(0,0,fileName,topIn,bottomIn,leftIn,rightIn,
													PixmapTilingStyle::Scale,PixmapTilingStyle::Scale,format,flags);

	if (!(p->valid()))
	{
		delete p;	//done this way so that there is no issues if ownership policy from loader -> caller change, in cases where the caller
		//would want to delete the invalid pixmap
		return 0;
	}
	p->setParent(p_setOwner);
	return p;
}

//virtual
Pixmap3HTileObject * PixmapObjectLoader::quickLoadThreeHorizTiled(const QString& fileName,const quint32 leftIn,const quint32 rightIn,
													const char * format, Qt::ImageConversionFlags flags,QObject * p_setOwner)
{
	Pixmap3HTileObject * p = new Pixmap3HTileObject(0,0,fileName,leftIn,rightIn,
			PixmapHTilingStyle::Scale,format,flags);

	if (!(p->valid()))
	{
		delete p;	//done this way so that there is no issues if ownership policy from loader -> caller change, in cases where the caller
		//would want to delete the invalid pixmap
		return 0;
	}
	p->setParent(p_setOwner);
	return p;
}

//virtual
Pixmap3VTileObject * PixmapObjectLoader::quickLoadThreeVertTiled(const QString& fileName,const quint32 topIn,const quint32 bottomIn,
														const char * format, Qt::ImageConversionFlags flags,QObject * p_setOwner)
{
	Pixmap3VTileObject * p = new Pixmap3VTileObject(0,0,fileName,topIn,bottomIn,
			PixmapVTilingStyle::Scale,format,flags);

	if (!(p->valid()))
	{
		delete p;	//done this way so that there is no issues if ownership policy from loader -> caller change, in cases where the caller
		//would want to delete the invalid pixmap
		return 0;
	}
	p->setParent(p_setOwner);
	return p;
}

//virtual
QList<PixmapObject *> PixmapObjectLoader::loadMulti(const QList<QRect>& coordinateRects, const QString& fileName ,
											const char * format, Qt::ImageConversionFlags flags,QObject * p_setOwner)
{
	//TODO: a bit wasteful...loads the whole pixmap and then copies
	QPixmap wholePm = QPixmap(fileName,format,flags);
	QList<PixmapObject *> resultList;
	if (wholePm.isNull())
	{
		return QList<PixmapObject *>();
	}
	for (QList<QRect>::const_iterator it = coordinateRects.constBegin();
			it != coordinateRects.constEnd();++it)
	{
		QPixmap * pCopyPm = new QPixmap(wholePm.copy(*it));
		PixmapObject * pPmo = 0;
		if (pCopyPm->isNull())
		{
			delete pCopyPm;
			pPmo = 0;		//if the copy failed, the spot in the result list still has to be 0 for a placeholder
		}
		else
		{
			pPmo = new PixmapObject(pCopyPm);
			pPmo->setParent(p_setOwner);
		}
		resultList << pPmo;
	}
	return resultList;
}

//virtual
PixmapFilmstripObject * PixmapObjectLoader::quickLoadFilmstrip(const QSize& frameSize,quint32 numFrames,FrameDirection::Enum direction,
		const QString & fileName,const QPoint& startOffset,
		const char * format, Qt::ImageConversionFlags flags,QObject * p_setOwner)
{
	PixmapFilmstripObject * pObj = new PixmapFilmstripObject(frameSize,numFrames,direction,fileName,format,flags,startOffset);
	if (!pObj->valid())
	{
		delete pObj;
		return 0;
	}
	pObj->setParent(p_setOwner);
	return pObj;
}
