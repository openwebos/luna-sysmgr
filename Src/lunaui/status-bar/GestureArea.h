/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Josh Palmer
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




#ifndef GESTUREAREA_H
#define GESTUREAREA_H

#include <QGraphicsObject>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QRadialGradient>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
QT_END_NAMESPACE

class GestureArea : public QGraphicsObject
{
	Q_OBJECT
	Q_PROPERTY(QPointF gradientFocus READ gradientFocus WRITE setGradientFocus)

public:
	GestureArea(int width, int height);
	~GestureArea();

	void init();
	void resize(int w, int h);
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void animateBar(int direction);

	QRectF boundingRect() const { return m_bounds; }
	QPointF gradientFocus() const { return m_gradientFocus; }
	void setGradientFocus(QPointF gradientFocus) { m_gradientFocus = gradientFocus; update(); }

private:
	QRect m_bounds;

	QPixmap* m_lightbarLPixmap;
	QPixmap* m_lightbarRPixmap;
	QRadialGradient m_radialGrad;

	qreal m_lightbarY;
	QPoint m_startPos;
	bool m_fired;
	bool m_isPortrait;

	QPropertyAnimation* m_focusAnimOut;
	QPropertyAnimation* m_focusAnimIn;

	QPointF m_gradientFocus;
	int m_animDir;

	bool sceneEvent(QEvent* event);

private Q_SLOTS:
	void focusAnimOutFinished();

};

#endif /* GESTUREAREA_H */
