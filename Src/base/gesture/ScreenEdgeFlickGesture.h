/* @@@LICENSE
*
*      Copyright (c) 2011-2013 LG Electronics, Inc.
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




#ifndef SCREENEDGEFLICKGESTURE_H
#define SCREENEDGEFLICKGESTURE_H

#include "Common.h"

#include <QGesture>
#include <QPoint>

class ScreenEdgeFlickGesture : public QGesture
{
public:

	enum Edge {
		EdgeUnknown = 0,
		EdgeTop,
		EdgeBottom,
		EdgeLeft,
		EdgeRight
	};

    ScreenEdgeFlickGesture(QObject* parent = 0)
        : QGesture(parent)
		, m_edge(EdgeUnknown)
		, m_yDistance(0){
	}

	Edge edge() const { return m_edge; }
	int yDistance() const { return m_yDistance; }
    static Qt::GestureType gestureType() {return type;}
    static void setGestureType (Qt::GestureType t) {  type = t;}
private:

	Edge m_edge;
	int m_yDistance;
    static Qt::GestureType type;
private:
    friend class ScreenEdgeFlickGestureRecognizer;
};

#endif /* SCREENEDGEFLICKGESTURE_H */
