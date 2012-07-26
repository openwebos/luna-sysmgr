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




#ifndef PAGE_H_
#define PAGE_H_

#include <QUuid>
#include <QRectF>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QBrush>
#include <QObject>

#include "thingpaintable.h"
#include "icon.h"
#include "pixmapobject.h"
#include "frictiontransform.h"
#include "dimensionstypes.h"

class LauncherObject;
class QGraphicsSceneMouseEvent;
class QGesture;
class QGestureEvent;
class QTouchEvent;
class QPanGesture;
class QSwipeGesture;
class QPinchGesture;
class QTapAndHoldGesture;
class QTapGesture;
class FlickGesture;
class ScrollableObject;
class IconBase;
class IconLayout;
class QAbstractAnimation;
class QAnimationGroup;
class QState;
class QStateMachine;

#include "KineticScroller.h"

namespace PageScrollDirectionLock
{
	enum Enum
	{
		INVALID,
		None,
		Horizontal,
		Vertical
	};
}

namespace PageAreas
{
	enum Enum
	{
		INVALID,
		Content,			//inner area of the page; means "normal", nothing special.
		LeftBorder,		// the left edge, usually designated to trigger a horizontal page pan left
		RightBorder,	// the right edge...
		TopBorder,			// the top, same idea as left/right but for vertical scrolls in the page
		BottomBorder		// ....
	};
}

namespace PageMode
{
	enum Enum
	{
		INVALID,
		Normal,
		Reorder
	};
}

class PageTouchRedirectContext : public RedirectContext
{
	Q_OBJECT
public:
	PageTouchRedirectContext() {}
	virtual ~PageTouchRedirectContext() {}
};

class PageIconTransferRedirectContext : public PageTouchRedirectContext
{
	Q_OBJECT
public:
	PageIconTransferRedirectContext(Page * p_srcPage,IconBase * p_icon)
	: m_qp_srcPage(p_srcPage) , m_qp_icon(p_icon) { m_valid = true; }
	virtual ~PageIconTransferRedirectContext() {}
	QPointer<Page> m_qp_srcPage;
	QPointer<IconBase> m_qp_icon;
};

class Page : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	Q_PROPERTY(qint32 pageuiindex READ pageIndex WRITE setPageIndex NOTIFY signalPageIndexChanged)
	Q_PROPERTY(QString pagename READ pageName WRITE setPageName NOTIFY signalPageNameChanged)
	Q_PROPERTY(QString pagedesignator READ pageDesignator WRITE setPageDesignator NOTIFY signalPageDesignatorChanged)
	Q_PROPERTY(bool pageactive READ pageActive)

public:

	friend class PageRestore;
	static const char * PageNamePropertyName;
	static const char * PageIndexPropertyName;
	static const char * PageDesignatorPropertyName;
	static QString PageDesignator_AllAlphabetic;
	static QString PageDesignator_Favorites;
	static QString PageDesignatorAll() { return PageDesignator_AllAlphabetic; }
	static QString PageDesignatorFavorites() { return PageDesignator_Favorites; }
	friend class QTimer;		//give it access to my stuff for page physics
	friend class LauncherObject;

/// ----------------------------------------------

	Q_INVOKABLE Page(const QRectF& pageGeometry,LauncherObject * p_belongsTo);
	virtual ~Page();

	qint32 pageIndex() const { return m_pageUiIndex; }
	void setPageIndex(const qint32 v) { m_pageUiIndex = v; }

	QString pageName() const { return m_pageName; }
	void setPageName(const QString& v);
	QString pageDesignator() const { return m_pageDesignator; }
	void setPageDesignator(const QString& v) { m_pageDesignator = v; }

	bool pageActive() const { return m_pageActive; }

	virtual PageMode::Enum pageMode() const { return PageMode::INVALID; }
	virtual void setPageMode(PageMode::Enum v) {}

	static QSize PageSizeFromLauncherSize(quint32 width,quint32 height);

	virtual bool resize(quint32 w, quint32 h);

	virtual bool take(Thing * p_takerThing);
	virtual bool taking(Thing * p_victimThing, Thing * p_takerThing);
	virtual void taken(Thing * p_takenThing,Thing * p_takerThing);

	//	intended to be a general query (i.e. according to page type).
	// Don't modify it to be more specific; better just add other functions
	// for that
	virtual bool canAcceptIcons() const;
	virtual bool acceptIncomingIcon(IconBase * p_newIcon);
	virtual bool releaseTransferredIcon(IconBase * p_transferredIcon);

	virtual bool layoutFromItemList(const ThingList& items);
	virtual bool layoutFromItemList(const IconList& items);

	virtual bool setIconLayout(IconLayout * p_iconLayout);
	virtual IconLayout * currentIconLayout() const;

	//layoutCoord must be in layout ICS
	virtual IconBase * iconAtLayoutCoordinate(const QPointF& layoutCoord,QPointF * r_p_intraIconCoord);

	//this is a somewhat strange function, needed because of the way layouts work (they aren't actual graphics items)
	// if there is a scrollableobject, then the layout is its descendant w.r.t. graphicsview so the coordinates of the page
	// and the coords of the layout are modified by the scrollable.
	// if there is no scrollable, then (at least for now), there is no modification
	virtual QPointF	   	pageCoordinateFromLayoutCoordinate(const QPointF& layoutCoord);
	virtual QPointF		layoutCoordinateFromPageCoordinate(const QPointF& pageCoordinate);

	friend uint qHash(const QPointer<Page>& p);

	virtual QRectF areaScroller() const;
	virtual QRectF areaTopEdgeShadow() const;
	virtual QRectF areaBottomEdgeShadow() const;
	virtual QRectF areaLeftBorder() const;
	virtual QRectF areaRightBorder() const;
	virtual QRectF areaTopBorder() const;
	virtual QRectF areaBottomBorder() const;
	//areaCenter() is anything not a border , on the inner side of the borders
	virtual QRectF areaCenter() const;

	virtual PageAreas::Enum classifyPageLocation(const QPointF& pageCoordinate) const;

Q_SIGNALS:

	// params:
	//[0] old index
	//[1] new index
	void signalPageIndexChanged(qint32,qint32);
	void signalPageIndexChanged();

	// params:
	//[0] old name
	//[1] new name
	void signalPageNameChanged(const QString&, const QString&);
	void signalPageNameChanged();

	void signalPageDesignatorChanged();

	void signalPageModeChanged();
	void signalPageModeChanged(PageMode::Enum oldV,PageMode::Enum newV);

	void signalIconActivatedTap(IconBase* pIcon);

	void signalPageActive();
	void signalPageDeactivated();

	void signalRedirectFlick(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent);

	//LISTEN TO THIS SIGNAL TO TRIGGER SAVES; it's much better than listening to signalPageNameChanged or something else that signals a state change.
	void signalPageNeedsSave();

	// does what a "Done" button press on the tab bar would do
	void signalPageRequestExitReorder();

public Q_SLOTS:

	virtual void dbg_slotPrintLayout();

protected Q_SLOTS:

	virtual void slotAnimationEnsembleFinished();
	virtual void slotStopAnimationEnsemble(bool * r_result=0);
	virtual void slotStartAnimationEnsemble(bool canInterrupt=true);
	virtual void slotAddAnimationToEnsemble(QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType = DimensionsTypes::AnimationType::None);
	virtual void slotAddAnimationTo(QAnimationGroup * p_addToGroup,QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType = DimensionsTypes::AnimationType::None);

	virtual void slotScrollingPhysicsUpdate(qreal t,qreal d,qreal v,qreal a);
	virtual void slotPageOverscrolled();
	virtual void slotUndoOverscroll();

	virtual void slotBroadcastPageActivated(QUuid pageUid);
	virtual void slotBroadcastAllPagesDeactivated();

	virtual void slotKineticScrollerSpew(qreal oldScroll);

	//used by the scroll FSM. Only a slot to make it convenient for the FSM to call it
	virtual void slotOkToScroll();

	virtual void slotLauncherCmdEndReorderMode();

	virtual void slotLauncherBlockedInteractions();
	virtual void slotLauncherAllowedInteractions();
	virtual void slotClearTouchRegisters();

	//called as the launcher is about to be brought up and shown to the user, and as it is about to be brought down and hidden
	virtual void slotLauncherActivating();
	virtual void slotLauncherDeactivating();

protected:

	//this one used by PageRestore to bring back specific uids.
	// never, ever call directly with random or invalid uids - collisions will occur
	Q_INVOKABLE Page(const QUuid& specificUid,const QRectF& pageGeometry,LauncherObject * p_belongsTo);

	virtual void activatePage();
	virtual void deactivatePage();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);
	virtual void paintShadows(QPainter * painter);

	virtual bool sceneEvent(QEvent * event);
	virtual bool gestureEvent(QGestureEvent *gestureEvent) { return true; }
	virtual bool panGesture(QPanGesture *panEvent) { return true; }
	virtual bool swipeGesture(QSwipeGesture *swipeEvent,QGestureEvent * baseGestureEvent);
	virtual bool flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent);
	virtual bool pinchGesture(QPinchGesture *pinchEvent) { return true; }
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent) { return true; }
	virtual bool tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent) { return true; }
	virtual bool customGesture(QGesture *customGesture) { return true; }

	//DEFAULT POLICY: allow if no other gesture has claimed the touch point yet, else, reject.
	// When the return == true, then the register is updated with the gesture as the trigger for that id
	virtual bool okToUseFlick(int id);
	virtual bool okToUseTap(int id);
	virtual bool okToUseTapAndHold(int id);

	//These are to map the points from Page CS to ScrollingSurface CS
	// if there is no ScrollingSurface assigned, then the point passed in is the point returned (and the result is 'true')
	//ASSUMPTION: scroll surface is always completely contained inside the page (i.e. page geom totally encompasses scroll surface geom)
	virtual QPointF translatePagePointToScrollSurfacePoint(const QPointF& point,bool clipToGeom=false);
	virtual bool testAndTranslatePagePointToScrollSurfacePoint(const QPointF& point,QPointF& r_translatedPoint);

	virtual bool isAnimationEnsembleRunning() const;
	virtual bool stopAnimationEnsemble();
	virtual void animationFinishedProcessGroup(QAnimationGroup * pAnim);
	virtual void animationFinishedProcessAnim(QAbstractAnimation * pAnim);

	virtual bool addIcon(IconBase * p_icon) { return false; }
	virtual bool addIconNoAnimations(IconBase * p_icon) { return false; }
	virtual bool removeIcon(const QUuid& iconUid) { return false; }
	virtual bool removeIconNoAnimations(const QUuid& iconUid) { return false; }

	virtual IconBase * removeIconFromPageAddWaitlist(const QUuid& iconUid);
	virtual bool	isIconInPageAddWaitlist(const QUuid& iconUid);
	virtual void	addIconToPageAddWaitlist(IconBase * p_icon);
	//returns the number of items NOT handled (still waitlisted) ...so 0 would mean total success
	virtual qint32 	addWaitlistHandler();

	virtual void 	removeIconFromPageRemoveWaitlist(const QUuid& iconUid);
	virtual bool	isIconInPageRemoveWaitlist(const QUuid& iconUid);
	virtual void	addIconToPageRemoveWaitlist(IconBase * p_icon);
	virtual void	addIconToPageRemoveWaitlist(const QUuid& iconUid);
	//returns the number of items NOT handled (still waitlisted) ...so 0 would mean total success
	virtual qint32 	removeWaitlistHandler();

protected:

	QString m_pageName;

	qint32 	m_pageUiIndex;		//for quick indexing in the ui (owner)
	QString m_pageDesignator;
	bool 	m_pageActive;
	QPointer<LauncherObject> m_qp_currentUIOwner;
	QPointer<PixmapObject> m_qp_backgroundPmo;
	QPixmap m_topBorderShadowPixmap;
	QRectF m_topBorderShadowArea;

	QPixmap m_bottomBorderShadowPixmap;
	QRectF m_bottomBorderShadowArea;

	QBrush m_topBorderShadowBrush;
	QBrush m_bottomBorderShadowBrush;

	KineticScroller m_failmaticScroller;

	ScrollableObject * m_p_scroll;
	QPointer<QAnimationGroup> m_qp_ensembleAnimation;

	//TODO: MULTI-TOUCH:  only 1 touch id can control scroll, realistically. But what happens during M.T.
	//						on seemingly conflicting scroll touches?
	PageScrollDirectionLock::Enum m_scrollDirectionLock;
	int m_scrollLockTouchId;		//only valid when m_scrollDirectionLock != None

	QPointer<IconLayout> m_qp_iconLayout;
	QPoint m_iconLayoutOffsetCoord;

	// When an app icon needs to be added or removed (the most likely case of this is from the app installer), but this page is performing
	//	a reorder, then these "waitlists" are used to hold the icon/icon ref. When it is safe to do so, the waitlistHandler()s are called which will take care
	// of all of these waiting icons.
	// currently impl. as a map and set by icon uid, so that, in case instruction flow CAN manage to remove the icon from the page/system before the
	// waitlistHandler runs, it will be easy to find the icon / ref

	QMap<QUuid,QPointer<IconBase> > m_pendingExternalIconAddWaitlist;
	QSet<QUuid> m_pendingExternalIconRemoveWaitlist;

/////////////////////////// Touch and Touch FSM related /////////////////////////////////////////////

protected:

	QStateMachine * m_p_touchFSM;
	QState * m_p_fsmStateNoTouch;
	QState * m_p_fsmStateTouchMotionTracking;
	QState * m_p_fsmStateTouchStationaryTracking;

	static const char * TouchFSMPropertyName_isTracking;
	static const char * TouchFSMProperyName_trackedId;

	QMap<int,TouchRegister> m_touchRegisters;

	virtual bool anyTouchTracking(int * r_p_mainTouchId=0);

	virtual bool touchPointTriggerType(int id,TouchTriggerType::Enum& r_type);
	virtual bool testAndSetTriggerOnRegister(int id,TouchTriggerType::Enum conditionV,TouchTriggerType::Enum setV);
	//same as above, except will true condition also includes if the register is missing/invalid (it will create it)
	virtual bool testAndSetTriggerOnRegisterCreate(int id,TouchTriggerType::Enum conditionV,TouchTriggerType::Enum setV);

	virtual RedirectingType::Enum isRedirecting(int id,Thing ** r_pp_redirectTargetThing,RedirectContext ** r_pp_redirContext);
	virtual bool isDeferCancelled(int id,bool doCancel=false);
	virtual bool redirectTo(int id,Thing * p_redirectTargetThing,RedirectContext * p_redirContext = 0);

	//TODO: making public as a temp workaround to let proxy redirection changes work.  Redo this the OOP-correct way
public:
	virtual bool changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext = 0);
	virtual bool cancelRedirection(int id,bool deferred=false);
protected:
	virtual void setupTouchFSM();
	virtual void startTouchFSM();
	virtual void stopTouchFSM();

	virtual void clearAllTouchRegisters();

	//these are the interface to QT...see sceneEvent()
	virtual bool touchStartEvent(QTouchEvent *event);
	virtual bool touchUpdateEvent(QTouchEvent *event);
	virtual bool touchEndEvent(QTouchEvent *event);

	virtual void handleTrackedTouchPointRelease(QTouchEvent::TouchPoint* touchPoint);

	//passing in id, but really for now only 1 id can be tracked
	virtual void touchTrackedPointStarted(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);

	virtual void autoScrollDown();
	virtual void autoScrollUp();

protected Q_SLOTS:
	void dbg_slotStatePropertiesAssigned();

Q_SIGNALS:

	//Triggers: what causes FSM state changes
	void signalTouchFSMTouchBegin_Trigger(int touchId);
	void signalTouchFSMTouchPausedMotion_Trigger();
	void signalTouchFSMTouchResumedMotion_Trigger();
	void signalTouchFSMTouchEnd_Trigger();

	//Actions: what the FSM outputs to signal state changes to this object
	void signalTouchFSMBeginTrack_Action();			//on a new touch [kicks off the FSM]
	void signalTouchFSMTouchLost_Action();			// when a new touch appears when a touch is already tracking;
											// this is used when I only support single touch (like in this class)
	void signalTouchFSMStationary_Action();			// when the motion stops for a period of time (TODO: implement)
	void signalTouchFSMMoving_Action();				// when the touch point is moving again (only signalled after a stop, not after Begin)
	void signalTouchEndTrack_Action();				// when the touch is released

////////////////////////// end Touch FSM //////////////////////////////////////////////////

///////////////////////// Scroll Delay FSM ////////////////////////////////////////////////

Q_SIGNALS:

	void signalScrollDelayFSM_UserScrolled();

protected:
	virtual void setupScrollDelayFSM();
	virtual void startScrollDelayFSM();
	virtual void stopScrollDelayFSM();

	//this one is an easy interface from subclasses to signal the start of scroll
	virtual void scrollActionTrigger();

	//TODO: wonky naming...especially confusing w.r.t other okTo___ functions and slotOkToScroll()
	virtual bool okToPerformAutoScroll() const;
protected:

	QStateMachine * m_p_scrollDelayFSM;
	QState * m_p_fsmStateScrollOk;
	QState * m_p_fsmStateScrollDelay;
	QTimer m_scrollFSMTimer;

	static const char * ScrollDelayFSMPropertyName_isScrollOk;

	bool	m_interactionsBlocked;

///////////////////////// end Scroll Delay FSM

/////////////////////// --- DEBUG --- /////////////////////////////////////////////////////

Q_SIGNALS:

	void dbg_signalTriggerCamera(ThingPaintable * pTp);
};

uint qHash(const QPointer<Page>& p);

#endif /* PAGE_H_ */
