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




#include "Common.h"

#include <math.h>
#include <QtDebug>

#include "Window.h"

#include "ApplicationManager.h"
#include "WindowServer.h"

Window::Window(Type type, const uint32_t bufWidth, const uint32_t bufHeight, bool hasAlpha)
	: m_type(type)
	, m_appDesc(0)
	, m_removed(false)
    , m_disableKeepAlive(false)
	, m_bufWidth(bufWidth)
	, m_bufHeight(bufHeight)
	, m_initialWidth(bufWidth)
	, m_initialHeight(bufHeight)
{
	setVisibleDimensions(bufWidth, bufHeight);

	if (type != Type_QtNativePaintWindow)
	{
		//launcher windows have no backing store. They're purely natively drawn
		m_screenPixmap = QPixmap(m_bufWidth, m_bufHeight);

#if defined(TARGET_DESKTOP) && defined(HAVE_OPENGL)
		hasAlpha = false;
#endif

		QColor fillColor(255, 255, 255, hasAlpha ? 0 : 255);
		m_screenPixmap.fill(fillColor);
	}
	WindowServer::registerWindow(this);
}

Window::Window(Type type, const QPixmap& pix)
	: m_type(type)
	, m_appDesc(0)
	, m_removed(false)
    , m_disableKeepAlive(false)
	, m_screenPixmap(pix)
	, m_bufWidth(pix.width())
	, m_bufHeight(pix.height())
	, m_initialWidth(pix.width())
	, m_initialHeight(pix.height())
{
	setVisibleDimensions(m_bufWidth, m_bufHeight);

	WindowServer::registerWindow(this);
}

Window::~Window()
{
	WindowServer::unregisterWindow(this);
}


void Window::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	//lock();

	if (m_type == Type_QtNativePaintWindow)
	{
		g_critical("%s: window of type QtNativePaintWindow attempting base-class paint()!",__PRETTY_FUNCTION__);
		return;
	}

	const QPixmap* pix = acquireScreenPixmap();
	if (!pix) {
		g_critical("Failed to acquire screen pixmap");
		return;
	}
	
	painter->drawPixmap(m_visibleBounds.x(), m_visibleBounds.y(), *pix,
						0, 0, m_visibleBounds.width(), m_visibleBounds.height());

	//unlock();
}

bool Window::mouseGrabbed() const
{
	return (scene() != NULL) && (scene()->mouseGrabberItem() == this);
}

void Window::setMouseGrabbed(bool grabbed)
{
	if (grabbed) {
		//qDebug() << "sm grabbing mouse";
		grabMouse();
	} else {
		//qDebug() << "sm ungrabbing mouse";
		ungrabMouse();
	}
}

void Window::setName(const std::string& name)
{
	m_name = name;
}

std::string Window::name() const
{
    return m_name;
}


void Window::setAppId(const std::string& id)
{
	m_appId = id;
}

std::string Window::appId() const
{
	return m_appId;
}

void Window::setProcessId(const std::string& id)
{
	m_processId = id;
}

std::string Window::processId() const
{
	return m_processId;
}

void Window::setLaunchingAppId(const std::string& id)
{
	m_launchingAppId = id;
}

std::string Window::launchingAppId() const
{
	return m_launchingAppId;
}

void Window::setLaunchingProcessId(const std::string& id)
{
    m_launchingProcId = id;
}

std::string Window::launchingProcessId() const
{
	return m_launchingProcId;
}

ApplicationDescription* Window::appDescription() const
{
	if (m_appDesc)
		return m_appDesc;

	m_appDesc = ApplicationManager::instance()->getAppById(m_appId);

	return m_appDesc;
}

// ------------------------------------------------------------------

void WindowProperties::merge(const WindowProperties& props)
{
    if (props.flags & isSetBlockScreenTimeout)
		setBlockScreenTimeout(props.isBlockScreenTimeout);

	if (props.flags & isSetSubtleLightbar)
		setSubtleLightbar(props.isSubtleLightbar);

	if (props.flags & isSetFullScreen)
		setFullScreen(props.fullScreen);

	if (props.flags & isSetActiveTouchpanel)
		setActiveTouchpanel(props.activeTouchpanel);

	if (props.flags & isSetAlsDisabled)
		setAlsDisabled(props.alsDisabled);

	if (props.flags & isSetOverlayNotifications)
		setOverlayNotificationsPosition(props.overlayNotificationsPosition);

	if (props.flags & isSetSuppressBannerMessages)
		setSuppressBannerMessages(props.suppressBannerMessages);

	if (props.flags & isSetHasPauseUi)
		setHasPauseUi(props.hasPauseUi);

	if (props.flags & isSetSuppressGestures)
		setSuppressGestures(props.suppressGestures);

	if (props.flags & isSetDashboardManualDragMode)
		setDashboardManualDragMode(props.dashboardManualDrag);

	if (props.flags & isSetStatusBarColor)
		setStatusBarColor(props.statusBarColor);

	if (props.flags & isSetRotationLockMaximized)
		setRotationLockMaximized(props.rotationLockMaximized);

	if (props.flags & isSetAllowResizeOnPositiveSpaceChange)
		setAllowResizeOnPositiveSpaceChange(props.allowResizeOnPositiveSpaceChange);

	if (props.flags & isSetGyro)
		setAllowGyroEvents(props.gyroEnabled);

	if (props.flags & isSetEnableCompassEvents)
		setCompassEvents(props.compassEnabled);
}

void WindowProperties::setOverlayNotificationsPosition(unsigned int position)
{
	flags |= isSetOverlayNotifications;
	if (position > OverlayNotificationsBottom && position < OverlayNotificationsLast)
		overlayNotificationsPosition = position;
	else
		overlayNotificationsPosition = OverlayNotificationsBottom;
}

void Window::setVisibleDimensions(uint32_t width, uint32_t height)
{
	if ((m_visibleBounds.width() == width && m_visibleBounds.height() == height) ||
		(height > m_bufHeight || width > m_bufWidth))
		return;

	prepareGeometryChange();
	m_visibleBounds.setRect(-(int)width/2, -(int)height/2, width, height);
}

QSize Window::getVisibleDimensions() const
{
	return m_visibleBounds.size().toSize();
}

void Window::resize(int w, int h)
{
	m_bufWidth = w;
	m_bufHeight = h;

	setVisibleDimensions(w, h);

	// preserve alpha
	bool hasAlpha = m_screenPixmap.hasAlpha();
	if (m_type != Type_QtNativePaintWindow)
	{
		m_screenPixmap = QPixmap(w, h);

		QColor fillColor(255, 255, 255, hasAlpha ? 0 : 255);
		m_screenPixmap.fill(fillColor);
	}
//	g_debug("%s: Window resized to %d x %d", __PRETTY_FUNCTION__,
//			m_bufWidth, m_bufHeight);
}

