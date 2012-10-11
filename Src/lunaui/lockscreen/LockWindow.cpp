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

#include <string>
#include <cmath>
#include <map>

#include "LockWindow.h"
#include "Settings.h"
#include "HostBase.h"
#include "Localization.h"
#include "Preferences.h"

#include "WindowServerLuna.h"
#include "DashboardWindowManager.h"
#include "DashboardWindowContainer.h"
#include "DashboardWindow.h"
#include "Logging.h"
#include "MenuWindowManager.h"
#include "SystemUiController.h"
#include "SystemService.h"
#include "Security.h"
#include "EASPolicyManager.h"
#include "WebAppMgrProxy.h"
#include "AlertWindow.h"
#include "CardWindow.h"
#include "CoreNaviManager.h"
#include "Time.h"
#include "DisplayStates.h"
#include "DockModeWindowManager.h"
#include "BannerMessageHandler.h"
#include "AnimationSettings.h"
#include "GraphicsItemContainer.h"
#include "Utils.h"
#include "QtUtils.h"
#include "ClockWindow.h"
#include "IMEController.h"
#include "QmlInputItem.h"

#include <QPropertyAnimation>
#include <QTextLayout>
#include <QVector>
#include <QHash>
#include <QGraphicsPixmapItem>
#include <QPainterPath>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>


// includes for the 9tile
#include "pixmaploader.h"
#include "pixmap9tileobject.h"

#define USE_CLIP_PATH // used only for dashboards, popups and banners have padding around them.

#define TOP_LEFT_CORNER 0
#define TOP_RIGHT_CORNER 1
#define BOTTOM_LEFT_CORNER 2
#define BOTTOM_RIGHT_CORNER 3

#define LOCK_BUTTON_OFFSET (0.1)

static unsigned int kTopPadding = 0;
static unsigned int kMaxWidth = 320; // From Larry

static const unsigned int kAlertsFromBottom = 84;
static unsigned int kBannerWidgetHeight = 0;

static const int kSaucerRadiusSquared = 146 * 146;

static const unsigned int kHideHelpTimeoutInMS = 1000;
static const unsigned int kPaintTick = 1000;

// strings used as keys to lookup locale versions
static const char* kUnlockLabel = "Drag up to unlock";
static const char* kAnswerPhoneLabel = "Drag up to answer";

static DashboardWindowManager* getDashboardWindowManager();

static	Pixmap9TileObject* gBackground9Tile;
static unsigned int kShadowWidth = 10;
static unsigned int kBackgroundCornerWidth = 9;


class LockButton : public QGraphicsPixmapItem
{
public:
	LockButton();
	virtual ~LockButton();

	enum Type {
		ImagePadlock = 0,
		ImageIncomingCall = 2
	};

	void press(bool down);
	void setAnchor(int x, int y);
	int distanceToAnchorSquared(int x, int y);
	bool atRest() const;
	void reset();
	bool contains(int x, int y) const;
	void setImageType(Type type);

	bool m_pressed;

private:

	Type m_imageType;
	QVector<QPixmap> m_buttonImages;
	int m_anchorX;
	int m_anchorY;
	QRect m_hitTarget;
};

// ----------------------------------------------------------------------------------------------

class TransparentNode : public QGraphicsObject
{
	Q_OBJECT

public:
	TransparentNode(bool _stayVisible = false);
	bool stayVisible() const { return m_stayVisible; }
	void animateOpacity(qreal newOpacity);

private Q_SLOTS:
	void fadeAnimationFinished();

private:
	bool m_stayVisible;
	QPropertyAnimation m_fadeAnimation;
};

// ------------------------------------------------------------------------------------------------

class HelpWindow : public TransparentNode
{
public:
	HelpWindow();
	virtual ~HelpWindow();

	// QGraphicsItem::boundingRect
	QRectF boundingRect() const;

	// QGraphicsItem::paint
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void setLabel(QString label);

private:

	std::vector<gunichar2> m_labelUtf16;
	std::vector<int> m_labelGlyphOffsets;
	unsigned int m_splitIndex;
	QFont* m_font;
	QPixmap* m_surf;
	QTextLayout textLayout;

	QRect m_bounds;
};

// -----------------------------------------------------------------------------------------------

class LockBackground : public QGraphicsObject
{
	Q_OBJECT

public:
	LockBackground();
	virtual ~LockBackground();

	// QGraphicsItem::boundingRect
	QRectF boundingRect() const;

	// QGraphicsItem::paint
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void showBackground(bool visible);
	bool isBackgroundShown() { return m_showBackground; }

	void setCompositionMode(QPainter::CompositionMode mode);

	void resize(int width, int height);

	void setStatusBar(StatusBar* ptr) { m_statusBarPtr = ptr; }

private Q_SLOTS:

	void slotWallpaperImageChanged(QPixmap* wallpaper, bool fullScreen, int rotationAngle);

private:

	StatusBar* m_statusBarPtr;

	bool m_wallpaperFullScreen;
	QPixmap* m_wallpaperPtr;
	int m_wallpaperRotation;
	QPixmap m_topMask, m_bottomMask;
	QRect m_bounds;
	bool m_showBackground;
	QPainter::CompositionMode m_compMode;
};

// ------------------------------------------------------------------------------------------------

class DashboardAlerts : public TransparentNode
{
public:
	DashboardAlerts();
	virtual ~DashboardAlerts();

	// QGraphicsItem::boundingRect
	QRectF boundingRect() const;

	// QGraphicsItem::paint
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void sendClickAt(int x, int y);

private:
	QList<DashboardWindow*> getDashboardWindows() const;

	QRect m_bounds;

	QPixmap m_scrollFade;
	QPixmap m_divider;

	int kDashboardItemHeight;
	int kDashboardWidgetHeight;
	int kMaxDashboardItems;
	int kBottomPadding;
	int kTopPadding;

	float kVisibleDashboard;
	friend class LockWindow;
};

// ------------------------------------------------------------------------------------------------

class BannerAlerts : public TransparentNode,
                     public BannerMessageView
{
public:
	BannerAlerts();
	virtual ~BannerAlerts();

	// QGraphicsItem::boundingRect
	QRectF boundingRect() const;

	// QGraphicsItem::paint
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	
	int  bmViewGetWidth() const;
	void bmViewUpdated();
	void bmViewMessageCountUpdated(int count);
	void bmViewShowingNonSuppressibleMessage(bool val);

	void bmViewAddActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time);
	void bmViewRemoveActiveCallBanner();
	void bmViewUpdateActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time);

	QRect m_bounds;
	int m_msgCount;
	int kPadding;

};

// ------------------------------------------------------------------------------------------------

class PopUpAlert : public TransparentNode
{
public:
	PopUpAlert();
	virtual ~PopUpAlert();

	// QGraphicsItem::boundingRect
	QRectF boundingRect() const;

	// QGraphicsItem::paint
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	bool hasDisplayableContent() const;
	bool hasIncomingCallAlert() const;

	void adjustAlertBounds();

private:
	const Window* getActiveAlertWin() const;

	QRect m_bounds;
	int kPadding;
};

// ------------------------------------------------------------------------------------------------


LockWindow::LockWindow(uint32_t maxWidth, uint32_t maxHeight)
	: m_paintTimer(HostBase::instance()->masterTimer(), this, &LockWindow::paint)
	, m_hideHelpTimer(HostBase::instance()->masterTimer(), this, &LockWindow::hideHelpTimeout)
	, m_lastLocked(0)
	, m_lockTimeout(0)
	, m_popupActive(false)
	, m_bannerActive(false)
	, m_activeCall(false)
	, m_state(StateUnlocked)
	, m_lockButtonX(0)
	, m_lockButtonY(0)
	, m_bgNode(0)
	, m_helpWin(0)
	, m_lockButton(0)
	, m_clockWin(0)
	, m_dashboardAlerts(0)
	, m_bannerAlerts(0)
	, m_popUpAlert(0)
//	, m_phoneAppWin(0)
	, m_qmlUnlockPanel(0)
	, bannerViewRegistered(false)
//	, m_phoneAppShownY(0)
//	, m_phoneAppHiddenY(0)
	, m_elementsShown(true)
	, m_statusBar(0)
	, m_unlockPanel(0)
	, m_unlockDialog(0)
	, m_setupNewPin(false)
	, m_setupNewPassword(false)

{
	m_bounds = QRectF((int)-maxWidth/2, (int)-maxHeight/2, maxWidth, maxHeight);
	setOpacity(0.0);
	setVisible(false);
	
	connect(SystemUiController::instance(), SIGNAL(signalAlertActivated()), this, SLOT(slotAlertActivated()));
	connect(SystemUiController::instance(), SIGNAL(signalAlertDeactivated()), this, SLOT(slotAlertDeactivated()));
	connect(SystemUiController::instance(), SIGNAL(signalTransientAlertActivated()), this, SLOT(slotTransientAlertActivated()));
	connect(SystemUiController::instance(), SIGNAL(signalTransientAlertDeactivated()), this, SLOT(slotTransientAlertDeactivated()));
	connect(SystemUiController::instance(), SIGNAL(signalBannerActivated()), this, SLOT(slotBannerActivated()));
	connect(SystemUiController::instance(), SIGNAL(signalBannerDeactivated()), this, SLOT(slotBannerDeactivated()));

	connect(SystemUiController::instance(), SIGNAL(signalBannerAboutToUpdate(QRect&)), this, SLOT(slotBannerAboutToUpdate(QRect&)));
	connect(SystemUiController::instance(), SIGNAL(signalBootFinished()), this, SLOT(slotBootFinished()));

//	connect(SystemService::instance(), SIGNAL(signalDeviceUnlocked()), this, SLOT(slotDeviceUnlocked()));
//	connect(SystemService::instance(), SIGNAL(signalCancelPinEntry()), this, SLOT(slotCancelPasswordEntry()));

	connect(Preferences::instance(), SIGNAL(signalSetLockTimeout(uint32_t)), this, SLOT(slotSetLockTimeout(uint32_t)));

	connect(WindowServer::instance(), SIGNAL(signalWindowUpdated(Window*)), this, SLOT(slotWindowUpdated(Window*)));

	connect(this, SIGNAL(visibleChanged()), SLOT(slotVisibilityChanged()));

	setLockTimeout(Preferences::instance()->lockTimeout());
}

LockWindow::~LockWindow()
{
}

void LockWindow::init()
{
	connect(EASPolicyManager::instance(), SIGNAL(signalPolicyChanged(const EASPolicy* const)),
			this, SLOT(slotPolicyChanged(const EASPolicy* const)));

	connect(DisplayManager::instance(), SIGNAL(signalDisplayStateChange(int)), this, SLOT(slotDisplayStateChanged(int)));
    connect(DisplayManager::instance(), SIGNAL(signalLockStateChange(int, int)), this, SLOT(slotLockStateChanged(int, int)));
	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceAboutToChange(const QRect&, bool, bool)), 
											SLOT(slotPositiveSpaceAboutToChange(const QRect&, bool, bool)));
	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChanged(const QRect&)), 
											SLOT(slotPositiveSpaceChanged(const QRect&)));
	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChangeFinished(const QRect&)), 
											SLOT(slotPositiveSpaceChangeFinished(const QRect&)));

	connect(SystemUiController::instance(), SIGNAL(signalUiRotationCompleted()),
											SLOT(slotUiRotationCompleted()));

	Settings* settings = Settings::LunaSettings();

	m_windowFadeAnimation.setTargetObject(this);
	m_windowFadeAnimation.setPropertyName("opacity");
	m_windowFadeAnimation.setDuration(AS(lockWindowFadeDuration));
	m_windowFadeAnimation.setEasingCurve(AS_CURVE(lockWindowFadeCurve));

	connect(&m_windowFadeAnimation, SIGNAL(finished()), SLOT(slotWindowFadeAnimationFinished()));

	kTopPadding = settings->positiveSpaceTopPadding;
	kBannerWidgetHeight = BannerMessageHandler::instance()->viewHeight();
	if (kBannerWidgetHeight < 20) // DFISH-5147: Total banner height >= 40, shadows are 10 each, so widget height >= 20
		kBannerWidgetHeight = 20;
	
	m_statusBar = new StatusBar(StatusBar::TypeLockScreen, m_bounds.width(), settings->positiveSpaceTopPadding);
	m_statusBar->init();
	m_statusBar->setParentItem(this);
	m_statusBar->setPos(0, m_bounds.y() + m_statusBar->boundingRect().height() / 2);
	m_statusBar->setZValue(100);

	// Background (Status/Wallpaper/Dashboard/Banner/Mask)
	m_bgNode = new LockBackground();
	m_bgNode->setParentItem(this);
	m_bgNode->setPos(0,0);
	m_bgNode->setStatusBar(m_statusBar);

	// Clock
	m_clockWin = new ClockWindow();
	m_clockWin->setParentItem(this);
	m_clockWin->setPos(0,-(SystemUiController::instance()->currentUiHeight() * 0.35));
	m_clockWin->tick();

	// Dashboard Alerts
	m_dashboardAlerts = new DashboardAlerts();
	m_dashboardAlerts->setParentItem(this);
	m_dashboardAlerts->setPos(0, 0);

	// BannerAlerts
	m_bannerAlerts = new BannerAlerts();
	m_bannerAlerts->setParentItem(this);
	m_bannerAlerts->setPos(0, 0);

	// Popup Alert
	m_popUpAlert = new PopUpAlert();
	m_popUpAlert->setParentItem(this);
	m_popUpAlert->setPos(0, 0);

	// Help
	m_helpWin = new HelpWindow();
	m_helpWin->setLabel(fromStdUtf8(LOCALIZED(kUnlockLabel)));
	m_helpWin->setParentItem(this);
	m_helpWin->setPos(0, boundingRect().bottom() - boundingRect().height() * LOCK_BUTTON_OFFSET - (m_helpWin->boundingRect().height()) / 2);

	// Lock Button
	m_lockButton = new LockButton();
	m_lockButtonX = 0;
	m_lockButtonY = boundingRect().bottom() - boundingRect().height()* LOCK_BUTTON_OFFSET - m_lockButton->boundingRect().height()/2;
	m_lockButton->setAnchor(m_lockButtonX, m_lockButtonY);

	m_lockButton->setParentItem(this);

	std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/popup-bg.png";
	gBackground9Tile = PixmapObjectLoader::instance()->quickLoadNineTiled (QString (filePath.c_str()), 
			(quint32)(kShadowWidth + kBackgroundCornerWidth),
			(quint32)(kShadowWidth + kBackgroundCornerWidth),
			(quint32)(kShadowWidth + kBackgroundCornerWidth),
			(quint32)(kShadowWidth + kBackgroundCornerWidth));

	// Unlock Dialog
	QDeclarativeEngine* qmlEngine = WindowServer::instance()->declarativeEngine();
	if(qmlEngine) {
		QDeclarativeContext* context =	qmlEngine->rootContext();

		std::string qmlPath = settings->lunaQmlUiComponentsPath + "UnlockPanel/UnlockPanel.qml";
		QUrl url = QUrl::fromLocalFile(qmlPath.c_str());
        qmlRegisterType<InputItem>("CustomComponents", 1, 0, "InputItem");
		m_qmlUnlockPanel = new QDeclarativeComponent(qmlEngine, url, this);
		if(m_qmlUnlockPanel) {
			m_unlockPanel = qobject_cast<InputItem *>(m_qmlUnlockPanel->create());
			if(m_unlockPanel) {
				m_unlockPanel->setPos (-m_unlockPanel->boundingRect().width()/2, -m_unlockPanel->boundingRect().height()/2);
				static_cast<QGraphicsObject*>(m_unlockPanel)->setParentItem(this);
				m_unlockPanel->setVisible(false);
				m_unlockPanel->setOpacity(0.0);

				connect(m_unlockPanel, SIGNAL(entryCanceled()), SLOT(slotCancelPasswordEntry()));
				connect(m_unlockPanel, SIGNAL(passwordSubmitted(QString, bool)), SLOT(slotPasswordSubmitted(QString, bool)));
				connect(m_unlockPanel, SIGNAL(requestFocusChange(bool)), SLOT(slotPinPanelFocusRequest(bool)));
			}
		}

		qmlPath = settings->lunaQmlUiComponentsPath + "MessageDialog/MessageDialog.qml";
		url = QUrl::fromLocalFile(qmlPath.c_str());
		m_qmlUnlockDialog = new QDeclarativeComponent(qmlEngine, url, this);
		if(m_qmlUnlockDialog) {
			m_unlockDialog = qobject_cast<QGraphicsObject *>(m_qmlUnlockDialog->create());
			if(m_unlockPanel) {
				m_unlockDialog->setPos (-m_unlockDialog->boundingRect().width()/2, -m_unlockDialog->boundingRect().height()/2);
				m_unlockDialog->setParentItem(this);
				m_unlockDialog->setVisible(false);
				m_unlockDialog->setOpacity(0.0);

				connect(m_unlockDialog, SIGNAL(button1Pressed()), SLOT(slotDialogButton1Pressed()));
				connect(m_unlockDialog, SIGNAL(button2Pressed()), SLOT(slotDialogButton2Pressed()));
				connect(m_unlockDialog, SIGNAL(button3Pressed()), SLOT(slotDialogButton3Pressed()));
			}
		}
	}

	m_newPasscode.clear();
}

bool LockWindow::okToResize()
{
	if(m_lockButton) {
		if(!m_lockButton->atRest() || m_lockButton->m_pressed)
			return false;
	}

	if(m_windowFadeAnimation.state() == QAbstractAnimation::Running)
		return false;
//	if(m_phoneWinAnimation.state() == QAbstractAnimation::Running)
//		return false;

	return true;
}

void LockWindow::resize(int width, int height)
{
	m_bounds = QRectF((int)-width/2, (int)-height/2, width, height);

	if(m_bgNode) {
		m_bgNode->resize(width,height);
	}

	if(m_statusBar) {
		m_statusBar->resize(width, Settings::LunaSettings()->positiveSpaceTopPadding);
		m_statusBar->setPos(0, m_bounds.y() + m_statusBar->boundingRect().height() / 2);
		m_statusBar->update();
	}

	if(m_clockWin) {
		m_clockWin->resize(width, height);
		m_clockWin->setPos(0,-(height * 0.35));
		m_clockWin->tick();
	}

	if(m_dashboardAlerts)
		m_dashboardAlerts->setPos(0,0); 

	if(m_bannerAlerts)
		m_bannerAlerts->setPos(0, 0);

	if(m_popUpAlert)
		m_popUpAlert->setPos(0,0); 

	if(m_helpWin)
		m_helpWin->setPos(0, boundingRect().bottom() - boundingRect().height() * LOCK_BUTTON_OFFSET - (m_helpWin->boundingRect().height()) / 2);

	if(m_lockButton) {
		m_lockButtonY = boundingRect().bottom() - boundingRect().height() * LOCK_BUTTON_OFFSET - m_lockButton->boundingRect().height()/2;
		m_lockButton->setAnchor(m_lockButtonX, m_lockButtonY);
	}

//	if(m_phoneAppWin) {
//		bool phoneAppWinShown = false;
//
//		if(m_state == StateInPhoneCall) {
//			phoneAppWinShown = true;
//		}
//
//		m_phoneAppWin->resizeWindowBufferEvent(width, height, m_bounds.adjusted(0, kTopPadding, 0, 0).toRect());
//
//		m_phoneAppShownY  = boundingRect().y() + kTopPadding + (height - Settings::LunaSettings()->positiveSpaceTopPadding)/2;
//		m_phoneAppHiddenY = boundingRect().height() + (height - Settings::LunaSettings()->positiveSpaceTopPadding)/2;
//
//		if(pinWinShown) {
//			 m_phoneAppWin->setPos(0, m_phoneAppShownY);
//		} else {
//			 m_phoneAppWin->setPos(0, m_phoneAppHiddenY);
//		}
//	}

	if(isVisible())
		update();
}

void LockWindow::slotUiRotationCompleted()
{
	if(m_state == StatePinEntry) {
		static_cast<QGraphicsObject*>(m_unlockPanel)->setFocus(); // make sure the PIN [anel has focus when in PIN entry mode
	}
}


bool LockWindow::setLockTimeout(uint32_t timeout)
{
#if !defined(TARGET_DEVICE)
	return false;
#endif

	const EASPolicy * const policy = EASPolicyManager::instance()->getPolicy();
	if (policy == 0 || !policy->validInactivityInSeconds()) {

		// make sure we are setting a system supported timeout value
		m_lockTimeout = Preferences::roundLockTimeout(timeout);
	}
	else {
		m_lockTimeout = policy->clampInactivityInSeconds(timeout);
	}
	Preferences::instance()->setLockTimeout(m_lockTimeout);
	return true;
}

void LockWindow::slotSetLockTimeout(uint32_t timeout)
{
	setLockTimeout(timeout);
}

void LockWindow::slotPolicyChanged(const EASPolicy * const policy)
{
	// re-clamp our lock timeout value if needed
	setLockTimeout(m_lockTimeout);
}

void LockWindow::addPhoneAppWindow(Window* win)
{
	// not supporting Phone App card on Lock screen in Dartfish, so just clsoe this card
	static_cast<CardWindow*>(win)->close();

//	if (m_phoneAppWin != 0) {
//		g_warning("%s: Phone application tried to add more than 1 PIN window!", __PRETTY_FUNCTION__);
//		WebAppMgrProxy::instance()->closeWindow(win);
//		return;
//	}
//
//	m_phoneAppWin = static_cast<CardWindow*>(win);
//	m_phoneAppWin->setParentItem(this);
//	m_phoneAppWin->stackBefore(m_lockButton);
//	// set the correct screen position for the pin card
//	m_phoneAppWin->setPos(0,boundingRect().height() + m_phoneAppWin->boundingRect().height()/2);

//	m_phoneWinAnimation.setTargetObject(m_phoneAppWin);
//	m_phoneWinAnimation.setPropertyName("y");
//	m_phoneWinAnimation.setDuration(AS(lockPinDuration));
//	m_phoneWinAnimation.setEasingCurve(AS_CURVE(lockPinCurve));
//	connect(&m_phoneWinAnimation, SIGNAL(finished()), SLOT(slotPhoneWindowAnimationFinished()));
//
//	m_phoneAppShownY  = boundingRect().y() + kTopPadding + m_phoneAppWin->boundingRect().height()/2;
//	m_phoneAppHiddenY = boundingRect().height() + m_phoneAppWin->boundingRect().height()/2;
//
//	g_message("%s: PIN/password application was added to the lock screen", __PRETTY_FUNCTION__);
}

void LockWindow::slotBootFinished()
{
	// lock automatically on boot if the user has a passcode set
	if (requiresPasscode()) {
		DisplayManager::instance()->lock();
		m_lastLocked = 0;
	}

#if !defined(TARGET_DESKTOP)
	// If this fails, either the Phone application stopped early due to an uncaught exception before
	// the PIN window could be created or someone tried to subvert the locking mechanism
//	if (m_phoneAppWin == 0) {
//		g_critical("%s: (FATAL) The PIN/password unlock window was not received from the Phone app!", __PRETTY_FUNCTION__);
//		exit(-1);
//	}
#endif
}

void LockWindow::slotWindowUpdated(Window* win)
{
	if (!isLocked() || !win)
		return;

	switch (win->type()) {
	case Window::Type_PopupAlert:
	case Window::Type_BannerAlert:
		{
			AlertWindow* alert = (AlertWindow*)win;
			if(!alert->isTransientAlert()) {
				if (m_popUpAlert) {
					m_popUpAlert->adjustAlertBounds();
				}
			} else {
				// update the Transient alert contents here
			}
		}
		break;
	default: break;
	}
}

void LockWindow::delayDockModeLocking()
{
	m_delayDockModeLocking = true;
}

void LockWindow::performDelayedLock()
{
	if(m_delayDockModeLocking) {
        g_message("%s: calling lock (NIGHTSTAND) (DELAYED CALL DUE TO DOCK MODE)", __PRETTY_FUNCTION__);

		changeState(StateDockMode);
		unregisterBannerView();
		m_delayDockModeLocking = false;
	}
}

void LockWindow::slotLockStateChanged(int event, int displayEvent)
{
    switch (event) {
	case DISPLAY_LOCK_SCREEN:
		g_message("%s: calling lock (NORMAL)", __PRETTY_FUNCTION__);

		changeState(StateNormal);
		registerBannerView();
		break;
	case DISPLAY_UNLOCK_SCREEN:
		g_message("%s: calling tryUnlock with event %d", __PRETTY_FUNCTION__, displayEvent);

		unregisterBannerView();

		if (!isLocked())
			return;

		if (displayEvent == DisplayEventIncomingCall) {
			DisplayManager::instance()->lock();
		}
		else if (requiresPasscode() && displayEvent == DisplayEventApiOn) {
			DisplayManager::instance()->lock();
		}
		else if (!requiresPasscode() ||
				 ((displayEvent != DisplayEventUsbIn &&
				   displayEvent != DisplayEventOnPuck &&
				   displayEvent != DisplayEventProximityOff) ||
				   !m_popUpAlert->hasIncomingCallAlert()))
		{
			// attempt to unlock the screen if:
			// 	- the user has no pin/password set and there isn't a pending EAS policy update
			// 	- the reason for unlocking is not due to the USB being plugged in or the device
			// 		being removed from the puck	unless, there isn't a pending incoming call
			tryUnlock();
		}
		break;
	case DISPLAY_DOCK_SCREEN:
		g_message("%s: calling lock (DOCK)", __PRETTY_FUNCTION__);

		unregisterBannerView();
		changeState(StateDockMode);
		break;
	default: break;
	}
}

void LockWindow::slotDisplayStateChanged(int event)
{
	switch (event) {
	case DISPLAY_SIGNAL_ON:
	case DISPLAY_SIGNAL_DIM:
		// screen is about to come on
		g_debug("%s: power on/dim (%d)", __PRETTY_FUNCTION__, isLocked());
		handlePowerOn();
		break;
	case DISPLAY_SIGNAL_OFF:
		// screen has turned off
		g_debug ("%s: power off (%d)", __PRETTY_FUNCTION__, isLocked());
		handlePowerOff();
		break;
	default: break;
	}
}

void LockWindow::handlePowerOn()
{
	if (!isLocked())
		return;

	// start up the paint timer
	m_clockWin->tick();
	if (!m_paintTimer.running())
		m_paintTimer.start(kPaintTick);
}

void LockWindow::handlePowerOff()
{
	if (!isLocked())
		return;

	m_paintTimer.stop(); // no need to update while off
}

void LockWindow::activatePopUpAlert()
{
	// start showing the active popup alert
	hideAlert(m_dashboardAlerts);
	hideAlert(m_bannerAlerts);

	if (m_popUpAlert->hasIncomingCallAlert()) {
		m_lockButton->setImageType(LockButton::ImageIncomingCall);
		m_helpWin->setLabel(fromStdUtf8(LOCALIZED(kAnswerPhoneLabel)));
		showHelp();
	}
	else {
		m_lockButton->setImageType(LockButton::ImagePadlock);
		m_helpWin->setLabel(fromStdUtf8(LOCALIZED(kUnlockLabel))); 
	}

	m_popUpAlert->adjustAlertBounds();

	showAlert(m_popUpAlert);
}

void LockWindow::registerBannerView() 
{
	if(bannerViewRegistered) return;
	
	if(m_bannerAlerts)
		BannerMessageHandler::instance()->registerView(m_bannerAlerts);

	bannerViewRegistered = true;
}

void LockWindow::unregisterBannerView() 
{
	if(!bannerViewRegistered) return;
	
	if(m_bannerAlerts)
		BannerMessageHandler::instance()->unregisterView(m_bannerAlerts);

	bannerViewRegistered = false;
}

void LockWindow::activateBannerAlert()
{
	// start showing 1 or more banner alerts
	showAlert(m_bannerAlerts);
	hideAlert(m_dashboardAlerts);
}

void LockWindow::slotAlertActivated()
{
	// we don't want to show alerts that have nothing visible
	if (!m_popUpAlert->hasDisplayableContent())
		return;

	g_message("%s: has displayable content", __PRETTY_FUNCTION__);

	m_popupActive = true;

	// we always track the status of alerts whether we are locked or not.
	// this is due to inconsistent ordering of system events
	if (!isLocked())
		return;

	// only incoming call alerts should interrupt pin/password entry
	if (m_popUpAlert->hasIncomingCallAlert()) {
		DisplayManager::instance()->lock();
	}

	if(m_state == StatePinEntry)
		return;

	activatePopUpAlert();
}

void LockWindow::slotAlertDeactivated()
{
	// if the popup was never active then there's nothing to clean up
	if (!m_popupActive)
		return;

	g_message("%s: popup alert deactivated", __PRETTY_FUNCTION__);

	m_popupActive = false;

	// we always track the status of alerts whether we are locked or not.
	// this is due to inconsistent ordering of system events
	if (!isLocked())
		return;

	// reset back to initial state
	hideAlert(m_popUpAlert);

	if (Preferences::instance()->showAlertsWhenLocked() && (m_state == StateNormal)) {
		if (m_bannerActive)
			showAlert(m_bannerAlerts);
		else
			showAlert(m_dashboardAlerts);
	}

	hideHelp();

	m_lockButton->setImageType(LockButton::ImagePadlock);
	m_helpWin->setLabel(fromStdUtf8(LOCALIZED(kUnlockLabel)));
}

void LockWindow::slotTransientAlertActivated()
{
	g_message("%s: has displayable content", __PRETTY_FUNCTION__);
	// TODO: handle this for Lock Screen
}

void LockWindow::slotTransientAlertDeactivated()
{
	g_message("%s: transient alert deactivated", __PRETTY_FUNCTION__);
	// TODO: handle this for Lock Screen
}

void LockWindow::slotBannerActivated()
{
	if (!Preferences::instance()->showAlertsWhenLocked())
		return;

	g_message("%s", __PRETTY_FUNCTION__);

	m_bannerActive = true;

	// we always track the status of alerts whether we are locked or not.
	// this is due to inconsistent ordering of system events
	if (!isLocked() || m_popupActive || (m_state == StatePinEntry))
		return;

	activateBannerAlert();
}

void LockWindow::slotBannerDeactivated()
{
	if (!Preferences::instance()->showAlertsWhenLocked())
		return;

	g_message("%s", __PRETTY_FUNCTION__);

	m_bannerActive = false;

	// we always track the status of alerts whether we are locked or not.
	// this is due to inconsistent ordering of system events
	if (!isLocked() || m_popupActive)
		return;

	// banner has finished showing all messages
	hideAlert(m_bannerAlerts);

	if(m_state == StateNormal)
		showAlert(m_dashboardAlerts);
}

void LockWindow::slotBannerAboutToUpdate(QRect& target)
{
	// target is the area of the screen which is about to get invalidated
	// to draw the banner.
	if (m_state == StateNormal || m_state == StateDockMode) {
		target = mapRectFromItem(m_bannerAlerts, m_bannerAlerts->m_bounds).toRect(); // $$$ what is this used for?
	}
}

void LockWindow::changeState(State newState)
{
	if (newState == m_state)
		return;

	switch (m_state) {
	case StateUnlocked:

		// make sure the clock is correct
		m_clockWin->updateTimeFormat();
		m_clockWin->tick();

		// put the unlock button at its resting spot
		m_lockButton->reset();

		// kick off our timer
		m_paintTimer.start(kPaintTick);

		// time stamp when the screen was locked
		m_lastLocked = Time::curSysTimeMs();
		break;
	case StateNormal:
	case StateDockMode:
		// our new state will setup normal vs nightstand

		break;
	case StatePinEntry:
		m_newPasscode.clear();
		m_setupNewPin = false;
		m_setupNewPassword = false;

		if (newState != StateUnlocked)
			hidePinPanel();
		break;
	case StateLastTryDialog:
	case StateNewPinDialog:
		hideDialog();
		break;
//	case StateInPhoneCall:
//		if (newState != StateUnlocked)
//			hidePhoneApp();
//		else{
//			SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::TOP_LEVEL_WINDOW_MANAGER, m_phoneAppWin, false);
//			if (m_phoneAppWin)
//				m_phoneAppWin->setPaintCompositionMode(QPainter::CompositionMode_SourceOver);
//			// hide elements under the PIN app for the fade effect
//			toggleElemetsVisibleUnderPhoneApp(false);
//		}
//		break;
	default: luna_assert(0);
	}

	// setup new state
	switch (newState) {
	case StateUnlocked:
		unlock();
		break;
	case StateNormal:
		m_lockButton->reset();
		m_lockButton->setVisible(true);

//		toggleElemetsVisibleUnderPhoneApp(true);
//		if (m_phoneAppWin)
//			m_phoneAppWin->setPaintCompositionMode(QPainter::CompositionMode_Source);

		hidePinPanel();

		m_bgNode->showBackground(newState == StateNormal);

		if (m_popupActive) {
			activatePopUpAlert();
		}
		else if (Preferences::instance()->showAlertsWhenLocked()) {

			hideAlert(m_popUpAlert);
			if (m_bannerActive) {
				activateBannerAlert();
			}
			else {
				hideAlert(m_bannerAlerts);
				showAlert(m_dashboardAlerts);
			}
		}
		else {
			hideAlert(m_bannerAlerts);
			hideAlert(m_dashboardAlerts);
			hideAlert(m_popUpAlert);
		}

		// we are now locked
		if (m_state == StateUnlocked) {
			Q_EMIT signalScreenLocked(ScreenLocking);
		}
		break;
	case StatePinEntry:
		// prevent the lock button from showing up on top of the pin app
		m_lockButton->setVisible(false);

		// make the status bar visible
		m_bgNode->showBackground(true);

		hideAlert(m_bannerAlerts);
		hideAlert(m_dashboardAlerts);
		hideAlert(m_popUpAlert);

		showPinPanel();
		break;
	case StateNewPinDialog:
		m_lockButton->setVisible(false);

		// make the status bar visible
		m_bgNode->showBackground(true);

		hideAlert(m_bannerAlerts);
		hideAlert(m_dashboardAlerts);
		hideAlert(m_popUpAlert);

		if (EASPolicyManager::instance()->policyPending()
				&& EASPolicyManager::instance()->getPolicy() 
				&& EASPolicyManager::instance()->getPolicy()->requiresAlphaNumeric())
		{
			QMetaObject::invokeMethod(m_unlockDialog, "setupDialog", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Password Required"))),
					Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Your security requirements have changed. To access your device, you must set a new password. This will automatically turn on Secure Unlock"))),
					Q_ARG(QVariant, 3));
			QMetaObject::invokeMethod(m_unlockDialog, "setButton1", Q_ARG(QVariant, ""), Q_ARG(QVariant, "disabled"));
			QMetaObject::invokeMethod(m_unlockDialog, "setButton2", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("New Password"))), Q_ARG(QVariant, "affirmative"));
			QMetaObject::invokeMethod(m_unlockDialog, "setButton3", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Cancel"))), Q_ARG(QVariant, "normal"));
		}
		else {
			QMetaObject::invokeMethod(m_unlockDialog, "setupDialog", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("PIN Required"))),
					Q_ARG(QVariant, fromStdUtf8(LOCALIZED("To access your device, you must first set a PIN or Password for Secure Unlock. This will automatically turn on Secure Unlock"))),
					Q_ARG(QVariant, 3));
			QMetaObject::invokeMethod(m_unlockDialog, "setButton1", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("New PIN"))), Q_ARG(QVariant, "affirmative"));
			QMetaObject::invokeMethod(m_unlockDialog, "setButton2", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("New Password"))), Q_ARG(QVariant, "affirmative"));
			QMetaObject::invokeMethod(m_unlockDialog, "setButton3", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Cancel"))), Q_ARG(QVariant, "normal"));
		}

		showDialog();
		break;
	case StateLastTryDialog:
		m_lockButton->setVisible(false);

		// make the status bar visible
		m_bgNode->showBackground(true);

		hideAlert(m_bannerAlerts);
		hideAlert(m_dashboardAlerts);
		hideAlert(m_popUpAlert);

		if(0 == Security::instance()->getLockMode().compare("pin")) {
			QMetaObject::invokeMethod(m_unlockDialog, "setupDialog", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Warning"))),
																	 Q_ARG(QVariant, fromStdUtf8(LOCALIZED("PIN incorrect. If you enter an incorrect PIN now your device will be erased"))),
																	 Q_ARG(QVariant, 1));
		} else {
			QMetaObject::invokeMethod(m_unlockDialog, "setupDialog", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Warning"))),
																	 Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Password incorrect. If you enter an incorrect Password now your device will be erased"))),
																	 Q_ARG(QVariant, 1));
		}
		QMetaObject::invokeMethod(m_unlockDialog, "setButton1", Q_ARG(QVariant,fromStdUtf8( LOCALIZED("Ok"))), Q_ARG(QVariant, "normal"));
		QMetaObject::invokeMethod(m_unlockDialog, "setButton2", Q_ARG(QVariant, ""), Q_ARG(QVariant, "disabled"));
		QMetaObject::invokeMethod(m_unlockDialog, "setButton3", Q_ARG(QVariant, ""), Q_ARG(QVariant, "disabled"));

		showDialog();
		break;
//	case StateInPhoneCall:
//		// prevent the lock button from showing up on top of the phone app
//		m_lockButton->setVisible(false);
//
//		// make the status bar visible
//		m_bgNode->showBackground(true);
//
////		if (m_phoneAppWin)
////			m_phoneAppWin->setPaintCompositionMode(QPainter::CompositionMode_Source);
//
//		hideAlert(m_bannerAlerts);
//		hideAlert(m_dashboardAlerts);
//		hideAlert(m_popUpAlert);
//
//		showPhoneApp();
//		break;
	case StateDockMode:
		// prevent the lock button from showing up on top of the pin app
		m_lockButton->setVisible(false);

		// make the status bar visible
		m_bgNode->showBackground(false);
//		toggleElemetsVisibleUnderPhoneApp(true);
//		if (m_phoneAppWin)
//			m_phoneAppWin->setPaintCompositionMode(QPainter::CompositionMode_Source);
		hidePinPanel();

		hideAlert(m_bannerAlerts);
		hideAlert(m_dashboardAlerts);
		hideAlert(m_popUpAlert);

		if (m_state == StateUnlocked)
			Q_EMIT signalScreenLocked(ScreenLocking);
		break;

	default: luna_assert(0);
	}

	m_state = newState;

	update();
}

void LockWindow::mapCoordToWindow(Window* win, int& x, int& y) const
{
	if (!win)
		return;
	// $$$
	x = x - win->boundingRect().x();
	y = MAX(0, y - win->boundingRect().y());
}

void LockWindow::fadeWindow(bool in)
{
	if(m_windowFadeAnimation.state() != QAbstractAnimation::Stopped)
		m_windowFadeAnimation.stop();

	if(!isVisible() || !DisplayManager::instance()->isDisplayOn()) {
		// invisible because the TopLevel WM was set to invisible or the screen is off
		// dont animate in this case, just set it to the desired state
		if(in) {
			setOpacity(1.0);
			setVisible(true);
			Q_EMIT signalScreenLocked(ScreenLocked);
		} else {
			setOpacity(0.0);
			setVisible(false);
			hidePinPanel(false);
		}
		return;
	}

	m_windowFadeAnimation.setStartValue(opacity());
	if(in) {
		// fade in
		setVisible(true);

		if(m_statusBar && m_bgNode) {
			m_statusBar->setVisible(m_bgNode->isBackgroundShown());
		}
		m_windowFadeAnimation.setEndValue(1.0);
	} else{
		// fade out
		m_windowFadeAnimation.setEndValue(0.0);

		if(m_statusBar && !SystemUiController::instance()->isInFullScreenMode()) {
			m_statusBar->setVisible(false);
		}
	}

	if(m_bgNode)
		m_bgNode->setCompositionMode(QPainter::CompositionMode_SourceOver);

	m_windowFadeAnimation.start();
}

void LockWindow::slotWindowFadeAnimationFinished()
{
	if(m_bgNode)
		m_bgNode->setCompositionMode(QPainter::CompositionMode_Source);

	if(opacity() == 0.0) {
		setVisible(false);
//		if (m_phoneAppWin)
//			m_phoneAppWin->setPaintCompositionMode(QPainter::CompositionMode_Source);
		hidePinPanel(false);
//		toggleElemetsVisibleUnderPhoneApp(true);

		m_lockButton->reset();

		// make sure we deactivate the PIN app
//		if (m_phoneAppWin)
//			m_phoneAppWin->focusEvent(false);
	} else {
		Q_EMIT signalScreenLocked(ScreenLocked);
	}
}

//void LockWindow::toggleElemetsVisibleUnderPhoneApp(bool visible)
//{
//	if(m_elementsShown == visible)
//		return;
//
//	m_clockWin->setVisible(visible);
//	m_bgNode->setVisible(visible);
//	m_dashboardAlerts->setVisible(visible);
//	m_bannerAlerts->setVisible(visible);
//	m_popUpAlert->setVisible(visible);
//
//	// always hide the help window
//	hideHelp(false);
//
//	m_elementsShown = visible;
//}

void LockWindow::tryUnlock()
{
	if (!isLocked())
		return;

	if (requiresPasscode()) {
		if (EASPolicyManager::instance()->policyPending()) {
			// User needs to set up a new PIN/Password
			changeState(StateNewPinDialog);

		} else {
			// must enter pin/password to really unlock
			g_message ("%s: switching to pin entry", __PRETTY_FUNCTION__);

			changeState(StatePinEntry);

//        	if(!m_popupActive || !m_popUpAlert->hasIncomingCallAlert()) {
//        		changeState(StatePinEntry);
//        	} else {
//        		changeState(StateInPhoneCall);
//        	}
		}
	}
	else {
		// we can finally unlock
        g_message ("%s: unlocking without pin entry", __PRETTY_FUNCTION__);
		changeState(StateUnlocked);
	}

	DisplayManager::instance()->unlock();
}

void LockWindow::unlock()
{
	if (!isLocked())
		return;

	g_message("screen unlocked");

	// open the dashboard unless we are answering a call or the banner is updating
	if (!m_popUpAlert->hasIncomingCallAlert() && !m_bannerActive &&
		CoreNaviManager::instance()->numStandbyRequests() > 0)
	{
		getDashboardWindowManager()->openDashboard();
	}

	Q_EMIT signalScreenLocked(ScreenUnlocked);

	// reset our state
	m_helpWin->setLabel(fromStdUtf8(LOCALIZED(kUnlockLabel)));
	hideHelp(false); // don't animate the help fade

	m_lockButton->setImageType(LockButton::ImagePadlock);

	m_paintTimer.stop();

	m_lastLocked = 0;
}

bool LockWindow::requiresPasscode() const
{
	if (EASPolicyManager::instance()->policyPending())
		return true;
	if (Security::instance()->passcodeSet()) {
		uint32_t timeDiff = (Time::curSysTimeMs() - m_lastLocked) / 1000;
		if (timeDiff >= m_lockTimeout)
			return true;
	}
	return false;
}

void LockWindow::slotDeviceUnlocked()
{
	// Pin application verified the user's pin, unlock
	if (m_state != StatePinEntry)
		return;

	changeState(StateUnlocked);

	// tell display we have unlocked
	DisplayManager::instance()->unlock();
}

void LockWindow::slotCancelPasswordEntry()
{
	// user wants out of the pin application
	if (m_state != StatePinEntry)
		return;

	m_setupNewPin = false;
	m_setupNewPassword = false;
	m_newPasscode.clear();

	// tell display manager to relock us
	DisplayManager::instance()->lock();
}

void LockWindow::slotPasswordSubmitted(QString password, bool isPIN)
{
	int minLen = 0;
	char hint[30];
	std::string hintStr;
	if(EASPolicyManager::instance()->getPolicy() && EASPolicyManager::instance()->getPolicy()->validMinLength()) {
		minLen = EASPolicyManager::instance()->getPolicy()->minLength();
	}

	if(!m_setupNewPin && !m_setupNewPassword) {
		bool unlocked = false;
		int retriesLeft = 0;
		bool lockedOut = false;
		std::string pass = std::string(password.toUtf8().data());

		unlocked = Security::instance()->matchPasscode(pass, retriesLeft, lockedOut);

		if(unlocked) {
			// Pin application verified the user's pin, unlock
			if (m_state != StatePinEntry)
				return;

			changeState(StateUnlocked);

			// tell display we have unlocked
			DisplayManager::instance()->unlock();
		} else {
			bool isPin = true;
			std::string title;
			std::string message;

			retriesLeft = EASPolicyManager::instance()->retriesLeft();

			if(retriesLeft > 1) {
				char tries[100];
				sprintf (tries, LOCALIZED("%d Tries Remaining").c_str(), retriesLeft);
				message = tries;
			} else if (retriesLeft == 1) {
				changeState(StateLastTryDialog);
			} else {
				if(EASPolicyManager::instance()->getPolicy() && EASPolicyManager::instance()->getPolicy()->validMaxRetries()) {
					// Device Will Be Erased
					hidePinPanel();
					if(0 == Security::instance()->getLockMode().compare("pin")) {
						QMetaObject::invokeMethod(m_unlockDialog, "setupDialog", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("PIN Incorrect"))),
								                                                 Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Your device will now be erased."))),
								                                                 Q_ARG(QVariant, 0));
					} else {
						QMetaObject::invokeMethod(m_unlockDialog, "setupDialog", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Password Incorrect"))),
								                                                 Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Your device will now be erased."))),
								                                                 Q_ARG(QVariant, 0));
					}
					showDialog();
					return;
				} else {
					message = LOCALIZED("Try Again");
				}
			}

			if(0 == Security::instance()->getLockMode().compare("password")) {
				isPin = false;
				title = LOCALIZED("Password Incorrect");
				QMetaObject::invokeMethod(m_unlockPanel, "queueUpTitle", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Device Locked"))), Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter Password"))));
			} else {
				isPin = true;
				title = LOCALIZED("PIN Incorrect");
				QMetaObject::invokeMethod(m_unlockPanel, "queueUpTitle", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Device Locked"))), Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter PIN"))));
			}

			QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, isPin),
									  Q_ARG(QVariant, fromStdUtf8(title)), Q_ARG(QVariant, fromStdUtf8(message)), Q_ARG(QVariant, false), Q_ARG(QVariant, (int)0));
		}
	} else if(m_setupNewPin){
		if(m_newPasscode.isEmpty()) {
			m_newPasscode = password;
			QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, true),
									  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter PIN Again"))),
									  Q_ARG(QVariant, " "), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, minLen));
		} else {
			if(m_newPasscode != password) {
				// passwords don't match, force re-entry
				m_newPasscode.clear();

				QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, true),
										  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("PIN Doesn't Match"))),
										  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Try Again"))), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, minLen));

				if(minLen > 0) {
					sprintf(hint, LOCALIZED("Must be at least %d numbers").c_str(), minLen);
				} else {
					sprintf(hint, " ");
				}
				hintStr=hint;

				QMetaObject::invokeMethod(m_unlockPanel, "queueUpTitle", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter PIN"))), Q_ARG(QVariant, fromStdUtf8(hintStr)));
			} else {
				// validate and set new PIN
				std::string errorText;
				int res = Security::instance()->setPasscode("pin", std::string(password.toUtf8().data()), errorText);

				if(res == Security::Success) {
					changeState(StateUnlocked);
				} else {
					m_newPasscode.clear();

					std::string titleText;
					if (res == Security::FailureWeakPasswordRepeat || res == Security::FailureWeakPasswordSequence) {
							titleText = LOCALIZED("PIN Not Secure");
					}
					else {
							titleText = LOCALIZED("Enter PIN");
					}

					QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, true),
											  Q_ARG(QVariant, fromStdUtf8(titleText)),
											  Q_ARG(QVariant, fromStdUtf8(errorText)), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, minLen));

					if(minLen > 0) {
						sprintf(hint, LOCALIZED("Must be at least %d numbers").c_str(), minLen);
					} else {
						sprintf(hint, " ");
					}
					hintStr = hint;
					QMetaObject::invokeMethod(m_unlockPanel, "queueUpTitle", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter PIN"))), Q_ARG(QVariant, fromStdUtf8(hintStr)));
				}
			}
		}
	} else if(m_setupNewPassword) {
		if(m_newPasscode.isEmpty()) {
			m_newPasscode = password;
			QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, false),
									  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter Password Again"))),
									  Q_ARG(QVariant, " "), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, minLen));
		} else {
			if(m_newPasscode != password) {
				// passwords don't match, force re-entry
				m_newPasscode.clear();

				QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, false),
										  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Password Doesn't Match"))),
										  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Try Again"))), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, minLen));

				if(minLen > 0) {
					sprintf(hint, LOCALIZED("Must be at least %d characters").c_str(), minLen);
				} else {
					sprintf(hint, " ");
				}
				hintStr = hint;
				QMetaObject::invokeMethod(m_unlockPanel, "queueUpTitle", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter Password"))), Q_ARG(QVariant, fromStdUtf8(hintStr)));
			} else {
				// validate and set new PIN
				std::string errorText;
				int res = Security::instance()->setPasscode("password", std::string(password.toUtf8().data()), errorText);

				if(res == Security::Success) {
					changeState(StateUnlocked);
				} else {
					m_newPasscode.clear();

					std::string titleText;
					if (res == Security::FailureWeakPasswordRepeat || res == Security::FailureWeakPasswordSequence) {
							titleText = LOCALIZED("Password Not Secure");
					}
					else {
							titleText = LOCALIZED("Enter Password");
					}

					QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, false),
											  Q_ARG(QVariant, fromStdUtf8(titleText)),
											  Q_ARG(QVariant, fromStdUtf8(errorText)), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, minLen));

					if(minLen > 0) {
						sprintf(hint, LOCALIZED("Must be at least %d characters").c_str(), minLen);
					} else {
						sprintf(hint, " ");
					}
					hintStr = hint;
					QMetaObject::invokeMethod(m_unlockPanel, "queueUpTitle", Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter Password"))), Q_ARG(QVariant, fromStdUtf8(hintStr)));
				}
			}
		}

	}
}

void LockWindow::slotPinPanelFocusRequest(bool focusRequest)
{
	if(focusRequest) {
		static_cast<QGraphicsObject*>(m_unlockPanel)->setFocus();

		IMEController::instance()->setClient(m_unlockPanel);
		IMEController::instance()->notifyInputFocusChange(m_unlockPanel, true);
		IMEController::instance()->notifyAutoCapChanged(m_unlockPanel, false);

	} else {
		m_unlockPanel->clearFocus();

		IMEController::instance()->removeClient(m_unlockPanel);
	}
}

void LockWindow::slotDialogButton1Pressed()
{
	if(m_state == StateNewPinDialog) {
		// user pressed Set New PIN
		m_setupNewPin = true;
		m_setupNewPassword = false;
		changeState(StatePinEntry);
	} else if(m_state == StateLastTryDialog) {
		changeState(StatePinEntry);
	}
}

void LockWindow::slotDialogButton2Pressed()
{
	if(m_state == StateNewPinDialog) {
		// user pressed Set New Password
		m_setupNewPin = false;
		m_setupNewPassword = true;
		changeState(StatePinEntry);
	}
}

void LockWindow::slotDialogButton3Pressed()
{
	if(m_state == StateNewPinDialog) {
		// user pressed CANCEl
		// tell display manager to relock us
		DisplayManager::instance()->lock();
	}
}

bool LockWindow::paint()
{
	if (m_clockWin) {
		m_clockWin->tick();
	}
	return true;
}

QRectF LockWindow::boundingRect() const
{
	return m_bounds;
}

void LockWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{

}

static const int kSlopFactorForClicks = 5;

bool LockWindow::handleFilteredSceneEvent(QEvent* event)
{
	static QPointF mouseDownPos;
	static bool mouseDown = false;

	if ((event->type() == QEvent::KeyPress) || (event->type() == QEvent::KeyRelease)) { // filter the key events
		if((m_state == StateNormal) || (m_state == StateDockMode) || ((m_state == StateUnlocked) && (isVisible())))
			return true; // eat away all keys is the screen is locked
	}

//	if(m_state == StateInPhoneCall) {
//	   if(event->type() == QEvent::KeyRelease || event->type() == QEvent::KeyPress) {
//			if(static_cast<QKeyEvent*>(event)->key() ==  Qt::Key_CoreNavi_Home ) {
//				if(event->type() == QEvent::KeyRelease)
//					changeState(StatePinEntry);;//tryUnlock();
//				return true;
//			}
//	   }
//		return false;
//	}

	if((m_state != StateNormal) && (m_state != StateDockMode)) // only get mouse events when locked
		return false;

	if (event->type() == QEvent::GestureOverride) {
		event->accept();
		return true;
	}

	QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
	Event ev;
	ev.setMainFinger(true);

	ev.x = mapFromScene(mouseEvent->pos()).x();
	ev.y = mapFromScene(mouseEvent->pos()).y();
	ev.modifiers = Event::modifiersFromQt(mouseEvent->modifiers());

	switch (mouseEvent->type()) {
	case QEvent::MouseButtonPress: {
		mouseDown = true;
		mouseDownPos = mouseEvent->pos();

		ev.type = Event::PenDown;
		handlePenDownEvent(&ev);
	}
	break;

	case QEvent::MouseButtonRelease:{
		if (mouseDown) {
			mouseDown = false;
			QPointF diff = mouseEvent->pos() - mouseDownPos;
			ev.setClicked(diff.manhattanLength() <= kSlopFactorForClicks);
		}
		ev.type = Event::PenUp;
		handlePenUpEvent(&ev);
	}
	break;

	case QEvent::MouseMove:{
		if(!mouseDown)
		{
			mouseDown = true; // fake the mouse down
			mouseDownPos = mouseEvent->pos();
		}

		ev.type = Event::PenMove;
		handlePenMoveEvent(&ev);
	}
	break;

	default:
		break;
	}

	return true;
}


void LockWindow::handlePenDownEvent(Event* event)
{
	switch (m_state) {
	case StateNormal:
	case StateDockMode:
		handlePenDownStateNormal(event);
		break;
//	case StateInPhoneCall:
//		if (!event->rejected() && m_phoneAppWin) {
//			mapCoordToWindow(m_phoneAppWin, event->x, event->y);
//			m_phoneAppWin->inputEvent(event);
//		}
//		break;
	default: break;
	}
}

void LockWindow::handlePenDownStateNormal(Event* event)
{
	// allow the button to be picked up if it is at rest
	if (!m_lockButton->atRest() || !m_lockButton->contains(event->x, event->y))
		return;

	m_lockButton->press(true);

	showHelp();
}

void LockWindow::handlePenMoveEvent(Event* event)
{
	switch (m_state) {
	case StateNormal:
	case StateDockMode:
		handlePenMoveStateNormal(event);
		break;
//	case StateInPhoneCall:
//		if (!event->rejected() && m_phoneAppWin) {
//			mapCoordToWindow(m_phoneAppWin, event->x, event->y);
//			m_phoneAppWin->inputEvent(event);
//		}
//		break;
	default: break;
	}
}

void LockWindow::handlePenMoveStateNormal(Event* event)
{
	if (!m_lockButton->m_pressed) {
		// allow a move to pick up the unlock button
		if (!m_lockButton->atRest() || !m_lockButton->contains(event->x, event->y))
			return;

		m_lockButton->press(true);
	}

	m_lockButton->setPos(event->x, event->y);

	int distanceSquared = m_lockButton->distanceToAnchorSquared(event->x, event->y);
	if ((distanceSquared > kSaucerRadiusSquared) && (event->y < m_lockButtonY))
		hideHelp();
	else
		showHelp();
}

void LockWindow::handlePenUpEvent(Event* event)
{
	switch (m_state) {
	case StateNormal:
	case StateDockMode:
		handlePenUpStateNormal(event);
		break;
//	case StateInPhoneCall:
//		if (!event->rejected() && m_phoneAppWin) {
//			mapCoordToWindow(m_phoneAppWin, event->x, event->y);
//			m_phoneAppWin->inputEvent(event);
//		}
//		break;
	default: break;
	}
}

void LockWindow::handlePenUpStateNormal(Event* event)
{
	if (m_lockButton->m_pressed) {

		m_lockButton->press(false);

		int distanceSquared = m_lockButton->distanceToAnchorSquared(event->x, event->y);
		if ((distanceSquared > kSaucerRadiusSquared) && (event->y < m_lockButtonY)){
			tryUnlock();
			SystemService::instance()->postLockButtonTriggered();
		}
	}
	
	if (!isLocked())
		return;

	if (m_state != StatePinEntry) {

		m_lockButton->setPos(m_lockButtonX, m_lockButtonY);

		startHideHelpTimer();
	}
	else {
		m_lockButton->reset();
	}

	// Is the click on a dashboard window?
	if (event->clicked()) {

		// Dashboard alert not active. Do nothing
		if (!m_dashboardAlerts->isVisible())
			return;

		// Help saucer is visible. Do nothing
		if (m_helpWin && m_helpWin->isVisible())
			return;

		DashboardWindowManager* dashMgr = getDashboardWindowManager();
		if (!dashMgr)
			return;

		QPoint click = m_dashboardAlerts->mapFromParent(QPoint(event->x, event->y)).toPoint();
		if (!m_dashboardAlerts->contains(click))
			return;
		
		m_dashboardAlerts->sendClickAt(click.x(), click.y());
	}
}

void LockWindow::handlePenFlickEvent(Event* event)
{
	switch (m_state) {
//	case StateInPhoneCall:
//		if (!event->rejected() && m_phoneAppWin) {
//			mapCoordToWindow(m_phoneAppWin, event->x, event->y);
//			m_phoneAppWin->inputEvent(event);
//		}
//		break;
	default: break;
	}
}

void LockWindow::handlePenCancelEvent(Event* event)
{
	switch (m_state) {
	case StateNormal:
	case StateDockMode:
		handlePenCancelStateNormal(event);
		break;
//	case StateInPhoneCall:
//		if (!event->rejected() && m_phoneAppWin) {
//			mapCoordToWindow(m_phoneAppWin, event->x, event->y);
//			m_phoneAppWin->inputEvent(event);
//		}
//		break;
	default: break;
	}
}

void LockWindow::handlePenCancelStateNormal(Event* event)
{
	if (!m_lockButton->m_pressed)
		return;

	m_lockButton->press(false);

	// animate button back to rest location
	m_lockButton->setPos(m_lockButtonX, m_lockButtonY);

	startHideHelpTimer();
}

bool LockWindow::handleKeyDownEvent(Event* event)
{
	switch (event->key) {
		// pass through the following keys
		case (Event::Key_Ringer):
		case (Event::Key_VolumeDown):
		case (Event::Key_VolumeUp):
		case (Event::Key_MediaPlay):
		case (Event::Key_MediaPause):
		case (Event::Key_MediaStop):
		case (Event::Key_MediaNext):
		case (Event::Key_MediaPrevious):
		case (Event::Key_MediaRepeatAll):
		case (Event::Key_MediaRepeatTrack):
		case (Event::Key_MediaRepeatNone):
		case (Event::Key_MediaShuffleOn):
		case (Event::Key_MediaShuffleOff):
		case (Event::Key_HeadsetButton):
		case (Event::Key_Headset):
		case (Event::Key_HeadsetMic): {
			return false;
		}
		default: break;
	}

	// don't eat the event unless we are locked
	return isLocked();
}

bool LockWindow::handleKeyUpEvent(Event* event)
{
	switch (event->key) {
		// pass through the following keys
		case (Event::Key_Ringer):
		case (Event::Key_VolumeDown):
		case (Event::Key_VolumeUp):
		case (Event::Key_MediaPlay):
		case (Event::Key_MediaPause):
		case (Event::Key_MediaStop):
		case (Event::Key_MediaNext):
		case (Event::Key_MediaPrevious):
		case (Event::Key_MediaRepeatAll):
		case (Event::Key_MediaRepeatTrack):
		case (Event::Key_MediaRepeatNone):
		case (Event::Key_MediaShuffleOn):
		case (Event::Key_MediaShuffleOff):
		case (Event::Key_HeadsetButton):
		case (Event::Key_Headset):
		case (Event::Key_HeadsetMic): {
			return false;
		}
		default: break;
	}

//	if (m_state == StatePinEntry && !event->rejected() && m_phoneAppWin) {
//		m_phoneAppWin->inputEvent(event);
//	}

	// don't eat the event unless we are locked
	return isLocked();
}

void LockWindow::startHideHelpTimer()
{
	// prevent the help hint from hiding while a call is incoming
	if (!m_popupActive || !m_popUpAlert->hasIncomingCallAlert()) {

		m_hideHelpTimer.start(kHideHelpTimeoutInMS, true);
	}
}

bool LockWindow::hideHelpTimeout()
{
	hideHelp();
	return false;
}

void LockWindow::showHelp()
{
	if (!m_helpWin)
		return;

	m_helpWin->animateOpacity(1.0);
	m_helpWin->setVisible(true);
	m_hideHelpTimer.stop();
}

void LockWindow::hideHelp(bool animate)
{
	if (!m_helpWin || !m_helpWin->isVisible())
		return;

	if (animate) {
		m_helpWin->animateOpacity(0.0);
	}
	else {
		m_helpWin->setVisible(false);
		m_helpWin->setOpacity(0.0);
	}
}

void LockWindow::showPinPanel()
{
	if(!m_unlockPanel)
		return;

	bool isPin = true;

	if(!m_setupNewPin && !m_setupNewPassword) {
		int retriesLeft = EASPolicyManager::instance()->retriesLeft();
		std::string message;

		if(0 == Security::instance()->getLockMode().compare("password")) {
			isPin = false;
		}

		if((retriesLeft == 1) && EASPolicyManager::instance()->getPolicy() && EASPolicyManager::instance()->getPolicy()->validMaxRetries()) {
			message = LOCALIZED("Final Try");
		} else {
			if(isPin) {
				message = LOCALIZED("Enter PIN");
			} else {
				message = LOCALIZED("Enter Password");
			}
		}

		if(isPin) {
			QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, isPin),
									  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Device Locked"))),
									  Q_ARG(QVariant, fromStdUtf8(message)), Q_ARG(QVariant, false), Q_ARG(QVariant, (int)0));
		} else {
			QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, isPin),
									  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Device Locked"))),
									  Q_ARG(QVariant, fromStdUtf8(message)), Q_ARG(QVariant, false), Q_ARG(QVariant, (int)0));
		}
	} else if(m_setupNewPin || m_setupNewPassword) {
		int minLen = 0;
		char hint[30];
		std::string hintStr;
		if(EASPolicyManager::instance()->getPolicy() && EASPolicyManager::instance()->getPolicy()->validMinLength()) {
			minLen = EASPolicyManager::instance()->getPolicy()->minLength();
		}

		if(m_setupNewPin) {
			if(minLen > 0) {
				sprintf(hint, LOCALIZED("Must be at least %d numbers").c_str(), minLen);
			} else {
				sprintf(hint, " ");
			}
			hintStr = hint;
			isPin = true;
			QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, isPin),
									  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter PIN"))),
									  Q_ARG(QVariant, fromStdUtf8(hintStr)), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, (int)minLen));
		} else if(m_setupNewPassword) {
			if(minLen > 0) {
				sprintf(hint, LOCALIZED("Must be at least %d characters").c_str(), minLen);
			} else {
				sprintf(hint, " ");
			}
			hintStr = hint;
			isPin = false;
			QMetaObject::invokeMethod(m_unlockPanel, "setupDialog", Q_ARG(QVariant, isPin),
									  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Enter Password"))),
									  Q_ARG(QVariant, fromStdUtf8(hintStr)), Q_ARG(QVariant, minLen>0), Q_ARG(QVariant, (int)minLen));
		}
	}

 	if(isPin)
 		m_unlockPanel->setPos (-m_unlockPanel->boundingRect().width()/2, -m_unlockPanel->boundingRect().height()/2);
 	else
 		m_unlockPanel->setPos (-m_unlockPanel->boundingRect().width()/2, -m_unlockPanel->boundingRect().height());

	QMetaObject::invokeMethod(m_unlockPanel, "fade", Q_ARG(QVariant, true), Q_ARG(QVariant, AS(lockFadeDuration)));
}

void LockWindow::hidePinPanel(bool animate)
{
	if(!m_unlockPanel)
		return;

	QMetaObject::invokeMethod(m_unlockPanel, "fade", Q_ARG(QVariant, false), Q_ARG(QVariant, AS(lockFadeDuration)));
}

void LockWindow::showDialog()
{
	if(!m_unlockDialog)
		return;

	m_unlockDialog->setPos (-m_unlockDialog->boundingRect().width()/2, -m_unlockDialog->boundingRect().height());

	QMetaObject::invokeMethod(m_unlockDialog, "fade", Q_ARG(QVariant, true), Q_ARG(QVariant, AS(lockFadeDuration)));
}

void LockWindow::hideDialog(bool animate)
{
	if(!m_unlockDialog)
		return;

	QMetaObject::invokeMethod(m_unlockDialog, "fade", Q_ARG(QVariant, false), Q_ARG(QVariant, AS(lockFadeDuration)));
}

//void LockWindow::showPhoneApp()
//{
//	if (!m_phoneAppWin)
//		return;
//
//	m_statusBar->setBarOpaque(true);
//
//	// If already there or animating to the right spot, just return
//	if(((m_phoneWinAnimation.state() == QAbstractAnimation::Stopped) && (m_phoneAppWin->y() == m_phoneAppShownY)) ||
//	   ((m_phoneWinAnimation.state() != QAbstractAnimation::Stopped) && (m_phoneWinAnimation.endValue() == m_phoneAppShownY)))
//	return;
//
//	m_phoneAppWin->focusEvent(true);
//
//	if(m_phoneWinAnimation.state() != QAbstractAnimation::Stopped)
//		m_phoneWinAnimation.stop();
//
//	if(!isVisible() || !DisplayManager::instance()->isDisplayOn()) {
//		m_phoneAppWin->setPos(m_phoneAppWin->pos().x(), m_phoneAppShownY);
//		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::TOP_LEVEL_WINDOW_MANAGER, m_phoneAppWin, true);
//	} else {
//		m_phoneWinAnimation.setStartValue(m_phoneAppWin->pos().y());
//		m_phoneWinAnimation.setEndValue(m_phoneAppShownY);
//		m_phoneWinAnimation.start();
//	}
//}
//
//void LockWindow::hidePhoneApp(bool animate)
//{
//	if(!m_unlockPanel)
//		return;
//
//	m_unlockPanel->clearFocus();
//
//	QMetaObject::invokeMethod(m_unlockPanel, "fade", Q_ARG(QVariant, false), Q_ARG(QVariant, 300));
//
//	if (!m_phoneAppWin)
//		return;
//
//	m_statusBar->setBarOpaque(false);
//
//	// If already there or animating to the right spot, just return
//	if(((m_phoneWinAnimation.state() == QAbstractAnimation::Stopped) && (m_phoneAppWin->y() == m_phoneAppHiddenY)) ||
//	   ((m_phoneWinAnimation.state() != QAbstractAnimation::Stopped) && (m_phoneWinAnimation.endValue() == m_phoneAppHiddenY)))
//	return;
//
//	if (!isLocked())
//		m_phoneAppWin->focusEvent(false);
//
//	SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::TOP_LEVEL_WINDOW_MANAGER, m_phoneAppWin, false);
//	if(m_phoneWinAnimation.state() != QAbstractAnimation::Stopped)
//		m_phoneWinAnimation.stop();
//
//	if (animate) {
//		m_phoneWinAnimation.setStartValue(m_phoneAppWin->pos().y());
//		m_phoneWinAnimation.setEndValue(m_phoneAppHiddenY);
//		m_phoneWinAnimation.start();
//	}
//	else {
//		m_phoneAppWin->setY(m_phoneAppHiddenY);
//	}
//}
//
//void LockWindow::slotPhoneWindowAnimationFinished()
//{
//	if (m_phoneAppWin && m_state == StateInPhoneCall) {
//		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::TOP_LEVEL_WINDOW_MANAGER, m_phoneAppWin, true);
//	}
//}

void LockWindow::slotVisibilityChanged()
{
//	if(isVisible() && (m_state == StateInPhoneCall) && m_phoneAppWin) {
//		// just became visible again, so refocus the PIN window
//		m_phoneAppWin->focusEvent(true);
//	}
}

void LockWindow::showAlert(TransparentNode* alertNode)
{
	if (!alertNode)
		return;
	
	// we will start showing a completely transparent node.
	alertNode->setVisible(true);
	alertNode->animateOpacity(1.0);
}

void LockWindow::hideAlert(TransparentNode* alertNode)
{
	if (!alertNode)
		return;

	alertNode->animateOpacity(0.0);
}

void LockWindow::slotPositiveSpaceAboutToChange(const QRect& r, bool fullscreen, bool resizing)
{
	// lock screen does not support full screen
	if (r.y() == (qreal)kTopPadding) {

//		if (m_phoneAppWin) {
//			m_phoneAppWin->positiveSpaceAboutToChange(r, fullscreen);
//			if (m_targetPositiveSpace.height() < r.height()) {
//                // suppress a synchronous resize if the user won't see it
//                if (m_state == StateInPhoneCall)
//                    m_phoneAppWin->resizeEventSync(r.width(), r.height());
//                else
//                    m_phoneAppWin->resizeEvent(r.width(), r.height());
//			}
//		}
		m_targetPositiveSpace = r;
	}
}

void LockWindow::slotPositiveSpaceChanged(const QRect& r)
{
	static bool initialBounds = true;
	if (initialBounds) {

		m_targetPositiveSpace = r;
		initialBounds = false;
	}

//	if (r.y() == (qreal)kTopPadding) {
//
//		if (m_phoneAppWin)
//			m_phoneAppWin->positiveSpaceChanged(r);
//	}
}

void LockWindow::slotPositiveSpaceChangeFinished(const QRect& r)
{
//	if (r.y() == (qreal)kTopPadding && m_phoneAppWin) {
//
//		m_phoneAppWin->resizeEvent(r.width(), r.height());
//		m_phoneAppWin->positiveSpaceChangeFinished(r);
//	}
}

// ------------------------------------------------------------------------------------------------

TransparentNode::TransparentNode(bool _stayVisible)
{
	setOpacity(0.0);
	setVisible(false);
	m_stayVisible = _stayVisible;

	m_fadeAnimation.setTargetObject(this);
	m_fadeAnimation.setPropertyName("opacity");
	m_fadeAnimation.setDuration(AS(lockFadeDuration));
	m_fadeAnimation.setEasingCurve(AS_CURVE(lockFadeCurve));

	connect(&m_fadeAnimation, SIGNAL(finished()), SLOT(fadeAnimationFinished()));

}

void TransparentNode::fadeAnimationFinished()
{
	// nodes which aren't visible should not receive render calls
	if(opacity() == 0.0 && !m_stayVisible)
		setVisible(false);
}


void TransparentNode::animateOpacity(qreal newOpacity)
{
	if(opacity() == newOpacity)
		return;
	if((m_fadeAnimation.state() == QAbstractAnimation::Running) &&
	   (m_fadeAnimation.endValue() == newOpacity))
		return;

	if(m_fadeAnimation.state() != QAbstractAnimation::Stopped)
		m_fadeAnimation.stop();

	m_fadeAnimation.setStartValue(opacity());
	m_fadeAnimation.setEndValue(newOpacity);
	m_fadeAnimation.start();
}



// ----------------------------------------------------------------------------------------

LockButton::LockButton()
	: m_pressed(false)
	, m_imageType(ImagePadlock)
	, m_buttonImages(QVector<QPixmap>(4))
{
	setVisible(true);

	Settings* settings = Settings::LunaSettings();
	QString prefix = qFromUtf8Stl(settings->lunaSystemResourcesPath);
	// padlock images
	QString filePath = prefix + "/screen-lock-padlock-off.png";
	m_buttonImages[ImagePadlock] = QPixmap(filePath);
	filePath = prefix + "/screen-lock-padlock-on.png";
	m_buttonImages[ImagePadlock+1] = QPixmap(filePath);

	// incoming call images
	filePath = prefix + "/screen-lock-incoming-call-off.png";
	m_buttonImages[ImageIncomingCall] = QPixmap(filePath);
	filePath = prefix + "/screen-lock-incoming-call-on.png";
	m_buttonImages[ImageIncomingCall+1] = QPixmap(filePath);

	setPixmap(m_buttonImages[m_imageType + (m_pressed?1:0)]);
	setOffset(-boundingRect().width()/2,-boundingRect().height()/2);

	if (!m_buttonImages[m_imageType].isNull()) {
		// make the unlock button easier to pick up
		m_hitTarget = QRect(-(boundingRect().width() * 0.75), -(boundingRect().height() * 0.75),
				            boundingRect().width() * 1.5, boundingRect().height() * 1.5);
	}


}

LockButton::~LockButton()
{
}

void LockButton::press(bool down)
{
	m_pressed = down;
	setPixmap(m_buttonImages[m_imageType + (m_pressed?1:0)]);
}

void LockButton::setAnchor(int x, int y)
{
	m_anchorX = x;
	m_anchorY = y;

	reset();
}

int LockButton::distanceToAnchorSquared(int x, int y)
{
	//return (int)sqrt( pow(m_anchorX - x, 2) + pow(m_anchorY - y, 2) );
	int dx = m_anchorX - x;
	int dy = m_anchorY - y;
	return dx * dx + dy * dy;
}

bool LockButton::atRest() const
{
	return pos().x() == m_anchorX && pos().y() == m_anchorY;
}

void LockButton::reset()
{
	setPos(m_anchorX, m_anchorY);

	press(false);
}

bool LockButton::contains(int x, int y) const
{
	return m_hitTarget.contains(mapFromParent(QPoint(x, y)).toPoint());
}

void LockButton::setImageType(Type type)
{
	switch (type) {
	case ImagePadlock:
	case ImageIncomingCall:
		break;
	default: return;
	}

	m_imageType = type;
	setPixmap(m_buttonImages[m_imageType + (m_pressed?1:0)]);
}

// ------------------------------------------------------------------------------------------------

HelpWindow::HelpWindow()
	: TransparentNode(false)
	, m_splitIndex(0)
	, m_font(0)
	, m_surf(0)
{
	setVisible(false);
	setOpacity(0.0);

	std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/screen-lock-target-scrim.png";
	m_surf = new QPixmap(filePath.c_str());

	const char* fontName = Settings::LunaSettings()->fontLockWindow.c_str();
	m_font = new QFont(fontName, 20); // $$$ font size
	m_font->setPixelSize(20);
	m_font->setBold(true);

	textLayout.setFont(*m_font);

	if (m_surf) {
		m_bounds = QRect(-m_surf->width()/2, -m_surf->height()/2, m_surf->width(), m_surf->height());
	}
}

HelpWindow::~HelpWindow()
{
	if (m_font)
		delete m_font;
	if (m_surf)
		delete m_surf;
}

QRectF HelpWindow::boundingRect() const
{
	return m_bounds;
}

void HelpWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	// draw the saucer
	if (m_surf)
		painter->drawPixmap(m_bounds, *m_surf);

	QPen oldPen = painter->pen();

	// draw the label
	painter->setPen(QColor(0xFF, 0xFF, 0xFF, 0xFF));

	textLayout.draw(painter, QPoint(-textLayout.boundingRect().width()/2,-textLayout.boundingRect().height()/2));

	painter->setPen(oldPen);
}

void HelpWindow::setLabel(QString label)
{
	textLayout.clearLayout();
	textLayout.setText(label);
	textLayout.setTextOption(QTextOption(Qt::AlignCenter));
	QFontMetrics fontMetrics(*m_font);

	// create the text layout for the label
	int leading = fontMetrics.leading();
	qreal height = 0;
	textLayout.beginLayout();
	int i = 0;
	while (i < 2) {
		QTextLine line = textLayout.createLine();
		if (!line.isValid())
			break;
		line.setLineWidth(boundingRect().width());
		height += leading;
		line.setPosition(QPointF(0, height));
		height += line.height();
	}
	textLayout.endLayout();

	update();
}

// ----------------------------------------------------------------------------------------

LockBackground::LockBackground()
	: m_showBackground(true)
	, m_compMode(QPainter::CompositionMode_Source)
	, m_statusBarPtr(0)
	, m_wallpaperFullScreen(true)
	, m_wallpaperPtr(0)
	, m_wallpaperRotation(0)
{
	// this covers the whole screen which blocks anything underneath it from being drawn
	setOpacity(1.0);

	// fill out the entire screen
	int width = SystemUiController::instance()->currentUiWidth();
	int height = SystemUiController::instance()->currentUiHeight();

	m_bounds = QRect(-width/2, -height/2, width, height);

	connect(((WindowServerLuna*)WindowServer::instance()), SIGNAL(signalWallpaperImageChanged(QPixmap*, bool, int)),
			this, SLOT(slotWallpaperImageChanged(QPixmap*, bool, int)));

	// Load mask pixmaps
	std::string maskFilePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/screen-lock-wallpaper-mask-bottom.png";
	m_bottomMask = QPixmap(qFromUtf8Stl(maskFilePath));

	maskFilePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/screen-lock-wallpaper-mask-top.png";
	m_topMask = QPixmap(qFromUtf8Stl(maskFilePath));
}

LockBackground::~LockBackground()
{
}

// QGraphicsItem::boundingRect
QRectF LockBackground::boundingRect() const
{
	return m_bounds;
}

void LockBackground::resize(int width, int height)
{
	m_bounds = QRect(-width/2, -height/2, width, height);
}

void LockBackground::setCompositionMode(QPainter::CompositionMode mode)
{
	m_compMode = mode;
}

// QGraphicsItem::paint
void LockBackground::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPainter::CompositionMode oldMode = painter->compositionMode();
	painter->setCompositionMode(m_compMode);

	if (m_showBackground && m_wallpaperPtr) {
		// blit wallpaper
		if (m_wallpaperPtr && !m_wallpaperPtr->isNull()) {
			if((m_wallpaperPtr->width() < m_bounds.width()) || (m_wallpaperPtr->height() < m_bounds.height())) {
				painter->fillRect(m_bounds, Qt::darkGray);
			}
			painter->rotate(-m_wallpaperRotation);
			painter->drawPixmap(-m_wallpaperPtr->width()/2, -m_wallpaperPtr->height()/2, m_wallpaperPtr->width(), m_wallpaperPtr->height(), *m_wallpaperPtr);
			painter->rotate(m_wallpaperRotation);
		} else {
			// draw a solid gray background if we have not walpaper image
			painter->fillRect(m_bounds, Qt::darkGray);
		}

		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

		if (!m_bottomMask.isNull()) {
			painter->drawPixmap(QRectF(m_bounds.x(), m_bounds.y() + m_bounds.height() - m_bottomMask.height(), m_bounds.width(), m_bottomMask.height()), m_bottomMask, m_bottomMask.rect());
		}

		if (!m_topMask.isNull()) {
			painter->drawPixmap(m_bounds.x(), m_bounds.y() + kTopPadding, m_bounds.width(), m_topMask.height(), m_topMask);
		}
	}
	else {
		// draw a solid black background
		painter->fillRect(m_bounds, QColor(0x0,0x0,0x0,0xff));
	}
	painter->setCompositionMode(oldMode);
}

void LockBackground::showBackground(bool visible)
{
	if(m_statusBarPtr) {
		m_statusBarPtr->setVisible(visible);
	}

	if (m_showBackground != visible) {
		m_showBackground = visible;
		update();
	}
}

void LockBackground::slotWallpaperImageChanged(QPixmap* wallpaper, bool fullScreen, int rotationAngle)
{
	m_wallpaperFullScreen = fullScreen;
	m_wallpaperPtr = wallpaper;
	m_wallpaperRotation = rotationAngle;

	if(isVisible())
		update();
}

// -------------------------------------------------------------------------------------------

DashboardAlerts::DashboardAlerts()
{

	std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/dashboard-scroll-fade.png";
	m_scrollFade = QPixmap (filePath.c_str());
	if (m_scrollFade.isNull())
		g_warning ("scrollFade image missing");

	filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-divider.png";
	m_divider = QPixmap (filePath.c_str());
	if (m_divider.isNull())
		g_warning ("divider image missing");

	kMaxDashboardItems = 6;
	kDashboardItemHeight = 52; // max height of the dashboard
	kVisibleDashboard = 5.5;
	kBottomPadding = 3;
	kTopPadding = 1;
	
	kDashboardWidgetHeight = kDashboardItemHeight * kVisibleDashboard + m_divider.height() * (kMaxDashboardItems - 1);
	m_bounds = QRect(-(kMaxWidth/2), -(kDashboardWidgetHeight/2), kMaxWidth, kDashboardWidgetHeight);

	setVisible(true);
	setOpacity(1.0);
}

DashboardAlerts::~DashboardAlerts()
{
}

QRectF DashboardAlerts::boundingRect() const
{
	return m_bounds;
}


void DashboardAlerts::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	int itemHeight = 0;
	QList<DashboardWindow*> dashboardWindows = getDashboardWindows();

	int numSurfaces = MIN(dashboardWindows.size(), (int)kMaxDashboardItems);
	if (numSurfaces == 0)
		return;

	qreal dashboardHeight = numSurfaces * kDashboardItemHeight + (numSurfaces - 1) * m_divider.height();
	if (numSurfaces == kMaxDashboardItems)
		dashboardHeight -= kDashboardItemHeight/2;

	m_bounds = QRect(-(kMaxWidth/2), -(dashboardHeight)/2, kMaxWidth, dashboardHeight);
	QRect imageBounds = m_bounds.adjusted (-kShadowWidth, -kShadowWidth -kTopPadding, kShadowWidth, kShadowWidth + kBottomPadding);

	if (gBackground9Tile) {
		gBackground9Tile->resize(imageBounds.width(), imageBounds.height());
		gBackground9Tile->paint (painter, imageBounds.topLeft());
	}

#if defined(USE_CLIP_PATH)
	bool	hasClipping = false;
	QPainterPath oldClipPath, paintPath;
	paintPath.addRoundedRect (m_bounds, 10,10);

	if (painter->hasClipping()) {
		hasClipping = true;
		oldClipPath = painter->clipPath();
	}
	painter->setClipPath (paintPath);
#endif

	int yOffset = m_bounds.top();
	for (int i = 0; i < numSurfaces; ++i) {

		const QPixmap* pix = dashboardWindows[dashboardWindows.size() - 1 - i]->acquireScreenPixmap();
		if (!pix) {
			g_critical("%s: Failed to acquireScreenPixmap", __PRETTY_FUNCTION__);
			continue;
		}

		itemHeight = pix->height();

		painter->drawPixmap(-m_bounds.width()/2, yOffset, m_bounds.width(), itemHeight, *pix);
		yOffset += itemHeight;

		if (i != numSurfaces - 1) { // not the last one, draw a divider
			painter->drawPixmap(QRect (-m_bounds.width()/2, yOffset, m_bounds.width(), m_divider.height()), m_divider, m_divider.rect());
			yOffset += m_divider.height();
		}
		else {
			if (numSurfaces  == kMaxDashboardItems) {
				painter->drawPixmap (QRect (-m_bounds.width()/2, m_bounds.bottom() - m_scrollFade.height(), m_bounds.width(), m_scrollFade.height()), m_scrollFade); 
			}
		}
	}

#if defined(USE_CLIP_PATH)
	if (hasClipping)
		painter->setClipPath(oldClipPath);
	else
		painter->setClipping (false);
#endif
}


QList<DashboardWindow*> DashboardAlerts::getDashboardWindows() const
{
	QList<DashboardWindow*> windows;
	DashboardWindowManager* dashMgr = 0;
	DashboardWindowContainer* container = 0;

	dashMgr = getDashboardWindowManager();
	if(dashMgr)
		container = dashMgr->dashboardWindowContainer();

	if (container) {
 		windows = container->windows();
	}
	return windows;
}

void DashboardAlerts::sendClickAt(int x, int y) // x,y are in this item's coordinate space
{
	DashboardWindowManager* dashMgr = getDashboardWindowManager();
	if (!dashMgr)
		return;

	QList<DashboardWindow*> dashboardWindows = getDashboardWindows();
	if (dashboardWindows.empty())
		return;

	int numSurfaces    = MIN(dashboardWindows.size(), (int)kMaxDashboardItems);
	int surfaceHit     = (m_bounds.height()/2 + y) / (kDashboardItemHeight + m_divider.height()); // 0,0 of the m_bounds is set at the center in the paint method
	int dashboardIndex = dashboardWindows.size() - 1 - surfaceHit;

	if ((dashboardIndex < dashboardWindows.size() - numSurfaces)
			|| (dashboardIndex >= dashboardWindows.size()))
		return;

	y = ((m_bounds.height()/2 + y) % (kDashboardItemHeight + m_divider.height())) - kDashboardItemHeight/2;

	dashMgr->sendClicktoDashboardWindow(dashboardIndex, x, y, true);
}

// -------------------------------------------------------------------------------------------

BannerAlerts::BannerAlerts()
	: BannerMessageView(BannerMessageView::NoScroll)
{
	m_bounds = QRect(-(int)kMaxWidth/2, -(int)kBannerWidgetHeight/2,
				     kMaxWidth, kBannerWidgetHeight);	
	kPadding = 10;
}

BannerAlerts::~BannerAlerts()
{
}

QRectF BannerAlerts::boundingRect() const
{
	return m_bounds;
}

int  BannerAlerts::bmViewGetWidth() const
{
	return m_bounds.width();
}

void BannerAlerts::bmViewUpdated()
{
	update();
}

void BannerAlerts::bmViewMessageCountUpdated(int count)
{
	m_msgCount = count;
}

void BannerAlerts::bmViewShowingNonSuppressibleMessage(bool val)
{
	return;
}

void BannerAlerts::bmViewAddActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time)
{
	return;
}

void BannerAlerts::bmViewRemoveActiveCallBanner()
{
	return;
}

void BannerAlerts::bmViewUpdateActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time)
{
	return;
}

void BannerAlerts::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QRect imageBounds = m_bounds.adjusted (-kShadowWidth - kPadding, - kShadowWidth - kPadding, kShadowWidth + kPadding, kShadowWidth + kPadding);

	if (gBackground9Tile) {
		gBackground9Tile->resize(imageBounds.width(), imageBounds.height());
		gBackground9Tile->paint (painter, QPointF (-imageBounds.width()/2, -imageBounds.height()/2));
	}
	else {
		painter->fillRect (m_bounds, QColor (0, 0, 0, 0xff));
	}

	if (m_msgCount) {
		int tx = m_bounds.x();
		int ty = m_bounds.center().y() - kBannerWidgetHeight/2;
		painter->translate(tx, ty);
		BannerMessageHandler::instance()->drawView(painter, this);
		painter->translate(-tx, -ty);
	}
}

// -------------------------------------------------------------------------------------------

PopUpAlert::PopUpAlert()
{
	m_bounds = QRect(-(int)kMaxWidth/2, 0, kMaxWidth, 0);
	kPadding = 10;
}

PopUpAlert::~PopUpAlert()
{
}

bool PopUpAlert::hasIncomingCallAlert() const
{
	const Window* alert = getDashboardWindowManager()->getActiveAlertWin();
	return (alert && static_cast<const AlertWindow*>(alert)->isIncomingCallAlert());
}

void PopUpAlert::adjustAlertBounds()
{
	const AlertWindow* alertWin = (AlertWindow*)getActiveAlertWin();
	const QRect r = (alertWin ? alertWin->contentRect() : QRect(0,0,0,0));

	if (r.isEmpty())
		return;

	if (hasIncomingCallAlert()) {
		// FIXME: yet to be resolved, Larry to send visual design
		m_bounds.setHeight(SystemUiController::instance()->currentUiHeight() - kTopPadding - kAlertsFromBottom);
		m_bounds.moveTop(-m_bounds.height()/2);
		
		setPos(0, parentItem()->boundingRect().y() + kTopPadding + m_bounds.height()/2);
	}
	else {
		m_bounds.setHeight(r.height() + kPadding * 2);
		m_bounds.moveTop(-m_bounds.height()/2 - kPadding);

		setPos(0, 0);
	}

	prepareGeometryChange();
}

QRectF PopUpAlert::boundingRect() const
{
	return m_bounds;
}

void PopUpAlert::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	const AlertWindow* alertWin = (AlertWindow*)getActiveAlertWin();

	if (alertWin) {

		QRect alertBounds = alertWin->contentRect();
		if (!alertBounds.isEmpty()) {
			const QPixmap* alertSurf = const_cast<AlertWindow*>(alertWin)->acquireScreenPixmap();
			QRect imageBounds = alertBounds.adjusted (-kShadowWidth -kPadding , -kShadowWidth -kPadding, kShadowWidth + kPadding, kShadowWidth + kPadding);

			if (gBackground9Tile) {
				gBackground9Tile->resize(imageBounds.width(), imageBounds.height());
				gBackground9Tile->paint (painter, QPointF (-imageBounds.width()/2, -imageBounds.height()/2));
			}
			else {
				painter->fillRect (m_bounds, QColor (0, 0, 0, 0xff));
			}

			// render cut-out part of alert
			painter->drawPixmap(-alertBounds.width()/2, -alertBounds.height()/2, alertBounds.width(), alertBounds.height(),
					            *alertSurf,
					            alertBounds.x(), alertBounds.y(), alertBounds.width(), alertBounds.height());

		}
	}
}

bool PopUpAlert::hasDisplayableContent() const
{
	const AlertWindow* win = (const AlertWindow*)getActiveAlertWin();
	if (win) {
		return !win->contentRect().isEmpty();
	}
	return false;
}

const Window* PopUpAlert::getActiveAlertWin() const
{
	DashboardWindowManager* dashMgr = getDashboardWindowManager();
	return (dashMgr	? dashMgr->getActiveAlertWin() : 0);
}

// -----------------------------------------------------------------------------------------

DashboardWindowManager* getDashboardWindowManager()
{
	WindowManagerBase* dashMgr = static_cast<WindowServerLuna*>(WindowServer::instance())->dashboardWindowManager();
	return (dashMgr ? static_cast<DashboardWindowManager*>(dashMgr) : 0);
}


#include "LockWindow.moc"
