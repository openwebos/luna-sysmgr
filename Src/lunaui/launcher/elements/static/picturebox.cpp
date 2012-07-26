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




#include "picturebox.h"
#include "dimensionsglobal.h"
#include "debugglobal.h"
#include "pixmapobject.h"

#include <QPainter>
#include <QString>
#include <QDebug>

PictureBox::PictureBox(PixmapObject * p_mainPmo,PixmapObject * p_backgroundPmo,const QRectF& geom)
: ThingPaintable(geom)
, m_qp_innerPic(p_mainPmo)
, m_qp_background(p_backgroundPmo)
{
	if (!m_qp_innerPic)
	{
		setFlag(ItemHasNoContents,true);	//death!
	}
	else
	{
		ThingPaintable::resize((*m_qp_innerPic)->size());
	}
}

//virtual
PictureBox::~PictureBox()
{
	//TODO: PMO-MANAGE: i don't own the pmos....
}

//virtual
void PictureBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	//TODO: background paint???
//	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,boundingRect(),7);
	m_qp_innerPic->paint(painter,m_geom.topLeft());
}

//virtual
void PictureBox::paintOffscreen(QPainter *painter)
{
}
