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




#ifndef DIMENSIONSLAUNCHER_H_
#define DIMENSIONSLAUNCHER_H_

#include <QUuid>
#include <QRectF>
#include <QVector>
#include <QList>
#include <QSet>
#include <QEvent>

#include "groupanchoritem.h"
#include "dimensionstypes.h"
#include "icon.h"
#include "page.h"
#include "vcamera.h"
#include "appmonitor.h"
#include "pagetab.h"

class DimensionsUI;
class Page;
class PageTabBar;
class AlphabetPage;
class ReorderablePage;
class PageMovementControl;

class QGraphicsSceneMouseEvent;
class QGesture;
class QGestureEvent;
class QTouchEvent;
class QPanGesture;
class QSwipeGesture;
class QPinchGesture;
class QTapAndHoldGesture;
class QTapGesture;
class QAnimationGroup;
class QEvent;
class QDeclarativeComponent;

class PixmapButton;
class PixButton2State;
class ColorRoundRectButton;
class OverlayLayer;

namespace DimensionsSystemInterface
{
class ExternalApp;
class WebOSApp;
class PageSaver;
class PageRestore;
}

namespace LauncherAreas
{
	enum Enum
	{
		INVALID,
		PageInner,
		PageLeftSide,
		PageRightSide,
		QuickLaunchBar,
		PageTabBar,
		LauncherEther		//nowhere in particular
	};
}

class LauncherTouchRedirectContext : public RedirectContext
{
	Q_OBJECT
public:
	LauncherTouchRedirectContext() {}
	virtual ~LauncherTouchRedirectContext() {}
};

class LauncherPageHPanRedirectContext : public LauncherTouchRedirectContext
{
	Q_OBJECT
public:
	LauncherPageHPanRedirectContext() { m_valid = true; }
	virtual ~LauncherPageHPanRedirectContext() {}
};

class LauncherPageIconTransferRedirectContext : public LauncherTouchRedirectContext
{
	Q_OBJECT
public:
	enum Direction
	{
		INVALID,
		Left,
		Right,
		QuickLaunch,
		TabBar
	};
	LauncherPageIconTransferRedirectContext(Page * p_srcPage,IconBase * p_icon,LauncherPageIconTransferRedirectContext::Direction d)
	: m_qp_srcPage(p_srcPage) , m_qp_icon(p_icon) , m_direction(d) , m_targetPageIndex(-1) { m_valid = true; }
	LauncherPageIconTransferRedirectContext(Page * p_srcPage,IconBase * p_icon,LauncherPageIconTransferRedirectContext::Direction d,qint32 targetPageIndex)
	: m_qp_srcPage(p_srcPage) , m_qp_icon(p_icon) , m_direction(d) , m_targetPageIndex(targetPageIndex) { m_valid = true; }
	virtual ~LauncherPageIconTransferRedirectContext() {}
	QPointer<Page> m_qp_srcPage;
	QPointer<IconBase> m_qp_icon;
	Direction m_direction;
	qint32 m_targetPageIndex;

	static QString dbgDirectionToString(Direction d)
	{
		switch (d)
		{
		case LauncherPageIconTransferRedirectContext::Left:
			return QString("Left");
		case LauncherPageIconTransferRedirectContext::Right:
			return QString("Right");
		case LauncherPageIconTransferRedirectContext::QuickLaunch:
			return QString("QuickLaunch");
		case LauncherPageIconTransferRedirectContext::TabBar:
			return QString("TabBar");
		case LauncherPageIconTransferRedirectContext::INVALID:
		default:
			return QString("INVALID");
		}
		return QString("INVALID?");
	}
};

class LauncherObject : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)
	Q_PROPERTY(quint32 numPages READ numPages)

public:

	friend class Page;
	friend class DimensionsSystemInterface::PageSaver;
	friend class DimensionsSystemInterface::PageRestore;
	friend class VirtualCamera;

	//TODO: should make this return a const obj, and assure all functions that are useful to the outside are const as well
	//  or, DimensionsUI could be a proper singleton...but that precludes some eeeevil, advanced ideas I have for it....
	static LauncherObject * primaryInstance();

	LauncherObject(const QRectF& geometry,DimensionsUI * p_mainWindow);
	virtual ~LauncherObject();

	//returns the app uid, or QUuid() (nonvalid) if no such icon exists
	QUuid 					appUidFromIconUid(const QUuid& iconUid) const;
	QRectF 					areaTabBar() const;
	QRectF 					areaQuickLaunchBar() const;
	QRectF					areaPageLeftSide(Page * pPage=0) const;
	QRectF					areaPageRightSide(Page * pPage=0) const;
	//uid should be an icon uid, but if an app uid is provided, the app's main icon uid will be used automatically
	bool 					canShowRemoveDeleteDecoratorOnIcon(const QUuid& uid);
	qint32 					centerPageIndex() const;		//-1 if nothing is currently center or the ui is not currently static (i.e. it's animating pages)
	bool					checkAllowedIconTransferInterPage(Page * p_srcPage,Page * p_destPage,IconBase * p_icon);
	LauncherAreas::Enum 	classifyPageLocation(const QPointF& launcherCoordinate) const;
	qint32 					closestToCenter(qint32& r_dist) const;		//-1 if no pages, r_dist unspecified
	qint32 					closestToCenterDistance() const;		// 0 if no pages
	qint32 					closestToCenterPageIndex() const;	//-1 if no pages
	//called when the ui finally gets information about the max-visible-area size
	//this can only be done once, while the m_geom rect is w,h == 0. After that,
	//I can resize() but not do this init anymore
	void 					fullSizeInit(quint32 width,quint32 height);
	quint32 				horizontalLengthOfPageTrain() const;
	//returns the icon uid of the primary (default) launch point, or QUuid() (nonvalid) if no such app exists
	QUuid 					iconUidFromAppUid(const QUuid& appUid) const;
	//uid can be either an app uid or an icon uid (that belongs to an app). The function will do the right thing regardless, automatically.
	bool 					isAppRemovable(const QUuid& uid);
	quint32 				numPages() const;
	virtual bool 			offer(Thing * p_offer,Thing * p_offeringThing);
	qint32					pageHorizontalOffsetToCenter(quint32 pageIndex) const;
	Page *					currentPage() const;
	Page *					pageLeft(Page * p_fromPage) const;
	Page *					pageRight(Page * p_fromPage) const;
	virtual void 			resize(int w, int h);
	void 					saveCurrentLauncherLayouts();
	void					sendIconToQuickLaunchBar(IconBase * p_icon);
	PageTab *				tabForPage(Page * p_page) const;
	virtual bool 			take(Thing * p_takerThing);
	virtual void 			taken(Thing * p_takenThing,Thing * p_takerThing);
	virtual bool 			taking(Thing * p_victimThing, Thing * p_takerThing);
	Page *					testForIntersectOnPageTab(const QPointF& scenePosition,PageTab ** r_p_tabForPage=0);
	bool					testForIntersectPageArea(const QPointF& scenePosition);
	qint32					testForIntersectPageAreaAndPageActive(const QPointF& scenePosition);		//-1 if no page active (in center) or param not in page area; center/active index otherwise





	virtual bool event(QEvent * e);

	//TODO: TEMP: EXPERIMENTAL:
	VirtualCamera * 		virtualCamera() { return &m_dbg_vcam; }

	static QSize LauncherSizeFromScreenSize(quint32 width,quint32 height);


	//TODO: HACK: makes the temp. way of communicating icon txfer context, opaque
	static bool wasIconDroppedOnTab(IconBase * p_icon);
	static bool wasIconMovedOntoPage(IconBase * p_icon);
	static void clearIconTxContext(IconBase * p_icon);
	static void setIconTxContextDroppedOnTab(IconBase * p_icon);
	static void setIconTxContextMovedOntoPage(IconBase * p_icon);

public Q_SLOTS:

	void slotAppScanCompleted(bool initialScan=false);

	//prior to the appmonitor deleting all the app structures
	void slotAppPreRemove(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);
	//post appmonitor deleting all the app structures
	void slotAppPostRemove(const QUuid& removedAppUid,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);

	void slotAppAuxiliaryIconRemove(const QUuid& appUid,
			const QString& launchpointId,
			DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);

	//for adding the full app to the launcher; this particular slot is used when the app scan has already completed and all those apps have already been added;
	// the other time that it would trigger is when the installer started installing a new app sometime after that
	void slotAppAdd(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);

	void slotAppAuxiliaryIconAdd(const QUuid& appUid,
			const QUuid& newLaunchPointIconUid,
			DimensionsSystemInterface::AppMonitorSignalType::Enum);

	//this will destory the current page state (i.e. delete all pages that are currently
	// set up and replace them with what was saved)
	void slotRestorePagesFromSaved();

	//immediately saves the entire launcher state w.r.t. pages
	void slotSavePages();
	void slotIconActivatedTap(IconBase* pIcon);

	//TODO: TEMP: shortcutting for now to get to the page quicker.
	// 	replace this with uid of page and a lookup...
	void slotTabActivatedTap(Page * tabRelatedPage);
	void slotTabActivatedTapAndHold(Page * tabRelatedPage);

	//This will create and attempt an animation to the given page index
	// It will not interrupt currently running animations; it will just silently not do anything
	void slotGotoPageIndex(quint32 pageIndex);
	//the no animation version.
	// It will not interrupt currently running animations; it will just silently not do anything
	void slotGotoPageIndexNoAnim(quint32 pageIndex);
	//this are simplified variants to just move 1 page left or right
	// (same restrictions and behaviors apply as the others. They do animate)
	// if already at left or right -most pages, they do nothing
	void slotGotoLeftPage();
	void slotGotoRightPage();

	//called when the system (overlay window manager in the current sys version) is showing the launcher
	// DO NOT rely on this being called before or after it is actually visually showing. If that is what's needed
	// implement a new function for it
	void slotSystemShowingLauncher();
	//called when the system is hiding the launcher from view
	// again, DO NOT rely on a particular time that this is called; before or after the visual hide
	void slotSystemHidingLauncher();

	//called when system is showing/hiding something ON TOP of the launcher (usually Just Type/Quick search)
	// same conditions as slotSystemShowingLauncher....
	void slotSystemShowingLauncherOverlay();
	void slotSystemHidingLauncherOverlay();

	//TODO: it's all or nothing; either all the pages are in reorder or none of 'em
	void slotPageModeChanged(PageMode::Enum,PageMode::Enum);
	void slotReorderDoneButtonTap();
	void slotReorderExitPageRequest();

	void slotQuicklaunchFullyOpen();
	void slotQuicklaunchFullyClosed();

	void slotLauncherFullyOpen();
	void slotLauncherFullyClosed();

//////////////////////////////////////// QUEUED-MODE QSLOTS //////////////////////////////////////////
//	These are always to be connected with QueuedConnection as the connection type param to connect()
// 		This will NOT be enforced by language features, so it must be respected by convention
public Q_SLOTS:

	// cmdRequest should be cast as IconActionRequest::Enum  (see iconbase.h). if p_iconSource = 0, then sender() will be used to get the icon source
	virtual void slotIconActionRequest(int cmdRequest,IconBase * p_iconSource=0);

///////////////////////////////////////////////////////////////////////////////////////////////////////



Q_SIGNALS:

	void signalGeometryChange();
	void signalNumPagesChanged();
	void signalUiStateChanged();

	void signalReady();
	void signalNotReady();

	void signalHideMe(DimensionsTypes::HideCause::Enum cause = DimensionsTypes::HideCause::None);
	void signalShowMe(DimensionsTypes::ShowCause::Enum cause = DimensionsTypes::ShowCause::None);

	//parameter: the launchpoint id (from ApplicationManager land; the external world) of the icon to send
	void signalDropIconOnQuicklaunch(const QString&);

	///TODO: UNUSED: ?
	void signalPageCentered(QUuid pageUid);

	void signalPagesStartReorderMode();
	void signalPagesEndReorderMode();

	//these are for the page panning control mechanism
	void signalPagePanLeft();
	void signalPagePanRight();
	void signalPageMovementEnd();

	/* These two are necessary to protect/synchronize touch propagation and interactions when the various layers of the launcher are visible....
		The problem is that some of the layers of the launcher, such as the app info dialog, can be effectors on the lower layers, such as the pages. An instance of this is
		when the app info dialog removes an app: touches/gestures to the app dialog can effect the page below (due to touch propagation in Qt) but the "remove" also effects the
		page and page layouts since it reorders the icons. This can lead to potential problems. Thus, the launcher object will be given the power to block page interactions when necessary
	*/
	void signalBlockPageInteraction();
	void signalRestorePageInteraction();

protected:

	void blockPageInteraction();
	void restorePageInteraction();

	void		appDeleteDecoratorActivated(DimensionsSystemInterface::ExternalApp * p_eapp,const QUuid& iconUid);
	void 		showAppInfoDialog(const QString& dialogTitle,
									const QString& innerText,
									const QString& appIdContext,
									const QUuid& iconUid,
									bool showRemoveButton,
									const QPointF& dialogPos = QPointF());
	void		hideAppInfoDialog(bool ignoreInvisible=false,bool noAnimation=false);
	bool		appInfoDialogIsVisible() const;

	virtual bool touchEvent(QTouchEvent * event) { return true; }
	virtual bool touchStartEvent(QTouchEvent *event) { return true; }
	virtual bool touchUpdateEvent(QTouchEvent *event) { return true; }
	virtual bool touchEndEvent(QTouchEvent *event) { return true; }

	virtual bool sceneEvent(QEvent * event);
	virtual bool gestureEvent(QGestureEvent *gestureEvent) { return true; }
	virtual bool panGesture(QPanGesture *panEvent) { return true; }
	virtual bool swipeGesture(QSwipeGesture *swipeEvent) { return true; }
	virtual bool flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent);
	virtual bool pinchGesture(QPinchGesture *pinchEvent) { return true; }
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent) { return true; }
	virtual bool tapGesture(QTapGesture *tapEvent) { return true; }
	virtual bool customGesture(QGesture *customGesture) { return true; }

protected Q_SLOTS:

	void slotAnimationEnsembleFinished();
	void slotStopAnimationEnsemble(bool * r_result=0);
	void slotStartAnimationEnsemble(bool canInterrupt=true);
	void slotAddAnimationToEnsemble(QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType = DimensionsTypes::AnimationType::None);
	void slotAddAnimationTo(QAnimationGroup * p_addToGroup,QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType = DimensionsTypes::AnimationType::None);

	void slotHorizontalAnchorStopped();

	//Since gestures can't seem to be redirected during the final, GestureComplete stage, I have to do it this way
	//WARNING: This gesture object and event does NOT belong to me in here! don't do anything but read from it
	//TODO: perhaps make this safer by only sending relevant details as params, like hot spot and velocity
	void slotRedirectedFlick(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent);

	void slotAppInfoDialogRemoveButtonPressed(const QString& appIdContext,const QString& iconUidAsString);
	void slotAppInfoDialogCancelButtonPressed();

	void slotCancelLaunchFeedback();

	void slotDbgPageSaverDebugProcessDone(int exitCode);

	///////// TOUCH HANDLING //////////////////////////////////////
public:

	virtual void redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);
	virtual void redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);

	virtual void redirectedTTPMovedLimbo(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);
	virtual void redirectedTTPReleasedLimbo(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext);

protected:

	virtual void touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);
	virtual void touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);
	void 	paintBackground(QPainter * painter);

	bool isAnimationEnsembleRunning() const;
	//this one will return whether or not an immediate call to stopAnimationEnsemble will succeed
	bool canStopAnimationEnsemble() const;
	bool stopAnimationEnsemble();

	QList<QPointer<Page> > pages() const;
	quint32 initPages();

	//and these are the helpers
	quint32 helperInitPagesInitialConfigurationFavoritesAll();
	quint32 helperInitPagesInitialConfigurationFavoritesEmpty();
	quint32 helperInitPagesInitialConfigurationMultiPageConfigurator();

	/// pageIndexForApp():  this only checks the stats of the app (if already scanned) and determines the page index, based on the settings
	// in effect (see OperationalSettings). It does not do anything about apps that already exist nor does it modify any app uid maps
	// here in the launcherobject.
	quint32 pageIndexForApp(const DimensionsSystemInterface::ExternalApp& eapp);
	//these two are the helpers to pageIndexForApp
	bool pageIndexForAppByLoadedMappings(const DimensionsSystemInterface::ExternalApp& eapp,quint32& r_index);			//see OperationalSettings (look at info for the 'preferAppKeywordsForAppPlacement' key)
	bool pageIndexForAppByPredefinedDesignators(const DimensionsSystemInterface::ExternalApp& eapp,quint32& r_index);	//see OperationalSettings...

//GEMSTONE-RD
//	quint32 addDefaultPages();
//GEMSTONE-RD

	quint32 addMissingPredefinedDesignatorPages();

	//if ignoreRestoredPageIndex = true, then all restored pages are appended instead of inserted
	// into the list at index positions that were saved with the page
	quint32 addRestoredPages(bool ignoreRestoredPageIndex=false);
	//special case utility fn to create a favorites page with all available icons
	// returns null if the page can't be created
	Page * favoritePageWithAll();
	//...and the version that creates a blank favorites
	Page * favoritePageEmpty();

	//another special case utility fn to create an initial layout of launcher3 pages.
	// This one will create pages with icons as defined and implemented in
	// pageIndexForApp , pageIndexForAppByLoadedMappings , and/or pageIndexForAppByPredefinedDesignators
	// Essentially, it will create N pages, as defined in OperationalSettings::appKeywordsToPageDesignatorMapFilepath
	// and then go through all the currently registered apps, placing each on the page that the decision logic of the aforementioned functions
	// decides.
	// The parameter leftoverIconsToIndex will place all the apps that didn't map to anything explicitly into that index.
	// if the index passed in is invalid, it will default to 0 (again, 1 page must always exist, so that will be index 0)
	QList<Page *> dynamicMultiPageConfiguration();

	// ..and this variant will read in the configuration from a file.
	QList<Page *> staticMultiPageConfiguration();

	//never use these on their own...they're meant to be used from within e.g. staticMultiPageConfiguration
	QList<Page *> helperCreatePagesFromConfiguration(QList<QPair<QString,QList<QString> > >& staticLauncherConfig);
	IconList helperPageIconsFromConfiguration(Page * p_page,
			const QString& pageDesignator,const QList<QString>& webosAppLaunchPointIdList);

	//goes through the launcher's icon registry, and any icon that hasn't been placed on a page is assigned according
	//	to the policies in place (see pageIndexForApp )
	qint32 assignLeftoversToPages();

	//PREREQUISITE: pages must all be loaded
	//RESULT: the page designated in the layout settings as "startOnPageDesignated" will be in the center and active
	//	returns false if the page specified in the settings couldn't be set as the center (probably) because it wasn't found)
	//	true if it was set, including if no startup page was specified in settings
	// NOTE: no animation takes place. This is an insta-set by repositioning the horizontal anchor. So don't do it if the launcher ui is visible
	bool	setStartupPage();

	//shortcuts to the special pages - it doesn't create any pages. Returns null is no such special page was created

	ReorderablePage * favoritesPage();
	ReorderablePage * pageByDesignator(const QString& designator);

	void eraseAllLauncherFiles() const;

	ReorderablePage * pageByUid(const QString& uidString);
	//

	bool isLaunchpointAssignedToSomePage(const QUuid& uid) const;
	void launchPointAssigned(const QUuid& uid);

	// will reinitialize the tab bar
	bool createTabsForAllPages();

	void activateCenterPage();
	void deactivateCenterPage();

	//these two active___() are used in conjunction with slotPageModeChanged()
	void activePageEnteredReorderMode();
	void activePageExitedReorderMode();

	void initObjects();
	void initSignalSlotConnections();

	//see the "slot" versions of these for info (the same name, just slot prepended)
	bool gotoPageIndex(quint32 pageIndex, qint32 xSpeed = 0, bool canInterrupt = false);
	bool gotoPageIndexNoAnim(quint32 pageIndex);
	bool gotoLeftPage(qint32 xSpeed = 0);
	bool gotoRightPage(qint32 xSpeed = 0);

	void _animationFinishedProcessGroup(QAnimationGroup * pAnim);
	void _animationFinishedProcessAnim(QAbstractAnimation * pAnim);
	void _pageFinishedAddAnim(Page * p_page);
	void _pageFinishedHPanAnim();
	void _pageAddDirect(Page * p_page,quint32 idx);


	//work in conjunction with take/taken system to temporarily own and manage icons during reorders and other transitions that have the icon
	//	in-flight
	void acceptIncomingIcon(IconBase * p_icon);
	void releaseIcon(IconBase * p_icon);
	IconBase * getIconInLimbo(const QUuid& iconUid);
	bool iconLimboContains(const QUuid& iconUid);
	bool iconLimboContains(IconBase * p_icon);

	bool isPageAnchorInOverscroll();

	void setAppLaunchFeedback(IconBase* pIcon);
	void cancelLaunchFeedback();

	// ------------------- APP HANDLING ----------------------------------//

	//on a successful return (true), r_iconList contains all the icons that were mapped into the app<->icon translation maps
	bool processNewWebOSApp(DimensionsSystemInterface::WebOSApp * p_app,bool mainIconOnly,IconList& r_iconList);
	void restoreLauncherLayouts() {}

	//removes an icon with the given uid from any page currently in the launcher object
	//	does not call any other cleanup function in the launcher object (e.g. removeIconUidFromMaps) on its own
	void removeIconFromPages(IconBase * p_icon);

	//will not remove any of its copies. it *will* take care of the reverse map too, if it's a "main"/primary icon
	// if the appid is given as a hint, then the reverse map can be more quickly processed
	void removeIconUidFromMaps(const QUuid& iconUid,const QUuid& optAppUid = QUuid());

	//adds this icon to the given page. Make sure that the icon doesn't already belong to any other page. If it does, then make a clone first
	// and pass that in. This is will NOT be enforced in code and is by convention
	//	this function doesn't do any automatic adjustments to the structures on this object, e.g. the app->icon uid maps, etc
	void addIconToPage(IconBase * p_icon,Page * p_page);

	// ------------------- DEBUG: /////////////////////////////////////////

	void dbgHorizPanAnchorContents();
	void dbgPageVectorContents();

protected:

	QPointer<DimensionsUI> m_qp_mainWindow;
	static QPointer<LauncherObject> s_qp_primaryInstance;
	bool	m_fsInitHasRun;

	bool m_drawBackground;
	QPointer<PixmapObject> m_qp_background;
	QSize m_numBackgroundTiles;
	QSize m_edgeTileSizes;

	QSize m_currentPageSize;
	QRectF m_currentPageGeom;	//pre-computed from m_currentPageSize
	QPointF m_currentPagePos;		//pre-computed, relative to where the pagetab and quicklaunch bar are located
	QRectF m_currentPageLeftBorderArea;
	QRectF m_currentPageRightBorderArea;	// these two also pre-computed; they're for the icon page->page movement detection
										// (they are basically a guideline of where the page left/right "hot" area for movement detect.
										//	WOULD be if a page of m_currentPageGeom geometry was in the center (active)
										//  For the actual left/right rect of a page in the system, ask the page itself (e.g. Page::areaLeftBorder()) )

	QList<QPointer<Page> > m_pages;
	//this is wrong...qset shouldnt have a qp item
	QSet<QPointer<Page> > m_pageLimbo;			//anything temporarily floating
	QMap<QUuid,QPointer<IconBase> > m_iconLimbo;		// icons that are temporarily the property of the launcher

	GroupAnchorItem m_horizPanAnchor;
	bool            m_seenHorizPanFlick;
	qint32          m_touchStartPageIndex;

	QPointer<PageTabBar> m_qp_pageTabBar;
	QPointer<QAnimationGroup> m_qp_ensembleAnimation;

	QPointer<PixButton2State> m_qp_doneButton;
	QPointer<ColorRoundRectButton> m_qp_testButton;

	QPointer<OverlayLayer> m_qp_overlay;

	QDeclarativeComponent* m_qmlAppInfoDialog;
	QGraphicsObject* m_appInfoDialog;

	IconBase* m_iconShowingFeedback;
	QTimer    m_feedbackTimer;
	QTimer		m_motionControlTimer;

	PageMovementControl * m_p_pageMovementController;

	bool m_blockingPageInteractions;

	// ------------------- APP HANDLING ----------------------------------//

	typedef	QMap<QUuid,QUuid> UidTranslationMap;
	typedef UidTranslationMap::iterator UidTranslationMapIter;
	typedef UidTranslationMap::const_iterator UidTranslationMapConstIter;
	UidTranslationMap m_appMapByIconUid;
	UidTranslationMap m_appMapByAppUid;	//reverse map - only for the main icon. So, app uid -> main icon uid   //TODO: bad naming; rename so it's more clear

	QSet<QUuid> m_launchPointInitializationTracker;		///only used on initPages, to keep track of what app/launchpt went to what page, to prevent multiple
														// assignments when the leftovers get assigned
														// the set stores launchpoint uids. Right now, these are icon uids of the primary app icon.
														// but this can easily just be swapped for whatever uid->lpoint mapping desired
														// After all icons are assigned, this set is empty and should represent a very minimal memory impact/cost

	///// ----------------- DEBUG: ----------------------------/////////////

	VirtualCamera 	m_dbg_vcam;
	bool 	m_dbg_pageAddTriggered;

protected Q_SLOTS:

	void dbg_slotTriggerCamera(ThingPaintable * excludeMe);
	void dbg_slotPrint();

};

#endif /* DIMENSIONSLAUNCHER_H_ */
