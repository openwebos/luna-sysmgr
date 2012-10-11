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




#include "dimensionsglobal.h"
#include "pixbuttonsimple.h"
#include "pixmapobject.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QString>
#include <QDebug>

#include <QEvent>
#include <QGesture>
#include <QGestureEvent>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

//public:

PixButtonSimple::PixButtonSimple(PixmapObject * p_pix)
: PixButton(QRectF(0,0,1,1))
, m_qp_pix(p_pix)
, m_valid(true)
, m_qp_hitArea(0)
{
	PixButtonSimple::commonCtor();
}

PixButtonSimple::PixButtonSimple(PixmapObject * p_pix,const QSize& hitAreaSize)
: PixButton(QRectF(0,0,1,1))
, m_qp_pix(p_pix)
, m_valid(true)
{
	m_qp_hitArea = new PixButtonExtraHitArea(*this,hitAreaSize);
	PixButtonSimple::commonCtor();
}

//virtual
void PixButtonSimple::commonCtor()
{
	if (!m_qp_pix)
	{
		m_valid = false;
		//prevent it from being painted, and avoid costly if-checks in paint()
		setFlag(ItemHasNoContents,true);
	}
	else
	{
		ThingPaintable::resize(m_qp_pix->size());
	}

	if (m_qp_hitArea)
	{
		connect(m_qp_hitArea,SIGNAL(signalHit()),
				this,SLOT(slotExtraHitAreaTriggered()));
		m_qp_hitArea->setParent(this);		//so it'll get deleted automatically with this one
	}
}

//virtual
PixButtonSimple::~PixButtonSimple()
{
}

//virtual
void PixButtonSimple::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	m_qp_pix->paint(painter,m_geom.topLeft());
}

//virtual
void PixButtonSimple::paintOffscreen(PixmapObject * p_pix)
{
	//TODO: IMPLEMENT
}

//virtual
bool PixButtonSimple::valid()
{
	return m_valid;
}

