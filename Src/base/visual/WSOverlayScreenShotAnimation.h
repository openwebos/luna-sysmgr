/* @@@LICENSE
*
*      Copyright (c) 2010-2013 LG Electronics, Inc.
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




#ifndef WSOVERLAYSCREENSHOTANIMATION_H
#define WSOVERLAYSCREENSHOTANIMATION_H

#include "Common.h"

#include <QAbstractAnimation>
#include <QGraphicsObject>
#include <QPointer>
#include <QRadialGradient>

class WSOverlayScreenShotAnimation : public QGraphicsObject
{
	Q_OBJECT

public:

	WSOverlayScreenShotAnimation();
	virtual ~WSOverlayScreenShotAnimation();

private Q_SLOTS:
	
	void start();
	void stop();
	void finished();

private:

	virtual QRectF boundingRect () const;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

private:

	QPointer<QAbstractAnimation> m_animation;
	QRectF m_boundingRect;
	QRadialGradient m_gradient;
};
	

#endif /* WSOVERLAYSCREENSHOTANIMATION_H */
