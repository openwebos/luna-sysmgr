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




#include "dimensionsglobal.h"
#include "alphabetpage.h"
#include "alphabeticonlayout.h"
#include "icon.h"
#include "iconheap.h"
#include "pixmapobject.h"
#include "pixmaphugeobject.h"
#include "scrollingsurface.h"
#include "scrollinglayoutrenderer.h"
#include "dotgrid.h"
#include "dimensionslauncher.h"

#include <QPainter>
#include <QEvent>
#include <QGesture>
#include <QGestureEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPair>
#include <QPropertyAnimation>

AlphabetPage::AlphabetPage(const QRectF& pageGeometry,LauncherObject * p_belongsTo)
: Page(pageGeometry,p_belongsTo)			//make it explicit in case ctors change
, m_pageMode(PageMode::Normal)
, m_qp_cached_trackingIcon(0)
{
}

//virtual
AlphabetPage::~AlphabetPage()
{
}

bool AlphabetPage::layoutFromItemList(const ThingList& items)
{
	//since this type of layout deals only with ICONS, go through the list
	// and eliminate non-icon Things
	IconList iconItems;
	for (ThingListConstIter it = items.constBegin();
			it != items.constEnd(); ++it)
	{
		IconBase * pIcon = qobject_cast<IconBase*>(*it);
		if (!pIcon)
		{
			continue;	//not an icon
		}
		iconItems << pIcon;
	}
	return layoutFromItemList(iconItems);
}

bool AlphabetPage::layoutFromItemList(const IconList& items)
{
	//if there are no icon items, then exit
	if (items.empty())
	{
		return true;		//considered a success.
	}

	//if there is a current layout, then get rid of it
	delete m_qp_iconLayout;
	AlphabetIconLayout * pAlphaLayout = new AlphabetIconLayout(this);

	QSize newLayoutSize = QSize(m_geom.size().toSize().width() - (m_geom.size().toSize().width() % 2),
				m_geom.size().toSize().height() - (m_geom.size().toSize().height() % 2));
//	pAlphaLayout->resizeWidth((quint32)(m_geom.width()-40.0));
	pAlphaLayout->resizeWidth((quint32)(newLayoutSize.width()));

	//now init the layout from the icon list
	AlphabetIconLayout::initLayoutFromSequentialIconListFullEnglishAlpha(*pAlphaLayout,items);
	m_qp_iconLayout = pAlphaLayout;

	pAlphaLayout->recomputePageLayoutTransforms();

	dbg_slotPrintLayout();
	//set the scrollable with the layout

	m_p_scroll = new ScrollingLayoutRenderer(DimensionsGlobal::realRectAroundRealPoint(newLayoutSize),*pAlphaLayout);
	m_p_scroll->setParentItem(this);
	m_p_scroll->enable();

	m_failmaticScroller.setMinValue((qreal)(m_p_scroll->topLimit()));
	m_failmaticScroller.setMaxValue((qreal)(m_p_scroll->bottomLimit()));
	m_failmaticScroller.setMaxOverscroll(10.0);

//	DotGrid * p_dbg_grid = new DotGrid(m_p_scroll->geometry(),QSize(20,20));
//	p_dbg_grid->setParentItem(this);
	return true;
}

//virtual
bool AlphabetPage::canAcceptIcons() const
{
	return false;
}

//virtual
PageMode::Enum AlphabetPage::pageMode() const
{
	return m_pageMode;
}

//virtual
void AlphabetPage::setPageMode(PageMode::Enum v)
{
	if (v == m_pageMode)
	{
		//do nothing
		return;
	}

	switch (v)
	{
	case PageMode::Normal:
		if (m_pageMode == PageMode::Reorder)
		{
			//switch back from Reorder to Normal...
			switchToNormalMode();
		}
		break;
	case PageMode::Reorder:
		if (m_pageMode == PageMode::Normal)
		{
			//switch from Normal to Reorder...
			switchToReorderMode();
		}
		break;
	default:
		break;
	}

	PageMode::Enum old = m_pageMode;
	m_pageMode = v;
	Q_EMIT signalPageModeChanged(old,v);
}

//virtual
bool AlphabetPage::releaseTransferredIcon(IconBase * p_transferredIcon)
{
	if (p_transferredIcon != m_qp_cached_trackingIcon)
	{
		return false;
	}

	//this can't do a regular iconRelease() because that function does cleanup that shouldn't
	// be done since the icon has actually just relocated.

	AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
	if (pLayout != NULL)
	{
		pLayout->trackedIconLeavingLayout(m_qp_cached_trackingIcon->uid());
	}
	m_qp_cached_trackingIcon = 0;
	//TODO: MULTI-TOUCH
	m_trackingIconUids = QPair<QUuid,QUuid>();
	return true;
}

//virtual
bool AlphabetPage::addIcon(IconBase * p_icon)
{
	//TODO: for now just a passthrough to addIconNoAnimations
	if (!p_icon)
	{
		return false;
	}
	qDebug() << __FUNCTION__ << ": trying to add icon " << p_icon->uid();
	return addIconNoAnimations(p_icon);
}

//virtual
bool AlphabetPage::addIconNoAnimations(IconBase * p_icon)
{
	if (!p_icon)
	{
		return false;
	}
	//if there is an icon tracking or a reorder going on, then reject
	//TODO: need a way to retry this afterward the reorder
	if (m_qp_cached_trackingIcon)
	{
		qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") error: currently tracking an icon";
		return false;
	}

	//there needs to be a layout
	AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") error: no layout";
		return false;
	}

	//TODO: SLOW: TEMP:
//	This can be done with an alphabetic insert into the existing rows, and then altering only the rows that need it -
//	this would include expanding a current alpha-category to n+1 rows (if there are currently n full rows in that category)
//	and needing to shift the existing icons appropriately to place the new icon into its correct alphabetic spot, since most
//	probably, the new icon would belong somewhere in the middle of the current icons for that alpha-category
//
//	For now though, I'll do something similarly simpler-but-slower as what removeIcon does: Insert the icon at the end of the layout,
//	then destroy all the existing rows and re-alphabetize the cells, relaying them out into new rows

	qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") trying to add icon to layout";
	pLayout->insertAlphabetically(p_icon);
	pLayout->relayoutExisting();
	//the size of the layout could have changed
	if (m_p_scroll)
	{
		//TODO: TEMP: the scroller should get a signal from the layout that the
		// geom has changed
		m_p_scroll->slotSourceContentSizeChanged(pLayout->geometry().size());
	}
	update();
	qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") done...exiting";
	return true;
}

//virtual
bool AlphabetPage::removeIcon(const QUuid& iconUid)
{
	//TODO: IMPLEMENT
	// for now it'll just be a passthrough to removeIconNoAnimations()

	qDebug() << __FUNCTION__ << ": trying to remove icon " << iconUid;
	return removeIconNoAnimations(iconUid);
}

//virtual
bool AlphabetPage::removeIconNoAnimations(const QUuid& iconUid)
{
	//if there is an icon tracking or a reorder going on, then reject
	//TODO: need a way to retry this afterward the reorder
	if (m_qp_cached_trackingIcon)
	{
		return false;
	}

	//there needs to be a layout
	AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		return false;
	}

	//find the icon in the layout
	QPoint iconCoord;
	if (!pLayout->findIconByUid(iconUid,iconCoord))
	{
		//icon not found in this layout/page
		return false;
	}

	if (pLayout->removeIconCell(iconCoord))
	{
		//relayout the icons
		pLayout->relayoutExisting();
		//the size of the layout could have changed
		if (m_p_scroll)
		{
			//TODO: TEMP: the scroller should get a signal from the layout that the
			// geom has changed
			m_p_scroll->slotSourceContentSizeChanged(pLayout->geometry().size());
		}
		return true;
	}
	return false;
}

//virtual
bool AlphabetPage::resize(quint32 w, quint32 h)
{
	//TODO: return codes...
	(void)Page::resize(w,h);

	QSize newLayoutSize = QSize(m_geom.size().toSize().width() - (m_geom.size().toSize().width() % 2),
			m_geom.size().toSize().height() - (m_geom.size().toSize().height() % 2));

	//TODO: OOP fix this...these should be done from subclass' resize()
	if (m_qp_iconLayout)
	{
		m_qp_iconLayout->resizeWidth((quint32)(newLayoutSize.width()));
		m_qp_iconLayout->recomputePageLayoutTransforms();
	}

	if (m_p_scroll)
	{
		//TODO: TEMP: the scroller should get a signal from the layout that the
		// geom has changed
		m_p_scroll->slotSourceContentSizeChanged(m_qp_iconLayout->geometry().size());
		m_p_scroll->resize((quint32)newLayoutSize.width(),(quint32)newLayoutSize.height());
		m_failmaticScroller.setMinValue((qreal)(m_p_scroll->topLimit()));
		m_failmaticScroller.setMaxValue((qreal)(m_p_scroll->bottomLimit()));
		m_failmaticScroller.setMaxOverscroll(10.0);
		m_failmaticScroller.setScrollOffset(0.0);
	}
	return true;
}

//protected:

//virtual
void AlphabetPage::paintOffscreen(QPainter *painter)
{
	//TODO: IMPLEMENT
}

///////////////////////////////    UI - GESTURES AND EVENTS ///////////////////////////

//virtual
bool AlphabetPage::tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent)
{
	//TODO: This is going to get SUPER MESSY when multi-touch comes in. Essentially, I need a set of (touch)points in the gesture so I can
	// associate what touch point(s) is/are going with what gesture, since multiple of both may be in-flight (in progress) concurrently.
	//QTapGesture and QGestureEvent don't do this...this info is "lost" in the Gesture Recognizers.
	//TODO: TEMP:	assume that the touch point being tracked currently is the one that is associated with this gesture

	qDebug() << __PRETTY_FUNCTION__ << ": Tap Gesture";

	//if nothing is tracking, reject
	int mainTouchId;
	if (!anyTouchTracking(&mainTouchId))
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Rejected; touch FSM reports no touchId is currently tracked";
		return true;
	}
	//TODO: MULTI-TOUCH: here is where I can get into deep trouble; I've just assumed that the main touch point is part of this gesture. It need not be
	if (!okToUseTap(mainTouchId))
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Rejected; a previous gesture has already claimed touch id " << mainTouchId;
		return true;
	}

	//figure out which (if any) icon is being acted on

	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapEvent->hotSpot()));
	QPointF intraIconPositionOfTapICS;
	IconBase * pIcon = iconAtLayoutCoordinate(positionOfTapICS,&intraIconPositionOfTapICS);
	if (pIcon == NULL)
	{
		//no icon there
		return true;
	}

	//check to see if the icon should handle the event internally
	IconInternalHitAreas::Enum hitArea;
	if (pIcon->tapIntoIcon(intraIconPositionOfTapICS,hitArea))
	{
//		qDebug() << "Hoorah!";
		return true;
	}
	else
	{
		qDebug() << "Icon didn't want to deal with a intra-icon coord claiming to be " << intraIconPositionOfTapICS;
	}
	Q_EMIT signalIconActivatedTap(pIcon);
	return true;
}

//#include <QDrag>
//#include <QMimeData>

//virtual
bool AlphabetPage::tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent)
{
//	qDebug() << __PRETTY_FUNCTION__ << ": Tap-N-Hold Gesture";

	//if nothing is tracking, reject
	int mainTouchId;
	if (!anyTouchTracking(&mainTouchId))
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Rejected; touch FSM reports no touchId is currently tracked";
		return true;
	}
	//TODO: MULTI-TOUCH: here is where I can get into deep trouble; I've just assumed that the main touch point is part of this gesture. It need not be
	if (!okToUseTapAndHold(mainTouchId))
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Rejected; a previous gesture has already claimed touch id " << mainTouchId;
		return true;
	}

	AlphabetIconLayout * pAlphaLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
	if (!pAlphaLayout)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Rejected; There doesn't seem to be a layout on this page (or it isn't an Alpha layout)";
		return true;
	}

	//if I'm not in reorder mode, then flip to it now; this is just going to change all the graphics in the icons
	// and enable them to sense taps on special, decorator areas
	setPageMode(PageMode::Reorder);	//note calling this when already in Reorder mode does nothing, so it's ok to just call it and avoid the "if"

	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapHoldEvent->hotSpot()));
//	qDebug() << "tap-n-hold position from the event: " << positionOfTapICS;

	//TODO: TEMP: the tap coordinate is in PAGE ICS...the layout ICS is a little bit different. It needs to be mapped for total correctness
	//		for now, this will work
	QPointF positionOfTapLayoutCS = layoutCoordinateFromPageCoordinate(positionOfTapICS);

	//first, it there is an icon already in motion, silently consume event (yum!)
	if (m_qp_cached_trackingIcon)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Rejected; Already tracking an icon";
		return true;
	}

	//tell the layout to start tracking an icon at the position given

	if (!pAlphaLayout->startTrackingIcon(positionOfTapLayoutCS,m_trackingIconUids))
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Rejected; No icon found at tap-n-hold coordinate " << positionOfTapLayoutCS;
		return true;
	}

	//grab the icon ptr from the icon heap
	m_qp_cached_trackingIcon = IconHeap::iconHeap()->getIcon(m_trackingIconUids.second,FindIconHint::Copied);
	if (!m_qp_cached_trackingIcon)
	{
		//TODO: TEMP: DEBUG: this doesn't have to end fatally - but until I'm ready for release of the code, i'd like to catch heap-fail problems explicitly
		//DIE!!!
		qCritical() << __PRETTY_FUNCTION__ << ": icon heap doesn't have icon uid " << m_trackingIconUids.second;		/// <<<--- This should abort the whole process
		return true;
	}

//	QDrag *drag = new QDrag(baseGestureEvent->widget());
//	QMimeData *mimeData = new QMimeData;
//
//	mimeData->setText("something");
//	drag->setMimeData(mimeData);
////	drag->setPixmap(0);
//
//	qDebug() << "exec!";
//	Qt::DropAction dropAction = drag->exec();
//	qDebug() << "post exec";

	m_qp_cached_trackingIcon->slotEnableIconAutoRepaint();
	m_qp_cached_trackingIcon->take(this);	///theoretically this could be rejected! //TODO: handling
	m_qp_cached_trackingIcon->setVisible(true);
	m_qp_cached_trackingIcon->setPos(positionOfTapICS);
	m_qp_cached_trackingIcon->update();
	return true;

}

//TODO: TEMP: clean this up and move to the right place to make OOP gods happy
static qint32 gEventCounter0 = 0;

static void resetEventCounters()
{
	gEventCounter0 = 0;
}

//virtual
void AlphabetPage::touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{
	//if an icon is NOT being moved, then just punt to the Page:: handler
	if (!m_qp_cached_trackingIcon)
	{
		return Page::touchTrackedPointMoved(id,scenePosition,lastScenePosition,initialPosition);
	}

	QPointF pageCoordinate = mapFromScene(scenePosition);
	m_qp_cached_trackingIcon->setPos(pageCoordinate);

	AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
	if (pLayout == NULL)
	{
		//no layout object???
		return Page::touchTrackedPointMoved(id,scenePosition,lastScenePosition,initialPosition);
	}

	if ((quint32)(++gEventCounter0) >= pLayout->m_reorderEventSampleRate)
	{
		//time to sample to see if the icon has been dragged to anywhere of importance
		if (detectAndHandleSpecialMoveAreas(id,pageCoordinate))
		{
			//there was a special move generated...do not reset the event counter, just return;
			return;
		}
		//reset the event counter used
		gEventCounter0 = 0;
	}

}

//virtual
bool AlphabetPage::detectAndHandleSpecialMoveAreas(int id,const QPointF& pageCoordinate)
{

	//TODO: IMPLEMENT (unfinished)

	PageAreas::Enum pageAreaOfIcon = classifyPageLocation(pageCoordinate);
	bool queryLauncher = false;
	switch (pageAreaOfIcon)
	{
	case PageAreas::LeftBorder:
//		qDebug() << __FUNCTION__ << ": Left Border";
		return handleLeftBorderSpecialMoveArea(id,pageCoordinate);
	case PageAreas::RightBorder:
//		qDebug() << __FUNCTION__ << ": Right Border";
		return handleRightBorderSpecialMoveArea(id,pageCoordinate);
	case PageAreas::TopBorder:
//		qDebug() << __FUNCTION__ << ": Top Border";
		return handleTopBorderSpecialMoveArea(id,pageCoordinate);
	case PageAreas::BottomBorder:
//		qDebug() << __FUNCTION__ << ": Bottom Border";
		return handleBottomBorderSpecialMoveArea(id,pageCoordinate);
	case PageAreas::Content:
//		qDebug() << __FUNCTION__ << ": Warm, chewy center!";
		return false;
	default:
		queryLauncher = true;
//		qDebug() << __FUNCTION__ << ": Nuthin'!";
		break;
	}
	if ((queryLauncher) && (m_qp_currentUIOwner))
	{
		LauncherAreas::Enum launcherArea = m_qp_currentUIOwner->classifyPageLocation(mapToItem(m_qp_currentUIOwner,pageCoordinate));
		switch (launcherArea)
		{
		case LauncherAreas::PageTabBar:
//			qDebug() << __FUNCTION__ << ": Page Tab Bar";
			break;
		case LauncherAreas::QuickLaunchBar:
//			qDebug() << __FUNCTION__ << ": Quick Launch Bar";
			break;
		case LauncherAreas::PageInner:
//			qDebug() << __FUNCTION__ << ": Launcher claims it's in the Page";
			break;
		default:
//			qDebug() << __FUNCTION__ << ": Launcher Nuthin'!";
			break;
		}
		return false;
	}

	return true;
}

//virtual
bool AlphabetPage::handleTopBorderSpecialMoveArea(int id,const QPointF& pageCoordinate)
{
	return true;
}

//virtual
bool AlphabetPage::handleBottomBorderSpecialMoveArea(int id,const QPointF& pageCoordinate)
{
	//This is a move to the quick launch
	if (m_qp_cached_trackingIcon)
	{
		//redirect the movements to the launcher object, so that it can deal with transferring this icon to the Quick launch
		redirectTo(id,m_qp_currentUIOwner,new LauncherPageIconTransferRedirectContext(this,m_qp_cached_trackingIcon,LauncherPageIconTransferRedirectContext::QuickLaunch));
	}
	return true;
}

//virtual
bool AlphabetPage::handleLeftBorderSpecialMoveArea(int id,const QPointF& pageCoordinate)
{
	//TODO: this is hardcoded right now since there is only 1 Alphabet page and it's always on the left most position
	//		so this is a no-op
	return true;
}

//virtual
bool AlphabetPage::handleRightBorderSpecialMoveArea(int id,const QPointF& pageCoordinate)
{
	//TODO:  this is hardcoded right now since there is only 1 Alphabet page and it's always on the left most position
	//	and there is always 1 ReorderablePage to the right of it

	//also, this only works if there is an icon that's being tracked
	if (m_qp_cached_trackingIcon)
	{
		//redirect the movements to the launcher object, so that it can deal with panning right.
		redirectTo(id,m_qp_currentUIOwner,new LauncherPageIconTransferRedirectContext(this,m_qp_cached_trackingIcon,LauncherPageIconTransferRedirectContext::Right));
	}
	return true;
}

//virtual
void AlphabetPage::activatePage()
{
	Page::activatePage();
}

//virtual
void AlphabetPage::deactivatePage()
{
	Page::deactivatePage();
}

//virtual
void AlphabetPage::switchToNormalMode()
{
	//pretty much a passthrough to the layout, which will in turn switch all the icons inside it to normal mode graphics
	AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		//uhhhh..
		return;
	}
	pLayout->switchIconsToNormalGraphics();
}

//virtual
void AlphabetPage::switchToReorderMode()
{
	//pretty much a passthrough to the layout, which will in turn switch all the icons inside it to normal mode graphics
	AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		//uhhhh..
		return;
	}
	pLayout->switchIconsToReorderGraphics();
}

//virtual
bool AlphabetPage::detectAndHandleSpecialReleaseAreas(int id,const QPointF& pageCoordinate)
{
	//TODO: IMPLEMENT (unfinished)

	PageAreas::Enum pageAreaOfIcon = classifyPageLocation(pageCoordinate);
	bool queryLauncher = false;
	switch (pageAreaOfIcon)
	{
	case PageAreas::LeftBorder:
//		qDebug() << __FUNCTION__ << ": Left Border";
		return false;
	case PageAreas::RightBorder:
//		qDebug() << __FUNCTION__ << ": Right Border";
		return false;
	case PageAreas::TopBorder:
//		qDebug() << __FUNCTION__ << ": Top Border";
		return false;
	case PageAreas::BottomBorder:
//		qDebug() << __FUNCTION__ << ": Bottom Border";
		return false;
	case PageAreas::Content:
//		qDebug() << __FUNCTION__ << ": Warm, chewy center!";
		return false;
	default:
		queryLauncher = true;
//		qDebug() << __FUNCTION__ << ": Nuthin'!";
		break;
	}
	if ((queryLauncher) && (m_qp_currentUIOwner))
	{
		LauncherAreas::Enum launcherArea = m_qp_currentUIOwner->classifyPageLocation(mapToItem(m_qp_currentUIOwner,pageCoordinate));
		switch (launcherArea)
		{
		case LauncherAreas::PageTabBar:
//			qDebug() << __FUNCTION__ << ": Page Tab Bar";
			return false;
			break;
		case LauncherAreas::QuickLaunchBar:
//			qDebug() << __FUNCTION__ << ": Quick Launch Bar";
//			return handleQuickLaunchSpecialReleaseArea(id,pageCoordinate);
			return false;
			break;
		case LauncherAreas::PageInner:
//			qDebug() << __FUNCTION__ << ": Launcher claims it's in the Page";
			return false;
		default:
//			qDebug() << __FUNCTION__ << ": Launcher Nuthin'!";
			return false;
		}
	}

	return true;
}

//virtual
bool AlphabetPage::handleQuickLaunchSpecialReleaseArea(int id,const QPointF& pageCoordinate)
{
	//drop the currently tracking item onto the QL bar
	m_qp_currentUIOwner->sendIconToQuickLaunchBar(m_qp_cached_trackingIcon);
	return false;	//false == do not suppress default finalize code; this dropping icon action is to be done "in parallel" and
	//unbeknownst to this page
}

//virtual
void AlphabetPage::touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{

	QPointF pageCoordinate = mapFromScene(scenePosition);
	if (detectAndHandleSpecialReleaseAreas(id,pageCoordinate))
	{
		//suppress the normal handling!!! However, make extra sure that the handler actually does the "cleanup" like release the icon; see below
		// this is somewhat different than detectAndHandleSpecialMoveAreas in that MoveAreas usually gets called many times
		// and doesn't really need to clean up/finalize anything
		return;
	}

	iconRelease();
	Page::touchTrackedPointReleased(id,scenePosition,lastScenePosition,initialPosition);
	resetEventCounters();
}

//virtual
void AlphabetPage::iconRelease()
{
	if (m_qp_cached_trackingIcon)
	{
		AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout);
		if (pLayout != NULL)
		{
			pLayout->stopTrackingIcon(m_qp_cached_trackingIcon->uid());
		}
		m_qp_cached_trackingIcon = 0;
		m_trackingIconUids = QPair<QUuid,QUuid>();
	}
	update();

}

void AlphabetPage::slotTrackedIconCancelTrack(const QUuid& uid)
{
	//TODO: TEMP: for now, this page only tracks one icon, so just kill that one w/o checking uid
	m_qp_cached_trackingIcon = 0;
	m_trackingIconUids = QPair<QUuid,QUuid>();
}

//virtual
void AlphabetPage::slotReorderInLayoutStarted()
{

}

//virtual
void AlphabetPage::slotReorderInLayoutEnded()
{
	//signal to save the launcher state

	//TODO: if in the end of the project, this function remains as just a re-emitter, then just optimize it out and
	//connect signal->signal
	//TODO: IMPLEMENT

}

//virtual
void AlphabetPage::slotLauncherCmdStartReorderMode()
{
	//this is merely a notification from launcher that some page started reordering, so this page
	// should switch to reorder mode graphics. No actual reordering will start until this page itself
	// starts to be messed w/ directly by the user (see all the various functions for this in the .h file)

	setPageMode(PageMode::Reorder);
}

//virtual
void AlphabetPage::slotLauncherCmdEndReorderMode()
{

	//This page type doesn't really have a reorder mode, so this is mainly just to swap the graphics back to no frames
	// and no badging. It's not as a complicated as ReorderablePage::slotLauncherCmdEndReorderMode(). (see that .cpp file)
	// However, one case does exist where a cleanup needs to be taken seriously: this page creates clones of icons it is
	// reordering, and if a clone doesn't make it to another page before an "end", then it could be lost (memory leak wise)
	// This is unlikely in Dfish so this will remain a
	// TODO: !!!


	setPageMode(PageMode::Normal);
}

////////////////////////////// -- DEBUG -- ///////////////////////////////////////////

#include <QDebug>
//virtual
void AlphabetPage::dbg_slotPrintLayout()
{
	AlphabetIconLayout * pLayout = qobject_cast<AlphabetIconLayout *>(m_qp_iconLayout.data());
	if (!pLayout)
	{
		//qDebug() << "Layout is not an alpha layout or is null";
		return;
	}
}

