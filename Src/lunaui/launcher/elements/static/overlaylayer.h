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




#ifndef OVERLAYLAYER_H_
#define OVERLAYLAYER_H_

#include "thingpaintable.h"

#include <QPointer>

class PixmapObject;

namespace OverlayLayerMode
{
	enum Enum
	{
		INVALID,
		Scrim,
		OverlayShadows
	};
}

class OverlayLayer : public ThingPaintable
{
	Q_OBJECT
	Q_PROPERTY(int mode READ mode WRITE setMode)

public:
	OverlayLayer(const QRectF& geometry);
	virtual ~OverlayLayer();

	//these are done separately from the ctor because they will access the parentObject (the launcher)
	// to get the correct positions of the shadows
	// this is a little more convenient than having to constantly pass launcherobject into each constructed child element
	// or having to call the "primary instance singleton" getter of launcherobject
	// Thus, the correct sequence of initializing overlaylayer is:
	// obj = new OverlayLayer
	// obj->setParentItem(launcherobj)
	// obj->setTabBarShadow
	// obj->setQuickLaunchShadow
	void	setTabBarShadow(PixmapObject * p_shadowPmo);
	void	setQuickLaunchShadow(PixmapObject * p_shadowPmo);

	virtual int mode() const;
	virtual void setMode(int v);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget);
	virtual void paintOffscreen(QPainter *painter);

	virtual bool resize(const QSize& newSize);
	virtual bool resize(quint32 newWidth,quint32 newHeight);

	void recomputeShadowPositions();

protected:

	void recomputeTabBarShadowPosition();
	void recomputeQuickLaunchShadowPosition();

protected:

	OverlayLayerMode::Enum m_mode;
	QPointer<PixmapObject> m_qp_tabBarShadow;
	QRectF m_tabBarShadowArea;
	QPointer<PixmapObject> m_qp_quickLaunchShadow;
	QRectF m_quickLaunchShadowArea;
};

#endif /* OVERLAYLAYER_H_ */
