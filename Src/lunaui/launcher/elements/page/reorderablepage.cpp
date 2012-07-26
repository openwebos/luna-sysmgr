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




#include "reorderablepage.h"
#include "reorderableiconlayout.h"
#include "icon.h"
#include "pixmapobject.h"
#include "pixmaphugeobject.h"
#include "scrollingsurface.h"
#include "scrollinglayoutrenderer.h"
#include "textbox.h"
#include "picturebox.h"
#include "stringtranslator.h"
#include "dimensionslauncher.h"
#include "quicklaunchbar.h"
#include "vcamera.h"
#include "dimensionsmain.h"
#include "iconheap.h"
#include "pixmaploader.h"
#include "iconlayoutsettings.h"
#include "gfxsettings.h"

#include <QPainter>
#include <QEvent>
#include <QGesture>
#include <QGestureEvent>
#include <QGraphicsSceneMouseEvent>
#include <QDateTime>

#include "Localization.h"
//static std::string EmptyPageText()
//{
//	static std::string s = LOCALIZED("Press and hold any app icon, then drag it here to add a favorite app.");
//	return s;
//}

static QString EmptyPageText()
{
	static QString s = StringTranslator::inputString(LOCALIZED(
			StringTranslator::outputString(IconLayoutSettings::settings()->reorderablelayout_emptyPageText)));
	return s;
}

#define MOVING_ICON_Y_OFFSET 15

#define EMPTY_ICON_FILEPATH QString("/launcher-empty-page.png")

const char * ReorderablePage::IsReorderingLayoutBoolPropertyName = "layout_in_reorder";

ReorderablePage::ReorderablePage(const QRectF& pageGeometry,LauncherObject * p_belongsTo)
: Page(pageGeometry,p_belongsTo)			//make it explicit in case ctors change
, m_pageMode(PageMode::Normal)
, m_qp_iconInMotion(0)
, m_qp_emptyPageText(0)
, m_qp_emptyPageIcon(0)
{
	commonCtor();
}

ReorderablePage::ReorderablePage(const QUuid& specificUid,const QRectF& pageGeometry,LauncherObject * p_belongsTo)
: Page(specificUid,pageGeometry,p_belongsTo)			//make it explicit in case ctors change
, m_pageMode(PageMode::Normal)
, m_qp_iconInMotion(0)
, m_qp_emptyPageText(0)
, m_qp_emptyPageIcon(0)
{
	commonCtor();
}

//virtual
void ReorderablePage::commonCtor()
{
	QString emptyText = EmptyPageText();
	if (!emptyText.isEmpty())
	{
//		m_qp_emptyPageText = new TextBox(0,m_geom.adjusted(m_geom.width()/3.0,m_geom.height()/4.0,
//				-m_geom.width()/3.0,-m_geom.height()/4.0),
//				EmptyPageText());
		QRectF textBoxGeom = DimensionsGlobal::realRectAroundRealPoint(IconLayoutSettings::settings()->reorderablelayout_emptyPageTextBoxSize);
		m_qp_emptyPageText = new TextBox(0,textBoxGeom,
				EmptyPageText());
		m_qp_emptyPageText->setParentItem(this);
//		m_qp_emptyPageText->setPos(0.0,m_geom.top()-m_qp_emptyPageText->geometry().top()+m_geom.height()/15);
		m_qp_emptyPageText->setPos(m_geom.center()+IconLayoutSettings::settings()->reorderablelayout_emptyPageTextOffsetFromCenter);
		m_qp_emptyPageText->setVisible(true);
	}

	//load the empty page icon, if there is one
	PixmapObject * pEmptyPicPmo =
			PixmapObjectLoader::instance()->quickLoad(QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + EMPTY_ICON_FILEPATH));

	if (pEmptyPicPmo)
	{

		m_qp_emptyPageIcon = new PictureBox(pEmptyPicPmo);
		m_qp_emptyPageIcon->setParentItem(this);
		m_qp_emptyPageIcon->setPos(m_geom.center()+IconLayoutSettings::settings()->reorderablelayout_emptyPageIconOffsetFromCenter);
		m_qp_emptyPageIcon->setVisible(true);
	}
}

//virtual
ReorderablePage::~ReorderablePage()
{
}

bool ReorderablePage::layoutFromItemList(const ThingList& items)
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

bool ReorderablePage::layoutFromItemList(const IconList& items)
{
	if (m_qp_emptyPageText)
	{
		m_qp_emptyPageText->setVisible((items.isEmpty() ? true : false));
		if (m_qp_emptyPageIcon)
		{
			m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
		}
	}

	//if there is a current layout, then get rid of it
	delete m_qp_iconLayout;
	ReorderableIconLayout * pReorderLayout = new ReorderableIconLayout(this);

	QSize newLayoutSize = QSize(m_geom.size().toSize().width() - (m_geom.size().toSize().width() % 2),
					m_geom.size().toSize().height() - (m_geom.size().toSize().height() % 2));

	pReorderLayout->resizeWidth((quint32)(newLayoutSize.width()));

	//connect here to avoid potential emits from these signals due to the resizeWidth
	connect(pReorderLayout,SIGNAL(signalReorderStarted()),
			this,SLOT(slotReorderInLayoutStarted()));
	connect(pReorderLayout,SIGNAL(signalReorderEnded()),
			this,SLOT(slotReorderInLayoutEnded()));

	//now init the layout from the icon list
	ReorderableIconLayout::initLayoutFromSequentialIconList(*pReorderLayout,items);
	m_qp_iconLayout = pReorderLayout;

	m_qp_iconLayout->recomputePageLayoutTransforms();

	//dbg_slotPrintLayout();
	//set the scrollable with the layout

	m_p_scroll = new ScrollingLayoutRenderer(DimensionsGlobal::realRectAroundRealPoint(newLayoutSize),*pReorderLayout);
	m_p_scroll->setParentItem(this);
	m_p_scroll->enable();

	m_failmaticScroller.setMinValue((qreal)(m_p_scroll->topLimit()));
	m_failmaticScroller.setMaxValue((qreal)(m_p_scroll->bottomLimit()));
	m_failmaticScroller.setMaxOverscroll(10.0);
	m_failmaticScroller.setScrollOffset(0.0);

//	DotGrid * p_dbg_grid = new DotGrid(m_p_scroll->geometry(),QSize(20,20));
//	p_dbg_grid->setParentItem(this);
	return true;
}

//virtual
bool ReorderablePage::canAcceptIcons() const
{
	return true;
}

//virtual
PageMode::Enum ReorderablePage::pageMode() const
{
	return m_pageMode;
}

//virtual
void ReorderablePage::setPageMode(PageMode::Enum v)
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
void ReorderablePage::switchToNormalMode()
{
	//pretty much a passthrough to the layout, which will in turn switch all the icons inside it to normal mode graphics
	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		//uhhhh..
		return;
	}
	pLayout->switchIconsToNormalGraphics();
}

//virtual
void ReorderablePage::switchToReorderMode()
{
	//pretty much a passthrough to the layout, which will in turn switch all the icons inside it to normal mode graphics
	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		//uhhhh..
		return;
	}
	pLayout->switchIconsToReorderGraphics();
}

//virtual
bool ReorderablePage::resize(quint32 w, quint32 h)
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
void ReorderablePage::paintOffscreen(QPainter *painter)
{
	//TODO: IMPLEMENT
}

//virtual
IconBase * ReorderablePage::iconAtLayoutCoordinate(const QPointF& layoutCoord,QPoint& r_gridCoordinate,QPointF * r_p_intraIconCoord)
{
	QPointF pt;
	if (!m_qp_iconLayout)
	{
		//no layout..
		return 0;
	}

	if (m_p_scroll)
	{
		if (!m_p_scroll->mapToContentSpace(layoutCoord,pt))
		{
			//qDebug() << "point " << layoutCoord << " isn't inside the layout";
			return 0;
		}
	}
	else
	{
		//no scroller, point is 1:1 to layout
		// TODO: consider offsets, etc
		pt = layoutCoord;
	}

//	//qDebug() << __PRETTY_FUNCTION__ << ": point click " << layoutCoord
//						<< " mapped to content point " << pt;
	IconCell * p_iconCell = m_qp_iconLayout->iconCellAtLayoutCoordinate(pt,r_gridCoordinate);
	if (p_iconCell)
	{
		if (r_p_intraIconCoord)
		{
			*r_p_intraIconCoord = pt - p_iconCell->relativeGeometry().center();
		}
		//qDebug() << "found icon cell at this point!";
		if (p_iconCell->m_qp_icon)
		{
			//qDebug() << "icon: " << p_iconCell->m_qp_icon->property("iconlabel");
			return p_iconCell->m_qp_icon;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		//qDebug() << "no icon at " << layoutCoord << " in the layout";
	}
	return 0;
}

//virtual
qint32 ReorderablePage::rowAtPageCoordinate(const QPointF& pageCoord)
{
	QPointF pt;
	if (!m_qp_iconLayout)
	{
		//no layout..
		return -1;
	}

	if (m_p_scroll)
	{
		if (!m_p_scroll->mapToContentSpace(pageCoord,pt))
		{
			//qDebug() << "point " << layoutCoord << " isn't inside the layout";
			return -1;
		}
	}
	else
	{
		//no scroller, point is 1:1 to layout
		// TODO: consider offsets, etc
		pt = pageCoord;
	}

	return m_qp_iconLayout->rowAtLayoutCoordinate(pt);
}

///////////////////////////////    UI - GESTURES AND EVENTS ///////////////////////////

//TODO: TEMP: clean this up and move to the right place to make OOP gods happy
static qint32 gEventCounter0 = 0;

static void resetEventCounters()
{
	gEventCounter0 = 0;
}

//virtual
bool ReorderablePage::tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent)
{
	//ignore if an icon is currently moving (tracking)
	//TODO: MULTI-TOUCH: (obvious)
	if (m_qp_iconInMotion)
	{
		//yep..
		g_warning("[ICON-TAP-TRACE] %s: m_qp_iconInMotion == true, early-exit",__FUNCTION__);
		return true;
	}

	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return true;
	}

	//figure out which (if any) icon is being acted on
	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapEvent->hotSpot()));
	QPoint gridCoord;
	QPointF intraIconPositionOfTapICS;
	IconBase * pIcon = iconAtLayoutCoordinate(positionOfTapICS,gridCoord,&intraIconPositionOfTapICS);
	if (pIcon == NULL)
	{
		//no icon there....if in reorder mode, then simulate a "Done" button tap to get out of it

		g_warning("[ICON-TAP-TRACE] %s: no icon at point coordinate (%d,%d), early-exit",
				__FUNCTION__,(int)(positionOfTapICS.x()),(int)(positionOfTapICS.y()));

		if (pageMode() == PageMode::Reorder)
		{
			Q_EMIT signalPageRequestExitReorder();
		}
		return true;
	}

	//check to see if the icon should handle the event internally
	IconInternalHitAreas::Enum hitArea;
	if (pIcon->tapIntoIcon(intraIconPositionOfTapICS,hitArea))
	{
//		qDebug() << "Hoorah!";
//		g_warning("[ICON-TAP-TRACE] %s: icon dealt with tap internally",__FUNCTION__);
	}
	else
	{
		//qDebug() << "Icon didn't want to deal with a intra-icon coord claiming to be " << intraIconPositionOfTapICS;
//		g_warning("[ICON-TAP-TRACE] %s: icon didn't want to deal with tap internally",__FUNCTION__);
		if (hitArea == IconInternalHitAreas::Other)
		{
			Q_EMIT signalIconActivatedTap(pIcon);
		}
	}
	return true;
}

//virtual
bool ReorderablePage::tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent)
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

	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return true;
	}

	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapHoldEvent->hotSpot()));
//	qDebug() << "tap-n-hold position from the event: " << positionOfTapICS;

	//TODO: TEMP: the tap coordinate is in PAGE ICS...the layout ICS is a little bit different. It needs to be mapped for total correctness
	//		for now, this will work

	//first, it there is an icon already in motion, silently consume event (yum!)
	if (m_qp_iconInMotion)
	{
		return true;
	}

	//figure out which (if any) icon is being acted on
	if (!iconAtLayoutCoordinate(positionOfTapICS,m_iconInMotionCurrentGridPosition))
	{
		//no icon there
		return true;
	}
	//from here on in, needs to be a reorderable layout for things to not get too general and abstract

	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		return true;
	}

	//check to see if the grid coordinate given represents a row that is COMPLETELY within the rendered view.
	QRectF gridRowArea = pLayout->rowArea(m_iconInMotionCurrentGridPosition.y());
	if ((pageCoordinateFromLayoutCoordinate(gridRowArea.topLeft()) == QPointF())
			|| (pageCoordinateFromLayoutCoordinate(gridRowArea.bottomRight()) == QPointF()))
	{
		//the row is not completely rendered in the page at the moment.
		//silently return
//		qDebug() << "The row at grid-y =  " << m_iconInMotionCurrentGridPosition.y() << " is not completely rendered. Not doing any moves";
		return true;
	}

	//if I'm not in reorder mode, then flip to it now; this is just going to change all the graphics in the icons
	// and enable them to sense taps on special, decorator areas
	setPageMode(PageMode::Reorder);	//note calling this when already in Reorder mode does nothing, so it's ok to just call it and avoid the "if"

	//this had better not fail
	IconBase * pIcon = pLayout->startTrackingIcon(m_iconInMotionCurrentGridPosition);
	if (!pIcon)
	{
		//Ugh!!! ...this being out-of-sync with iconAtLayoutCoordinate() is very very bad!!! --go check code@!@@
		return true;
	}

	//this icon shouldn't have the decorator while it's moving
	pIcon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
	pIcon->take(this);	///theoretically this could be rejected! //TODO: handling
	pIcon->clearAutopaintClipRect();
	pIcon->slotEnableIconAutoRepaint();
	pIcon->setParentItem(this);
	pIcon->setVisible(true);
	pIcon->setPos(positionOfTapICS - QPointF(0, MOVING_ICON_Y_OFFSET));
	m_qp_iconInMotion = pIcon;
	pIcon->update();
	return true;
//
//	if (m_qp_goggles)
//	{
//		delete m_qp_goggles;
//	}
//
//	qDebug() << "STARTING BLUR3";
//	//take a picture
//	//TODO: DEBUG: TEST: take a pic with the vcamera
//	Q_EMIT dbg_signalTriggerCamera(pIcon);
//	VirtualCamera * pVcam = LauncherObject::primaryInstance()->virtualCamera();
//	QRectF viewRect;
//	PixmapObject * bgpmo = pVcam->lastImageCaptured(&viewRect);
//	m_qp_goggles = new DemoGoggles3(m_presetGoggleGeom,bgpmo,viewRect);
//	m_qp_goggles->setParentItem(this);
//	m_qp_goggles->activate();
//	m_qp_goggles->setPos(positionOfTapICS);
//	m_qp_goggles->setVisible(true);
//	return true;
}

//GEMSTONE-RD
////virtual
//bool ReorderablePage::acceptIncomingIcon(IconBase * p_newIcon)
//{
//	if (p_newIcon == 0)
//	{
//		qDebug() << __FUNCTION__ << ": error: null icon parameter";
//		return false;
//	}
//	//if there is already an icon inflight, i'll disallow this
//	//TODO: MULTI-TOUCH: this would not be a very good restriction
//	if (m_qp_iconInMotion)
//	{
//		qDebug() << __FUNCTION__ << ": error: a reorder is currently in progress";
//		return false;
//	}
//	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
//	if (!pLayout)
//	{
//		qDebug() << __FUNCTION__ << ": error: no layout!?";
//		return false;
//	}
//
//	//create a new reorder cell for this icon
//	m_iconInMotionCurrentGridPosition = pLayout->addEmptyCell();
//	if (m_iconInMotionCurrentGridPosition.x() == -1)	//the special value to indicate a fail
//	{
//		qDebug() << __FUNCTION__ << ": error: failed to add an empty cell to start reorder of new icon";
//		return false;
//	}
//	//relayout the icons
//	pLayout->relayoutExisting();
//	if (pLayout->m_iconRows.isEmpty())
//	{
//		if (m_qp_emptyPageText)
//		{
//			m_qp_emptyPageText->setVisible(true);
//		}
//	}
//	else
//	{
//		if (m_qp_emptyPageText)
//		{
//			m_qp_emptyPageText->setVisible(false);
//		}
//	}
//	//the size of the layout could have changed
//	if (m_p_scroll)
//	{
//		//TODO: TEMP: the scroller should get a signal from the layout that the
//		// geom has changed
//		m_p_scroll->slotSourceContentSizeChanged(pLayout->geometry().size());
//	}
//	//after this call, pIcon should == p_newIcon...
//	IconBase * pIcon = pLayout->startTrackingIconFromTransfer(m_iconInMotionCurrentGridPosition,p_newIcon);
//	pIcon->slotEnableIconAutoRepaint();
//	QPointF iconScenePos = p_newIcon->mapToScene(p_newIcon->pos());
//	pIcon->take(this);	///theoretically this could be rejected! //TODO: handling
//	pIcon->setVisible(true);
//	pIcon->setPos(this->mapFromScene(iconScenePos));
//	m_qp_iconInMotion = pIcon;
//	pIcon->update();
//	return true;
//}
//GEMSTONE-RD

////virtual
bool ReorderablePage::flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent)
{
	//first, it there is an icon already in motion, silently consume event (yum!)
	if (m_qp_iconInMotion)
	{
		return true;
	}

	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return true;
	}

	return Page::flickGesture(flickEvent, baseGestureEvent);
}


bool ReorderablePage::acceptIncomingIcon(IconBase * p_newIcon)
{
	if (p_newIcon == 0)
	{
		qDebug() << __FUNCTION__ << ": error: null icon parameter";
		return false;
	}

	//mark this page as the last to be "responsible" for this icon
	p_newIcon->setProperty(IconBase::IconLastPageVisitedIndexPropertyName,pageIndex());

	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		qDebug() << __FUNCTION__ << ": error: no layout!?";
		return false;
	}

	//cleanup and commit all reorders right now! accepting a new icon cannot be done while reorders are pending
	pLayout->cancelAllReorder();

	//create a new reorder cell for this icon
	m_iconInMotionCurrentGridPosition = pLayout->addEmptyCell();
	if (m_iconInMotionCurrentGridPosition.x() == -1)	//the special value to indicate a fail
	{
		qDebug() << __FUNCTION__ << ": error: failed to add an empty cell to start reorder of new icon";
		return false;
	}
	//relayout the icons
	pLayout->relayoutExisting();
	if (pLayout->m_iconRows.isEmpty())
	{
		if (m_qp_emptyPageText)
		{
			m_qp_emptyPageText->setVisible(true);
			m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
		}
	}
	else
	{
		if (m_qp_emptyPageText)
		{
			m_qp_emptyPageText->setVisible(false);
			m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
		}
	}
	//the size of the layout could have changed
	if (m_p_scroll)
	{
		//TODO: TEMP: the scroller should get a signal from the layout that the
		// geom has changed
		m_p_scroll->slotSourceContentSizeChanged(pLayout->geometry().size());
		m_failmaticScroller.setMinValue((qreal)(m_p_scroll->topLimit()));
		m_failmaticScroller.setMaxValue((qreal)(m_p_scroll->bottomLimit()));
		m_failmaticScroller.setMaxOverscroll(10.0);
		m_failmaticScroller.setScrollOffset(0.0);
	}
	//after this call, pIcon should == p_newIcon...
	IconBase * pIcon = pLayout->startTrackingIconFromTransfer(m_iconInMotionCurrentGridPosition,p_newIcon);
	if (!pIcon)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": FATALITY: got all the way into (and past) startTrackingIconFromTransfer() in this fn, only to have startTrackingIconFromTransfer() return NULL";
		qFatal("death by startTrackingIconFromTransfer -> null");
		exit(-1);	//just in case;
	}
	pIcon->slotEnableIconAutoRepaint();

//	Page * pPrevPage = qobject_cast<Page *>(p_newIcon->parentObject());
//	if (pPrevPage)
//	{
//		qDebug() << "icon's owner is page " << pPrevPage->pageIndex();
//	}
//	else
//	{
//		qDebug() << "icon's prev owner isn't a page!? ("
//				<< (p_newIcon->parentObject() ? QString(p_newIcon->parentObject()->metaObject()->className()) : QString("(null)"))
//				<< ")";
//	}
//	qDebug() << "icon's position is " << p_newIcon->pos();

	m_qp_iconInMotion = pIcon;
	return true;
}

//virtual
bool ReorderablePage::releaseTransferredIcon(IconBase * p_transferredIcon)
{
	//this icon has to be the one that was in motion
	//TODO: this becomes more complex if MULTI-TOUCH and multiple icons can be in motion
	if (!p_transferredIcon || (p_transferredIcon != m_qp_iconInMotion))
	{
		return false;
	}

	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (pLayout != NULL)
	{
		pLayout->trackedIconLeavingLayout(m_qp_iconInMotion->uid());
		//relayout the icons
		pLayout->relayoutExisting();
		//if there are no rows left, then assure that the empty page text is shown
		if (pLayout->m_iconRows.empty())
		{
			if (m_qp_emptyPageText)
			{
				m_qp_emptyPageText->setVisible(true);
				m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
			}
		}
		else
		{
			if (m_qp_emptyPageText)
			{
				m_qp_emptyPageText->setVisible(false);
				m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
			}
		}
		//the size of the layout could have changed
		if (m_p_scroll)
		{
			//TODO: TEMP: the scroller should get a signal from the layout that the
			// geom has changed
			m_p_scroll->slotSourceContentSizeChanged(pLayout->geometry().size());
			m_failmaticScroller.setMinValue((qreal)(m_p_scroll->topLimit()));
			m_failmaticScroller.setMaxValue((qreal)(m_p_scroll->bottomLimit()));
			m_failmaticScroller.setMaxOverscroll(10.0);
			m_failmaticScroller.setScrollOffset(0.0);
		}
	}

	m_qp_iconInMotion = 0;
	//TODO: MULTI-TOUCH

	return true;

}


//virtual
bool ReorderablePage::addIcon(IconBase * p_icon)
{
	///DON'T USE THESE add__ functions for reordering "add" from other pages. See .h file...
	return addIconNoAnimations(p_icon);
}

//virtual
bool ReorderablePage::addIconNoAnimations(IconBase * p_icon)
{
	if (!p_icon)
	{
		return false;
	}
	//if there is an icon tracking or a reorder going on, then the add can't complete immediately. Waitlist it
	if (m_qp_iconInMotion)
	{
		qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") : currently tracking an icon, so waitlisting this add";
		addIconToPageAddWaitlist(p_icon);
		return true;
	}

	//there needs to be a layout
	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") error: no layout";
		return false;
	}

	p_icon->setUsePrerenderedLabel(true);

	//if the page is in reorder mode, then make the icon match
	if (pageMode() == PageMode::Reorder)
	{
		pLayout->switchIconToReorderGraphics(p_icon);
	}
	//set the other things appropriately
	p_icon->slotDisableIconAutoRepaint();
	p_icon->setParentItem(this);
	p_icon->take(this);			///this won't do anything bad if the icon was already "taken" by this page; and it should have been in normal cases.
								/// it's placement here also addresses the cases where it just came into creation and is being placed on this page
								/// (e.g. slotAppAdd in launcherobject)

	//TODO: SLOW: TEMP:
	//	For now though, I'll do something similarly simpler-but-slower as what removeIcon does: Insert the icon at the end of the layout,
	//	then destroy all the existing rows and relayout them into new rows

	qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") trying to add icon to layout - current size is " << pLayout->geometry().size();
	pLayout->addIcon(p_icon);
	pLayout->relayoutExisting();

	if (pLayout->m_iconRows.isEmpty())
	{
		if (m_qp_emptyPageText)
		{
			m_qp_emptyPageText->setVisible(true);
			m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
		}
	}
	else
	{
		if (m_qp_emptyPageText)
		{
			m_qp_emptyPageText->setVisible(false);
			m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
		}
	}

	//the size of the layout could have changed
	if (m_p_scroll)
	{
		//TODO: TEMP: the scroller should get a signal from the layout that the
		// geom has changed
		m_p_scroll->slotSourceContentSizeChanged(pLayout->geometry().size());
		qDebug() << __FUNCTION__ << ": reporting new layout size as " << pLayout->geometry().size();
		m_failmaticScroller.setMinValue((qreal)(m_p_scroll->topLimit()));
		m_failmaticScroller.setMaxValue((qreal)(m_p_scroll->bottomLimit()));
		m_failmaticScroller.setMaxOverscroll(10.0);
		m_failmaticScroller.setScrollOffset(0.0);
	}
	update();
	qDebug() << __FUNCTION__ << ": (icon = " << p_icon->uid() << ") done...exiting";
	return true;
}

//virtual
bool ReorderablePage::removeIcon(const QUuid& iconUid)
{
	//TODO: IMPLEMENT
	// for now it'll just be a passthrough to removeIconNoAnimations()

	qDebug() << __FUNCTION__ << ": trying to remove icon " << iconUid;
	return removeIconNoAnimations(iconUid);
}

//virtual
bool ReorderablePage::removeIconNoAnimations(const QUuid& iconUid)
{
	//if there is an icon tracking or a reorder going on, then waitlist the remove
	if (m_qp_iconInMotion)
	{
		qDebug() << __FUNCTION__ << ": (iconUid (for removal) = " << iconUid << ") : currently tracking an icon, so waitlisting this remove";
		addIconToPageRemoveWaitlist(iconUid);
		return true;
	}

	//there needs to be a layout
	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		qDebug() << __FUNCTION__ << " if (!pLayout) return false ";
		return false;
	}

	//find the icon in the layout
	QPoint iconCoord;
	if (!pLayout->findIconByUid(iconUid,iconCoord,true))
	{
		//icon not found in this layout/page
		qDebug() << __FUNCTION__ << " icon not found in this layout/page";
		return false;
	}

	if (pLayout->removeIconCell(iconCoord))
	{
		//relayout the icons
		pLayout->relayoutExisting();
		//if there are no rows left, then assure that the empty page text is shown
		if (pLayout->m_iconRows.empty())
		{
			if (m_qp_emptyPageText)
			{
				m_qp_emptyPageText->setVisible(true);
				m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
			}
		}
		else
		{
			if (m_qp_emptyPageText)
			{
				m_qp_emptyPageText->setVisible(false);
				m_qp_emptyPageIcon->setVisible(m_qp_emptyPageText->isVisible());
			}
		}
		//the size of the layout could have changed
		if (m_p_scroll)
		{
			//TODO: TEMP: the scroller should get a signal from the layout that the
			// geom has changed
			m_p_scroll->slotSourceContentSizeChanged(pLayout->geometry().size());
			m_failmaticScroller.setMinValue((qreal)(m_p_scroll->topLimit()));
			m_failmaticScroller.setMaxValue((qreal)(m_p_scroll->bottomLimit()));
			m_failmaticScroller.setMaxOverscroll(10.0);
			m_failmaticScroller.setScrollOffset(0.0);
		}
		return true;
	}
	return false;
}

//virtual
void ReorderablePage::touchTrackedPointStarted(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{

	//if I am in reorder mode, then treat this just like a tap-n-hold. Else, exit
	if (pageMode() != PageMode::Reorder)
	{
		return;
	}

	QPointF positionOfTapICS = mapFromScene(scenePosition);
//	qDebug() << "tap-n-hold position from the event: " << positionOfTapICS;

	//TODO: TEMP: the tap coordinate is in PAGE ICS...the layout ICS is a little bit different. It needs to be mapped for total correctness
	//		for now, this will work

	//first, it there is an icon already in motion, silently consume event (yum!)
	if (m_qp_iconInMotion)
	{
		return;
	}

	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return;
	}

	//figure out which (if any) icon is being acted on
	IconBase * pIcon = 0;
	QPointF intraIconPositionOfTapICS;
	if (!(pIcon = iconAtLayoutCoordinate(positionOfTapICS,m_iconInMotionCurrentGridPosition,&intraIconPositionOfTapICS)))
	{
		//no icon there
		return;
	}

	//since this is basically a tap (start), find out if the icon wants to deal with it because one of its active areas was hit
	// (e.g. the delete decorator)
	//check to see if the icon should handle the event internally
	IconInternalHitAreas::Enum hitArea;
	if (pIcon->tapIntoIcon(intraIconPositionOfTapICS,hitArea))
	{
		//it will handle it. Bail here just to prevent further processing; the tapGesture or some other gesture handler will do what needs to be done
		g_warning("[ICON-TAP-TRACE] %s: Bail here just to prevent further processing; the tapGesture or some other gesture handler will do what needs to be done",__FUNCTION__);
		return;
	}

	//from here on in, needs to be a reorderable layout for things to not get too general and abstract
	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (!pLayout)
	{
		return;
	}

	pLayout->m_lastTimeUsed = QDateTime::currentMSecsSinceEpoch();

	//check to see if the grid coordinate given represents a row that is COMPLETELY within the rendered view.
	QRectF gridRowArea = pLayout->rowArea(m_iconInMotionCurrentGridPosition.y());
	if ((pageCoordinateFromLayoutCoordinate(gridRowArea.topLeft()) == QPointF())
			|| (pageCoordinateFromLayoutCoordinate(gridRowArea.bottomRight()) == QPointF()))
	{
		//the row is not completely rendered in the page at the moment.
		//silently return
//		qDebug() << "The row at grid-y =  " << m_iconInMotionCurrentGridPosition.y() << " is not completely rendered. Not doing any moves";
		return;
	}

	//if I'm not in reorder mode, then flip to it now; this is just going to change all the graphics in the icons
	// and enable them to sense taps on special, decorator areas
	setPageMode(PageMode::Reorder);	//note calling this when already in Reorder mode does nothing, so it's ok to just call it and avoid the "if"

	//this had better not fail
	pIcon = pLayout->startTrackingIcon(m_iconInMotionCurrentGridPosition);
	if (!pIcon)
	{
		//Ugh!!! ...this being out-of-sync with iconAtLayoutCoordinate() is very very bad!!! --go check code@!@@
		return;
	}

	//this icon shouldn't have the decorator while it's moving
	pIcon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
	pIcon->take(this);	///theoretically this could be rejected! //TODO: handling
	pIcon->clearAutopaintClipRect();
	pIcon->slotEnableIconAutoRepaint();
	pIcon->setParentItem(this);
	pIcon->setVisible(true);
	pIcon->setPos(positionOfTapICS - QPointF(0, MOVING_ICON_Y_OFFSET));
	m_qp_iconInMotion = pIcon;
	pIcon->update();
	return;

}
//virtual
void ReorderablePage::touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{

	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return;
	}

	//if an icon is NOT being moved, then just punt to the Page:: handler
	if (!m_qp_iconInMotion)
	{
		return Page::touchTrackedPointMoved(id,scenePosition,lastScenePosition,initialPosition);
	}

	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (pLayout == NULL)
	{
		//no layout object???
		return Page::touchTrackedPointMoved(id,scenePosition,lastScenePosition,initialPosition);
	}

	QPointF pageCoordinate = mapFromScene(scenePosition);

	m_qp_iconInMotion->setPos(pageCoordinate - QPointF(0, MOVING_ICON_Y_OFFSET));

	quint32 timeUnitsElapsed =
			(quint32)((pLayout->m_lastTimeUsed = QDateTime::currentMSecsSinceEpoch() - pLayout->m_lastTimeUsed) / pLayout->m_timeNormalizationUnit);
	pLayout->m_lastTimeUsed = QDateTime::currentMSecsSinceEpoch();
	quint32 dist = (lastScenePosition.toPoint()-scenePosition.toPoint()).manhattanLength() << pLayout->m_magFactor;
	quint32 approxVelocity = ((timeUnitsElapsed == 0) ? INT_MAX :  dist / timeUnitsElapsed);

//	qDebug() << "approx Velocity = " << approxVelocity << "( " << "dist = " << dist << " , tunits = " << timeUnitsElapsed << ")";

	if ( ((quint32)(++gEventCounter0) >= pLayout->m_reorderEventSampleRate)
		&& (approxVelocity < pLayout->m_maxVelocityForSampling) )
	{
		//time to sample against the layout to see if moves need to be generated

		RedirectContext nullCtx;
		if (detectAndHandleSpecialMoveAreas(id,pageCoordinate,nullCtx))
		{
			//there was a special move generated...do not reset the event counter, just return;
			return;
		}
		QPoint gridCoord;
		if (iconAtLayoutCoordinate(pageCoordinate,gridCoord))
		{
			//check to see if the grid coordinate given represents a row that is COMPLETELY within the rendered view.
			QRectF gridRowArea = pLayout->rowArea(gridCoord.y());
			if ((pageCoordinateFromLayoutCoordinate(gridRowArea.topLeft()) == QPointF())
				|| (pageCoordinateFromLayoutCoordinate(gridRowArea.bottomRight()) == QPointF()))
			{
				//the row is not completely rendered in the page at the moment.
				//silently return
//				qDebug() << "The row at grid-y =  " << gridCoord.y() << " is not completely rendered. Not doing any moves";
				return;
			}
			if (gridCoord != m_iconInMotionCurrentGridPosition)
			{
				pLayout->trackedIconMovedTo(m_qp_iconInMotion,gridCoord,m_iconInMotionCurrentGridPosition);
			}
		}
		else
		{
			QPoint lastPosition = pLayout->lastOccupiedGridPosition();

			//if the icon moved to x > last cell, y within last row, OR y > last row, then interpret that as the last icon cell in the layout

			if (pLayout->m_iconRows.isEmpty())
			{
				//error! can't possibly have NO rows at this point. It means "add" icon never succeeded when this moving icon made it to this page
				return;
			}

			QPointF layoutCoordinate;
			if (m_p_scroll)
			{
				if (!m_p_scroll->mapToContentSpace(pageCoordinate,layoutCoordinate))
				{
					// pageCoordinate is outside the layout...

					//if the bottom row is completely rendered and the pageCoordinate is below it, then this is a move to the last position
					//check to see if the grid coordinate given represents a row that is COMPLETELY within the rendered view.
					QRectF gridRowArea = pLayout->rowArea(pLayout->m_iconRows.size()-1);
					QPointF bottomRightPt = pageCoordinateFromLayoutCoordinate(gridRowArea.bottomRight());
					if ((pageCoordinateFromLayoutCoordinate(gridRowArea.topLeft()) == QPointF())
							|| (pageCoordinateFromLayoutCoordinate(bottomRightPt) == QPointF()))
					{
						//the row is not completely rendered in the page at the moment.
						//silently return
//						qDebug() << "The row at grid-y =  " << (pLayout->m_iconRows.size()-1) << " is not completely rendered. Not doing any moves";
						return;
					}
					else
					{
//						qDebug() << "The row at grid-y =  " << (pLayout->m_iconRows.size()-1) << " IS completely rendered...";
						//last row is completely rendered
						if (bottomRightPt.y() < pageCoordinate.y())
						{
//							qDebug() << "... and moving to last spot!";
							pLayout->trackedIconMovedTo(m_qp_iconInMotion,lastPosition,m_iconInMotionCurrentGridPosition);
						}
						else
						{
//							qDebug() << "... and NOT moving to last spot: bottomRightPt = " << bottomRightPt << " vs pageCoordinate = " << pageCoordinate;
						}
					}
				}
			}
			else
			{
				//no scroller, point is 1:1 to layout
				layoutCoordinate = pageCoordinate;
			}

			qint32 rowDest = pLayout->rowAtLayoutCoordinateFuzzy(layoutCoordinate);
			if (rowDest == -1)
			{
				//ignore...it'll come back around
				return;
			}
//			qDebug() << "OUTER ---------- rowDest: " << rowDest << " , lastPosition: " << lastPosition
//										<< "\tlayoutCoord: " << layoutCoordinate << "\tpageCoordinate: " << pageCoordinate;
			if (rowDest == lastPosition.y())
			{
				IconCell * pLastCell = pLayout->iconCellAtGridCoordinate(lastPosition);
				qreal xBound = ( pLastCell ? pLastCell->relativeGeometry().right()
											: 0.0);
				if (layoutCoordinate.x() >= xBound)
				{
					//at the last row, to the right of last icon
					pLayout->trackedIconMovedTo(m_qp_iconInMotion,lastPosition,m_iconInMotionCurrentGridPosition);
				}
			}

		}
		//reset the event counter used
		gEventCounter0 = 0;
	}
}

//virtual
void ReorderablePage::redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	//at this point, the icon in motion should have been set at the transferred one...
	//TODO: MULTI-TOUCH: obviously...same as comments elsewhere...this only works for one touch point
//	qDebug() << __FUNCTION__ << " on page " << this->pageIndex();
	PageIconTransferRedirectContext const * pContext = qobject_cast<PageIconTransferRedirectContext const *>(&redirContext);
	if (pContext)
	{
		if (pContext->m_qp_icon == m_qp_iconInMotion)
		{
			//			qDebug() << __FUNCTION__;
			//if an icon is NOT being moved, then just punt to the Page:: handler
			if (!m_qp_iconInMotion)
			{
				//TODO: if Page handler can also redirect, then this will have to be split out as well into a Page::touchMoved and Page::redirectTouchMoved...
				return Page::touchTrackedPointMoved(id,scenePosition,lastScenePosition,initialPosition);
			}

			ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
			if (pLayout == NULL)
			{
				//no layout object???
				//TODO: if Page handler can also redirect, then this will have to be split out as well into a Page::touchMoved and Page::redirectTouchMoved...
				return Page::touchTrackedPointMoved(id,scenePosition,lastScenePosition,initialPosition);
			}

			QPointF pageCoordinate = mapFromScene(scenePosition);

			m_qp_iconInMotion->setPos(pageCoordinate - QPointF(0, MOVING_ICON_Y_OFFSET));
			quint32 timeUnitsElapsed =
					(quint32)((pLayout->m_lastTimeUsed = QDateTime::currentMSecsSinceEpoch() - pLayout->m_lastTimeUsed) / pLayout->m_timeNormalizationUnit);
			pLayout->m_lastTimeUsed = QDateTime::currentMSecsSinceEpoch();
			quint32 dist = (lastScenePosition.toPoint()-scenePosition.toPoint()).manhattanLength() << pLayout->m_magFactor;
			quint32 approxVelocity = ((timeUnitsElapsed == 0) ? INT_MAX :  dist / timeUnitsElapsed);

		//	qDebug() << "approx Velocity = " << approxVelocity << "( " << "dist = " << dist << " , tunits = " << timeUnitsElapsed << ")";

			if ( ((quint32)(++gEventCounter0) >= pLayout->m_reorderEventSampleRate)
				&& (approxVelocity < pLayout->m_maxVelocityForSampling) )
			{
				//time to sample against the layout to see if moves need to be generated

				if (detectAndHandleSpecialMoveAreas(id,pageCoordinate,redirContext))
				{
					//there was a special move generated...do not reset the event counter, just return;
					return;
				}
				QPoint gridCoord;
				if (iconAtLayoutCoordinate(pageCoordinate,gridCoord))
				{
					//check to see if the grid coordinate given represents a row that is COMPLETELY within the rendered view.
					QRectF gridRowArea = pLayout->rowArea(gridCoord.y());
					if ((pageCoordinateFromLayoutCoordinate(gridRowArea.topLeft()) == QPointF())
							|| (pageCoordinateFromLayoutCoordinate(gridRowArea.bottomRight()) == QPointF()))
					{
						//the row is not completely rendered in the page at the moment.
						//silently return
						//				qDebug() << "The row at grid-y =  " << gridCoord.y() << " is not completely rendered. Not doing any moves";
						return;
					}
					if (gridCoord != m_iconInMotionCurrentGridPosition)
					{
						pLayout->trackedIconMovedTo(m_qp_iconInMotion,gridCoord,m_iconInMotionCurrentGridPosition);
					}
				}
				else
				{
					QPoint lastPosition = pLayout->lastOccupiedGridPosition();

					//if the icon moved to x > last cell, y within last row, OR y > last row, then interpret that as the last icon cell in the layout

					if (pLayout->m_iconRows.isEmpty())
					{
						//error! can't possibly have NO rows at this point. It means "add" icon never succeeded when this moving icon made it to this page
						return;
					}

					QPointF layoutCoordinate;
					if (m_p_scroll)
					{
						if (!m_p_scroll->mapToContentSpace(pageCoordinate,layoutCoordinate))
						{
							// pageCoordinate is outside the layout...

							//if the bottom row is completely rendered and the pageCoordinate is below it, then this is a move to the last position
							//check to see if the grid coordinate given represents a row that is COMPLETELY within the rendered view.
							QRectF gridRowArea = pLayout->rowArea(pLayout->m_iconRows.size()-1);
							QPointF bottomRightPt = pageCoordinateFromLayoutCoordinate(gridRowArea.bottomRight());
							if ((pageCoordinateFromLayoutCoordinate(gridRowArea.topLeft()) == QPointF())
									|| (pageCoordinateFromLayoutCoordinate(bottomRightPt) == QPointF()))
							{
								//the row is not completely rendered in the page at the moment.
								//silently return
								//qDebug() << "The row at grid-y =  " << (pLayout->m_iconRows.size()-1) << " is not completely rendered. Not doing any moves";
								return;
							}
							else
							{
								//qDebug() << "The row at grid-y =  " << (pLayout->m_iconRows.size()-1) << " IS completely rendered...";
								//last row is completely rendered
								if (bottomRightPt.y() < pageCoordinate.y())
								{
									//qDebug() << "... and moving to last spot!";
									pLayout->trackedIconMovedTo(m_qp_iconInMotion,lastPosition,m_iconInMotionCurrentGridPosition);
								}
								else
								{
									//qDebug() << "... and NOT moving to last spot: bottomRightPt = " << bottomRightPt << " vs pageCoordinate = " << pageCoordinate;
								}
							}
						}
					}
					else
					{
						//no scroller, point is 1:1 to layout
						layoutCoordinate = pageCoordinate;
					}

					qint32 rowDest = pLayout->rowAtLayoutCoordinateFuzzy(layoutCoordinate);
					if (rowDest == -1)
					{
						//ignore...it'll come back around
						return;
					}
					if (rowDest == lastPosition.y())
					{
						IconCell * pLastCell = pLayout->iconCellAtGridCoordinate(lastPosition);
						qreal xBound = ( pLastCell ? pLastCell->relativeGeometry().right()
								: 0.0);
						if (layoutCoordinate.x() >= xBound)
						{
							//at the last row, to the right of last icon
							pLayout->trackedIconMovedTo(m_qp_iconInMotion,lastPosition,m_iconInMotionCurrentGridPosition);
						}
					}

				}
				//reset the event counter used
				gEventCounter0 = 0;
			}
		}
	}
}

//virtual
bool ReorderablePage::detectAndHandleSpecialMoveAreas(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{

	//TODO: IMPLEMENT (unfinished)

	PageAreas::Enum pageAreaOfIcon = classifyPageLocation(pageCoordinate);
	bool queryLauncher = false;
	bool result = false;
	bool pageTab = false;
	switch (pageAreaOfIcon)
	{
	case PageAreas::LeftBorder:
//		qDebug() << __FUNCTION__ << ": Left Border";
		result = handleLeftBorderSpecialMoveArea(id,pageCoordinate,redirContext);
		break;
	case PageAreas::RightBorder:
//		qDebug() << __FUNCTION__ << ": Right Border";
		result = handleRightBorderSpecialMoveArea(id,pageCoordinate,redirContext);
		break;
	case PageAreas::TopBorder:
//		qDebug() << __FUNCTION__ << ": Top Border";
		result = handleTopBorderSpecialMoveArea(id,pageCoordinate,redirContext);
  	        break;			//DangL.-coverity-investigation
	case PageAreas::BottomBorder:
//		qDebug() << __FUNCTION__ << ": Bottom Border";
		result = handleBottomBorderSpecialMoveArea(id,pageCoordinate,redirContext);
                break;
	case PageAreas::Content:
//		qDebug() << __FUNCTION__ << ": Warm, chewy center!";
		result = false;
		break;
	default:
		queryLauncher = true;
		result = false;
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
			result = handlePageTabBarSpecialMoveArea(id,pageCoordinate,redirContext);
			pageTab = true;
			break;
		case LauncherAreas::QuickLaunchBar:
//			qDebug() << __FUNCTION__ << ": Quick Launch Bar";
			result = handleQuickLaunchSpecialMoveArea(id,pageCoordinate,redirContext);
			break;
		case LauncherAreas::PageInner:
//			qDebug() << __FUNCTION__ << ": Launcher claims it's in the Page";
			result = false;
			break;
		default:
//			qDebug() << __FUNCTION__ << ": Launcher Nuthin'!";
			result = false;
			break;
		}
	}

	//handle the tab highlight clear manually, if the handlePageTab... function was NOT used
	if (!pageTab)
	{
		//my tab was not hit, so make sure it's unhighlighted
		PageTab * pTabForPage = m_qp_currentUIOwner->tabForPage(this);
		if (pTabForPage)
		{
			pTabForPage->slotUnHighlight();
		}
	}
	return result;
}

//virtual
bool ReorderablePage::handleTopBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{
	//generate an upward scroll of at least 1 row, and emit a scroll signal for Page's scroll FSM to kick in
	autoScrollUp();
	return true;
}

//virtual
bool ReorderablePage::handleBottomBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{
	//generate a downward scroll of at least 1 row, and emit a scroll signal for Page's scroll FSM to kick in
	autoScrollDown();
	return true;
}

bool ReorderablePage::handlePageTabBarSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{
	//redirect to the launcher
	if (m_qp_iconInMotion)
	{
		//redirect the movements to the launcher object, so that it can deal with movements via the tab bar
		//if this is already being redirected, then change redirection instead
		if (redirContext.isValid())
		{
			//yes, it's already being redirected. The source in the redirect context is the source page, so change its redirect to the launcher
			///....well, actually... it BETTER be a Page. And the context should be LauncherPageIconTransferRedirectContext
			PageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<PageIconTransferRedirectContext const *>(&redirContext);
			if (!pIconTxferCtx)
			{
				//yikes! what kind of $#@ redirect was this to begin with
				qDebug() << __FUNCTION__ << ": no redirect context!?";
				return false;
			}
			Page * pSourcePage = pIconTxferCtx->m_qp_srcPage;
			if (!pSourcePage)
			{
				//redirect context is corrupt apparently
				qDebug() << __FUNCTION__ << ": redirect context has no source!?";
				return false;
			}

			//check to make sure that this page's tab isn't being hit

			PageTab * pTabForPage = 0;
			Page * pPageTabHit = m_qp_currentUIOwner->testForIntersectOnPageTab(mapToScene(pageCoordinate),&pTabForPage);
			if (pPageTabHit == this)
			{
				//my own tab...so just ignore all this
				qDebug() << __FUNCTION__ << ": hit my own tab...ignoring";
				//but highlight it anyways for feedback
				pTabForPage->slotHighlight();
				return false;
			}
			else
			{
				//my tab was not hit, so make sure it's unhighlighted
				pTabForPage = m_qp_currentUIOwner->tabForPage(this);
				if (pTabForPage)
				{
					pTabForPage->slotUnHighlight();
				}
			}
			if (pSourcePage == this)
			{
				//I'm back to my own page, from where the redirections started.
				// just kill the redirect and start handling locally again
				qDebug() << __FUNCTION__ << ": page " << this->pageIndex() << " : self-redirect, cancelling original redirection";
				(void)cancelRedirection(id);

				return true;
			}
			qDebug() << __FUNCTION__ << ": page " << this->pageIndex() << " : change redirect";
			//signature, for ref: changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext)
			pSourcePage->changeRedirectTo(id,this,m_qp_currentUIOwner,new LauncherPageIconTransferRedirectContext(pSourcePage,m_qp_iconInMotion,LauncherPageIconTransferRedirectContext::TabBar));
		}
		else
		{
			//no redirect yet...set up a new redirect to launcher
			qDebug() << __PRETTY_FUNCTION__ << ": page " << this->pageIndex() << " : no redirect yet...set up a new redirect to launcher";
			redirectTo(id,m_qp_currentUIOwner,
					new LauncherPageIconTransferRedirectContext(this,m_qp_iconInMotion,LauncherPageIconTransferRedirectContext::TabBar));
		}

	}
	else
	{
		qDebug() << __FUNCTION__ << ": page " << this->pageIndex() << " : no icon in motion";
	}
	return true;
}
//virtual
bool ReorderablePage::handleLeftBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{
	//also, this only works if there is an icon that's being tracked

	if (m_qp_iconInMotion)
	{
		//ask if this transfer is ok
		if (!(m_qp_currentUIOwner->checkAllowedIconTransferInterPage(this,
				m_qp_currentUIOwner->pageLeft(this),
				m_qp_iconInMotion)
		))
		{
			//not allowed to move in this direction
			qDebug() << __FUNCTION__ << ": not allowed to move the icon to the Left";
			return false;
		}
		//redirect the movements to the launcher object, so that it can deal with panning right.
		//if this is already being redirected, then change redirection instead
		if (redirContext.isValid())
		{
			//yes, it's already being redirected. The source in the redirect context is the source page, so change its redirect to the launcher
			///....well, actually... it BETTER be a Page. And the context should be LauncherPageIconTransferRedirectContext
			PageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<PageIconTransferRedirectContext const *>(&redirContext);
			if (!pIconTxferCtx)
			{
				//yikes! what kind of redirect was this to begin with
				qDebug() << __FUNCTION__ << ": no redirect context!?";
				return false;
			}
			Page * pSourcePage = pIconTxferCtx->m_qp_srcPage;
			if (!pSourcePage)
			{
				//redirect context is corrupt apparently
				qDebug() << __FUNCTION__ << ": redirect context has no source!?";
				return false;
			}
			if (pSourcePage == this)
			{
				//I'm back to my own page, from where the redirections started.
				// just kill the redirect and start handling locally again
				(void)cancelRedirection(id);
//				qDebug() << __FUNCTION__ << ": self-redirect - canceling redirection for id " << id;
				return true;
			}
			//signature, for ref: changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext)
			pSourcePage->changeRedirectTo(id,this,m_qp_currentUIOwner,new LauncherPageIconTransferRedirectContext(pSourcePage,m_qp_iconInMotion,LauncherPageIconTransferRedirectContext::Left));
		}
		else
		{
			//no redirect yet...set up a new redirect to launcher
			redirectTo(id,m_qp_currentUIOwner,
				new LauncherPageIconTransferRedirectContext(this,m_qp_iconInMotion,LauncherPageIconTransferRedirectContext::Left));
		}
		//at this point it redirected left
	}
	return true;
}

//virtual
bool ReorderablePage::handleRightBorderSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{

	//also, this only works if there is an icon that's being tracked
	if (m_qp_iconInMotion)
	{
		//ask if this transfer is ok
		if (!(m_qp_currentUIOwner->checkAllowedIconTransferInterPage(this,
				m_qp_currentUIOwner->pageRight(this),
				m_qp_iconInMotion)
		))
		{
			//not allowed to move in this direction
			qDebug() << __FUNCTION__ << ": not allowed to move the icon to the Right";
			return false;
		}
		//redirect the movements to the launcher object, so that it can deal with panning right.
		//if this is already being redirected, then change redirection instead
		if (redirContext.isValid())
		{
			//yes, it's already being redirected. The source in the redirect context is the source page, so change its redirect to the launcher
			///....well, actually... it BETTER be a Page. And the context should be LauncherPageIconTransferRedirectContext
			PageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<PageIconTransferRedirectContext const *>(&redirContext);
			if (!pIconTxferCtx)
			{
				//yikes! what kind of $#@ redirect was this to begin with
				qDebug() << __FUNCTION__ << ": no redirect context!?";
				return false;
			}
			Page * pSourcePage = pIconTxferCtx->m_qp_srcPage;
			if (!pSourcePage)
			{
				//redirect context is corrupt apparently
				qDebug() << __FUNCTION__ << ": redirect context has no source!?";
				return false;
			}
			if (pSourcePage == this)
			{
				//I'm back to my own page, from where the redirections started.
				// just kill the redirect and start handling locally again
				(void)cancelRedirection(id);
				qDebug() << __FUNCTION__ << ": self-redirect - canceling redirection for id " << id;
				return true;
			}
			//signature, for ref: changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext)
			pSourcePage->changeRedirectTo(id,this,m_qp_currentUIOwner,new LauncherPageIconTransferRedirectContext(pSourcePage,m_qp_iconInMotion,LauncherPageIconTransferRedirectContext::Right));
		}
		else
		{
			//no redirect yet...set up a new redirect to launcher
			redirectTo(id,m_qp_currentUIOwner,
				new LauncherPageIconTransferRedirectContext(this,m_qp_iconInMotion,LauncherPageIconTransferRedirectContext::Right));
		}
		//at this point it redirected right
	}
	return true;
}

//virtual
bool ReorderablePage::handleQuickLaunchSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{
	QuickLaunchBar* quicklaunch = Quicklauncher::primaryInstance()->quickLaunchBar();

	if(!quicklaunch || !quicklaunch->canAcceptIcons()) // can't move icons to the QL right now
		return false;

	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if(!pLayout || pLayout->areTherePendingReorderAnimations()){ // only allow moving to QL when icons are no longer moving around
		return false;
	}

	//also, this only works if there is an icon that's being tracked

	if (m_qp_iconInMotion)
	{
		//redirect the movements to the launcher object, so that it can deal with panning right.
		//if this is already being redirected, then change redirection instead
		if (redirContext.isValid())
		{
			//yes, it's already being redirected. The source in the redirect context is the source page, so change its redirect to the quick launch
			///....well, actually... it BETTER be a Page. And the context should be LauncherPageIconTransferRedirectContext
			PageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<PageIconTransferRedirectContext const *>(&redirContext);
			if (!pIconTxferCtx)
			{
				//yikes! what kind of redirect was this to begin with
				qDebug() << __FUNCTION__ << ": no redirect context!?";
				return false;
			}
			Page * pSourcePage = pIconTxferCtx->m_qp_srcPage;
			if (!pSourcePage)
			{
				//redirect context is corrupt apparently
				qDebug() << __FUNCTION__ << ": redirect context has no source!?";
				return false;
			}
			if (pSourcePage == this)
			{
				//I'm back to my own page, from where the redirections started.
				// just kill the redirect and start handling locally again
				(void)cancelRedirection(id);
				return true;
			}

			// now clone and transfer the icon and set up a new redirect to the quick launch

			//copy it
			IconBase* pClone = IconHeap::iconHeap()->copyIcon(m_qp_iconInMotion->master()->uid());

			if(!pClone)
				return false;

			pClone->setPos(mapToScene(m_qp_iconInMotion->pos()));

			//this can all be done now via the offer()-take()... sequence. Just offer the icon to the quick launch bar
			if (!quicklaunch->offer((Thing *)(pClone),(Thing *)(this)))
			{
				//the destination refused the offer or the take failed
				IconHeap::iconHeap()->deleteIconCopy(pClone->uid());
				return false;
			}

			IconCell * pDstCell = pLayout->iconCellAtGridCoordinate(m_iconInMotionCurrentGridPosition);
			if(pDstCell) {
				m_qp_iconInMotion->setPos(pLayout->pageCoordinateFromLayoutCoordinate(pDstCell->position()));
			}

			pLayout->stopTrackingIcon(m_qp_iconInMotion);

			//signature, for ref: changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext)
			pSourcePage->changeRedirectTo(id,this,quicklaunch,new LauncherPageIconTransferRedirectContext(pSourcePage,pClone,LauncherPageIconTransferRedirectContext::QuickLaunch));

			resetEventCounters();
		}
		else
		{
			//no redirect yet...clone and transfer the icon and set up a new redirect to the quick launch

			//copy it
			IconBase* pClone = IconHeap::iconHeap()->copyIcon(m_qp_iconInMotion->master()->uid());

			if(!pClone)
				return false;

			pClone->setPos(mapToScene(m_qp_iconInMotion->pos()));

			//this can all be done now via the offer()-take()... sequence. Just offer the icon to the quick launch bar
			if (!quicklaunch->offer((Thing *)(pClone),(Thing *)(this)))
			{
				//the destination refused the offer or the take failed
				IconHeap::iconHeap()->deleteIconCopy(pClone->uid());
				return false;
			}

			IconCell * pDstCell = pLayout->iconCellAtGridCoordinate(m_iconInMotionCurrentGridPosition);
			if(pDstCell) {
				m_qp_iconInMotion->setPos(pLayout->pageCoordinateFromLayoutCoordinate(pDstCell->position()));
			}

			pLayout->stopTrackingIcon(m_qp_iconInMotion);

			redirectTo(id,quicklaunch,
				new LauncherPageIconTransferRedirectContext(this,pClone,LauncherPageIconTransferRedirectContext::QuickLaunch));

			resetEventCounters();
		}
		//at this point it redirected left so signal this to the FSM for controlling page moves
	}
	return true;
}


//virtual
void ReorderablePage::touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{
	if (m_interactionsBlocked)
	{
		//launcher is blocking interactions to pages. silently swallow
		g_warning("%s: page %s [%d] has interactions blocked, and is silently consuming this event",__FUNCTION__,qPrintable(pageName()),pageIndex());
		return;
	}

	iconRelease();
	Page::touchTrackedPointReleased(id,scenePosition,lastScenePosition,initialPosition);
	resetEventCounters();
}


//virtual
void ReorderablePage::redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	//TODO: MULTI-TOUCH: obviously...same as comments elsewhere...this only works for one touch point
	PageIconTransferRedirectContext const * pContext = qobject_cast<PageIconTransferRedirectContext const *>(&redirContext);
	if (pContext)
	{
		if (pContext->m_qp_icon == m_qp_iconInMotion)
		{
//			qDebug() << __FUNCTION__;
			touchTrackedPointReleased(id,scenePosition,lastScenePosition,initialPosition);
		}
	}
}

//virtual
void ReorderablePage::iconRelease()
{
	if (m_qp_iconInMotion)
	{
		ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
		if (pLayout != NULL)
		{
			pLayout->stopTrackingIcon(m_qp_iconInMotion);
		}
		m_qp_iconInMotion = 0;
		m_iconInMotionCurrentGridPosition = QPoint();
	}
	update();
}

void ReorderablePage::slotTrackedIconCancelTrack(const QUuid& uid)
{
	//TODO: TEMP: for now, this page only tracks one icon, so just kill that one w/o checking uid
	m_qp_iconInMotion = 0;
	m_iconInMotionCurrentGridPosition = QPoint();
}

//virtual
void ReorderablePage::slotReorderInLayoutStarted()
{

}

//virtual
void ReorderablePage::slotReorderInLayoutEnded()
{
	//signal to save the launcher state

	//TODO: if in the end of the project, this function remains as just a re-emitter, then just optimize it out and
	//connect signal->signal
	ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
	if (pLayout)
	{
		if (pLayout->isReorderStateConsistent())
		{
			qint32 unhandledWaitlisted = 0;
			//run the addWaitlistHandler(), to handle any apps that might have queued up icons for add
			unhandledWaitlisted += addWaitlistHandler();
			//and then the remove one
			unhandledWaitlisted += removeWaitlistHandler();
			//TODO: if there are unhandled items...queue up event to retry???
		}
		else
		{
			qDebug() << __FUNCTION__ << ": reorder state is still unstable";
			//TODO: have a way to queue up an event to retry this????
		}
	}
}

//virtual
void ReorderablePage::slotLauncherCmdStartReorderMode()
{
	//this is merely a notification from launcher that some page started reordering, so this page
	// should switch to reorder mode graphics. No actual reordering will start until this page itself
	// starts to be messed w/ directly by the user (see all the various functions for this in the .h file)

	setPageMode(PageMode::Reorder);
}

//virtual
void ReorderablePage::slotLauncherCmdEndReorderMode()
{
	//Be cautious! : in theory, a reorder operation -either tracked icon(s) returning or a reorder list pending -
	// can be going on. If that is the case, then a "cancel" or a "commit immediately" needs to happen. Otherwise, the launcher
	// (which is presumably the only thing signaling this) would need to detect that a page or pages have failed to exit reorder
	// and would need to retry the signal, and in the meantime block up every other user interaction so that the user can't
	// force the system more out of sync. This is complicated and bad...so the better solution is to just end reorder immediately
	// and synchronously; don't let this function exit without having done its duty
	// Practically though, at least for Dfish, there is no way a user will manage to hit the Done button to signal this
	// while things are still reordering...so we'll leave it as a...
	// TODO: !!!!

	setPageMode(PageMode::Normal);
}

//virtual
bool ReorderablePage::offer(Thing * p_offer,Thing * p_offeringThing)
{
	IconBase * pIconOffer = qobject_cast<IconBase *>(p_offer);
	if (!pIconOffer)
	{
		return false;
	}

	//TODO: don't accept duplicates
	bool fromQL = false;
	QuickLaunchBar * pQuicklaunch = qobject_cast<QuickLaunchBar *>(p_offeringThing);
	if (pQuicklaunch)
	{
		// if this is coming from the QuickLaunch, it must then be a Clone of an icon we were initially moving around on this page.
		// Make sure that is the case, accept the clone, destroy it, and restore a pointer to the original icon.
		IconBase * pMaster = pIconOffer->cloneOf();
		if (!pMaster) // error, this should have been a clone...
			return false;

		if (pMaster != m_qp_iconInMotion) // we can only accept the same icon we were moving previously
			return false;

	}

	//else, initiate a take
	bool takeSuccess = pIconOffer->take(this);
	if (takeSuccess)
	{
		if(!pQuicklaunch) {
			//it's my icon - at this point, the previous owner already released all interest in the icon, so I'm free to assimilate it!
			//TODO: special handling just in case it fails! hmmm...should probably send it back to where it came from immediately

			//TODO: HACK: check to see how this page got the icon...if it was a Tab drop, then don't acceptIncomingIcon(), but do
			// an addIcon instead
			if (LauncherObject::wasIconDroppedOnTab(pIconOffer))
			{
				addIcon(pIconOffer);
			}
			else
			{
				acceptIncomingIcon(pIconOffer);
			}
			LauncherObject::clearIconTxContext(pIconOffer);

			QPointF oldPos = pIconOffer->scenePos();
			pIconOffer->setParentItem(this);
			pIconOffer->setPos(mapFromScene(oldPos) - QPointF(0, MOVING_ICON_Y_OFFSET));
			pIconOffer->setVisible(true);
		} else {
			// get the clone back from the QL and destroy it

			IconBase * pMaster = pIconOffer->master();
			IconHeap::iconHeap()->deleteIconCopy(pIconOffer->uid());

			ReorderableIconLayout * pLayout = qobject_cast<ReorderableIconLayout *>(m_qp_iconLayout);
			QPoint currentGridPos;

			//commit the layout immediately
			pLayout->cancelAllReorder();

			if (! (pLayout->startTrackingIcon(pMaster,currentGridPos)))
			{
				qDebug() << "ReorderablePage::"<<__FUNCTION__ << ": didn't find the icon that's tracking in the page/layout!";
				return false;
			}
			else
			{
				m_qp_iconInMotion = pMaster;
				m_iconInMotionCurrentGridPosition = currentGridPos;
			}
			m_qp_iconInMotion->setVisible(true);
			m_qp_iconInMotion->setParentItem(this);
			m_qp_iconInMotion->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
			m_qp_iconInMotion->take(this);	///theoretically this could be rejected! //TODO: handling
			m_qp_iconInMotion->clearAutopaintClipRect();
			m_qp_iconInMotion->slotEnableIconAutoRepaint();
			m_qp_iconInMotion->setVisible(true);
			m_qp_iconInMotion->update();
		}

		update();
		return true;
	}
	return takeSuccess;
}

//virtual
bool ReorderablePage::take(Thing * p_takerThing)
{
	//sure, but i am a Page...so only launcher would conceivably do this
	bool ok = (m_qp_takerOwner ? m_qp_takerOwner->taking(this,p_takerThing) : true);
	if ((ok) && (m_qp_takerOwner))
	{
		m_qp_takerOwner->taken(this,p_takerThing);
	}
	return ok;
}

//virtual
bool ReorderablePage::taking(Thing * p_victimThing, Thing * p_takerThing)
{
	return true;	//allow abductions of anything i own
}

//virtual
void ReorderablePage::taken(Thing * p_takenThing,Thing * p_takerThing)
{
	IconBase * pTakenIcon = qobject_cast<IconBase *>(p_takenThing);
	if (!pTakenIcon)
	{
		return;	//don't know what this was...i only recognize icons
	}

	// Two cases here: another Page could have taken it, or the Quicklaunch took it
	//TODO: other cases???? (tab bar maybe?)

	Page * pPage = qobject_cast<Page *>(p_takerThing);
	if (pPage)
	{
		//another page took it...
		//TODO: release the icon from my domain: all tracking structures, layouts, etc
		releaseTransferredIcon(pTakenIcon);		//is this enough??
	}

	QuickLaunchBar * pQuicklaunch = qobject_cast<QuickLaunchBar *>(p_takerThing);
	if (pQuicklaunch)
	{
		//the quicklaunch is taking the icon away from a page (since there cannot be more than 1 quicklaunch bar)
		//TODO: this icon is actually a clone so there isn't anything to release
		// (this clone usage will be by convention.. The launcher will do it before it initiates the proxy offer()-take() sequence
	}

	LauncherObject * pLauncher = qobject_cast<LauncherObject *>(p_takerThing);
	if (pLauncher)
	{
		//launcher object took it (into the Limbo)
		//TODO: release the icon from my domain: all tracking structures, layouts, etc
		releaseTransferredIcon(pTakenIcon);  //is this enough??
	}
}

//virtual
void ReorderablePage::externalIconMoveTerminated()
{
	// this is invoked by the QuickLaunch when we drag an icon from a page to teh QL and drop it there
	// we need at this point stop tracking the icon we were dragging
	iconRelease();
}

//virtual
bool ReorderablePage::isLayoutReorderingCurrently() const
{
	return property(ReorderablePage::IsReorderingLayoutBoolPropertyName).toBool();
}
