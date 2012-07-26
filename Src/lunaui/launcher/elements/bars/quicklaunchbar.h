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




#ifndef QUICKLAUNCHBAR_H_
#define QUICKLAUNCHBAR_H_

#include "thingpaintable.h"
#include "pixmapobject.h"
#include "pixmap9tileobject.h"
#include "dimensionstypes.h"
#include "appmonitor.h"

#include <QUuid>
#include <QPointer>
#include <QPointF>
#include <QSize>
#include <QList>
#include <QPair>
#include <QParallelAnimationGroup>
#include <QTimer>

class QGraphicsSceneMouseEvent;
class QGesture;
class QGestureEvent;
class QTapAndHoldGesture;
class QTapGesture;
class FlickGesture;

class Quicklauncher;
class IconBase;
class PixButton2State;
class QStateMachine;
class QState;
class QTouchEvent;


namespace DimensionsSystemInterface
{
class ExternalApp;
class WebOSApp;
class PageSaver;
class PageRestore;
}

class QuickLaunchBar : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:

	QuickLaunchBar(const QRectF& geom,Quicklauncher * p_quicklauncherWindow);
	virtual ~QuickLaunchBar();

	virtual bool canAcceptIcons();
	virtual bool offer(Thing * p_offer,Thing * p_offeringThing);
	virtual bool take(Thing * p_takerThing);
	virtual bool taking(Thing * p_victimThing, Thing * p_takerThing);
	virtual void taken(Thing * p_takenThing,Thing * p_takerThing);

	virtual bool acceptIncomingIcon(IconBase * p_newIcon);
	virtual bool releaseTransferredIcon(IconBase * p_transferredIcon);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);
	inline void paintBackground(QPainter * painter);

	virtual bool fullInit(quint32 screenWidth,quint32 screenHeight);

	//NOTE: pass in the ACTUAL size that the QL should be resized to, not the screen size or other "base" size
	//	that a QL size would be derived from
	virtual bool resize(const QSize& s);

	static QSize QuickLaunchSizeFromScreenSize(int screenWidth,int screenHeight);

	virtual QList<QPointer<IconBase> > iconsInFlowOrder() const;		//left to right , in order of appearance

	//will only work if there are no items currently
	//TODO: remove this limitation
	virtual bool restoreFromSave();

	void cancelLaunchFeedback();

public Q_SLOTS:

	// **These mirror the versions with the same names in LauncherObject

	//prior to the appmonitor deleting all the app structures
	void slotAppPreRemove(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID) {}
	//post appmonitor deleting all the app structures
	void slotAppPostRemove(const QUuid& removedAppUid,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID) {}
	void slotAppAuxiliaryIconRemove(const QUuid& appUid,
		const QString& launchpointId,
		DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID) {}

Q_SIGNALS:

	void signalIconAdded(QUuid iconUid);
	void signalIconRemoved(QUuid iconUid);

	void signalQuickLaunchBarTriggerActivatedTap();
	void signalQuickLaunchBarTriggerActivatedTapAndHold();

	void signalFlickAction(QPointF normVectorDirection = QPointF());

	void signalToggleLauncher();

protected:

	virtual bool sceneEvent(QEvent * event);
	virtual bool gestureEvent(QGestureEvent *gestureEvent) { return true; }
	virtual bool flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent);
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent);
	virtual bool tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent);

	void setAppLaunchFeedback(IconBase* pIcon);

	//rearrange all the current items based on the current sizing
	//PREREQUISITE: m_itemAreaXRange and m_itemsY are set correctly
	virtual void rearrangeIcons(bool animate = false);

	//index >= items.size will append safely
	virtual qint32 getCurrentIconListIndex(IconBase * p_icon);
	virtual bool addIcon(quint32 index,IconBase * p_icon, bool animate = false);
	virtual bool moveIcon(quint32 oldIndex, quint32 newIndex,IconBase * p_icon, bool animate = false);
	virtual void removeIcon(IconBase * p_icon, bool animate = false);
	virtual bool detectAndHandleSpecialMoveAreas(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);

	virtual void deleteIcon(IconBase * p_icon, bool animate); // removes icon from the list and also deletes the icon object

	virtual IconBase * getIconPtr(const QUuid& iconUid,bool paramIsMasterUid=true);

	virtual void iconActivatedTap(QUuid iconUid);

protected Q_SLOTS:

	virtual void slotIconDeleted(QObject * p);
	void slotCancelLaunchFeedback();

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
	virtual bool redirectTo(int id,Thing * p_redirectTargetThing,RedirectContext * p_redirContext = 0);
	virtual bool changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext = 0);

	virtual IconBase * iconAtCoordinate(const QPointF& coord);

	static void resetEventCounters();

	virtual void setupTouchFSM();
	virtual void startTouchFSM();
	virtual void stopTouchFSM();

	//these are the interface to QT...see sceneEvent()
	virtual bool touchStartEvent(QTouchEvent *event);
	virtual bool touchUpdateEvent(QTouchEvent *event);
	virtual bool touchEndEvent(QTouchEvent *event);

	//passing in id, but really for now only 1 id can be tracked
	virtual void touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);

	virtual void redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);
	virtual void redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);

	virtual bool cancelRedirection(int id);

	virtual bool handleLauncherPageSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);

	//DEFAULT POLICY: allow if no other gesture has claimed the touch point yet, else, reject.
	// When the return == true, then the register is updated with the gesture as the trigger for that id
	virtual bool okToUseFlick(int id);
	virtual bool okToUseTap(int id);
	virtual bool okToUseTapAndHold(int id);

	virtual qint32 iconSlotForInsertingAtXCoord(qint32 x);

Q_SIGNALS:

//Triggers: what causes FSM state changes
	void signalTouchFSMTouchBegin_Trigger(QVariant touchId);
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

	void signalIconActivatedTap(IconBase* pIcon);         // user has tapped on a valid Icon


////////////////////////// end Touch FSM //////////////////////////////////////////////////

protected:

	static qint32           sEventCounter0;
	bool					m_fullInitPerformed;
	QPointer<Quicklauncher> m_qp_ownerWindow;
	QPointer<PixmapObject> m_qp_backgroundTranslucent;
	QPointer<PixmapObject> m_qp_backgroundSolid;

	QPointer<PixmapObject> m_qp_currentBg;	//alias to one of the two; solid or translucent

	QPointer<PixButton2State> m_qp_launcherAccessButton;

	//items are constrained to within this x range in ICS
	QPair<qint32,qint32>	m_itemAreaXrange;
	//...and this is the centerline
	qint32					m_itemsY;

	QList<qint32>			m_layoutAnchorsXcoords;
	QList<QPointer<IconBase> > m_iconItems;				//must be IN ORDER, as arranged on the QL. ==> when a reorder on the QL happens, this list needs to be shuffled to match
	typedef QList<QPointer<IconBase> >::iterator IconListIter;

	QPointer<IconBase> m_qp_iconInMotion;
	qint32             m_iconInMotionCurrentIndex;

	QPointer<QParallelAnimationGroup> m_qp_reorderAnimationGroup;

	IconBase* m_iconShowingFeedback;
	QTimer    m_feedbackTimer;
};

#endif /* QUICKLAUNCHBAR_H_ */
