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




#ifndef WINDOW_H
#define WINDOW_H

#include "Common.h"

#include <stdint.h>
#include <string>
#include <glib.h>

#include <QGraphicsObject>
#include <QSize>


class ApplicationDescription;
class WindowManagerBase;
struct WindowProperties;

/**
  * Mandatory Qt signal/slot connection between another object's signal
  * and this object's slot.
  */
#define QM_CONNECT(source, sSignal, tSlot)                                 \
    do {                                                                   \
        if (!connect(source, sSignal, tSlot)) {                            \
            qFatal("Failed to connect signal %s::%s to this::%s",          \
                    #source, sSignal, tSlot);                              \
        }                                                                  \
    } while (0)

/**
  * Mandatory Qt signal/slot connection between the target slot and the source
  * signal.
  */
#define QM_CONNECT_EX(target, source, sSignal, tSlot)                      \
    do {                                                                   \
        if (target == NULL) qFatal("Cannot connect to NULL object");       \
        if (!(target)->connect(source, sSignal, tSlot)) {                  \
            qFatal("Failed to connect signal %s::%s to %s::%s",            \
                    #source, sSignal, #target, tSlot);                     \
        }                                                                  \
    } while (0)

class Window : public QGraphicsObject
{
	Q_OBJECT
	Q_PROPERTY(bool grabMouse READ mouseGrabbed WRITE setMouseGrabbed)
public:

	enum Type {
		Type_Invalid    			= UserType + 1,
		Type_StatusBar  			= UserType + 2,
		Type_Card       			= UserType + 3,
		Type_ChildCard  			= UserType + 4,
		Type_Overlay    			= UserType + 5,
		Type_Launcher				= UserType + 6,
		Type_Dashboard  			= UserType + 7,
		Type_PopupAlert  			= UserType + 8,
		Type_BannerAlert 			= UserType + 9,
		Type_Menu        			= UserType + 10,
		Type_PIN					= UserType + 11,
		Type_Emergency  			= UserType + 12,
		Type_QtNativePaintWindow		= UserType + 13,
		Type_DockModeWindow 			= UserType + 14,
		Type_DockModeLoadingWindow 		= UserType + 15,
		Type_ModalChildWindowCard 		= UserType + 16,
		Type_None       			= UserType + 32 // arbitrary, can be as large as 0xFFFEFFFF
	};

	Window(Type type, const uint32_t bufWidth, const uint32_t bufHeight, bool hasAlpha=false);
	Window(Type type, const QPixmap& pix);
	virtual ~Window();

	// QGraphicsItem::type
	virtual int type() const { return m_type; }				// has-base-impl

	// QGraphicsItem::boundingRect
	virtual QRectF boundingRect() const { return m_visibleBounds; }					// has-base-impl

	virtual bool mouseGrabbed() const;				// has-base-impl
	virtual void setMouseGrabbed(bool grabbed);				// has-base-impl

	// QGraphicsItem::paint
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);				// has-base-impl

	virtual bool isIpcWindow() const { return false; }				// has-base-impl

	void setName(const std::string& name);
	std::string name() const;
	virtual void setAppId(const std::string& id);
	std::string appId() const;
	void setProcessId(const std::string& id);
	std::string processId() const;
	void setLaunchingAppId(const std::string& id);
	std::string launchingAppId() const;
	void setLaunchingProcessId(const std::string& id);
	std::string launchingProcessId() const;
	ApplicationDescription* appDescription() const;

	void setRemoved() { m_removed = true; }
	bool removed() const { return m_removed; }

    void setDisableKeepAlive() { m_disableKeepAlive = true; }
    bool disableKeepAlive() const { return m_disableKeepAlive; }

	virtual void setWindowProperties (const WindowProperties& attr) { }

	virtual void setVisibleDimensions(uint32_t width, uint32_t height);				// has-base-impl
	virtual QSize getVisibleDimensions() const;				// has-base-impl

	virtual void resize(int w, int h);			//has-base-impl

	virtual const QPixmap* acquireScreenPixmap() { return &m_screenPixmap; }

	inline int initialWidth() const { return m_initialWidth; }
	inline int initialHeight() const { return m_initialHeight; }

protected:

	virtual void lock() {}
	virtual void unlock() {}

	Type m_type;
	std::string m_name;
	std::string m_appId;
	std::string m_processId;
	std::string m_launchingAppId;
	std::string m_launchingProcId;
	mutable ApplicationDescription* m_appDesc;
	bool m_removed;
    bool m_disableKeepAlive;
	uint32_t m_bufWidth;
	uint32_t m_bufHeight;
	int m_initialWidth;
	int m_initialHeight;
	QRectF m_visibleBounds;
	QPixmap m_screenPixmap;

    friend class HostWindow;

    Window(const Window&);
    Window& operator=(const Window&);
};



struct WindowProperties {
    enum {
		isSetNothing                 = 0,
		isSetBlockScreenTimeout      = 1 << 0,
		isSetSubtleLightbar          =  1 << 1,
		isSetFullScreen              = 1 << 2,
		isSetFastAccelerometer       = 1 << 3,
		isSetOverlayNotifications    = 1 << 4,
		isSetSuppressBannerMessages  = 1 << 5,
		isSetHasPauseUi              = 1 << 6,
		isSetSuppressGestures        = 1 << 7,
		isSetDashboardManualDragMode = 1 << 9,
		isSetStatusBarColor 	     = 1 << 10,
		isSetRotationLockMaximized	 = 1 << 11,
		isSetAllowResizeOnPositiveSpaceChange = 1 << 12,
		isSetEnableCompassEvents  	 = 1 << 13,
		isSetGyro					 = 1 << 14,
		isSetActiveTouchpanel       = 1 << 15,
		isSetAlsDisabled       = 1 << 16,
		isSetLast
    };

	enum {
		OverlayNotificationsBottom = 0,
		OverlayNotificationsLeft,
		OverlayNotificationsRight,
		OverlayNotificationsTop,
		OverlayNotificationsLast
	};

    unsigned int  flags;

	bool    isBlockScreenTimeout;
	bool    isSubtleLightbar;
	bool    fullScreen;
	bool    activeTouchpanel;
	bool	alsDisabled;
	unsigned int overlayNotificationsPosition;
	bool    suppressBannerMessages;
	bool    hasPauseUi;
	bool    suppressGestures;
	bool	dashboardManualDrag;
	unsigned int dockBrightness;
	unsigned int statusBarColor;
	bool	rotationLockMaximized;
	bool	allowResizeOnPositiveSpaceChange;
	bool 	gyroEnabled;
	bool 	compassEnabled;

    WindowProperties()
		: flags(0)
		, isBlockScreenTimeout(false)
		, isSubtleLightbar(false)
		, fullScreen(false)
		, activeTouchpanel(false)
		, alsDisabled (false)
		, overlayNotificationsPosition(OverlayNotificationsBottom)
		, suppressBannerMessages(false)
		, hasPauseUi(false)
		, suppressGestures(false)
		, dockBrightness(100)
		, dashboardManualDrag(false)
		, statusBarColor(0x00000000)
		, rotationLockMaximized(false)
		, allowResizeOnPositiveSpaceChange(true)
		, gyroEnabled(false)
		, compassEnabled(false)
	{
	}

	void merge(const WindowProperties& props);

    // convenience functions
	void setBlockScreenTimeout (bool enable) { flags |= isSetBlockScreenTimeout; isBlockScreenTimeout = enable; }
	void setSubtleLightbar (bool enable) { flags |= isSetSubtleLightbar; isSubtleLightbar = enable; }
	void setActiveTouchpanel(bool enable) { flags |= isSetActiveTouchpanel; activeTouchpanel = enable; }
	void setAlsDisabled(bool disable) { flags |= isSetAlsDisabled; alsDisabled = disable; }
	void setFullScreen(bool enable) { flags |= isSetFullScreen; fullScreen = enable; }
	void setOverlayNotificationsPosition(unsigned int position);
	void setSuppressBannerMessages(bool enable) { flags |= isSetSuppressBannerMessages; suppressBannerMessages = enable; }
	void setHasPauseUi(bool val) { flags |= isSetHasPauseUi; hasPauseUi = val; }
	void setSuppressGestures(bool val) { flags |= isSetSuppressGestures; suppressGestures = val; }
	void setDashboardManualDragMode(bool isManual) { flags |= isSetDashboardManualDragMode; dashboardManualDrag = isManual; }
	void setStatusBarColor(unsigned int color) { flags |= isSetStatusBarColor; statusBarColor = color; }
	void setRotationLockMaximized(bool enable) { flags |= isSetRotationLockMaximized; rotationLockMaximized = enable;}
	void setAllowResizeOnPositiveSpaceChange(bool allow) { flags |= isSetAllowResizeOnPositiveSpaceChange; allowResizeOnPositiveSpaceChange = allow; }
	void setAllowGyroEvents(bool allow) { flags |= isSetGyro; gyroEnabled = allow; }
	void setCompassEvents(bool enable) { flags |= isSetEnableCompassEvents; compassEnabled = enable; }
};

#endif /* WINDOW_H */
