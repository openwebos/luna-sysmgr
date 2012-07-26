/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#include "pagesaver.h"
#include "dimensionslauncher.h"
#include "quicklaunchbar.h"
#include "page.h"
#include "iconlayout.h"
#include "reorderablepage.h"
#include "stringtranslator.h"
#include "appmonitor.h"
#include "externalapp.h"
#include "webosapp.h"
#include "filenames.h"
#include "operationalsettings.h"

#include "safefileops.h"

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QVariantMap>
#include <QList>
#include <QPointer>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include <glib.h>

#include "BackupManager.h"

namespace DimensionsSystemInterface
{

QString PageSaver::SaveTagKey_PageType = QString("pagetype");
QString PageSaver::SaveTagKey_PageName = QString("pagename");
QString PageSaver::SaveTagKey_PageDesignator = QString("pagedesignator");
QString PageSaver::SaveTagKey_PageUid = QString("pageuid");
QString PageSaver::SaveTagKey_PageFile = QString("filepath");
QString PageSaver::SaveTagKey_PageIndex = QString("pageindex");
QString PageSaver::SaveTagKey_PageNumIcons = QString("numicons");
QString PageSaver::SaveTagKey_PageRestoreObjectList = QString("restoreobjlist");

QString PageSaver::MasterTagHeaderKey_TimeStamp = QString("time_created");
QString PageSaver::MasterTagHeaderKey_SimpleName = QString("simple_name");
QString PageSaver::MasterTagHeaderKey_FileName = QString("file_name");
QString PageSaver::MasterTagHeaderKey_NumPages = QString("num_pages");
QString PageSaver::MasterTagHeaderKey_SaveSystemVersion = QString("save_sys_version");

QString PageSaver::QuicklaunchTagHeaderKey_TimeStamp = QString("time_created");
QString PageSaver::QuicklaunchTagHeaderKey_SimpleName = QString("simple_name");
QString PageSaver::QuicklaunchTagHeaderKey_FileName = QString("file_name");
QString PageSaver::QuicklaunchTagHeaderKey_SaveSystemVersion = QString("save_sys_version");

quint32 PageSaver::SaveSystemVersion = 7;

QPointer<PageSaver> PageSaver::s_qp_instance = 0;

//static
PageSaver * PageSaver::instance()
{
	if (!s_qp_instance)
	{
		s_qp_instance = new PageSaver();
	}
	return s_qp_instance;
}

//static
void PageSaver::filesForBackup(QList<QString> * pFileList)
{
	g_message("%s: entry",__FUNCTION__);
	if (!pFileList)
	{
		return;
	}

	//the files to be saved are
	// 1. master files
	// 2. the page files
	// 3. the quicklauncher save files

	//TODO: be more selective about saving only the valid files. For now, just save it all

	g_message("%s: about to scan: %s",__FUNCTION__,qPrintable(OperationalSettings::settings()->savedPagesDirectory));
	QList<QString> rlist;
	QDir dir = QDir(OperationalSettings::settings()->savedPagesDirectory);
	dir.setFilter(QDir::Files | QDir::Hidden);
//	QStringList filters;
//	filters << "*.msave";
//	dir.setNameFilters(filters);
	QFileInfoList list = dir.entryInfoList();
	g_message("%s: found %d entries",__FUNCTION__,list.size());
	//qDebug() << "processing dir " << dir.absolutePath() << " , " << list.size() << " entries";
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);
		g_message("%s: %s",__FUNCTION__,qPrintable(QString("Processing %1").arg(fileInfo.fileName())));
		*pFileList << fileInfo.absoluteFilePath();
	}
}

PageSaver::PageSaver()
{

}

//virtual
PageSaver::~PageSaver()
{

}

//static
QVariantMap PageSaver::savePage(Page * p_page,LauncherObject * p_uiContext)
{
	if ((!p_page)
		|| (!p_page))
	{
		//bail
		return QVariantMap();
	}

	//TODO: add new page types here....
	ReorderablePage * pReorderPage = qobject_cast<ReorderablePage *>(p_page);
	if (!pReorderPage)
	{
		//don't support any other page types at the moment
		return QVariantMap();
	}
	return saveReorderablePage(pReorderPage,p_uiContext);
}

//static
bool PageSaver::saveLauncher(LauncherObject * p_uiContext)
{
	if (!p_uiContext)
	{
		return false;
	}
	//grab the pages from the ui context
	QList<QPointer<Page> > pages = p_uiContext->pages();

	quint64 timeStamp = (quint64)QDateTime::currentMSecsSinceEpoch();
	QList<QVariantMap> pagesSaved;
	for (QList<QPointer<Page> >::const_iterator it = pages.constBegin();
			it != pages.constEnd();++it)
	{
		QVariantMap pageInfoMap = savePage(*it,p_uiContext);
		if (pageInfoMap.isEmpty())
		{
			//didn't save the page for whatever reason. Skip
			continue;
		}
		pagesSaved << pageInfoMap;
	}

	int numPagesSaved = pagesSaved.size();
	if (numPagesSaved == 0)
	{
		//no pages could be saved
		return false;
	}

	QString nameTimeFragment = ( OperationalSettings::settings()->useSingleMasterSaveFileName
								? QString("fixed")
								: (
									OperationalSettings::settings()->usePreciseTimeForMasterSaveFileName
											? QString("%1").arg(timeStamp)
											: QDate::currentDate().toString("dd-MM-yyyy")
									)
								);
	if (nameTimeFragment.isEmpty())
	{
		nameTimeFragment = QString("null");
	}

	//create the masterfile
	QString masterFilepath = OperationalSettings::settings()->savedPagesDirectory + QString("/") +
							QString("launcher_")+nameTimeFragment+QString(".msave");
	SafeFileOperator safesave(SafeFileOperator::Write,masterFilepath,QSettings::IniFormat);
	QSettings& master = safesave.safeSettings();
	if (master.status() != QSettings::NoError)
	{
		//problem with the file op
		return false;
	}

	master.beginGroup("header");
	master.setValue(PageSaver::MasterTagHeaderKey_TimeStamp,timeStamp);
	master.setValue(PageSaver::MasterTagHeaderKey_SimpleName,QString("default"));
	master.setValue(PageSaver::MasterTagHeaderKey_NumPages,numPagesSaved);
	master.setValue(PageSaver::MasterTagHeaderKey_SaveSystemVersion,SaveSystemVersion);
	master.endGroup();

	master.beginWriteArray("pages");
	int pageIdx=0;
	for (QList<QVariantMap>::iterator it = pagesSaved.begin();
			it != pagesSaved.end();++it,++pageIdx)
	{
		master.setArrayIndex(pageIdx);
		master.setValue(PageSaver::SaveTagKey_PageType,it->value(PageSaver::SaveTagKey_PageType));
		master.setValue(PageSaver::SaveTagKey_PageFile,it->value(PageSaver::SaveTagKey_PageFile));
		master.setValue(PageSaver::SaveTagKey_PageIndex,it->value(PageSaver::PageSaver::SaveTagKey_PageIndex));
	}
	master.endArray();

	return true;
}

//static
bool PageSaver::saveQuickLaunch(LauncherObject * p_uiContext,QuickLaunchBar * p_quickLauncher)
{
	if ((!p_uiContext) || (!p_quickLauncher))
	{
		return false;
	}

	quint64 timeStamp = (quint64)QDateTime::currentMSecsSinceEpoch();

	QString nameTimeFragment =
		( OperationalSettings::settings()->useSingleQuicklaunchSaveFileName
			? QString("fixed")
			: QDate::currentDate().toString("dd-MM-yyyy")
		);

	if (nameTimeFragment.isEmpty())
	{
		nameTimeFragment = QString("null");
	}

	//create the quicklaunch save file
	QString quickLaunchFilepath = OperationalSettings::settings()->savedPagesDirectory + QString("/") +
			QString("quicklaunch_")+nameTimeFragment+QString(".qlsave");			///WARNING: don't change extension or the file scanner for restore won't work (see pagerestore.cpp)

	SafeFileOperator safesave(SafeFileOperator::Write,quickLaunchFilepath,QSettings::IniFormat);
	QSettings& saver = safesave.safeSettings();
	if (saver.status() != QSettings::NoError)
	{
		//problem with the file op
		return false;
	}

	saver.beginGroup("header");
	saver.setValue(PageSaver::QuicklaunchTagHeaderKey_TimeStamp,timeStamp);
	saver.setValue(PageSaver::QuicklaunchTagHeaderKey_SimpleName,QString("default"));
	saver.setValue(PageSaver::QuicklaunchTagHeaderKey_SaveSystemVersion,SaveSystemVersion);
	saver.endGroup();

	saver.beginWriteArray("icons");
	QList<QPointer<IconBase> > iconFlowList = p_quickLauncher->iconsInFlowOrder();

	int idx = 0;
	int iti = 0;
	for (QList<QPointer<IconBase> >::iterator it = iconFlowList.begin();
			it != iconFlowList.end();++it,++iti)
	{
		//grab the icon, then look up the app info for it via one call to the main ui's (p_uiContext)
		//function for mapping icon uid to app uid, and then one call to appmonitor to grab the app specifics
		// through this app uid
		IconBase * pIcon = (*it);
		if (!pIcon)
		{
			qDebug() << __FUNCTION__ << ": error (iti = " << iti << "): no icon at this cell";
			continue;
		}
		//TODO: there are a few ways to assure icon clones map to the right app. This is one of them, not necessarily the best way.
		//	Other places in the system (see LauncherObject::addRestoredPages()) actually add the clones to the map that
		// appUidFromIconUid consults.
		IconBase * pMasterIcon = pIcon->master();
		QUuid appUid = p_uiContext->appUidFromIconUid(pMasterIcon->uid());
		if (appUid.isNull())
		{
			qDebug() << __FUNCTION__ << ": error (iti = " << iti << "): icon did not map to a valid app";
			continue;
		}
		WebOSApp * pApp = qobject_cast<WebOSApp *>(AppMonitor::appMonitor()->appByUid(appUid));

		//TODO: TEMP: only deal with WebOSApp for now
		if (!pApp)
		{
			qDebug() << __FUNCTION__ << ": error (iti = " << iti << "): app uid = " << appUid << " isn't a WebOS app";
			continue;
		}
		saver.setArrayIndex(idx++);
		saver.setValue("type",QString(pApp->metaObject()->className()));
		saver.setValue("id",pApp->appId());

		//check and see what kind of icon this is...i.e. what launchpoint it represents
		WOAppIconType::Enum t;
		QString launchptId = pApp->launchpointIdOfIcon(pMasterIcon->uid(),&t);
		if (t != WOAppIconType::INVALID)
		{
			saver.setValue("launchtype",(int)t);
			saver.setValue("launchid",launchptId);
		}

	}

	saver.endArray();
	return true;
}

///protected:

//virtual
QVariantMap PageSaver::saveReorderablePage(ReorderablePage * p_reorderPage,LauncherObject * p_uiContext)
{
	/*
	 *
	 * A ReorderablePage is an icon layout in a specific order, in a grid pattern. However, it is laid out top to bottom, left to right
	 *	ReorderablePage-s are allowed to grow and shrink width-wise depending on screen resizes, incl. during rotations, so hardcoding
	 *	the grid point coords doesn't make much sense. Instead, the "launch point" ids are saved in an ordered 1-D array
	 *
	 */

	//TODO: isolate this through a Page function
	IconLayout * pLayout = p_reorderPage->currentIconLayout();
	if (!pLayout)
	{
		return QVariantMap();
	}


	//create a filename to use
	QString filepath = OperationalSettings::settings()->savedPagesDirectory + QString("/")
			+ QString("page_")
			+ QString(p_reorderPage->metaObject()->className())
			+ QString("_")+p_reorderPage->property(Page::PageNamePropertyName).toString()
			+ QString("_")+p_reorderPage->uid().toString();

	SafeFileOperator safesave(SafeFileOperator::Write,filepath,QSettings::IniFormat);
	QSettings& settings = safesave.safeSettings();

	if (settings.status() != QSettings::NoError)
	{
		//problem with the file op
		return QVariantMap();
	}

	//clear it out, and re-sync
	settings.clear();
	settings.sync();

	//write out the page info first

	settings.beginGroup("header");
	settings.setValue(PageSaver::SaveTagKey_PageName,p_reorderPage->property(Page::PageNamePropertyName).toString());
	settings.setValue(PageSaver::SaveTagKey_PageDesignator,p_reorderPage->property(Page::PageDesignatorPropertyName).toString());
	settings.setValue(PageSaver::SaveTagKey_PageType,QString(p_reorderPage->metaObject()->className()));
	settings.setValue(PageSaver::SaveTagKey_PageUid,p_reorderPage->uid().toString());
	settings.endGroup();

	QVariantMap rmap;
	rmap[PageSaver::SaveTagKey_PageName] = p_reorderPage->property(Page::PageNamePropertyName).toString();
	rmap[PageSaver::SaveTagKey_PageDesignator] = p_reorderPage->property(Page::PageDesignatorPropertyName).toString();
	rmap[PageSaver::SaveTagKey_PageType] = QString(p_reorderPage->metaObject()->className());
	rmap[PageSaver::SaveTagKey_PageUid] = p_reorderPage->uid().toString();
	rmap[PageSaver::SaveTagKey_PageFile] = filepath;
	rmap[PageSaver::SaveTagKey_PageIndex] = p_reorderPage->property(Page::PageIndexPropertyName).toInt();

	settings.beginWriteArray("icons");
	QList<IconCell *> iconFlowList = pLayout->iconCellsInFlowOrder();
	int idx = 0;
	int iti = 0;
	for (QList<IconCell *>::iterator it = iconFlowList.begin();
			it != iconFlowList.end();++it,++iti)
	{
		//grab the icon, then look up the app info for it via one call to the main ui's (m_qp_uiContext)
		//function for mapping icon uid to app uid, and then one call to appmonitor to grab the app specifics
		// through this app uid
		IconCell * pCell = (*it);
		if (!pCell)
		{
			qDebug() << __FUNCTION__ << ": error (iti = " << iti << "): cell is null";
			continue;
		}
		IconBase * pIcon = pCell->m_qp_icon;
		if (!pIcon)
		{
			qDebug() << __FUNCTION__ << ": error (iti = " << iti << "): no icon at this cell";
			continue;
		}
		//TODO: there are a few ways to assure icon clones map to the right app. This is one of them, not necessarily the best way.
		//	Other places in the system (see LauncherObject::addRestoredPages()) actually add the clones to the map that
		// appUidFromIconUid consults.
		IconBase * pMasterIcon = pIcon->master();
		QUuid appUid = p_uiContext->appUidFromIconUid(pMasterIcon->uid());
		if (appUid.isNull())
		{
			qDebug() << __FUNCTION__ << ": error (iti = " << iti << "): icon did not map to a valid app";
			continue;
		}
		WebOSApp * pApp = qobject_cast<WebOSApp *>(AppMonitor::appMonitor()->appByUid(appUid));

		//TODO: TEMP: only deal with WebOSApp for now
		if (!pApp)
		{
			qDebug() << __FUNCTION__ << ": error (iti = " << iti << "): app uid = " << appUid << " isn't a WebOS app";
			continue;
		}
		settings.setArrayIndex(idx++);
		settings.setValue("type",QString(pApp->metaObject()->className()));
		settings.setValue("id",pApp->appId());
		//check and see what kind of icon this is...i.e. what launchpoint it represents
		WOAppIconType::Enum t;
		QString launchptId = pApp->launchpointIdOfIcon(pMasterIcon->uid(),&t);
		if (t != WOAppIconType::INVALID)
		{
			settings.setValue("launchtype",(int)t);
			settings.setValue("launchid",launchptId);
		}

		//qDebug() << __FUNCTION__ << ": saved appid = " << pApp->appId() << " at index " << idx-1;
	}
	settings.endArray();
	rmap[PageSaver::SaveTagKey_PageNumIcons] = idx;
	return rmap;
}

//static
void PageSaver::dbgPackUpAndSaveCurrentLauncher3Dir(const QString& extraFilenameTag,QList<QVariantMap> * p_dbgLauncherConf)
{

	//if the launcher config was passed in, create a dbg file with the contents
	//create a filename to use

	quint64 timeStamp = (quint64)QDateTime::currentMSecsSinceEpoch();
	QString nameTimeFragment = QString("%1").arg(timeStamp) + QDate::currentDate().toString("dd-MM-yyyy");

	if (p_dbgLauncherConf)
	{
		QString filepath = OperationalSettings::settings()->savedPagesDirectory + QString("/")
								+ QString("DEBUG_") + extraFilenameTag + QString("_%1.save").arg(nameTimeFragment);
		SafeFileOperator safesave(SafeFileOperator::Write,filepath,QSettings::IniFormat);
		QSettings& dbgset = safesave.safeSettings();
		if (dbgset.status() == QSettings::NoError)
		{
			//clear it out, and re-sync
			dbgset.clear();
			dbgset.sync();

			quint32 index=0;
			for (QList<QVariantMap>::iterator it = p_dbgLauncherConf->begin();
					it != p_dbgLauncherConf->end();++it,++index)
			{
				dbgset.beginGroup(QString(QString("index_%1").arg(index)));
				for (QVariantMap::iterator iit = it->begin();
						iit != it->end();++iit)
				{
					if (!(iit.value().isValid()))
					{
						g_message("%s: skipping qvariant because it's invalid (type = %s )",__FUNCTION__,iit.value().typeName());
						continue;
					}
					dbgset.setValue(iit.key(),iit.value());
				}
				dbgset.endGroup();
			}
			dbgset.sync();
		}
	}

	//tar.gz up the whole directory
	QString outputfile = OperationalSettings::settings()->logDirPath + QString("/")
										+ QString("LAUNCHER3_DEBUG_") + extraFilenameTag + QString("_%1.tar.gz").arg(nameTimeFragment);
	QString program = OperationalSettings::settings()->archiverExeFilename;
	QStringList arguments;
	arguments << "czf" << outputfile << OperationalSettings::settings()->savedPagesDirectory;
	///bin/sh /usr/palm/applications/com.palm.sysapp.launchermode0/.system.sh
	g_message("%s: About to run %s , with args: %s",__FUNCTION__,qPrintable(program),qPrintable(arguments.join(" ")));
	QProcess *myProcess = new QProcess();
	connect(myProcess,SIGNAL(finished(int)),
			LauncherObject::primaryInstance(),
			SLOT(slotDbgPageSaverDebugProcessDone(int)));

	myProcess->start(program, arguments);
}


} //end namespace
