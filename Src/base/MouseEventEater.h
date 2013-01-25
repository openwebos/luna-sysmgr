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
#include <QMouseEvent>
#include <QObject>
#include <QWidget>

#include "HostBase.h"

class MouseEventEater : public QObject
{
    Q_OBJECT

public:
    MouseEventEater(QObject *parent = 0) : QObject(parent) {}

protected:
    virtual bool eventFilter(QObject *o, QEvent *e) {
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

            // Filter out mouse events inside the window area, they will
            // be converted to touch events by Qt
            if (me->globalY() < (info.displayHeight + frameHeight)) {
                e->ignore();
                return true;
            }
        }

        return false;
    }
};

#endif /* MOUSEEVENTEATER_H */
