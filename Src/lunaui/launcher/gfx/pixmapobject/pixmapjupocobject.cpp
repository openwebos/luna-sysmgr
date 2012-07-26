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




#include "pixmapjupocobject.h"
#include "pixmapjupocrefobject.h"
#include "gfxsettings.h"

#include <QPainter>

///public:
QUuid	PixmapJUPOCObject::addPart(QPixmap& srcPixmap,const QRect& srcRect)
{
	if (srcPixmap.isNull())
	{
		return QUuid();
	}
	//proper clipped rect, please
	QRect csrcRect = (srcRect.isEmpty() ? srcPixmap.rect() : srcRect.intersect(srcPixmap.rect()));
	QRect targetRect = findSpace(csrcRect.size());
	if (targetRect.isEmpty())
	{
		//couldn't find space
		return QUuid();
	}
	//copy in
	QPainter localPainter(pm);
	localPainter.drawPixmap(targetRect,srcPixmap,csrcRect);
	QUuid nUid = QUuid::createUuid();
	m_parts[nUid] = targetRect;
	return nUid;
}

//virtual
QRect PixmapJUPOCObject::findSpace(QSize s)
{
	QRect r;
	//try to the right of last rect added
	int rc = trySize(s,r);
	if (rc == 2)
	{
		//space allocated ok
		return r;
	}
	else if (rc == 1)
	{
		//try a new row
		if (advanceRow((quint32)s.height()))
		{
			if (trySize(s,r) == 2)
			{
				return r;
			}
		}
	}
	//else no space
	return QRect();
}

bool PixmapJUPOCObject::advanceRow(quint32 heightNeeded)
{
	if (m_gridLocator.bottom()+heightNeeded >= (quint32)pm->height())
	{
		return false;
	}
	m_gridLocator = QRect(m_gridLocator.bottomLeft(),QSize(0,0));
	return true;
}

int PixmapJUPOCObject::trySize(QSize s,QRect& r_rect)
{
	//try to the right of last rect added
	if (m_gridLocator.right()+s.width()+2*GraphicsSettings::settings()->jupocInnerSpacing.width() < pm->width())
	{
		//fits width wise...how about height?
		if (m_gridLocator.top()+2*GraphicsSettings::settings()->jupocInnerSpacing.height() < pm->height())
		{
			//sure!...found space
			r_rect = QRect(QPoint(m_gridLocator.right()+GraphicsSettings::settings()->jupocInnerSpacing.width(),
					m_gridLocator.bottom()+GraphicsSettings::settings()->jupocInnerSpacing.height()),s);
			m_lastRectAdded =
					r_rect.adjusted(-GraphicsSettings::settings()->jupocInnerSpacing.width(),
							-GraphicsSettings::settings()->jupocInnerSpacing.height(),
							GraphicsSettings::settings()->jupocInnerSpacing.width(),
							GraphicsSettings::settings()->jupocInnerSpacing.height());
			//adjust the bottom of the gridlocator
			m_gridLocator.setHeight(qMax(m_gridLocator.height(),m_lastRectAdded.height()));
			//and the right edge
			m_gridLocator.setRight(m_lastRectAdded.right());
			return 2;
		}
		else
		{
			//doesn't fit height-wise. Nothing that can be done about this...not enough space
			return 0;
		}
	}
	//doesn't fit width wise...should try a new row
	return 1;
}

PixmapJUPOCRefObject * PixmapJUPOCObject::addAndGetPart(QPixmap& srcPixmap,const QRect& srcRect)
{
	if (srcPixmap.isNull())
	{
		return 0;
	}
	//proper clipped rect, please
	QRect csrcRect = (srcRect.isEmpty() ? srcPixmap.rect() : srcRect.intersect(srcPixmap.rect()));
	QRect targetRect = findSpace(csrcRect.size());
	if (targetRect.isEmpty())
	{
		//couldn't find space
		return 0;
	}
	//copy in
	QPainter localPainter(pm);
	localPainter.drawPixmap(targetRect,srcPixmap,csrcRect);
	QUuid nUid = QUuid::createUuid();
	m_parts[nUid] = targetRect;

	return new PixmapJUPOCRefObject(*this,nUid,targetRect);
}

PixmapJUPOCRefObject * PixmapJUPOCObject::getPart(const QUuid& partUid)
{
	QMap<QUuid,QRect>::const_iterator f = m_parts.constFind(partUid);
	if (f != m_parts.constEnd())
	{
		return new PixmapJUPOCRefObject(*this,partUid,f.value());
	}
	return 0;
}

//static
PixmapJUPOCRefObject * PixmapJUPOCObject::transfer(PixmapJUPOCObject * p_jupoc,PixmapObject * p_src)
{
	if ((p_jupoc == 0) || (p_src == 0))
	{
		return 0;
	}

	QRect targetRect = p_jupoc->findSpace(p_src->size());
	if (targetRect.isEmpty())
	{
		//no space...
		return 0;
	}

	//copy in
	QPainter localPainter(p_jupoc->pm);
	p_src->paint(&localPainter,QRectF(targetRect));
	QUuid nUid = QUuid::createUuid();
	p_jupoc->m_parts[nUid] = targetRect;

	//TODO: dispose properly
	delete p_src;
	return new PixmapJUPOCRefObject(*p_jupoc,nUid,targetRect);
}

//virtual
void PixmapJUPOCObject::fill(const QColor& c)
{
	//invalidates the jupoc
	m_parts.clear();
	m_lastRectAdded = QRect();
	m_gridLocator = QRect();
	PixmapObject::fill(c);
}

//static
PixmapJUPOCObject * PixmapJUPOCObject::newJUPOC(quint32 width,quint32 height)
{
	if ((width == 0) || (width > (quint32)GraphicsSettings::settings()->maxPixSize.width())
			|| (height == 0) || (height > (quint32)GraphicsSettings::settings()->maxPixSize.height()))
	{
		return 0;	//invalid size
	}
	return new PixmapJUPOCObject((int)width,(int)height);
}

//virtual
PixmapJUPOCObject::~PixmapJUPOCObject()
{
}

///protected:

PixmapJUPOCObject::PixmapJUPOCObject(int width,int height)
: PixmapObject(width,height)
{
	fill(Qt::transparent);
}
