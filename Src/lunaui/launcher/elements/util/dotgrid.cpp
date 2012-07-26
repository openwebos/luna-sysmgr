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




#include "dotgrid.h"
#include <QPainter>
#include <QPen>

DotGrid::DotGrid(const QRectF& geom,const QSize& cellsize)
: ThingPaintable(geom)
{
	m_cellSize = QSize( (cellsize.width() != 0 ? qAbs(cellsize.width()) : 2),
						(cellsize.height() != 0 ? qAbs(cellsize.height()) : 2));
}

//virtual
DotGrid::~DotGrid()
{
}

//virtual
void DotGrid::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	QPen savePen = painter->pen();
	painter->setPen(Qt::red);
	QPointF pt = m_geom.topLeft();
	for (int j=0;j<(int)m_geom.height();++j)
	{
		for (int i=0;i<(int)m_geom.width();++i)
		{
			painter->drawEllipse(pt,0.5,0.5);
			pt += QPointF((qreal)m_cellSize.width(),0.0);
		}
		pt.setX(m_geom.left());
		pt += QPointF(0.0,(qreal)m_cellSize.height());
	}
	painter->setPen(savePen);
}

//virtual
void DotGrid::paintOffscreen(QPainter *painter)
{
	paint(painter);
}
