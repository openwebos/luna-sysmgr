/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef FULLERASECONFIRMATIONWINDOW_H_
#define FULLERASECONFIRMATIONWINDOW_H_

#include "Common.h"

#include <QGraphicsObject>
#include <QPainter>
#include <QFont>
#include <QTimer>

class FullEraseConfirmationWindow: public QGraphicsObject
{
	Q_OBJECT

public:

	FullEraseConfirmationWindow();
	virtual ~FullEraseConfirmationWindow();

	virtual QRectF boundingRect() const { return m_boundingRect; }

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

Q_SIGNALS:

	void signalFullEraseConfirmed();

private Q_SLOTS:

	void slotTimerTicked();

private:

	void setupLabels();
	void updateCountDownText();

	QPixmap m_warningPix;
	qreal m_warningOffsetY;

	int m_secondsLeft;

	QString m_title;
	QRectF m_titleBounds;

	QString m_description;
	QRectF m_descBounds;

	QString m_countDown;
	QRectF m_countDownBounds;

	QFont m_font;
	QRectF m_boundingRect;
	QTimer m_timer;
};

#endif /* FULLERASECONFIRMATIONWINDOW_H_ */
