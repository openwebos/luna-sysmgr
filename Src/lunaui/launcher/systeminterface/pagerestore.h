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




#ifndef PAGERESTORE_H_
#define PAGERESTORE_H_

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QList>
#include <QUuid>
#include <QVariantMap>
#include <QMap>
#include <QString>
#include <QPair>

#include "safefileops.h"

class DimensionsUI;
class Page;
class ReorderablePage;

namespace DimensionsSystemInterface
{

namespace PageRestoreMSaveFileSelector
{
	enum Enum
	{
		INVALID,
		Any,
		MostRecent,
		SpecificName
	};
}

namespace QuickLaunchRestoreSaveFileSelector
{
	enum Enum
	{
		INVALID,
		Any,
		MostRecent,
		SpecificName
	};
}

class WebOSAppRestoreObject
{
public:
	WebOSAppRestoreObject() : m_valid(false) {}
	WebOSAppRestoreObject(const QUuid& appuid,const QString& appid,const QString& launchpointid,quint32 pos)
	: m_valid(true) , appUid(appuid) , appId(appid) , launchpointId(launchpointid) , positionInSaveFile(pos) {}

	bool m_valid;
	QUuid appUid;
	QString appId;
	QString launchpointId;
	quint32 positionInSaveFile;
};

static QMap<QString,QPair<QString,quint32> > s_positionsAsStoredOnDiskMap;
//

class PageRestore : public QObject
{
	Q_OBJECT
public:
	PageRestore();
	virtual ~PageRestore();

	//return the current set of app uids based on the apps stored in the settings file
	// the uids are looked up in the AppMonitor
	static QList<QVariantMap> restoreLauncher(const QString& masterSaveFilepath);
	static QVariantMap restoreQuickLaunch(const QString& quicklaunchSaveFilepath);

	static QVariantMap restorePage(const QString& pageSaveFilepath);
	static QString pageSaveFilepathFromPageTag(const QVariantMap& pageTag);

	//// ---- MASTER FILE HANDLING ------
	static QString selectMasterFile(const QList<QString>& masterFileList,
								PageRestoreMSaveFileSelector::Enum selector,
								QVariant selectValue = QVariant());
	static QList<QString> scanForSavedMasterFiles();
	static QVariantMap masterFileStat(const QString& filePath);
	static QList<QVariantMap> processMasterFile(const QString& masterFilepath);

	static bool masterfileSaveSysVersion(const QVariantMap& masterFileHeader,quint32& r_saveSysVersion);
	static bool masterfileTimeStamp(const QVariantMap& masterFileHeader,quint64& r_timeStamp);
	static bool masterfileSimpleName(const QVariantMap& masterFileHeader,QString& r_simpleName);
	static bool masterfileNumPages(const QVariantMap& masterFileHeader,quint32& r_numPages);

	static bool isMasterFileCompatibleWithCurrentSaveSysVersion(const QVariantMap& masterFileHeader);

	//// ---- QUICK LAUNCH FILE HANDLING
	static QString selectQuicklaunchFile(const QList<QString>& quickLaunchFileList,
			QuickLaunchRestoreSaveFileSelector::Enum selector,
			QVariant selectValue = QVariant());
	static QList<QString> scanForSavedQuicklaunchFiles();
	static QVariantMap quicklaunchFileStat(const QString& filepath);
	static QVariantMap processQuicklaunchFile(const QString& filepath);

	static bool quicklaunchFileTimeStamp(const QVariantMap& quicklaunchFileHeader,quint64& r_timeStamp);
	static bool quicklaunchFileSimpleName(const QVariantMap& quicklaunchFileHeader,QString& r_simpleName);
	static bool quicklaunchFileSaveSysVersion(const QVariantMap& quicklaunchFileHeader,quint32& r_saveSysVersion);
	static bool isQuicklaunchFileCompatibleWithCurrentSaveSysVersion(const QVariantMap& quicklaunchFileHeader);

	static bool convertLegacyJsonQuicklaunchConfig(const QString& sourceFilepath,const QString& destinationFilepath=QString());
	static QList<QPair<QString,QList<QString> > > loadStaticLegacyLauncherConfig(const QString& sourceFilePath);

	static QPair<QString,quint32> itemPositionAsStoredOnDisk(const QString& webOsAppId);
	//
protected:

	static QList<WebOSAppRestoreObject> restoreReorderablePage(QSettings& savedSettingsObj,const QString& pageUid);

};

} //end namespace

Q_DECLARE_METATYPE (QUuid)
Q_DECLARE_METATYPE (QList<QUuid>)
Q_DECLARE_METATYPE (DimensionsSystemInterface::WebOSAppRestoreObject)
Q_DECLARE_METATYPE (QList<DimensionsSystemInterface::WebOSAppRestoreObject>)

#endif /* PAGERESTORE_H_ */
