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




#include "StatusBar.h"
#include "Settings.h"
#include "Preferences.h"
#include "SystemService.h"
#include "StatusBarTitle.h"
#include "StatusBarClock.h"
#include "StatusBarBattery.h"
#include "StatusBarInfo.h"
#include "StatusBarItem.h"
#include "StatusBarNotificationArea.h"
#include "StatusBarItemGroup.h"
#include "StatusBarServicesConnector.h"
#include "AnimationSettings.h"
#include "WindowServerLuna.h"
#include "DashboardWindowManager.h"
#include "DockModeWindowManager.h"
#include "DockModeMenuManager.h"
#include "DashboardWindowContainer.h"
#include "SystemUiController.h"


#include <QPainter>
#include <QGesture>
#include <QGestureEvent>

QColor StatusBar::s_defaultColor = QColor(0x51, 0x55, 0x58, 0xFF);
static const std::string kDefaultCarrierName = "Open webOS";

StatusBar::StatusBar(StatusBarType type, int width, int height)
	: m_type(type)
	, m_showRssiIndicators(false)
	, m_rssiIndex(RSSI_0)
	, m_rssi1xIndex(RSSI_1X_0)
	, m_rssiShown(false)
	, m_rssi1xShown(false)
	, m_appMaximized(false)
	, m_showAppTitle(false)
	, m_svcConnector(0)
	, m_bkgPixmap(0)
	, m_battery(0)
	, m_clock(0)
	, m_title(0)
	, m_infoItems(0)
	, m_notif(0)
	, m_systemUiGroup(0)
	, m_titleGroup(0)
	, m_notifGroup(0)
	, m_forceOpaque(false)
	, m_platformHasPhoneRadio(false)
{
	m_carrierText = kDefaultCarrierName;
	m_appTitle = " ";

	m_bounds = QRect(-width/2, -height/2, width, height);

    // Charging icon
    m_battery = new StatusBarBattery();

    unsigned int clockPadding = 0;
    if(m_type == TypeDockMode)
        clockPadding = 5;

    // Text for clock
    m_clock = new StatusBarClock(clockPadding);

    m_infoItems = new StatusBarInfo(m_type);

	// Title Bar (a value of true on the third arg turns on non-tablet UI)
	m_title = new StatusBarTitle(Settings::LunaSettings()->statusBarTitleMaxWidth, height, false);

	if(Settings::LunaSettings()->tabletUi) {
		m_systemUiGroup = new StatusBarItemGroup(height, (m_type == TypeNormal || m_type == TypeDockMode), (m_type == TypeNormal || m_type == TypeDockMode), StatusBarItemGroup::AlignRight);
		connect(m_systemUiGroup, SIGNAL(signalBoundingRectChanged()), this, SLOT(slotChildBoundingRectChanged()));
		connect(m_systemUiGroup, SIGNAL(signalActivated(StatusBarItemGroup*)), this, SLOT(slotMenuGroupActivated(StatusBarItemGroup*)));
		m_systemUiGroup->setParentItem(this);

		if(m_clock) {
			if (m_type == TypeLockScreen)
				m_clock->setParentItem(this);
			else 
				m_systemUiGroup->addItem(m_clock);
		}
		m_systemUiGroup->addItem(m_battery);
		m_systemUiGroup->addItem(m_infoItems);

		m_titleGroup = new StatusBarItemGroup(height, (m_type == TypeNormal || m_type == TypeDockMode), (m_type == TypeNormal || m_type == TypeDockMode), StatusBarItemGroup::AlignLeft);
		connect(m_titleGroup, SIGNAL(signalBoundingRectChanged()), this, SLOT(slotChildBoundingRectChanged()));
		connect(m_titleGroup, SIGNAL(signalActivated(StatusBarItemGroup*)), this, SLOT(slotMenuGroupActivated(StatusBarItemGroup*)));
		m_titleGroup->setParentItem(this);

		m_titleGroup->addItem(m_title);

		if(m_type == TypeNormal || m_type == TypeDockMode || m_type == TypeFirstUse) {
			m_titleGroup->setActionable(false);
			connect(m_titleGroup, SIGNAL(signalActionTriggered(bool)), this, SLOT(slotAppMenuMenuAction(bool)));
		}

		if(m_type == TypeNormal || m_type == TypeDockMode) {
			m_notif = new StatusBarNotificationArea();
			if (m_type == TypeNormal) {
				m_notif->registerBannerView(); // default banner view, never gets unregistered
			}
			connect(m_notif, SIGNAL(signalNotificationArealVisibilityChanged(bool)), this, SLOT(slotNotificationArealVisibilityChanged(bool)));
			connect(m_notif, SIGNAL(signalBannerMessageActivated()), this, SLOT(slotBannerMessageActivated()));

			m_notif->setMaxHeight(m_bounds.height());
			m_notif->setMaxWidth(MAX_NOTIF_ICONS * (NOTIF_ICON_WIDTH + ICON_SPACING));

			m_notifGroup = new StatusBarItemGroup(height, false, true, StatusBarItemGroup::AlignRight);
			connect(m_notifGroup, SIGNAL(signalBoundingRectChanged()), this, SLOT(slotChildBoundingRectChanged()));
			connect(m_notifGroup, SIGNAL(signalActivated(StatusBarItemGroup*)), this, SLOT(slotMenuGroupActivated(StatusBarItemGroup*)));
			m_notifGroup->setParentItem(this);

			m_notifGroup->addItem(m_notif);
			m_notifGroup->setVisible(false);

			m_systemUiGroup->setActionable(true);
			connect(m_systemUiGroup, SIGNAL(signalActionTriggered(bool)), this, SLOT(slotSystemMenuMenuAction(bool)));

			m_notifGroup->setActionable(true);
			connect(m_notifGroup, SIGNAL(signalActionTriggered(bool)), this, SLOT(slotNotificationMenuAction(bool)));
		}
	} else {
		if(m_clock)
			m_clock->setParentItem(this);

		if(m_type == TypeNormal || m_type == TypeDockMode) {
			m_systemUiGroup = new StatusBarItemGroup(height, false, false, StatusBarItemGroup::AlignRight);
			if(m_systemUiGroup) {
				m_systemUiGroup->setParentItem(this);
				connect(m_systemUiGroup, SIGNAL(signalActionTriggered(bool)), this, SLOT(slotSystemMenuMenuAction(bool)));
				m_systemUiGroup->setActionable(true);
				if(m_battery)
					m_systemUiGroup->addItem(m_battery);
				if(m_infoItems)
					m_systemUiGroup->addItem(m_infoItems);
			}
		}
		if (m_type == TypeNormal || m_type == TypeDockMode || m_type == TypeFirstUse) {
			m_titleGroup    = new StatusBarItemGroup(height, true, false, StatusBarItemGroup::AlignLeft);
			if(m_titleGroup) {
				m_titleGroup->setParentItem(this);
				m_titleGroup->setActionable(false);
				connect(m_titleGroup, SIGNAL(signalActionTriggered(bool)), this, SLOT(slotAppMenuMenuAction(bool)));
				if(m_title)
					m_titleGroup->addItem(m_title);
			}
		} else {
			if(m_battery)
				m_battery->setParentItem(this);
			if(m_title)
				m_title->setParentItem(this);
			if(m_infoItems)
				m_infoItems->setParentItem(this);
		}
	}
	grabGesture (Qt::TapGesture);

	setMaximizedAppTitle(false);
}

StatusBar::~StatusBar()
{
	ungrabGesture (Qt::TapGesture);

	if (m_battery)
		delete m_battery;

	if (m_clock)
		delete m_clock;

	if (m_title)
		delete m_title;

	if (m_infoItems)
		delete m_infoItems;

	if(m_systemUiGroup)
		delete m_systemUiGroup;

	if(m_titleGroup)
		delete m_titleGroup;

	if(m_bkgPixmap)
		delete m_bkgPixmap;

	if(m_fadeAnimPtr)
		delete m_fadeAnimPtr;

	if(m_colorAnimPtr)
		delete m_colorAnimPtr;

	if(m_notifGroup)
		delete m_notifGroup;

	if(m_notif)
		delete m_notif;
}

void StatusBar::init()
{
	// Instantiates and initialized the TIL connector
	m_svcConnector = StatusBarServicesConnector::instance();

	Settings* settings = Settings::LunaSettings();
	std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/status-bar-background.png";

	if(m_battery) {
		m_battery->init();
	}

	if(m_infoItems)
		m_infoItems->init();

	// check if we know already if this platform has a modem or not
	if(PHONE_TYPE_UNKNOWN != m_svcConnector->getPhoneType() && PHONE_TYPE_NONE != m_svcConnector->getPhoneType())
		setRssiShown(true);

	if(Settings::LunaSettings()->tabletUi) {
		m_bkgPixmap = new QPixmap(statusBarImagesPath.c_str());
		if(!m_bkgPixmap->isNull()) {
			m_barColor = s_defaultColor;
			m_curColor = m_barColor;
			m_newColor = m_barColor;
			m_bkgOpacity = 0.0;

			m_fadeAnimPtr = new tStatusBarAnim(this, &StatusBar::fadeAnimValueChanged);
			m_fadeAnimPtr->setEasingCurve(AS_CURVE(statusBarFadeCurve));
			m_fadeAnimPtr->setDuration(AS(statusBarFadeDuration));
			m_fadeAnimPtr->setStartValue(0.0);
			m_fadeAnimPtr->setEndValue(1.0);
			m_fadeAnimPtr->setDirection(QAbstractAnimation::Forward);

			if(m_type == TypeNormal) {
				m_colorAnimPtr = new tStatusBarAnim(this, &StatusBar::colorAnimValueChanged);
				m_colorAnimPtr->setEasingCurve(AS_CURVE(statusBarColorChangeCurve));
				m_colorAnimPtr->setDuration(AS(statusBarColorChangeDuration));
				m_colorAnimPtr->setStartValue(0.0);
				m_colorAnimPtr->setEndValue(1.0);
				m_colorAnimPtr->setDirection(QAbstractAnimation::Forward);
			}
		}

		if(m_notif) {
			m_notif->init((DashboardWindowManager*) (((WindowServerLuna*)WindowServer::instance())->dashboardWindowManager()));
			connect(SystemUiController::instance(), SIGNAL(signalBannerActivated()), this, SLOT(slotBannerActivated()));
			connect(SystemUiController::instance(), SIGNAL(signalBannerDeactivated()), this, SLOT(slotBannerDeactivated()));
		}

		if(m_notifGroup) {
			// tie the DashboardWindowContainer to the menu item group
			QGraphicsObject* dashboardMenu = 0;
			dashboardMenu = ((DashboardWindowManager*)(((WindowServerLuna*)WindowServer::instance())->dashboardWindowManager()))->dashboardMenu();
			if(dashboardMenu) {
				dashboardMenu->setOpacity(0.0);
				dashboardMenu->setVisible(false);
				m_notifGroup->setMenuObject(dashboardMenu);
			}
		}

		if (m_type == TypeDockMode) {
			DockModeMenuManager* dmm = (DockModeMenuManager*)(((WindowServerLuna*)WindowServer::instance())->dockModeMenuManager());
			QGraphicsObject* dockModeAppMenu = dmm->getAppMenu();
			if(dockModeAppMenu) {
				dockModeAppMenu->setOpacity(0.0);
				dockModeAppMenu->setVisible(false);
				m_titleGroup->setMenuObject(dockModeAppMenu);
			}

			DockModeWindowManager* dmwm = (DockModeWindowManager*)(((WindowServerLuna*)WindowServer::instance())->dockModeManager());
			connect (dmwm, SIGNAL(signalDockModeStatusChanged(bool)), this, SLOT(slotDockModeStatusChanged(bool)));
		}
	}

	connect(Preferences::instance(),SIGNAL(signalTimeFormatChanged(const char*)), SLOT(slotTimeFormatChanged(const char*)));
	connect(m_svcConnector,SIGNAL(signalPhoneTypeUpdated()), SLOT(slotPhoneTypeUpdated()));
	connect(m_svcConnector,SIGNAL(signalCarrierTextChanged(const char*)), SLOT(slotCarrierTextChanged(const char*)));
	connect(m_svcConnector,SIGNAL(signalRssiIndexChanged(bool, StatusBar::IndexRSSI)), SLOT(slotRssiIndexChanged(bool, StatusBar::IndexRSSI)));
    connect(Preferences::instance(),SIGNAL(signalRotationLockChanged(OrientationEvent::Orientation)), SLOT(slotRotationLockChanged(OrientationEvent::Orientation)));
	connect(Preferences::instance(),SIGNAL(signalMuteSoundChanged(bool)), SLOT(slotMuteSoundChanged(bool)));

	if((m_type == TypeNormal) || (m_type == TypeLockScreen) || (m_type == TypeDockMode) || (m_type == TypeFirstUse)) {
		connect(m_svcConnector,SIGNAL(signalTTYStateChanged(bool)), SLOT(slotTTYStateChanged(bool)));
		connect(m_svcConnector,SIGNAL(signalHACStateChanged(bool)), SLOT(slotHACStateChanged(bool)));
		connect(m_svcConnector,SIGNAL(signalCallForwardStateChanged(bool)), SLOT(slotCallForwardStateChanged(bool)));
		connect(m_svcConnector,SIGNAL(signalRoamingStateChanged(bool)), SLOT(slotRoamingStateChanged(bool)));
		connect(m_svcConnector,SIGNAL(signalVpnStateChanged(bool)), SLOT(slotVpnStateChanged(bool)));
		connect(m_svcConnector,SIGNAL(signalWanIndexChanged(bool, StatusBar::IndexWAN)), SLOT(slotWanIndexChanged(bool, StatusBar::IndexWAN)));
		connect(m_svcConnector,SIGNAL(signalBluetoothIndexChanged(bool, StatusBar::IndexBluetooth)), SLOT(slotBluetoothIndexChanged(bool, StatusBar::IndexBluetooth)));
		connect(m_svcConnector,SIGNAL(signalWifiIndexChanged(bool, StatusBar::IndexWiFi)), SLOT(slotWifiIndexChanged(bool, StatusBar::IndexWiFi)));
	}

	switch (m_type) {
		case TypeNormal: {
			if(m_clock)
				m_clock->setDisplayDate(false);
		}
		break;

		case TypeLockScreen: {
			if(m_clock)
				m_clock->setDisplayDate(true);
		}
		break;

		case TypeDockMode: {
			if(m_clock)
				m_clock->setDisplayDate(false);

		}
		break;

		case TypeFirstUse: {
			if(m_clock)
				m_clock->setDisplayDate(false);
		}
		break;

		default: {
			g_error("%s : Invalid Status Bar type specified: %d", __PRETTY_FUNCTION__, m_type);
		}
	}

	layout();
}

bool StatusBar::sceneEvent (QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g) {
			event->accept();
			return true;
		}
	}

	if (event->type() == QEvent::Gesture) {
		QGestureEvent* gevt = static_cast<QGestureEvent*>(event);
		if (gevt) {
			QTapGesture* tap = (QTapGesture*) gevt->gesture (Qt::TapGesture);
			if (tap && tap->state() == Qt::GestureFinished) {
				gevt->accept();
				update();
				return true;
			}
		}
	}
	return QGraphicsObject::sceneEvent(event);
}

void StatusBar::setSystemMenuObject(QGraphicsObject* menu)
{
	if(menu && m_systemUiGroup) {
		// tie the SystemMenu to the system item group
		menu->setOpacity(0.0);
		menu->setVisible(false);
		m_systemUiGroup->setMenuObject(menu);
	}
}

void StatusBar::layout()
{
	if(Settings::LunaSettings()->tabletUi) {
		// Tablet UI layout

		// This item is Left Aligned (The position  of the icon is the position of the LEFT EDGE of the bounding rect)
		if(m_titleGroup)
			m_titleGroup->setPos(-m_bounds.width()/2, 0);

		// This item is Right Aligned (The position  of the icon is the position of the RIGHT EDGE of the bounding rect)
		if(m_systemUiGroup)
			m_systemUiGroup->setPos(m_bounds.width()/2, 0);

		if(m_type == TypeLockScreen && m_clock)
			m_clock->setPos (0, 0);

		// This item is Right Aligned (The position  of the icon is the position of the RIGHT EDGE of the bounding rect)
		if(m_notifGroup) {
			int offset = 0;

			if(m_systemUiGroup)
				offset = m_systemUiGroup->boundingRect().width();

			m_notifGroup->setPos(m_bounds.width()/2 - offset, 0);

			if(m_systemUiGroup)
				offset -= m_systemUiGroup->separatorWidth()/2; // adjust it so it matches with the divider

			Q_EMIT signalDashboardAreaRightEdgeOffset(offset);
		}
	} else {
		// static layout (for Phone UI)
		if(m_type == TypeNormal || m_type == TypeFirstUse) {
			// This item is Left Aligned (The position  of the icon is the position of the LEFT EDGE of the bounding rect)
			if(m_titleGroup)
				m_titleGroup->setPos(-m_bounds.width()/2, 0);

			// This item is Right Aligned (The position  of the icon is the position of the RIGHT EDGE of the bounding rect)
			if(m_systemUiGroup)
				m_systemUiGroup->setPos(m_bounds.width()/2, 0);
		} else {
			if(m_battery)
				m_battery->setPos (m_bounds.width()/2 - m_battery->width()/2, 0);

			if(m_title)
				m_title->setPos (-m_bounds.width()/2, 0);

			if(m_infoItems)
				m_infoItems->setPos (m_bounds.width()/2 - m_battery->width(), 0); // position here is of the right edge of the info item
		}

		if(m_clock)
			m_clock->setPos (0, 0);
	}
}

void StatusBar::resize(int w, int h)
{
	m_bounds = QRect(-w/2, -h/2, w, h);

	if(m_notif) {
		if (h != m_notif->maxHeight()) {
			m_notif->setMaxHeight(h);
		}
	}

	if(m_titleGroup)
		m_titleGroup->setHeight(h);

	if(m_systemUiGroup)
		m_systemUiGroup->setHeight(h);

	if(m_notifGroup)
		m_notifGroup->setHeight(h);

	layout();
}

void StatusBar::setBarOpaque(bool opaque)
{
	// this case is used when we simply want to turn the bar opaque, without showing an app title (Lock Screen - Pin app)
	m_forceOpaque = opaque;

	if(m_forceOpaque) {
		fadeBar(true);
	} else {
		if(!m_appMaximized)
			fadeBar(false);
	}
}


void StatusBar::setMaximizedAppTitle(bool appMaximized, const char* title, const unsigned int customColor, bool appTitleActionable)
{
	m_appMaximized = appMaximized;

	if(appMaximized) {
		if (m_type != TypeDockMode) {
			QColor color = QColor((customColor & 0xFF000000) >> 24,
					(customColor & 0x00FF0000) >> 16,
					(customColor & 0x0000FF00) >> 8,
					(customColor & 0x000000FF));
			if(!title) {
				m_showAppTitle = true;
				m_appTitle = " ";
				if(m_platformHasPhoneRadio)
					m_title->setTitleString(m_carrierText, false);
				else
					m_title->setTitleString(kDefaultCarrierName, false);
				setBackgroundColor(false);
			} else if(!strcmp(title, "@CARRIER")) {
				m_showAppTitle = false;
				m_title->setTitleString(m_carrierText, true);
				setBackgroundColor(customColor != 0, color);
			} else {
				m_showAppTitle = true;
				m_appTitle = title;
				m_title->setTitleString(m_appTitle, true);
				setBackgroundColor(customColor != 0, color);
			}
			if(m_titleGroup)
				m_titleGroup->setActionable(appTitleActionable);
			fadeBar(true);
		}
		else {
			m_showAppTitle = true;
			m_appTitle = title;
			m_title->setTitleString(m_appTitle, true);
			setBackgroundColor(false);
			if(m_titleGroup)
				m_titleGroup->setActionable(appTitleActionable);
			fadeBar(false);
		}
	} else {
		m_showAppTitle = false;
		m_appTitle = " ";

		if(m_platformHasPhoneRadio)
			m_title->setTitleString(m_carrierText, false);
		else
			m_title->setTitleString(kDefaultCarrierName, false);

		if(m_titleGroup)
			m_titleGroup->setActionable(false);

		if(!m_forceOpaque)
			fadeBar(false);
	}
}

void StatusBar::setCarrierText(const char* carrierTxt)
{
	m_carrierText = carrierTxt;

	if(!m_showAppTitle && m_platformHasPhoneRadio) {
		m_title->setTitleString(m_carrierText, m_appMaximized);
	}
}

void StatusBar::setRssiShown(bool shown)
{
	m_showRssiIndicators = shown;
	if(m_infoItems) {
		m_infoItems->setRSSI(m_showRssiIndicators && m_rssiShown, m_rssiIndex);
		m_infoItems->setRSSI1x(m_showRssiIndicators && m_rssi1xShown, m_rssi1xIndex);
	}
}

void StatusBar::setNotificationWindowOpen(bool open)
{
	if(m_notifGroup) {
		if(open)
			m_notifGroup->activate();
		else
			m_notifGroup->deactivate();
	}
}

void StatusBar::setSystemMenuOpen(bool open)
{
	if(m_systemUiGroup) {
		if(open)
			m_systemUiGroup->activate();
		else
			m_systemUiGroup->deactivate();
	}
}

void StatusBar::setDockModeAppMenuOpen(bool open)
{
	if(m_type == TypeDockMode && m_titleGroup) {
		if(open)
			m_titleGroup->activate();
		else
			m_titleGroup->deactivate();
	}
}


void StatusBar::slotTimeFormatChanged(const char* format)
{
	if(m_clock)
		m_clock->updateTimeFormat(format);
}

void StatusBar::slotPhoneTypeUpdated()
{
	if(PHONE_TYPE_UNKNOWN != m_svcConnector->getPhoneType() && PHONE_TYPE_NONE != m_svcConnector->getPhoneType()) {
		setRssiShown(true);
		m_platformHasPhoneRadio = true;
	}
	else {
		setRssiShown(false);
		m_platformHasPhoneRadio = false;
	}
}

void StatusBar::slotCarrierTextChanged(const char* text)
{
	if(m_carrierText.compare(text)) // only update if different
		setCarrierText(text);
}

void StatusBar::slotRssiIndexChanged(bool show, StatusBar::IndexRSSI index)
{
	m_rssiIndex = index;
	m_rssiShown = show;
	bool showRssiIcon = show &&(m_showRssiIndicators || (index == RSSI_FLIGHT_MODE));
	if(m_infoItems)
		m_infoItems->setRSSI(showRssiIcon, index);
}

void StatusBar::slotRssi1xIndexChanged(bool show, StatusBar::IndexRSSI1x index)
{
	m_rssi1xIndex = index;
	m_rssi1xShown = show;
	if(m_infoItems)
		m_infoItems->setRSSI1x(m_showRssiIndicators && show, index);
}

void StatusBar::slotTTYStateChanged(bool enabled)
{
	if(m_infoItems)
		m_infoItems->setTTY(enabled);
}

void StatusBar::slotHACStateChanged(bool enabled)
{
	if(m_infoItems)
		m_infoItems->setHAC(enabled);
}

void StatusBar::slotCallForwardStateChanged(bool enabled)
{
	if(m_infoItems)
		m_infoItems->setCallForward(enabled);
}

void StatusBar::slotRoamingStateChanged(bool enabled)
{
	if(m_infoItems)
		m_infoItems->setRoaming(enabled);
}

void StatusBar::slotVpnStateChanged(bool enabled)
{
	if(m_infoItems)
		m_infoItems->setVpn(enabled);
}

void StatusBar::slotWanIndexChanged(bool show, StatusBar::IndexWAN index)
{
	if(m_infoItems)
		m_infoItems->setWAN(show, index);
}

void StatusBar::slotBluetoothIndexChanged(bool show, StatusBar::IndexBluetooth index)
{
	if(m_infoItems)
		m_infoItems->setBluetooth(show, index);
}

void StatusBar::slotWifiIndexChanged(bool show, StatusBar::IndexWiFi index)
{
	if(m_infoItems)
		m_infoItems->setWifi(show, index);
}

void StatusBar::slotChildBoundingRectChanged()
{
	layout();
}

void StatusBar::slotRotationLockChanged(OrientationEvent::Orientation rotationLock)
{
	if(m_infoItems)
        m_infoItems->setRotationLock(rotationLock != OrientationEvent::Orientation_Invalid);
}

void StatusBar::slotMuteSoundChanged(bool muteOn)
{
	if(m_infoItems)
		m_infoItems->setMute(muteOn);
}

void StatusBar::slotNotificationArealVisibilityChanged(bool visible)
{
	if(m_notifGroup) {
		if(visible) {
			m_notifGroup->show();
		} else {
			m_notifGroup->hide();
		}
	}
}

void StatusBar::slotBannerMessageActivated()
{
	if(m_notifGroup) {
		if(m_notifGroup->isActivated()){
			m_notifGroup->deactivate();
		}
	}
}

void StatusBar::slotNotificationMenuAction(bool active)
{
	if(m_notif && m_notif->handleBannerMsgTap()) // tap redirected to Banner Message Window
		return;

	if(active) {
		SystemUiController::instance()->openDashboard();
	} else {
		SystemUiController::instance()->closeDashboard(true);
	}
}

void StatusBar::slotSystemMenuMenuAction(bool active)
{
	Q_EMIT signalSystemMenuStateChanged(active);
}

void StatusBar::slotAppMenuMenuAction(bool active)
{
	if (m_type != TypeDockMode)
		SystemUiController::instance()->toggleCurrentAppMenu();
	else 
		signalDockModeMenuStateChanged (active);
}

void StatusBar::slotBannerActivated()
{
	if(SystemUiController::instance()->isDashboardOpened())
		SystemUiController::instance()->closeDashboard(true);
}

void StatusBar::slotBannerDeactivated()
{
}

void StatusBar::slotMenuGroupActivated(StatusBarItemGroup* group)
{
	if(m_systemUiGroup && (m_systemUiGroup != group) && (m_systemUiGroup->isActivated()))
		m_systemUiGroup->actionTriggered();
	if(m_titleGroup && (m_titleGroup != group) && (m_titleGroup->isActivated()))
		m_titleGroup->actionTriggered();
	if(m_notifGroup && (m_notifGroup != group) && (m_notifGroup->isActivated()))
		m_notifGroup->actionTriggered();
}

void StatusBar::slotDockModeStatusChanged(bool enabled)
{
	if (m_type == TypeDockMode) {
		if (enabled) {
			m_notif->registerBannerView();
		}
		else  {
			m_notif->unregisterBannerView();
		}
	}
}

void StatusBar::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(!Settings::LunaSettings()->tabletUi || (m_type != TypeNormal && m_type != TypeLockScreen && m_type != TypeDockMode && m_type != TypeFirstUse) || !m_bkgPixmap || (m_bkgPixmap->isNull())) {
		painter->fillRect(m_bounds, Qt::black);
	} else {
		if(m_bkgOpacity > 0.0) {
			qreal opacity = painter->opacity();
			painter->setOpacity(opacity * m_bkgOpacity);
			painter->fillRect(m_bounds, m_barColor);
			painter->setOpacity(opacity);
		}
		painter->drawTiledPixmap(m_bounds, *m_bkgPixmap);
	}
}

void StatusBar::fadeBar(bool in)
{
	if(!Settings::LunaSettings()->tabletUi)
		return;

	if(in) {
		if(m_fadeAnimPtr.isNull()) {
			m_bkgOpacity = 1.0;
		} else {
			if((m_fadeAnimPtr->state() == QAbstractAnimation::Stopped) && (m_bkgOpacity == 1.0))
				return;

			if((m_fadeAnimPtr->state() != QAbstractAnimation::Stopped) && (m_fadeAnimPtr->direction() != QAbstractAnimation::Forward)) {
				m_fadeAnimPtr->pause();
				m_fadeAnimPtr->setDirection(QAbstractAnimation::Forward);
				m_fadeAnimPtr->resume();
			} else {
				m_fadeAnimPtr->setStartValue(0.0);
				m_fadeAnimPtr->setEndValue(1.0);
				m_fadeAnimPtr->setDirection(QAbstractAnimation::Forward);
				m_fadeAnimPtr->start();
			}
		}
	} else {
		if(m_fadeAnimPtr.isNull()) {
			m_bkgOpacity = 0.0;
		} else {
			if((m_fadeAnimPtr->state() == QAbstractAnimation::Stopped) && (m_bkgOpacity == 0.0))
				return;

			if((m_fadeAnimPtr->state() != QAbstractAnimation::Stopped) && (m_fadeAnimPtr->direction() != QAbstractAnimation::Backward)) {
				m_fadeAnimPtr->pause();
				m_fadeAnimPtr->setDirection(QAbstractAnimation::Backward);
				m_fadeAnimPtr->resume();
			} else {
				m_fadeAnimPtr->setStartValue(0.0);
				m_fadeAnimPtr->setEndValue(1.0);
				m_fadeAnimPtr->setDirection(QAbstractAnimation::Backward);
				m_fadeAnimPtr->start();
			}
		}
	}
}

void StatusBar::fadeAnimValueChanged(const QVariant& value)
{
	m_bkgOpacity = value.toReal();

	if(isVisible())
		update();
}

void StatusBar::setBackgroundColor(bool custom, QColor color)
{
	if(!custom) {
		m_newColor = s_defaultColor;
	} else {
		m_newColor = color;
	}

	if(!isVisible() || m_colorAnimPtr.isNull()) {
		m_barColor = m_newColor;
	} else {
		if((m_colorAnimPtr->state() != QAbstractAnimation::Stopped)) {
			m_colorAnimPtr->stop();
		}

		m_curColor = m_barColor;

		m_colorAnimPtr->setStartValue(0.0);
		m_colorAnimPtr->setEndValue(1.0);
		m_colorAnimPtr->start();
	}
}

void StatusBar::colorAnimValueChanged(const QVariant& value)
{
	qreal progress = value.toReal();
	qreal r, g, b;

	r = ((1.0 - progress) * m_curColor.redF())   + (progress * m_newColor.redF());
	g = ((1.0 - progress) * m_curColor.greenF()) + (progress * m_newColor.greenF());
	b = ((1.0 - progress) * m_curColor.blueF())  + (progress * m_newColor.blueF());

	m_barColor.setRedF(r);
	m_barColor.setGreenF(g);
	m_barColor.setBlueF(b);

	if(isVisible())
		update();
}

