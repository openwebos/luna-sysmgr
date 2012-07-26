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




#ifndef REORDERABLEPAGE_H_
#define REORDERABLEPAGE_H_

#include "page.h"
#include <QPointer>
#include "thing.h"
#include "icon.h"

class ReorderableIconLayout;
class PixmapObject;
class TextBox;
class PictureBox;

class ReorderablePage : public Page
{
	Q_OBJECT

public:

	static const char * IsReorderingLayoutBoolPropertyName;

	friend class ReorderableIconLayout;
	friend class LauncherObject;

	Q_INVOKABLE ReorderablePage(const QRectF& pageGeometry,LauncherObject * p_belongsTo);
	void commonCtor();

	virtual ~ReorderablePage();

	//returns true if at least 1 item was laid out, false for a total fail
	virtual bool layoutFromItemList(const ThingList& items);
	virtual bool layoutFromItemList(const IconList& items);

	virtual bool canAcceptIcons() const;

	virtual bool offer(Thing * p_offer,Thing * p_offeringThing);
	virtual bool take(Thing * p_takerThing);
	virtual bool taking(Thing * p_victimThing, Thing * p_takerThing);
	virtual void taken(Thing * p_takenThing,Thing * p_takerThing);
	virtual void externalIconMoveTerminated();

	virtual PageMode::Enum pageMode() const;
	virtual void setPageMode(PageMode::Enum v);

	virtual bool resize(quint32 w, quint32 h);

	//specific to this page...
	//TODO: possibly a common base class under Page that encompasses Pages that have layouts with "grids"
	//		or at least icon coordinates that can be expressed as <int,int> (i.e. a QPoint)
	virtual IconBase * iconAtLayoutCoordinate(const QPointF& layoutCoord,QPoint& r_gridCoordinate,QPointF * r_p_intraIconCoord = 0);

	virtual qint32 rowAtPageCoordinate(const QPointF& pageCoord);

	virtual void redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);
	virtual void redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);

	virtual bool acceptIncomingIcon(IconBase * p_newIcon);
	virtual bool releaseTransferredIcon(IconBase * p_transferredIcon);

	//for property IsReorderingLayoutBoolPropertyName opaqueness
	virtual bool isLayoutReorderingCurrently() const;

protected Q_SLOTS:

//	the reorder layout uses this to tell me that the tracking has been canceled/lost
//	DON'T USE TO DO ANYTHING "HEAVY"...it's just meant for dealing with the Page's version of tracking
//	the icon(s) being moved by the user's touch point inputs
	virtual void slotTrackedIconCancelTrack(const QUuid& uid);

	virtual void slotReorderInLayoutStarted();
	virtual void slotReorderInLayoutEnded();

	virtual void slotLauncherCmdStartReorderMode();
	virtual void slotLauncherCmdEndReorderMode();

protected:

	Q_INVOKABLE ReorderablePage(const QUuid& specificUid,const QRectF& pageGeometry,LauncherObject * p_belongsTo);

	virtual bool tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent);
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent);
	virtual bool flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent);

	//passing in id, but really for now only 1 id can be tracked
	virtual void touchTrackedPointStarted(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void iconRelease();

	virtual void paintOffscreen(QPainter *painter);

	virtual bool detectAndHandleSpecialMoveAreas(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);
	virtual bool handleTopBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);
	virtual bool handleBottomBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);
	virtual bool handleLeftBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);
	virtual bool handleRightBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);
	virtual bool handleQuickLaunchSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);
	virtual bool handlePageTabBarSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext);

	virtual void switchToNormalMode();
	virtual void switchToReorderMode();

	//addIcon goes right through to addIconNoAnimation... these are used to add to this page from the launcher/appmonitor for appinstalls,etc.
	// otherwise, for reorders, acceptIncomingIcon does the adding.
	virtual bool addIcon(IconBase * p_icon);
	virtual bool addIconNoAnimations(IconBase * p_icon);
	virtual bool removeIcon(const QUuid& iconUid);
	virtual bool removeIconNoAnimations(const QUuid& iconUid);

protected:

	QPointer<TextBox> m_qp_emptyPageText;
	QPointer<PictureBox> m_qp_emptyPageIcon;

	PageMode::Enum		m_pageMode;

	///////////// --- icon motion tracking ----------- //////////////
	//TODO: implement a better system; this is just done ad-hoc for scaffolding to get page reorder going
	// in particular, I'd like multi-touch support.
	QPointer<IconBase> m_qp_iconInMotion;
	QPoint m_iconInMotionCurrentGridPosition;

};

#endif /* REORDERABLEPAGE_H_ */
