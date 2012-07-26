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




#ifndef IPCSERVER_H
#define IPCSERVER_H

#include "Common.h"

#include <map>
#include <set>
#include <string>

#include <PIpcServer.h>

#include "Timer.h"
#include "ApplicationDescription.h"

class IpcClientHost;

class IpcServer : public PIpcServer
{
public:

	static IpcServer* instance();

	int launchNativeProcess(const std::string& appId, const char* path, char* const argv[], ApplicationDescription::Type appType, int requiredMemory = 0);
	int launchWebAppProcess(const std::string& appId, const char* url, const char** argv);
	void suspendProcess(int pid);
	void resumeProcess(int pid);
	void killProcess(int pid, bool notifyUser=false);
	void processRemoved(int pid, bool doCleanup=true);
	void addProcessToNukeList(int pid);
	void ipcClientHostQuit(IpcClientHost* client);

private:

	IpcServer();
	~IpcServer();

	virtual void clientConnected(int pid, const std::string& name, PIpcChannel* channel);

	bool nukeProcessTimer();
	std::string appIdFromPid(int pid);
	IpcClientHost* clientHostForAppId(const std::string& appId) const;

	static void childProcessDiedCallback(GPid pid, gint status, gpointer data);
	void childProcessDied(GPid, gint status);
	
private:

	typedef std::map<std::string, int> ProcessMap;
	typedef std::set<int> ProcessSet;
	typedef std::set<IpcClientHost*> ClientSet;

	ProcessMap m_nativeProcessMap;
	ProcessMap m_webAppProcessMap;
	ProcessSet m_nukeSet;
	ClientSet m_clientHostSet;

	Timer<IpcServer> m_nukeProcessTimer;
};

#endif /* IPCSERVER_H */
