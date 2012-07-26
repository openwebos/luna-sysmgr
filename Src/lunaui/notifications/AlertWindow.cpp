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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QGraphicsSceneMouseEvent>
#include <QUrl>

#include <PIpcBuffer.h>
#include <PIpcChannel.h>

#include "AlertWindow.h"

#include "ApplicationManager.h"
#include "ApplicationDescription.h"
#include "BannerMessageHandler.h"
#include "HostWindowData.h"
#include "Logging.h"
#include "PersistentWindowCache.h"
#include "Preferences.h"
#include "Settings.h"
#include "SoundPlayerPool.h"
#include "Utils.h"
#include "WebAppMgrProxy.h"
#include "WindowServer.h"
#include "IpcClientHost.h"
#include "Time.h"
#include "QtUtils.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

// app id's/dialog names for our special cases
static const std::string kPhoneAppId 		= "com.palm.app.phone";
static const std::string kIncomingCallAlert = "incoming";

AlertWindow::AlertWindow(Window::Type winType, int width, int height, bool hasAlpha)
	: HostWindow(winType, width, height, hasAlpha)
	, m_player(0)
	, m_playedSound(false)
	, m_isTransientAlert(false)
{
}

// for IPC windows
AlertWindow::AlertWindow(Window::Type winType, HostWindowData* data,
		                 IpcClientHost* clientHost)
	: HostWindow(winType, data, clientHost)
	, m_player(0)
	, m_playedSound(false)
	, m_isTransientAlert(false)
{
}

AlertWindow::~AlertWindow()
{
	hide();
}


void AlertWindow::onMessageReceived(const PIpcMessage& msg)
{
	bool msgIsOk;

	IPC_BEGIN_MESSAGE_MAP(AlertWindow, msg, msgIsOk)
		IPC_MESSAGE_HANDLER(ViewHost_Alert_SetSoundParams, setSoundParams)
		IPC_MESSAGE_HANDLER(ViewHost_Alert_SetContentRect, onSetContentRect)
		// not handled here, forward it to the base class
		IPC_MESSAGE_UNHANDLED( HostWindow::onMessageReceived(msg); )
	IPC_END_MESSAGE_MAP()
}

void AlertWindow::onSetContentRect(int left, int right, int top, int bottom)
{
	QRect bounds = boundingRect().toRect();
	QRect contentRect(left, top, right - left, bottom - top);


	lock();
	setContentRect(contentRect);
	unlock();
}

void AlertWindow::setSoundParams(const std::string& fileName, const std::string& soundClass)
{
	g_message("Set sound params: '%s', '%s'\n",
			  fileName.empty() ? "" : fileName.c_str(),
			  soundClass.empty() ? "" : soundClass.c_str());

	m_setFilePath = fileName;
	m_setSoundClass = soundClass;
}

void AlertWindow::activate()
{
	if (m_player.get() == 0 && !m_playedSound) {

		// reset file path in case the preference changed in the meantime
		// (happens if this is a persistent window)
		m_filePath = std::string();
		extractSoundParams();


		m_player = BannerMessageHandler::instance()->playSound(appId(),
															   m_soundClass,
															   m_filePath,
															   -1,
															   m_soundClass == "ringtones");

		// If this is a persistent window, we need to play everytime the window
		// is shown
		if (!PersistentWindowCache::instance()->shouldPersistWindow(this))
			m_playedSound = true;
	}

	if (m_channel)
		m_channel->sendAsyncMessage(new View_Focus(routingId(), true));
}

void AlertWindow::deactivate()
{
	if (m_player.get()) {

		SoundPlayerPool::instance()->stop(m_player);
		m_player = 0;
	}

	if (m_channel)
		m_channel->sendAsyncMessage(new View_Focus(routingId(), false));
}

static bool fileExists(const std::string& filePath) {

	if (filePath.empty())
		return false;

	struct stat stBuf;
	if (::stat(filePath.c_str(), &stBuf) != 0)
		return false;

	return S_ISREG(stBuf.st_mode);
}

void AlertWindow::extractSoundParams()
{
	m_soundClass = m_setSoundClass;
	if (m_soundClass == "none")
		return;

	if (!m_setFilePath.empty()) {

		if (strstr(m_setFilePath.c_str(), "://")) {
			QUrl url(qFromUtf8Stl(m_setFilePath));
			m_filePath = getResourcePathFromString(url.path().toStdString(), appId(), Settings::LunaSettings()->lunaSystemSoundsPath);
		}
		else {
			m_filePath = getResourcePathFromString(m_setFilePath, appId(),  Settings::LunaSettings()->lunaSystemSoundsPath);
		}
	}
	else {

		m_filePath = std::string();
	}

	// Workaround for app misspelling
	if (m_soundClass == "alert")
		m_soundClass = "alerts";
	else if (m_soundClass == "notification")
		m_soundClass = "notifications";
	else if (m_soundClass == "ringtone")
		m_soundClass = "ringtones";

	// set sound class to notifications if we didn't get one of ringtones or alerts or alarm
	if (m_soundClass != "ringtones" &&
		m_soundClass != "alerts" &&
		m_soundClass != "alarm" &&
		m_soundClass != "calendar")
		m_soundClass = "notifications";

	if (m_filePath.empty()) {

		std::string prefToneFilePath;

		if (m_soundClass == "ringtones")
			prefToneFilePath = Preferences::instance()->getCurrentRingtone();
		else if (m_soundClass == "alerts" ||
				 m_soundClass == "alarm"  ||
				 m_soundClass == "calendar")
			prefToneFilePath = Preferences::instance()->getCurrentAlerttone();
		else
			prefToneFilePath = Preferences::instance()->getCurrentNotificationtone();

		if (prefToneFilePath.length() > 0)
			m_filePath = prefToneFilePath;
	}

	if (fileExists(m_filePath))
		return;

	// if file doesn't exist, we will choose a default one based on the sound class
	if (m_soundClass == "ringtones")
		m_filePath = Settings::LunaSettings()->lunaSystemSoundsPath + "/" +
					 Settings::LunaSettings()->lunaDefaultRingtoneSound;
	else
		m_filePath = Settings::LunaSettings()->lunaSystemSoundsPath + "/" +
					 Settings::LunaSettings()->lunaDefaultAlertSound;
}

void AlertWindow::setContentRect(const QRect& r)
{
	m_contentRect = r;
}

QRect AlertWindow::contentRect() const
{
	return m_contentRect;
}

bool AlertWindow::isIncomingCallAlert() const
{
	if (appId() == kPhoneAppId) {

		std::string::size_type startPos = name().find(kIncomingCallAlert, 0);
		if (startPos == 0)
			return true;
	}

	return false;
}

void AlertWindow::onUpdateWindowRegion(int x, int y, int w, int h)
{
	HostWindow::onUpdateWindowRegion(x, y, w, h);
	WindowServer::instance()->windowUpdated(this);
}

void AlertWindow::onUpdateFullWindow()
{
	HostWindow::onUpdateFullWindow();
	WindowServer::instance()->windowUpdated(this);
}

void AlertWindow::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    sendInputEvent(event, Event::PenDown, 1);
}

void AlertWindow::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    sendInputEvent(event, Event::PenMove);
}

void AlertWindow::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    Event::Type type;
    if (!event->canceled())
        type = Event::PenUp;
    else
        type = Event::PenCancel;
    sendInputEvent(event, type);
}

void AlertWindow::sendInputEvent(QGraphicsSceneMouseEvent* event, Event::Type type, int clickCount)
{
    QPointF p = event->pos();
    p -= boundingRect().topLeft();

    Event evt;
    evt.type = type;
    evt.x = p.x();
    evt.y = p.y();
    evt.z = 0;
    evt.key = Event::Key_Null;
    evt.button = Event::Left;
    evt.modifiers = Event::modifiersFromQt(event->modifiers());
    evt.time = Time::curTimeMs();
    evt.clickCount = clickCount;

    WebAppMgrProxy::instance()->inputEvent(this, &evt);
}
