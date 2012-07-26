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




#ifndef TIMEDELAYTRANSITION_H_
#define TIMEDELAYTRANSITION_H_

#include <QObject>
#include <QSignalTransition>
#include <QVariant>
#include <QTimer>

class QEvent;
class TimeDelayTransition : public QSignalTransition
{
	Q_OBJECT
public:
	TimeDelayTransition(const quint32 delayMs);
	virtual ~TimeDelayTransition();

	virtual void setDelayMs(const quint32 delayMs);

public Q_SLOTS:
	virtual void slotRestartTimer();
	virtual void slotAbort();

protected:
	virtual bool eventTest(QEvent *e);

	QTimer m_timer;
	quint32 m_currentDelayMs;
};

#endif /* TIMEDELAYTRANSITION_H_ */
