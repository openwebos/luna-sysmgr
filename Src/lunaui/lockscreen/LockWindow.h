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




#ifndef LOCKWINDOW_H
#define LOCKWINDOW_H

#include "Common.h"

#include "Timer.h"
#include "sptr.h"
#include "Event.h"
#include "StatusBar.h"

#include <QObject>
#include <QPointer>
#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QDeclarativeComponent>


class Window;
class CardWindow;
class HelpWindow;
class LockButton;
class ClockWindow;
class LockBackground;
class DashboardAlerts;
class BannerAlerts;
class PopUpAlert;
class TransparentNode;
class EASPolicy;
class QMouseEvent;
class QGraphicsPixmapItem;
class GraphicsItemContainer;
class InputItem;


class LockWindow : public QGraphicsObject
{
	Q_OBJECT

public:

	LockWindow(uint32_t maxWidth, uint32_t maxHeight);
	virtual ~LockWindow();

	virtual void init();

	void addPhoneAppWindow(Window* win);

	bool isLocked() const { return m_state != StateUnlocked; }
	bool isDockMode() const { return m_state == StateDockMode; }

	void fadeWindow (bool in);

	bool handleFilteredSceneEvent(QEvent* event);
	
	bool okToResize();
	void resize(int width, int height);
	void delayDockModeLocking();
	void performDelayedLock();

	enum ScreenState {
		ScreenUnlocked = 0,
		ScreenLocking,
		ScreenLocked
	};


Q_SIGNALS:

	void signalScreenLocked(int screenState);

private Q_SLOTS:

	void slotDisplayStateChanged(int event);
	void slotLockStateChanged(int event, int displayEvent);

	void slotAlertActivated();
	void slotAlertDeactivated();
	void slotTransientAlertActivated();
	void slotTransientAlertDeactivated();
	void slotBannerActivated();
	void slotBannerDeactivated();
	void slotDeviceUnlocked();
	void slotCancelPasswordEntry();
	void slotPasswordSubmitted(QString password, bool isPIN);
	void slotPinPanelFocusRequest(bool focusRequest);
	void slotBannerAboutToUpdate(QRect& target);
	void slotBootFinished();
	void slotWindowUpdated(Window* win);
	void slotPolicyChanged(const EASPolicy * const policy);
	void slotSetLockTimeout(uint32_t timeout);
	void slotDialogButton1Pressed();
	void slotDialogButton2Pressed();
	void slotDialogButton3Pressed();

	void slotWindowFadeAnimationFinished();
	void slotVisibilityChanged();

//	void slotPhoneWindowAnimationFinished();

	void slotPositiveSpaceAboutToChange(const QRect& r, bool fullscreen, bool resizing);
	void slotPositiveSpaceChanged(const QRect& r);
	void slotPositiveSpaceChangeFinished(const QRect& r);

	void slotUiRotationCompleted();

private:

	enum State {
		StateUnlocked = 0,
		StateNormal,
		StateDockMode,
		StatePinEntry,
		StateLastTryDialog,
		StateNewPinDialog
//		, StateInPhoneCall // disabled for now in Dartfish
	};

	// QGraphicsItem::boundingRect
	QRectF boundingRect() const;

	// QGraphicsItem::paint
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void handlePowerOn();
	void handlePowerOff();

	void activatePopUpAlert();
	void activateBannerAlert();
	void registerBannerView();
	void unregisterBannerView();

	void changeState(State newState);

	void handlePenDownEvent(Event* event);
	void handlePenMoveEvent(Event* event);
	void handlePenUpEvent(Event* event);
	void handlePenFlickEvent(Event* event);
	void handlePenCancelEvent(Event* event);

	bool handleKeyDownEvent(Event* event);
	bool handleKeyUpEvent(Event* event);

	void handlePenDownStateNormal(Event* event);
	void handlePenMoveStateNormal(Event* event);
	void handlePenUpStateNormal(Event* event);
	void handlePenCancelStateNormal(Event* event);

	void mapCoordToWindow(Window* win, int& x, int& y) const;

	void tryUnlock();
	void unlock();
	bool requiresPasscode() const;

	// handles updating the screen (alerts/dashboard/clock)
	Timer<LockWindow> m_paintTimer;
	bool paint();

	Timer<LockWindow> m_hideHelpTimer;
	void startHideHelpTimer();
	bool hideHelpTimeout();
	void showHelp();
	void hideHelp(bool animate = true);

	void showPinPanel();
	void hidePinPanel(bool animate = true);

	void showDialog();
	void hideDialog(bool animate = true);

//	void showPhoneApp();
//	void hidePhoneApp(bool animate = true);
	
//	void toggleElemetsVisibleUnderPhoneApp(bool visible);

	void showAlert(TransparentNode* alertNode);
	void hideAlert(TransparentNode* alertNode);

	QRectF m_bounds;

	uint32_t m_lastLocked;
	uint32_t m_lockTimeout;
	bool setLockTimeout(uint32_t timeout);

	bool m_popupActive;
	bool m_bannerActive;
	bool m_activeCall;
	State m_state;
	
	bool m_elementsShown;
	
	bool m_delayDockModeLocking;

	int m_lockButtonX, m_lockButtonY;

	QPropertyAnimation m_windowFadeAnimation;
//	QPropertyAnimation m_phoneWinAnimation;

	// lock screen widgets
	LockBackground* m_bgNode;
	StatusBar*      m_statusBar;

	HelpWindow* m_helpWin;
	LockButton* m_lockButton;
	ClockWindow* m_clockWin;
	
//	int m_phoneAppShownY, m_phoneAppHiddenY;

	DashboardAlerts* m_dashboardAlerts;
	BannerAlerts* m_bannerAlerts;
	PopUpAlert* m_popUpAlert;

	GraphicsItemContainer* m_cornerContainer;
	QGraphicsPixmapItem* m_corners[4];
	void positionCornerWindows(const QRect& r);
	
	bool bannerViewRegistered;

//	CardWindow* m_phoneAppWin;

	QDeclarativeComponent* m_qmlUnlockPanel;
	QDeclarativeComponent* m_qmlUnlockDialog;
	InputItem* m_unlockPanel;
	QGraphicsObject* m_unlockDialog;


	QString m_newPasscode;
	bool m_setupNewPin;
	bool m_setupNewPassword;

	QRect m_targetPositiveSpace;
};

#endif

