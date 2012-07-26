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




#ifndef PIXMAPJUPOCOBJECT_H_
#define PIXMAPJUPOCOBJECT_H_

#include "pixmapobject.h"
#include <QRect>
#include <QUuid>
#include <QMap>

class PixmapJUPOCRefObject;

class PixmapJUPOCObject : public PixmapObject
{
	Q_OBJECT

public:

	friend class PixmapJUPOCRefObject;

	//adds the pixmap to this JUPOC, if it can be added. Only a part of the pixmap can be added, by specifying a srcRect (if it's empty, then
	//	the whole pixmap is added). If successfull, a Uid is returned that can be used to retrieve the part with getPart(), later.
	QUuid	addPart(QPixmap& srcPixmap,const QRect& srcRect = QRect());
	PixmapJUPOCRefObject * addAndGetPart(QPixmap& srcPixmap,const QRect& srcRect = QRect());
	PixmapJUPOCRefObject * getPart(const QUuid& partUid);

	//copies the Pmo p_src to the jupoc.
	// if return != null, then the object was copied and p_src disposed (do not attempt to use it after)
	// if return == null, then the call failed, nothing was copied, and p_src is untouched
	static PixmapJUPOCRefObject * transfer(PixmapJUPOCObject * p_jupoc,PixmapObject * p_src);

	virtual void fill(const QColor& c);

	static PixmapJUPOCObject * newJUPOC(quint32 width,quint32 height);
	virtual ~PixmapJUPOCObject();
protected:

	PixmapJUPOCObject(int width,int height);

	virtual QRect	findSpace(QSize s);
	bool			advanceRow(quint32 heightNeeded);
	//uses following return values: 0 = no space , 1 = try new row , 2 = rect allocated
	int			trySize(QSize s,QRect& r_rect);
protected:

	QMap<QUuid,QRect>	m_parts;
	QRect	m_gridLocator;		//my findSpace() uses this as well as m_lastRectAdded to do its little naive space finding algo
	QRect	m_lastRectAdded;
};

#endif /* PIXMAPJUPOCOBJECT_H_ */
