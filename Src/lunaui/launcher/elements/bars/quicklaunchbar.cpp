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




#include "quicklaunchbar.h"
#include "dimensionsmain.h"
#include "dimensionslauncher.h"
#include "pixmaploader.h"
#include "gfxsettings.h"
#include "layoutsettings.h"
#include "dimensionsglobal.h"
#include "icon.h"
#include "iconheap.h"
#include "pixbutton2state.h"
#include "page.h"
#include "propertysettingsignaltransition.h"
#include "appmonitor.h"
#include "pagesaver.h"
#include "pagerestore.h"
#include "stringtranslator.h"
#include "appeffector.h"
#include "dynamicssettings.h"
#include "reorderablepage.h"
#include "operationalsettings.h"
#include "icongeometrysettings.h"

#include <QPainter>
#include <QString>
#include <QEvent>
#include <QGesture>
#include <QGestureEvent>
#include <QPropertyAnimation>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#include <QDebug>

#include "Settings.h"

#define QUICKLAUNCH_BG_SOLID	QString("/quicklaunch-bg-solid.png")
#define QUICKLAUNCH_BG_TRANSLUCENT QString("/quicklaunch-bg.png")

#define LA_BUTTON_FILEPATH QString("quicklaunch-button-launcher.png")
#define LA_BUTTON_NORMAL_LOC QRect(0,0,64,64)
#define LA_BUTTON_ACTIVE_LOC QRect(0,64,64,64)
#define MOVING_ICON_Y_OFFSET 15


qint32 QuickLaunchBar::sEventCounter0 = 0;


static PixButton2State * LoadLauncherAccessButton()
{
	QList<QRect> buttonStateCoords;
	buttonStateCoords << LA_BUTTON_NORMAL_LOC << LA_BUTTON_ACTIVE_LOC;
	QList<PixmapObject *> buttonStatePmos =
			PixmapObjectLoader::instance()->loadMulti(
					buttonStateCoords,
					GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + LA_BUTTON_FILEPATH);

	if (buttonStatePmos.length() > 1)
	{
		PixButton2State * pButton = new PixButton2State("",
														buttonStatePmos.at(0),buttonStatePmos.at(1));
		return pButton;
	}
	return 0;
}

QuickLaunchBar::QuickLaunchBar(const QRectF& geom,Quicklauncher * p_quicklauncherWindow)
: ThingPaintable(geom)
, m_p_touchFSM(0)
, m_fullInitPerformed(false)
, m_qp_ownerWindow(p_quicklauncherWindow)
, m_qp_backgroundTranslucent(0)
, m_qp_backgroundSolid(0)
, m_qp_currentBg(0)
, m_qp_launcherAccessButton(0)
, m_qp_iconInMotion(0)
, m_iconInMotionCurrentIndex(-1)
, m_iconShowingFeedback(0)
, m_feedbackTimer(this)
{

	m_qp_backgroundTranslucent =
			PixmapObjectLoader::instance()->quickLoad(
					QString(GraphicsSettings::settings()->graphicsAssetBaseDirectory + QUICKLAUNCH_BG_TRANSLUCENT)
			);
	m_qp_backgroundSolid =
				PixmapObjectLoader::instance()->quickLoad(
						QString(GraphicsSettings::settings()->graphicsAssetBaseDirectory + QUICKLAUNCH_BG_SOLID)
			);
	m_qp_currentBg = m_qp_backgroundTranslucent;

	m_qp_launcherAccessButton = LoadLauncherAccessButton();
	if (m_qp_launcherAccessButton)
	{
		m_qp_launcherAccessButton->setParentItem(this);
		m_qp_launcherAccessButton->setVisible(false);

		connect(m_qp_launcherAccessButton,SIGNAL(signalContact()),this,SIGNAL(signalToggleLauncher()));
	}
	setAcceptTouchEvents(true);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture(Qt::TapAndHoldGesture);
	grabGesture(Qt::TapGesture);

	connect(&m_feedbackTimer, SIGNAL(timeout()), this, SLOT(slotCancelLaunchFeedback()));

	setupTouchFSM();
	startTouchFSM();
}

//virtual
QuickLaunchBar::~QuickLaunchBar()
{
}

//virtual
bool QuickLaunchBar::canAcceptIcons()
{
	if ((quint32)(m_iconItems.size()) >= LayoutSettings::settings()->quickLaunchMaxItems)
	{
		return false;
	}

	if(m_qp_iconInMotion)
		return false;

	return true;
}

//virtual
bool QuickLaunchBar::offer(Thing * p_offer,Thing * p_offeringThing)
{
	IconBase * pIconOffer = qobject_cast<IconBase *>(p_offer);
	if (!pIconOffer)
	{
		return false;
	}

	bool arleadyPresent = false;
	IconBase* pMasterOffered = pIconOffer->master();
	IconBase* pMasterLocal = 0;

	for (QList<QPointer<IconBase> >::iterator it = m_iconItems.begin();	it != m_iconItems.end();++it) {
		IconBase * pIcon = *it;
		if(pIcon)
			pMasterLocal = pIcon->master();
		if(!pMasterLocal)
			continue;

		if(pMasterLocal == pMasterOffered) {// we already have a copy of this master icon on the QL
			arleadyPresent = true;
			break;
		}
	}

	if(arleadyPresent)
		return false;

	//else, initiate a take
	bool takeSuccess = pIconOffer->take(this);
	if (takeSuccess)
	{
		//it's my icon - at this point, the previous owner already released all interest in the icon, so I'm free to assimilate it!
		//TODO: special handling just in case it fails! hmmm...should probably send it back to where it came from immediately
		if(acceptIncomingIcon(pIconOffer)) {
			pIconOffer->setParentItem(this);
			pIconOffer->setPos(mapFromScene(pIconOffer->pos()));
			pIconOffer->setIconLabelVisibility(false);
			pIconOffer->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
			pIconOffer->slotChangeIconFrameVisibility(false);
			pIconOffer->clearAutopaintClipRect();
			pIconOffer->slotEnableIconAutoRepaint();
			pIconOffer->setVisible(true);
			pIconOffer->clearAutopaintClipRect();
			pIconOffer->slotEnableIconAutoRepaint();
		}
	}
	return takeSuccess;
}

//virtual
bool QuickLaunchBar::take(Thing * p_takerThing)
{
	return false;	//can't take me
}

//virtual
bool QuickLaunchBar::taking(Thing * p_victimThing, Thing * p_takerThing)
{
	return true;	//allow abductions
}

//virtual
void QuickLaunchBar::taken(Thing * p_takenThing,Thing * p_takerThing)
{
	IconBase * pTakenIcon = qobject_cast<IconBase *>(p_takenThing);
	if (!pTakenIcon)
	{
		return;	//don't know what this was...i only recognize icons
	}

	releaseTransferredIcon(pTakenIcon);
//	removeIcon(pTakenIcon, true);
}

//virtual
bool QuickLaunchBar::acceptIncomingIcon(IconBase * p_newIcon)
{
	bool res = true;
	if(m_qp_iconInMotion)
		return false;

	// Add to the begining of the list.
	//TODO: can we somehow pre-determine the correct location for this icon?
	m_qp_iconInMotion = p_newIcon;
	quint32 index = iconSlotForInsertingAtXCoord((quint32)(mapFromScene(p_newIcon->pos()).x()));
	res = addIcon(index,p_newIcon, true);
	if(res) {
		m_iconInMotionCurrentIndex = index;
	} else {
		m_qp_iconInMotion = 0;
	}

	return res;
}

//virtual
bool QuickLaunchBar::releaseTransferredIcon(IconBase * p_transferredIcon)
{
	removeIcon(p_transferredIcon, true);
	return true;
}

//virtual
void QuickLaunchBar::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	if (m_qp_currentBg)
	{
		paintBackground(painter);
	}
}

//virtual
void QuickLaunchBar::paintOffscreen(QPainter *painter)
{
	//TODO: IMPLEMENT
}

inline void QuickLaunchBar::paintBackground(QPainter * painter)
{
	//not sure if this is necessary (to save, i mean...the origin call IS necessary)
	QPoint sbo = painter->brushOrigin();
	painter->setBrushOrigin(m_geom.topLeft());
	painter->fillRect(m_geom, QBrush(*(*m_qp_currentBg)));
	painter->setBrushOrigin(sbo);
}

//virtual
bool QuickLaunchBar::fullInit(quint32 screenWidth,quint32 screenHeight)
{
	if (m_fullInitPerformed)
	{
		return true;
	}
	//determine the size of this ql bar
	QSize s = QuickLaunchSizeFromScreenSize(screenWidth,screenHeight);
	if (!resize(s))
	{
		return false;	//invalid size
	}
	m_fullInitPerformed = true;

	//Now, the QL saved items should be loaded here....BUT...they require the LauncherObject (the launcher itself) to be operational
	//	because the icon<->app mappings are there - it would be silly and error prone to replicate them since at no time will there
	// EVER be a QL without a launcher - and the loading code will need this mapping info to get the right icons
	///So...just return here and have the launcher call "restore" when it restores its own pages

	return true;
}

//virtual
bool QuickLaunchBar::resize(const QSize& s)
{
	if ((s.width() == 0) || (s.height() == 0))
	{
		return false; //invalid
	}
	ThingPaintable::resize(s);	//this will take care of geom and b-rect computation

	//reposition the launcher access button
	//TODO: hardcoded to reference topRight of QL
	if (m_qp_launcherAccessButton)
	{
		// Changing this for now so that we treat the launcher icon as having the same size of a regular icon, so we can have
		// everything ordered symmetrically in the QL bar.
		m_qp_launcherAccessButton->setPos(
				m_geom.topRight()
				+QPoint(-IconGeometrySettings::settings()->absoluteGeomSizePx.width()/2, LayoutSettings::settings()->quickLaunchBarLauncherAccessButtonOffsetPx.y() + m_qp_launcherAccessButton->geometry().height()/2));

		m_qp_launcherAccessButton->setVisible(true);
	}

	//item area recalc...item area starts at either:
	// 1. geom.topLeft   (if the access button is on the right edge)
	// 2. Point(button.right,geom.top)     (if the access button is on the left edge)
	//  + settings.quickLaunchItemAreaOffsetPx

	//TODO: for now, access button is hardcoded to the right edge, so it's always (1)

	m_itemAreaXrange.first = (qint32)(geometry().left()) + LayoutSettings::settings()->quickLaunchItemAreaOffsetPx.x();
	m_itemAreaXrange.second = (qint32)(geometry().right())
								-(qint32)(IconGeometrySettings::settings()->absoluteGeomSizePx.width());

	//the settings spec is relative to the top but m_itemsY will be used as a coordinate in ICS, so remap it so
	// that it's center-origin based (i.e. it's in ICS)
	m_itemsY = m_geom.top()+LayoutSettings::settings()->quickLaunchItemAreaOffsetPx.y();

	//recompute the position of all the items currently here
	rearrangeIcons(false);
	return true;
}

//virtual
QList<QPointer<IconBase> > QuickLaunchBar::iconsInFlowOrder() const		//left to right , in order of appearance
{
	return m_iconItems;
}

typedef QList<QUuid> AppUidList;
typedef QList<DimensionsSystemInterface::WebOSAppRestoreObject> WebOSRestoreObjectList;

//virtual
bool QuickLaunchBar::restoreFromSave()
{
	if (!DimensionsSystemInterface::AppMonitor::appMonitor()->completedScan())
	{
		return false;
	}

	if (!m_iconItems.isEmpty())
	{
		return false;
	}
	bool legacyConverted = false;
	//scan for masterfiles
	QList<QString> qlfiles = DimensionsSystemInterface::PageRestore::scanForSavedQuicklaunchFiles();
	if (qlfiles.isEmpty())
	{
		//no qlfiles found...nothing to restore...
		//try and create one from legacy confs
		QString legacyConfFilepath;

		if (!OperationalSettings::settings()->ignoreLegacyCustomizationFiles)
		{
			legacyConfFilepath = StringTranslator::inputString(Settings::LunaSettings()->quicklaunchCustomPositions);
		}
		if (!DimensionsSystemInterface::PageRestore::convertLegacyJsonQuicklaunchConfig(legacyConfFilepath))
		{
			legacyConfFilepath = StringTranslator::inputString(Settings::LunaSettings()->quicklaunchDefaultPositions);
			if (!DimensionsSystemInterface::PageRestore::convertLegacyJsonQuicklaunchConfig(legacyConfFilepath))
			{
				//hopeless
				return false;
			}
		}
		//rescan
		qlfiles = DimensionsSystemInterface::PageRestore::scanForSavedQuicklaunchFiles();
		if (qlfiles.isEmpty())
		{
			//lies, and hopeless!
			return false;
		}
		legacyConverted = true;
	}

	//select the most recently used quicklaunch save file

	QString MRUqlfile = DimensionsSystemInterface::PageRestore::selectQuicklaunchFile
			(qlfiles,DimensionsSystemInterface::QuickLaunchRestoreSaveFileSelector::MostRecent);
	if (MRUqlfile.isEmpty() && !legacyConverted)
	{
		//try creating from legacy confs
		QString legacyConfFilepath;
		if (!OperationalSettings::settings()->ignoreLegacyCustomizationFiles)
		{
			legacyConfFilepath = StringTranslator::inputString(Settings::LunaSettings()->quicklaunchCustomPositions);
		}
		if (!DimensionsSystemInterface::PageRestore::convertLegacyJsonQuicklaunchConfig(legacyConfFilepath))
		{
			legacyConfFilepath = StringTranslator::inputString(Settings::LunaSettings()->quicklaunchDefaultPositions);
			if (!DimensionsSystemInterface::PageRestore::convertLegacyJsonQuicklaunchConfig(legacyConfFilepath))
			{
				//hopeless
				return false;
			}
		}
		//rescan
		qlfiles = DimensionsSystemInterface::PageRestore::scanForSavedQuicklaunchFiles();
		if (qlfiles.isEmpty())
		{
			//lies, and hopeless!
			return false;
		}
		MRUqlfile = DimensionsSystemInterface::PageRestore::selectQuicklaunchFile
				(qlfiles,DimensionsSystemInterface::QuickLaunchRestoreSaveFileSelector::MostRecent);
		if (MRUqlfile.isEmpty())
		{
			//hopelessly hopeless!
			return false;
		}
	}
	//attempt to retrieve quicklauncher saved state via this file
	QVariantMap quicklauncherConfiguration =  DimensionsSystemInterface::PageRestore::restoreQuickLaunch(MRUqlfile);
	if (quicklauncherConfiguration.isEmpty())
	{
		//nothing could be restored
		return false;
	}

	WebOSRestoreObjectList restoredAppsList = (quicklauncherConfiguration.value(DimensionsSystemInterface::PageSaver::SaveTagKey_PageRestoreObjectList)).value<WebOSRestoreObjectList>();
	//restore only up to the max allowable icons
	int restoredCount = 0;
	for (WebOSRestoreObjectList::const_iterator app_it = restoredAppsList.constBegin();
			((app_it != restoredAppsList.constEnd()) && ((quint32)restoredCount < LayoutSettings::settings()->quickLaunchMaxItems));++app_it)
	{
		//This is the quicklaunch, which means that the icons here must be CLONES. So each one needs to be cloned before being added here
		DimensionsSystemInterface::WebOSAppRestoreObject const& pRestoreObj = *app_it;
		IconBase * pIcon = 0;
		if ((pRestoreObj.launchpointId == pRestoreObj.appId) || (pRestoreObj.launchpointId.isEmpty()))
		{
			//this was a main icon
			QUuid iconUid = LauncherObject::primaryInstance()->iconUidFromAppUid(pRestoreObj.appUid);
			if (iconUid.isNull())
			{
				//skip it; this launcher has decided it didn't like that app or it is no longer installed
				continue;
			}
			pIcon = IconHeap::iconHeap()->getIcon(iconUid);
		}
		else
		{
			pIcon = IconHeap::iconHeap()->getIcon(pRestoreObj.appId,pRestoreObj.launchpointId);
		}
		if (!pIcon)
		{
			//doesn't exist????
			continue;
		}
		//copy it
		IconBase * pIconCopy = IconHeap::iconHeap()->copyIcon(pIcon->master()->uid());
		if(pIconCopy) {
			//and append it
			pIconCopy->setIconLabelVisibility(false);
			addIcon(INT_MAX,pIconCopy, false);
			pIconCopy->setParentItem(this);
			pIconCopy->slotEnableIconAutoRepaint();
			pIconCopy->setVisible(true);
		}
	}
	rearrangeIcons(false);
	return true;
}

bool QuickLaunchBar::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {

		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g) {
			QTapGesture* tap = static_cast<QTapGesture*>(g);
			if (tap->state() == Qt::GestureFinished) {
				return tapGesture(tap,ge);
			}
		}
		g = ge->gesture(Qt::TapAndHoldGesture);
		if (g) {
			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
			if (hold->state() == Qt::GestureFinished) {
				return tapAndHoldGesture(hold,ge);
			};
		}
		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g) {
			FlickGesture* flick = static_cast<FlickGesture*>(g);
			if (flick->state() == Qt::GestureFinished) {
				return flickGesture(flick,ge);
			}
		}
	}
	else if (event->type() == QEvent::TouchBegin)
	{
		return touchStartEvent(static_cast<QTouchEvent *>(event));
	}
	else if (event->type() == QEvent::TouchUpdate)
	{
		return touchUpdateEvent(static_cast<QTouchEvent *>(event));
	}
	else if (event->type() == QEvent::TouchEnd)
	{
		return touchEndEvent(static_cast<QTouchEvent *>(event));
	}

	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool QuickLaunchBar::flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent)
{

	Q_EMIT signalFlickAction();
	return true;
}

//virtual
bool QuickLaunchBar::tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent)
{
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

	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapHoldEvent->hotSpot()));

	//first, it there is an icon already in motion, silently consume event (yum!)
	if (m_qp_iconInMotion)
	{
		return true;
	}

	//figure out which (if any) icon is being acted on
	IconBase* pIcon = iconAtCoordinate(positionOfTapICS);
	if (pIcon == NULL)
	{
		//no icon there
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
	m_iconInMotionCurrentIndex = getCurrentIconListIndex(m_qp_iconInMotion);
	pIcon->update();
	return true;
}
//virtual
bool QuickLaunchBar::tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent)
{
	//ignore event if we are currently moving an item around
	if (m_qp_iconInMotion)
	{
		return true;
	}

	QPointF positionOfTapICS = mapFromScene(baseGestureEvent->mapToGraphicsScene(tapEvent->hotSpot()));

	// determine if we hit any of the icons on teh quick launch bar
	IconBase* pIcon = iconAtCoordinate(positionOfTapICS);
	if (pIcon == NULL)
	{
		//no icon there
		return true;
	}

	QPointF intraIconPositionOfTapICS = pIcon->geometry().center();
	//check to see if the icon should handle the event internally
	IconInternalHitAreas::Enum hitArea;
	if (pIcon->tapIntoIcon(intraIconPositionOfTapICS,hitArea))
	{
		return true;
	}

	iconActivatedTap(pIcon->uid());

	setAppLaunchFeedback(pIcon);

	return true;
}

//static
QSize QuickLaunchBar::QuickLaunchSizeFromScreenSize(int screenWidth,int screenHeight)
{
	if (LayoutSettings::settings()->quickLaunchBarUseAbsoluteSize)
	{
		return QSize(screenWidth,qMin(LayoutSettings::settings()->quickLaunchBarHeightAbsolute,(quint32)screenHeight));
	}
	QSize r = QSize(
			qBound((quint32)2,
					(quint32)DimensionsGlobal::roundDown((qreal)screenWidth * LayoutSettings::settings()->quickLaunchBarSizePctScreenRelative.width()),
					(quint32)screenWidth),
			qBound((quint32)2,
					(quint32)DimensionsGlobal::roundDown((qreal)screenHeight * LayoutSettings::settings()->quickLaunchBarSizePctScreenRelative.height()),
					(quint32)screenHeight)
	);

	//make evenly divisible (multiple of 2)
	r.setWidth(r.width() - (r.width() % 2));
	r.setHeight(r.height() - (r.height() % 2));
	return r;

}

//virtual
void QuickLaunchBar::rearrangeIcons(bool animate)
{
	qint32 itemSpace = 0;
	qint32 nItems = 0;
	//calc the total size taken up by the items in the list
	for (QList<QPointer<IconBase> >::iterator it = m_iconItems.begin();
			it != m_iconItems.end();++it)
	{
		IconBase * pIcon = *it;
		if (!pIcon)
		{
			continue;
		}
		++nItems;
		itemSpace += (qint32)pIcon->geometry().width();
	}

	if(!nItems)
		return;

	if(!m_qp_reorderAnimationGroup.isNull()) {
		if(m_qp_reorderAnimationGroup->state() != QAbstractAnimation::Stopped)
			m_qp_reorderAnimationGroup->stop();

		delete m_qp_reorderAnimationGroup;
	}

	if(animate) {
		m_qp_reorderAnimationGroup = new QParallelAnimationGroup(this);
	}

	qint32 interSpace = qMax((int)0,(int)(((m_itemAreaXrange.second - m_itemAreaXrange.first)-itemSpace)/(nItems)));

	m_layoutAnchorsXcoords.clear();
	if (itemSpace == 0)
	{
		//nothing to do
		return;
	}

	qint32 xoffs = 0;
	int idx=0;
	QPointF iconPos;
	iconPos.setY(m_itemsY);

	for (QList<QPointer<IconBase> >::iterator it = m_iconItems.begin();
			it != m_iconItems.end();++it)
	{
		IconBase * pIcon = *it;
		if (!pIcon)
		{
			continue;
		}
		qint32 iconHalfW = (qint32)(pIcon->geometry().width()/2.0);

		if(pIcon != m_qp_iconInMotion) {// no need to move the icon that the user is dragging around
			iconPos.setX(m_itemAreaXrange.first+xoffs+iconHalfW);

			if(pIcon->pos() != iconPos) {
				if(!animate) {
					pIcon->setPos(iconPos);
				} else {
					// animate the icon to its new position
					QPropertyAnimation* anim = new QPropertyAnimation(pIcon, "pos");
					anim->setEndValue(iconPos);
					anim->setDuration(DynamicsSettings::settings()->iconReorderIconMoveAnimTime);
					anim->setEasingCurve(DynamicsSettings::settings()->iconReorderIconMoveAnimCurve);
					m_qp_reorderAnimationGroup->addAnimation(anim);
				}
			}
		}

		m_layoutAnchorsXcoords << m_itemAreaXrange.first+xoffs+iconHalfW;
		xoffs+=interSpace+pIcon->geometry().width();
	}

	if(animate && !m_qp_reorderAnimationGroup.isNull() && m_qp_reorderAnimationGroup->animationCount()) {
		m_qp_reorderAnimationGroup->start(QAbstractAnimation::DeleteWhenStopped);
	}

	update();
}

// virtual
qint32 QuickLaunchBar::getCurrentIconListIndex(IconBase * p_icon)
{
	if (!p_icon)
	{
		//invalid parameter
		return -1;
	}

	bool found=false;
	qint32 index = 0;

	for (QList<QPointer<IconBase> >::iterator it = m_iconItems.begin();
			it != m_iconItems.end();++it)
	{
		if (p_icon == (*it).data())
		{
			//found it.
			found=true;
			break;
		}
		index++;
	}
	if (found)
	{
		return index;
	} else {
		return -1;
	}
}


//virtual
bool QuickLaunchBar::addIcon(quint32 index,IconBase * p_icon, bool animate)
{
	if (!p_icon)
	{
		return false;
	}

	if ((quint32)(m_iconItems.size()) >= LayoutSettings::settings()->quickLaunchMaxItems)
	{
		g_warning("%s: early-exit: max items (%d) reached",__FUNCTION__,LayoutSettings::settings()->quickLaunchMaxItems);
		return false;
	}

	if (index >= (quint32)(m_iconItems.size()))
	{
		m_iconItems.append(QPointer<IconBase>(p_icon));
	}
	else
	{
		m_iconItems.insert(index,QPointer<IconBase>(p_icon));
	}
	if (!connect(p_icon,SIGNAL(destroyed(QObject *)),
			this,SLOT(slotIconDeleted(QObject *))))
	{
		g_warning("%s: connect to kill signal on icon uid [%s] didn't work\n",__FUNCTION__,qPrintable(p_icon->uid().toString()));
	}
	rearrangeIcons(animate);
	return true;
}

//virtual
bool QuickLaunchBar::moveIcon(quint32 oldIndex, quint32 newIndex ,IconBase * p_icon, bool animate)
{
	if (!p_icon) {
		return false;
	}

	if((oldIndex >= (quint32)(m_iconItems.size())) || (newIndex >= (quint32)(m_iconItems.size()))) {
		return false;
	}

	m_iconItems.move(oldIndex, newIndex);

	rearrangeIcons(animate);
	return true;
}

//virtual
void QuickLaunchBar::slotIconDeleted(QObject * p)
{
	//An icon got deleted...scan the list and anything that is now null should be removed
	// NOTE: this depends on QPointer getting signalled first, which in general happens since it hooked the signal first.
	// however, this is a hard assumption and could be brittle.
	// TODO: make the actual mechanism for deleting an icon more explicit...e.g. make it trigger off of slotPreRemove or similar in launcher object

	uint count=0;
	QList<QPointer<IconBase> >::iterator it = m_iconItems.begin();
	while (it != m_iconItems.end())
	{
		if ((*it).isNull())
		{
			//found it.
			it = m_iconItems.erase(it);
			++count;
		}
		else
		{
			++it;
		}
	}

	g_warning("%s: FOUND %d delete",__FUNCTION__,count);
	if (count)
	{
		rearrangeIcons(true);
	}
}

//virtual
void QuickLaunchBar::removeIcon(IconBase * p_icon, bool animate)
{
//	The idea here is that this will be called as a part of the user pulling the icon off of the quicklaunch bar
//	As such, at that point I'll only have the ptr to the icon stored away in the tracking var. Thus, I need a
//	way to remove from the list via the ptr. This is done by this fn.

	if (!p_icon)
	{
		return;
	}

	QUuid fUid = p_icon->uid();
	QList<QPointer<IconBase> >::iterator f  = m_iconItems.begin();
	for (;f != m_iconItems.end();++f)
	{
		if (!(*f))
		{
			continue;
		}
		if ((*f)->uid() == fUid)
		{
			break;
		}
	}
	if (f != m_iconItems.end())
	{
		m_iconItems.erase(f);
	}

	rearrangeIcons(animate);
}

//virtual
void QuickLaunchBar::deleteIcon(IconBase * p_icon, bool animate)
{
	if(!p_icon)
		return;

	removeIcon(p_icon, animate);
	p_icon->setVisible(false);
	p_icon->setParentItem(NULL);
	IconHeap::iconHeap()->deleteIconCopy(p_icon->uid());
}

//virtual
IconBase * QuickLaunchBar::getIconPtr(const QUuid& iconUid,bool paramIsMasterUid)
{
	QUuid fUid;
	if (!paramIsMasterUid)
	{
		IconBase * pIconCopy = IconHeap::iconHeap()->getIcon(iconUid);
		if (!pIconCopy)
		{
			return 0;
		}
		fUid = pIconCopy->master()->uid();
	}
	else
	{
		fUid = iconUid;
	}
	QList<QPointer<IconBase> >::iterator f  = m_iconItems.begin();
	for (;f != m_iconItems.end();++f)
	{
		if (!(*f))
		{
			continue;
		}
		if ((*f)->master()->uid() == fUid)
		{
			break;
		}
	}
	if (f != m_iconItems.end())
	{
		return *f;
	}
	return 0;
}

//// ----- Touch and stuff

//virtual
bool QuickLaunchBar::okToUseFlick(int id)
{
	return testAndSetTriggerOnRegisterCreate(id,TouchTriggerType::INVALID,TouchTriggerType::Flick);
}

//virtual
bool QuickLaunchBar::okToUseTap(int id)
{
	return testAndSetTriggerOnRegisterCreate(id,TouchTriggerType::INVALID,TouchTriggerType::Tap);
}

//virtual
bool QuickLaunchBar::okToUseTapAndHold(int id)
{
	return testAndSetTriggerOnRegisterCreate(id,TouchTriggerType::INVALID,TouchTriggerType::TapAndHold);
}

//statics, see .h file
const char * QuickLaunchBar::TouchFSMPropertyName_isTracking = "isTracking";
const char * QuickLaunchBar::TouchFSMProperyName_trackedId = "trackedId";

//virtual
bool QuickLaunchBar::anyTouchTracking(int * r_p_mainTouchId)
{
	if (m_p_touchFSM->property(TouchFSMPropertyName_isTracking).toBool())
	{
		if (r_p_mainTouchId)
		{
			*r_p_mainTouchId = m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt();
		}
		return true;
	}
	return false;
}

//virtual
bool QuickLaunchBar::touchPointTriggerType(int id,TouchTriggerType::Enum& r_type)
{
	QMap<int,TouchRegister>::const_iterator f = m_touchRegisters.constFind(id);
	if (f != m_touchRegisters.constEnd())
	{
		if (f.value().valid)
		{
			r_type = f.value().triggerType;
			return true;
		}
	}
	return false;
}

//virtual
bool QuickLaunchBar::testAndSetTriggerOnRegister(int id,TouchTriggerType::Enum conditionV,TouchTriggerType::Enum setV)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if ((f.value().valid) && (f.value().triggerType == conditionV))
		{
			f.value().triggerType = setV;
			return true;
		}
	}
	return false;
}

//virtual
bool QuickLaunchBar::testAndSetTriggerOnRegisterCreate(int id,TouchTriggerType::Enum conditionV,TouchTriggerType::Enum setV)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if ((f.value().valid) && ((f.value().touchId != id) || (f.value().triggerType != conditionV)))
		{
			return false;
		}
	}
	m_touchRegisters[id] = TouchRegister(id,setV);
	return true;
}

//virtual
RedirectingType::Enum QuickLaunchBar::isRedirecting(int id,Thing ** r_pp_redirectTargetThing,RedirectContext ** r_pp_redirContext)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if ((f.value().valid) && (f.value().redirecting))
		{
			if (r_pp_redirectTargetThing)
			{
				*r_pp_redirectTargetThing = f.value().qpRedirectTarget;
			}
			if (r_pp_redirContext)
			{
				*r_pp_redirContext = f.value().pRedirectContext;
			}
			if (f.value().qpRedirectTarget)
			{
				return RedirectingType::Redirect;
			}
			else
			{
				return RedirectingType::RedirectTargetMissing;
			}
		}
	}
	return RedirectingType::NoRedirect;
}

//virtual
bool QuickLaunchBar::redirectTo(int id,Thing * p_redirectTargetThing,RedirectContext * p_redirContext)
{
	//if already redirecting somewhere else, then fail.
	//TODO: possibly a way to allow these situations too; it would probably need a way to alert the previous redirectee that
	//		no more redirects are coming
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if (( !f.value().valid) || ((f.value().redirecting)
				&& (f.value().qpRedirectTarget) && (f.value().qpRedirectTarget != p_redirectTargetThing)))
		{
			//already redirecting elsewhere, or an invalid register
			return false;
		}
		f.value().redirecting = true;
		f.value().qpRedirectTarget = p_redirectTargetThing;
		if (f.value().pRedirectContext)
		{
			delete f.value().pRedirectContext;
		}
		f.value().pRedirectContext = p_redirContext;
		return true;
	}
	return false;	//not found
}

///TODO: unsafe...make it more so.
//virtual
bool QuickLaunchBar::changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext)
{

	if (this == p_newRedirectedToThing)
	{
		//self-redirection is a cancel
		qDebug() << "QuickLaunchBar::" <<__FUNCTION__ << "--------- CANCELLED REDIRECTIONS!!!";
		return cancelRedirection(id);
	}

	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		if ( !f.value().valid)
		{
			//invalid register
			return false;
		}
		f.value().redirecting = true;
		f.value().qpRedirectTarget = p_newRedirectedToThing;
		if (f.value().pRedirectContext)
		{
			delete f.value().pRedirectContext;
		}
		f.value().pRedirectContext = p_redirContext;
		return true;
	}
	return false;	//not found
}

IconBase* QuickLaunchBar::iconAtCoordinate(const QPointF& coord)
{
	int index = 0;
	// determine if the provided coordinates falls into the bounds of any of the icons in the m_iconItems list
	for (IconListIter it = m_iconItems.begin(); it != m_iconItems.end(); ++it)
	{
		qint32 slotX = m_layoutAnchorsXcoords.value(index);
		QRectF target = (*it)->geometry().translated(QPoint(slotX, m_itemsY));

		if (target.contains(coord))
		{
			// return the icon under coord
			return *it;
		}
		index++;
	}
	// no icon found
	return 0;
}

//virtual
qint32 QuickLaunchBar::iconSlotForInsertingAtXCoord(qint32 x)
{
	if ((quint32)(m_iconItems.size()) >= LayoutSettings::settings()->quickLaunchMaxItems)
		return -1;

	// returns the most likely slot a new icon will end up when inserted at the provided X coord
	if(x <= m_itemAreaXrange.first)
		return 0;
	if(x >= m_itemAreaXrange.second)
		return m_iconItems.size();

	quint32 nItems = m_iconItems.size() + 1;
	quint32 slotSize = (m_itemAreaXrange.second - m_itemAreaXrange.first) / nItems;

	quint32 index = (x - m_itemAreaXrange.first) / slotSize;

	return index;
}

//helper
static QState* createState(QString name, QState *parent=0)
{
	QState *result = new QState(parent);
	result->setObjectName(name);
	return result;
}

void QuickLaunchBar::resetEventCounters()
{
	sEventCounter0 = 0;
}

//virtual
void QuickLaunchBar::setupTouchFSM()
{

	if (m_p_touchFSM)
	{
		qDebug() << __FUNCTION__ << ": attempting to setup a new touch FSM when one already exists! Bad! ignoring...";
		return;
	}
	m_p_touchFSM = new QStateMachine(this);
	m_p_touchFSM->setObjectName("touchfsm");

	m_p_fsmStateNoTouch           		= createState("touchfsmstate_notouch");
	m_p_fsmStateTouchMotionTracking     = createState("touchfsmstate_motion");
	m_p_fsmStateTouchStationaryTracking = createState("touchfsmstate_stationary");

	// ------------------- STATE: touchfsmstate_notouch -----------------------------------
	//	touchfsmstate_notouch PROPERTIES
	m_p_fsmStateNoTouch->assignProperty(m_p_touchFSM,TouchFSMPropertyName_isTracking, false);
	m_p_fsmStateNoTouch->assignProperty(m_p_touchFSM,TouchFSMProperyName_trackedId, -1);

	//  touchfsmstate_notouch TRANSITIONS
	PropertySettingSignalTransition * pTransition =
			new PropertySettingSignalTransition(this,SIGNAL(signalTouchFSMTouchBegin_Trigger(QVariant)),
												m_p_touchFSM,QString(TouchFSMProperyName_trackedId));
	pTransition->setTargetState(m_p_fsmStateTouchMotionTracking);
	m_p_fsmStateNoTouch->addTransition(pTransition);

	// ------------------- STATE: touchfsmstate_motion ------------------------------------
	// touchfsmstate_motion PROPERTIES
	m_p_fsmStateTouchMotionTracking->assignProperty(m_p_touchFSM,TouchFSMPropertyName_isTracking, true);

	// touchfsmstate_motion TRANSITIONS
	m_p_fsmStateTouchMotionTracking->addTransition(this, SIGNAL(signalTouchFSMTouchEnd_Trigger()),m_p_fsmStateNoTouch);

	m_p_touchFSM->addState(m_p_fsmStateNoTouch);
	m_p_touchFSM->addState(m_p_fsmStateTouchMotionTracking);
	m_p_touchFSM->addState(m_p_fsmStateTouchStationaryTracking);

	m_p_touchFSM->setInitialState(m_p_fsmStateNoTouch);
}

//virtual
void QuickLaunchBar::startTouchFSM()
{
	if (!m_p_touchFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot start; FSM does not exist";
		return;
	}
	if (m_p_touchFSM->isRunning())
	{
		m_p_touchFSM->stop();
	}
	m_p_touchFSM->setInitialState(m_p_fsmStateNoTouch);
	m_p_touchFSM->start();
}

//virtual
void QuickLaunchBar::stopTouchFSM()
{
	if (!m_p_touchFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot stop; FSM does not exist";
		return;
	}
	m_p_touchFSM->stop();
}

//virtual
bool QuickLaunchBar::touchStartEvent(QTouchEvent *event)
{
	event->accept();
//	qDebug() << __FUNCTION__ << " : points: " << event->touchPoints();
	if (event->touchPoints().isEmpty())
	{
		return true;
	}

	//TODO: MULTI-TOUCH:
	//	assume that multiple touch starts can't happen for a single id
	//
	int id = event->touchPoints().first().id();
	m_touchRegisters[id] = TouchRegister(id,TouchTriggerType::INVALID);	//INVALID marks the register as not having a specific gesture yet, not that the reg itself is invalid
	Q_EMIT signalTouchFSMTouchBegin_Trigger(QVariant());
	return true;
}

//virtual
bool QuickLaunchBar::touchUpdateEvent(QTouchEvent *event)
{
	event->accept();

	if (m_p_touchFSM->property(TouchFSMPropertyName_isTracking).toBool() == false)
	{
		//fsm says not tracking, so I don't recognize this... swallow it
		return true;
	}

	//TODO: MULTI-TOUCH
	//dig out the touch point which is being tracked
	QList<QTouchEvent::TouchPoint> points = event->touchPoints();
	for (QList<QTouchEvent::TouchPoint>::iterator it = points.begin();
			it != points.end();++it)
	{
		if (it->id() == m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt())
		{
			//found it...
			//if it is being redirected, then keep redirecting it, but if the target got destroyed, then just silently swallow the events
			Thing * pRedirectTarget = 0;
			RedirectContext * pRedirContext = 0;
			RedirectingType::Enum redirType = isRedirecting(it->id(),&pRedirectTarget,&pRedirContext);
			if (redirType == RedirectingType::Redirect)
			{
				if (pRedirContext)
				{
					pRedirectTarget->redirectedTouchTrackedPointMoved(this,it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos(),*pRedirContext);
				}
				else
				{
					RedirectContext nullContext;
					//bypass the qemuXXX toolchain issue
					pRedirectTarget->redirectedTouchTrackedPointMoved(this,it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos(),nullContext);
				}
			}
			else if (redirType == RedirectingType::NoRedirect)
			{
				touchTrackedPointMoved(it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos());
			}
			//else the target was destroyed..just silently consume
			break;
		}
	}
	return true;
}

//virtual
bool QuickLaunchBar::touchEndEvent(QTouchEvent *event)
{
//	qDebug() << __PRETTY_FUNCTION__;
	event->accept();
	//TODO: MULTI-TOUCH
	//dig out the touch point which is being tracked
	QList<QTouchEvent::TouchPoint> points = event->touchPoints();
	bool handled = false;
	int id = -1;
	RedirectContext * pRedirContext = 0;
	for (QList<QTouchEvent::TouchPoint>::iterator it = points.begin();
			it != points.end();++it)
	{
		if (it->id() == m_p_touchFSM->property(TouchFSMProperyName_trackedId).toInt())
		{
			//found it...
			//if it is being redirected, then keep redirecting it, but if the target got destroyed, then just silently swallow the events
			Thing * pRedirectTarget = 0;
			RedirectingType::Enum redirType = isRedirecting(it->id(),&pRedirectTarget,&pRedirContext);
			if (redirType == RedirectingType::Redirect)
			{
				if (pRedirContext)
				{
					pRedirectTarget->redirectedTouchTrackedPointReleased(this,it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos(),*pRedirContext);
				}
				else
				{
					RedirectContext nullContext;
					pRedirectTarget->redirectedTouchTrackedPointReleased(this,it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos(),nullContext);
				}
			}
			else if (redirType == RedirectingType::NoRedirect)
			{
				touchTrackedPointReleased(it->id(),it->scenePos(),it->lastScenePos(),it->startScenePos());
			}
			//else the target was destroyed, just consume the event

			//mark it so that the outer, final code does what it needs to do
			handled = true;
			id = it->id();
			break;
		}
	}
	Q_EMIT signalTouchFSMTouchEnd_Trigger();
	//remove the register entry
	if (handled)
	{
		if (pRedirContext)
		{
			delete pRedirContext;
		}
		m_touchRegisters.remove(id);
	}
	return true;
}

///virtual
void QuickLaunchBar::touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{
//	qDebug() << __FUNCTION__ << ": id " << id;
	//if an icon is NOT being moved, then just ignore the event
	if (!m_qp_iconInMotion)
	{
		return;
	}

	QPointF barCoordinate = mapFromScene(scenePosition);

	m_qp_iconInMotion->setPos(barCoordinate - QPointF(0, MOVING_ICON_Y_OFFSET));

	if ((quint32)(++sEventCounter0) >= DynamicsSettings::settings()->iconReorderSampleRate)
	{
		//time to sample against the layout to see if moves need to be generated

		RedirectContext nullCtx;					
		if (detectAndHandleSpecialMoveAreas(id,barCoordinate,nullCtx))
		{
			//there was a special move generated...do not reset the event counter, just return;
			return;
		}

		IconBase* pIcon = iconAtCoordinate(barCoordinate);
		qint32 newIndex = getCurrentIconListIndex(pIcon);
		if (pIcon != NULL && newIndex >= 0 && pIcon != m_qp_iconInMotion && newIndex != m_iconInMotionCurrentIndex)
		{
			m_qp_iconInMotion->setOpacity(1.0);
			if(moveIcon(m_iconInMotionCurrentIndex, newIndex, pIcon, true)) {
				m_iconInMotionCurrentIndex = newIndex;
			}
		}
		else {
			if(barCoordinate.y() <= geometry().y()) {
				m_qp_iconInMotion->setOpacity(0.5);
			} else {
				m_qp_iconInMotion->setOpacity(1.0);
			}
		}
		//reset the event counter used
		sEventCounter0 = 0;
	}
}

//virtual
void QuickLaunchBar::redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	//qDebug() << __FUNCTION__ << ": id " << id;
	//if an icon is NOT being moved, then just ignore the event
	if (!m_qp_iconInMotion)
	{
		return;
	}

	QPointF barCoordinate = mapFromScene(scenePosition);

	m_qp_iconInMotion->setPos(barCoordinate - QPointF(0, MOVING_ICON_Y_OFFSET));

	if ((quint32)(++sEventCounter0) >= DynamicsSettings::settings()->iconReorderSampleRate)
	{
		//time to sample against the layout to see if moves need to be generated

		if (detectAndHandleSpecialMoveAreas(id,barCoordinate, redirContext))
		{
			//there was a special move generated...do not reset the event counter, just return;
			return;
		}

		IconBase* pIcon = iconAtCoordinate(barCoordinate);
		qint32 newIndex = getCurrentIconListIndex(pIcon);
		if (pIcon != NULL && newIndex >= 0 && pIcon != m_qp_iconInMotion && newIndex != m_iconInMotionCurrentIndex)
		{
			m_qp_iconInMotion->setOpacity(1.0);
			if(moveIcon(m_iconInMotionCurrentIndex, newIndex, pIcon, true)) {
				m_iconInMotionCurrentIndex = newIndex;
			}
		}
		else {
			if(barCoordinate.y() <= geometry().y()) {
				m_qp_iconInMotion->setOpacity(0.5);
			} else {
				m_qp_iconInMotion->setOpacity(1.0);
			}
		}
		//reset the event counter used
		sEventCounter0 = 0;
	}
}

//virtual
void QuickLaunchBar::touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{
//	qDebug() << __FUNCTION__ << ": id " << id;

	//if an icon is NOT being moved, then just ignore the event
	if (!m_qp_iconInMotion)
	{
		return;
	}

	QPointF barCoordinate = mapFromScene(scenePosition);

	m_qp_iconInMotion->setPos(barCoordinate - QPointF(0, MOVING_ICON_Y_OFFSET));

	IconBase* pIcon = iconAtCoordinate(barCoordinate);
	qint32 newIndex = getCurrentIconListIndex(pIcon);

	IconBase* pMovingIcon = m_qp_iconInMotion;
	m_qp_iconInMotion = NULL;
	qint32 movingIndex = m_iconInMotionCurrentIndex;
	m_iconInMotionCurrentIndex = -1;

	if (pIcon != NULL && newIndex >= 0 && pIcon != m_qp_iconInMotion && newIndex != movingIndex)
	{
		if(moveIcon(m_iconInMotionCurrentIndex, newIndex, pIcon, true)) {
			m_iconInMotionCurrentIndex = newIndex;
		}
	} else {
		if(barCoordinate.y() <= geometry().y()) {
			deleteIcon(pMovingIcon, true);
		} else {
			rearrangeIcons(true);
		}
	}

	DimensionsSystemInterface::PageSaver::saveQuickLaunch(LauncherObject::primaryInstance(), this);
}

//virtual
void QuickLaunchBar::redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
//	qDebug() << __FUNCTION__ << ": id " << id;
	//if an icon is NOT being moved, then just ignore the event
	if (!m_qp_iconInMotion)
	{
		return;
	}

	QPointF barCoordinate = mapFromScene(scenePosition);

	m_qp_iconInMotion->setPos(barCoordinate - QPointF(0, MOVING_ICON_Y_OFFSET));

	IconBase* pIcon = iconAtCoordinate(barCoordinate);
	qint32 newIndex = getCurrentIconListIndex(pIcon);

	IconBase* pMovingIcon = m_qp_iconInMotion;
	m_qp_iconInMotion = NULL;
	qint32 movingIndex = m_iconInMotionCurrentIndex;
	m_iconInMotionCurrentIndex = -1;

	if (pIcon != NULL && newIndex >= 0 && pIcon != m_qp_iconInMotion && newIndex != movingIndex)
	{
		if(moveIcon(m_iconInMotionCurrentIndex, newIndex, pIcon, true)) {
			m_iconInMotionCurrentIndex = newIndex;
		}
	} else {
		if(barCoordinate.y() <= geometry().y()) {
			deleteIcon(pMovingIcon, true);
		} else {
			rearrangeIcons(true);
		}
	}

	// now notify the source page for this icon that we have release it and it can stop tracking it

	Page* pIconSrcPage = LauncherObject::primaryInstance()->currentPage();
	if (pIconSrcPage)
	{
		ReorderablePage* pReorderIconSrcPage = qobject_cast<ReorderablePage*>(pIconSrcPage);
		if(pReorderIconSrcPage){
			pReorderIconSrcPage->externalIconMoveTerminated();
		}
	}

	DimensionsSystemInterface::PageSaver::saveQuickLaunch(LauncherObject::primaryInstance(), this);
}

//virtual
bool QuickLaunchBar::cancelRedirection(int id)
{
	QMap<int,TouchRegister>::iterator f = m_touchRegisters.find(id);
	if (f != m_touchRegisters.end())
	{
		f.value().redirecting = false;
		f.value().qpRedirectTarget = 0;
		if (f.value().pRedirectContext)
		{
			delete f.value().pRedirectContext;
			f.value().pRedirectContext = 0;
		}
	}
	return true;	//not found - so it's not redirecting by definition
}

bool QuickLaunchBar::detectAndHandleSpecialMoveAreas(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{

	if(pageCoordinate.y() < geometry().y() && redirContext.isValid()) {
		// we are dragging an icon BACK to the launcher
		return handleLauncherPageSpecialMoveArea(id,pageCoordinate,redirContext);
	}

	return false;
}

//virtual
bool QuickLaunchBar::handleLauncherPageSpecialMoveArea(int id,const QPointF& pageCoordinate,const RedirectContext& redirContext)
{

	//also, this only works if there is an icon that's being tracked and the drag started in the launcher
	if (m_qp_iconInMotion && redirContext.isValid())
	{
		//this is being redirected here from a launcher page, so the redirect source is the page we need to give the icon back to
		///....well, actually... it BETTER be a Page. And the context should be LauncherPageIconTransferRedirectContext
		LauncherPageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<LauncherPageIconTransferRedirectContext const *>(&redirContext);
		if (!pIconTxferCtx)
		{
			//yikes! what kind of $#@ redirect was this to begin with
			qDebug() << __FUNCTION__ << ": no redirect context!?";
			return false;
		}
		Page * pSourcePage = pIconTxferCtx->m_qp_srcPage;
		Page* pDestPage = LauncherObject::primaryInstance()->currentPage();
		if (!pSourcePage)
		{
			//redirect context is corrupt apparently
			qDebug() << __FUNCTION__ << ": redirect context has no source!?";
			return false;
		}
		if (!pDestPage)
		{
			qDebug() << __FUNCTION__ << ": no launcher center page!?";
			return false;
		}

		IconBase* pMaster = m_qp_iconInMotion->master();

		// first transfer the icon back to the page
		if (!pDestPage->offer((Thing *)(m_qp_iconInMotion.data()),(Thing *)(this)))
		{
			//the destination refused the offer or the take failed
			return false;
		}

		m_qp_iconInMotion = 0;

		// now redirect events to the page
		//signature, for ref: changeRedirectTo(int id,Thing * p_currentlyRedirectedToThing,Thing * p_newRedirectedToThing,RedirectContext * p_redirContext)
		pSourcePage->changeRedirectTo(id,this,pDestPage,new PageIconTransferRedirectContext(pSourcePage,pMaster));
	}
	return true;
}

void QuickLaunchBar::iconActivatedTap(QUuid iconUid)
{
	LauncherObject* launcher = LauncherObject::primaryInstance();

	if(!launcher)
		return;

	if(m_iconShowingFeedback && (m_iconShowingFeedback->uid() == iconUid)) {
		// user tapped again on the same icon while we are still working on launching the previous
		// request, so just ignore it
		return;
	}

	//translate to app uid
	QUuid appUidForIcon = launcher->appUidFromIconUid(iconUid);
	QUuid effectiveIconUid = iconUid;
	if (appUidForIcon.isNull())
	{
		//it might be an unmapped clone. Try it as its master
		IconBase * pIcon = IconHeap::iconHeap()->getIcon(iconUid);
		if (pIcon)
		{
			pIcon = pIcon->master();
			effectiveIconUid = pIcon->uid();
			appUidForIcon = launcher->appUidFromIconUid(pIcon->uid());
			if (appUidForIcon.isNull())
			{
				//still nothing
				qDebug() << __FUNCTION__
						<< ": icon uid " << iconUid
						<< " tap'd, but it doesn't have an app mapped to it , nor does its master icon (uid = " << pIcon->uid() << ")";
				return;
			}
		}
		else
		{
			qDebug() << __FUNCTION__ << ": icon uid " << iconUid << " tap'd, but it doesn't have an app mapped to it , nor does it have a master icon (it probably is a master itself)";
			return;
		}
	}

	DimensionsSystemInterface::ExternalApp * pApp = DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(appUidForIcon);
	if (pApp)
	{
		DimensionsSystemInterface::WebOSApp * pWoApp = qobject_cast<DimensionsSystemInterface::WebOSApp *>(pApp);
		if (pWoApp)
		{
			DimensionsSystemInterface::AppEffector::appEffector()->launch(pWoApp,effectiveIconUid);
		}
	}
}

void QuickLaunchBar::setAppLaunchFeedback(IconBase* pIcon)
{
	if(!pIcon)
		return;

	cancelLaunchFeedback();

	pIcon->setLaunchFeedbackVisibility(true);

	m_iconShowingFeedback = pIcon;

	m_feedbackTimer.start(DynamicsSettings::settings()->launchFeedbackTimeout);
}

void QuickLaunchBar::cancelLaunchFeedback()
{
	if(m_iconShowingFeedback) {
		m_iconShowingFeedback->setLaunchFeedbackVisibility(false);
		m_iconShowingFeedback = 0;
	}
	m_feedbackTimer.stop();
}

void QuickLaunchBar::slotCancelLaunchFeedback()
{
	cancelLaunchFeedback();
}
