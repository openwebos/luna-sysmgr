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




#ifndef ALPHABETPAGE_H_
#define ALPHABETPAGE_H_

#include "page.h"
#include <QPointer>
#include "thing.h"
#include "icon.h"

class AlphabetIconLayout;
class PixmapObject;
class AlphabetPage : public Page
{
	Q_OBJECT

public:
	friend class AlphabetIconLayout;

	Q_INVOKABLE AlphabetPage(const QRectF& pageGeometry,LauncherObject * p_belongsTo);
	virtual ~AlphabetPage();

	//returns true if at least 1 item was laid out, false for a total fail
	virtual bool layoutFromItemList(const ThingList& items);
	virtual bool layoutFromItemList(const IconList& items);

	virtual bool canAcceptIcons() const;
	virtual PageMode::Enum pageMode() const;
	virtual void setPageMode(PageMode::Enum v);

	virtual bool releaseTransferredIcon(IconBase * p_transferredIcon);

	virtual bool resize(quint32 w, quint32 h);

	virtual bool addIcon(IconBase * p_icon);
	virtual bool addIconNoAnimations(IconBase * p_icon);
	virtual bool removeIcon(const QUuid& iconUid);
	virtual bool removeIconNoAnimations(const QUuid& iconUid);


public Q_SLOTS:

	virtual void dbg_slotPrintLayout();

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

	virtual bool tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent);
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent);

	//passing in id, but really for now only 1 id can be tracked
	virtual void touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void iconRelease();

	/// ARE YOU LOOKING FOR PAINT() ??? See scrollinglayoutrenderer.h/cpp
	virtual void paintOffscreen(QPainter *painter);

	//TODO: OOP-S: all the detect and handle special area fns should be moved to the Page class; overrides can happen here as needed
	virtual bool detectAndHandleSpecialMoveAreas(int id,const QPointF& pageCoordinate);
	virtual bool handleTopBorderSpecialMoveArea(int id,const QPointF& pageCoordinate);
	virtual bool handleBottomBorderSpecialMoveArea(int id,const QPointF& pageCoordinate);
	virtual bool handleLeftBorderSpecialMoveArea(int id,const QPointF& pageCoordinate);
	virtual bool handleRightBorderSpecialMoveArea(int id,const QPointF& pageCoordinate);

	virtual bool detectAndHandleSpecialReleaseAreas(int id,const QPointF& pageCoordinate);
	virtual bool handleQuickLaunchSpecialReleaseArea(int id,const QPointF& pageCoordinate);

	virtual void activatePage();
	virtual void deactivatePage();

	virtual void switchToNormalMode();
	virtual void switchToReorderMode();

protected:

	PageMode::Enum		m_pageMode;
	///////////// --- icon motion tracking ----------- //////////////
	//TODO: implement a better system; this is just done ad-hoc for scaffolding to get page reorder going
	// in particular, I'd like multi-touch support.
	QPair<QUuid,QUuid>	m_trackingIconUids;
	QPointer<IconBase> m_qp_cached_trackingIcon;		//value2 of the uid pair above is this icon; it's stored here so it doesn't have to be
														// looked up from the heap each time

};

#endif /* ALPHABETPAGE_H_ */
