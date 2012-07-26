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




#ifndef WINDOWSERVERLUNA_H
#define WINDOWSERVERLUNA_H

#include "Common.h"

#include "WindowServer.h"
#include "HostWindow.h"

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QTimer>
#include <QQueue>
#include <QWeakPointer>

#include "QmlAlertWindow.h"
#include "VariantAnimation.h"

class WindowManagerBase;
class QGraphicsPixmapItem;
class FullEraseConfirmationWindow;
class QDeclarativeEngine;


class WindowServerLuna : public WindowServer
{
	Q_OBJECT

public:

	WindowServerLuna();
	virtual ~WindowServerLuna();

	virtual void prepareAddWindow(Window* win);
	virtual void addWindow(Window* win);
	virtual void addWindowTimedOut(Window* win);
	virtual void removeWindow(Window* win);
	virtual void focusWindow(Window* win);
	virtual void unfocusWindow(Window* win);
	virtual void windowUpdated(Window* win);
	virtual void startDrag(int x, int y, void* imgRef, const std::string& lpid);
	virtual void endDrag(int x, int y, const std::string& lpid, bool handled);

	virtual void applyLaunchFeedback(int centerX, int centerY);
	virtual void appLaunchPreventedUnderLowMemory();

	WindowManagerBase* overlayWindowManager() const { return m_overlayMgr; }
	WindowManagerBase* cardWindowManager() const { return m_cardMgr; }
	WindowManagerBase* dashboardWindowManager() const { return m_dashboardMgr; }
	WindowManagerBase* menuWindowManager() const { return m_menuMgr; }
	WindowManagerBase* topLevelWindowManager() const { return m_topLevelMgr; }
	WindowManagerBase* emergencyModeWindowManager() const { return m_emergencyModeMgr; }
	WindowManagerBase* inputWindowManager() const { return m_inputWindowMgr; }
	WindowManagerBase* dockModeMenuManager() const { return m_dockModeMenuMgr; }
	WindowManagerBase* dockModeManager() const { return m_dockModeMgr; }

	virtual WindowManagerBase* windowManagerForWindow(Window* wm) const;

	virtual bool okToResizeUi(bool ignorePendingRequests=false);
	void resizeWindowManagers(int width, int height);
	virtual QDeclarativeEngine* declarativeEngine() { return m_qmlEngine; }

	virtual QRectF mapRectToRoot(const QGraphicsItem* item, const QRectF& rect) const;

Q_SIGNALS:

	void signalWallpaperImageChanged(QPixmap* wallpaper, bool fullScreen, int rotationAngle);
	void signalDockModeAnimationStarted();
	void signalDockModeAnimationComplete();

private Q_SLOTS:
	void slotWallPaperChanged(const char* filePath);
	void slotDisplayStateChange(int state);
	void slotScreenLockStatusChanged(bool locked);
	void slotDockAnimationFinished();
	void slotDockModeEnable(bool enabled);
	void slotPuckConnected(bool connected);
	void slotFullEraseDevice();
	void slotShowFullEraseWindow();
	void slotMemoryStateChanged(bool critical);
	void slotAppLaunchPreventedUnderLowMemory();
	void slotBrickModeFailed();
        void slotFirstCardRun();

private:

	WindowManagerBase* windowManagerForWindowType(int type) const;
	
	bool sysmgrEventFilters(QEvent* event);
	
	void initDockModeAnimations();
	void setupDockModeAnimations();
	void cleanupDockModeAnimations();
	void dockModeUiTransition(bool enter);
	void animateDockMode(bool in);
	void reorderWindowManagersForDockMode(bool enabled);

	bool triggerFullEraseCountdown();
	void cancelFullEraseCountdown();
	static bool cbFullEraseCallback(LSHandle*, LSMessage*, void*);

	bool processSystemShortcut( QEvent* event );
	
	QPixmap* takeScreenShot();
	
	void generateWallpaperImages();
    void updateWallpaperForRotation(OrientationEvent::Orientation newOrient);
	void drawBackground ( QPainter * painter, const QRectF & rect );

	void createMemoryAlertWindow();
	void createMsmEntryFailedAlertWindow();
        void createDismissCardWindow();

	WindowManagerBase* m_overlayMgr;
	WindowManagerBase* m_cardMgr;
	WindowManagerBase* m_dashboardMgr;
	WindowManagerBase* m_menuMgr;
	WindowManagerBase* m_topLevelMgr;
	WindowManagerBase* m_emergencyModeMgr;
	WindowManagerBase* m_dockModeMenuMgr;
	WindowManagerBase* m_dockModeMgr;

	QDeclarativeEngine* m_qmlEngine;

	QString  m_wallpaperFileName;
	QPixmap  m_normalWallpaperImage, m_rotatedWallpaperImage;
	QPixmap* m_currWallpaperImg;
	bool     m_drawWallpaper;
	bool     m_wallpaperFullScreen;

	QBrush m_oldBrush;
	bool m_restoreBrush;
	QPixmap *m_screenShot, *m_dockImage;
	QGraphicsPixmapObject *m_screenShotObject, *m_dockImageObject;	
	QPropertyAnimation m_screenFade, m_dockFade;
	QPropertyAnimation m_screenScale, m_dockScale;
	QParallelAnimationGroup m_screenGroup, m_dockGroup;
	QSequentialAnimationGroup m_dockSequence, m_dockModeAnimation;

	struct PowerVolumeKeyComboState {
		bool powerKeyPress;
		bool volDownKeyPress;
		bool volUpKeyPress;

		PowerVolumeKeyComboState()
			: powerKeyPress(false)
			, volDownKeyPress(false)
			, volUpKeyPress(false) {
		}

		bool fullEraseComboDown() const {
			return powerKeyPress && volUpKeyPress;
		}

		bool msmEntryComboDown() const {
			return powerKeyPress && volDownKeyPress;
		}

		bool comboDown() const {
			return fullEraseComboDown() || msmEntryComboDown();
		}

		void reset() {
			powerKeyPress = volDownKeyPress = volUpKeyPress = false;
		}
	};

	PowerVolumeKeyComboState m_powerVolumeKeyComboState;
	FullEraseConfirmationWindow* m_fullEraseConfirmationWindow;
	bool m_fullErasePending;

	bool m_dashboardOpenInDockMode;
	bool m_inDockModeTransition;
	bool m_dockModeTransitionDirection;

	QWeakPointer<QmlAlertWindow> m_memoryAlert;
	QWeakPointer<QmlAlertWindow> m_msmEntryFailedAlert;
        QWeakPointer<QmlAlertWindow> m_dismissCardDialog;
};

#endif /* WINDOWSERVERLUNA_H */
