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




#ifndef WINDOWSERVER_H
#define WINDOWSERVER_H

#include "Common.h"

#include <list>
#include <string>

#include "DisplayManager.h"
#include "InputManager.h"
#include "CustomEvents.h"
#include "VariantAnimation.h"
#include "HostWindow.h"
#include "ProgressAnimation.h"
#include "TouchPlot.h"

#include <QGraphicsView>
#include <QGraphicsObject>
#include <QTouchEvent>
#include <QObject>
#include <QQueue>
#include <QTime>
#include <QTimer>
#include <QPixmap>
#ifdef DEBUG_RECORD_PAINT
#include <QTextStream>
#include <QFile>
#endif

#include <QMultiMap>
#include <QPointer>

class Window;
struct WindowProperties;
class WindowManagerBase;
class MetaKeyManager;
class CoreNaviManager;
class ReticleItem;
class QDeclarativeEngine;
class SharedGlobalProperties;

class QGraphicsPixmapObject : public QGraphicsObject {
public:
	QGraphicsPixmapObject();
	~QGraphicsPixmapObject() ;
	QRectF boundingRect() const;

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	
	void setPixmap(QPixmap* img);
	void setInternalRotation(qreal rotation) { m_rotation = rotation; }
	qreal internalRotation() { return m_rotation; }
	
	QPixmap* getPixmap() const { return m_pixmap; }

	QPixmap* m_pixmap;
	QPainter::CompositionMode m_compMode;
	int m_topClipHeight;
	qreal m_rotation;
};

class UiRootItem : public QGraphicsItem {
public:
	UiRootItem() {
		setHandlesChildEvents(false);
		setFlag(QGraphicsItem::ItemHasNoContents, true);
		m_bounds = QRectF(0, 0, 0, 0);
	}

	~UiRootItem() {}
	QRectF boundingRect() const { return m_bounds; }
	void setBoundingRect(QRectF bounds) { prepareGeometryChange(); m_bounds = bounds; }

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) { }

	QRectF m_bounds;
};

class Runtime : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int orientation READ orientation NOTIFY orientationChanged)
	Q_PROPERTY(bool twelveHourClock READ twelveHourClock NOTIFY clockFormatChanged)

public:
	static Runtime* instance();

	enum Orientation { // these values are used in the DockModeTime QML app.
		Orientation_Portrait = 0,
		Orientation_Landscape = 1,
		Orientation_PortraitInverted = 2,
		Orientation_LandscapeInverted = 3
	};

    void setOrientation (OrientationEvent::Orientation orient);

    Q_INVOKABLE QString getLocalizedString(QString text);
        Q_INVOKABLE QString getLocalizedDay();
        Q_INVOKABLE QString getLocalizedMonth();
        Q_INVOKABLE QString getLocalizedAMPM();

	Q_INVOKABLE int orientation() { return m_orientation; }
	Q_INVOKABLE bool twelveHourClock() { return m_twelveHourClock; }

Q_SIGNALS:
	void orientationChanged();
	void clockFormatChanged();

public Q_SLOTS:
	void slotTimeFormatChanged(const char*);

private:
	Runtime(QObject *parent=0);

	int m_orientation;
	bool m_twelveHourClock;
};

typedef struct CardFlipRequest {
	QPointer<HostWindow> windowPtr;
	QRect                windowScreenBounds;
	bool                 sync;
} t_cardFlipRequest;



class WindowServer : public QGraphicsView
{
	Q_OBJECT

public:

	static WindowServer* instance();
	virtual ~WindowServer();

	static void registerWindow(Window* win);
	static void unregisterWindow(Window* win);
	static bool windowIsRegistered(Window* win);

	virtual void addWindow(Window* win);
	virtual void removeWindow(Window* win);
	virtual void focusWindow(Window* win);
	virtual void unfocusWindow(Window* win);
	virtual void windowUpdated(Window* win);
	virtual void prepareAddWindow(Window* win);
	virtual void addWindowTimedOut(Window* win);

	void bootupFinished();

	void startProgressAnimation(ProgressAnimation::Type type = ProgressAnimation::TypeHp);
	void stopProgressAnimation();
	bool progressRunning() const { return m_runningProgress; }

	virtual void startDrag(int x, int y, void* imgRef, const std::string& lpid);
	virtual void endDrag(int x, int y, const std::string& lpid, bool handled);

	virtual void applyLaunchFeedback(int centerX, int centerY);
	virtual void appLaunchPreventedUnderLowMemory() {}

	// following are not thread-safe
	DisplayManager* displayManager() const { return m_displayMgr; }
	MetaKeyManager* metaKeyManager() const { return m_metaKeyMgr; }
	CoreNaviManager* coreNaviManager() const { return m_coreNaviMgr; }

	bool takeScreenShot(const char* path);
	virtual QPixmap* takeScreenShot();

    OrientationEvent::Orientation getOrientation() const { return m_orientation; }
    OrientationEvent::Orientation getUiOrientation() const  { return m_currentUiOrientation; }

	void setPaintingDisabled(bool val);

	virtual void cancelVibrations();

	static void markBootStart();
	static void markBootFinish();

	void shutdown();

	void setWindowProperties (Window* win, const WindowProperties& props);

	WindowManagerBase* inputWindowManager() const {  return m_inputWindowMgr; }

	virtual WindowManagerBase* getWindowManagerByClassName(const QString& undecoratedClassName) const;
	virtual WindowManagerBase* windowManagerForWindow(Window* wm) const = 0;

	static void enableFpsCounter(bool enable);
	static void dumpFpsHistory();
	static void resetFpsBuffer(int newBufSize);

	static void enableTouchPlotOption(TouchPlot::TouchPlotOption_t type, bool enable);

	virtual void resizeWindowManagers(int width, int height);

	virtual bool okToResizeUi(bool ignorePendingRequests=false) { return true; }

	bool deviceIsPortraitType() const { return m_deviceIsPortraitType; }

	virtual QDeclarativeEngine* declarativeEngine() { return NULL; }

    // maps a rect given in item's coordinate space into 
    virtual QRectF mapRectToRoot(const QGraphicsItem* item, const QRectF& rect) const;

    // during a UI resize, offscreen cards are queued up for resizing (flip event) after the animation is complete
    virtual bool enqueueWindowForFlip(Window* window, QRect& windowScreenBoundaries, bool sync);
    virtual bool removeWindowFromFlipQueue(Window* window);
	enum RotationMode {
		RotationMode_FreeRotation           = 0,
		RotationMode_NoRotation             = 1,
		RotationMode_LimitedLandscape       = 2,
		RotationMode_LimitedPortrait        = 3,
		RotationMode_FixedPortrait          = 4,
		RotationMode_FixedLandscape         = 5,
		RotationMode_FixedPortraitInverted  = 6,
		RotationMode_FixedLandscapeInverted = 7
	};

	void setUiRotationMode(RotationMode uiRotationMode, bool cardMaximizing = false, bool skipAnimation = false);

    bool touchOnScreen() const { return m_fingerDownOnScreen; }

	static SharedGlobalProperties* globalProperties();

protected:

	enum UiRotationAnimationType {
		Rotation_NoAnimation = 0,
		Rotation_RotateAndCrossFade,
		Rotation_CrossFadeOnly
	};

	WindowServer();

	virtual bool handleEvent(QEvent* event);
	virtual bool viewportEvent(QEvent* event);
	virtual bool sysmgrEventFilters(QEvent* event) = 0;
	virtual bool eventFilter(QObject *, QEvent *);

	virtual bool processSystemShortcut( QEvent* event );

    void setOrientation(OrientationEvent::Orientation newOrient);
    int angleForOrientation(OrientationEvent::Orientation orient);

    virtual bool isOrientationAllowed(OrientationEvent::Orientation newOrient);

    void setUiOrientation(OrientationEvent::Orientation newOrient, UiRotationAnimationType animation = Rotation_RotateAndCrossFade);
    void rotateUi(OrientationEvent::Orientation newOrientation, UiRotationAnimationType animation = Rotation_RotateAndCrossFade, bool forceResize = false);
    virtual void rotatePendingWindows();
	virtual void cancelPendingWindowRotations();
    virtual void updateWallpaperForRotation(OrientationEvent::Orientation) { }

    // begin/end methods for caching and 'restoring' the QGraphicsScenes' currently focused 
    // QGraphicsItem. These are meant to wrap calls of setVisible(false) on WindowManagerBase's 
    // and the root ui item.
    // NOTE: a call to cacheFocusedItem MUST finish with a call restoreCachedFocusItem!
    void cacheFocusedItem();
    void restoreCachedFocusItem();

protected:

	DisplayManager* m_displayMgr;
	InputManager* m_inputMgr;
	MetaKeyManager* m_metaKeyMgr;
	CoreNaviManager* m_coreNaviMgr;

	WindowManagerBase* m_inputWindowMgr;

	uint32_t m_screenWidth;
	uint32_t m_screenHeight;
	bool     m_deviceIsPortraitType;

	QGraphicsItemGroup* m_uiElementsGroup;
	ReticleItem* m_reticle;

	bool m_bootingUp;

	bool   m_fingerDownOnScreen;

	bool m_runningProgress;
	ProgressAnimation* m_progressAnim;

	UiRootItem         m_uiRootItem;

	UiRotationAnimationType m_inRotationAnimation;

	RotationMode                    m_uiRotationMode;
    OrientationEvent::Orientation m_orientation;
    OrientationEvent::Orientation m_currentUiOrientation;
    OrientationEvent::Orientation m_pendingOrientation;
	UiRotationAnimationType         m_pendingRotationType;
	int m_animationTargetRotationAngle;
	QQueue<t_cardFlipRequest> m_pendingFlipRequests;


	//key: QMetaObject class name .. see getWindowManagerByClassName()
	// multi-map in case we decide to extend the system to support multiple window managers of the same type
    // in the system at the same time
	typedef QMap<QString,QPointer<WindowManagerBase> >::const_iterator WindowManagerMapConstIter;
	typedef QMap<QString,QPointer<WindowManagerBase> >::iterator WindowManagerMapIter;
	QMultiMap<QString,QPointer<WindowManagerBase> > m_windowManagerMap;

Q_SIGNALS:

	void signalWindowUpdated(Window*);
	void signalAboutToTakeScreenShot();
	void signalTookScreenShot();
	void signalUiRotated();
    void signalTouchesReleasedFromScreen();

private Q_SLOTS:

    void slotResizePendingTimerTicked();
    void slotRotationLockChanged(OrientationEvent::Orientation rotationLock);
	void slotProgressAnimationCompleted();
	void slotRotationAnimFinished();
	void rotationValueChanged(const QVariant& value);
	void slotDeferredNewOrientation();

private:

	void showReticle(const QPoint& pos);
	void gestureEvent(QGestureEvent* event);
	void paintBootupScreen();
	void enableOverlay(const QString& str);
	void disableOverlay();
	void takeAndSaveScreenShot();
	QImage getScreenShotImage();
	QImage getScreenShotImageFromFb();
    OrientationEvent::Orientation getInitialDeviceOrientation();

	bool m_screenShotImagesValid;
	QGraphicsPixmapObject *m_beforePixItem, *m_afterPixItem;
	QPixmap *m_rotationImageBeforePtr, *m_rotationImageAfterPtr;
	VariantAnimation<WindowServer>* m_rotationAnim;
	QTimer m_resizePendingTimer;
    QGraphicsItem* m_cachedFocusedItem;

	QTimer m_unaliasPaintEvent;
	QTime m_timeSinceLastPaint;
	QPixmap m_bootupScreen;

    OrientationEvent::Orientation m_deferredNewOrientation;
	QTimer m_deferredNewOrientationTimer;
	
#ifdef DEBUG_RECORD_PAINT
	void tracePaint(qreal durationMs);

	QTextStream *m_paintTrace;
	QFile m_paintTraceFile;
#endif
};


#endif /* WINDOWSERVER_H */
