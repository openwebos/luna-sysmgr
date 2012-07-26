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




#include "Common.h"
#include "TouchPlot.h"
#include <QPainter>

TouchPlot::TouchPlot() :
m_width(1024),
m_height(768),
m_plotRectSize(5),
m_touches_down(0),
m_render_trails(false),
m_render_crosshairs(false),
m_collecting(false)
{
}

TouchPlot::~TouchPlot()
{
}

void TouchPlot::setSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

void TouchPlot::enableTouchPlotOption(TouchPlotOption_t TouchPlotOption, bool enable)
{
	switch(TouchPlotOption) {
		case TouchPlotOption_Trails: m_render_trails = enable; break;
		case TouchPlotOption_Crosshairs: m_render_crosshairs = enable; break;
		case TouchPlotOption_Collection: m_collecting = enable; break;
		default: break;
	}
}

bool TouchPlot::hasOptionsEnabled()
{
	return (m_render_trails || m_render_crosshairs || m_collecting);
}

void TouchPlot::updateTouches(QTouchEvent* touchEvents)
{
	QList<QTouchEvent::TouchPoint>::const_iterator it;

	for (it = touchEvents->touchPoints().begin(); it != touchEvents->touchPoints().end(); ++it) {

		bool valid_id = (*it).id() < m_max_touches;

		switch((*it).state())
		{
			case Qt::TouchPointPressed:
				if(m_touches_down == 0) {
					for(int i=0; i < m_max_touches; i++)
						m_touchPointHistory[i].clear();
				} else {
					if(valid_id)
						m_touchPointHistory[(*it).id()].clear();
				}
				if(m_touches_down <= m_max_touches)
					m_touches_down++;
				break;
			case Qt::TouchPointMoved: break;
			case Qt::TouchPointStationary: break;
			case Qt::TouchPointReleased:
				if(m_touches_down > 0)
					m_touches_down--;
				break;
			default: break;
		}

		if(valid_id)
			m_touchPointHistory[(*it).id()].push_back((*it));
	}
}

QRectF TouchPlot::boundingRect() const
{
	return QRectF(0, 0, m_width-1, m_height-1);
}

QBrush TouchPlot::stateToBrush(Qt::TouchPointState state)
{
	switch(state) {
		case Qt::TouchPointPressed: return QBrush(Qt::green);
		case Qt::TouchPointMoved: return  QBrush(Qt::blue);
		case Qt::TouchPointStationary: return QBrush(Qt::yellow);
		case Qt::TouchPointReleased: return QBrush(Qt::red);
		default: break;
	}

	return QBrush(Qt::black);
}

void TouchPlot::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	QList<QTouchEvent::TouchPoint>::const_iterator it;
	QPointF pos;
	QBrush brush;

	painter->setPen(QPen(Qt::white));

	for(int i=0; i < m_max_touches; i++)
	{
		QPointF prev;

		if(m_touchPointHistory[i].size() == 0) continue;

		if(m_render_trails) {
			it = m_touchPointHistory[i].begin();

			prev = (*it).pos();

			painter->setBrush(stateToBrush((*it).state()));

			painter->drawRect(prev.x() - m_plotRectSize/2, prev.y() - m_plotRectSize/2, m_plotRectSize, m_plotRectSize);

			for( ; it != m_touchPointHistory[i].end(); ++it)
			{
				painter->drawLine(prev, (*it).pos());
				prev = (*it).pos();
				painter->setBrush(stateToBrush((*it).state()));
				painter->drawRect(prev.x() - m_plotRectSize/2, prev.y() - m_plotRectSize/2, m_plotRectSize, m_plotRectSize);

			}
		}

		if(m_render_crosshairs && m_touchPointHistory[i].last().state() != Qt::TouchPointReleased) {
				pos = m_touchPointHistory[i].last().pos();

				painter->drawLine(0, pos.y(), m_width, pos.y());
				painter->drawLine(pos.x(), 0, pos.x(), m_height);
		}
	}
}
