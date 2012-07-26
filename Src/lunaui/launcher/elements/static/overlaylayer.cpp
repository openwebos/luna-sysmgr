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




#include "overlaylayer.h"
#include "dimensionslauncher.h"
#include "pixmapobject.h"

#include <QSize>
#include <QPainter>

OverlayLayer::OverlayLayer(const QRectF& geometry)
: ThingPaintable(geometry)
, m_mode(OverlayLayerMode::OverlayShadows)
, m_qp_tabBarShadow(0)
, m_qp_quickLaunchShadow(0)
{
}

//virtual
OverlayLayer::~OverlayLayer()
{

}

//virtual
int OverlayLayer::mode() const
{
	return (int)m_mode;
}

//virtual
void OverlayLayer::setMode(int v)
{
	m_mode = (OverlayLayerMode::Enum)v;
}

void OverlayLayer::setTabBarShadow(PixmapObject * p_shadowPmo)
{
	if (!p_shadowPmo)
	{
		return;
	}

	//compute the position
	LauncherObject * pLauncherObject = qobject_cast<LauncherObject *>(parentObject());
	if (!pLauncherObject)
	{
		return;
	}
	m_qp_tabBarShadow = p_shadowPmo;
	recomputeTabBarShadowPosition();
}

#include <QDebug>

void OverlayLayer::setQuickLaunchShadow(PixmapObject * p_shadowPmo)
{
	if (!p_shadowPmo)
	{
		return;
	}

	//compute the position
	LauncherObject * pLauncherObject = qobject_cast<LauncherObject *>(parentObject());
	if (!pLauncherObject)
	{
		return;
	}
	m_qp_quickLaunchShadow = p_shadowPmo;
	recomputeQuickLaunchShadowPosition();
}

//virtual
bool OverlayLayer::resize(const QSize& newSize)
{
	ThingPaintable::resize(newSize);
	//reposition the two shadows...the launcher knows where they go, and it MUST be the parent item
	recomputeShadowPositions();
	return true;
}

void OverlayLayer::recomputeShadowPositions()
{
	recomputeTabBarShadowPosition();
	recomputeQuickLaunchShadowPosition();
}

void OverlayLayer::recomputeTabBarShadowPosition()
{
	LauncherObject * pLauncherObject = qobject_cast<LauncherObject *>(parentObject());
	if (!pLauncherObject)
	{
		return;
	}

	if (m_qp_tabBarShadow)
	{
		QRectF tabBarArea = mapRectFromItem(pLauncherObject,pLauncherObject->areaTabBar());
		m_tabBarShadowArea.setTopLeft(tabBarArea.bottomLeft());
		m_tabBarShadowArea.setHeight((qreal)(m_qp_tabBarShadow->height()));
		m_tabBarShadowArea.setWidth(tabBarArea.width());
	}
	update();
}

void OverlayLayer::recomputeQuickLaunchShadowPosition()
{
	LauncherObject * pLauncherObject = qobject_cast<LauncherObject *>(parentObject());
	if (!pLauncherObject)
	{
		return;
	}

	if (m_qp_quickLaunchShadow)
	{
		QRectF r = pLauncherObject->areaQuickLaunchBar();
		QRectF quickLaunchArea = mapRectFromItem(pLauncherObject,r);
		m_quickLaunchShadowArea.setTopLeft(quickLaunchArea.topLeft()-QPointF(0.0,(qreal)(m_qp_quickLaunchShadow->height())));
		m_quickLaunchShadowArea.setHeight((qreal)(m_qp_quickLaunchShadow->height()));
		m_quickLaunchShadowArea.setWidth(quickLaunchArea.width());
	}
	update();
}

//virtual
bool OverlayLayer::resize(quint32 newWidth,quint32 newHeight)
{
	//really have to consolidate the resize() on a base class level one of these days...
	return resize(QSize(newWidth,newHeight));
}

//virtual
void OverlayLayer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	if (m_mode == OverlayLayerMode::Scrim)
	{
		QBrush sb = painter->brush();
		qreal so = painter->opacity();
		painter->setBrush(Qt::black);
		painter->setOpacity(0.65);
		painter->drawRect(m_geom);
		painter->setBrush(sb);
		painter->setOpacity(so);
	}
	else		///TODO: WARNING: change this if there are more modes or INVALID is a likely case that shouldn't paint "normal" mode things
	{
		if (m_qp_tabBarShadow)
		{
			QPoint sbo = painter->brushOrigin();
			painter->setBrushOrigin(m_tabBarShadowArea.topLeft());
			painter->fillRect(m_tabBarShadowArea, QBrush(*(*m_qp_tabBarShadow)));
			//painter->fillRect(m_tabBarShadowArea, QBrush(Qt::blue));
			painter->setBrushOrigin(sbo);
		}
		if (m_qp_quickLaunchShadow)
		{
			QPoint sbo = painter->brushOrigin();
			painter->setBrushOrigin(m_quickLaunchShadowArea.topLeft());
			painter->fillRect(m_quickLaunchShadowArea, QBrush(*(*m_qp_quickLaunchShadow)));
			//painter->fillRect(m_quickLaunchShadowArea, QBrush(Qt::red));
			painter->setBrushOrigin(sbo);
		}
	}
	//painter->setPen(Qt::green);
	//painter->drawRect(m_tabBarShadowArea);
	//painter->drawRect(m_quickLaunchShadowArea);
}

//virtual
void OverlayLayer::paintOffscreen(QPainter *painter)
{
}
