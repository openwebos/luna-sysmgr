/****************************************************************************
**
** Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
** All rights reserved.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Palm gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef FLICKEVENT_H
#define FLICKEVENT_H
#include <QEvent>
#include <SysMgrDefs.h>
// this is also defined in SysMgrDefs.h
//enum SysMgrGesture {
//    SysMgrGestureFlick = 0x0100 + 1,
//    SysMgrGestureSingleClick,
//    SysMgrGestureScreenEdgeFlick,
//    SysMgrGestureLast = 0xFFFFFFFF
//};

class SysMgrGestureEvent :public QEvent
{
 protected:
    Qt::GestureState m_state;
    QPointF m_hotSpot;
    SysMgrGestureEvent(QEvent::Type type):QEvent(type) {}
 public:
    QPointF	hotSpot () const {return m_hotSpot;}
    void	setHotSpot ( const QPointF & value ) { m_hotSpot = value;}
    Qt::GestureState  state () const {return m_state;}
    void  setState (Qt::GestureState value) { m_state = value;}
};

class FlickEvent : public SysMgrGestureEvent
{
public:

    FlickEvent(QObject* parent = 0):SysMgrGestureEvent((QEvent::Type)SysMgrGestureFlick) {}
    QPoint velocity() const { return m_velocity; }
    QPoint startPos() const { return m_startPos; }
    QPoint endPos() const { return m_endPos; }



public:
    static const QEvent::Type myType = static_cast<QEvent::Type>(QEvent::User+1);

private:

    QPoint m_velocity;
    QPoint m_endPos;
    QPoint m_startPos;
};

class ScreenEdgeFlickEvent : public SysMgrGestureEvent
{
public:

    enum Edge {
        EdgeUnknown = 0,
        EdgeTop,
        EdgeBottom,
        EdgeLeft,
        EdgeRight
    };

    ScreenEdgeFlickEvent(QObject* parent = 0)
        : SysMgrGestureEvent((QEvent::Type)SysMgrGestureScreenEdgeFlick)
        , m_edge(EdgeUnknown)
        , m_yDistance(0){
    }

    Edge edge() const { return m_edge; }
    int yDistance() const { return m_yDistance; }

private:

    Edge m_edge;
    int m_yDistance;

};
#endif // FLICKEVENT_H
