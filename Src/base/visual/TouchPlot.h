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




#ifndef TOUCHPLOT_H
#define TOUCHPLOT_H

#include <stdint.h>
#include "Common.h"

#include <QGraphicsObject>
#include <QTouchEvent>

class TouchPlot : public QGraphicsItem
{
public:

	TouchPlot();
	virtual ~TouchPlot();

	void setSize(uint32_t width, uint32_t height);
	void updateTouches(QTouchEvent* touchEvents);

	enum TouchPlotOption_t {
		TouchPlotOption_Collection,
		TouchPlotOption_Trails,
		TouchPlotOption_Crosshairs,
	};

	void enableTouchPlotOption(TouchPlotOption_t TouchPlotOption, bool enable);

	bool hasOptionsEnabled();

private:

	virtual QRectF boundingRect () const;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);
	QBrush stateToBrush(Qt::TouchPointState state);

private:
	static const int m_max_touches = 10;
	uint32_t m_width, m_height;
	int m_plotRectSize;

	bool m_render_trails;
	bool m_render_crosshairs;
	bool m_collecting;

	QList<QTouchEvent::TouchPoint> m_touchPointHistory[m_max_touches];
	int m_touches_down;

};

#endif /* TOUCHPLOT_H */
