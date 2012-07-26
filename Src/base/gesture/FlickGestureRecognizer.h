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




#ifndef FLICKGESTURERECOGNIZER_H
#define FLICKGESTURERECOGNIZER_H

#include "Common.h"

#include <QGestureRecognizer>
#include <QPoint>
#include <QQueue>

class FlickGesture;

class FlickGestureRecognizer : public QGestureRecognizer
{
public:

	FlickGestureRecognizer();
	virtual ~FlickGestureRecognizer();

	virtual QGesture* create(QObject* target);
	virtual QGestureRecognizer::Result recognize(QGesture* gesture, QObject* watched, QEvent* event);

private:
	void addSample(const QPoint& position, int timestamp);

	const int m_maxSamples;
	QQueue<QPoint> m_points;
	QQueue<int> m_timestamps;
};

#endif /* FLICKGESTURERECOGNIZER_H */
