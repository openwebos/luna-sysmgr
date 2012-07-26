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




#include "demogoggles.h"
#include "pixmapobject.h"
#include "dimensionsglobal.h"
#include <QString>
#include <QRectF>
#include <QPainter>

DemoGoggles::DemoGoggles(const QRectF& geometry,PixmapObject * p_backgroundPmo,const QRectF& viewRect)
: ThingPaintable(geometry)
, m_qp_backgroundPmo(p_backgroundPmo)
, m_sceneBgViewrect(viewRect)
{
	setGraphicsEffect(&m_effect);
	m_effect.setEnabled(false);
	if (!m_qp_backgroundPmo)
	{
		setFlag(ItemHasNoContents,true);
	}
}

//virtual
DemoGoggles::~DemoGoggles()
{
	delete m_qp_backgroundPmo;
}

//virtual
void DemoGoggles::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	//map myself into the scene
	QRectF sceneRect = mapRectToScene(boundingRect());
	QRectF sourceRect = sceneRect.intersected(m_sceneBgViewrect).translated(-m_sceneBgViewrect.topLeft());
	m_qp_backgroundPmo->paint(painter,boundingRect().toRect(),sourceRect.toRect());
}

//virtual
void DemoGoggles::paintOffscreen(QPainter *painter)
{
}

//virtual
void DemoGoggles::updateBackground(PixmapObject * p_newBgPmo,const QRectF& viewRect)
{
	delete m_qp_backgroundPmo;
	m_qp_backgroundPmo = p_newBgPmo;
	m_sceneBgViewrect = viewRect;
	if (!m_qp_backgroundPmo)
	{
		setFlag(ItemHasNoContents,true);
	}
	else
	{
		setFlag(ItemHasNoContents,false);
	}

}

//virtual
void DemoGoggles::activate()
{
	m_effect.setEnabled(true);
}

//virtual
void DemoGoggles::deactivate()
{
	m_effect.setEnabled(false);
}
