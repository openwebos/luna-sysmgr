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




#ifndef STATUSBARCLOCK_H
#define STATUSBARCLOCK_H

#include "StatusBarItem.h"
#include <QGraphicsObject>
#include <QTextLayout>

class StatusBarClock : public StatusBarItem
{
	Q_OBJECT

public:

	StatusBarClock(unsigned int padding = 0);
	virtual ~StatusBarClock();

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void updateTimeFormat(const char* format);

	void setDisplayDate(bool date);

	int width() const { return m_bounds.width(); }
	int height() const { return m_bounds.height(); }

	void setPadding( unsigned int padding);

private Q_SLOTS:
	void tick();
	void slotSystemTimeChanged();

private:
	static const int kMaxTimeChars = 10;

	void setTimeText(const char *time, bool doUpdate = true);
	void setTimeText(const QString& time, bool doUpdate = true);

	QString m_timeText;
	QRectF m_textRect;
	QFont* m_font;

	bool m_twelveHour;
	bool m_displayDate;
	char m_timeBuf[kMaxTimeChars];
	char* m_curTimeStr;
	unsigned int m_textPadding;

	struct {
		int year;
		int month;
		int day;
		int hour;
		int min;
	} m_lastUpdateTime;

	// TODO: use QBasicTimer instead? (may be more lightweight)
	QTimer *m_clockTimer;
};



#endif /* STATUSBARCLOCK_H */
