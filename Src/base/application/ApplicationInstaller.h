/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef APPLICATIONINSTALLER_H_
#define APPLICATIONINSTALLER_H_

#include "Common.h"

#include <list>
#include <string>
#include <map>
#include <vector>
#include <glib.h>
#include <cjson/json.h>
#include <cjson/json_util.h>

#include <lunaservice.h>

#include "MutexLocker.h"

#include <QObject>

#include <sys/statfs.h>
#include <sys/statvfs.h>

class PackageDescription;

// for debug only
typedef int (*statfsfn)(const char *, struct statfs *);
typedef int (*statvfsfn)(const char *,struct statvfs *);

// ipkg install and ipkg remove processes will run with this priority instead of inheriting sysmgr's
#define IPKG_PROCESS_PRIORITY 1

class CommandParams {
public:
	enum Type {
		Install = 0,
		Remove
	};

	CommandParams(Type t) :
		_type(t), _childStdOutChannel(0), _childStdOutSource(0) {
	}
	
	virtual ~CommandParams() {
		if (_childStdOutChannel) {
			g_io_channel_unref(_childStdOutChannel);
		}
		if (_childStdOutSource) {
			g_source_destroy(_childStdOutSource);
			g_source_unref(_childStdOutSource);
		}
	}

	Type _type;
	GIOChannel* _childStdOutChannel;
	GSource* _childStdOutSource;
};

class InstallParams : public CommandParams {
public: 
	InstallParams(const std::string& target, const std::string& id, const unsigned long ticket, LSHandle * lshandle,const LSMessage * msg,const unsigned int uncompressedSizeInKB, bool verify = true, bool systemMode = false)
		: CommandParams(CommandParams::Install), _target(target) , _id(id), ticketId(ticket) , _lshandle(lshandle) , _msg(msg) , _verify(verify), _sysMode(systemMode), _uncompressedSizeInKB(uncompressedSizeInKB)
	{ }
	const std::string _target;
	const std::string _id;
	const unsigned long ticketId;
	LSHandle * _lshandle;
	const LSMessage * _msg;
	bool _verify;
	bool _sysMode;
	const unsigned int _uncompressedSizeInKB;
	std::string _packageId;
};

class RemoveParams : public CommandParams {
public: 
	RemoveParams(const std::string& packageName,const unsigned long ticket,LSHandle * lshandle,const LSMessage * msg,int cause) 
		: CommandParams(CommandParams::Remove), _packageName(packageName) , ticketId(ticket) , _lshandle(lshandle) , _msg(msg)  , _cause(cause)
	{}
	const std::string _packageName;
	const unsigned long ticketId;
	const LSHandle * _lshandle;
	const LSMessage * _msg;
	const int 		  _cause;
};

class ApplicationInstaller : public QObject
{
	Q_OBJECT

public:

	static ApplicationInstaller* instance();
	static std::string	s_installer_version;
	
	//Luna bus functions
	static bool cbInstallProgressQuery(LSHandle* lshandle, LSMessage *msg,void *user_data);
	static bool cbInstall(LSHandle* lshandle, LSMessage *msg,void *user_data);
	static bool cbInstallNoVerify(LSHandle* lshandle, LSMessage *msg,void *user_data);
	static bool cbRemove(LSHandle* lshandle, LSMessage *msg,void *user_data);
	static gboolean cbShallowRemove(gpointer param);
	static bool cbIsInstalled(LSHandle* lshandle, LSMessage *msg,void *user_data);
	static bool cbNotifyOnChange(LSHandle* lshandle, LSMessage *msg,void *user_data);
	static bool cbGetSizes(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbQueryInstallCapacity(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbDetermineInstallSpaceNeeded(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbRevoke(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbPubSubRegister(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool cbPubSubStatus(LSHandle* handle, LSMessage* msg, void* ctxt);
	
	static bool cbDbgGetPkgInfoFromStatusFile(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbDbgCopyDir(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool	cbDbgFakeFsSize(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool	cbDbgUnFakeFsSizes(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbGetFsSize(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbDbgFillSize(LSHandle* lshandle,LSMessage *msg,void *user_data);
	static bool cbDbgGetAppSizeOnFs(LSHandle* lshandle,LSMessage *msg,void *user_data);
	
	//Native interface (for direct calls w/in lunasysmgr)
	bool install(const std::string& targetPackageName, unsigned int uncompressedAppSizeInKB, const unsigned long ticket);
	void notifyAppInstalled(const std::string& appId,const std::string& appVersion);

	bool downloadAndInstall (LSHandle* handle, const std::string& targetPackageFile, struct json_object* authToken, struct json_object* deviceId,
			unsigned long ticket, bool subscribe);
	void oneCommandProcessed();

	static uint64_t getSizeOfAppDir(const std::string& dirName);
	
	static Mutex		s_sizeFnMutex;
	static uint64_t		s_sizeFnAccumulator;
	static uint64_t		s_auxSizeFnAccumulator;
	static uint64_t		s_targetFsBlockSize;
	static std::string  s_sizeFnBaseDir;
	static json_object * s_manifestJobj;
	
	static int _getSizeCbFn(const char *fpath, const struct stat *sb,int typeflag, struct FTW *ftwbuf);
	static int _getSizeOfAppCbFn(const char *fpath, const struct stat *sb,int typeflag, struct FTW *ftwbuf);
	
//	static uint64_t getSizeOfAppDir_opt(const std::string& dirName);
	static uint64_t getSizeOfAppOnFs(const std::string& destFsPath,const std::string& dirName,uint32_t * r_pBsize=NULL);
	static uint64_t getSizeOfPackageOnFsGenerateManifest(const std::string& destFsPath, PackageDescription* packageDesc, uint32_t * r_pBsize);
	static uint64_t getSizeOfPackageById(const std::string& packageId);

	static uint64_t getFsFreeSpaceInMB(const std::string& pathOnFs);
	static uint64_t getFsFreeSpaceInBlocks(const std::string& pathOnFs,uint64_t * pBlockSize = 0);
		
	static bool	extractVersionFromAppInfo(const std::string& appBaseDir,std::string& r_versionString);
	
#define APPREMOVED_CAUSE_UNKNOWN				-1
#define APPREMOVED_CAUSE_USERDELETED			0
#define APPREMOVED_CAUSE_APPREVOKED				1
	void notifyAppRemoved(const std::string& appId,const std::string& appVersion,int cause=APPREMOVED_CAUSE_UNKNOWN);
	static bool getDownloadPathBasedOnSpaceRemaining(std::string& packageDownloadPath);
	static bool isValidInstallURI(const std::string& url);

	bool allowSuspend();

	json_object * packageInfoFileToJson(const std::string& packageId);

private Q_SLOTS:

	void slotMediaPartitionAvailable(bool val);
	
private:

	ApplicationInstaller();
	~ApplicationInstaller();

	LSHandle* m_service;

	std::list<std::string>	m_appInstallBaseDirs;
	
	bool init();
	void startService();
	void stopService();

	void enterBrickMode();
	void exitBrickMode();

	bool lunasvcInstallProgressQuery(LSHandle* lshandle, LSMessage *msg,void *user_data);
	int lunasvcRemove(RemoveParams* params);
	bool lunasvcIsInstalled(LSHandle* lshandle, LSMessage *msg,void *user_data);
	bool lunasvcNotifyOnChange(const std::string& packageName,LSHandle * lshandle,LSMessage *msg);
	bool lunasvcGetInstalledSizes(LSHandle * lshandle,LSMessage *msg);
	int lunasvcQueryInstallCapacity(const std::string& packageId,uint64_t packageSizeInKB,uint64_t uncompressedPackageSizeInKB,uint64_t& r_spaceNeeded);

	void processOrQueueCommand(CommandParams* cmd);
	bool processInstallCommand(InstallParams* params);
	bool processRemoveCommand(RemoveParams* params);
	bool processNextCommand();

	void closeApp(const std::string& appId);

	static bool packageNameFromControl(const std::string& controlTarGzPathAndFile,const std::string& tempDir,std::string& return_PackageName);
	static int getAllUserInstalledAppNames(std::vector<std::string>& appList,std::string basePkgDirName);
	static bool findUserInstalledAppName(const std::string& packageName,const std::string& basePkgDirName);
	static int getAllUserInstalledAppSizes(std::vector<std::pair<std::string,uint64_t> >& appList,std::string basePkgDirName);

	static bool	arePathsOnSameFilesystem(const std::string& path1,const std::string& path2);

	static int doSignatureVerifyOnFile(const std::string& file,const std::string& signatureFile,const std::string& pubkeyFile);
	static int doSignatureVerifyOnFiles(std::vector<std::string>& files,const std::string& signatureFile,const std::string& pubkeyFile);
	static int extractPublicKeyFromCert(const std::string& certFile,const std::string& pubkeyFile);
	static int runOpenSSL(std::vector<std::string>& params,const std::string& command);
	static int runIpkgRemove(const std::string& ipkgRoot,const std::string& packageName);
	
	static std::list<CommandParams*> s_commandParams;
	bool m_inBrickMode;

	struct CommandState {
		CommandState() {
			reset();
		}
		
		void reset() {
			processing = false;
			sourceId = 0;
			pid = -1;
		}
			
		bool processing;
		guint sourceId;
		GPid pid;		
	};

	CommandState m_cmdState;
	
	//------------------------------------------------ DEBUG -----------------------------------------------------------
	
	static statfsfn	s_statfsFn;
	static statvfsfn s_statvfsFn;
	static int dbg_restoreFakeFsEntriesAtStartup();
	static int dbg_statfs(const char *, struct statfs *);
	static int dbg_statvfs(const char *,struct statvfs *);
	static int dbg_fill(std::string& path,uint32_t& bsize,uint32_t& nblocks);
	static int dbg_getAppSizeOnFs(const std::string& fspath,const std::string& appbasepath,uint64_t& r_size);
	static std::map<uint32_t,std::pair<uint32_t,uint32_t> > dbg_statfs_map;
	static std::map<uint32_t,std::pair<uint32_t,uint32_t> > dbg_statvfs_map;
	static json_object * dbg_statxfs_persistent;
	
}; 		//end class ApplicationInstaller

#endif /*APPLICATIONINSTALLER_H_*/
