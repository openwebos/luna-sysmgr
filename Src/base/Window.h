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
#include <WindowTypes.h>

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

    Window(WindowType::Type type, const uint32_t bufWidth, const uint32_t bufHeight, bool hasAlpha=false);
    Window(WindowType::Type type, const QPixmap& pix);
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

    WindowType::Type m_type;
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

#endif /* WINDOW_H */
