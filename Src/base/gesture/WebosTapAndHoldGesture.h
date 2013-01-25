/* @@@LICENSE
*
*      Copyright (c) 2013 Hewlett-Packard Development Company, L.P.
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

#ifndef WEBOSTAPANDHOLDGESTURE_H
#define WEBOSTAPANDHOLDGESTURE_H
#include <QPoint>
#include <QGesture>

class WebosTapAndHoldGesture : public QGesture
{
    Q_OBJECT

    QPointF pos;
    int timerId;
    int tapTimeout;
    static Qt::GestureType type;
public:
    WebosTapAndHoldGesture(QObject *parent = 0) {
        tapTimeout = 700;
    }

    QPointF position() const { return pos;}
    void setPosition(const QPointF &p) { pos = p;}

    void setTimeout(int msecs) { tapTimeout = msecs;}
    int timeout() { return tapTimeout;}
    int tapTimerId () { return timerId;}
    void startTapTimer() {  timerId = this->startTimer(tapTimeout);}
    void stopTapTimer() {
        if (timerId)
            this->killTimer(timerId);
        timerId = 0;
    }

    static Qt::GestureType gestureType() { return type;}
    static void setGestureType (Qt::GestureType t) { type = t;}

    friend class WebosTapAndHoldGestureRecognizer;
};

#endif // WEBOSTAPANDHOLDGESTURE_H
