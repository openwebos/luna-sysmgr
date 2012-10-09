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



#ifndef __CardWebApp_h__
#define __CardWebApp_h__

#include "Common.h"

#include <sptr.h>

#include "WindowedWebApp.h"
#include "SysMgrDefs.h"
#include <QGraphicsItem>
#include <QGraphicsView>

class PIpcChannel;
class NativeGraphicsContext;
class NativeGraphicsSurface;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsWebView;
class QGLWidget;
class myGraphicsView;

class CardWebApp : public WindowedWebApp, public QGraphicsView
{
public:

	CardWebApp(Window::Type winType, PIpcChannel *channel, ApplicationDescription* desc = 0);
	~CardWebApp( );

	virtual void thawFromCache();
	virtual void freezeInCache();

	virtual bool isCardApp() const { return true; }
	virtual bool isChildApp() const;

    virtual void paint();


	virtual void inputEvent(sptr<Event> e);
	virtual void keyEvent(QKeyEvent* e);
	virtual void focusedEvent(bool focused);
	virtual int  resizeEvent(int newWidth, int newHeight, bool resizeBuffer);
	virtual void flipEvent(int newWidth, int newHeight);
	virtual void asyncFlipEvent(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight);

	virtual void setOrientation(Event::Orientation orient);
	Event::Orientation orientation() const;
	
	void setFixedOrientation(Event::Orientation orient);
	void setAllowOrientationChange(bool value);
	bool allowsOrientationChange() const;

	virtual void enableFullScreenMode(bool enable);
	
	void receivePageUpDownInLandscape(bool val);

	virtual void invalidate();

	virtual void displayOn();
	virtual void displayOff();

	void allowResizeOnPositiveSpaceChange(bool allowResize);

	bool isRenderingSuspended() { return m_renderingSuspended; }

	CardWebApp* parentWebApp() const;
    void resizeWebPage(uint32_t width, uint32_t height);
	
private:

	virtual void attach(SysMgrWebBridge* page);
	virtual SysMgrWebBridge* detach();
	virtual bool isWindowed() const;
    virtual bool isLeafApp() const;

	void addChildCardWebApp(CardWebApp* app);
	void removeChildCardWebApp(CardWebApp* app);
	void setParentCardWebApp(CardWebApp* app);

	virtual void resizeWindowForOrientation(Event::Orientation orient);
	virtual void resizeWindowForFixedOrientation(Event::Orientation orient);
	virtual void getFixedOrientationDimensions(int& width, int& height, int& wAdjust, int& hAdjust);
	bool isOrientationPortrait(Event::Orientation orient);
	void handlePendingChanges();
	
    virtual void invalContents(int x, int y, int width, int height);
	virtual void loadFinished();

	void callMojoScreenOrientationChange();
	void callMojoScreenOrientationChange(Event::Orientation orient);
	virtual void onSetComposingText(const std::string& text);
	virtual void onCommitComposingText();
	virtual void onCommitText(const std::string& text);
	virtual void onPerformEditorAction(int action);
	virtual void onRemoveInputFocus();

    virtual void onInputEvent(const SysMgrEventWrapper& wrapper);
    virtual Event::Orientation orientationForThisCard(Event::Orientation orient);

protected:
	int  angleForOrientation(Event::Orientation orient) const;
	void updateWindowProperties();
	void animationFinished();

	virtual void setVisibleDimensions(int width, int height);
	virtual void onDirectRenderingChanged();

    virtual void directRenderingChanged(bool directRendering, int renderX, int renderY, SysMgrEvent::Orientation angle);
	virtual void directRenderingAllowed();
	virtual void directRenderingDisallowed();

	virtual void focus();
	virtual void screenSize(int& width, int& height);

    void forcePaint();

/*!
    \brief Rotates the card according to the card orientation in case of direct rendering

    This method only performs the card rotation for direct rendering. Otherwise the window manager
    handles the rotation transformation.
*/
    void applyCardOrientation();

public:
	virtual void suspendAppRendering();
	virtual void resumeAppRendering();

protected: // QGraphicsView overloads
#ifdef GFX_DEBUGGING
    bool event(QEvent* event);
    bool viewportEvent(QEvent* event);
#endif
    void paintEvent(QPaintEvent* event);

protected:

	CardWebApp* m_parentWebApp;
	CardWebApp* m_childWebApp;
	
	// these are the dimensions of the paint buffer seen by webkit
	// (are adjusted to app rotation)
	int m_appBufWidth;
	int m_appBufHeight;
		
    // m_CardOrientation is to keep track of the current orientation
    // of the card. m_orientation & m_fixedOrientation cannot be used
    // reliably always to query current orientation of the CardWebApp
    Event::Orientation m_CardOrientation;
	Event::Orientation m_orientation;
	Event::Orientation m_fixedOrientation;
	bool m_allowsOrientationChange;

	int m_currAngleForAnim;
	int m_targAngleForAnim;
	
	int m_pendingResizeWidth;
	int m_pendingResizeHeight;	
	Event::Orientation m_pendingOrientation;
	int m_pendingFullScreenMode;

	int m_setWindowWidth;
	int m_setWindowHeight;

	bool m_enableFullScreen;

	bool m_doPageUpDownInLandscape;

	bool m_directRendering;
	int m_renderOffsetX;
	int m_renderOffsetY;
    SysMgrEvent::Orientation m_renderOrientation;
	bool m_paintingDisabled;

	bool m_renderingSuspended;
	
    QGraphicsWebView* m_webview;
    QGLWidget* m_glw;
    bool m_lastPaintIPCBuffer;
private:
	
	CardWebApp& operator=( const CardWebApp& );
	CardWebApp( const CardWebApp& );
};

#endif


