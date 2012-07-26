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




/*
    This is to share helper definitions/functions/classes between Palm's keyboard implementations.
    It is not meant to be shared outside the keyboard, meaning, it is meant to be changed freely, as often as needed, however needed...
*/

#ifndef PALMIMEHELPERS_H
#define PALMIMEHELPERS_H

#include <stdint.h>
#include <vector>

#include "Logging.h"
#include "qcolor.h"
#include "qpoint.h"

struct Mapper_IF
{
	virtual std::string		pointToKeys(const QPoint & point) = 0;
};

quint64 currentTime();

inline bool isUnicodeQtKey(Qt::Key key)		{ return key >= ' ' && key < Qt::Key_Escape; }
inline bool isFunctionKey(Qt::Key key)		{ return key >= Qt::Key_Escape; }

class PerfMonitor
{
public:
	PerfMonitor(const char * text = NULL) : m_text(text), m_sys_timeFirst(0), m_user_timeFirst(0), m_sys_timeLast(0), m_user_timeLast(0)
	{
		reset();
	}
	void trace(const char * message, GLogLevelFlags logLevel = G_LOG_LEVEL_DEBUG);

	void reset();

	~PerfMonitor();
private:
	bool takeTime(quint64 & sysTime, quint64 & userTime);
	static void traceTime(const char * step, const char * message, GLogLevelFlags logLevel, const quint64 & sysTime, const quint64 & userTime, const quint64 & sysTimeRef, const quint64 & userTimeRef);

	const char *	m_text;
	quint64			m_sys_timeFirst;
	quint64			m_user_timeFirst;
	quint64			m_sys_timeLast;
	quint64			m_user_timeLast;
};

class ColorMap
{
public:
	ColorMap() { m_colors.reserve(530); }
	const QColor &	operator[](quint32 index)
	{
		if (m_colors.size() < index + 1)
		{
			m_colors.reserve(index + 1);
			while (m_colors.size() < index + 1)
			{
				int k = m_colors.size() + 1;
				m_colors.push_back(QColor(k * 1297519 % 256, k * 507919 % 256, k * 353527 % 256));
			}
		}
		return m_colors[index];
	}

private:
	std::vector<QColor>	m_colors;
};

#endif // PALMIMEHELPERS_H
