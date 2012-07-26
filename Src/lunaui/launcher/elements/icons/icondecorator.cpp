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




#include "dimensionsglobal.h"
#include "icondecorator.h"
#include "pixmapobject.h"

IconDecorator::IconDecorator()
: m_qp_pixmap(0)
, m_shown(false)
{
}

IconDecorator::IconDecorator(PixmapObject * p_pix,const QRectF& ownerCoordRelativeBounds)
: m_qp_pixmap(p_pix)
, m_bounds(ownerCoordRelativeBounds)
, m_shown(false)
{
	m_iconGeom = DimensionsGlobal::realRectToPixelCompatibleForm(m_bounds).toRect();
}

IconDecorator::~IconDecorator()
{
	//assume i don't own the pixmapobject
}


PixmapObject * IconDecorator::setNewPix(PixmapObject * p_newPix, bool show)
{
	if (!p_newPix)
		return m_qp_pixmap;
	PixmapObject * v = m_qp_pixmap;
	m_qp_pixmap = p_newPix;
	m_shown = show;
	return v;
}

bool IconDecorator::hitTest(const QPointF& pt) const
{
	return m_bounds.contains(pt);
}

bool IconDecorator::hitTest(const QPoint& pt) const
{
	return m_bounds.contains(QPointF(pt));
}

void IconDecorator::show()
{
	m_shown = true;
}
void IconDecorator::hide()
{
	m_shown = false;
}

PixmapObject * IconDecorator::pix() const
{
	if ((!m_qp_pixmap) || (!m_shown))
		return 0;
	return m_qp_pixmap;
}
