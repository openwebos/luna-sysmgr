/* @@@LICENSE
*
*      Copyright (c) 2010-2013 Hewlett-Packard Development Company, L.P.
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




#ifndef MOUSEEVENTEATER_H
#define MOUSEEVENTEATER_H

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <QObject>
#include <QWidget>
#include "HostBase.h"
#include "WindowServer.h"

class MouseEventEater : public QObject
{
    Q_OBJECT

public:
    MouseEventEater(QObject *parent = 0) : QObject(parent) {}

protected:
    virtual bool eventFilter(QObject *o, QEvent *e) {

#if defined(TARGET_DESKTOP)
        bool insideGestureStrip(false);

        static QString gestureStrip("GestureStrip");
        QString objectClassName(o->metaObject()->className());

        if (objectClassName == gestureStrip) {
            insideGestureStrip = true;
        } else if (o->parent()) {
            QString parentClassName(o->parent()->metaObject()->className());
            if (parentClassName == gestureStrip) {
                insideGestureStrip = true;
            }
        }

        if (insideGestureStrip) {
            e->ignore();
            return false;
        }
#else
        Q_UNUSED(o);
#endif


        bool handled = false;

        if (e->type() == QEvent::MouseButtonRelease ||
            e->type() == QEvent::MouseButtonPress ||
            e->type() == QEvent::MouseButtonDblClick ||
            e->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);
            HostInfo info = HostBase::instance()->getInfo();
            int frameHeight = 0;
            QWidget *w = QApplication::activeWindow();

            if (w) {
                frameHeight = w->frameGeometry().height() -
                              w->geometry().height();
            }

            // Try sending incoming mouse events to QGraphicsScene first, then
            // filter out the ones not handled by the scene. They will be
            // converted to touch events by Qt
            if (me->pos().y() < (info.displayHeight + frameHeight)) {
                QGraphicsScene *scene = WindowServer::instance()->scene();
                QPointF pos(me->pos());

                QGraphicsSceneMouseEvent *event = 0;

                if (me->type() == QEvent::MouseButtonPress) {
                    event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMousePress);
                } else if (me->type() == QEvent::MouseButtonRelease) {
                    event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseRelease);
                } else if (me->type() == QEvent::MouseButtonDblClick) {
                    event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseDoubleClick);
                } else if (me->type() == QEvent::MouseMove) {
                    event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseMove);
                }

                if (event) {
                    event->setScenePos(pos);
                    event->setScreenPos(me->globalPos());
                    event->setButton(me->button());
                    event->setButtons(me->buttons());
                    event->setModifiers(me->modifiers());
                    handled = QApplication::sendEvent(scene, event);
                    delete event;
                }

                if (handled) {
                    e->ignore();
                }
            }
        }

        return handled;
    }
};

#endif /* MOUSEEVENTEATER_H */
