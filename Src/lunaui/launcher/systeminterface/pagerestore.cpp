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




#include "pagerestore.h"
#include "dimensionsmain.h"
#include "appmonitor.h"
#include "pagesaver.h"
#include "stringtranslator.h"
#include "webosapp.h"
#include "reorderablepage.h"
#include "operationalsettings.h"

#include <QDir>
#include <QFileInfoList>
#include <QDebug>

#include "ApplicationManager.h"
#include "LaunchPoint.h"
#include "ApplicationDescription.h"

#include <cjson/json.h>

////public:
namespace DimensionsSystemInterface
{

PageRestore::PageRestore()
{
}

//virtual
PageRestore::~PageRestore()
{

}

//static
QList<QVariantMap> PageRestore::restoreLauncher(const QString& masterSaveFilepath)
{
	QList<QVariantMap> pageInfoList = processMasterFile(masterSaveFilepath);
	if (pageInfoList.isEmpty())
	{
		return QList<QVariantMap>();
	}

	/*
	map[PageSaver::SaveTagKey_PageType] = pageType;
	map[PageSaver::SaveTagKey_PageFile] = filePath;
	map[PageSaver::SaveTagKey_PageIndex] = pageIndex;
	*/

	for (QList<QVariantMap>::iterator it = pageInfoList.begin();
			it != pageInfoList.end();++it)
	{
		QVariantMap pageMap = restorePage(it->value(PageSaver::SaveTagKey_PageFile,QString("")).toString());
		if (pageMap.isEmpty())
		{
			continue;
		}

		/*
		 * rmap[PageSaver::SaveTagKey_PageType]
		 * rmap[PageSaver::SaveTagKey_PageName]
		 * rmap[PageSaver::SaveTagKey_PageDesignator]
		 * rmap[PageSaver::SaveTagKey_PageUuid]
		 * rmap[PageSaver::SaveTagKey_PageAppUidList]
		 */
		it->insert(PageSaver::SaveTagKey_PageRestoreObjectList,pageMap.value(PageSaver::SaveTagKey_PageRestoreObjectList));
		it->insert(PageSaver::SaveTagKey_PageName,pageMap.value(PageSaver::SaveTagKey_PageName));
		it->insert(PageSaver::SaveTagKey_PageDesignator,pageMap.value(PageSaver::SaveTagKey_PageDesignator));
		it->insert(PageSaver::SaveTagKey_PageUid,pageMap.value(PageSaver::SaveTagKey_PageUid));
	}
	return pageInfoList;
}

//static
QVariantMap PageRestore::restoreQuickLaunch(const QString& quicklaunchSaveFilepath)
{
	return (processQuicklaunchFile(quicklaunchSaveFilepath));
}

//static
QVariantMap PageRestore::restorePage(const QString& pageSaveFilepath)
{
	if (pageSaveFilepath.isEmpty())
	{
		return QVariantMap();
	}
	SafeFileOperator safesave(SafeFileOperator::Read,pageSaveFilepath,QSettings::IniFormat);
	QSettings& settings = safesave.safeSettings();

	if (settings.status() != QSettings::NoError)
	{
		//problem with the file op
		return QVariantMap();
	}

	//check the page type...

	settings.beginGroup("header");
	QString pageType = settings.value(PageSaver::SaveTagKey_PageType,QString("")).toString();
	QString pageName = settings.value(PageSaver::SaveTagKey_PageName,QString("")).toString();
	QString pageDesignator = settings.value(PageSaver::SaveTagKey_PageDesignator,QString("")).toString();
	QString pageUid = settings.value(PageSaver::SaveTagKey_PageUid,QString("")).toString();
	settings.endGroup();

	if (pageType != QString(ReorderablePage::staticMetaObject.className()))
	{
		//TODO: the ReorderablePage assumption...
		return QVariantMap();
	}

	QVariantMap rmap;
	rmap[PageSaver::SaveTagKey_PageType] = pageType;
	if (!pageName.isEmpty())
	{
		rmap[PageSaver::SaveTagKey_PageName] = pageName;
	}
	if (!pageDesignator.isEmpty())
	{
		rmap[PageSaver::SaveTagKey_PageDesignator] = pageDesignator;
	}
	if (!pageUid.isEmpty())
	{
		rmap[PageSaver::SaveTagKey_PageUid] = pageUid;
	}

	QList<WebOSAppRestoreObject> restoreObjectList = restoreReorderablePage(settings,pageUid);
	if (restoreObjectList.isEmpty())
	{
		return rmap;
	}

	QVariant appListV;
	appListV.setValue(restoreObjectList);
	rmap[PageSaver::SaveTagKey_PageRestoreObjectList] = appListV;

	return rmap;
}

//static
QPair<QString,quint32> PageRestore::itemPositionAsStoredOnDisk(const QString& webOsAppId)
{
	QMap<QString,QPair<QString,quint32> >::const_iterator f = s_positionsAsStoredOnDiskMap.constFind(webOsAppId);
	if (f != s_positionsAsStoredOnDiskMap.constEnd())
	{
		return f.value();
	}
	return QPair<QString,quint32>();
}
//

//static
QString PageRestore::pageSaveFilepathFromPageTag(const QVariantMap& pageTag)
{
	//figure out the filepath based on the tag
		return
		( OperationalSettings::settings()->savedPagesDirectory + QString("/")
							+ QString("page_")
							+ pageTag[PageSaver::SaveTagKey_PageType].toString()
							+ QString("_")+pageTag[PageSaver::SaveTagKey_PageName].toString()
							+ QString("_")+pageTag[PageSaver::SaveTagKey_PageUid].toString()
		);
}

//static
QString PageRestore::selectMasterFile(const QList<QString>& masterFileList,PageRestoreMSaveFileSelector::Enum selector,QVariant selectValue)
{
	if (masterFileList.isEmpty())
	{
		return QString();
	}
	if ((selector == PageRestoreMSaveFileSelector::INVALID)
		|| (selector == PageRestoreMSaveFileSelector::Any))
	{
		//grab the first file and bail
		return masterFileList.front();
	}

	if (selector == PageRestoreMSaveFileSelector::MostRecent)
	{
		//scan the master list and select the one with the most recent
		// time created
		quint64 timeMostRecent = 0;
		QString fileMostRecent;
		for (QList<QString>::const_iterator it = masterFileList.constBegin();
				it != masterFileList.constEnd();++it)
		{
			quint64 fileTime = 0;
			QVariantMap headerMap = masterFileStat(*it);

			//check the save system version...if it's incorrect, then ignore the file
			if (!PageRestore::isMasterFileCompatibleWithCurrentSaveSysVersion(headerMap))
			{
				qDebug() << __FUNCTION__ << ": skipping " << (*it) << " because it's incompatible with the current system version";
				continue;
			}

			if (masterfileTimeStamp(headerMap,fileTime))
			{
				if (fileTime > timeMostRecent)
				{
					timeMostRecent = fileTime;
					fileMostRecent = *it;
				}
			}
		}
		if (!fileMostRecent.isEmpty())
		{
			return fileMostRecent;
		}
		return QString();
	}

	if ((selector == PageRestoreMSaveFileSelector::SpecificName)
		&& 	(!(selectValue.toString().isEmpty())))
	{
		for (QList<QString>::const_iterator it = masterFileList.constBegin();
				it != masterFileList.constEnd();++it)
		{
			QString simpleName;
			QVariantMap headerMap = masterFileStat(*it);

			//check the save system version...if it's incorrect, then ignore the file
			if (!PageRestore::isMasterFileCompatibleWithCurrentSaveSysVersion(headerMap))
			{
				qDebug() << __FUNCTION__ << ": skipping " << (*it) << " because it's incompatible with the current system version";
				continue;
			}

			if (masterfileSimpleName(headerMap,simpleName))
			{
				if (simpleName == selectValue.toString())
				{
					return *it;
				}
			}
		}
		return QString();
	}

	return QString();
}

//static
QList<QString> PageRestore::scanForSavedMasterFiles()
{
	QList<QString> rlist;
	QDir dir = QDir(OperationalSettings::settings()->savedPagesDirectory);
	dir.setFilter(QDir::Files | QDir::Hidden);
	QStringList filters;
	filters << "*.msave";
	dir.setNameFilters(filters);
	QFileInfoList list = dir.entryInfoList();
	//qDebug() << "processing dir " << dir.absolutePath() << " , " << list.size() << " entries";
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		//qDebug() << qPrintable(QString("Processing %1").arg(fileInfo.fileName()));
		rlist << fileInfo.absoluteFilePath();
	}
	return rlist;
}

//static
QVariantMap PageRestore::masterFileStat(const QString& filePath)
{
	if (filePath.isEmpty())
	{
		return QVariantMap();
	}

	SafeFileOperator safesave(SafeFileOperator::Read,filePath,QSettings::IniFormat);
	QSettings& settings = safesave.safeSettings();

	if (settings.status() != QSettings::NoError)
	{
		//problem with the file op
		return QVariantMap();
	}

	QVariantMap rmap;
	//encode the filename into the map for convenience to the caller
	rmap[PageSaver::MasterTagHeaderKey_FileName] = filePath;

	//grab the timestamp
	settings.beginGroup("header");
	quint64 timeStamp = settings.value(PageSaver::MasterTagHeaderKey_TimeStamp,0).toULongLong();
	if (timeStamp)
	{
		rmap[PageSaver::MasterTagHeaderKey_TimeStamp] = timeStamp;
	}

	//find the save system version
	quint32 saveSysVersion = (quint32)(settings.value(PageSaver::MasterTagHeaderKey_SaveSystemVersion,0).toUInt());
	if (saveSysVersion != 0)
	{
		rmap[PageSaver::MasterTagHeaderKey_SaveSystemVersion] = saveSysVersion;
	}

	//and the simple/short name
	QString simpleName = settings.value(PageSaver::MasterTagHeaderKey_SimpleName,QString("")).toString();
	if (!simpleName.isEmpty())
	{
		rmap[PageSaver::MasterTagHeaderKey_SimpleName] = simpleName;
	}

	settings.endGroup();
	//count the number of pages saved in this master file
	int numPages = settings.beginReadArray("pages");
	rmap[PageSaver::MasterTagHeaderKey_NumPages] = numPages;

	return rmap;
}

//static
QList<QVariantMap> PageRestore::processMasterFile(const QString& masterFilepath)
{
	QList<QVariantMap> rlist;

	SafeFileOperator safesave(SafeFileOperator::Read,masterFilepath,QSettings::IniFormat);
	QSettings& settings = safesave.safeSettings();

	if (settings.status() != QSettings::NoError)
	{
		//problem with the file op
		return QList<QVariantMap>();
	}

	//check the save system version
	settings.beginGroup("header");
	quint32 saveSysVersion = (quint32)(settings.value(PageSaver::MasterTagHeaderKey_SaveSystemVersion,0).toUInt());
	if (saveSysVersion != PageSaver::saveSystemVersion())
	{
		//incompatible file
		return QList<QVariantMap>();
	}
	settings.endGroup();

	int numPages = settings.beginReadArray("pages");
	if (numPages == 0)
	{
		//no pages
		return QList<QVariantMap>();
	}
	for (int i = 0;i < numPages;++i)
	{
		settings.setArrayIndex(i);
		QString pageType = settings.value(PageSaver::SaveTagKey_PageType,QString("")).toString();
		//TODO: TEMP: only handling ReorderablePage at the moment
		if (pageType != QString(ReorderablePage::staticMetaObject.className()))
		{
			continue;
		}
		QString filePath = settings.value(PageSaver::SaveTagKey_PageFile,QString("")).toString();
		if (filePath.isEmpty())
		{
			continue;
		}
		qint32 pageIndex = settings.value(PageSaver::SaveTagKey_PageIndex,-1).toInt();

		QVariantMap map;
		map[PageSaver::SaveTagKey_PageType] = pageType;
		map[PageSaver::SaveTagKey_PageFile] = filePath;
		map[PageSaver::SaveTagKey_PageIndex] = pageIndex;
		rlist << map;
	}
	return rlist;
}

typedef QMap<QString,QVariant>::const_iterator VariantMapConstIter;
typedef QMap<QString,QVariant>::iterator VariantMapIter;

//static
bool PageRestore::masterfileSaveSysVersion(const QVariantMap& masterFileHeader,quint32& r_saveSysVersion)
{
	VariantMapConstIter it = masterFileHeader.constFind(PageSaver::MasterTagHeaderKey_SaveSystemVersion);
	if (it != masterFileHeader.constEnd())
	{
		r_saveSysVersion = (quint32)(it.value().toUInt());
		return true;
	}
	return false;
}

//static
bool PageRestore::isMasterFileCompatibleWithCurrentSaveSysVersion(const QVariantMap& masterFileHeader)
{
	quint32 version = 0;
	if (!masterfileSaveSysVersion(masterFileHeader,version))
	{
		return false;
	}
	return (version == PageSaver::saveSystemVersion());
}

//static
bool PageRestore::masterfileTimeStamp(const QVariantMap& masterFileHeader,quint64& r_timeStamp)
{
	VariantMapConstIter it = masterFileHeader.constFind(PageSaver::MasterTagHeaderKey_TimeStamp);
	if (it != masterFileHeader.constEnd())
	{
		r_timeStamp = it.value().toULongLong();
		return true;
	}
	return false;
}

//static
bool PageRestore::masterfileSimpleName(const QVariantMap& masterFileHeader,QString& r_simpleName)
{
	VariantMapConstIter it = masterFileHeader.constFind(PageSaver::MasterTagHeaderKey_SimpleName);
	if (it != masterFileHeader.constEnd())
	{
		r_simpleName = it.value().toString();
		return true;
	}
	return false;
}

//static
bool PageRestore::masterfileNumPages(const QVariantMap& masterFileHeader,quint32& r_numPages)
{
	VariantMapConstIter it = masterFileHeader.constFind(PageSaver::MasterTagHeaderKey_NumPages);
	if (it != masterFileHeader.constEnd())
	{
		r_numPages = it.value().toUInt();
		if (r_numPages > INT_MAX)		//means that most likely a negative int was specified
		{
			r_numPages = 0;
			return false;
		}
		return true;
	}
	return false;
}

/// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static
QString PageRestore::selectQuicklaunchFile(const QList<QString>& quickLaunchFileList,QuickLaunchRestoreSaveFileSelector::Enum selector,QVariant selectValue)
{
	if (quickLaunchFileList.isEmpty())
	{
		return QString();
	}
	if ((selector == QuickLaunchRestoreSaveFileSelector::INVALID)
		|| (selector == QuickLaunchRestoreSaveFileSelector::Any))
	{
		//grab the first file and bail
		return quickLaunchFileList.front();
	}

	if (selector == QuickLaunchRestoreSaveFileSelector::MostRecent)
	{
		//scan the master list and select the one with the most recent
		// time created
		quint64 timeMostRecent = 0;
		QString fileMostRecent;
		for (QList<QString>::const_iterator it = quickLaunchFileList.constBegin();
				it != quickLaunchFileList.constEnd();++it)
		{
			quint64 fileTime = 0;
			QVariantMap headerMap = quicklaunchFileStat(*it);
			//check version
			if (!isQuicklaunchFileCompatibleWithCurrentSaveSysVersion(headerMap))
			{
				qDebug() << __FUNCTION__ << ": skipping " << (*it) << " because it's incompatible with the current system version";
				continue;
			}
			if (quicklaunchFileTimeStamp(headerMap,fileTime))
			{
				if (fileTime > timeMostRecent)
				{
					timeMostRecent = fileTime;
					fileMostRecent = *it;
				}
			}
		}
		if (!fileMostRecent.isEmpty())
		{
			return fileMostRecent;
		}
		return QString();
	}

	if ((selector == QuickLaunchRestoreSaveFileSelector::SpecificName)
		&& 	(!(selectValue.toString().isEmpty())))
	{
		for (QList<QString>::const_iterator it = quickLaunchFileList.constBegin();
				it != quickLaunchFileList.constEnd();++it)
		{
			QString simpleName;
			QVariantMap headerMap = quicklaunchFileStat(*it);
			//check version
			if (!isQuicklaunchFileCompatibleWithCurrentSaveSysVersion(headerMap))
			{
				qDebug() << __FUNCTION__ << ": skipping " << (*it) << " because it's incompatible with the current system version";
				continue;
			}

			if (quicklaunchFileSimpleName(headerMap,simpleName))
			{
				if (simpleName == selectValue.toString())
				{
					return *it;
				}
			}
		}
		return QString();
	}

	return QString();
}

//static
QList<QString> PageRestore::scanForSavedQuicklaunchFiles()
{
	QList<QString> rlist;
	QDir dir = QDir(OperationalSettings::settings()->savedPagesDirectory);
	dir.setFilter(QDir::Files | QDir::Hidden);
	QStringList filters;
	filters << "*.qlsave";
	dir.setNameFilters(filters);
	QFileInfoList list = dir.entryInfoList();
	//qDebug() << "processing dir " << dir.absolutePath() << " , " << list.size() << " entries";
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		//qDebug() << qPrintable(QString("Processing %1").arg(fileInfo.fileName()));
		rlist << fileInfo.absoluteFilePath();
	}
	return rlist;
}

//static
QVariantMap PageRestore::quicklaunchFileStat(const QString& filePath)
{
	if (filePath.isEmpty())
	{
		return QVariantMap();
	}

	SafeFileOperator safesave(SafeFileOperator::Read,filePath,QSettings::IniFormat);
	QSettings& settings = safesave.safeSettings();

	if (settings.status() != QSettings::NoError)
	{
		//problem with the file op
		return QVariantMap();
	}

	QVariantMap rmap;
	//encode the filename into the map for convenience to the caller
	rmap[PageSaver::QuicklaunchTagHeaderKey_FileName] = filePath;

	//grab the timestamp
	settings.beginGroup("header");
	quint32 saveSysVersion = settings.value(PageSaver::QuicklaunchTagHeaderKey_SaveSystemVersion,0).toUInt();
	if (saveSysVersion != 0)
	{
		rmap[PageSaver::QuicklaunchTagHeaderKey_SaveSystemVersion] = saveSysVersion;
	}
	quint64 timeStamp = settings.value(PageSaver::QuicklaunchTagHeaderKey_TimeStamp,0).toULongLong();
	if (timeStamp)
	{
		rmap[PageSaver::QuicklaunchTagHeaderKey_TimeStamp] = timeStamp;
	}
	QString simpleName = settings.value(PageSaver::QuicklaunchTagHeaderKey_SimpleName,QString("")).toString();
	if (!simpleName.isEmpty())
	{
		rmap[PageSaver::QuicklaunchTagHeaderKey_SimpleName] = simpleName;
	}

	settings.endGroup();

	return rmap;
}

//static
QVariantMap PageRestore::processQuicklaunchFile(const QString& filepath)
{
	SafeFileOperator safesave(SafeFileOperator::Read,filepath,QSettings::IniFormat);
	QSettings& settings = safesave.safeSettings();

	if (settings.status() != QSettings::NoError)
	{
		//problem with the file op
		return QVariantMap();
	}

	//check the save system version
	settings.beginGroup("header");
	quint32 saveSysVersion = (quint32)(settings.value(PageSaver::QuicklaunchTagHeaderKey_SaveSystemVersion,0).toUInt());
	if (saveSysVersion != PageSaver::saveSystemVersion())
	{
		//incompatible file
		return QVariantMap();
	}
	settings.endGroup();

	QList<WebOSAppRestoreObject> appRestoreList;
	int numIcons = settings.beginReadArray("icons");
	for (int i = 0;i < numIcons;++i)
	{
		//TODO: TEMP: see PageSaver for WebOSApp-only restriction
		settings.setArrayIndex(i);
		QString appId = settings.value("id",QString("")).toString();
		if (appId.isEmpty())
		{
			continue;
		}
		//look up the app uid in the AppMonitor
		WebOSApp * pWoApp = AppMonitor::appMonitor()->webosAppByAppId(appId);
		if (pWoApp == 0)
		{
			continue;
		}

		WOAppIconType::Enum t = (WOAppIconType::Enum)(settings.value("launchtype",(int)(WOAppIconType::INVALID))).toInt();
		QString launchpointId = settings.value("launchid",QString("")).toString();

		appRestoreList << WebOSAppRestoreObject(pWoApp->uid(),appId,launchpointId,i);
	}

	QVariantMap rmap;
	QVariant appListV;
	appListV.setValue(appRestoreList);
	rmap[PageSaver::SaveTagKey_PageRestoreObjectList] = appListV;
	return rmap;
}

typedef QMap<QString,QVariant>::const_iterator VariantMapConstIter;
typedef QMap<QString,QVariant>::iterator VariantMapIter;

//static
bool PageRestore::quicklaunchFileTimeStamp(const QVariantMap& quicklaunchFileHeader,quint64& r_timeStamp)
{
	VariantMapConstIter it = quicklaunchFileHeader.constFind(PageSaver::QuicklaunchTagHeaderKey_TimeStamp);
	if (it != quicklaunchFileHeader.constEnd())
	{
		r_timeStamp = it.value().toULongLong();
		return true;
	}
	return false;
}

//static
bool PageRestore::quicklaunchFileSimpleName(const QVariantMap& quicklaunchFileHeader,QString& r_simpleName)
{
	VariantMapConstIter it = quicklaunchFileHeader.constFind(PageSaver::QuicklaunchTagHeaderKey_SimpleName);
	if (it != quicklaunchFileHeader.constEnd())
	{
		r_simpleName = it.value().toString();
		return true;
	}
	return false;
}

//static
bool PageRestore::quicklaunchFileSaveSysVersion(const QVariantMap& quicklaunchFileHeader,quint32& r_saveSysVersion)
{
	VariantMapConstIter it = quicklaunchFileHeader.constFind(PageSaver::QuicklaunchTagHeaderKey_SaveSystemVersion);
	if (it != quicklaunchFileHeader.constEnd())
	{
		r_saveSysVersion = (quint32)(it.value().toUInt());
		return true;
	}
	return false;
}

//static
bool PageRestore::isQuicklaunchFileCompatibleWithCurrentSaveSysVersion(const QVariantMap& quicklaunchFileHeader)
{
	quint32 version = 0;
	if (!quicklaunchFileSaveSysVersion(quicklaunchFileHeader,version))
	{
		return false;
	}
	return (version == PageSaver::saveSystemVersion());
}

//static
bool PageRestore::convertLegacyJsonQuicklaunchConfig(const QString& sourceFilepath,const QString& destinationFilepath)
{
	json_object * root = json_object_from_file(sourceFilepath.toAscii().data());
	if (!root || is_error(root))
	{
		return false;
	}

	/*
	 * {
	"quicklauncher": [
		"com.palm.app.calculator_default",
		"com.palm.app.enyo-contacts_default",
		"com.palm.app.enyo-email_default",
		"com.palm.app.enyo-calendar_default"
		]
		}
	 */

	json_object * jsonArray = json_object_object_get(root,"quicklauncher");
	if (!jsonArray || is_error(jsonArray))
	{
		json_object_put(root);
		return false;
	}
	if (!json_object_is_type(jsonArray,json_type_array))
	{
		json_object_put(root);
		return false;
	}

	//try and create the output file in the right format
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
	QString quickLaunchFilepath =
		( destinationFilepath.isEmpty()
			? OperationalSettings::settings()->savedPagesDirectory + QString("/") + QString("quicklaunch_")+nameTimeFragment+QString(".qlsave")		///WARNING: don't change extension or the file scanner for restore won't work (see pagerestore.cpp)
			: destinationFilepath
		);

	SafeFileOperator safesave(SafeFileOperator::Write,quickLaunchFilepath,QSettings::IniFormat);
	QSettings& saver = safesave.safeSettings();

	if (saver.status() != QSettings::NoError)
	{
		//problem with the file op
		json_object_put(root);
		return false;
	}

	saver.beginGroup("header");
	saver.setValue(PageSaver::QuicklaunchTagHeaderKey_TimeStamp,timeStamp);
	saver.setValue(PageSaver::QuicklaunchTagHeaderKey_SimpleName,QString("default"));
	saver.setValue(PageSaver::QuicklaunchTagHeaderKey_SaveSystemVersion,PageSaver::saveSystemVersion());
	saver.endGroup();

	saver.beginWriteArray("icons");
	int numItems = json_object_array_length(jsonArray);
	int idx=0;
	for (int i = 0;i < numItems;++i)
	{
		json_object * jsonLaunchptItem = json_object_array_get_idx(jsonArray,i);
		if (!jsonLaunchptItem || is_error(jsonLaunchptItem))
		{
			continue;
		}
		QString launchpointId = QString(json_object_get_string(jsonLaunchptItem));
		QString appId = AppMonitor::appIdFromLaunchpointId(launchpointId);
		if (appId.isEmpty())
		{
			continue;
		}
		saver.setArrayIndex(idx++);
		saver.setValue("id",appId);
		//qDebug() << __FUNCTION__ << ": saved appid = " << appId << " at index " << idx-1;
	}
	saver.endArray();
	json_object_put(root);
	return true;
}

//static
QList<QPair<QString,QList<QString> > > PageRestore::loadStaticLegacyLauncherConfig(const QString& sourceFilepath)
{

	QList<QPair<QString,QList<QString> > > pagelist;

	json_object * root = json_object_from_file(sourceFilepath.toAscii().data());
	if (!root || is_error(root))
	{
		return pagelist;
	}

	//"root" is an array - ugh. questionable json

	QSet<QString> duplicateEliminator;		//because I don't trust anyone, anymore

	if (!json_object_is_type(root,json_type_array))
	{
		json_object_put(root);
		return pagelist;
	}
/*
	[
	    {
	        "title" : "Palm",
	        "items" : [
	            "com.palm.app.phone_default",
	            ...
	        ]
	    },
	    {
	        "title" : "Services",
	        "items" : [
	            ...
	        ]
	    },
	    ...
	    ]
	    */

	int numPageItems = json_object_array_length(root);
	for (int pageIdx=0;pageIdx<numPageItems;++pageIdx)
	{
		json_object * pageItem = json_object_array_get_idx(root,pageIdx);
		if (!pageItem || is_error(pageItem))
		{
			continue;
		}
		json_object * label = json_object_object_get(pageItem,"title");
		if (!label || is_error(label))
		{
			continue;
		}
		QString designator = QString(json_object_get_string(label)).toLower();
		if (designator.isEmpty())
		{
			continue;
		}
		json_object * jsonItemArray = json_object_object_get(pageItem,"items");
		if (!jsonItemArray || is_error(jsonItemArray))
		{
			continue;
		}
		if (!json_object_is_type(jsonItemArray,json_type_array))
		{
			continue;
		}
		int numLaunchpointItems = json_object_array_length(jsonItemArray);
		QList<QString> rlist;
		for (int itemIdx=0;itemIdx<numLaunchpointItems;++itemIdx)
		{
			json_object * launchptItem = json_object_array_get_idx(jsonItemArray,itemIdx);
			if (!launchptItem || is_error(launchptItem))
			{
				continue;
			}
			QString itemName = json_object_get_string(launchptItem);
			if (itemName.isEmpty())
			{
				continue;
			}

			if (duplicateEliminator.contains(itemName))
			{
				continue;
			}
			duplicateEliminator.insert(itemName);
			rlist << itemName;
		}
		if (!rlist.isEmpty())
		{
			pagelist << QPair<QString,QList<QString> >(designator,rlist);
		}
	} //end loop pages
	json_object_put(root);
	return pagelist;
}

////protected:

//static
QList<WebOSAppRestoreObject> PageRestore::restoreReorderablePage(QSettings& settings,const QString& pageUid)
{
	//TODO: TEMP: improve this for features and completeness...for now, just skip to the icons array and read them
	int numIcons = settings.beginReadArray("icons");
	QList<WebOSAppRestoreObject> rlist;
	if (numIcons == 0)
	{
		return rlist;
	}

	for (int i = 0;i < numIcons;++i)
	{
		//TODO: TEMP: see PageSaver for WebOSApp-only restriction
		settings.setArrayIndex(i);
		QString appId = settings.value("id",QString("")).toString();
		if (appId.isEmpty())
		{
			continue;
		}

		//	//TODO: HF DFISH-14598
		s_positionsAsStoredOnDiskMap[appId] = QPair<QString,quint32>(pageUid,i);
		//

		//look up the app uid in the AppMonitor
		WebOSApp * pWoApp = AppMonitor::appMonitor()->webosAppByAppId(appId);
		if (pWoApp == 0)
		{
			continue;
		}

		WOAppIconType::Enum t = (WOAppIconType::Enum)(settings.value("launchtype",(int)(WOAppIconType::INVALID))).toInt();
		QString launchpointId = settings.value("launchid",QString("")).toString();

		rlist << WebOSAppRestoreObject(pWoApp->uid(),appId,launchpointId,i);
	}

	return rlist;
}

} //end namespace
