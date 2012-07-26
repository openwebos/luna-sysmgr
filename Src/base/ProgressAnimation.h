/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef PROGRESSANIMATION_H
#define PROGRESSANIMATION_H

#include "Common.h"

#include <QGraphicsObject>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class ProgressAnimation : public QGraphicsObject
{
	Q_OBJECT
	Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness)

public:

    enum Type {
        TypeHp = 0,
        TypeMsm,
        TypeFsck
    };

	ProgressAnimation(Type type = TypeHp);
	virtual ~ProgressAnimation();

	void start();
	void stop();

	virtual QRectF boundingRect() const;

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

private Q_SLOTS:

	void slotPulseCompleted();

Q_SIGNALS:

	void signalProgressAnimationCompleted();

private:

	qreal brightness() const { return m_brightness; }
	void setBrightness(qreal brightness);

    void constructLogo(const QString& title, const QString& description);

	QPixmap m_normalPix;
	QPixmap m_brightPix;
    QPixmap m_logoPix;

	qreal m_brightness;

	bool m_pulsing;
	QPropertyAnimation m_pulseAnimation;
	QParallelAnimationGroup m_zoomFadeAnimation;

	QRectF m_boundingRect;
};

#endif /* PROGRESSANIMATION_H */
