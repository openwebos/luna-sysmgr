/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#include "Common.h"

#include "IpcServer.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "HostBase.h"
#include "IpcClientHost.h"
#include "WebAppMgrProxy.h"
#include "Settings.h"
#include "WindowServer.h"
#include "ApplicationManager.h"
#include "SystemService.h"
#include "MemoryMonitor.h"
#include "CpuAffinity.h"

#ifndef TARGET_DESKTOP	
	#include "rolegen.h"
#endif

// the prefix that comes before the appid in all app paths
const char *APP_PREFIX = "/media/cryptofs/apps/usr/palm/applications/";


static int kNukeProcessTimeoutMs = 2000;

static int PrvForkFunction(void* data) __attribute__((unused));
static pid_t PrvLaunchProcess(char* const args[]);

// strips one level of directory from the path
// /foo/bar/your mom/ becomes /foo/bar/
// /foo/bar/ becomes /foo/
// /foo/ becomes /
// This function was copied as-is from libhelpers
static void stripOneDirectoryLevel(char *path)
{
    // start off at the last character
    char *p = path + strlen(path) - 1;

    if ( p <= path )
    {
        // there's nothing left to do
        path[0] = 0;
        return;
    }

    // we use a do-while rather than a while to ensure
    // we take a step before checking. In most cases,
    // we will start off with p at a slash. wouldn't
    // want to suddenly end right there.
    do
    {
        p--;
        if ( p == path )
        {
            // we reached the beginning without ever finding a slash?
            // there's something weird going on here. just clear it out
            // I guess
            path[0] = 0;
            return;
        }
    } while ( *p != '/' );

    // p is currently sitting on a slash. We want
    // to be 1 character after that (because we want to keep
    // the slash character in the string)
    p++;

    // end the string here.
    *p = 0;

    // that's all there is to do.
}

// This function was copied as-is from libhelpers
static int HApp_GetBaseFile(const char *fullPath, char *outBuffer, int outBufferLen)
{
    char workingPath[PATH_MAX];
    strncpy(workingPath, fullPath, PATH_MAX);
    stripOneDirectoryLevel(workingPath);

    // did we strip the whole thing? If so, that means there was no path at all.
    if ( workingPath[0] == 0 )
    {
        // just give them back what they sent us. There's no path to remove
        strncpy(outBuffer, fullPath, outBufferLen);
        return 1;
    }

    // it stripped something. The part it stripped is the part we want.
    int len = strlen(workingPath);

    // we want to start our copying in the full path at the character
    // after the "/". That means taking the length of the stripped
    // path then adding that to the fullPath pointer. That'll be
    // the first character of the base name
    const char *startCopy = fullPath + len;
    strncpy(outBuffer, startCopy, outBufferLen);
    return 1;
}



IpcServer* IpcServer::instance()
{
	static IpcServer* s_server = 0;
	if (G_UNLIKELY(s_server == 0)) {
		s_server = new IpcServer;
	}

	return s_server;    
}

IpcServer::IpcServer()
	: PIpcServer("sysmgr", HostBase::instance()->mainLoop())
	, m_nukeProcessTimer(HostBase::instance()->masterTimer(), this,
						 &IpcServer::nukeProcessTimer)
{
	static const int kGroupId = 1000;  // group "luna"

	// flash needs this:
	chmod("/tmp/pipcserver.sysmgr", S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	chown((char*)"/tmp/pipcserver.sysmgr", 0, kGroupId);
}


IpcServer::~IpcServer()
{
}

void IpcServer::clientConnected(int pid, const std::string& name, PIpcChannel* channel)
{
	// Find the appid from the path of the file
	std::string appId = appIdFromPid(pid);
	if (appId.empty())
		appId = name;
	
    g_message("%s (%d): Client connected %s (%d)", __PRETTY_FUNCTION__, __LINE__,
			  appId.c_str(), pid);

	// Update/Add the new pid into the processMap
	ProcessMap::iterator it = m_nativeProcessMap.find(appId);
	if (it != m_nativeProcessMap.end()){
		if(pid != it->second)
		{
			// update the memory watcher for this process, in case there is one
			MemoryMonitor::instance()->monitorNativeProcessMemory(pid, 0, it->second);
		}
		m_nativeProcessMap.erase(it);
	}

	m_nativeProcessMap[appId] = pid;
	
	if (0 != strcmp(name.c_str(), "WebAppManager"/*FIXME:qtwebkit WEB_APP_MGR_IPC_NAME*/))
	{ // regular (native) app connecting
		IpcClientHost* c = new IpcClientHost(pid, appId, channel);
		m_clientHostSet.insert(c);
	} else {
		// WebKit WebApp Manager connecting
		WebAppMgrProxy* c = WebAppMgrProxy::connectWebAppMgr(pid, channel);
		m_clientHostSet.insert(c);
	}
	
}

/*
int IpcServer::launchNativeProcess(const std::string& appId, const char* path, char* const argv[], int requiredMemory)
{
	// default to "game" type
	return launchNativeProcess(appId,path,argv,"game",requiredMemory);
}
*/

int IpcServer::launchNativeProcess(const std::string& appId, const char* path, char* const argv[], ApplicationDescription::Type appType, int requiredMemory)
{
	// figure out the jailer to use for this app type
	const char *jailerType = "web"; // default is "web" type
	if ( appType == ApplicationDescription::Type_Native )
	{
		// native apps get "game"
		jailerType = "game";
	}
	else if ( appType == ApplicationDescription::Type_PDK 
           || appType == ApplicationDescription::Type_Qt )
	{
		// pdk apps get "pdk"
		jailerType = "pdk";
	}
	
	ProcessMap::const_iterator it = m_nativeProcessMap.find(appId);
	if (it != m_nativeProcessMap.end()) {
		g_debug("%s: Process %s (%s) already running. Not launching",
				__PRETTY_FUNCTION__, appId.c_str(), path);

		IpcClientHost* host = clientHostForAppId(appId);
		if (host)
			host->relaunch();
		
		return it->second;
	}
	
	if(!MemoryMonitor::instance()->allowNewNativeAppLaunch(requiredMemory)){ // not enough memory...
		g_warning("%s: Low memory condition Not allowing native app for appId: %s. RequiredMemory = %d",
				  __PRETTY_FUNCTION__, appId.c_str(), requiredMemory);
		WindowServer::instance()->appLaunchPreventedUnderLowMemory();
		WebAppMgrProxy::instance()->performLowMemoryActions(); // try to free up some memory and notify the user to close cards
		return 0; // return 0 for low memory failure
	}
	
	// role generation is not supported in desktop
#ifndef TARGET_DESKTOP	
	// push a role file. This will allow the native app to make
	// luna-service calls. This call is fast, and safe to call when the role file
	// already exists, so we just call it every launch.
	pdkGenerateRole(appId, path);
#endif

	// assemble the working path
	char workingPath[PATH_MAX];
	snprintf(workingPath, PATH_MAX, "%s%s", APP_PREFIX, appId.c_str());

	char* const minArgs[] = { (char*) path, NULL };
	char* const* args = argv;
	if (!args)
		args = minArgs;
	
	// build new arg list with stuff for jailer
	// first count existing args
	int i,e_argc;
	i = 0;
	for (e_argc = 0; args[e_argc]; e_argc++ );
	const char *newargs[e_argc + 14];

	// get the base file name
	char baseFile[PATH_MAX];
	HApp_GetBaseFile(path, baseFile, PATH_MAX);
	
	newargs[i++] = "/usr/sbin/setcpushares-pdk"; // the cpu share manager
	newargs[i++] = "-a";
	newargs[i++] = appId.c_str();
    newargs[i++] = "-b";
    newargs[i++] = baseFile;
	newargs[i++] = "/usr/bin/jailer";
	newargs[i++] = "-t";
	newargs[i++] = jailerType;
	newargs[i++] = "-i";
	newargs[i++] = appId.c_str();
	newargs[i++] = "-p";
	newargs[i++] = workingPath;
	newargs[i++] = path;
	for(e_argc = 0; args[e_argc]; e_argc++) 
		newargs[i++] = args[e_argc];
	newargs[i] = NULL;
	g_message("%s: Process %s launching in jail with args %s %s %s %s %s %s",
		__PRETTY_FUNCTION__,appId.c_str(),newargs[0],newargs[1],
		newargs[2],newargs[3],newargs[4],newargs[5]);
	
	
	// set the lib path value
    setenv("LD_LIBRARY_PATH", workingPath, 1); 

    // for PVR texture decompression
    if ( appType != ApplicationDescription::Type_Qt ) {
        setenv("LD_PRELOAD", "libpvrtc.so", 1);
    }
    else {
        setenv("QT_QPA_PLATFORM", "webos-offscreen", 1);
        setenv("QML_IMPORT_PATH", "/usr/plugins/imports", 1);
    }

    // fire off the process (jailer will preserve the lib path value in the new environment)
	pid_t pid = PrvLaunchProcess((char **)newargs);

	// clear the lib path value
    unsetenv("LD_LIBRARY_PATH"); 
	
	if (pid < 0) {
		g_critical("%s:%d failed to fork: %s", __PRETTY_FUNCTION__, __LINE__,
				   strerror(errno));
		return -1; // return -1 for launch error
	}
	else if (requiredMemory > 0)
	{
		// add a memory watch for this native app
		MemoryMonitor::instance()->monitorNativeProcessMemory(pid, requiredMemory);
	}

	m_nativeProcessMap[appId] = pid;
	g_message("%s: Process %s (%s) launched with pid: %d", __PRETTY_FUNCTION__,
			  appId.c_str(), path, pid);

	g_child_watch_add_full(G_PRIORITY_HIGH, pid, IpcServer::childProcessDiedCallback,
						   this, NULL);
	
	return pid;  
}

int IpcServer::launchWebAppProcess(const std::string& appId, const char* url, const char** argv)
{
    // Not implemented
	return 0;
}

void IpcServer::suspendProcess(int pid)
{
    ::kill(pid, SIGSTOP);
}

void IpcServer::resumeProcess(int pid)
{
    ::kill(pid, SIGCONT);
}

void IpcServer::killProcess(int pid, bool notifyUser)
{
	g_warning("%s: killing process: %d", __PRETTY_FUNCTION__, pid);
	
	if(notifyUser) {
		std::string nullString;
		std::string appId = appIdFromPid(pid);
		std::string appName("Application");
		std::string appTitle;
		
		if(!appId.empty()){
			ApplicationDescription*  desc = ApplicationManager::instance()->getAppById(appId);
			if(desc){
				appName = desc->menuName();
				if (desc->getDefaultLaunchPoint()){
					appTitle = desc->getDefaultLaunchPoint()->title();			
				}
			}
		}
		
		SystemService::instance()->postApplicationHasBeenTerminated(appTitle, appName, appId);
	}
	
	::kill(pid, SIGKILL);
	processRemoved(pid, false);
}
		
void IpcServer::processRemoved(int pid, bool doCleanup)
{
	if (doCleanup)
		::waitid(P_PID, pid, NULL, WEXITED | WNOHANG);	

	for (ProcessMap::iterator it = m_nativeProcessMap.begin();
		 it != m_nativeProcessMap.end(); ++it) {

		if (it->second == pid) {
			g_message("%s: pid: %d, appId: %s:", __PRETTY_FUNCTION__,
					  pid, it->first.c_str());
			m_nativeProcessMap.erase(it);
			return;
		}
	}

	for (ProcessMap::iterator it = m_webAppProcessMap.begin();
		 it != m_webAppProcessMap.end(); ++it) {

		if (it->second == pid) {
			g_message("%s: pid: %d, appId: %s:", __PRETTY_FUNCTION__,
					  pid, it->first.c_str());
			m_webAppProcessMap.erase(it);
			return;
		}
	}
}

void IpcServer::addProcessToNukeList(int pid)
{
	m_nukeSet.insert(pid);
	
	m_nukeProcessTimer.stop();
	m_nukeProcessTimer.start(kNukeProcessTimeoutMs, true);
}

bool IpcServer::nukeProcessTimer()
{
	for (ProcessSet::const_iterator it = m_nukeSet.begin();
		 it != m_nukeSet.end(); ++it) {
		killProcess(*it);
	}

	m_nukeSet.clear();
	
	return false;    
}

std::string IpcServer::appIdFromPid(int pid)
{
	const int len = 256;
	gchar buf[len];
	gchar  *exeName;
	GError *error = 0;
	
	snprintf(buf, len-1, "/proc/%d/exe", pid);
	exeName = g_file_read_link(buf, &error);

	if (error)
	{
		g_critical("IpcServer::appIdFromPid failed: %s", error->message);
		g_error_free(error);
	}

	
	if (NULL != exeName) {
		snprintf(buf, len-1, "%s", exeName);
		free(exeName);
	}
	else {
		return std::string();	
	}
		

	std::string appInstallRelative = Settings::LunaSettings()->appInstallRelative;	
	gchar* location = g_strrstr(buf, appInstallRelative.c_str());
	if (!location)
	{
		return std::string();
	}

	gchar* appIdStart = location + strlen(appInstallRelative.c_str());

	while (true) {
		if (*appIdStart == 0) {
			// reached end of string
			return std::string();
		}

		if (*appIdStart != '/')
			break;

		appIdStart++;
	}

	gchar* appIdEnd = appIdStart;
	while (true) {
		if (*appIdEnd == 0) {
			// reached end of string
			return std::string();
		}

		if (*appIdEnd == '/')
			break;

		appIdEnd++;
	}

	if (appIdEnd <= appIdStart)
	{
		return std::string();
	}
	
	return std::string(appIdStart, appIdEnd - appIdStart);
}

void IpcServer::ipcClientHostQuit(IpcClientHost* client)
{
	m_clientHostSet.erase(client);
	// FIXME: this prevents having multiple client hosts per process
	m_nukeSet.erase(client->pid());
	processRemoved(client->pid(), false);
}


IpcClientHost* IpcServer::clientHostForAppId(const std::string& appId) const
{
	for (ClientSet::const_iterator it = m_clientHostSet.begin();
		 it != m_clientHostSet.end(); ++it) {

		g_debug("Checking clientHost: '%s' against '%s'", (*it)->name().c_str(),
				appId.c_str());
		if ((*it)->name() == appId)
			return *it;
	}

	return 0;
}

void IpcServer::childProcessDiedCallback(GPid pid, gint status, gpointer data)
{
	IpcServer* server = (IpcServer*) data;
	server->childProcessDied(pid, status);
}

void IpcServer::childProcessDied(GPid pid, gint status)
{
	g_message("%s: pid: %d, status %d", __PRETTY_FUNCTION__, pid, status); 
	processRemoved(pid, false);
	g_spawn_close_pid(pid);
}

static int PrvForkFunction(void* data)
{
	// Move this process into the root cgroup
	int pid = getpid();

	const int maxSize = 64;
	char buf[maxSize];

	snprintf(buf, maxSize - 1, "%d", pid);
	buf[maxSize - 1] = 0;

	int fd = ::open("/dev/cgroup/tasks", O_WRONLY);
	if (fd >= 0) {
		ssize_t result = ::write(fd, buf, ::strlen(buf) + 1);
		Q_UNUSED(result);
		::close(fd);
	}
	else {
		perror("root cgroup tasks file not found");
	}

	// Reset cpu affinity before entering the jail because the jail 
	// prevents pdk apps from accessing libaffinity system files.
	resetCpuAffinity(0);

	// Now exec the process
	
	char ** argv = (char **) data;
	int ret = ::execv(argv[0], argv);
	if (ret < 0)
		perror("execv failed");

	return 0;	
}


static pid_t PrvLaunchProcess(char* const args[])
{
	pid_t pid = ::fork();
	if (pid < 0)
		return pid;

	if (pid == 0) {
		(void) PrvForkFunction((char**)args);
		exit(-1);
	}

	return pid;
}
