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




#ifndef PIXMAPJUPOCREFOBJECT_H_
#define PIXMAPJUPOCREFOBJECT_H_

#include "pixmapjupocrefobject.h"
#include "pixmapobject.h"
#include "pixmapjupocobject.h"
#include <QPointer>

class PixmapJUPOCObject;
class PixmapJUPOCRefObject : public PixmapObject
{
	Q_OBJECT

public:

	friend class PixmapJUPOCObject;

	virtual ~PixmapJUPOCRefObject();

	virtual bool valid() const;
	virtual QSize size() const;
	virtual int width() const;
	virtual int height() const;
	virtual QSizeF sizeF() const;
	virtual quint64 sizeOf() const;
	virtual bool isSquare() const;

	virtual QSize nativeSize() const;
	virtual int nativeWidth() const;
	virtual int nativeHeight() const;
	virtual QSizeF nativeSizeF() const;

	virtual void fill(const QColor& c);
	virtual void paint(QPainter * painter);
	virtual void paint(QPainter * painter,const QPointF& targetOriginInPainterCS);
	virtual void paint(QPainter * painter,const QRectF& targetRectInPainterCS);
	virtual void paint(QPainter * painter,const QRect& targetRectInPainterCS,
											  const QRect& sourceRect);

protected:

	PixmapJUPOCRefObject(PixmapJUPOCObject& rPmo,const QUuid& myUid,const QRect& sourceRect);

private:

	PixmapJUPOCRefObject();

protected:

	QUuid						m_refJupocPmoUid;
	QPointer<PixmapJUPOCObject> m_qp_refJupocPmo;	//for potentially quicker access; it's the pmo whos uid is stored in m_refJupocPmoUid
	QRect 						m_jupocSrcRect;
};

#endif /* PIXMAPJUPOCREFOBJECT_H_ */
