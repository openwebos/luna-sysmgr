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




#include "dimensionslauncher.h"
#include "dimensionsmain.h"
#include "dimensionsglobal.h"
#include "debugglobal.h"
#include "page.h"
#include "alphabetpage.h"
#include "reorderablepage.h"
#include "icon.h"
#include "iconcmdevents.h"
#include "iconheap.h"
#include "pagetabbar.h"
#include "layoutsettings.h"
#include "operationalsettings.h"
#include "dynamicssettings.h"
#include "gfxsettings.h"
#include "gfxsepiaeffect.h"
#include "pixbuttonsimple.h"
#include "pixbutton2state.h"
#include "colorroundrectbutton.h"
#include "pixmapobject.h"
#include "pixmap9tileobject.h"
#include "pixmap3htileobject.h"
#include "pixmap3vtileobject.h"
#include "pixmaploader.h"
#include "appmonitor.h"
#include "appeffector.h"
#include "externalapp.h"
#include "webosapp.h"
#include "pagesaver.h"
#include "pagerestore.h"
#include "stringtranslator.h"
#include "overlaylayer.h"
#include "pagemovement.h"

#include <QCoreApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEvent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include <QProcess>
#include <QDir>

#include <QUrl>

#include <QDebug>

#include <limits.h>

#include "Settings.h"
#include "Localization.h"
#include "QtUtils.h"
#include "WindowServer.h"

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#include <QByteArray>
#include <QDataStream>

#define TAB_NORMAL_BACKGROUND_FILEPATH 	QString("tab-bg.png")
#define TAB_LAUNCHER_BACKGROUND_FILEPATH QString("launcher-bg.png")
#define DONE_BUTTON_FILEPATH QString("edit-button-done.png")
#define DONE_BUTTON_NORMAL_LOC QRect(1,2,98,34)
#define DONE_BUTTON_ACTIVE_LOC QRect(1,42,98,34)

#define TAB_SHADOW_FILEPATH QString("launcher-scrollfade-top.png")
#define QUICKLAUNCH_SHADOW_FILEPATH QString("launcher-scrollfade-bottom.png")

static PixButton2State * LoadDoneButton()
{
	QList<QRect> buttonStateCoords;
	buttonStateCoords << DONE_BUTTON_NORMAL_LOC << DONE_BUTTON_ACTIVE_LOC;
	QList<PixmapObject *> buttonStatePmos =
			PixmapObjectLoader::instance()->loadMulti(
					buttonStateCoords,
					GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + DONE_BUTTON_FILEPATH);

	if (!buttonStatePmos.empty())
	{
		PixButton2State * pButton = new PixButton2State(StringTranslator::inputString(LOCALIZED("DONE")),
														buttonStatePmos.at(0),buttonStatePmos.at(1));
		pButton->setLabelVerticalAdjust(LayoutSettings::settings()->doneButtonTextVerticalPosAdjust);

		return pButton;
	}
	return 0;
}

static OverlayLayer * LoadOverlayLayer(LauncherObject * p_launcher)
{
	PixmapObject * pTabBarShadowPmo =
			PixmapObjectLoader::instance()->quickLoad(
					GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + TAB_SHADOW_FILEPATH
			);
	PixmapObject * pQuickLaunchShadowPmo =
			PixmapObjectLoader::instance()->quickLoad(
					GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + QUICKLAUNCH_SHADOW_FILEPATH
			);
	OverlayLayer * pOverlay = new OverlayLayer(p_launcher->geometry());
	pOverlay->setParentItem(p_launcher);
	pOverlay->setTabBarShadow(pTabBarShadowPmo);
	pOverlay->setQuickLaunchShadow(pQuickLaunchShadowPmo);
	pOverlay->setMode((int)(OverlayLayerMode::OverlayShadows));
	return pOverlay;
}

QPointer<LauncherObject> LauncherObject::s_qp_primaryInstance = 0;

//static
LauncherObject * LauncherObject::primaryInstance()
{
	return s_qp_primaryInstance;
}

LauncherObject::LauncherObject(const QRectF& geometry,DimensionsUI * p_mainWindow)
: ThingPaintable(geometry)
, m_qp_mainWindow(p_mainWindow)
, m_fsInitHasRun(false)
, m_drawBackground(false)
, m_qp_background(0)
, m_numBackgroundTiles(QSize(0,0))
, m_qp_pageTabBar(0)
, m_qp_ensembleAnimation(0)
, m_qp_doneButton(0)
, m_qp_testButton(0)
, m_qp_overlay(0)
, m_dbg_vcam(this)
, m_touchStartPageIndex(-1)
, m_iconShowingFeedback(0)
, m_feedbackTimer(this)
, m_p_pageMovementController(0)
, m_blockingPageInteractions(false)
{

	if (!s_qp_primaryInstance)
	{
		s_qp_primaryInstance = this;
	}

	//to avoid doing this at init time, do it here (because init time calls to Qt internals have proven bad)
	if (IconCmdRequestEvent::s_eventType == QEvent::MaxUser+1)
	{
		IconCmdRequestEvent::s_eventType = (QEvent::Type)(QEvent::registerEventType());
	}
	initObjects();
	initSignalSlotConnections();

	grabGesture((Qt::GestureType) SysMgrGestureFlick);

	setObjectName("launcher_object");

	connect(&m_feedbackTimer, SIGNAL(timeout()), this, SLOT(slotCancelLaunchFeedback()));

	m_p_pageMovementController = new PageMovementControl(this);

	//NEED TO CALL fullSizeInit() from caller, right AFTER THIS!
}

QList<QPointer<Page> > LauncherObject::pages() const
{
	return m_pages;
}

quint32 LauncherObject::initPages()
{
	//make sure all the apps process and the main icons are created
	if (!DimensionsSystemInterface::AppMonitor::appMonitor()->completedScan())
	{
		qDebug() << __FUNCTION__ << " aborting; AppMonitor reports that no full scans have completed just yet";
		return 0;
	}

	g_warning("[BACKUPTRACE] %s : starting the allocation of all pages",__FUNCTION__);

	//grab the apps from the AppMonitor
	QList<QPointer<DimensionsSystemInterface::ExternalApp> > appList = DimensionsSystemInterface::AppMonitor::appMonitor()->allApps();
	//get icons for all the apps
	IconList allIcons;
	QMap<IconBase *,QUuid> allAuxIcons;			//key = icon , val = owner app's uid

	for (int a=0;a<appList.size();++a)
	{
		//only deal with WebOSApps for now
		//TODO: TEMP: remove this restriction
		DimensionsSystemInterface::WebOSApp * pWoApp = qobject_cast<DimensionsSystemInterface::WebOSApp *>(appList[a]);
		IconList appIcons;
		if (!processNewWebOSApp(pWoApp,false,appIcons))
		{
			qDebug() << __FUNCTION__ << ":Warn: WebOS app " << pWoApp->appId() << " didn't pass the launcher processNewApp stage";
		}
		allIcons.append(appIcons);

		//add the aux icons of the app to a set to track them
		//and make entries to the app map so that they will launch the right app
		QList<IconBase *> auxIconListForApp = pWoApp->auxAppIcons();
		for (QList<IconBase *>::iterator iit = auxIconListForApp.begin(); iit != auxIconListForApp.end();++iit)
		{
			IconBase * pIcon = *iit;
			if (pIcon)
			{
				pIcon->setUsePrerenderedLabel(true);
				allAuxIcons[pIcon] = pWoApp->uid();
			}
		}

		// these icons are used in launcher pages, so set them to use prerendered labels
		for(IconList::Iterator iit = appIcons.begin(); iit != appIcons.end(); iit++) {
			(*iit)->setUsePrerenderedLabel(true);
		}
	}

	//turn the iconList into a set for tracking
	for (IconListConstIter icon_it = allIcons.constBegin();
			icon_it != allIcons.constEnd();++icon_it)
	{
		m_launchPointInitializationTracker.insert((*icon_it)->uid());
	}
	qDebug() << __FUNCTION__ << ": tracking " << m_launchPointInitializationTracker.size() << " for purposes of page allocation";
	quint32 numRestoredPages = addRestoredPages();

	if (numRestoredPages == 0)
	{
		//NOTHING RESTORED! This is a FreshFlash (pTM) configuration situation!!!
		numRestoredPages = helperInitPagesInitialConfigurationMultiPageConfigurator();
	}

	//OK, AT THIS POINT, WHETHER PAGES WERE RESTORED FROM SAVE OR AN INITIAL-CONFIGURATOR CREATED THEM, THERE IS *SOME* SET OF PAGES (INCLUDING THE EMPTY SET)
	// IN THE LAUNCHER...

	//now scan the rest of the pages and create empty ones for all the designators that were unrealized by the restore
	// This also renames pages to bring them in line with any configuration changes (see OperationalSettings::appKeywordsToPageDesignatorMapFilepath)
	quint32 numFilledInPages = addMissingPredefinedDesignatorPages();

	//now find the "favorites" page, which MUST exist - and CREATE IT if it doesn't
	Page * pFavesPage = favoritesPage();
	if (m_pages.isEmpty())
	{
		// No pages exist (empty set of pages), so create the favorites with ALL the icons
		qDebug() << "No Favorites page found (and no others exist), so creating one with ALL the icons";
		numFilledInPages += helperInitPagesInitialConfigurationFavoritesAll();
	}
	else if (!pFavesPage)
	{
		//not sure which apps are in the other pages, so just make a blank favorites
		qDebug() << "No Favorites page found (but others exist), so creating an empty one";
		numFilledInPages += helperInitPagesInitialConfigurationFavoritesEmpty();
	}
	pFavesPage = favoritesPage();
	if (!pFavesPage)
	{
		 qFatal("Despite best efforts, the launcher cannot create a single, usable app page!");
		 exit(-1);	//just in case the above didn't bail out of the process.
	}

	qDebug() << "(Using " << pFavesPage->pageIndex() << " as the Favorites page index)";

	//find out what apps/icons still exist that haven't ended up on any pages
	// ALWAYS RUN THIS LAST!!! it needs the page layouts to be CREATED!! (it's what all the other functions above do)
	qint32 assignedLeftoverCount = assignLeftoversToPages();
	if (assignedLeftoverCount)
	{
		qDebug() << "(assigned " << assignedLeftoverCount << " left-over icons to pages; "
				<< (m_launchPointInitializationTracker.isEmpty()
										? QString("no left-overs")
										: QString("%1 left-overs remain").arg(m_launchPointInitializationTracker.size()));
	}
	else
	{
		qDebug() << "(no icons restored as left-overs; "
				<< (m_launchPointInitializationTracker.isEmpty()
						? QString("no left-overs")
						: QString("%1 left-overs remain").arg(m_launchPointInitializationTracker.size()));

	}

	// RESTORE ALL THE AUXILIARY LAUNCHPOINTS THAT WEREN'T EXPLICITLY RESTORED
	for (QMap<IconBase *,QUuid>::iterator iit = allAuxIcons.begin();iit != allAuxIcons.end();++iit)
	{
		IconBase * pIcon = iit.key();
		if (pIcon)
		{
			//if tagged already, then ignore
			if (!IconHeap::iconHeap()->iconLocationTag(pIcon->uid()).isNull())
			{
				continue;
			}
			//not tagged. Put it in favorites, and tag it
			pFavesPage->addIcon(pIcon);
			IconHeap::iconHeap()->tagIconWithLocationUid(pIcon->uid(),pFavesPage->uid(),true);

			//add entry to launcher's app map so that the aux will launch the app
			m_appMapByIconUid.insert(pIcon->uid(),iit.value());
		}
	}

	//RESTORE THE QUICKLAUNCH! (see comment in quicklaunchbar.cpp fullInit() on the sequencing explanation)
	Quicklauncher::primaryInstance()->quickLaunchBar()->restoreFromSave();

	if (OperationalSettings::settings()->debugArchiveLauncherSavesOnLauncherInit)
	{
		g_message("%s: [LAUNCHER-PAGE-INIT]: Debug: archiving launcher pages",__PRETTY_FUNCTION__);
		DimensionsSystemInterface::PageSaver::dbgPackUpAndSaveCurrentLauncher3Dir("initPagesEnd");
	}
	return numRestoredPages + numFilledInPages;
}

quint32 LauncherObject::helperInitPagesInitialConfigurationFavoritesAll()
{
	//Create a single favorites page that has ALL of the available apps
	quint32 favoritesPageIndex = ( OperationalSettings::settings()->favoritesPageIndex <= (quint32)m_pages.size() ? OperationalSettings::settings()->favoritesPageIndex : 0);
	Page * pFavesPage = favoritePageWithAll();
	_pageAddDirect(pFavesPage,favoritesPageIndex);
	return 1;
}

quint32 LauncherObject::helperInitPagesInitialConfigurationFavoritesEmpty()
{
	quint32 favoritesPageIndex = ( OperationalSettings::settings()->favoritesPageIndex <= (quint32)m_pages.size() ? OperationalSettings::settings()->favoritesPageIndex : 0);
	Page * pFavesPage = favoritePageEmpty();
	_pageAddDirect(pFavesPage,favoritesPageIndex);
	return 1;
}

quint32 LauncherObject::helperInitPagesInitialConfigurationMultiPageConfigurator()
{
	QList<Page *> initialConfiguratorPagesList;
	if (OperationalSettings::settings()->useDynamicMultiPageInitialConfigurator)
	{
		initialConfiguratorPagesList = dynamicMultiPageConfiguration();
	}
	else
	{
		initialConfiguratorPagesList = staticMultiPageConfiguration();
	}

	quint32 numRestoredPages = initialConfiguratorPagesList.size();
	//add them all officially...
	for (quint32 i=0;i<numRestoredPages;++i)
	{
		_pageAddDirect(initialConfiguratorPagesList[i],i);
	}
	return numRestoredPages;
}

//Algorithm: (Basically, rename any existing pages with any new designators, and if there are remaining new designators, create new pages from them.
//				This will allow the new designators to be used, and will not delete any pages which have been setup by the user and saved previously)
// create a list S of all aux designators specified
// iterate over all the existing pages.
// for each page, look to see if it has a designator that matches one in S (*favorites being a special case, and is skipped)
// If YES, then remove that designator from S
// If NO, then add that page to a list L.
//end iterations
// for each page in L, rename it to an existing designator in S (order of S is important!)
// if any designators are left over, create new pages with them

quint32 LauncherObject::addMissingPredefinedDesignatorPages()
{
	QList<QString> auxDesignators = DimensionsSystemInterface::AppMonitor::appMonitor()->auxPageDesignators();
	Page * pFavesPage = favoritesPage();
	int skipFavoritesIndex = (pFavesPage ? pFavesPage->pageIndex() : -1);
	QList<Page *> unmatchedPageList;
	for (int index=0;index < m_pages.size();++index)
	{
		//remove from designator list
		if (auxDesignators.removeOne(m_pages[index]->property(Page::PageDesignatorPropertyName).toString()))
		{
			//success; it was in the list and now is removed.
			continue;
		}
		//it wasn't in the list, but it could be the favorites page
		if (index == skipFavoritesIndex)
		{
			continue;
		}
		//not in the set, not the favorites -- it's an unmatched page
		unmatchedPageList << m_pages[index];
	}

	for (QList<Page *>::iterator it = unmatchedPageList.begin();
			( (it != unmatchedPageList.end()) && (!(auxDesignators.isEmpty())) );
			++it)
	{
		//grab a designator from the set
		QString unassignedDesignator = auxDesignators.takeFirst();
		if (!unassignedDesignator.isEmpty())
		{
			//rename this page w/ it
			(*it)->setProperty(Page::PageNamePropertyName,
					DimensionsSystemInterface::AppMonitor::appMonitor()->pageNameFromDesignator(unassignedDesignator));
			(*it)->setProperty(Page::PageDesignatorPropertyName,unassignedDesignator);
		}
	}

	//if any designators remain, create new pages with them
	int index = m_pages.size();
	quint32 numAdded = 0;
	for (QList<QString>::const_iterator it = auxDesignators.begin();
			it != auxDesignators.end();++it,++index)
	{
		if (pageByDesignator(*it))
		{
			//already exists
			continue;
		}
		if (index == skipFavoritesIndex)
		{
			//the faves page is on this index
			++index;
		}
		ReorderablePage * pPage = new ReorderablePage(m_currentPageGeom,this);
		pPage->layoutFromItemList(IconList());
		pPage->setProperty(Page::PageNamePropertyName,DimensionsSystemInterface::AppMonitor::appMonitor()->pageNameFromDesignator(*it));
		pPage->setProperty(Page::PageDesignatorPropertyName,*it);
		_pageAddDirect(pPage,index);
		++numAdded;
	}
	return numAdded;
}

typedef QList<QUuid> AppUidList;
typedef QList<DimensionsSystemInterface::WebOSAppRestoreObject> WebOSRestoreObjectList;

quint32 LauncherObject::addRestoredPages(bool ignoreRestoredPageIndex)
{
	bool firstFavoritesFound = false;
	//check to see if a scan has already happened in the AppMonitor....if not, then abort as there are no
	// apps to display yet
	if (!DimensionsSystemInterface::AppMonitor::appMonitor()->completedScan())
	{
		//qDebug() << __FUNCTION__ << " aborting; AppMonitor reports that no full scans have completed just yet";
		g_warning("[BACKUPTRACE] %s: aborting; AppMonitor reports that no full scans have completed just yet",__FUNCTION__);
		return 0;
	}

	//scan for masterfiles
	QList<QString> masterfiles = DimensionsSystemInterface::PageRestore::scanForSavedMasterFiles();
	if (masterfiles.isEmpty())
	{
		//no masterfiles found...nothing to restore
		g_warning("[BACKUPTRACE] %s: aborting; no masterfiles found...nothing to restore",__FUNCTION__);
		return 0;
	}

	//select the most recently used master
	QString MRUmaster = DimensionsSystemInterface::PageRestore::selectMasterFile
										(masterfiles,DimensionsSystemInterface::PageRestoreMSaveFileSelector::MostRecent);
	//attempt to retrieve launcher saved state via masterfile
	QList<QVariantMap> launcherPageConfiguration =  DimensionsSystemInterface::PageRestore::restoreLauncher(MRUmaster);
	if (launcherPageConfiguration.isEmpty())
	{
		//nothing could be restored
		g_warning("[BACKUPTRACE] %s: aborting; launcherPageConfiguration is empty...nothing to restore",__FUNCTION__);
		return 0;
	}

	if (OperationalSettings::settings()->debugArchiveLauncherSavesOnLauncherInit)
	{
		g_warning("%s: [LAUNCHER-PAGE-INIT]: WARNING: 'debugArchiveLauncherSavesOnLauncherInit' is set. Saving all launcher state!",__FUNCTION__);
		DimensionsSystemInterface::PageSaver::dbgPackUpAndSaveCurrentLauncher3Dir("addRestoredPages",&launcherPageConfiguration);
	}
//	The variantmap now contains the following:
//	map[PageSaver::SaveTagKey_PageType]
//	map[PageSaver::SaveTagKey_PageFile]
//	map[PageSaver::SaveTagKey_PageIndex]
//	map[PageSaver::SaveTagKey_PageRestoreObjectList]
//  map[PageSaver::SaveTagKey_PageUid]

	quint32 pageCount=0;
	quint32 loopCount=0;
	bool triggerDebugArchive=false;
	for (QList<QVariantMap>::iterator it = launcherPageConfiguration.begin();
			it != launcherPageConfiguration.end();++it,++loopCount)
	{
		QVariantMap pageConf = *it;
		QString pageType = pageConf.value(DimensionsSystemInterface::PageSaver::SaveTagKey_PageType).toString();
		QUuid pageUid = QUuid(pageConf.value(DimensionsSystemInterface::PageSaver::SaveTagKey_PageUid).toString());

		//Qt allows for construction of QObject subclasses based on runtime type identification via the meta-object.
		// TODO: TEMP: utilize this instead of this less flexible, hardcoded way
		Page * pPage = 0;
		if (pageType == QString(ReorderablePage::staticMetaObject.className()))
		{
			if (pageUid.isNull())
			{
				pPage = new ReorderablePage(m_currentPageGeom,this);
			}
			else
			{
				pPage = new ReorderablePage(pageUid,m_currentPageGeom,this);
			}
		}

		if (!pPage)
		{
			//type couldn't be created
			continue;
		}

		++pageCount;
		QString defaultPageName = StringTranslator::inputString(LOCALIZED("Default"))
									+ QString("_%1").arg(pageCount);

		g_warning("%s: [LAUNCHER-PAGE-INIT]: INFO: loop[%u] page[%u]: page name contained? = {%s}",
				__FUNCTION__,
				loopCount,pageCount,
				( pageConf.contains(DimensionsSystemInterface::PageSaver::SaveTagKey_PageName) ? "true" : "false")
				);
		QString pageName = pageConf.value(
					DimensionsSystemInterface::PageSaver::SaveTagKey_PageName,defaultPageName)
					.toString();
		g_warning("%s: [LAUNCHER-PAGE-INIT]: INFO: loop[%u] page[%u]: page name = {%s}",__FUNCTION__,loopCount,pageCount,qPrintable(pageName));
		g_warning("%s: [LAUNCHER-PAGE-INIT]: INFO: loop[%u] page[%u]: page designator contained? = {%s}",
				__FUNCTION__,
				loopCount,pageCount,
				( pageConf.contains(DimensionsSystemInterface::PageSaver::SaveTagKey_PageDesignator) ? "true" : "false")
		);
		QString pageDesignator = pageConf.value(
					DimensionsSystemInterface::PageSaver::SaveTagKey_PageDesignator)
					.toString();
		g_warning("%s: [LAUNCHER-PAGE-INIT]: INFO: loop[%u] page[%u]: page designator = {%s}",__FUNCTION__,loopCount,pageCount,qPrintable(pageDesignator));
		pPage->setProperty(Page::PageNamePropertyName,pageName);
		if (!pageDesignator.isEmpty())
		{
			pPage->setProperty(Page::PageDesignatorPropertyName,pageDesignator);
		}
		else
		{
			g_warning("%s: [LAUNCHER-PAGE-INIT]: INFO: loop[%u] page[%u]: NOT SETTING PAGE DESIGNATOR!",__FUNCTION__,loopCount,pageCount);
		}

		//DEBUG*****

		//if the page name ended up being "default", then backup the current state!
		if (pageName == defaultPageName)
		{
			g_message("%s: [LAUNCHER-PAGE-INIT]: WARNING: CORRUPTION BUG FOUND, DUE TO PAGE AT loop[%u] page[%u]: page designator = {%s}",
					__FUNCTION__,
					loopCount,pageCount,
					( pageDesignator.isEmpty() ? "(empty)" : qPrintable(pageDesignator)));

			//FOR PRODUCTION, nuke the contents of the launcher3 folder and restart sysmgr

			if (OperationalSettings::settings()->safeLauncherBoot)
			{
				g_message("%s: [LAUNCHER-PAGE-INIT]: safeLauncherBoot is TRUE, wiping the save files and rebooting sysmgr.",__FUNCTION__);
				eraseAllLauncherFiles();
				exit(1);
			}

			triggerDebugArchive = true;
		}
		//go through the app list ("restored object" list; see pagerestore.h, WebOSAppRestoreObject )
		// that was saved, checking each against the list of registered apps in AppMonitor
		// for ones that are found, look up the icon, form the icon list, and let the page do the layout
		// TODO: provide more cues to the page in case "flow layout" isn't the appropriate way to restore;
		// 		perhaps i'll want icons in very specific locations and/or it'll vary by the page type created

		AppUidList restoredAppUids;
		WebOSRestoreObjectList restoredAppsList;
		if ((pageDesignator == Page::PageDesignator_Favorites) && (!firstFavoritesFound)
				&& (OperationalSettings::settings()->forceFavoritesToAllContent))
		{
			//The settings tell me to use ALL the apps for the Favorites page...
			restoredAppUids = DimensionsSystemInterface::AppMonitor::appMonitor()->allAppUids();
		}
		else
		{
			restoredAppsList = (pageConf.value(DimensionsSystemInterface::PageSaver::SaveTagKey_PageRestoreObjectList)).value<WebOSRestoreObjectList>();
		}
		if (pageDesignator == Page::PageDesignator_Favorites)
		{
			firstFavoritesFound = true;
		}

		//the page will want an IconList, which has pointers to actual icons.
		// GEMSTONE-RD: all the pages have ACTUAL icons, which means that no copies should be made.
		// It also strictly implies that an icon can only be on ONE page

		IconList iconList;
		if (!restoredAppUids.isEmpty())
		{

			for (AppUidList::const_iterator app_it = restoredAppUids.constBegin();
					app_it != restoredAppUids.constEnd();++app_it)
			{
				QUuid iconUid = iconUidFromAppUid(*app_it);
				if (iconUid.isNull())
				{
					//skip it; this launcher has decided it didn't like that app or it is no longer installed
					continue;
				}
				IconBase * pIcon = IconHeap::iconHeap()->getIcon(iconUid);
				if (!pIcon)
				{
					//doesn't exist????
					continue;
				}
				//make sure this icon is the master
				pIcon = pIcon->master();

				//must still be in the tracker to be added

				//try and tag it. if it fails, then it's been tagged already
				//			qDebug() << __FUNCTION__ << ": trying to tag icon " << iconUid << " for app " << *app_it << " to page " << pPage->pageIndex();
				if (!(IconHeap::iconHeap()->tagIconWithLocationUid(iconUid,pPage->uid())))
				{
					qDebug() << __FUNCTION__ << ": error: icon " << pIcon->uid() << " already tagged to another location";
					continue;
				}
				iconList << pIcon;
				//add this icon as an entry into the appUid-from-iconUid map, so that tap-actions on this page will work
				m_appMapByIconUid.insert(pIcon->uid(),*app_it);
			}
		}		//end if restore favorites-with-all-apps (only really used under special config situations; none of the auxiliary launch points on favorites will show up)
		else
		{
			/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			for (WebOSRestoreObjectList::const_iterator o_it = restoredAppsList.constBegin();
					o_it != restoredAppsList.constEnd();++o_it)
			{
				DimensionsSystemInterface::WebOSAppRestoreObject const& pRestoreObj = *o_it;
				IconBase * pIcon = 0;
				if (pRestoreObj.launchpointId == pRestoreObj.appId)
				{
					//this was a main icon
					QUuid iconUid = iconUidFromAppUid(pRestoreObj.appUid);
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
				//make sure this icon is the master
				pIcon = pIcon->master();

				//try and tag it. if it fails, then it's been tagged already
				//			qDebug() << __FUNCTION__ << ": trying to tag icon " << iconUid << " for app " << *app_it << " to page " << pPage->pageIndex();
				if (!(IconHeap::iconHeap()->tagIconWithLocationUid(pIcon->uid(),pPage->uid())))
				{
					qDebug() << __FUNCTION__ << ": error: icon " << pIcon->uid() << " already tagged to another location";
					continue;
				}
				iconList << pIcon;
				//add this icon as an entry into the appUid-from-iconUid map, so that tap-actions on this page will work
				m_appMapByIconUid.insert(pIcon->uid(),pRestoreObj.appUid);
			}
		}	/// end else normal page restore

		//do the layout for the page
		pPage->layoutFromItemList(iconList);
		//add the page
		qint32 restoredPageIndex = pageConf.value(DimensionsSystemInterface::PageSaver::SaveTagKey_PageIndex,-1).toInt();
		if ((ignoreRestoredPageIndex)
			|| (restoredPageIndex == -1))
		{
			//just append this page
			_pageAddDirect(pPage,m_pages.size());
		}
		else
		{
			_pageAddDirect(pPage,restoredPageIndex);
		}

		// DEBUG ****
		// archive and save here if flagged
		if (triggerDebugArchive)
		{
			g_warning("%s: [LAUNCHER-PAGE-INIT]: WARNING: DEBUG PAGE ARCHIVING ABOUT TO BEGIN",
					__FUNCTION__);
			DimensionsSystemInterface::PageSaver::dbgPackUpAndSaveCurrentLauncher3Dir("pageNameDefaultIncident");
			g_warning("%s: [LAUNCHER-PAGE-INIT]: WARNING: DEBUG PAGE ARCHIVING END (SPAWNED)",
								__FUNCTION__);
		}
	}

	if (launcherPageConfiguration.empty())
	{
		g_warning("%s: [LAUNCHER-PAGE-INIT]: INFO: Empty launcher page configuration. Nothing was done.",
					__FUNCTION__
				);
	}
	return pageCount;
}

void LauncherObject::eraseAllLauncherFiles() const
{
	//WARNING: DANGEROUS! if the savedPagesDirectory is set to something bad, then this will take out major chunks of the sys
	//do a very very basic check  (it is not meant to catch deliberate or even a lot of non-deliberate mistakes)
	QDir dir(OperationalSettings::settings()->savedPagesDirectory);
	QString canonicalDirPath = dir.canonicalPath();

	if ((canonicalDirPath == "/") || (canonicalDirPath == "/media/cryptofs") || (canonicalDirPath == "/var"))
	{
		g_message("%s: Path specified as %s, refusing to rm -fr it!",__FUNCTION__,qPrintable(canonicalDirPath));
	}

	QString cmdline = QString("rm -fr ")+OperationalSettings::settings()->savedPagesDirectory;
	int ret = system(cmdline.toLocal8Bit().constData());
	Q_UNUSED(ret);
}

Page * LauncherObject::favoritePageWithAll()
{
	Page * pPage = new ReorderablePage(m_currentPageGeom,this);
	pPage->setProperty(Page::PageNamePropertyName,
			DimensionsSystemInterface::AppMonitor::appMonitor()->pageNameFromDesignator(Page::PageDesignatorFavorites()));
	pPage->setProperty(Page::PageDesignatorPropertyName,Page::PageDesignatorFavorites());

	if (!DimensionsSystemInterface::AppMonitor::appMonitor()->completedScan())
	{
		//qDebug() << __FUNCTION__ << " aborting; AppMonitor reports that no full scans have completed just yet";
		return pPage;
	}

	AppUidList restoredAppUids = DimensionsSystemInterface::AppMonitor::appMonitor()->allAppUids();

	//the page will want an IconList, which has pointers to actual icons.
	// GEMSTONE-RD: all the pages have ACTUAL icons, which means that no copies should be made.
	// It also strictly implies that an icon can only be on ONE page

	IconList iconList;
	for (AppUidList::const_iterator app_it = restoredAppUids.constBegin();
			app_it != restoredAppUids.constEnd();++app_it)
	{
		QUuid iconUid = iconUidFromAppUid(*app_it);
		if (iconUid.isNull())
		{
			//skip it; this launcher has decided it didn't like that app or it is no longer installed
			continue;
		}
		IconBase * pIcon = IconHeap::iconHeap()->getIcon(iconUid);
		if (!pIcon)
		{
			//doesn't exist????
			continue;
		}
		//make sure icon is the master
		pIcon = pIcon->master();

		//it still has to be in the tracker set to continue
		if (isLaunchpointAssignedToSomePage(pIcon->uid()))
		{
			qDebug() << __FUNCTION__ << ": icon " << pIcon->uid() << " for app " << *app_it << " has apparently already been used";
			continue;
		}

		//try and tag it. if it fails, then it's been tagged already
//		qDebug() << __FUNCTION__ << ": trying to tag icon " << pIcon->uid() << " for app " << *app_it << " to page " << pPage->pageIndex();
		if (!(IconHeap::iconHeap()->tagIconWithLocationUid(pIcon->uid(),pPage->uid())))
		{
			qDebug() << __FUNCTION__ << ": error: icon " << pIcon->uid() << " already tagged to another location";
			continue;
		}
		iconList << pIcon;

		//remove from tracker
		launchPointAssigned(pIcon->uid());

		//add this icon as an entry into the appUid-from-iconUid map, so that tap-actions on this page will work
		//TODO: i think this is a hold over from when these pages had icon COPIES!...remove???
		m_appMapByIconUid.insert(pIcon->uid(),*app_it);
	}

	//do the layout for the page
	pPage->layoutFromItemList(iconList);
	return pPage;
}

Page * LauncherObject::favoritePageEmpty()
{
	Page * pPage = new ReorderablePage(m_currentPageGeom,this);
	pPage->setProperty(Page::PageNamePropertyName,
			DimensionsSystemInterface::AppMonitor::appMonitor()->pageNameFromDesignator(Page::PageDesignatorFavorites()));
	pPage->setProperty(Page::PageDesignatorPropertyName,Page::PageDesignatorFavorites());

	pPage->layoutFromItemList(IconList());
	return pPage;
}

QList<Page *> LauncherObject::dynamicMultiPageConfiguration()
{
	//TODO: IMPLEMENT
	return QList<Page *>();
}

QList<Page *> LauncherObject::staticMultiPageConfiguration()
{

	//cannot do this if the full app scan has not completed
	if (!DimensionsSystemInterface::AppMonitor::appMonitor()->completedScan())
	{
		qDebug() << __FUNCTION__ << " aborting; AppMonitor reports that no full scans have completed just yet";
		return QList<Page *>();
	}

	//load the static config...this is represented in LEGACY config files,
	// which are (or at least WERE at the time of this writing) in:
	// 	Settings::launcherDefaultPositions  ("/etc/palm/default-launcher-page-layout.json")
	// and Settings::launcherCustomPositions   ("/usr/lib/luna/customization/default-launcher-page-layout.json")

	//this map has a key = designator of the page - it's been converted to the designator launcher3 understands,
	// i.e. it's the actual Page designators as they're being used by PageSaver and PageRestore
	// the values in the list are WebOS LaunchPoint ids....so in the form of e.g. <appid>_default
	QList<QPair<QString,QList<QString> > > staticLauncherConfig;

	//the customized one first...if not blocked by settings
	if (!OperationalSettings::settings()->ignoreLegacyCustomizationFiles)
	{
		staticLauncherConfig =
			DimensionsSystemInterface::PageRestore::loadStaticLegacyLauncherConfig
			(StringTranslator::inputString(Settings::LunaSettings()->launcherCustomPositions));
	}

	if (staticLauncherConfig.isEmpty())
	{
		//the default one?
		staticLauncherConfig =
					DimensionsSystemInterface::PageRestore::loadStaticLegacyLauncherConfig
					(StringTranslator::inputString(Settings::LunaSettings()->launcherDefaultPositions));
	}
	if (staticLauncherConfig.isEmpty())
	{
		// no configuration could be loaded
		return QList<Page *>();
	}

	qint32 startNumLaunchPoints = m_launchPointInitializationTracker.size();
	QList<Page *> rlist = helperCreatePagesFromConfiguration(staticLauncherConfig);
	qint32 endNumLaunchPoints = m_launchPointInitializationTracker.size();
	qDebug() << __FUNCTION__ << ": results: Created " << rlist.size() << " pages , assigned "
			<< startNumLaunchPoints-endNumLaunchPoints << " ( " << endNumLaunchPoints << " still remain )";
	return rlist;
}

QList<Page *> LauncherObject::helperCreatePagesFromConfiguration(QList<QPair<QString,QList<QString> > >& staticLauncherConfig)
{

	QList<QPair<Page *,IconList> > pagesList;

	for (QList<QPair<QString,QList<QString> > >::iterator it = staticLauncherConfig.begin();
			it != staticLauncherConfig.end();++it)
	{
		QString pageDesignator = (*it).first;
		Page * pPage = new ReorderablePage(m_currentPageGeom,this);
		pPage->setProperty(Page::PageNamePropertyName,
			DimensionsSystemInterface::AppMonitor::appMonitor()->pageNameFromDesignator(pageDesignator));
		pPage->setProperty(Page::PageDesignatorPropertyName,pageDesignator);
		//get all the icons for the page...pass in the app uid set; the helper will remove uids that it adds to the page. This will serve to find out what the
		// "leftovers" are at the end
		IconList iconList = helperPageIconsFromConfiguration(pPage,(*it).first,(*it).second);
		pagesList << QPair<Page *,IconList>(pPage,iconList);
	}

	//do the layouts for all the pages
	QList<Page *> rlist;
	for (int i = 0;i < pagesList.size();++i)
	{
		Page * pPage = pagesList[i].first;
		IconList iconList = pagesList[i].second;
		//do the layout for the page
		pPage->layoutFromItemList(iconList);
		rlist << pPage;
	}

	return rlist;
}

IconList LauncherObject::helperPageIconsFromConfiguration(Page * p_page,const QString& pageDesignator,
			const QList<QString>& webosAppLaunchPointIdList)
{
	IconList iconList;

	for (QList<QString>::const_iterator it = webosAppLaunchPointIdList.constBegin();
			it != webosAppLaunchPointIdList.end();++it)
	{
		//go through the list of launchpoints and get the app icons for them
		//TODO: support more than just the main icon (default launchpt) for an app????

		QString appId = DimensionsSystemInterface::AppMonitor::appIdFromLaunchpointId(*it);
		if (appId.isEmpty())
		{
			qDebug() << __FUNCTION__ << " no appId associated with launchpointId " << *it;
			continue;
		}

		IconBase * pIcon = IconHeap::iconHeap()->getIcon(appId,*it);
		if (!pIcon)
		{
			//doesn't exist????
			continue;
		}

		//make sure it's the master copy and not a clone
		pIcon = pIcon->master();	//it will return itself if it's a master

		//it still has to be in the tracker set to continue
		if (isLaunchpointAssignedToSomePage(pIcon->uid()))
		{
			qDebug() << __FUNCTION__ << ": icon " << pIcon->uid() << " for appId " << appId << " has apparently already been used";
			continue;
		}
		// CHECK AGAINST THE APPS THAT WERE REGISTERED INTO THE APP<->ICON MAPS
		// (here in the LauncherObject)
		// The reason for this is that, even though the icon heap created the icon, the launcher might have rejected the
		// app for some reason - like blacklisting
		QUuid appUid = appUidFromIconUid(pIcon->uid());
		if (appUid.isNull())
		{
			//skip it; this launcher has decided it didn't like that app or it is no longer installed
			qDebug() << __FUNCTION__ << ": icon " << pIcon->uid() << " for appId " << appId << " is not registered with the launcher (probably rejected due to blacklisting or 'hide' flags)";
			continue;
		}

		//try and tag it. if it fails, then it's been tagged already
//		qDebug() << __FUNCTION__ << ": trying to tag icon " << pIcon->uid() << " for app " << appUid << " to page " << p_page->pageName();
		if (!(IconHeap::iconHeap()->tagIconWithLocationUid(pIcon->uid(),p_page->uid())))
		{
			qDebug() << __FUNCTION__ << ": error: icon " << pIcon->uid() << " already tagged to another location";
			continue;
		}
		//remove the app uid from the tracker set
		launchPointAssigned(pIcon->uid());

		//add to the icon list
		iconList << pIcon;
	}
	return iconList;
}

qint32 LauncherObject::assignLeftoversToPages()
{
	qint32 count = 0;
	for (UidTranslationMapIter it = m_appMapByIconUid.begin();
			it != m_appMapByIconUid.end();++it)
	{

		IconBase * pIcon = IconHeap::iconHeap()->getIcon(it.key());
		if (!pIcon)
		{
			//doesn't exist????
			qDebug() << __FUNCTION__ << ": error: icon " << it.key() << " isn't in the heap";
			continue;
		}

		//make sure it's the master copy and not a clone
		pIcon = pIcon->master();

		DimensionsSystemInterface::ExternalApp * pApp = DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(it.value());
		if (!pApp)
		{
			qDebug() << __FUNCTION__ << ": error: icon " << pIcon->uid() << " doesn't have a valid app association";
			continue;
		}
		quint32 pageIndex = pageIndexForApp(*pApp);
		Page * pPage = m_pages[pageIndex];

		DimensionsSystemInterface::WebOSApp * pWebOSapp = qobject_cast<DimensionsSystemInterface::WebOSApp *>(pApp);
		QString appId = (pWebOSapp ? pWebOSapp->appId() : QString("(not a webos app)"));
		//it still has to be in the tracker set to continue
		if (isLaunchpointAssignedToSomePage(pIcon->uid()))
		{
			qDebug() << __FUNCTION__ << ": icon " << pIcon->uid() << " for appId " << appId << " has apparently already been used";
			continue;
		}

		//try and tag it. if it fails, then it's been tagged already
//		qDebug() << __FUNCTION__ << ": trying to tag icon " << pIcon->uid() << " for app " << pApp->uid() << " to page " << pPage->pageName() << " , " << pPage->pageIndex() << " , " << pageIndex;
		if (!(IconHeap::iconHeap()->tagIconWithLocationUid(pIcon->uid(),pPage->uid())))
		{
			qDebug() << __FUNCTION__ << ": error: icon " << pIcon->uid() << " already tagged to another location";
			continue;
		}
		//place on the page - ASSUMES PAGE ALREADY HAS A LAYOUT DONE!
		if (pPage->addIcon(pIcon))
		{
			++count;
			//remove it from the tracker
			launchPointAssigned(pIcon->uid());
		}
		else
		{
			qDebug() << __FUNCTION__ << ": Warning: icon add for icon = " << pIcon->uid() << " failed on page " << pageIndex;
		}
	}
	return count;
}

ReorderablePage * LauncherObject::favoritesPage()
{
	ReorderablePage * pFound = 0;
	for (QList<QPointer<Page> >::iterator it = m_pages.begin();
			it != m_pages.end();++it)
	{
		ReorderablePage * pReorderPage = qobject_cast<ReorderablePage *>(*it);
		if (pReorderPage)
		{
			if (pReorderPage->property(Page::PageDesignatorPropertyName).toString() == Page::PageDesignatorFavorites())
			{
				pFound = pReorderPage;
				break;
			}
		}
	}
	return pFound;
}

ReorderablePage * LauncherObject::pageByDesignator(const QString& designator)
{
	ReorderablePage * pFound = 0;
	for (QList<QPointer<Page> >::iterator it = m_pages.begin();
			it != m_pages.end();++it)
	{
		ReorderablePage * pReorderPage = qobject_cast<ReorderablePage *>(*it);
		if (pReorderPage)
		{
			if (pReorderPage->property(Page::PageDesignatorPropertyName).toString() == designator)
			{
				pFound = pReorderPage;
				break;
			}
		}
	}
	return pFound;
}

ReorderablePage * LauncherObject::pageByUid(const QString& uidString)
{
	ReorderablePage * pFound = 0;
	for (QList<QPointer<Page> >::iterator it = m_pages.begin();
			it != m_pages.end();++it)
	{
		ReorderablePage * pReorderPage = qobject_cast<ReorderablePage *>(*it);
		if (pReorderPage)
		{
			if (pReorderPage->uid().toString() == uidString)
			{
				pFound = pReorderPage;
				break;
			}
		}
	}
	return pFound;
}
//

bool LauncherObject::isLaunchpointAssignedToSomePage(const QUuid& uid) const
{
	return !m_launchPointInitializationTracker.contains(uid);
}

void LauncherObject::launchPointAssigned(const QUuid& uid)
{
	m_launchPointInitializationTracker.remove(uid);
}

bool LauncherObject::setStartupPage()
{
	//find out which page should be in the center

	QString designatorSettings = OperationalSettings::settings()->startOnPageDesignated;
	if (designatorSettings.isEmpty())
	{
		return true;	//nothing specific set in settings.
	}

	//find the page with this designator
	qint32 idx = 0;
	for (;idx < m_pages.size();++idx)
	{
		Page * pPage = m_pages[idx];
		if (!pPage)
		{
			continue;
		}
		if (designatorSettings == pPage->property(Page::PageDesignatorPropertyName).toString())
		{
			//found it
			break;
		}
	}
	if (idx == m_pages.size())
	{
		//didn't find it
		return false;
	}

	if (!m_pages.at(idx))
	{
		//oops this page index seems to have been deleted!
		return false;
	}

	qint32 centerIndex = centerPageIndex();
	if (centerIndex != -1)
	{
		if (idx == centerIndex)
		{
			//the given index to make the center is already the center
			//just (re)activate it an bail
			m_pages[centerIndex]->activatePage();
		}
		else
		{
			//need to bring a new center
			m_pages[centerIndex]->deactivatePage();
			slotGotoPageIndexNoAnim(idx);
			m_pages[idx]->activatePage();
		}
	}

	return true;
}

bool LauncherObject::createTabsForAllPages()
{
	if (!m_qp_pageTabBar)
	{
		//no tab bar, no tabs...
		return false;
	}

	for (QList<QPointer<Page> >::const_iterator it = m_pages.constBegin();
			it != m_pages.constEnd();++it)
	{
		Page * pPage = *it;
		if (!pPage)
		{
			continue;
		}
		QString tabLabel = pPage->property(Page::PageNamePropertyName).toString().toUpper();
		if (tabLabel.isEmpty())
		{
			//TODO: LOCALIZE:
			tabLabel = QString("(UNTITLED)");
		}
		else
		{
			m_qp_pageTabBar->slotAddTab(tabLabel,pPage);
		}
	}
	return true;
}

void LauncherObject::activateCenterPage()
{
	//check for a centered page
	qint32 centerIndex = centerPageIndex();
	if (centerIndex == -1)
	{
		//nothing centered
		return;
	}

	//emit signal with the uid of the page that is in the center
	Page * pCenterPage = m_pages[centerIndex];
	if (!pCenterPage)
	{
		//something bad happened and the page at the index was previously
		//deleted
		return;
	}
	pCenterPage->activatePage();
}

void LauncherObject::deactivateCenterPage()
{
	//check for a centered page
	qint32 centerIndex = centerPageIndex();
	if (centerIndex == -1)
	{
		//nothing centered
		return;
	}

	//emit signal with the uid of the page that is in the center
	Page * pCenterPage = m_pages[centerIndex];
	if (!pCenterPage)
	{
		//something bad happened and the page at the index was previously
		//deleted
		return;
	}
	pCenterPage->deactivatePage();
}

bool LauncherObject::isPageAnchorInOverscroll()
{
	if(m_horizPanAnchor.pos().x() > 0)
		return true;

	if(m_horizPanAnchor.pos().x() < -(m_currentPageSize.width() * (m_pages.size() - 1)))
		return true;

	return false;
}

void LauncherObject::initObjects()
{
	IconHeap::iconHeap();
	DimensionsSystemInterface::AppMonitor::appMonitor();
}

void LauncherObject::initSignalSlotConnections()
{
	//---------------- AppMonitor connections
	connect(DimensionsSystemInterface::AppMonitor::appMonitor(),SIGNAL(signalFullScanCompleted(bool)),
			this,SLOT(slotAppScanCompleted(bool)));

	connect(DimensionsSystemInterface::AppMonitor::appMonitor(),SIGNAL(signalRemovedApp(const DimensionsSystemInterface::ExternalApp&,DimensionsSystemInterface::AppMonitorSignalType::Enum)),
			this,SLOT(slotAppPreRemove(const DimensionsSystemInterface::ExternalApp&,DimensionsSystemInterface::AppMonitorSignalType::Enum))
			);
	connect(DimensionsSystemInterface::AppMonitor::appMonitor(),SIGNAL(signalRemovedAppComplete(const QUuid&,DimensionsSystemInterface::AppMonitorSignalType::Enum)),
			this,SLOT(slotAppPostRemove(const QUuid&,DimensionsSystemInterface::AppMonitorSignalType::Enum))
			);
	connect(DimensionsSystemInterface::AppMonitor::appMonitor(),SIGNAL(signalAppAuxiliaryIconRemove(const QUuid&,const QString&,DimensionsSystemInterface::AppMonitorSignalType::Enum)),
			this,SLOT(slotAppAuxiliaryIconRemove(const QUuid&,const QString&,DimensionsSystemInterface::AppMonitorSignalType::Enum))
		);

	connect(DimensionsSystemInterface::AppMonitor::appMonitor(),SIGNAL(signalNewApp(const DimensionsSystemInterface::ExternalApp&,DimensionsSystemInterface::AppMonitorSignalType::Enum)),
			this,SLOT(slotAppAdd(const DimensionsSystemInterface::ExternalApp&,DimensionsSystemInterface::AppMonitorSignalType::Enum))
		);

	connect(DimensionsSystemInterface::AppMonitor::appMonitor(),
			SIGNAL(signalNewAdditionalLaunchPointForApp(const QUuid&,const QUuid&,DimensionsSystemInterface::AppMonitorSignalType::Enum)),
			this,
			SLOT(slotAppAuxiliaryIconAdd(const QUuid&,
					const QUuid&,
					DimensionsSystemInterface::AppMonitorSignalType::Enum))
		);
}

void LauncherObject::fullSizeInit(quint32 width,quint32 height)
{
	if (m_fsInitHasRun)
	{
		//bail. not good to call this multiple times
		return;
	}
	QSize derivedLauncherSize = LauncherSizeFromScreenSize(width,height);
	m_geom = DimensionsGlobal::realRectAroundRealPoint(derivedLauncherSize);

	m_qp_background = new PixmapObject(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + TAB_LAUNCHER_BACKGROUND_FILEPATH);
	if (m_qp_background)
	{
		m_drawBackground = true;
	}


	m_dbg_pageAddTriggered = false;

	QSize pageTabBarSize = PageTabBar::PageTabSizeFromLauncherSize((quint32)m_geom.width(),(quint32)m_geom.height());
	m_qp_pageTabBar = new PageTabBar(DimensionsGlobal::realRectAroundRealPoint(pageTabBarSize),this);
	Pixmap9TileObject * pNormalBgPmo = PixmapObjectLoader::instance()->quickLoadNineTiled(
				QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + TAB_NORMAL_BACKGROUND_FILEPATH),
				20,20,4,4
	);

	m_qp_pageTabBar->setBackground(pNormalBgPmo);
	m_qp_pageTabBar->setVisible(true);
	m_qp_pageTabBar->setPos(0.0,m_geom.top()-m_qp_pageTabBar->geometry().top());

	connect(m_qp_pageTabBar,SIGNAL(signalTabForPageActivatedTap(Page *)),
			this,SLOT(slotTabActivatedTap(Page *)));

	connect(m_qp_pageTabBar,SIGNAL(signalTabForPageActivatedTapAndHold(Page *)),
			this,SLOT(slotTabActivatedTapAndHold(Page *)));

	connect(this,SIGNAL(signalBlockPageInteraction()),
			m_qp_pageTabBar,SLOT(slotLauncherBlockedInteractions())
			);
	connect(this,SIGNAL(signalRestorePageInteraction()),
			m_qp_pageTabBar,SLOT(slotLauncherAllowedInteractions())
			);

	m_qp_doneButton = LoadDoneButton();
	if (m_qp_doneButton)
	{
		m_qp_doneButton->setParentItem(m_qp_pageTabBar);
		m_qp_doneButton->setVisible(false);
		m_qp_doneButton->setPos(QPoint(m_qp_pageTabBar->geometry().right()-m_qp_doneButton->geometry().width(),0.0)
								+LayoutSettings::settings()->doneButtonPositionAdjust);

		connect(m_qp_doneButton,SIGNAL(signalActivated()),
				this,SLOT(slotReorderDoneButtonTap()));
	}

//	m_qp_testButton = new ColorRoundRectButton(QSize(50,30),"TEST",Qt::blue);
//	m_qp_testButton->setParentItem(m_qp_pageTabBar);
//	m_qp_testButton->setVisible(false);
//	qDebug() << m_qp_testButton->geometry();
//	m_qp_testButton->setPos(m_qp_pageTabBar->geometry().right()-m_qp_testButton->geometry().width()-100.0,0.0);

	// AppInfo Dialog
	QDeclarativeEngine* qmlEngine = WindowServer::instance()->declarativeEngine();
	if(qmlEngine) {
		QDeclarativeContext* context =	qmlEngine->rootContext();
		QString qmlPath = StringTranslator::inputString(Settings::LunaSettings()->lunaQmlUiComponentsPath + "AppInfoDialog/AppInfoDialog.qml");
		QUrl url = QUrl::fromLocalFile(qmlPath);
		m_qmlAppInfoDialog = new QDeclarativeComponent(qmlEngine, url, this);
		if(m_qmlAppInfoDialog) {
			m_appInfoDialog = qobject_cast<QGraphicsObject *>(m_qmlAppInfoDialog->create());
			if(m_appInfoDialog) {
				m_appInfoDialog->setAcceptTouchEvents(true);
				m_appInfoDialog->setPos (-m_appInfoDialog->boundingRect().width()/2, -m_appInfoDialog->boundingRect().height()/2);
				m_appInfoDialog->setParentItem(this);
				m_appInfoDialog->setVisible(false);
				m_appInfoDialog->setOpacity(0.0);

				connect(m_appInfoDialog, SIGNAL(removeButtonPressed(const QString&,const QString&)), SLOT(slotAppInfoDialogRemoveButtonPressed(const QString&,const QString&)));
				connect(m_appInfoDialog, SIGNAL(cancelButtonPressed()), SLOT(slotAppInfoDialogCancelButtonPressed()));
			}
		}
		else
		{
			delete m_qmlAppInfoDialog;
			m_qmlAppInfoDialog = 0;
		}
	}
	//compute the page size from ui size
	if (LayoutSettings::settings()->autoPageSize)
	{
		//get the area of the quicklaunch bar
		QRectF quickLaunchArea;
		if (m_qp_mainWindow)
		{
			quickLaunchArea = m_qp_mainWindow->quickLaunchArea();
		}
		qDebug() << __FUNCTION__ << ": using QuickLaunchArea = " << quickLaunchArea;
		QSize r = QSize(DimensionsGlobal::roundDown(m_geom.width()),
				qMax(2,DimensionsGlobal::roundDown(m_geom.bottom()
													-m_qp_pageTabBar->positionRelativeGeometry().bottom()
													-quickLaunchArea.height()-1.0)));
		//make evenly divisible (multiple of 2)
		r.setWidth(r.width() - (r.width() % 2));
		r.setHeight(r.height() - (r.height() % 2));
		m_currentPageSize = r;
	}
	else
	{
		m_currentPageSize = Page::PageSizeFromLauncherSize((quint32)m_geom.width(),(quint32)m_geom.height());
	}

	//pre-compute geom
	m_currentPageGeom = DimensionsGlobal::realRectAroundRealPoint(m_currentPageSize);
	m_currentPagePos = QPointF(0.0,m_qp_pageTabBar->positionRelativeGeometry().bottom()-m_currentPageGeom.top());	// - because top will be neg.
	m_horizPanAnchor.setOwner(this);
	m_horizPanAnchor.setPos(0,LayoutSettings::settings()->centerUiVerticalOffset + m_currentPagePos.y());
	m_seenHorizPanFlick = false;
	{
		int borderWidth = LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.width();
		if (borderWidth > m_currentPageGeom.width()/2)
		{
			borderWidth = m_currentPageGeom.width()/2;
		}
		m_currentPageLeftBorderArea =  QRectF(m_currentPageGeom.topLeft(),QSize(borderWidth,m_currentPageGeom.size().toSize().height()));
	}
	{
		int borderWidth = LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.height();		//it's not actually "height". see layoutsettings.cpp
		if (borderWidth > m_currentPageGeom.width()/2)
		{
			borderWidth = m_currentPageGeom.width()/2;
		}

		m_currentPageRightBorderArea = QRectF(m_currentPageGeom.topRight()-QPointF((qreal)borderWidth,0.0),
				QSize(borderWidth,m_currentPageGeom.size().toSize().height()));
	}

	m_qp_overlay = LoadOverlayLayer(this);
	if (m_qp_overlay)
	{
		m_qp_overlay->setVisible(true);
		m_qp_overlay->setZValue(1.0);
	}

}

LauncherAreas::Enum LauncherObject::classifyPageLocation(const QPointF& rawLauncherCoordinate) const
{
	//TODO: could be optimized

	QRectF borderArea = areaTabBar();
	//clip point
	QPointF launcherCoordinate = QPointF(
								qBound(geometry().left(),rawLauncherCoordinate.x(),geometry().right()),
								qBound(geometry().top(),rawLauncherCoordinate.y(),geometry().bottom())
								);

//	qDebug() << __FUNCTION__ << "launcherCoordinate (clipped): " << launcherCoordinate << " , (raw = " << rawLauncherCoordinate << " )"
//				<< " vs.: areaTabBar = " << borderArea;

	//from the top:

	if (launcherCoordinate.y() < borderArea.y())
	{
		//Make it so the tab bar is all the way to the top...can't go above it. Clip-point will assure this
//		qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::PageTabBar ( above top)";
		return LauncherAreas::PageTabBar;
	}

	if (borderArea.contains(launcherCoordinate))
	{
//		qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::PageTabBar (contains)";
		return LauncherAreas::PageTabBar;
	}

	borderArea = areaQuickLaunchBar();

	if (launcherCoordinate.y() < borderArea.top())
	{
		//check left and right sides of the page
		if (areaPageLeftSide().contains(launcherCoordinate))
		{
//			qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::PageLeftSize (above ql-top)";
			return LauncherAreas::PageLeftSide;
		}
		else if (areaPageRightSide().contains(launcherCoordinate))
		{
//			qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::PageRightSide (above ql-top)";
			return LauncherAreas::PageRightSide;
		}
//		qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::Page (above ql-top)";
		return LauncherAreas::PageInner;
	}

	/// ?????????
	//m_qp_overlay->resize(m_geom.size().toSize());

//	qDebug() << __FUNCTION__ << "launcherCoordinate: " << launcherCoordinate
//		<< " vs.: areaQuickLaunchBar = " << borderArea;

	if (borderArea.contains(launcherCoordinate))
	{
//		qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::QuickLaunchBar (contains)";
		return LauncherAreas::QuickLaunchBar;
	}

	if (launcherCoordinate.y() > borderArea.y())
	{
//		qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::QuickLaunchBar (below ql-bottom)";
		return LauncherAreas::QuickLaunchBar;
	}

	//return launcher nothingness!
//	qDebug() << __FUNCTION__ << ": RETURNING: LauncherAreas::LauncherEther (!!!fall-through!!!)";
	return LauncherAreas::LauncherEther;
}

QRectF LauncherObject::areaTabBar() const
{
	if (m_qp_pageTabBar)
	{
		return m_qp_pageTabBar->positionRelativeGeometry();
	}
	return QRectF();
}

//virtual
Page *	LauncherObject::testForIntersectOnPageTab(const QPointF& scenePosition,PageTab ** r_p_tabForPage)
{
	if (m_qp_pageTabBar)
	{
		return m_qp_pageTabBar->testForIntersectOnPageTab(scenePosition,r_p_tabForPage);
	}
	return 0;
}

bool LauncherObject::testForIntersectPageArea(const QPointF& scenePosition)
{
	return (classifyPageLocation(mapFromScene(scenePosition)) == LauncherAreas::PageInner);
}

qint32	LauncherObject::testForIntersectPageAreaAndPageActive(const QPointF& scenePosition)
{
	if  (classifyPageLocation(mapFromScene(scenePosition)) == LauncherAreas::PageInner)
	{
		return centerPageIndex();
	}
	return -1;
}

QRectF LauncherObject::areaQuickLaunchBar() const
{
	//TODO: SLOW: cache the quickLaunchArea locally
	return mapRectFromItem(m_qp_mainWindow,m_qp_mainWindow->quickLaunchArea());
//	QPointF position = mapFromItem(m_qp_mainWindow,m_qp_mainWindow->quickLaunchPosition());
//	return Quicklauncher::primaryInstance()->geometry().translated(position);
}

QRectF	LauncherObject::areaPageLeftSide(Page * pPage) const
{
	if (!pPage)
	{
		//use the default version
		return m_currentPageLeftBorderArea;
	}

	return mapRectFromItem(pPage,pPage->areaLeftBorder());
}

QRectF	LauncherObject::areaPageRightSide(Page * pPage) const
{
	if (!pPage)
	{
		//use the default version
		return m_currentPageRightBorderArea;
	}
	return mapRectFromItem(pPage,pPage->areaRightBorder());
}

//virtual
LauncherObject::~LauncherObject()
{
	//qDebug() << "IEEEEEEEEEEE!!!!!!!!";
}

#include <QPen>
#include <QPainter>
#include <QLinearGradient>

//virtual
void LauncherObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	if (m_drawBackground)
	{
		paintBackground(painter);
	}
//	QPen sp = painter->pen();
//	painter->setPen(Qt::yellow);
//	painter->drawEllipse(QPoint(0,0),2,2);
//	painter->setPen(sp);
}

//virtual
void LauncherObject::paintOffscreen(QPainter *painter)
{

}

void LauncherObject::paintBackground(QPainter * painter)
{
	//not sure if this is necessary
	QPoint sbo = painter->brushOrigin();
	painter->setBrushOrigin(m_geom.topLeft());
	painter->fillRect(m_geom, QBrush(*(*m_qp_background)));
	painter->setBrushOrigin(sbo);
}

//void LauncherObject::paintBackground(QPainter * painter)
//{
	//THE DEBUG VERSION!
//	painter->fillRect(m_geom,QBrush(Qt::yellow));
//}

//virtual
void LauncherObject::touchTrackedPointMoved(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{

}

//virtual
void LauncherObject::touchTrackedPointReleased(int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition)
{

}

//virtual
void LauncherObject::redirectedTouchTrackedPointMoved(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
//	qDebug() << __PRETTY_FUNCTION__ << ": getting redirected id " << id << " from Thing: " << p_sourceThing;

	if (!qobject_cast<LauncherTouchRedirectContext const *>(&redirContext))
	{
		qDebug() << __FUNCTION__ << ": error: not properly redirected; the context object isn't the right type";
		return; //not properly redirected
	}

	LauncherPageHPanRedirectContext const * pPanCtx = qobject_cast<LauncherPageHPanRedirectContext const *>(&redirContext);
	if (pPanCtx)
	{
		//TODO: OPTIMIZE: consider making the local touch point tracking take the event's ICS instead of the scene, and
		// leave the "redirect" versions as scene....this should avoid the extra mapping here back to ICS
		QPointF localAmountMoved = mapFromScene(scenePosition) - mapFromScene(lastScenePosition);

		qreal moveByX = localAmountMoved.x();
		//if there is a page centered, deactivate it
		deactivateCenterPage();

		m_touchStartPageIndex = qobject_cast<Page *>(p_sourceThing)->pageIndex();

		if(isAnimationEnsembleRunning()) {
			//there can't be any unstoppable launcher animations happening; it would prevent the panning
			if (!canStopAnimationEnsemble())
			{
				//nope...cancel the redirect and get out
	//			pSourcePage->cancelRedirection(id);
				return;
			}
			stopAnimationEnsemble();
			m_seenHorizPanFlick = false;
		}

		if(isPageAnchorInOverscroll())
			moveByX /= 2;

		m_horizPanAnchor.moveBy(moveByX,0.0);
		return;
	}
	LauncherPageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<LauncherPageIconTransferRedirectContext const *>(&redirContext);
	if (pIconTxferCtx)
	{
//		qDebug() << __PRETTY_FUNCTION__ << ": Icon transfer to the "
//				<< LauncherPageIconTransferRedirectContext::dbgDirectionToString((pIconTxferCtx->m_direction));
		//the redirector should be a page -GEMSTONE-RD: a reorderable page to be exact
		ReorderablePage * pSourcePage = qobject_cast<ReorderablePage *>(pIconTxferCtx->m_qp_srcPage);
		if (!pSourcePage)
		{
			qDebug() << __PRETTY_FUNCTION__ << ": no source page within the context obj!?";
			return;
		}

		if (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::INVALID)
		{
			//balk at it! bad caller!!!
			return;
		}
		if ( (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::Left)
				|| (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::Right))
		{

			//IF THE ICON IS IN LIMBO, THEN TAKE IT TO THE LIMBO HANDLERS!!!
			if (iconLimboContains(pIconTxferCtx->m_qp_icon))
			{
//				qDebug() << __FUNCTION__ << ": going to the Limbo handler for Moved";
				return redirectedTTPMovedLimbo(p_sourceThing,id,scenePosition,lastScenePosition,initialPosition,redirContext);
			}

			//else give it to the launcher
			if (!this->offer((Thing *)(pIconTxferCtx->m_qp_icon.data()),(Thing *)(pIconTxferCtx->m_qp_icon->owningPage())))
			{
				//I (this/the launcher) refused the offer or the take failed
				qDebug() << __FUNCTION__ << ": Launcher REFUSED the icon offer!";
				pSourcePage->cancelRedirection(id);
				return;
			}

			//if  offer was successful, then the launcher already accepted the icon and assimilated it
			qDebug() << __FUNCTION__ << ": Launcher ACCEPTED the icon offer!";
		}
		else if (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::QuickLaunch)
		{
//			//ask the quick launch if it can accept icons
//			if (Quicklauncher::primaryInstance()->quickLaunchBar()->offer(pIconTxferCtx->m_qp_icon,pSourcePage) == false)
//			{
//				//quick launch doesn't want it
//				return;
//			}
//			//TODO: FINISHME
		}
		else if (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::TabBar)
		{
			//check the Tab Bar to see if any Tab is hit
			if (!m_qp_pageTabBar)
			{
				qDebug() << __FUNCTION__ << ": no tab bar!?";
				//no tab bar!?
				return;
			}

			//IF THE ICON IS IN LIMBO, THEN TAKE IT TO THE LIMBO HANDLERS!!!
			if (iconLimboContains(pIconTxferCtx->m_qp_icon))
			{
//				qDebug() << __FUNCTION__ << ": going to the Limbo handler for Moved";
				return redirectedTTPMovedLimbo(p_sourceThing,id,scenePosition,lastScenePosition,initialPosition,redirContext);
			}

			//give it to the launcher
			if (!this->offer((Thing *)(pIconTxferCtx->m_qp_icon.data()),(Thing *)(pIconTxferCtx->m_qp_icon->owningPage())))
			{
				//I (this/the launcher) refused the offer or the take failed
				qDebug() << __FUNCTION__ << ": Launcher REFUSED the icon offer!";
				pSourcePage->cancelRedirection(id);
				return;
			}

			//if  offer was successful, then the launcher already accepted the icon and assimilated it
			qDebug() << __FUNCTION__ << ": Launcher ACCEPTED the icon offer!";
			///TODO: SLOW: this can be improved... The page that redirected here would have already tested for the tab bar position that was hit
			//		(or something in the chain that led here would have -- it's necessary to avoid pointless redirections and redirection loops)
			//		so, rather than do it again here, the Tab's info (that was hit) could be embedded in the context
			Page * pDestPage = m_qp_pageTabBar->testForIntersectOnPageTab(scenePosition,true);
			if (pDestPage)
			{
				qDebug() << __FUNCTION__ << ": hit the tab for page " << pDestPage->pageIndex();

				if (!canStopAnimationEnsemble())
				{
					//just bail out...the TTPMovedLimbo handler will retry this
					return;
				}
				//now make the panning happen
				stopAnimationEnsemble();
				gotoPageIndex(pDestPage->pageIndex());
			}
		}
	}
}

//virtual
void LauncherObject::redirectedTTPMovedLimbo(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	if (!qobject_cast<LauncherTouchRedirectContext const *>(&redirContext))
	{
		qDebug() << __FUNCTION__ << ": error: not properly redirected; the context object isn't the right type";
		return; //not properly redirected
	}

	LauncherPageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<LauncherPageIconTransferRedirectContext const *>(&redirContext);
	if (pIconTxferCtx)
	{
//		qDebug() << __PRETTY_FUNCTION__ << ": Icon transfer to the "
//				<< LauncherPageIconTransferRedirectContext::dbgDirectionToString((pIconTxferCtx->m_direction));
		//the redirector should be a page -GEMSTONE-RD: a reorderable page to be exact
		ReorderablePage * pSourcePage = qobject_cast<ReorderablePage *>(pIconTxferCtx->m_qp_srcPage);
		if (!pSourcePage)
		{
			qDebug() << __PRETTY_FUNCTION__ << ": no source page within the context obj!?";
			return;
		}

		if (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::INVALID)
		{
			//balk at it! bad caller!!!
			return;
		}
		if ( (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::Left)
				|| (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::Right))
		{

			//figure out where this icon currently is located while being moved. Two basic possibilities:
			// 1. The launcher JUST took this icon into Limbo (took ownership of it)
			//		This means that the icon is on the left or right side of the Page, which triggered this take-ing. Therefore, some kind of panning
			//		will need to take place to move the page. Also, some kind of limiting needs to take place to prevent movement again once the new page is completely panned in
			//		(because the icon at that point will be on the "far side" of the page again)
			// 2. The icon has been moving for some time after the initial panning
			//		If the icon is still on the side of the page, then it should try and pan again
			//		If the icon is now inside the page inner space, then the icon needs to be transferred to the page
			//
			// 	Unhandled cases: move to quicklaunch after panning at least once
			//						move to tabbar after panning at least once (4-22-2011)

			QPointF launcherCoordinate = mapFromScene(scenePosition);
			Page * pDestPage = 0;
			qint32 activePageIndex = 0;
			switch (classifyPageLocation(launcherCoordinate))
			{
			case LauncherAreas::PageInner:
				//ok time to give it to the page
				// but only if there is a page in the center
				activePageIndex = centerPageIndex();
				if (activePageIndex >= 0)
				{
					pDestPage = m_pages[activePageIndex];
				}
				Q_EMIT	signalPageMovementEnd();
				break;
			case LauncherAreas::PageLeftSide:
				activePageIndex = centerPageIndex();
				if (activePageIndex > 0)		//this also implies static state (no animation, i.e. panning already)
				{
					//Pan left (again)...if allowed
					if (!(m_p_pageMovementController->isLeftPageMoveRestricted()))
					{
						if (gotoPageIndex(activePageIndex-1))
						{
							Q_EMIT signalPagePanLeft();
						}
					}
				}
				return;
			case LauncherAreas::PageRightSide:
				activePageIndex = centerPageIndex();
				if (activePageIndex >= 0)		//this also implies static state (no animation, i.e. panning already)
				{
					//Pan right (again)...if allowed
					if (!(m_p_pageMovementController->isRightPageMoveRestricted()))
					{
						qDebug() << "PageRightSide to index " << (activePageIndex+1);
						if (gotoPageIndex(activePageIndex+1))		//gotoPageIndex takes care of index > last page index
						{
							Q_EMIT signalPagePanRight();
						}
					}
				}
				return;
			default:
				return;			//UNHANDLED
			}
			if (!pDestPage)
			{
				//can't
				return;
			}
			if (!pDestPage->canAcceptIcons())
			{
				//nope...just keep it floating
				return; //nope
			}

			//ok, all set to go transfer...

			//this can all be done now via the offer()-take()... sequence. Just offer the icon to the new page
			setIconTxContextMovedOntoPage(pIconTxferCtx->m_qp_icon);
			if (!pDestPage->offer((Thing *)(pIconTxferCtx->m_qp_icon.data()),this))
			{
				//the destination refused the offer or the take failed
				clearIconTxContext(pIconTxferCtx->m_qp_icon);
				pSourcePage->cancelRedirection(id);
				return;
			}
			clearIconTxContext(pIconTxferCtx->m_qp_icon);

			//if offer was successful, then it already accepted the icon and assimilated it

			//change the redirection to the target page - MUST be done right now to avoid duplicate redirects in here.
			// that would be bad since this if-branch would then call acceptIncomingIcon multiple times for the same icon
			pSourcePage->changeRedirectTo(id,this,pDestPage,new PageIconTransferRedirectContext(pSourcePage,pIconTxferCtx->m_qp_icon));
		}
		else if (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::QuickLaunch)
		{
			//TODO: IMPLEMENT ... if we choose to switch to the "Limbo Dance" model for QL transfers too
			// (right now, the experimental case is just for the tab bar)
		}
		else if (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::TabBar)
		{

			QPointF pageCoordinate = mapFromScene(scenePosition);
			pIconTxferCtx->m_qp_icon->setPos(pageCoordinate);

			//check the Tab Bar to see if any Tab is hit
			if (!m_qp_pageTabBar)
			{
				qDebug() << __FUNCTION__ << ": no tab bar!?";
				//no tab bar!?
				return;
			}

			Page * pDestPage = m_qp_pageTabBar->testForIntersectOnPageTab(scenePosition,true);
			if (pDestPage)
			{
//				qDebug() << __FUNCTION__ << ": hit the tab for page " << pDestPage->pageIndex();

				if (!canStopAnimationEnsemble())
				{
					//just bail out...the TTPMovedLimbo handler (i.e. this thing) will retry this
					return;
				}
				//now make the panning happen
				stopAnimationEnsemble();
				gotoPageIndex(pDestPage->pageIndex());
			}
			else
			{
				///if the motion is back in the page area, then give it to the page
				qint32 activePageIndex = testForIntersectPageAreaAndPageActive(scenePosition);
				if (activePageIndex < 0)
				{
					//nope..
					return;
				}

				//ok, give it to the center/active page
				pDestPage = m_pages[activePageIndex];
				//should never be NULL! !!!
				if (!pDestPage->canAcceptIcons())
				{
					return; //nope
				}
				//ok, all set to go transfer...

				//this can all be done now via the offer()-take()... sequence. Just offer the icon to the new page on behalf of the
				// source page
				LauncherObject::setIconTxContextMovedOntoPage(pIconTxferCtx->m_qp_icon);
				if (!pDestPage->offer((Thing *)(pIconTxferCtx->m_qp_icon.data()),this))
				{
					//the destination refused the offer or the take failed
					clearIconTxContext(pIconTxferCtx->m_qp_icon);
					qDebug() << __FUNCTION__ << ": page " << pDestPage->pageIndex() << " REFUSED the icon offer!";
					return;
				}
				clearIconTxContext(pIconTxferCtx->m_qp_icon);
				//if offer was successful, then it already accepted the icon and assimilated it
//				qDebug() << __FUNCTION__ << ": page " << pDestPage->pageIndex() << " ACCEPTED the icon offer!";

				//change the redirection to the target page - MUST be done right now to avoid duplicate redirects in here.
				// that would be bad since this if-branch would then call acceptIncomingIcon multiple times for the same icon
				pSourcePage->changeRedirectTo(id,this,pDestPage,new PageIconTransferRedirectContext(pSourcePage,pIconTxferCtx->m_qp_icon));
			}
		}
	}
}

//virtual
void LauncherObject::redirectedTouchTrackedPointReleased(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	qDebug() << __PRETTY_FUNCTION__ << ": getting redirected id " << id << " from Thing: " << p_sourceThing;

	//unhighlight all tabs
	if (m_qp_pageTabBar)
	{
		m_qp_pageTabBar->slotUnHighlightAll();
	}

	m_touchStartPageIndex = -1;

	//TODO: handle other scenarios.
	// At the moment, only Page-s redirect here

	if (!qobject_cast<LauncherTouchRedirectContext const *>(&redirContext))
	{
		return; //not properly redirected
	}
	LauncherPageHPanRedirectContext const * pPanCtx = qobject_cast<LauncherPageHPanRedirectContext const *>(&redirContext);
	if (pPanCtx)
	{
		if (m_pages.empty())
		{
			return;
		}

		if(m_seenHorizPanFlick) // we received a flick, so ignore this release
			return;

		//if some other animation is going on and can't be stopped, then bail
		if (!stopAnimationEnsemble())
		{
			//couldn't stop the running animation. So just abort this flick then. It must be an important anim and
			//un-interruptible
			return;
		}
		QPointF anchorPos = m_horizPanAnchor.pos();
		//lock to a deterministic pixel position
		anchorPos = QPointF(DimensionsGlobal::realAsPixelPosition(anchorPos.x()),DimensionsGlobal::realAsPixelPosition(anchorPos.y()));
		m_horizPanAnchor.setPos(anchorPos);
		qint32 mv = 10000;
		quint32 idx;
		for (int i=0;i<m_pages.size();++i)
		{
			qint32 cv = pageHorizontalOffsetToCenter(i);
			//qDebug() << "page " << i << " is " << cv << " from 0";
			if (qAbs(cv) > qAbs(mv))
				continue;
			mv = cv;
			idx = i;
		}
		if (qAbs(mv) > 0)
		{
			QPropertyAnimation * p_moveAnim = new QPropertyAnimation(&m_horizPanAnchor,"pos");
			p_moveAnim->setEndValue(anchorPos+QPointF((qreal)mv,0.0));
			p_moveAnim->setDuration(DynamicsSettings::DiUiDynamics()->snapbackAnimTime);
			p_moveAnim->setEasingCurve(DynamicsSettings::DiUiDynamics()->snapbackAnimCurve);
			(void)m_horizPanAnchor.setAnimation(p_moveAnim);
			slotAddAnimationToEnsemble(p_moveAnim,DimensionsTypes::AnimationType::HPan);
			slotStartAnimationEnsemble(false);
		}
		return;
	} ///////// end if redirection was due to a user controlled panning L/R between pages

	LauncherPageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<LauncherPageIconTransferRedirectContext const *>(&redirContext);
	if (pIconTxferCtx)
	{
		ReorderablePage * pSourcePage = qobject_cast<ReorderablePage *>(pIconTxferCtx->m_qp_srcPage);
		if (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::TabBar)
		{
			//IF THE ICON IS IN LIMBO, THEN TAKE IT TO THE LIMBO HANDLERS!!!
			if (iconLimboContains(pIconTxferCtx->m_qp_icon))
			{
				qDebug() << __FUNCTION__ << ": [Tabbar transfer] going to the Limbo handler for Released";
				return redirectedTTPReleasedLimbo(p_sourceThing,id,scenePosition,lastScenePosition,initialPosition,redirContext);
			}

			qDebug() << __FUNCTION__ << ": FIRST ????? not in limbo...which should be an impossible case for transfers to the tab bar";

			if (!pSourcePage)
			{
				qDebug() << __PRETTY_FUNCTION__ << ": no source page within the context obj!?";
				return;
			}
			clearIconTxContext(pIconTxferCtx->m_qp_icon);
			pSourcePage->cancelRedirection(id,true);			//use a deferred cancel or else the redirContext gets deleted and a crash when this returns
			return;
		}
		else
		if ( (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::Left)
						|| (pIconTxferCtx->m_direction == LauncherPageIconTransferRedirectContext::Right))
		{
			//went to another page and was released there
			//IF THE ICON IS IN LIMBO, THEN TAKE IT TO THE LIMBO HANDLERS!!!
			if (iconLimboContains(pIconTxferCtx->m_qp_icon))
			{
				qDebug() << __FUNCTION__ << ": [Page-to-Page transfer] going to the Limbo handler for Released";
				return redirectedTTPReleasedLimbo(p_sourceThing,id,scenePosition,lastScenePosition,initialPosition,redirContext);
			}
		}

		qDebug() << __FUNCTION__ << ": SECOND ????? not in limbo...which should be an impossible case for transfers to another page";
		if (!pSourcePage)
		{
			qDebug() << __PRETTY_FUNCTION__ << ": no source page within the context obj!?";
			return;
		}
		clearIconTxContext(pIconTxferCtx->m_qp_icon);
		pSourcePage->cancelRedirection(id,true);			//use a deferred cancel or else the redirContext gets deleted and a crash when this returns

		return; /// ????? not in limbo...which should be an impossible case for transfers to another page
	}
}

//virtual
void LauncherObject::redirectedTTPReleasedLimbo(Thing * p_sourceThing,int id,const QPointF& scenePosition,const QPointF& lastScenePosition,const QPointF& initialPosition,const RedirectContext& redirContext)
{
	LauncherPageIconTransferRedirectContext const * pIconTxferCtx = qobject_cast<LauncherPageIconTransferRedirectContext const *>(&redirContext);
	if (!pIconTxferCtx)
	{
		qDebug() << __FUNCTION__ << ": not a LauncherPageIconTransferRedirectContext...ignoring.";
		return;
	}
	//give the icon to the active page
	qint32 activePageIndex = centerPageIndex();
	if (activePageIndex < 0)
	{
		//no center page!? argh!  ok, try the LAST page that the icon was on
		bool propFound=false;
		activePageIndex = pIconTxferCtx->m_qp_icon->property(IconBase::IconLastPageVisitedIndexPropertyName).toInt(&propFound);
		if (!propFound)
		{
			qDebug() << __FUNCTION__ << ": no active page, no LAST page registered in the icon property!...last resort: sending to page 0 so the icon won't be lost!";
			activePageIndex = 0;
		}
	}

	//ok, give it the page....
	Page * pDestPage = m_pages[activePageIndex];
	//should never be NULL! !!!
	if (!pDestPage->canAcceptIcons())
	{
		return; //nope
		///TODO: icon is now going to be lost!!!  handle this better

	}
	//ok, all set to go transfer...this is a DROP onto the page

	//this can all be done now via the offer()-take()... sequence. Just offer the icon to the new page
	setIconTxContextDroppedOnTab(pIconTxferCtx->m_qp_icon);
	if (!pDestPage->offer((Thing *)(pIconTxferCtx->m_qp_icon.data()),this))
	{
		//the destination refused the offer or the take failed
		clearIconTxContext(pIconTxferCtx->m_qp_icon);
		qDebug() << __FUNCTION__ << ": page " << pDestPage->pageIndex() << " REFUSED the icon offer!";
		///TODO: icon is now going to be lost!!!  handle this better
		return;
	}
	clearIconTxContext(pIconTxferCtx->m_qp_icon);
	//if offer was successful, then it already accepted the icon and assimilated it
	qDebug() << __FUNCTION__ << ": page " << pDestPage->pageIndex() << " ACCEPTED the icon offer!";

}

bool LauncherObject::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {

		if (!stopAnimationEnsemble())
		{
			//can't stop the animation, so bail here - SILENTLY CONSUME EVENT.
			// I don't want anyone else handling the event. The user will retry is shortly when the anim completes
			return true;
		}

		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture * g = 0;
//		g = ge->gesture(Qt::TapGesture);
//		if (g) {
//			QTapGesture* tap = static_cast<QTapGesture*>(g);
//			if (tap->state() == Qt::GestureFinished) {
//				return tapGesture(tap,ge);
//			}
//		}
//		g = ge->gesture(Qt::TapAndHoldGesture);
//		if (g) {
//			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
//			if (hold->state() == Qt::GestureFinished) {
//				return tapAndHoldGesture(hold,ge);
//			}
//		}
		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g) {
			FlickGesture* flick = static_cast<FlickGesture*>(g);
			if (flick->state() == Qt::GestureFinished) {
				return flickGesture(flick,ge);
			}
		}
	}
	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool LauncherObject::event(QEvent * e)
{
	if (e->type() == IconCmdRequestEvent::s_eventType)
	{
//		g_message("[ICON-TAP-TRACE] %s: IconCmdRequestEvent arrived",__FUNCTION__);
		IconCmdRequestEvent * pCmdEvent = static_cast<IconCmdRequestEvent *>(e);
		if (pCmdEvent->m_qp_icon)
		{
//			g_message("[ICON-TAP-TRACE] %s: IconCmdRequestEvent invoking slotIconActionRequest() with cmdRequest %d and icon? %s ",__FUNCTION__,
//					pCmdEvent->m_cmdRequest,(pCmdEvent->m_qp_icon == 0 ? "NULL" : "NON-NULL" ));
			slotIconActionRequest(pCmdEvent->m_cmdRequest,pCmdEvent->m_qp_icon);
		}
		else
		{
			g_message("[ICON-TAP-TRACE] %s: IconCmdRequestEvent arrived - but there is NO ICON!",__FUNCTION__);
		}
		return true;
	}
	return QObject::event(e);
}

//virtual
bool LauncherObject::flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent)
{
	//if page interactions are blocked, then block this too
	if (m_blockingPageInteractions)
	{
		return true;
	}

	m_seenHorizPanFlick = true;

	if (flickEvent->velocity().x() > 0)
	{
		(void)gotoLeftPage(flickEvent->velocity().x());
	}
	else
	{
		(void)gotoRightPage(flickEvent->velocity().x());
	}

	return true;
}

//virtual
void LauncherObject::resize(int w, int h)
{

//MDK-LAUNCHER	CardLayout::resize(w,h);		//fake it!

	QSize newSize = LauncherSizeFromScreenSize(w,h);
	bool fsInit = false;

	ThingPaintable::resize(w,h);
	if (!m_fsInitHasRun)
	{
		fullSizeInit((quint32)w,(quint32)h);
		fsInit = true;
	}
	else
	{
		QSizeF rawNewSizeF = QSizeF(newSize);
		//RESIZING
		m_geom = DimensionsGlobal::realRectAroundRealPoint(rawNewSizeF);
		//qDebug() << __FUNCTION__ << ": LauncherObject object resizing to geometry: " << m_geom;

		QSize pageTabBarSize = PageTabBar::PageTabSizeFromLauncherSize((quint32)m_geom.width(),(quint32)m_geom.height());
		if (m_qp_pageTabBar)
		{
			//resize the tab bar - THIS WILL PROPAGATE TO ALL SUB-OBJECTS (i.e TABS) AND COME BACK UP
			m_qp_pageTabBar->resize(pageTabBarSize);
		}
		//reposition the tab bar
		m_qp_pageTabBar->setPos(0.0,m_geom.top()-m_qp_pageTabBar->geometry().top());

		//reposition the Done button
		if (m_qp_doneButton)
		{
			m_qp_doneButton->setPos(QPoint(m_qp_pageTabBar->geometry().right()-m_qp_doneButton->geometry().width(),0.0)
											+LayoutSettings::settings()->doneButtonPositionAdjust);
		}
		//compute the new page size from new ui size
		QRectF quickLaunchArea;
		if (LayoutSettings::settings()->autoPageSize)
		{
			//get the area of the quicklaunch bar
			if (m_qp_mainWindow)
			{
				quickLaunchArea = m_qp_mainWindow->quickLaunchArea();
			}
			QSize r = QSize(DimensionsGlobal::roundDown(m_geom.width()),
					qMax(2,DimensionsGlobal::roundDown(m_geom.bottom()
														-m_qp_pageTabBar->positionRelativeGeometry().bottom()
														-quickLaunchArea.height()-1.0)));
			//make evenly divisible (multiple of 2)
			r.setWidth(r.width() - (r.width() % 2));
			r.setHeight(r.height() - (r.height() % 2));
			m_currentPageSize = r;
		}
		else
		{
			m_currentPageSize = Page::PageSizeFromLauncherSize((quint32)m_geom.width(),(quint32)m_geom.height());
		}

		//re-pre-compute geom  =)
		m_currentPageGeom = DimensionsGlobal::realRectAroundRealPoint(m_currentPageSize);
		m_currentPagePos = QPointF(0.0,m_qp_pageTabBar->positionRelativeGeometry().bottom()-m_currentPageGeom.top());

		//resize all the pages...this will propagate down to layouts and icons, and whatever else is in a page, and come back up
		qint32 pageIndex =0;
		qint32 centerIndex = centerPageIndex();
		if (centerIndex == -1)
		{
			centerIndex = 0;
		}

		m_horizPanAnchor.setPos(0.0,0.0);
		Page * pCenterPage = m_pages[centerIndex];
		for (QList<QPointer<Page> >::iterator it = m_pages.begin();
				it != m_pages.end();++it)
		{
			(*it)->resize((quint32)m_currentPageSize.width(),(quint32)m_currentPageSize.height());
			//reposition the page correctly
			(*it)->setPos(pageIndex*(*it)->geometry().width(),0.0);
			++pageIndex;
		}
		//(leave the ones in the page limbo alone...they'll resize when/if they join the list)

		m_horizPanAnchor.setPos(-centerIndex*pCenterPage->geometry().width(),
									LayoutSettings::settings()->centerUiVerticalOffset + m_currentPagePos.y());

		//TODO: Any other main Ui level Things that need to resize -- do that right here
		if (m_qp_overlay)
		{
			m_qp_overlay->resize(QSize(w,h));
		}

		{
			int borderWidth = LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.width();
			if (borderWidth > m_currentPageGeom.width()/2)
			{
				borderWidth = m_currentPageGeom.width()/2;
			}
			m_currentPageLeftBorderArea =  QRectF(m_currentPageGeom.topLeft(),QSize(borderWidth,m_currentPageGeom.size().toSize().height()));
		}
		{
			int borderWidth = LayoutSettings::settings()->pageHorizontalBorderActivationAreaSizePx.height();		//it's not actually "height". see layoutsettings.cpp
			if (borderWidth > m_currentPageGeom.width()/2)
			{
				borderWidth = m_currentPageGeom.width()/2;
			}

			m_currentPageRightBorderArea = QRectF(m_currentPageGeom.topRight()-QPointF((qreal)borderWidth,0.0),
					QSize(borderWidth,m_currentPageGeom.size().toSize().height()));
		}

	}

	Q_EMIT signalGeometryChange();
	if (fsInit)
	{
		m_dbg_pageAddTriggered = true;
		m_fsInitHasRun = true;
		initPages();
		createTabsForAllPages();
		setStartupPage();
		activateCenterPage();
		saveCurrentLauncherLayouts();
		Q_EMIT signalReady();
	}
	update();
}

qint32 LauncherObject::centerPageIndex() const		//-1 if nothing is currently center or the ui is not currently static (i.e. it's animating pages)
{
	/*
	 *
	 * new idea!  just try using a mapping from THIS -> the card to be centered.
	 * Find it's point in THIS-space and compare to 0
	 *
	 */

	//SLOW
	//do a slow linear search for now

	int i=0;
	for (;i<m_pages.size();++i)
	{
//		qDebug() << "mapped position of page " << i << " is " << mapFromItem(&m_horizPanAnchor,m_pages.at(i)->pos()).x();
		if (qFuzzyCompare((float)(mapFromItem(&m_horizPanAnchor,m_pages.at(i)->pos()).x()+1.0),(float)1.0))
			break;
	}
	if (i==m_pages.size())
		return -1;
	return i;
}

qint32 LauncherObject::pageHorizontalOffsetToCenter(quint32 pageIndex) const
{
	// this function does not expect the ui to be static, unlike centerPageIndex
	if (pageIndex >= (quint32)m_pages.size())
		return 0.0;

	qint32 xp = DimensionsGlobal::realAsPixelSize(-(mapFromItem(&m_horizPanAnchor,m_pages.at(pageIndex)->pos()).x()));
	return xp;
}


Page * LauncherObject::currentPage() const
{
	if (m_pages.size() <= 0)
	{
		return 0;
	}

	int center = centerPageIndex();

	if (center < 0 || center >= m_pages.size())
		return 0;

	return m_pages[center];
}

//TODO: this might be better, faster with pageindex property, which is theoretically sync-maintained
Page * LauncherObject::pageLeft(Page * p_fromPage) const
{
	if (!p_fromPage)
	{
		return 0;
	}

	Page * pLeftPage = 0;
	for (QList<QPointer<Page> >::const_iterator it = m_pages.constBegin();
			it != m_pages.constEnd();pLeftPage=*it,++it)
	{
		if (*it == p_fromPage)
		{
			break;
		}
	}
	return pLeftPage;
}

Page * LauncherObject::pageRight(Page * p_fromPage) const
{
	if (!p_fromPage)
	{
		return 0;
	}

	QList<QPointer<Page> >::const_iterator it = m_pages.constBegin();
	for (;it != m_pages.constEnd();++it)
	{
		if (*it == p_fromPage)
		{
			++it;
			break;
		}
	}
	if (it == m_pages.constEnd())
	{
		return 0;
	}
	return *it;
}

bool LauncherObject::checkAllowedIconTransferInterPage(Page * p_srcPage,Page * p_destPage,IconBase * p_icon)
{
	ReorderablePage * pSrc = qobject_cast<ReorderablePage *>(p_srcPage);
	ReorderablePage * pDst = qobject_cast<ReorderablePage *>(p_destPage);
	if (!pSrc || !pDst || !p_icon)
	{
		return false;
	}

	//TODO: should check for possibility of icon duplicate on the dest page already, but given the GEMSTONE-RD assumptions
	//	this shouldn't be possible from page-to-page transfers
	return true;
}

qint32 LauncherObject::closestToCenterPageIndex()	const 	//-1 if no pages
{
	//SLOW
	// this function does not expect the ui to be static, unlike centerPageIndex
	if (m_pages.empty())
		return -1;
	qint32 mv = INT_MAX;
	quint32 idx = 0;
	for (int i=0;i<m_pages.size();++i)
	{
		qint32 cv = pageHorizontalOffsetToCenter(i);
		//qDebug() << "page " << i << " is " << cv << " from 0";
		if (qAbs(cv) > qAbs(mv))
			continue;
		mv = cv;
		idx = i;
	}
	return idx;
}

qint32 LauncherObject::closestToCenterDistance() const		// 0.0 if no pages
{

	//SLOW
	// this function does not expect the ui to be static, unlike centerPageIndex
	if (m_pages.empty())
		return 0;
	qint32 mv = INT_MAX;
	quint32 idx;
	for (int i=0;i<m_pages.size();++i)
	{
		qint32 cv = pageHorizontalOffsetToCenter(i);
		//qDebug() << "page " << i << " is " << cv << " from 0";
		if (qAbs(cv) > qAbs(mv))
			continue;
		mv = cv;
		idx = i;
	}
	return mv;

}

qint32 LauncherObject::closestToCenter(qint32& r_dist) const		//-1 if no pages, r_dist unspecified
{

	//SLOW
	// this function does not expect the ui to be static, unlike centerPageIndex
	if (m_pages.empty())
		return -1;
	r_dist = INT_MAX;
	quint32 idx = 0;
	for (int i=0;i<m_pages.size();++i)
	{
		qint32 cv = pageHorizontalOffsetToCenter(i);
		//qDebug() << "page " << i << " is " << cv << " from 0";
		if (qAbs(cv) > qAbs(r_dist))
			continue;
		r_dist = cv;
		idx = i;
	}
	return idx;
}

quint32 LauncherObject::numPages() const
{
	return (quint32)m_pages.size();
}

quint32 LauncherObject::horizontalLengthOfPageTrain() const
{
	//TODO: IMPLEMENT / unimplemened
	return 0;
}

//static
QSize LauncherObject::LauncherSizeFromScreenSize(quint32 width,quint32 height)
{

	QSize r = QSize(
				qBound((quint32)2,
						(quint32)DimensionsGlobal::roundDown((qreal)width * LayoutSettings::settings()->launcherSizePctScreenRelative.width()),
						(quint32)width),
				qBound((quint32)2,
						(quint32)DimensionsGlobal::roundDown((qreal)height * LayoutSettings::settings()->launcherSizePctScreenRelative.height()),
						(quint32)height)
	);

	//make evenly divisible (multiple of 2)
	r.setWidth(r.width() - (r.width() % 2));
	r.setHeight(r.height() - (r.height() % 2));
	return r;
}

void LauncherObject::slotAppScanCompleted(bool initialScan)
{
	//TODO: IMPLEMENT
	/*
	 * This should kick off a page creation routine, if pages have not already been created.
	 * Since conceivably, other things besides the primary AppMonitor instance can call this,
	 * it should be able to deal with those sources too; at the very least, consider sender() and don't just
	 * blindly use AppMonitor::appMonitor()
	 *
	 */

}

void LauncherObject::slotAppPreRemove(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin)
{

	//TODO: ICON-MANAGE: memleaks...deleting icons from the heap is not completely implemented in terms of releasing icon resources

	//prior to the appmonitor deleting all the app structures
	//get the main icon by the app uid

	QUuid mainIconUid = iconUidFromAppUid(eapp.uid());
	if (mainIconUid.isNull())
	{
		//ugh, problem!
		g_warning("%s: early-exit, uid references null main icon",__FUNCTION__);
		return;
	}
	//find all copies
	QList<IconBase *> iconListForApp = IconHeap::iconHeap()->findCopies(mainIconUid);
	//and retrieve the main icon's actual pointer
	IconBase * pMainIcon = IconHeap::iconHeap()->getIcon(mainIconUid);

	//iterate over the icon copies list and remove all the icons from the pages
	for (QList<IconBase *>::iterator it = iconListForApp.begin();
			it != iconListForApp.end();++it)
	{
		if ((*it) == 0)
		{
			continue;	//should never happen
		}
		g_warning("%s: removing AND deleting a copy with uid [%s]",__FUNCTION__,qPrintable((*it)->uid().toString()));
		removeIconFromPages(*it);
		removeIconUidFromMaps((*it)->uid());
		IconHeap::iconHeap()->deleteIconCopy((*it)->uid());
	}


	if (pMainIcon)
	{
		g_warning("%s: removing main icon with uid [%s]",__FUNCTION__,qPrintable(pMainIcon->uid().toString()));
		removeIconFromPages(pMainIcon);
		removeIconUidFromMaps(pMainIcon->uid());
	}

	//Now, if the app is a WebOSApp, then there could be other launchpoints , other than the main one.
	// Must delete them all
	const DimensionsSystemInterface::WebOSApp * pWebOSapp = qobject_cast<const DimensionsSystemInterface::WebOSApp *>(&eapp);
	if (pWebOSapp)
	{
		QList<IconBase *> otherIcons = pWebOSapp->auxAppIcons();
		for (QList<IconBase *>::iterator it = otherIcons.begin();
				it != otherIcons.end();++it)
		{
			if (!(*it))
			{
				continue;
			}
			//get all the copies of this aux icon
			iconListForApp = IconHeap::iconHeap()->findCopies((*it)->uid());
			for (QList<IconBase *>::iterator cit = iconListForApp.begin();
					cit != iconListForApp.end();++cit)
			{
				if ((*cit) == 0)
				{
					continue;	//should never happen
				}
				removeIconFromPages(*cit);
				removeIconUidFromMaps((*cit)->uid());
			}
			//remove the original
			removeIconFromPages(*it);
			removeIconUidFromMaps((*it)->uid());
		}
	}

	g_warning("%s: deleting main icon with uid [%s]",__FUNCTION__,qPrintable(pMainIcon->uid().toString()));
	IconHeap::iconHeap()->deleteIconUnguarded(pMainIcon->uid());

	saveCurrentLauncherLayouts();
	update();
}

void LauncherObject::slotAppPostRemove(const QUuid& removedAppUid,DimensionsSystemInterface::AppMonitorSignalType::Enum origin)
{
	//called after appmonitor having deleting all the app structures

}

//this one will only work for WebOS apps
void LauncherObject::slotAppAuxiliaryIconRemove(const QUuid& appUid,const QString& launchpointId,DimensionsSystemInterface::AppMonitorSignalType::Enum origin)
{
	const DimensionsSystemInterface::WebOSApp * pWebOSapp =
			qobject_cast<const DimensionsSystemInterface::WebOSApp *>(DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(appUid));
	if (pWebOSapp)
	{
		//get the main (master) copy of the icon for this appId+launchpointId
		IconBase * pMasterIcon = IconHeap::iconHeap()->getIcon(pWebOSapp->appId(),launchpointId);
		if (pMasterIcon)
		{
			//get all the copies of this aux icon
			QList<IconBase *> iconListForApp = IconHeap::iconHeap()->findCopies(pMasterIcon->uid());
			for (QList<IconBase *>::iterator cit = iconListForApp.begin();
					cit != iconListForApp.end();++cit)
			{
				if ((*cit) == 0)
				{
					continue;	//should never happen
				}
				removeIconFromPages(*cit);
				removeIconUidFromMaps((*cit)->uid());
			}
			//remove the original
			removeIconFromPages(pMasterIcon);
			removeIconUidFromMaps(pMasterIcon->uid());
		}
	}

	///TODO: ICON-MANAGE: MEMLEAK:

	saveCurrentLauncherLayouts();
	update();
}

quint32 LauncherObject::pageIndexForApp(const DimensionsSystemInterface::ExternalApp& eapp)
{
	quint32 appPageIndex = 0;
	if (OperationalSettings::settings()->preferAppKeywordsForAppPlacement)
	{
		if (!pageIndexForAppByLoadedMappings(eapp,appPageIndex))
		{
			//failed...use the other way
			(void)pageIndexForAppByPredefinedDesignators(eapp,appPageIndex);
		}
	}
	else
	{
		if (!pageIndexForAppByPredefinedDesignators(eapp,appPageIndex))
		{
			//failed...use the other way
			(void)pageIndexForAppByLoadedMappings(eapp,appPageIndex);
		}
	}

	// don't really know what worked and what didn't so assure that the page index is correct
	if (appPageIndex >= (quint32)m_pages.size())
	{
		appPageIndex = 0;	//default to 0 (first page, which must exist)
	}

	const DimensionsSystemInterface::WebOSApp * pApp = qobject_cast<const DimensionsSystemInterface::WebOSApp *>(&eapp);
	if (pApp)
	{
		g_warning("[APP-MARSHALL] %s: App [%s] is supposed to go to page %u",__FUNCTION__,qPrintable(pApp->appId()),appPageIndex);
	}
	return appPageIndex;

}

bool LauncherObject::pageIndexForAppByLoadedMappings(const DimensionsSystemInterface::ExternalApp& eapp,quint32& appPageIndex)
{
	//support WebOSApp only at the moment
	const DimensionsSystemInterface::WebOSApp * pWebOSapp = qobject_cast<const DimensionsSystemInterface::WebOSApp *>(&eapp);
	if (!pWebOSapp)
	{
		return false;
	}

	QString pageDesignator = DimensionsSystemInterface::AppMonitor::appMonitor()->pageDesignatorForWebOSApp(pWebOSapp->appId());
	if (pageDesignator.isEmpty())
	{
		return false;
	}

	//look up the page index for it, based on the currently loaded pages
	Page * pPage = pageByDesignator(pageDesignator);
	if (pPage)
	{
		appPageIndex = pPage->pageIndex();
		return true;
	}
	return false;
}

bool LauncherObject::pageIndexForAppByPredefinedDesignators(const DimensionsSystemInterface::ExternalApp& eapp,quint32& appPageIndex)
{
	//support WebOSApp only at the moment
	const DimensionsSystemInterface::WebOSApp * pWebOSapp = qobject_cast<const DimensionsSystemInterface::WebOSApp *>(&eapp);
	if (!pWebOSapp)
	{
		return false;;
	}

	QPair<QString,quint32> loc = DimensionsSystemInterface::PageRestore::itemPositionAsStoredOnDisk(pWebOSapp->appId());
	if (!loc.first.isEmpty())
	{
		//try and find the page specified
		ReorderablePage * pPage = pageByUid(loc.first);
		if (pPage)
		{
			appPageIndex = (quint32)pPage->property(Page::PageIndexPropertyName).toInt();
			g_warning("--> [APP-MARSHALL] %s: appId %s (Previously-Saved) determined to go to page of index %u",
						__FUNCTION__,qPrintable(pWebOSapp->appId()),appPageIndex);
			return true;
		}
	}

	if (pWebOSapp->appId() == OperationalSettings::settings()->appCatalogAppId)
	{
		//this is the app catalog app, which goes on the user installed apps page
		//TODO: assure that it ends up in the FIRST COLUMN, FIRST ROW of that page
		appPageIndex = OperationalSettings::settings()->installedAppsPageIndex;
		g_warning("--> [APP-MARSHALL] %s: appId %s (AppCatalog) determined to go to the 'User' page of index %u",
					__FUNCTION__,qPrintable(pWebOSapp->appId()),appPageIndex);
	}
	//TODO: Don't know if this is enough to differentiate it as a system app and a settings app
	else if ((pWebOSapp->platformApp()) && !(OperationalSettings::settings()->settingsAppCategoryDesignator.isEmpty())
			&& (OperationalSettings::settings()->settingsAppCategoryDesignator == pWebOSapp->category()))
	{
		//settings app...goes on the settings page
		appPageIndex = OperationalSettings::settings()->settingsPageIndex;
		g_warning("--> [APP-MARSHALL] %s: appId %s determined to go to the 'Settings' page of index %u",
				__FUNCTION__,qPrintable(pWebOSapp->appId()),appPageIndex);
	}
	else if (pWebOSapp->userInstalledApp())
	{
		//user installed from app catalog....goes on user installed apps page
		appPageIndex = OperationalSettings::settings()->installedAppsPageIndex;
		g_warning("--> [APP-MARSHALL] %s: appId %s determined to go to the 'User' page of index %u",
						__FUNCTION__,qPrintable(pWebOSapp->appId()),appPageIndex);
	}
	//TODO: add more discriminators as desired
	else
	{
		//the rest go on the "system" apps page
		appPageIndex = OperationalSettings::settings()->systemAppsPageIndex;
		g_warning("--> [APP-MARSHALL] %s: appId %s *defaulted* to the 'System' page of index %u",
						__FUNCTION__,qPrintable(pWebOSapp->appId()),appPageIndex);
	}

	return true;
}

void LauncherObject::slotAppAdd(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin)
{
	//NOTE: duplicate app adds should NOT enter here -- should have been sorted out by the appmonitor

	//add the main icon to the launcher maps and the proper pages - it's already been created by the appmonitor
	//support WebOSApp only at the moment
	const DimensionsSystemInterface::WebOSApp * pWebOSapp = qobject_cast<const DimensionsSystemInterface::WebOSApp *>(&eapp);
	if (!pWebOSapp)
	{
		return;
	}

	//if launcher hasn't done a full size init yet, then don't do this.
	// It won't be a problem. The full size init will restore the pages anyways
	if (!m_fsInitHasRun)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": ignoring add for " << pWebOSapp->appId() << " for the time being; launcher has not yet initialized";
		return;
	}

	IconBase * pMainIcon = pWebOSapp->mainAppIcon();
	if (!pMainIcon)
	{
		return;
	}

//GEMSTONE-RD
//	AlphabetPage * pAllPage = allPage();

	//GEMSTONE-RD: all the apps used to go to the AlphabeticPage "All". That is no longer the case...now they go to a designated page

	quint32 appPageIndex = pageIndexForApp(eapp);

	//assumption is that none of the pages in the m_pages list are null...that should have been cleaned up earlier/elsewhere by LauncherObject
	Page * pTargetPage = m_pages[appPageIndex];
//GEMSTONE-RD

	addIconToPage(pMainIcon,pTargetPage);

	if (!pMainIcon->connectRequestsToLauncher())
	{
		qDebug() << __FUNCTION__ << ": appId:[" << pWebOSapp->appId() << "] , main icon -- failed to connect to LauncherObject's request slot";
	}

	//add it to the app maps
	m_appMapByIconUid.insert(pMainIcon->uid(),eapp.uid());
	m_appMapByAppUid.insert(eapp.uid(),pMainIcon->uid());
	saveCurrentLauncherLayouts();

}

void LauncherObject::slotAppAuxiliaryIconAdd(const QUuid& appUid,
		const QUuid& newLaunchPointIconUid,
		DimensionsSystemInterface::AppMonitorSignalType::Enum source)
{
	//the icon has already been created, and the associated app determined.
	//two things remain:
	// 1. add the icon to the right page (DFISH, GEMSTONE-RD: this means Favorites)
	// 2. add the icon<->app mapping to the map so the right app can be launched

	//grab the icon from the heap
	IconBase * pIcon = IconHeap::iconHeap()->getIcon(newLaunchPointIconUid);
	if (!pIcon)
	{
		qDebug() << __FUNCTION__ << ": ERROR: No icon; it should have been created by the signaller/caller! aborting.";
		return;
	}

	if (!pIcon->connectRequestsToLauncher())
	{
		qDebug() << __FUNCTION__ << ": app uid :[" << appUid << "] , aux icon -- failed to connect to LauncherObject's request slot";
	}

	//TODO: the icon most certainly should NOT be added to any pages at this point...but perhaps check for that?

	Page * pFavoritesPage = favoritesPage();
	if (!pFavoritesPage)
	{
		qDebug() << __FUNCTION__ << ": ERROR: No favorites page!!!???....ok, using page 0";
		pFavoritesPage = m_pages[0];		//if there is no page 0, then disaster is pending
	}

	addIconToPage(pIcon,pFavoritesPage);
	m_appMapByIconUid.insert(newLaunchPointIconUid,appUid);
	m_appMapByAppUid.insert(appUid,newLaunchPointIconUid);
	saveCurrentLauncherLayouts();

}

void LauncherObject::slotRestorePagesFromSaved()
{
	//TODO: IMPLEMENT
}

//immediately saves the entire launcher state w.r.t. pages
void LauncherObject::slotSavePages()
{
	saveCurrentLauncherLayouts();
}

void LauncherObject::slotIconActivatedTap(IconBase* pIcon)
{
	if(!pIcon)
		return;

	QUuid iconUid = pIcon->uid();

	//QML-DOESN'T-SUPPORT-TOUCH WORKAROUND
	// 	can't consume the touch events in the QML dialog, so therefore I'll just prevent actions here if the dialog is visible
	//		this will have to be done everywhere that the QML dialog could occlude something actionable
	//TODO: investigate gating this in okToUseTap() and similar...

	if (m_appInfoDialog)
	{
		if (m_appInfoDialog->isVisible())
		{
			//reject
			return;
		}
	}

	if(m_iconShowingFeedback && (m_iconShowingFeedback == pIcon)) {
		// user tapped again on the same icon while we are still working on launching the previous
		// request, so just ignore it
		return;
	}

	//if reorder mode is on...
	//TODO: less hardcoded...see activePageEnteredReorderMode()
	if (property("reorder_mode").toBool() &&
			!(OperationalSettings::settings()->allowedToLaunchInReorderMode))
	{
		//launches in reorder mode are not allowed
		return;
	}

	cancelLaunchFeedback();

	//translate to app uid
	QUuid appUidForIcon = appUidFromIconUid(iconUid);
	if (appUidForIcon.isNull())
	{
		//it might be an unmapped clone. Try it as its master
		IconBase * pIcon = IconHeap::iconHeap()->getIcon(iconUid);
		if (pIcon)
		{
			pIcon->setLaunchFeedbackVisibility(true);
			pIcon = pIcon->master();
			appUidForIcon = appUidFromIconUid(pIcon->uid());
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

	setAppLaunchFeedback(pIcon);

	DimensionsSystemInterface::ExternalApp * pApp = DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(appUidForIcon);
	if (pApp)
	{
		DimensionsSystemInterface::WebOSApp * pWoApp = qobject_cast<DimensionsSystemInterface::WebOSApp *>(pApp);
		if (pWoApp)
		{
			//qDebug() << "WebOS App " << pWoApp->appId();
			DimensionsSystemInterface::AppEffector::appEffector()->launch(pWoApp,iconUid);
			Q_EMIT signalHideMe(DimensionsTypes::HideCause::SystemApplaunch);
		}
	}
}

void LauncherObject::setAppLaunchFeedback(IconBase* pIcon)
{
	if(!pIcon)
		return;

	cancelLaunchFeedback();

	pIcon->setLaunchFeedbackVisibility(true);

	m_iconShowingFeedback = pIcon;

	m_feedbackTimer.start(DynamicsSettings::settings()->launchFeedbackTimeout);
}

void LauncherObject::cancelLaunchFeedback()
{
	if(m_iconShowingFeedback) {
		m_iconShowingFeedback->setLaunchFeedbackVisibility(false);
		m_iconShowingFeedback = 0;
	}
	m_feedbackTimer.stop();
}

void LauncherObject::slotCancelLaunchFeedback()
{
	cancelLaunchFeedback();
}

void LauncherObject::slotTabActivatedTap(Page * tabRelatedPage)
{
	if (tabRelatedPage == 0)
	{
		//nothing to do
		return;
	}

	qint32 pageIndex = tabRelatedPage->property(Page::PageIndexPropertyName).toInt();
	slotGotoPageIndex(pageIndex);
}

void LauncherObject::slotTabActivatedTapAndHold(Page * tabRelatedPage)
{
	//TODO: IMPLEMENT
}

void LauncherObject::slotPageModeChanged(PageMode::Enum oldV,PageMode::Enum newV)
{
	Page * pPage = qobject_cast<Page *>(sender());
	if (pPage)
	{
		if (pPage->isActive())
		{
			if (newV == PageMode::Normal)
			{
				activePageExitedReorderMode();
			}
			else if (newV == PageMode::Reorder)
			{
				activePageEnteredReorderMode();
			}
		}
	}
}

void LauncherObject::activePageEnteredReorderMode()
{
	//bring up the done button
	//TODO: FANCY-FU: perhaps a fade in anim
	m_qp_doneButton->setVisible(true);

	//TODO: less hardcoded...
	setProperty("reorder_mode",true);

	//TODO: TEMP: this isn't quite the right way to do this since it could be slow and wasteful
	// turn all the other cards to reorder mode graphics
	Q_EMIT signalPagesStartReorderMode();
}

void LauncherObject::activePageExitedReorderMode()
{
	//get rid of the done button
	//TODO: FANCY-FU: perhaps a fade out anim
	m_qp_doneButton->setVisible(false);

	//TODO: less hardcoded...
	setProperty("reorder_mode",false);

	//TODO: TEMP: this isn't quite the right way to do this since it could be slow and wasteful
	// turn all the other cards to normal mode graphics
	Q_EMIT signalPagesEndReorderMode();
}

void LauncherObject::slotReorderDoneButtonTap()
{
	if (m_blockingPageInteractions)
	{
		return;
	}
	//if the appinfo dialog is (still) visible, then hide it
	// (the hide function is actually a no-op if it's already non-visible)
	hideAppInfoDialog();
	//signal pages that reorder mode should end
	Q_EMIT signalPagesEndReorderMode();
	//TODO: should really check if pages are consistent before saving (though in concept, the previous signalling should assure that they are)
	saveCurrentLauncherLayouts();
}

// was split from slotReorderDoneButtonTap just in case something different needs to be done
// when it isn't an actual Done button interaction. Otherwise, identical functionality
void LauncherObject::slotReorderExitPageRequest()
{
	//if the app dialog is up, don't allow this
	// TODO: add the ability to tell the page that I didn't do it
	if (appInfoDialogIsVisible())
	{
		return;
	}

	//signal pages that reorder mode should end
	Q_EMIT signalPagesEndReorderMode();
	//TODO: should really check if pages are consistent before saving (though in concept, the previous signalling should assure that they are)
	saveCurrentLauncherLayouts();
}

//virtual
void LauncherObject::slotIconActionRequest(int cmdRequest,IconBase * p_iconSource)
{
	IconActionRequest::Enum actionRequested = (IconActionRequest::Enum)cmdRequest;

	if ((actionRequested <= IconActionRequest::INVALID) || (actionRequested >= IconActionRequest::LAST_INVALID))
	{
		//qDebug() << __FUNCTION__ << ": sender requested an invalid action (" << cmdRequest << ")";
		g_message("[ICON-TAP-TRACE] %s: sender requested an invalid action %d",__FUNCTION__,cmdRequest);
		return;
	}
	//sender must be an icon
	if (!p_iconSource)
	{
		p_iconSource = qobject_cast<IconBase *>(sender());
		if (p_iconSource == 0)
		{
			//qDebug() << __FUNCTION__ << ": sender isn't an icon";
			g_message("[ICON-TAP-TRACE] %s: no icon param specified, and sender() isn't an icon",__FUNCTION__);
			return;
		}
	}

	//translate to app uid
	QUuid appUidForIcon = appUidFromIconUid(p_iconSource->uid());
	if (appUidForIcon.isNull())
	{
		//qDebug() << __FUNCTION__ << ": icon uid " << p_iconSource->uid() << " tap'd, but it doesn't have an app mapped to it";
		g_message("[ICON-TAP-TRACE] %s: icon uid %s tap'd, but it doesn't have an app mapped to it",__FUNCTION__,qPrintable(p_iconSource->uid().toString()));
		return;
	}

	//get the app associated
	DimensionsSystemInterface::ExternalApp * pApp = DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(appUidForIcon);

	//check the page that the icon claims it belongs to
	Page * pOwnerPage = p_iconSource->owningPage();

	if (qobject_cast<ReorderablePage *>(pOwnerPage))
	{
		//it's currently owned by a reorderable page
		//qDebug() << __FUNCTION__ << ": icon uid " << p_iconSource->uid() << ": removing from ReorderablePage";
		g_message("[ICON-TAP-TRACE] %s: icon uid %s : removing from ReorderablePage",__FUNCTION__,qPrintable(p_iconSource->uid().toString()));
		//if an app dialog is already up, then bail out! (this MAY be ok if another decorator was hit deliberately, but it could have been the (x) decorator right under the already-displayed dialog...annoying!)
		if (appInfoDialogIsVisible())
		{
			//qDebug() << __FUNCTION__ << ": icon uid " << p_iconSource->uid() << ": suppressing dialog because the dialog is already visible";
			g_message("[ICON-TAP-TRACE] %s: icon uid %s : suppressing dialog because the dialog is already visible",__FUNCTION__,qPrintable(p_iconSource->uid().toString()));
			return;
		}

		//present the app info dialog that will prompt the user to remove

//		g_message("[ICON-TAP-TRACE] %s: icon uid %s : BRINGING UP APP DIALOG",__FUNCTION__,qPrintable(p_iconSource->uid().toString()));

		appDeleteDecoratorActivated(pApp,p_iconSource->master()->uid());

		//the rest happens depending on what the user clicks in the dialog
	}
	else
	{
		g_message("[ICON-TAP-TRACE] %s: icon uid %s : NO OWNER PAGE!!???",__FUNCTION__,qPrintable(p_iconSource->uid().toString()));
	}

	update();
}

void LauncherObject::appDeleteDecoratorActivated(DimensionsSystemInterface::ExternalApp * p_eapp,const QUuid& iconUid)
{
	DimensionsSystemInterface::WebOSApp * pWebOSapp = qobject_cast<DimensionsSystemInterface::WebOSApp *>(p_eapp);
	if (!pWebOSapp)
	{
		//unsupported for now
		return;
	}

	DimensionsSystemInterface::WOAppIconType::Enum type;
	QString launchPointId = pWebOSapp->launchpointIdOfIcon(iconUid,&type);
	if (type == DimensionsSystemInterface::WOAppIconType::Auxiliary)
	{
		//TODO: LOCALIZE (once the text is agreed on)
		QString dialogTitle = fromStdUtf8(LOCALIZED("Remove Shortcut?"));
		QString innerText = pWebOSapp->launchPointTile(launchPointId)+ QString(" (") +pWebOSapp->title()+QString(")");
		showAppInfoDialog(dialogTitle,
				innerText,
				pWebOSapp->appId(),
				iconUid,
				true
						);
	}
	else
	{
	//TODO: LOCALIZE (once the text is agreed on)
	QString dialogTitle = fromStdUtf8(LOCALIZED("Remove Application?"));
	QString innerText = pWebOSapp->title()+QString(" - v.")+pWebOSapp->version();
	showAppInfoDialog(dialogTitle,
						innerText,
						pWebOSapp->appId(),
						iconUid,
						(pWebOSapp->nonRemovableSystemApp() == false)
					);

	}

}

void LauncherObject::showAppInfoDialog(const QString& dialogTitle,
										const QString& innerText,
										const QString& appIdContext,
										const QUuid& iconUid,
										bool showRemoveButton,
										const QPointF& dialogPos)
{
	if (m_qp_overlay)
	{
		m_qp_overlay->setMode((int)(OverlayLayerMode::Scrim));
	}
	if(!m_appInfoDialog)
		return;

	m_appInfoDialog->setProperty("appIdContext",appIdContext);
	m_appInfoDialog->setProperty("iconUidStringContext",iconUid.toString());
	m_appInfoDialog->setProperty("dialogTitle",dialogTitle);
	m_appInfoDialog->setProperty("dialogMessage",innerText);
	m_appInfoDialog->setProperty("numberOfButtons",(int)(showRemoveButton ? 2 : 1));
	m_appInfoDialog->setPos (dialogPos.x()-m_appInfoDialog->boundingRect().width()/2,
							dialogPos.y()-m_appInfoDialog->boundingRect().height());

	m_appInfoDialog->setZValue(100.0);
	QMetaObject::invokeMethod(m_appInfoDialog, "fade", Q_ARG(QVariant, true),
														Q_ARG(QVariant,DynamicsSettings::settings()->appInfoDialogFadeInTime));
	blockPageInteraction();
}

void LauncherObject::blockPageInteraction()
{
	m_blockingPageInteractions = true;
	Q_EMIT signalBlockPageInteraction();
}

void LauncherObject::restorePageInteraction()
{
	m_blockingPageInteractions = false;
	Q_EMIT signalRestorePageInteraction();
}

void LauncherObject::hideAppInfoDialog(bool ignoreInvisible,bool noAnimation)
{
//	g_warning("%s: ENTERED",__FUNCTION__);
	if (m_qp_overlay)
	{
		m_qp_overlay->setMode((int)(OverlayLayerMode::OverlayShadows));
	}
	if(!m_appInfoDialog)
	{
//		g_warning("%s: EXIT: no app info dlg object",__FUNCTION__);
		return;
	}
	if (!(m_appInfoDialog->isVisible()) && (!ignoreInvisible))
	{
		//not visible, so avoid the call
//		g_warning("%s: EXIT: app info dlg not visible",__FUNCTION__);
		return;
	}
	if (noAnimation)
	{
		m_appInfoDialog->setOpacity(0.0);
	}
	else
	{
//		g_warning("%s: INFO: set fade",__FUNCTION__);
		QMetaObject::invokeMethod(m_appInfoDialog, "fade", Q_ARG(QVariant, false),
														Q_ARG(QVariant,DynamicsSettings::settings()->appInfoDialogFadeOutTime));
	}

//	g_warning("%s: EXIT: normal",__FUNCTION__);
}

bool LauncherObject::appInfoDialogIsVisible() const
{
	if(!m_appInfoDialog)
		return false;

	return m_appInfoDialog->isVisible();
}

void LauncherObject::slotGotoPageIndex(quint32 pageIndex)
{
	(void)gotoPageIndex(pageIndex);
}

static unsigned int pcount = 0;

bool LauncherObject::gotoPageIndex(quint32 pageIndex, qint32 xSpeed, bool canInterrupt)
{
	if (pageIndex >= (quint32)m_pages.size())
	{
		return false;
	}

	Page * pPage = m_pages[pageIndex];

	qint32 centerIdx = centerPageIndex();
	if (centerIdx != -1)
	{
		if ((quint32)centerIdx == pageIndex)
		{
			//already in the center, nothing to do.
			// call in a reactivate, just in case
			pPage->activatePage();
			return true;
		}

	}

	if (!stopAnimationEnsemble())
	{
		//couldn't stop the running animation. So just abort this flick then. It must be an important anim and
		//un-interruptible
		return false;
	}

	//page needs to move to the center
	qint32 cv = pageHorizontalOffsetToCenter(pageIndex);

	QPointF anchorPos = m_horizPanAnchor.pos();
	//lock to a deterministic pixel position
	anchorPos = QPointF(DimensionsGlobal::realAsPixelPosition(anchorPos.x()),DimensionsGlobal::realAsPixelPosition(anchorPos.y()));
	m_horizPanAnchor.setPos(anchorPos);

	//TODO: TEMP: don't hiijack the snapback curve params
	QPropertyAnimation * p_moveAnim = new QPropertyAnimation(&m_horizPanAnchor,"pos");
	if(xSpeed == 0) {
		p_moveAnim->setEndValue(anchorPos+QPointF((qreal)cv,0.0));
		p_moveAnim->setDuration(DynamicsSettings::DiUiDynamics()->snapbackAnimTime);
		p_moveAnim->setEasingCurve(DynamicsSettings::DiUiDynamics()->snapbackAnimCurve);
	} else {
		qint32 dist = cv;
		qint32 time = dist /( ((qreal)xSpeed) / 1000);

		if (time < 0) time *= -1;
		time = qBound(200, time, 1200);

		p_moveAnim->setEndValue(anchorPos+QPointF((qreal)cv,0.0));
		p_moveAnim->setDuration(time);
		p_moveAnim->setEasingCurve(QEasingCurve::OutCubic);
	}
	(void)m_horizPanAnchor.setAnimation(p_moveAnim);
	slotAddAnimationToEnsemble(p_moveAnim,DimensionsTypes::AnimationType::HPan);
	//deactivate current center page
	deactivateCenterPage();
	//start the h-pan
	slotStartAnimationEnsemble(canInterrupt);
	return true;
}

void LauncherObject::slotGotoPageIndexNoAnim(quint32 pageIndex)
{
	(void)gotoPageIndexNoAnim(pageIndex);
}

bool LauncherObject::gotoPageIndexNoAnim(quint32 pageIndex)
{
	if (pageIndex >= (quint32)m_pages.size())
	{
		return false;
	}

	Page * pPage = m_pages[pageIndex];

	qint32 centerIdx = centerPageIndex();
	if (centerIdx != -1)
	{
		if ((quint32)centerIdx == pageIndex)
		{
			//already in the center, nothing to do.
			// call in a reactivate, just in case
			pPage->activatePage();
			return true;
		}

	}

	if (!stopAnimationEnsemble())
	{
		//couldn't stop the running animation. So just abort this flick then. It must be an important anim and
		//un-interruptible
		return false;
	}

	//page needs to move to the center
	qint32 cv = pageHorizontalOffsetToCenter(pageIndex);

	QPointF anchorPos = m_horizPanAnchor.pos();
	//lock to a deterministic pixel position
	anchorPos = QPointF(DimensionsGlobal::realAsPixelPosition(anchorPos.x()),DimensionsGlobal::realAsPixelPosition(anchorPos.y()));
	m_horizPanAnchor.setPos(anchorPos+QPointF((qreal)cv,0.0));

	update();
	return true;
}

void LauncherObject::slotGotoLeftPage()
{
	(void)gotoLeftPage();
}

void LauncherObject::slotGotoRightPage()
{
	(void)gotoRightPage();
}

bool LauncherObject::gotoLeftPage(qint32 xSpeed)
{
	qint32 centerIdx = closestToCenterPageIndex();
	if (centerIdx == -1)
	{
		return false;
	}

	bool rc = gotoPageIndex(qMax(centerIdx-1,0), centerIdx > 0 ? xSpeed : 0, xSpeed != 0 ? true : false);
	if (rc)
	{
		Q_EMIT signalPagePanLeft();
	}
	return rc;
}

bool LauncherObject::gotoRightPage(qint32 xSpeed)
{
	qint32 centerIdx = closestToCenterPageIndex();
	if (centerIdx == -1)
	{
		return false;
	}

	bool rc = gotoPageIndex(qMin(centerIdx+1,m_pages.size()-1), centerIdx < m_pages.size()-1 ? xSpeed : 0, xSpeed != 0 ? true : false);
	if (rc)
	{
		Q_EMIT signalPagePanRight();
	}
	return rc;
}

void LauncherObject::slotSystemShowingLauncher()
{
	restorePageInteraction();
}

void LauncherObject::slotSystemHidingLauncher()
{
	//exit reorder mode, if in it
	Q_EMIT signalPagesEndReorderMode();
	//and hide the appinfo dialog; ignore invisible
	hideAppInfoDialog(true,true);
	restorePageInteraction();
}

void LauncherObject::slotSystemShowingLauncherOverlay()
{
	g_warning("%s: entry",__FUNCTION__);
	//exit reorder mode, if in it
	Q_EMIT signalPagesEndReorderMode();
	//and hide the appinfo dialog; ignore invisible
	hideAppInfoDialog(true,true);
	//block page interactions
	blockPageInteraction();
}

void LauncherObject::slotSystemHidingLauncherOverlay()
{
	g_warning("%s: entry",__FUNCTION__);
	restorePageInteraction();
}

void LauncherObject::slotQuicklaunchFullyOpen()
{
}

void LauncherObject::slotQuicklaunchFullyClosed()
{
}

void LauncherObject::slotLauncherFullyOpen()
{
	if (m_qp_overlay)
	{
		m_qp_overlay->recomputeShadowPositions();
		m_qp_overlay->setVisible(true);
	}
}

void LauncherObject::slotLauncherFullyClosed()
{
	if (m_qp_overlay)
	{
		m_qp_overlay->setVisible(false);
	}

	//TODO:  same code as slotSystemHidingLauncher() and that function SHOULD be getting called prior. However, it isn't in all cases, and as a result
	// things like [launcher: Delete dialog does not dismiss when edit mode exit] happen.

	//make sure that the app dialog is closed, and reorder mode exited
	//exit reorder mode, if in it
	Q_EMIT signalPagesEndReorderMode();
	//and hide the appinfo dialog, if visible
	hideAppInfoDialog(true,true);
	if (OperationalSettings::settings()->forceSaveOnHide)
	{
		saveCurrentLauncherLayouts();
	}
	cancelLaunchFeedback();
}

void LauncherObject::slotAnimationEnsembleFinished()
{
	m_seenHorizPanFlick = false;
//	qDebug() << __FUNCTION__ << "LauncherObject [ANIMATION]: ensemble animation signaled a finish";
	//walk the animation tree and figure out what finished
	_animationFinishedProcessGroup(m_qp_ensembleAnimation);
}

void LauncherObject::slotStopAnimationEnsemble(bool * r_result)
{
	if (r_result)
		*r_result = stopAnimationEnsemble();
	else
		(void)stopAnimationEnsemble();
}

bool LauncherObject::isAnimationEnsembleRunning() const
{
	if (m_qp_ensembleAnimation)
	{
		//altered notion of "running". in my context, it means not-stopped
		return (m_qp_ensembleAnimation->state() != QAbstractAnimation::Stopped);
	}
	return false;
}

bool LauncherObject::canStopAnimationEnsemble() const
{
	if (!m_qp_ensembleAnimation)
	{
		return true;
	}
	if ((m_qp_ensembleAnimation->state() != QAbstractAnimation::Stopped)
		&& (!m_qp_ensembleAnimation->property("canInterrupt").toBool()))
	{
		return false;
	}
	return true;
}

bool LauncherObject::stopAnimationEnsemble()
{
//	qDebug() << __FUNCTION__ << "LauncherObject [ANIMATION]: stop requested for ensemble animation";
	if (!m_qp_ensembleAnimation.isNull())
	{
		if (m_qp_ensembleAnimation->property("canInterrupt").toBool())
		{
			m_qp_ensembleAnimation->stop();
			delete m_qp_ensembleAnimation;
		}
		else
		{
			qDebug() << __FUNCTION__ << "LauncherObject [ANIMATION]: unable to stop ensemble animation - it's been marked un-stoppable";
			return false;
		}
	}
	return true;
}

void LauncherObject::slotStartAnimationEnsemble(bool canInterrupt)
{
//	qDebug() << __FUNCTION__ << "LauncherObject [ANIMATION]: start requested for ensemble animation";

	if (isAnimationEnsembleRunning())
	{
		qDebug() << __FUNCTION__ << "LauncherObject [ANIMATION]: start requested but animation is already running";
		return;
	}

	(void)connect(m_qp_ensembleAnimation,SIGNAL(finished()),SLOT(slotAnimationEnsembleFinished()));
	m_qp_ensembleAnimation->setProperty("canInterrupt",QVariant(canInterrupt));
	m_qp_ensembleAnimation->start(QAnimationGroup::DeleteWhenStopped);
}

void LauncherObject::slotAddAnimationToEnsemble(QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType)
{
	//qDebug() << __FUNCTION__ << "LauncherObject [ANIMATION]: add requested for ensemble animation";
	if (m_qp_ensembleAnimation.isNull())
		m_qp_ensembleAnimation = new QParallelAnimationGroup();
	slotAddAnimationTo(m_qp_ensembleAnimation,p_anim,animType);

}
void LauncherObject::slotAddAnimationTo(QAnimationGroup * p_addToGroup,QAbstractAnimation * p_anim,DimensionsTypes::AnimationType::Enum animType)
{
	if ((!p_anim) || (!p_addToGroup))
		return;

	p_anim->setProperty("diui_animtype",animType);
	p_addToGroup->addAnimation(p_anim);

}

void LauncherObject::slotHorizontalAnchorStopped()
{
	activateCenterPage();
//	saveCurrentLauncherLayouts();
}

void LauncherObject::slotRedirectedFlick(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent)
{
	if (m_blockingPageInteractions)
	{
		return;
	}

	Page * pSenderPage = qobject_cast<Page *>(sender());
	if (pSenderPage == 0)
	{
		qDebug() << "No sender()";
		//I don't deal with redirected flicks from non-Page things
		return;
	}


	m_seenHorizPanFlick = true;

	qint32 xSpeed = flickEvent->velocity().x();

	if (flickEvent->velocity().x() > 0)
	{
		if(m_touchStartPageIndex >= 0) {
			gotoPageIndex(qMax(m_touchStartPageIndex-1,0),
					      m_touchStartPageIndex > 0 ? xSpeed : 0,
						  true);
		} else {
					(void)gotoLeftPage(flickEvent->velocity().x());
		}
	}
	else
	{
		if(m_touchStartPageIndex >= 0) {
			gotoPageIndex(qMin(m_touchStartPageIndex+1,m_pages.size()-1),
					           m_touchStartPageIndex < m_pages.size()-1 ? xSpeed : 0,
					      true);
		} else {
					(void)gotoRightPage(flickEvent->velocity().x());
		}
	}

}

void LauncherObject::slotAppInfoDialogRemoveButtonPressed(const QString& appIdContext,const QString& iconUidAsString)
{
	//convert the icon uid string back to a uid object
	QUuid iconUid = QUuid(iconUidAsString);
	qDebug() << __PRETTY_FUNCTION__ << " with appIdContext = " << appIdContext << " and icon uid = " << iconUid;
	DimensionsSystemInterface::AppEffector::appEffector()->remove(appIdContext,iconUid);
	hideAppInfoDialog();
	restorePageInteraction();
}

void LauncherObject::slotAppInfoDialogCancelButtonPressed()
{
	qDebug() << __PRETTY_FUNCTION__;
	hideAppInfoDialog();
	restorePageInteraction();
}

void LauncherObject::_animationFinishedProcessGroup(QAnimationGroup * pAnim)
{
	if (!pAnim)
		return;
	for (int i=0;i < pAnim->animationCount();++i)
		_animationFinishedProcessAnim(pAnim->animationAt(i));
}

void LauncherObject::_animationFinishedProcessAnim(QAbstractAnimation * pAnim)
{
	if (!pAnim)
		return;
	if (qobject_cast<QAnimationGroup *>(pAnim))
		return _animationFinishedProcessGroup(qobject_cast<QAnimationGroup *>(pAnim));

	QPropertyAnimation * pPropAnim=0;
	Page * pPage=0;

	DimensionsTypes::AnimationType::Enum animType = (DimensionsTypes::AnimationType::Enum)(pAnim->property("diui_animtype").toInt());
	switch (animType)
	{
	case DimensionsTypes::AnimationType::Add:
		pPropAnim = qobject_cast<QPropertyAnimation *>(pAnim);
		if (pPropAnim)
		{
			_pageFinishedAddAnim(qobject_cast<Page *>(pPropAnim->targetObject()));
		}
		break;
	case DimensionsTypes::AnimationType::HPan:
		_pageFinishedHPanAnim();
		break;
	default:
		break;
	}
}

void LauncherObject::_pageFinishedAddAnim(Page * p_page)
{
	if (!p_page)
		return;

	//finished an add. Commit it to the page list
	m_pageLimbo.remove(QPointer<Page>(p_page));
	qint32 idx = p_page->property("pageuiindex").toInt();
	//qDebug() << __FUNCTION__ << ": adding page into list at index " << idx;
	m_pages.insert(idx,p_page);
	m_horizPanAnchor.addToGroup(p_page);
	p_page->resetUiState();
	dbgHorizPanAnchorContents();
	dbgPageVectorContents();
}

void LauncherObject::_pageFinishedHPanAnim()
{
	//finished horizontal panning
	slotHorizontalAnchorStopped();
}

void LauncherObject::_pageAddDirect(Page * p_page,quint32 idx)
{
	if (!p_page)
		return;

	//find the current center page index, if any
	qint32 centerIndex = centerPageIndex();
	if (centerIndex != -1)
	{
		if (idx == (quint32)centerIndex)
		{
			//the new page is being inserted into the list at the center index position.
			//it will become the new center, so deactivate the old one
			if (m_pages.at(centerIndex))
			{
				m_pages[centerIndex]->deactivatePage();
			}
		}
	}
	//finished an add. Commit it to the page list
	m_pageLimbo.remove(p_page);
	//qDebug() << __FUNCTION__ << ": adding page into list at index " << idx;
	//if there is a page at the index position right now, it will increase in index as well
	// as well as any pages "to the right"
	// TODO: i don't like this. rethink...
	for (int i=idx;i<m_pages.size();++i)
	{
		if (m_pages.at(i))
		{
			m_pages[i]->setProperty(Page::PageIndexPropertyName,i+1);
		}
	}
	m_pages.insert(idx,p_page);
	p_page->setProperty(Page::PageIndexPropertyName,idx);

	m_horizPanAnchor.addToGroup(p_page);

	//get the area of the quicklaunch bar
	QRectF quickLaunchArea;
	if (m_qp_mainWindow)
	{
		quickLaunchArea = m_qp_mainWindow->quickLaunchArea();
	}
	p_page->setPos(idx*p_page->geometry().width(),0.0);
	p_page->resetUiState();
	p_page->setProperty(Page::PageIndexPropertyName,idx);
	dbgHorizPanAnchorContents();
	dbgPageVectorContents();
	connect(p_page,SIGNAL(signalIconActivatedTap(IconBase*)),
			this,SLOT(slotIconActivatedTap(IconBase*)));
	connect(p_page,SIGNAL(signalRedirectFlick(FlickGesture *,QGestureEvent *)),
			this,SLOT(slotRedirectedFlick(FlickGesture *,QGestureEvent * )));
	connect(p_page,SIGNAL(dbg_signalTriggerCamera(ThingPaintable *)),
			this,SLOT(dbg_slotTriggerCamera(ThingPaintable *)));

	connect(p_page,SIGNAL(signalPageNeedsSave()),
			this,SLOT(slotSavePages()));
	connect(p_page,SIGNAL(signalPageModeChanged(PageMode::Enum,PageMode::Enum)),
			this,SLOT(slotPageModeChanged(PageMode::Enum,PageMode::Enum)));
	connect(this,SIGNAL(signalPagesStartReorderMode()),
			p_page,SLOT(slotLauncherCmdStartReorderMode()));
	connect(this,SIGNAL(signalPagesEndReorderMode()),
			p_page,SLOT(slotLauncherCmdEndReorderMode()));

	connect(p_page,SIGNAL(signalPageRequestExitReorder()),
			this,SLOT(slotReorderExitPageRequest()));

	connect(this,SIGNAL(signalBlockPageInteraction()),
			p_page,SLOT(slotLauncherBlockedInteractions())
			);
	connect(this,SIGNAL(signalRestorePageInteraction()),
			p_page,SLOT(slotLauncherAllowedInteractions())
			);

	//the complement of the if clause above...activate the new page
	centerIndex = centerPageIndex();
	if (centerIndex != -1)
	{
		if (idx == (quint32)centerIndex)
		{
			if (m_pages.at(centerIndex))
			{
				m_pages[centerIndex]->activatePage();
			}
		}
	}
}


//////////////////////////////////// POSITIONING / LAYOUT CODE ///////////////////////////////////////




// ------------------- APP HANDLING ----------------------------------//

//virtual
bool LauncherObject::processNewWebOSApp(DimensionsSystemInterface::WebOSApp * p_app,bool mainIconOnly,QList<IconBase *>& r_iconList)
{
	if (!p_app)
	{
		return false;
	}
	//check to make sure it doesn't already exist
	if (!(iconUidFromAppUid(p_app->uid()).isNull()))
	{
		//exists...
		return false;
	}

	// Remember, all of these icons are in the icon heap.
	IconBase * pMainIcon = p_app->mainAppIcon();
	if (pMainIcon == 0)
	{
		//no icon
		return false;
	}

	//connect up icon request signal to this object
	pMainIcon->connectRequestsToLauncher();

	m_appMapByIconUid.insert(pMainIcon->uid(),p_app->uid());
	m_appMapByAppUid.insert(p_app->uid(),pMainIcon->uid());
	r_iconList.append(pMainIcon);

	if (!mainIconOnly)
	{
		QList<IconBase *> auxIcons = p_app->auxAppIcons();
		for (QList<IconBase *>::iterator it = auxIcons.begin();
				it != auxIcons.end();++it)
		{
			if (!(*it))
			{
				continue;
			}
			//connect icon request signal
			(*it)->connectRequestsToLauncher(this);
		}
		r_iconList.append(p_app->auxAppIcons());
	}

	return true;
}

void LauncherObject::saveCurrentLauncherLayouts()
{
	qDebug() << __FUNCTION__ << ": trying to save Launcher state";
	DimensionsSystemInterface::PageSaver::saveLauncher(this);
}

void LauncherObject::sendIconToQuickLaunchBar(IconBase * p_icon)
{
	if (p_icon == 0)
	{
		return;
	}

	//translate from IconBase all the way to the LaunchPoint id (see Src/base/LaunchPoint.h/cpp)
	///...I'll do this via the iconheap which has this info associated with the icon
	IconAttributes attr;
	if (IconHeap::iconHeap()->getIconEx(p_icon->uid(),attr) != p_icon)
	{
		//something is wrong; the icon wasn't found
		return;
	}

	//at this point, it should just be a straight signal send
	Q_EMIT signalDropIconOnQuicklaunch(attr.launchPtId);
}

//virtual
bool LauncherObject::offer(Thing * p_offer,Thing * p_offeringThing)
{
	//only accept icons
	IconBase * pIconOffer = qobject_cast<IconBase *>(p_offer);
	if (!pIconOffer)
	{
		return false;
	}

	//TODO: don't accept duplicates
	if (iconLimboContains(pIconOffer))
	{
		//duplicate
		return false;
	}

	//else, initiate a take
	bool takeSuccess = pIconOffer->take(this);
	if (takeSuccess)
	{
		QPointF oldPos = pIconOffer->scenePos();
		//it's my icon - at this point, the previous owner already released all interest in the icon, so I'm free to assimilate it!
		//TODO: special handling just in case it fails! hmmm...should probably send it back to where it came from immediately
		acceptIncomingIcon(pIconOffer);
		pIconOffer->setParentItem(this);
		pIconOffer->setPos(mapFromScene(oldPos));
		update();

	}
	return takeSuccess;
}

//virtual
bool LauncherObject::take(Thing * p_takerThing)
{
	return false; 		//no one can take the launcherobject!
}

//virtual
bool LauncherObject::taking(Thing * p_victimThing, Thing * p_takerThing)
{
	return true;	//allow abductions of anything i own
}

//virtual
void LauncherObject::taken(Thing * p_takenThing,Thing * p_takerThing)
{
	IconBase * pTakenIcon = qobject_cast<IconBase *>(p_takenThing);
	if (!pTakenIcon)
	{
		return;	//don't know what this was...i only recognize icons
	}

	releaseIcon(pTakenIcon);
}

void LauncherObject::acceptIncomingIcon(IconBase * p_icon)
{
	if (!p_icon)
	{
		return;
	}
	m_iconLimbo.insert(p_icon->uid(),QPointer<IconBase>(p_icon));
}

void LauncherObject::releaseIcon(IconBase * p_icon)
{
	if (!m_iconLimbo.remove(p_icon->uid()))
	{
		qDebug() << __FUNCTION__ << ": FAIL!";
	}
	else
	{
		qDebug() << __FUNCTION__ << ": OK";
	}
}

IconBase * LauncherObject::getIconInLimbo(const QUuid& iconUid)
{
	QMap<QUuid,QPointer<IconBase> >::iterator f = m_iconLimbo.find(iconUid);
	if (f != m_iconLimbo.end())
	{
		return *f;
	}
	return 0;
}

bool LauncherObject::iconLimboContains(const QUuid& iconUid)
{
	return m_iconLimbo.contains(iconUid);
}

bool LauncherObject::iconLimboContains(IconBase * p_icon)
{
	if (!p_icon)
	{
		return false;
	}
	return m_iconLimbo.contains(p_icon->uid());
}

bool LauncherObject::wasIconDroppedOnTab(IconBase * p_icon)
{
	if (!p_icon)
	{
		return false;
	}

	return (p_icon->property(IconBase::IconTransferContextPropertyName).toString() == "tabdrop");
}

bool LauncherObject::wasIconMovedOntoPage(IconBase * p_icon)
{
	if (!p_icon)
	{
		return false;
	}
	return (p_icon->property(IconBase::IconTransferContextPropertyName).toString() == "ontopage");
}

void LauncherObject::clearIconTxContext(IconBase * p_icon)
{
	if (p_icon)
	{
		p_icon->setProperty(IconBase::IconTransferContextPropertyName,QString());
	}
}

void LauncherObject::setIconTxContextDroppedOnTab(IconBase * p_icon)
{
	if (p_icon)
	{
		p_icon->setProperty(IconBase::IconTransferContextPropertyName,QString("tabdrop"));
	}
}

void LauncherObject::setIconTxContextMovedOntoPage(IconBase * p_icon)
{
	if (p_icon)
	{
		p_icon->setProperty(IconBase::IconTransferContextPropertyName,QString("ontopage"));
	}
}

void LauncherObject::removeIconFromPages(IconBase * p_icon)
{
	//TODO: more robust, maybe better param convention
	//	(should deal with the icon having a bad owning page set)

	//check the page that the icon claims it belongs to
	Page * pOwnerPage = p_icon->owningPage();
	//try it as a Alpha page
	if (qobject_cast<AlphabetPage *>(pOwnerPage))
	{
		qobject_cast<AlphabetPage *>(pOwnerPage)->removeIcon(p_icon->uid());
	}
	else if (qobject_cast<ReorderablePage *>(pOwnerPage))
	{
		qobject_cast<ReorderablePage *>(pOwnerPage)->removeIcon(p_icon->uid());
	}
	update();
}

void LauncherObject::removeIconUidFromMaps(const QUuid& iconUid,const QUuid& optAppUid)
{

	UidTranslationMapIter it = m_appMapByIconUid.find(iconUid);
	if (it != m_appMapByIconUid.end())
	{
		if (optAppUid.isNull())
		{
			UidTranslationMapIter ait = m_appMapByAppUid.find(it.value());
			if (ait != m_appMapByAppUid.end())
			{
				if (ait.value() == iconUid)
				{
					m_appMapByAppUid.erase(ait);
					//remove the reverse entry
				}
			}
		}
		else
		{
			m_appMapByAppUid.remove(optAppUid);
		}
		m_appMapByIconUid.erase(it);
	}
}

//used by slotAppAdd...this is NOT a reorder-start add!!! (i.e. do not enter a the tracking cycle)
void LauncherObject::addIconToPage(IconBase * p_icon,Page * p_page)
{
	if ((!p_icon) || (!p_page))
	{
		return;
	}

	p_icon->setUsePrerenderedLabel(true);

	//this is not a reordering add, so call addIcon on the page. This should now be ok, even during a reorder, as
	// addIcon can queue until after the reorder
	if (!p_page->addIcon(p_icon))
	{
		qDebug() << __PRETTY_FUNCTION__ << ": error: icon add fail!";
	}
}

//virtual
QUuid LauncherObject::appUidFromIconUid(const QUuid& iconUid) const
{
	UidTranslationMapConstIter it = m_appMapByIconUid.constFind(iconUid);
	if (it == m_appMapByIconUid.constEnd())
	{
		return QUuid();
	}
	return it.value();
}

//virtual
QUuid LauncherObject::iconUidFromAppUid(const QUuid& appUid) const
{
	UidTranslationMapConstIter it = m_appMapByAppUid.constFind(appUid);
	if (it == m_appMapByAppUid.constEnd())
	{
		return QUuid();
	}
	return it.value();
}

bool LauncherObject::isAppRemovable(const QUuid& uid)
{
	//try the uid as an app uid, directly
	bool uidIsIconUid = false;
	DimensionsSystemInterface::ExternalApp *pApp = DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(uid);
	if (!pApp)
	{
		//try uid as an icon uid
		pApp =
		DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(appUidFromIconUid(uid));
		if (!pApp)
		{
			return false;
		}
		uidIsIconUid = true;
	}
	DimensionsSystemInterface::WebOSApp * pWoApp = qobject_cast<DimensionsSystemInterface::WebOSApp *>(pApp);
	if (!pWoApp)
	{
		return true;
	}

	return pWoApp->removableOrHideable();
}

bool LauncherObject::canShowRemoveDeleteDecoratorOnIcon(const QUuid& uid)
{
	//try the uid as an app uid, directly
	bool uidIsIconUid = false;
	DimensionsSystemInterface::ExternalApp *pApp = DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(uid);
	if (!pApp)
	{
		//try uid as an icon uid
		pApp =
				DimensionsSystemInterface::AppMonitor::appMonitor()->appByUid(appUidFromIconUid(uid));
		if (!pApp)
		{
			return false;
		}
		uidIsIconUid = true;
	}
	DimensionsSystemInterface::WebOSApp * pWoApp = qobject_cast<DimensionsSystemInterface::WebOSApp *>(pApp);
	if (!pWoApp)
	{
		return true;
	}

	//if the icon is an auxiliary (not the main icon), then ALWAYS show the decorator (aux icons are always removable)
	if (uidIsIconUid)
	{
		DimensionsSystemInterface::WOAppIconType::Enum type;
		QString launchPointId = pWoApp->launchpointIdOfIcon(uid,&type);
		if (type == DimensionsSystemInterface::WOAppIconType::Auxiliary)
		{
			return true;
		}
	}

	//Ok, not an auxiliary...
	//Do not present the (x) remove button for apps in various stages of install, including failures.
	//I want them to go through software manager
	if (pWoApp->isReady() == false)
	{
		return false;
	}
	//TODO: maybe just roll up that last "ready" clause into the  removableOrHideable() fn
	return pWoApp->removableOrHideable();
}

PageTab * LauncherObject::tabForPage(Page * p_page) const
{
	if (m_qp_pageTabBar)
	{
		return m_qp_pageTabBar->tabByPage(p_page);
	}
	return 0;
}

//////////////////////////////////// DEBUG //////////////////////////////////////////////////////////

void LauncherObject::dbgHorizPanAnchorContents()
{
	QList<QGraphicsItem *> contents = m_horizPanAnchor.childItems();
	for (QList<QGraphicsItem *>::const_iterator it = contents.constBegin();
			it != contents.constEnd();++it)
	{
		//qDebug() << "horizPanAnchor item: page idx = " << ((Page *)(*it))->pageIndex();
	}
}

void LauncherObject::dbgPageVectorContents()
{
	for (QList<QPointer<Page> >::const_iterator it = m_pages.constBegin();
			it != m_pages.constEnd();++it)
	{
		//qDebug() << "m_pages item: page idx = " << ((Page *)(*it))->pageIndex();
	}
}

void LauncherObject::dbg_slotTriggerCamera(ThingPaintable * excludeMe)
{
	QList<QPointer<ThingPaintable> > exclusions;
	if (excludeMe)
	{
		exclusions << QPointer<ThingPaintable>(excludeMe);
	}
	m_dbg_vcam.setup(exclusions);
	m_dbg_vcam.trigger();
}

void LauncherObject::dbg_slotPrint()
{
	qDebug() << __FUNCTION__;
}

void LauncherObject::slotDbgPageSaverDebugProcessDone(int exitCode)
{
	QProcess * pFinishedProcess = qobject_cast<QProcess *>(sender());
	if (pFinishedProcess)
	{
		pFinishedProcess->deleteLater();
	}
}
