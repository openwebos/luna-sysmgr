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




#ifndef DEMOGOGGLES3_H_
#define DEMOGOGGLES3_H_

#include "thingpaintable.h"
#include "gogglesblurshadereffect.h"
#include <QPointer>

class PixmapObject;
class DemoGoggles3 : public ThingPaintable
{
	Q_OBJECT

public:
	DemoGoggles3(const QRectF& geometry,PixmapObject * p_backgroundPmo,const QRectF& viewRect);
	virtual ~DemoGoggles3();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);

	virtual void updateBackground(PixmapObject * p_newBgPmo,const QRectF& viewRect);
	virtual void activate();
	virtual void deactivate();

protected:

	QPointer<PixmapObject> m_qp_backgroundPmo;
	QRectF m_sceneBgViewrect;
	GogglesBlurShaderEffect m_effect;
};

#endif /* DEMOGOGGLES3_H_ */
