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




#ifndef CARDWINDOW_H
#define CARDWINDOW_H

#include "Common.h"
#include "Settings.h"
#include "Event.h"
#include "HostWindow.h"
#include "AppLaunchOptionsEvent.h"
#include "SystemUiController.h"
#include <QGraphicsSceneMouseEvent>
#include <QTouchEvent>
#include <QPainterPath>
#include <QPainter>
#include <QVector3D>
#include <QTransform>
#include <QPointer>
#include <QPropertyAnimation>

#include "CardRoundedCornerShaderStage.h"

class CardLoading;
class PIpcMessage;
class FlickGesture;
class SingleClickGesture;
class CardGroup;
class GhostCard;
class CardRoundedCornerShaderStage;
class BearingEvent;

class CardWindow : public HostWindow
{
	Q_OBJECT
	Q_PROPERTY(CardWindow::Position position READ position WRITE setPosition)
    Q_PROPERTY(float dimming READ dimming WRITE setDimming)

public:

	CardWindow(Window::Type type, HostWindowData* data, IpcClientHost* clientHost=0);
	virtual ~CardWindow();
	
	virtual QRectF boundingRect() const { return m_boundingRect; }

	void onMessageReceived(const PIpcMessage& msg);

	bool fullScreen() const { return m_winProps.fullScreen; }
	bool isBlockScreenTimeout() const { return m_winProps.isBlockScreenTimeout; }
	bool isSubtleLightbar() const { return m_winProps.isSubtleLightbar; }
	bool activeTouchpanel() const { return m_winProps.activeTouchpanel; }
	bool alsDisabled() const { return m_winProps.alsDisabled; }
	bool gyroEvents() const { return m_winProps.gyroEnabled; }
	bool compassEvents() const { return m_winProps.compassEnabled; }
	uint32_t overlayNotificationsPosition() const { return m_winProps.overlayNotificationsPosition; }
	bool suppressBannerMessages() const { return m_winProps.suppressBannerMessages; }
	bool suppressGestures() const { return m_winProps.suppressGestures; }
	uint32_t dockBrightness() const { return m_winProps.dockBrightness; }
	bool hasCustomStatusBarColor() const { return (m_winProps.flags & WindowProperties::isSetStatusBarColor) != WindowProperties::isSetNothing; }
	unsigned int statusBarColor() const { return m_winProps.statusBarColor; }
	
	virtual void inputEvent(Event* e);
	virtual void resizeEvent(int w, int h);
	virtual void resizeEventSync(int w, int h);
	virtual void resizeWindowBufferEvent(int w, int h, QRect windowScreenBounds, bool forceSync=false, bool ignoreFixedOrient=false);
	virtual void flipEventSync(bool fromQueue = false);
	virtual void queueUpWindowForFlip(QRect windowScreenBounds, bool sync);
	virtual void queuedFlipCanceled(QRect windowScreenBounds);
	virtual void flipEventAsync(QRect windowScreenBounds, bool fromQueue = false);
	virtual void asynchFlipCompleted(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight);

	void queueFocusAction(bool focused);
	void performPendingFocusAction();
	virtual void focusEvent(bool enable);
	virtual void aboutToFocusEvent(bool enable) {}
	bool focused() const { return m_focused; }

	void enableShadow();
	void disableShadow();

	virtual void setPrepareAddedToWindowManager() { m_prepareAddedToWm = true; }
	bool prepareAddedToWindowManager() const { return m_prepareAddedToWm; }

	virtual void setAddedToWindowManager() { m_addedToWm = true; }
	bool addedToWindowManager() const { return m_addedToWm; }

	virtual void setWindowProperties (const WindowProperties& props);

	bool rotationLockMaximized() const;

	bool allowResizeOnPositiveSpaceChange() const;
	void adjustForPositiveSpaceSize(int width, int height) const;

	virtual Event::Orientation getCardFixedOrientation() { return m_appFixedOrientation; }

	virtual void displayOff() {}
	virtual void displayDimmed() {}

	virtual bool isHost() const { return false; }
	virtual bool supportsPause() const { return false; }
	virtual void pause() {}
	
	bool delayPrepare();
	void stopLoadingOverlay();

	void setModalParent(CardWindow* parent);
	void setCardIsModalParent(bool isParent) { m_isCardModalParent = isParent;}
	bool isCardModalParent() const { return m_isCardModalParent; }

	enum ModalAcceptInputState {
		NoModalWindow = 0,
		ModalLaunchedNotAcceptingInput,
		ModalLaunchedAcceptingInput
	};

	void setModalAcceptInputState(ModalAcceptInputState state) { m_modalAcceptInputState = state;}
	void setModalChild(CardWindow* w);
	CardWindow* getModalChild() const { return m_modalChild; }

	void setAppLaunchOptions(const AppLaunchOptionsEventWrapper& options);

	const std::string &splashBackgroundName() const {
        return m_splashBackgroundName;
    }

	void setLaunchInNewGroup(bool launchInNewGroup) {
		m_launchInNewGroup = launchInNewGroup;
	}

	bool launchInNewGroup() {
		return m_launchInNewGroup;
	}

	void setPaintCompositionMode(QPainter::CompositionMode mode);
	QPainterPath paintPath() const { return m_paintPath; }

	struct Position {
		QVector3D trans;
		qreal zRot;

		Position();
		bool operator ==(const Position& other);
		Position operator +(const Position& right) const;
		Position operator -(const Position& right) const;
		Position operator *(qreal right) const;

		QTransform toTransform() const;
	};

	CardWindow::Position position() const { return m_position; }
    void setPosition(const CardWindow::Position& pos);

	CardGroup* cardGroup() const;
	void setCardGroup(CardGroup* group);

	// an attached CardWindow will cause CardGroup x/y changes
	// to effect the x/y pos of the window
	bool attachedToGroup() const;
	void setAttachedToGroup(bool attached);

	virtual void setMaximized(bool enable);
	virtual bool isMaximized() const { return m_maximized; }

	// This are called only when Card is maximized
	virtual void positiveSpaceAboutToChange(const QRect& r, bool fullScreen);
	virtual void positiveSpaceChanged(const QRect& r);
	virtual void positiveSpaceChangeFinished(const QRect& r);

    //cheats for telling the loading section that it is infact, big, if you fix this the right way, please remove
    virtual void setMaxAndLoading(bool enable);
    virtual bool isMaxAndLoading() const { return m_maxAndLoading; }

	void setBoundingRect(int width, int height) {

		int visibleWidth = width;
		int visibleHeight = height;

		if(type() != Window::Type_ModalChildWindowCard) {
			m_boundingRect.setRect(-width/2, -height/2, width, height);
		}
		else {

			// If we have the right width/height ignore
			if(boundingRect().width() == visibleWidth && boundingRect().height() == visibleHeight)
				return;

			qreal curY = boundingRect().y();
			//g_debug("__BEFORE KARTHIK CardWindow::setBoundingRect w: %d h: %d x: %f y: %f w: %f h: %f rotating: %d", width, height, boundingRect().x(), boundingRect().y(), boundingRect().width(), boundingRect().height(), SystemUiController::instance()->isUiRotating());
			m_boundingRect.setRect(-width/2, curY, width, height);
			//g_debug("__AFTER KARTHIK CardWindow::setBoundingRect w: %d h: %d x: %f y: %f w: %f h: %f rotating: %d", width, height, boundingRect().x(), boundingRect().y(), boundingRect().width(), boundingRect().height(), SystemUiController::instance()->isUiRotating());
		}

		setVisibleDimensions(visibleWidth, visibleHeight);
	}

	void computeModalWindowPlacementInf(int newPosSpace);
	virtual QRectF transitionBoundingRect();

    GhostCard* createGhost();

	void allowUpdates(bool allow);

    void setDimm(bool dimm);
    float dimming() const { return m_dimming; }
    void setDimming(float dimming) { m_dimming = dimming; update(); }

protected Q_SLOTS:

	void slotLoadingFinished();
	void slotDisplayStateChanged(int state);
	virtual void slotUiRotated();

	void slotShowIME();
	void slotHideIME();

protected:

	CardWindow(Window::Type type, const QPixmap& pixmap);
    void init();
	
	virtual void setVisibleDimensions(int width, int height);

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	virtual void paintBase(QPainter* painter, bool maximized);
	virtual void paintOverlay(QPainter* painter, bool maximized) {}

	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);

	virtual bool sceneEvent(QEvent* event);
	virtual bool touchEvent(QTouchEvent* event);

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

	bool mouseFlickEvent(QGestureEvent* event);
	bool pinchEvent(QGestureEvent* event);
	bool mouseSingleClickEvent(QGestureEvent* singleClick);
	bool coversScreenFully() const;
	void enableFullScreen();
	void disableFullScreen();
	virtual void fullScreenEnabled(bool val);
	void updateDirectRenderingPosition();

	// loading animation methods
	void startLoadingOverlay();
	bool loadingAnimTimerTicked();

	// window timeout methods
	void startLoadingTimer(int duration);
	void stopLoadingTimer();
	bool hasLoadingTimer() const { return m_loadingTimerId != 0; }
	static gboolean loadingTimeout(gpointer data);
	
	virtual void onEnableTouchEvents(bool enableTouchEvents);

	virtual void mapCoordinates(qreal& x, qreal& y);
	virtual void mapFlickVelocities(qreal& x, qreal& y);
	virtual void onSetAppFixedOrientation(int orientation, bool isPortrait);
	virtual void onSetAppOrientation(int orientation) {}
	virtual void onSetAppFreeOrientation(bool free) {}
	virtual void refreshAdjustmentAngle();

    virtual void initializeRoundedCornerStage();

	bool isInValidOrientation();

protected:
	
	enum PendingFocus {
		PendingFocusNone = 0,
		PendingFocusTrue,
		PendingFocusFalse
	};

	enum CapturedEvents {
		CapturedNone    = 0,
		CapturedPen     = 1 << 0,
		CapturedGesture = 1 << 1,
		CapturedInvalid = 1 << 31
	};

	bool m_prepareAddedToWm;
	bool m_addedToWm;
	uint32_t m_capturedEvents;
	bool m_fullScreenEnabled;
	bool m_fullScreenModeSuppressed;
	bool m_maximized;

    //hack to tell the loading anime that it is big
    bool m_maxAndLoading;

	Event::Orientation   m_appFixedOrientation;
	Event::Orientation   m_appPendingFixedOrientation;
	qreal m_adjustmentAngle;
	bool m_focusPendingRotation;
	bool m_keyboardShownMessageSent;


	// used for asynchronous resizing (flip)
	bool m_isResizing;
	int  m_flipsQueuedUp;
	bool m_pendingDirectRenderingEnable;
	int  m_pendingRotationRequests;
	int  m_ignorePendingRotationRequests;
	QBrush m_tempRotatedBrush;
	QRectF m_preFlipBoundingRect;

	bool m_forceFocus;
	bool m_focused;
	bool m_touchEventsEnabled;

	QRectF m_boundingRect;

	PendingFocus m_pendingFocus;

	WindowProperties m_winProps;
		
	CardLoading* m_loadingAnim;
	guint m_loadingTimerId;

	std::string m_splashBackgroundName;

	QPainterPath m_paintPath;
	QPainter::CompositionMode m_compMode;

	Position m_position;

	// reference to card group
	QPointer<CardGroup> m_group;
	bool m_attachedToGroup;

	bool m_launchInNewGroup;

	CardRoundedCornerShaderStage* m_roundedCornerShaderStage;
	
	QPropertyAnimation* m_activeAnimation;
    float m_dimming;

	virtual void customEvent ( QEvent* event );

	enum ModalCardResizeState {
		Invalid=0,
		CardHeightNotChanged,
		HeightAndPositionUnchanged,
		HeightUnchangedAnimatedUp,
		HeightUnchangedAnimatedDown,
		ReduceCardHeight,
		IncreaseCardHeight
	};

	// This struct holds all the necessary information about the position of the modal window
	struct ModalWindowPlacementInfo {
		int m_originalY;
		int m_positiveSpaceThreshold_Below;
		int m_positiveSpaceThreshold_Above;
		int m_negativeSpaceConsumed;
		int m_cardMoveDelta;
		int m_cardMoveDelataUnsigned;
		bool m_IsAnimating;
		bool m_centerModalWindow;
		ModalCardResizeState m_ModalCardResizeInf;

		ModalWindowPlacementInfo() {
			m_originalY = 0;
			m_positiveSpaceThreshold_Below = -1;
			m_positiveSpaceThreshold_Above = -1;
			m_negativeSpaceConsumed = 0;
			m_cardMoveDelta = 0;
			m_cardMoveDelataUnsigned = 0;
			m_IsAnimating = false;
			m_ModalCardResizeInf = Invalid;
			m_centerModalWindow = false;
		}

		void computeUnsignedCardMoveDelta() {
			m_cardMoveDelataUnsigned = (m_cardMoveDelta < 0) ? (m_cardMoveDelta * -1):m_cardMoveDelta;
		}

		void setCardMoveDelta(int val) {
			if(val != m_cardMoveDelta) {
				m_cardMoveDelta = val;
				computeUnsignedCardMoveDelta();
			}
		}

		void reset()
		{
			m_originalY = 0;
			m_positiveSpaceThreshold_Below = -1;
			m_positiveSpaceThreshold_Above = -1;
			m_negativeSpaceConsumed = 0;
			m_cardMoveDelta = 0;
			m_cardMoveDelataUnsigned = 0;
			m_IsAnimating = false;
			m_ModalCardResizeInf = Invalid;
		}
	};

	enum ParentWindowHandleEvent {
		ParentHandleEvent=0,
		ForwardEventToChild,
		WaitForChildToAcceptEvents
	};

	enum PositiveSpaceChangeNotificationState {
		Unknown = 0,
		GotPositiveSpaceAboutToChangeNotification,
		GotPositiveSpaceChangeNotification,
	};

	ModalAcceptInputState m_modalAcceptInputState;
	ModalWindowPlacementInfo m_initMWindPlacementInf;
	PositiveSpaceChangeNotificationState m_posSpChangeNotificationState;

	int m_ModalWindowEndY;
	int m_modalWindowShrinkHeight;
	int m_maxEndingPositionForOrientation;
	bool m_fRecomputeInitPositionsValues;
	bool m_isCardModalParent;

	static int sStartSpaceChangeValue;
	static int sLastKnownPositiveSpace;
	static float sBoundingRectYBeforeResize;
	static QPointF sModalWindowYBeforeResize;

	QPropertyAnimation* m_ModalPositionAnimation;

private:
	CardWindow* m_modalChild;
	CardWindow* m_modalParent;

	QPointF positionModalWindowWrpParent(int spaceAvailableAbove, int windowHeight);
	void positionModalForMorePositiveSpace(int availablePosSpace, int newPositiveSpace);
	void positionModalForLessPositiveSpace(int availablePosSpace, int newPositiveSpace);

	void increaseHeightAndPositionModalCard(int availablePosSpace, int newPositiveSpace);
	void decreaseHeightAndPositionModalCard(int availablePosSpace, int newPositiveSpace);

	void resetModalWindowPositionInfo();
	void startModalAnimation();
	void resizeModalCard();
	void centerModal(int availablePositiveSpace);
	bool canPositionModalAtY(int yLoc, bool increasePositiveSpace, int& correctYLoc);

	int forwardToModal();
};

QDebug operator<<(QDebug, const CardWindow::Position &);

QVariant positionInterpolator(const CardWindow::Position &start, const CardWindow::Position &end, qreal progress);

Q_DECLARE_METATYPE(CardWindow*)

#endif /* CARDWINDOW_H */
