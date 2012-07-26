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




#ifndef PAGESAVER_H_
#define PAGESAVER_H_

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QString>
#include <QList>

class LauncherObject;
class QuickLaunchBar;
class Page;
class ReorderablePage;

namespace DimensionsSystemInterface
{

class PageSaver : public QObject
{
	Q_OBJECT
public:

	static QString SaveTagKey_PageType;
	static QString SaveTagKey_PageName;
	static QString SaveTagKey_PageDesignator;		//a "category" like name to keep track of pages that serve a particular purpose
	static QString SaveTagKey_PageUid;
	static QString SaveTagKey_PageFile;
	static QString SaveTagKey_PageIndex;
	static QString SaveTagKey_PageNumIcons;
	static QString SaveTagKey_PageRestoreObjectList;

	static QString MasterTagHeaderKey_TimeStamp;
	static QString MasterTagHeaderKey_SimpleName;
	static QString MasterTagHeaderKey_FileName;
	static QString MasterTagHeaderKey_NumPages;
	static QString MasterTagHeaderKey_SaveSystemVersion;

	static QString QuicklaunchTagHeaderKey_TimeStamp;
	static QString QuicklaunchTagHeaderKey_SimpleName;
	static QString QuicklaunchTagHeaderKey_FileName;
	static QString QuicklaunchTagHeaderKey_SaveSystemVersion;

	//this is used to prevent issues from older versions of save files from causing problems
	// with newer versions of code. It's expected to be changed pretty heavily during development, and
	// not so much afterwards...but it's far easier to do this than have to explain company-wide on how to
	// remove saved files and the whole gamut of issues that manifest if this isn't done at the necessary times
	static quint32 SaveSystemVersion;

	PageSaver();
	virtual ~PageSaver();

	static QVariantMap savePage(Page * p_page,LauncherObject * p_uiContext);
	static bool	saveLauncher(LauncherObject * p_uiContext);
	static bool saveQuickLaunch(LauncherObject * p_uiContext,QuickLaunchBar * p_quickLauncher);

	static quint32 saveSystemVersion() { return SaveSystemVersion; }

	static PageSaver * instance();

	static void filesForBackup(QList<QString> * pFileList);

	static void dbgPackUpAndSaveCurrentLauncher3Dir(const QString& extraFilenameTag,QList<QVariantMap> * p_dbgLauncherConf = 0);

protected:

	static QPointer<PageSaver> s_qp_instance;
	static QVariantMap saveReorderablePage(ReorderablePage * p_reorderPage,LauncherObject * p_uiContext);

};

} //end namespace

#endif /* PAGESAVER_H_ */
