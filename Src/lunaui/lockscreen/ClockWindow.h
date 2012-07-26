/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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



#ifndef CLOCKWINDOW_H
#define CLOCKWINDOW_H

#include "Common.h"

#include "Timer.h"
#include "sptr.h"
#include "Event.h"

#include <QObject>
#include <QGraphicsObject>

class ClockWindow : public QGraphicsObject
{
public:
	ClockWindow();
	virtual ~ClockWindow();

	// QGraphicsItem::boundingRect
	QRectF boundingRect() const;
	void setBoundingRect(QRect bounds) { m_bounds = bounds; }

	// QGraphicsItem::paint
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void updateTimeFormat();
	void resize(int width, int height);

	void tick();

private:

	void loadDigits();

	static const unsigned int MaxTimeChars = 6;

	char m_time[MaxTimeChars];
	int m_width;
	bool m_twelveHour;

	typedef QHash<char, QPixmap> ImageMap;
	ImageMap m_digits;

	typedef struct {
		char key;
		const char* path;
	} ImagePath;

	QRect m_bounds;
};

#endif

