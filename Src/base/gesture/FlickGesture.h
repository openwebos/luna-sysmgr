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




#ifndef FLICKGESTURE_H
#define FLICKGESTURE_H

#include "Common.h"

#include <QGesture>
#include <QPoint>

#include <SysMgrDefs.h>

class FlickGesture : public QGesture
{
public:

	FlickGesture(QObject* parent = 0) : QGesture(parent, (Qt::GestureType) SysMgrGestureFlick) {}
	QPoint velocity() const { return m_velocity; }
	QPoint startPos() const { return m_startPos; }
	QPoint endPos() const { return m_endPos; }

private:

	QPoint m_velocity;
	QPoint m_endPos;
	QPoint m_startPos;

private:

	friend class FlickGestureRecognizer;

#if defined(HAVE_QPA)
	friend class QPAHiddTpHandler;
#else
	friend class QWSHiddTpHandlerPrivate;
#endif
};

#endif /* FLICKGESTURE_H */
