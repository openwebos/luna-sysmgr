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




#ifndef DOTGRID_H_
#define DOTGRID_H_

#include "thingpaintable.h"
#include <QRectF>
#include <QSize>

class DotGrid : public ThingPaintable
{
	Q_OBJECT

public:
	DotGrid(const QRectF& geom,const QSize& cellsize);
	virtual ~DotGrid();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);

protected:

	QSize m_cellSize;
};

#endif /* DOTGRID_H_ */
