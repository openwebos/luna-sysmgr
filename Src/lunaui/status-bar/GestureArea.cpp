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




#include <QApplication>
#include <QPainter>
#include <QGesture>
#include <QDebug>
#include <QWidget>
#include <QPropertyAnimation>

#include "GestureArea.h"
#include "SystemUiController.h"
#include "Settings.h"
#include "HostBase.h"

//DEBUG
#include "WindowServer.h"

enum AnimDir {
	Up = 0,
	Left,
	Right
};

static const int GESTURE_AREA_TRIGGER_DIST = Settings::LunaSettings()->virtualCoreNaviHeight/4;

GestureArea::GestureArea(int width, int height)
{
	m_bounds = QRect(-width/2, SystemUiController::instance()->currentUiHeight()/2, width, height);
	m_fired = false;
	m_animDir = AnimDir(Up);

	m_radialGrad = QRadialGradient(m_bounds.center(), m_bounds.width()/3);
	m_radialGrad.setColorAt(0, QColor(0, 0, 0, 0));
	m_radialGrad.setColorAt(1, QColor(0, 0, 0, 255));

	m_focusAnimOut = new QPropertyAnimation(this, "gradientFocus");
	m_focusAnimOut->setDuration(250);
	connect(m_focusAnimOut, SIGNAL(finished()), this, SLOT(focusAnimOutFinished()));

	m_focusAnimIn = new QPropertyAnimation(this, "gradientFocus");
	m_focusAnimIn->setDuration(250);

	setAcceptTouchEvents(true);
	grabGesture(Qt::TapGesture);
}

GestureArea::~GestureArea()
{
	if(m_focusAnimOut)
		delete m_focusAnimOut;

	if(m_focusAnimIn)
		delete m_focusAnimIn;

	if(m_lightbarLPixmap)
		delete m_lightbarLPixmap;

	if(m_lightbarRPixmap)
		delete m_lightbarRPixmap;
}

void GestureArea::init()
{
	Settings* settings = Settings::LunaSettings();

	//Light Bar
	std::string lightbarLImagePath = settings->lunaSystemResourcesPath + "/corenavi/light-bar-bright-left.png";
	std::string lightbarRImagePath = settings->lunaSystemResourcesPath + "/corenavi/light-bar-bright-right.png";
	m_lightbarLPixmap = new QPixmap(lightbarLImagePath.c_str());
	m_lightbarRPixmap = new QPixmap(lightbarRImagePath.c_str());

	m_lightbarY = m_bounds.bottom() - m_lightbarLPixmap->height();
	m_gradientFocus = QPointF(0, m_lightbarY + (m_lightbarLPixmap->height()/2));
}

void GestureArea::resize(int w, int h)
{
	m_bounds = QRect(-w/2, SystemUiController::instance()->currentUiHeight()/2, w, Settings::LunaSettings()->virtualCoreNaviHeight);
	m_lightbarY = m_bounds.bottom() - m_lightbarLPixmap->height();
	m_gradientFocus = QPointF(0, m_lightbarY + (m_lightbarLPixmap->height()/2));
	m_isPortrait = (h > w);
	update();
}

void GestureArea::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	//Background
	painter->fillRect(m_bounds, QBrush(Qt::black));

	//Light Bar
	if(m_lightbarLPixmap)
		painter->drawPixmap(
			-m_lightbarLPixmap->width()
			, m_lightbarY
			, *m_lightbarLPixmap
			);
	if(m_lightbarRPixmap)
		painter->drawPixmap(
			0
			, m_lightbarY
			, *m_lightbarRPixmap
			);

	//Animation Gradient
	m_radialGrad.setCenter(m_gradientFocus);
	m_radialGrad.setFocalPoint(m_gradientFocus);

	QBrush oldBrush = painter->brush();
	QPen oldPen = painter->pen();
	painter->setBrush(QBrush(m_radialGrad));
	painter->setPen(QPen(Qt::black));

	painter->drawRect(m_bounds);

	painter->setBrush(oldBrush);
	painter->setPen(oldPen);
}

bool GestureArea::sceneEvent(QEvent* event)
{
	QWidget* window = QApplication::focusWidget();

	//Eat all touch events to stop them falling through to the launcher
	if (event->type() == QEvent::TouchBegin || event->type() == QEvent::TouchUpdate || event->type() == QEvent::TouchEnd)
		return true;
	else if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture)
	{
		//Tap Gesture- Equivalent to Upswipe
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* t = ge->gesture(Qt::TapGesture);
		if (t) {
			QTapGesture* tap = static_cast<QTapGesture*>(t);
			if (tap->state() == Qt::GestureFinished) {
				//Is the tap centered? If in portrait, check the x, otherwise check the y
				if ((m_isPortrait && tap->position().x() > m_bounds.width()/3 && tap->position().x() < (m_bounds.width()/3) * 2)
				|| (!m_isPortrait && tap->position().y() > m_bounds.width()/3 && tap->position().y() < (m_bounds.width()/3) * 2))
				{
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Home, Qt::NoModifier));
					animateBar(AnimDir(Up));
				}
			}
		}
		return true;
	}
	else if (event->type() == QEvent::GraphicsSceneMousePress)
	{
		//Record the starting finger position for later
		QGraphicsSceneMouseEvent* ev = static_cast<QGraphicsSceneMouseEvent*>(event);
		m_startPos = ev->pos().toPoint();
		return true;
	}
	else if (event->type() == QEvent::GraphicsSceneMouseMove)
	{
		QGraphicsSceneMouseEvent* ev = static_cast<QGraphicsSceneMouseEvent*>(event);
		QPoint delta = ev->pos().toPoint() - m_startPos;

		//Don't fire events more than once
		if(m_fired)
			return true;

		//Swipe up
		if(delta.y() < -GESTURE_AREA_TRIGGER_DIST)
		{
			//Card View/Toggle Launcher
			if (window) {
				QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Home, Qt::NoModifier));
			}
			animateBar(AnimDir(Up));
			m_fired = true;
			return true;
		}
		//Swipe left
		else if(delta.x() < -GESTURE_AREA_TRIGGER_DIST)
		{
			//From the center
			if(m_startPos.x() > -m_bounds.width()/4 && m_startPos.x() < m_bounds.width()/4)
			{
				//Back Gesture
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Back, Qt::NoModifier));
				}
				animateBar(AnimDir(Left));
				m_fired = true;
				return true;
			}

			//From the right 1/3rd
			if(m_startPos.x() > m_bounds.width()/4)
			{
				//Switch to Next App
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Next, Qt::NoModifier));
				}
				animateBar(AnimDir(Left));
				m_fired = true;
				return true;
			}
		}
		//Swipe right
		else if(delta.x() > GESTURE_AREA_TRIGGER_DIST)
		{
			//From the center
			if(m_startPos.x() > -m_bounds.width()/4 && m_startPos.x() < m_bounds.width()/4)
			{
				//Play Animation And Carry On
				//DEBUG: Orientation Testing
				if (window) {
					if(WindowServer::instance()->getUiOrientation() == OrientationEvent::Orientation_Up)
						QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Right));
					else if(WindowServer::instance()->getUiOrientation() == OrientationEvent::Orientation_Right)
						QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Down));
					else if(WindowServer::instance()->getUiOrientation() == OrientationEvent::Orientation_Down)
						QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Left));
					else if(WindowServer::instance()->getUiOrientation() == OrientationEvent::Orientation_Left)
						QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Up));
				}
				animateBar(AnimDir(Right));
				m_fired = true;
				return true;
			}

			//From the left 1/3rd
			if(m_startPos.x() < -m_bounds.width()/4)
			{
				//Switch to Previous App
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Previous, Qt::NoModifier));
				}
				animateBar(AnimDir(Right));
				m_fired = true;
				return true;
			}
		}
		return true;
	}
	else if (event->type() == QEvent::GraphicsSceneMouseRelease)
	{
		//Reset the fired check
		m_fired = false;

		return true;
	}

	return QGraphicsObject::sceneEvent(event);
}

void GestureArea::animateBar(int direction)
{
	m_focusAnimOut->stop();
	m_focusAnimOut->setStartValue(QPointF(0, m_lightbarY + (m_lightbarLPixmap->height()/2)));

	//Animate the gradient in the swiped direction
	if(direction == AnimDir(Up))
	{
		m_focusAnimOut->setStartValue(QPointF(0, m_lightbarY + (m_lightbarLPixmap->height()/2)));
		m_focusAnimOut->setEndValue(QPointF(m_gradientFocus.x(), m_lightbarY - m_bounds.width()/3));
	}
	else if(direction == AnimDir(Right))
	{
		m_focusAnimOut->setEndValue(QPointF(m_bounds.width()/3, m_lightbarY));
		m_animDir = AnimDir(Right);
	}
	else if(direction == AnimDir(Left))
	{
		m_focusAnimOut->setEndValue(QPointF(-m_bounds.width()/3, m_lightbarY));
		m_animDir = AnimDir(Left);
	}

	m_animDir = direction;

	m_focusAnimOut->start();
}

void GestureArea::focusAnimOutFinished()
{
	m_focusAnimIn->stop();

	//Slingshot the animation around from the other side
	if(m_animDir == AnimDir(Up))
	{
		m_focusAnimIn->setStartValue(QPointF(0, m_lightbarY + m_bounds.width()/3));
		m_focusAnimIn->setEndValue(QPointF(0, m_lightbarY + (m_lightbarLPixmap->height()/2)));
	}
	else if(m_animDir == AnimDir(Right))
	{
		m_focusAnimIn->setStartValue(QPointF(-m_bounds.width()/3, m_lightbarY));
		m_focusAnimIn->setEndValue(QPointF(0, m_lightbarY + (m_lightbarLPixmap->height()/2)));
	}
	else if(m_animDir == AnimDir(Left))
	{
		m_focusAnimIn->setStartValue(QPointF(m_bounds.width()/3, m_lightbarY));
		m_focusAnimIn->setEndValue(QPointF(0, m_lightbarY + (m_lightbarLPixmap->height()/2)));
	}

	m_focusAnimIn->start();
}
