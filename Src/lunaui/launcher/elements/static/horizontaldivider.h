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




#ifndef HORIZONTALDIVIDER_H_
#define HORIZONTALDIVIDER_H_

#include "thingpaintable.h"
#include <QUuid>
#include <QRectF>
#include <QRect>
#include "pixmapobject.h"
#include <QPointer>

class HorizontalDivider : public ThingPaintable
{
	Q_OBJECT

public:
	HorizontalDivider(const QRectF& geom);
	virtual ~HorizontalDivider();

	virtual bool resize(const QSize& newSize);
	virtual bool resize(quint32 newWidth,quint32 newHeight);

};

#endif /* HORIZONTALDIVIDER_H_ */
