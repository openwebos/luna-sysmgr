/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#include "Common.h"

#include "DockModeWindow.h"

#include "AlertWindow.h"
#include "FlickGesture.h"
#include "Logging.h"
#include "MenuWindowManager.h"
#include "Settings.h"
#include "SystemUiController.h"
#include "WindowServer.h"
#include "WindowServerLuna.h"
#include "WindowManagerBase.h"
#include "ApplicationDescription.h"
#include "Time.h"
#include "IpcClientHost.h"
#include "DockModeLaunchPoint.h"
#include "DockModeWindowManager.h"
#include "CardDropShadowEffect.h"
#include "HostBase.h"
#include "DockModeWindow.h"
#include "AnimationSettings.h"

#include <SysMgrDefs.h>
#include <QGestureEvent>
#include "FlickGesture.h"
#include <QGraphicsDropShadowEffect>

#define DOCK_APP_LOADING_TIMEOUT   60000

unsigned int DockModeWindow::s_dockGlowRefCount = 0;
QPixmap* DockModeWindow::s_dockGlow = 0;

DockModeWindow::DockModeWindow(Window::Type type, HostWindowData* data, IpcClientHost* clientHost)
	: CardWindow(type, data, clientHost), m_pulseOpacity(0.0), m_nativeApp (false), m_loadingOverlayEnabled (true)
{
}

DockModeWindow::DockModeWindow(Window::Type type, const QPixmap& pixmap)
	: CardWindow(type, pixmap), m_pulseOpacity(0.0), m_nativeApp (true), m_loadingOverlayEnabled (true)
{
}

DockModeWindow::~DockModeWindow()
{
}

void DockModeWindow::setPrepareAddedToWindowManager() {
	m_prepareAddedToWm = true;
	if (G_LIKELY(s_dockGlow == 0)) {
		QString path(Settings::LunaSettings()->lunaSystemResourcesPath.c_str());
		path.append("/dockmode/dock-loading-glow.png");
		s_dockGlow = new QPixmap(path);
		if(s_dockGlow)
			s_dockGlowRefCount++;
		if (!s_dockGlow || s_dockGlow->isNull()) {
			g_critical("%s: Failed to load image '%s'", __PRETTY_FUNCTION__, qPrintable(path));
		}
	} else {
		s_dockGlowRefCount++;
	}

	ApplicationDescription* appDesc = static_cast<Window*>(this)->appDescription();
	int size = Settings::LunaSettings()->splashIconSize;
	m_icon.load(appDesc->splashIconName().c_str());
	if (!m_icon.isNull()) {
		// scale splash icon to fit the devices screen dimensions
		m_icon = m_icon.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}
	else {
		// just use the launcher icon
		m_icon = appDesc->getDefaultLaunchPoint()->icon();
		int newWidth = qMin((int)(m_icon.width()*1.5), size);
		int newHeight = qMin((int)(m_icon.height()*1.5), size);
		m_icon = m_icon.scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}
	

	// set up the pulsing animation
	QPropertyAnimation* pulseIn = new QPropertyAnimation(this, "pulseOpacity");
	pulseIn->setDuration(AS(cardLoadingPulseDuration));
	pulseIn->setEasingCurve(AS_CURVE(cardLoadingPulseCurve));
	pulseIn->setEndValue(1.0);

	QPropertyAnimation* pulseOut = new QPropertyAnimation(this, "pulseOpacity");
	pulseOut->setDuration(AS(cardLoadingPulseDuration));
	pulseOut->setEasingCurve(AS_CURVE(cardLoadingPulseCurve));
	pulseOut->setEndValue(0.0);

	QSequentialAnimationGroup* pulseGroup = new QSequentialAnimationGroup;
	pulseGroup->addAnimation(pulseIn);
	pulseGroup->addAnimation(pulseOut);
	pulseGroup->setLoopCount(-1);

	m_pulseAnimation.addPause(AS(cardLoadingTimeBeforeShowingPulsing));
	m_pulseAnimation.addAnimation(pulseGroup);

	m_loadingOverlayEnabled = true;
	m_pulseAnimation.start();

	update();
}

void DockModeWindow::setAddedToWindowManager() {
	m_addedToWm = true;
	m_pulseAnimation.stop();

	m_loadingOverlayEnabled = false;
	s_dockGlowRefCount--;
	if(!s_dockGlowRefCount) {
		delete s_dockGlow;
		s_dockGlow = 0;
	}
	update();
}


void DockModeWindow::setPulseOpacity(qreal opacity)
{
	m_pulseOpacity = opacity;
	update();
}


void DockModeWindow::setMaximized(bool enable)
{
	if(type() == Window::Type_DockModeWindow) {
		CardWindow::setMaximized(enable);
	} else {
		m_maximized = enable;		
	}
}


void DockModeWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setRenderHint(QPainter::SmoothPixmapTransform, !m_maximized);
	QPainter::CompositionMode previous = painter->compositionMode();
	painter->setCompositionMode(m_compMode);

	paintBase(painter, m_maximized);
	if (m_loadingOverlayEnabled) {
		bool maximized = qFuzzyCompare(scale(), (qreal)1.0) && qFuzzyCompare(x(), (qreal)0.0);

		painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
		QPainter::CompositionMode previous = painter->compositionMode();

		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter->fillRect(boundingRect(), QColor (0, 0, 0, 0xff));

		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

		// Draw the glow pulse
		painter->setBrushOrigin(boundingRect().x(), boundingRect().y());
		if (s_dockGlow) {
			painter->setOpacity(opacity() * pulseOpacity());
			painter->drawPixmap (boundingRect(), *s_dockGlow, (*s_dockGlow).rect());
			painter->setOpacity(opacity());
		}
		painter->setBrushOrigin(0, 0);

		// Draw the icon in front of the glow
		painter->drawPixmap(-m_icon.width()/2, -m_icon.height()/2, m_icon);

		painter->setCompositionMode(previous);
		painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	}

	paintOverlay(painter, m_maximized);

	painter->setCompositionMode(previous);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
}


