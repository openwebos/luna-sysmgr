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



#include "Common.h"

#include <cjson/json.h>
#include <list>

#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QFinalState>
#include <QFont>
#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>
#include <QSharedData>
#include <QSignalTransition>
#include <QStateMachine>
#include <QString>
#include <QTimer>

#include <BannerMessageEvent.h>

#include "sptr.h"
#include "AnimationEquations.h"
#include "AnimationSettings.h"
#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "DisplayManager.h"
#include "HostBase.h"
#include "Logging.h"
#include "Preferences.h"
#include "Settings.h"
#include "SoundPlayerPool.h"
#include "SystemUiController.h"
#include "SystemService.h"
#include "Time.h"
#include "Utils.h"
#include "WebAppMgrProxy.h"
#include "WindowServer.h"
#include "QtUtils.h"

#include "BannerMessageHandler.h"

static const int kFontHeight = 16;
static const int kTextMarginX = 5;
static const int kMinShowTimeMs = 2000;
static const int kMaxShowTimeMs = 5000;

static const char* kDefaultNotificationSound = "/usr/palm/sounds/notification.wav";
static const char* kDefaultAlertSound = "/usr/palm/sounds/alert.wav";
static const char* kDefaultNotificationSoundName = "sysmgr_notification";
static const char* kDefaultAlertSoundName = "sysmgr_alert";

// ----------------------------------------------------------------------

class BannerMessage : public QObject
					, public QSharedData
{
	Q_OBJECT
	Q_PROPERTY(qreal posAnimProgress READ posAnimProgress WRITE setPosAnimProgress)
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
	
public:

	BannerMessage(const std::string& _appId,
				  const std::string& _msgId,
				  const std::string& _msg,
				  const std::string& _launchParams,
				  const QFont*       _font,
				  const std::string& _icon,
				  const std::string& _soundClass,
				  const std::string& _soundFile,
				  int _soundDuration,
				  bool _doNotSuppress)
		: appId(_appId)
		, msgId(_msgId)
		, launchParams(_launchParams)
		, icon(_icon)
		, soundClass(_soundClass)
		, soundFile(_soundFile)
		, soundDuration(_soundDuration)
		, doNotSuppress(_doNotSuppress)
		, fontMetrics(*_font)
		, msg(_msg.empty() ? QString() : QString::fromUtf8(_msg.c_str()))
		, elidedWidth(0)
		, posAnimProg(0.0)
		, alpha(1.0f)
		, triggered(false)
		, timerStartTime(0)
		, stateMachine(new QStateMachine(this))
		, timer(new QTimer(this))
		, showPosAnimation(new QPropertyAnimation(this, "posAnimProgress", this))
		, showOpacityAnimation(new QPropertyAnimation(this, "opacity", this))
		, hidePosAnimation(new QPropertyAnimation(this, "posAnimProgress", this))
		, hideOpacityAnimation(new QPropertyAnimation(this, "opacity", this))
	{
		msg = msg.simplified();

		const int kDuration = 1000;

		showPosAnimation->setDuration(kDuration);
		showOpacityAnimation->setDuration(kDuration);
		hidePosAnimation->setDuration(kDuration);
		hideOpacityAnimation->setDuration(kDuration);

		showOpacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
		showPosAnimation->setEasingCurve(QEasingCurve::OutCubic);
	}

	qreal posAnimProgress() const {
		return posAnimProg;
	}

	void setPosAnimProgress(qreal val) {
		posAnimProg = val;
	}

	qreal opacity() const {
		return alpha;
	}

	void setOpacity(qreal val) {
		alpha = val;
	}

	void createElidedMessage(int width) {
		elidedMsg = fontMetrics.elidedText(msg, Qt::ElideRight, width);
	}

	int elidedMsgWidth(int width) {

		if(elidedMsg.isEmpty())
			createElidedMessage(width);

		return fontMetrics.boundingRect(elidedMsg).width();
	}

	std::string appId;
	std::string msgId;
	std::string launchParams;
	std::string icon;
	std::string soundClass;
	std::string soundFile;
	int soundDuration;
	bool doNotSuppress;

	QFontMetrics fontMetrics;
	QString msg;
	int elidedWidth;
	QString elidedMsg;
	QPixmap pixmap;

	qreal posAnimProg;
	qreal alpha;
	bool triggered;
	uint32_t timerStartTime;
	
	QStateMachine* stateMachine;
	QTimer* timer;
	QPropertyAnimation* showPosAnimation;
	QPropertyAnimation* showOpacityAnimation;
	QPropertyAnimation* hidePosAnimation;
	QPropertyAnimation* hideOpacityAnimation;
};

typedef QExplicitlySharedDataPointer<BannerMessage> BMMessagePtr;
typedef QList<BMMessagePtr> BMMessagePtrList;
typedef QList<BannerMessageView*> BMViewList;

// ----------------------------------------------------------------------

class BannerMessageHandlerPriv
{
public:

	BannerMessageHandlerPriv(BannerMessageHandler* h)
	{
		viewHeight = Settings::LunaSettings()->positiveSpaceBottomPadding;

		if (!Settings::LunaSettings()->fontBanner.empty()) {
			QString fontName = qFromUtf8Stl(Settings::LunaSettings()->fontBanner);
			font = QFont(fontName);
			font.setPixelSize(kFontHeight);
		}
	}

	BMMessagePtrList msgList;
	BMMessagePtrList deletedMsgList;
	BMViewList viewList;

	int viewHeight;
	QFont font;
};

// ----------------------------------------------------------------------

BannerMessageHandler* BannerMessageHandler::instance()
{
	static BannerMessageHandler* s_instance = 0;
	if (G_UNLIKELY(s_instance == 0))
		s_instance = new BannerMessageHandler;

	return s_instance;
}

BannerMessageHandler::BannerMessageHandler()
	: d(new BannerMessageHandlerPriv(this))
{
}

BannerMessageHandler::~BannerMessageHandler()
{
	// Not Reached
	delete d;
}

void BannerMessageHandler::registerView(BannerMessageView* view)
{
	d->viewList.append(view);

	view->bmViewMessageCountUpdated(msgCount());
}

void BannerMessageHandler::unregisterView(BannerMessageView* view)
{
	d->viewList.removeAll(view);

	if (!d->viewList.empty())
		d->viewList.last()->bmViewMessageCountUpdated(msgCount());
}

qreal BannerMessageHandler::getViewContentWidth(const BannerMessageView* view)
{
	BMMessagePtr mp;
	qreal width = 0;
	qreal temp;
	qreal viewMaxWidth = view->bmViewGetWidth();

	Q_FOREACH(mp, d->deletedMsgList) {
		BannerMessage* m = mp.data();
		temp = m->elidedMsgWidth(viewMaxWidth) + m->pixmap.width() + 2 * kTextMarginX;
//		temp = ((qreal)(temp)) * m->posAnimProg;
		temp = ((qreal)(viewMaxWidth)) * m->posAnimProg;
		if(width < temp)
			width = temp;
	}

	Q_FOREACH(mp, d->msgList) {
		BannerMessage* m = mp.data();
		if (!m->triggered)
			continue;

		temp = m->elidedMsgWidth(viewMaxWidth) + m->pixmap.width() + 2 * kTextMarginX;
//		temp = ((qreal)(temp)) * m->posAnimProg;
		temp = ((qreal)(viewMaxWidth)) * m->posAnimProg;
		if(width < temp)
			width = temp;
	}

	return width;

}

void BannerMessageHandler::drawView(QPainter* painter, const BannerMessageView* view)
{
	QPen oldPen = painter->pen();
	painter->setPen(Qt::white);
	painter->setFont(d->font);


	BMMessagePtr mp;

	Q_FOREACH(mp, d->deletedMsgList) {
		paintBanner(painter, mp.data());
	}

	Q_FOREACH(mp, d->msgList) {

		BannerMessage* m = mp.data();
		if (!m->triggered)
			continue;

		paintBanner(painter, m);
	}

	painter->setOpacity(1.0f);
	painter->setPen(oldPen);
}

void BannerMessageHandler::paintBanner(QPainter* painter, BannerMessage* m)
{
	if (d->viewList.empty())
		return;

	
	BannerMessageView* view = d->viewList.last();
	
	if(view->scrollType() == BannerMessageView::VerticalScroll) {
		int width = view->bmViewGetWidth();

		int offsetX = kTextMarginX;
		int offsetY = ((1.0 - m->posAnimProgress()) * d->viewHeight) +  d->viewHeight/2;

		painter->setOpacity(m->opacity());

		if (!m->pixmap.isNull()) {
			int x = offsetX;
			int y = offsetY - m->pixmap.height() / 2;

			painter->drawPixmap(x, y, m->pixmap);

			offsetX += m->pixmap.width() + kTextMarginX;
		}

		painter->drawText(QRect(offsetX, ((1.0 - m->posAnimProgress()) * d->viewHeight),
								width - offsetX,
								d->viewHeight),
						  Qt::AlignLeft | Qt::AlignVCenter,
						  m->msg);
	} else if(view->scrollType() == BannerMessageView::HorizontalScroll) {
		int width = view->bmViewGetWidth();

		painter->setClipRect(0, 0, width, d->viewHeight);

		int offsetX = ((1.0 - m->posAnimProgress()) *  width) +  kTextMarginX;
		int offsetY = d->viewHeight/2;
		int maxTextWidth = width - kTextMarginX;

		painter->setOpacity(m->opacity());

		if (!m->pixmap.isNull()) {
			int x = offsetX;
			int y = offsetY - m->pixmap.height() / 2;

			painter->drawPixmap(x, y, m->pixmap);

			offsetX += m->pixmap.width() + kTextMarginX;
			maxTextWidth -= m->pixmap.width() + kTextMarginX;
		}

		painter->drawText(QRect(offsetX, 0,
								width - offsetX,
								d->viewHeight),
						  Qt::AlignLeft | Qt::AlignVCenter,
						  m->elidedMsg);
		painter->setClipping(false);

	} else if(view->scrollType() == BannerMessageView::StatusBarScroll) {
		// This scroll mode is for the Status Bar Notification area
		// it assumes that the painter (0,0) is the top Right corner when starting to draw
		// and it always draws starting on the contents left edge, since the scrolling of the
		// banner message is taken care of by the status bar code.
		int width = view->bmViewGetWidth();
		int bw = width; //m->elidedMsgWidth(width) + m->pixmap.width() + 2 * kTextMarginX;
		int translationX = -(m->posAnimProgress() *  bw);
		if(view->bmIsViewFullyExpanded())
			translationX = -(m->posAnimProgress() *  width);
		painter->translate(translationX, 0);

		painter->setClipRect(0, 0, (m->posAnimProgress() *  bw), d->viewHeight);

		int offsetX = kTextMarginX;
		int offsetY = d->viewHeight/2;
		int maxTextWidth = width - kTextMarginX;

		painter->setOpacity(m->opacity());

		if (!m->pixmap.isNull()) {
			int x = offsetX;
			int y = offsetY - m->pixmap.height() / 2;

			painter->drawPixmap(x, y, m->pixmap);

			offsetX += m->pixmap.width() + kTextMarginX;
			maxTextWidth -= m->pixmap.width() + kTextMarginX;
		}

		painter->drawText(QRect(offsetX, 0,
								width - offsetX,
								d->viewHeight),
						  Qt::AlignLeft | Qt::AlignVCenter,
						  m->elidedMsg);
		painter->setClipRect(QRect(), Qt::NoClip);
		painter->setClipping(false);
		painter->translate(m->posAnimProgress() *  bw, 0);

	} else if(view->scrollType() == BannerMessageView::NoScroll) {
		int width = view->bmViewGetWidth();

		int offsetX = kTextMarginX;
		int offsetY = d->viewHeight/2;

		painter->setOpacity(m->opacity());

		if (!m->pixmap.isNull()) {
			int x = offsetX;
			int y = offsetY - m->pixmap.height() / 2;

			painter->drawPixmap(x, y, m->pixmap);

			offsetX += m->pixmap.width() + kTextMarginX;
		}

		painter->drawText(QRect(offsetX, 0, width - offsetX, d->viewHeight),
						  Qt::AlignLeft | Qt::AlignVCenter,
						  m->elidedMsg);
	}
}

void BannerMessageHandler::addMessage(const std::string& appId,
									  const std::string& msgId,
									  const std::string& msg,
									  const std::string& launchParams,
									  const std::string& icon,
									  const std::string& soundClass,
									  const std::string& soundFile,
									  int duration,
									  bool doNotSuppress)
									  
{
	if (appId.empty() || msgId.empty() || msg.empty())
		return;

	g_message("BannerMessageHandler::addMessage: %s, %s", appId.c_str(), msgId.c_str());

	// Convert it back to string format in a format the JS side will understand
	std::string launchParamsSanitized;

	if (!launchParams.empty()) {
		json_object* json = json_tokener_parse(launchParams.c_str());
		if (json && !is_error(json)) {
			launchParamsSanitized =  json_object_to_json_string(json);
			json_object_put(json);
		}
		else {
			g_warning("BannerMessageHandler::addMessage: Launch params could not be parsed by cjson");
		}
	}

	BMMessagePtr m(new BannerMessage(appId, msgId, msg, launchParamsSanitized, &(d->font),
									 icon, soundClass, soundFile, duration,
									 doNotSuppress));
	d->msgList.append(m);

	int viewWidth = 0;

	if (!d->viewList.empty()) {
		BannerMessageView* view = d->viewList.last();
		viewWidth = view->bmViewGetWidth();

		view->bmViewMessageCountUpdated(msgCount());
	}

	if (d->msgList.size() == 1) {

		// First message queued up. Trigger it

		generateIcon(m.data());
		setupState(m.data(), viewWidth);

		SystemUiController::instance()->notifyBannerActivated();
	}
	else {

		// Adjust the timer interval of the current active message
		BMMessagePtr mp = d->msgList.first();
		// But only do so, if it is the max interval
		if (mp->timer->interval() == kMaxShowTimeMs) {

			if (mp->timer->isActive()) {
				
				uint32_t currTime = Time::curTimeMs();
				int remainingTime = kMinShowTimeMs - (currTime - mp->timerStartTime);
				if (remainingTime < 0)
					remainingTime = 0;

				mp->timer->setInterval(remainingTime);
			}
			else {

				// Not running yet, set straight to min show time
				mp->timer->setInterval(kMinShowTimeMs);
			}
		}
	}
	
}

void BannerMessageHandler::removeMessage(const std::string& appId,
										 const std::string& msgId)
{
	if (appId.empty() || msgId.empty())
		return;

	g_message("BannerMessageHandler::removeMessage: %s, %s", appId.c_str(), msgId.c_str());

	if (d->msgList.empty())
		return;

	// Is this a banner message being currently (or about to be) displayed
	BMMessagePtr m = d->msgList.first();
	if (m->appId == appId && m->msgId == msgId) {
		Q_EMIT signalHideBanner();
		return;
	}

	Q_FOREACH(m, d->msgList) {

		// Quiety erase in the background
		if (m->appId == appId && m->msgId == msgId) {
			d->msgList.removeAll(m);
			return;
		}
	}
}

void BannerMessageHandler::clearMessages(const std::string& appId)
{
	if (appId.empty())
		return;

	g_message("BannerMessageHandler::clearMessages: %s", appId.c_str());

	int index = 0;
	BMMessagePtrList::iterator it = d->msgList.begin();

	bool emitHide = false;
	
	while (it != d->msgList.end()) {

		index++;

		BMMessagePtr m = *it;
		if (m->appId == appId) {
			if (index == 1) {
				// Is this a banner message being currently (or about to be) displayed
				emitHide = true;
				++it;
				continue;
			}
			else {
				it = d->msgList.erase(it);
				continue;
			}
		}

		++it;
	}

	if (emitHide)
		Q_EMIT signalHideBanner();
}


void BannerMessageHandler::generateIcon(BannerMessage* m)
{
	QImage surf;
	const ApplicationDescription* appDesc = 0;

	std::string filePath = getResourcePathFromString(m->icon, m->appId,
													 Settings::LunaSettings()->lunaSystemResourcesPath);
	if (!filePath.empty()) {
		surf = QImage(filePath.c_str());
		if (!surf.isNull())
			goto Done;
	}

	// we will fallback to the app mini-icon
	appDesc = ApplicationManager::instance()->getAppById(m->appId);
	if (!appDesc)
		goto Done;

	if (!appDesc->miniIcon().isNull())
		surf = appDesc->miniIcon().toImage();

Done:

	if (!surf.isNull()) {
		int iconMaxSize = Settings::LunaSettings()->positiveSpaceBottomPadding - 2;
		if (surf.height() > iconMaxSize){
			surf = surf.scaledToHeight(iconMaxSize, Qt::SmoothTransformation);
		}
	}

	m->pixmap = QPixmap::fromImage(surf);
}

void BannerMessageHandler::setupState(BannerMessage* m, int viewWidth)
{
	QState* idleState = new QState();
	idleState->assignProperty(m, "posAnimProgress", 0.0); // $$$ d->viewHeight
	idleState->assignProperty(m, "opacity", 1.0f);

	QState* showState = new QState();
	showState->assignProperty(m, "posAnimProgress", 1.0); // $$$ 0
	showState->assignProperty(m, "opacity", 1.0f);

	QState* closeState = new QState();
	closeState->assignProperty(m, "posAnimProgress", 0.0); // $$$ d->viewHeight
	closeState->assignProperty(m, "opacity", 0.25f);

	QFinalState* finalState = new QFinalState();	

	int maxTextWidth = viewWidth - kTextMarginX;

	if (!m->pixmap.isNull()) {
		maxTextWidth -= m->pixmap.width() + kTextMarginX;
	}

	m->createElidedMessage(maxTextWidth);

	if (d->msgList.size() == 1)
		m->timer->setInterval(kMaxShowTimeMs);
	else
		m->timer->setInterval(kMinShowTimeMs);
	m->timer->setSingleShot(true);

	QObject::connect(showState, SIGNAL(entered()),
					 m->timer, SLOT(start()));
	QObject::connect(showState, SIGNAL(entered()),
					 this, SLOT(aboutToShowBanner()));
	QObject::connect(closeState, SIGNAL(entered()),
					 this, SLOT(aboutToHideBanner()));

	QObject::connect(m->showPosAnimation, SIGNAL(valueChanged(const QVariant&)),
					 SLOT(bannerPositionChanged()));
	QObject::connect(m->hidePosAnimation, SIGNAL(valueChanged(const QVariant&)),
					 SLOT(bannerPositionChanged()));

	QSignalTransition* idleToShowTrans;
	QSignalTransition* idleToCloseTrans;
	QSignalTransition* showToCloseTrans;
	QSignalTransition* closeToFinalTrans;

	idleToShowTrans = idleState->addTransition(idleState, SIGNAL(entered()), showState);
	idleToShowTrans->addAnimation(m->showPosAnimation);
	idleToShowTrans->addAnimation(m->showOpacityAnimation);

	idleToCloseTrans = idleState->addTransition(this, SIGNAL(signalHideBanner()), closeState);
	idleToCloseTrans->addAnimation(m->hidePosAnimation);
	idleToCloseTrans->addAnimation(m->hideOpacityAnimation);
	
	showToCloseTrans = showState->addTransition(m->timer, SIGNAL(timeout()), closeState);
	showToCloseTrans->addAnimation(m->hidePosAnimation);
	showToCloseTrans->addAnimation(m->hideOpacityAnimation);

	showToCloseTrans = showState->addTransition(this, SIGNAL(signalHideBanner()), closeState);
	showToCloseTrans->addAnimation(m->hidePosAnimation);
	showToCloseTrans->addAnimation(m->hideOpacityAnimation);

	closeToFinalTrans = closeState->addTransition(closeState, SIGNAL(propertiesAssigned()),
												  finalState);
	
	m->stateMachine->addState(idleState);
	m->stateMachine->addState(showState);
	m->stateMachine->addState(closeState);
	m->stateMachine->addState(finalState);
    m->stateMachine->setInitialState(idleState);

	connect(m->stateMachine, SIGNAL(finished()), SLOT(bannerStateMachineFinished()));

	
	m->stateMachine->start();

	m->triggered = true;
}

void BannerMessageHandler::playSound(BannerMessage* m)
{
	playSound(m->appId, m->soundClass, m->soundFile, m->soundDuration);
}

sptr<SoundPlayer> BannerMessageHandler::playSound(const std::string& appId, const std::string& soundClass,
												  const std::string& soundFile, int duration, bool loop,
												  bool wakeupScreen)
{
	if (soundClass.empty() && soundFile.empty())
		return 0;

	if (soundClass == "none")
		return 0;

	if (soundClass == "vibrate") {
		SystemService::instance()->vibrate("notifications");
		return 0;
	}

	std::string streamClass(soundClass);

	if (streamClass == "alert")
		streamClass = "alerts";
	else if (streamClass == "notification")
		streamClass = "notifications";
	else if (streamClass == "ringtone")
		streamClass = "ringtones";

	if (streamClass != "alerts" &&
		streamClass != "alarm" &&
		streamClass != "calendar" &&
		streamClass != "notifications" &&
		streamClass != "ringtones" &&
		streamClass != "feedback")
		streamClass = "notifications";

	std::string soundFilePath = getResourcePathFromString(soundFile, appId,
														  Settings::LunaSettings()->lunaSystemSoundsPath);
	if (soundFilePath.empty()) {
		if (streamClass == "ringtones")
			soundFilePath = Preferences::instance()->getCurrentRingtone();
		else if (streamClass == "alerts" ||
				 streamClass == "alarm"  ||
				 streamClass == "calendar")
			soundFilePath = Preferences::instance()->getCurrentAlerttone();
		else
			soundFilePath = Preferences::instance()->getCurrentNotificationtone();
	}

	if (soundFilePath.empty())
		soundFilePath = Settings::LunaSettings()->lunaSystemSoundsPath + "/" +
						Settings::LunaSettings()->lunaDefaultAlertSound;

	if (soundFilePath.empty())
		return 0;

	if (duration <= 0)
		duration = -1; // use the duration of the file

	// If stream class is of type notifications, then cap duration to maximum allowed by system
	if (streamClass == "notifications" && duration <= 0)
		duration = Settings::LunaSettings()->notificationSoundDuration;

	if (wakeupScreen)
		DisplayManager::instance()->alert(DISPLAY_ALERT_GENERIC_ACTIVATED);

	if (isDefaultNotificationSound(soundFilePath)) {
		g_message("Playing default notification sound: %s", kDefaultNotificationSoundName);
		SoundPlayerPool::instance()->playFeedback(kDefaultNotificationSoundName,
												  pulseAudioSinkName(streamClass));
		return 0;
	}
	else if (isDefaultAlertSound(soundFilePath)) {
		g_message("Playing default alert sound: %s", kDefaultAlertSoundName);
		SoundPlayerPool::instance()->playFeedback(kDefaultAlertSoundName,
												  pulseAudioSinkName(streamClass));
		return 0;
	}
	else {
		g_message("Playing custom sound: %s", soundFilePath.c_str());
		return SoundPlayerPool::instance()->play(soundFilePath, streamClass, loop, duration);
	}
}

int BannerMessageHandler::viewHeight() const
{
	return d->viewHeight;
}

void BannerMessageHandler::handleBannerMessageEvent(BannerMessageEvent* e)
{
	switch (e->msgType) {
	case (BannerMessageEvent::Add): {
		addMessage(e->appId, e->msgId,
				   e->msg, e->launchParams,
				   e->icon, e->soundClass,
				   e->soundFile, e->duration,
				   e->doNotSuppress);

		// We will wake up the display as well
		if (!SystemUiController::instance()->isScreenLocked() ||
			Preferences::instance()->showAlertsWhenLocked()) {
			DisplayManager::instance()->alert(DISPLAY_BANNER_ACTIVATED);
		}

		break;
	}
	case (BannerMessageEvent::Remove):
		removeMessage(e->appId, e->msgId);
		break;
	case (BannerMessageEvent::Clear):
		clearMessages(e->appId);
		break;
	case (BannerMessageEvent::PlaySound):
		playSound(e->appId, e->soundClass, e->soundFile, e->duration, false, e->wakeupScreen);
		break;
	default:
		break;
	}
}

void BannerMessageHandler::handleActiveCallBannerEvent(ActiveCallBannerEvent* e)
{
	if (d->viewList.empty())
		return;

	Q_FOREACH(BannerMessageView* view, d->viewList) {

		// inform all views
		switch (e->type) {
		case (ActiveCallBannerEvent::Add):
			view->bmViewAddActiveCallBanner(e->msg, e->icon, e->time);
			break;
		case (ActiveCallBannerEvent::Remove):
			view->bmViewRemoveActiveCallBanner();
			break;
		case (ActiveCallBannerEvent::Update):
			view->bmViewUpdateActiveCallBanner(e->msg, e->icon, e->time);
			break;
		default: 
			break;
		}
	}
}


void BannerMessageHandler::activateCurrentMessage()
{
	if (d->msgList.empty())
		return;

	BMMessagePtr msg = d->msgList.first();
	if (msg->launchParams.empty()) {
		g_message("BannerMessageHandler::activateCurrentMessage: Launch params empty. Not activating");
		return;
	}

	g_message("BannerMessageHandler::activateCurrentMessage: Activating banner. AppId: %s, MsgId: %s",
			  msg->appId.c_str(), msg->msgId.c_str());

	std::string errMsg;
	WebAppMgrProxy::instance()->appLaunch(msg->appId, msg->launchParams, "", "", errMsg);
}

int BannerMessageHandler::msgCount() const
{
	return d->msgList.size();
}

void BannerMessageHandler::aboutToShowBanner()
{
	luna_assert(d->msgList.empty() == false);

	BMMessagePtr m = d->msgList.first();
	playSound(m.data());

	// (Re) Adjust the timer interval
	if (d->msgList.size() == 1)
		m->timer->setInterval(kMaxShowTimeMs);
	else
		m->timer->setInterval(kMinShowTimeMs);

	// Timer has also been fired off
	m->timerStartTime = Time::curTimeMs();

	if (m->doNotSuppress) {
		if (!d->viewList.empty())
			d->viewList.last()->bmViewShowingNonSuppressibleMessage(true);
	}		
}

void BannerMessageHandler::aboutToHideBanner()
{
	luna_assert(d->msgList.empty() == false);

	BMMessagePtr m = d->msgList.first();
	d->msgList.removeFirst();
	d->deletedMsgList.append(m);	

	if (m->doNotSuppress) {
		if (!d->viewList.empty())
			d->viewList.last()->bmViewShowingNonSuppressibleMessage(false);
	}		
	
	if (d->msgList.empty()) {
		SystemUiController::instance()->notifyBannerDeactivated();
	}
}

void BannerMessageHandler::bannerPositionChanged()
{
    if (!d->viewList.empty())
		d->viewList.last()->bmViewUpdated();
}

void BannerMessageHandler::bannerStateMachineFinished()
{
	if (d->msgList.empty()) {
		if (!d->viewList.empty())
			d->viewList.last()->bmViewMessageCountUpdated(0);
	} else {
		BMMessagePtr m = d->msgList.first();
		generateIcon(m.data());
		if (!d->viewList.empty())
			setupState(m.data(), d->viewList.last()->bmViewGetWidth());
}
	
	QStateMachine* sm = qobject_cast<QStateMachine*>(sender());
	if (!sm) {
		g_warning("BannerMessageHandler::bannerStateMachineFinished: sender is not a state machine");
		return;
	}

	BMMessagePtr mp;
	Q_FOREACH(mp, d->deletedMsgList) {

		BannerMessage* m = mp.data();
		if (m->stateMachine == sm) {
			d->deletedMsgList.removeAll(mp);
			return;
		}
	}

	g_warning("BannerMessageHandler::bannerStateMachineFinished: Could not find message for which the state machine finished");
}
			
bool BannerMessageHandler::isDefaultNotificationSound(const std::string& filePath) const
{
	if (filePath == kDefaultNotificationSound)
		return true;

	struct stat stBuf1, stBuf2;

	if (::stat(kDefaultNotificationSound, &stBuf1) != 0)
		return false;

	if (::stat(filePath.c_str(), &stBuf2) != 0)
		return false;

	return ((stBuf1.st_dev == stBuf2.st_dev) &&
			(stBuf1.st_ino == stBuf2.st_ino));
}

bool BannerMessageHandler::isDefaultAlertSound(const std::string& filePath) const
{
	if (filePath == kDefaultAlertSound)
		return true;

	struct stat stBuf1, stBuf2;

	if (::stat(kDefaultAlertSound, &stBuf1) != 0)
		return false;

	if (::stat(filePath.c_str(), &stBuf2) != 0)
		return false;

	return ((stBuf1.st_dev == stBuf2.st_dev) &&
			(stBuf1.st_ino == stBuf2.st_ino));
}

std::string BannerMessageHandler::pulseAudioSinkName(const std::string& streamClass) const
{
	if (streamClass == "alerts")
		return "palerts";

	if (streamClass == "notifications")
		return "pnotifications";

	if (streamClass == "calendar")
		return "pcalendar";

	if (streamClass == "feedback")
		return "pfeedback";

	return "pnotifications";
}

#include "BannerMessageHandler.moc"
