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




#include "pixmapjupocrefobject.h"
#include <QPainter>
#include <QRect>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

//virtual
PixmapJUPOCRefObject::~PixmapJUPOCRefObject()
{
}

//virtual
bool PixmapJUPOCRefObject::valid() const
{
	return false;
}
//virtual
QSize PixmapJUPOCRefObject::size() const
{
	return QSize();
}
//virtual
int PixmapJUPOCRefObject::width() const
{
	return 0;
}
//virtual
int PixmapJUPOCRefObject::height() const
{
	return 0;
}
//virtual
QSizeF PixmapJUPOCRefObject::sizeF() const
{
	return QSizeF();
}
//virtual
quint64 PixmapJUPOCRefObject::sizeOf() const
{
	return 0;
}
//virtual
bool PixmapJUPOCRefObject::isSquare() const
{
	return false;
}

//virtual
QSize PixmapJUPOCRefObject::nativeSize() const
{
	return QSize();
}
//virtual
int PixmapJUPOCRefObject::nativeWidth() const
{
	return 0;
}
//virtual
int PixmapJUPOCRefObject::nativeHeight() const
{
	return 0;
}
//virtual
QSizeF PixmapJUPOCRefObject::nativeSizeF() const
{
	return QSizeF();
}

//virtual
void PixmapJUPOCRefObject::fill(const QColor& c)
{
}

//virtual
void PixmapJUPOCRefObject::paint(QPainter * painter)
{

}
//virtual
void PixmapJUPOCRefObject::paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
{

}
//virtual
void PixmapJUPOCRefObject::paint(QPainter * painter,const QRectF& targetRectInPainterCS)
{

}
//virtual
void PixmapJUPOCRefObject::paint(QPainter * painter,const QRect& targetRectInPainterCS,
											  const QRect& sourceRect)
{

}

////protected:

PixmapJUPOCRefObject::PixmapJUPOCRefObject(PixmapJUPOCObject& rPmo,const QUuid& myUid,const QRect& sourceRect)
: PixmapObject(myUid)
{

}


