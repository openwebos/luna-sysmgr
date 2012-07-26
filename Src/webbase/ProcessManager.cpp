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




#include "Common.h"

#include <glib.h>

#include "ProcessManager.h"
#include "ProcessBase.h"
#include "ApplicationDescription.h"
#include "SystemUiController.h"
#include "Logging.h"
#include "WebAppManager.h"
#include "Settings.h"
#include "Utils.h"

#include <sstream>
#include <istream>

static ProcessManager*		s_processMgr = 0;
static const std::string	s_priv_args_prefix	= std::string("luna.priv.arg.");

class ProcessManagerEvent : public Event
{
public:
	ProcessManagerEvent() {}
	virtual ~ProcessManagerEvent() {}
	virtual void action(ProcessManager * pPMInstance) {}

};

class ProcessManagerLaunchEvent : public ProcessManagerEvent 
{
public:
	ProcessManagerLaunchEvent(const std::string& appId,const std::string& params,
							  const std::string& launchingAppId, const std::string& launchingProcId) :
		_appId(appId) , _params(params) , _launchingAppId(launchingAppId), _launchingProcId(launchingProcId) {}
	
	~ProcessManagerLaunchEvent() {}

	void action(ProcessManager * pPMInstance) {
		if (pPMInstance == NULL)
			return;
		
		std::string errMsg;
		pPMInstance->launch(_appId, _params, _launchingAppId, _launchingProcId, errMsg);
	}
	
private:
	
	std::string _appId;
	std::string _params;
	std::string _launchingAppId;
	std::string _launchingProcId;
};

ProcessManager::ProcessManager()
{
}

ProcessManager::~ProcessManager()
{
}

ProcessManager* ProcessManager::instance()
{
	if( !s_processMgr )
		s_processMgr = new ProcessManager();
	return s_processMgr;
}

std::string ProcessManager::processIdFactory( )
{
	// Need mutex lock here as this is also called from the WebKit thread

	static GStaticMutex s_mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock(&s_mutex);
	
	static int s_nextProcessId = 1000;
	std::ostringstream stream;
	stream << (s_nextProcessId++);

	g_static_mutex_unlock(&s_mutex);
	
	return stream.str();
}

std::string ProcessManager::launchBootTimeApp(const std::string& appDescString)
{
	std::string processId = processIdFactory();

	ApplicationDescription* desc = ApplicationDescription::fromJsonString(appDescString.c_str());
	
	WebAppManager::instance()->onLaunchUrl(desc->entryPoint().c_str(),
									  	   Window::Type_None,
									  	   appDescString.c_str(),
										   processId.c_str(),
										   "{\"launchedAtBoot\":true}", "", "");
	
	delete desc;
	return processId;
}


void ProcessManager::launch_threadsafe(const std::string& appId,const std::string& params,
									   const std::string& launchingAppId, const std::string& launchingProcId)
{
	sptr<Event> e = new ProcessManagerLaunchEvent(appId,params,launchingAppId,launchingProcId);
	postEvent(e);
}

/**
 * Launch an application (webApps only, not native).
 *
 * @param appId The application ID to launch.
 * @param params The call parameters.
 * @param the ID of the application performing the launch (can be NULL).
 * @param launchingProcId (can be NULL).
 * @param errMsg The error message (will be empty if this call was successful).
 *
 * @todo: this should now be moved private and be protected...leaving it for now as to not break stuff and make things
 * slightly faster for intra-sysmgr mainloop launches
 *
 * @return The process ID of the newly launched application. Empty upon error. If empty lool at errMsg.
 */
std::string ProcessManager::launch(const std::string& appDescString, const std::string& params, 
		const std::string& launchingAppId, const std::string& launchingProcId, std::string& errMsg)
{
	std::string processId = "";
	ApplicationDescription* desc = ApplicationDescription::fromJsonString(appDescString.c_str());
	errMsg.erase();

	// This now will only launch Web Apps. Native Apps are now launched by the WebAppMgrProxy.

	// Find out what type of window we need to create
	Window::Type winType = Window::Type_Card;
	std::string winTypeStr;
	if (extractFromJson(params, "windowType", winTypeStr)) {

		if (winTypeStr == "dashboard")
			winType = Window::Type_Dashboard;
		else if (winTypeStr == "popupalert")
			winType = Window::Type_PopupAlert;
		else if (winTypeStr == "emergency")
			winType = Window::Type_Emergency;
		else if (winTypeStr == "dockModeWindow") // $$$ FIX THIS, it is just a patch to test Dock mode apps for now
			winType = Window::Type_DockModeWindow;
		else {
			winType = Window::Type_Card;
		}
	}
	
	// Get a list of all apps
	typedef std::list<const ProcessBase*> ProcessList;	
	ProcessList runningApps = WebAppManager::instance()->runningApps();
	const ProcessBase* app = 0;
	
	for (ProcessList::const_iterator it = runningApps.begin(); it != runningApps.end(); ++it) {
		if ((*it)->appId() == QString::fromStdString(desc->id())) {
			app = (*it);
			processId = app->processId().toStdString();
			break;
		}
	}
	
	if (!app) {

		// App not running? Launch it

		std::string url = desc->entryPoint();

		if (desc->isHeadLess())
			winType = Window::Type_None;

		processId = processIdFactory();

		WebAppManager::instance()->onLaunchUrl(url.c_str(),
											   winType,
											   appDescString.c_str(),
											   processId.c_str(),
											   params.c_str(), launchingAppId.c_str(), launchingProcId.c_str());
		g_debug("Launched Id %s", processId.c_str() );
	}
	else {
		WebAppManager::instance()->onRelaunchApp(processId.c_str(), params.c_str(), launchingAppId.c_str(), launchingProcId.c_str());
	}
	
	delete desc;
	
	return processId;
}

/**
 * Launch an application (webApps only, not native).
 *
 * @param appId The application ID to launch.
 * @param params The call parameters.
 * @param the ID of the application performing the launch (can be NULL).
 * @param launchingProcId (can be NULL).
 * @param errMsg The error message (will be empty if this call was successful).
 *
 * @todo: this should now be moved private and be protected...leaving it for now as to not break stuff and make things
 * slightly faster for intra-sysmgr mainloop launches
 *
 * @return The process ID of the newly launched application. Empty upon error. If empty lool at errMsg.
 */
std::string ProcessManager::launchModal(const std::string& appDescString, const std::string& params,
		const std::string& launchingAppId, const std::string& launchingProcId, std::string& errMsg, int& errorCode, bool isHeadless, bool isParentPdk)
{
	std::string processId = "";
	errMsg.erase();
	Window::Type winType = Window::Type_ModalChildWindowCard;

	if(false == isParentPdk && 0 >= launchingProcId.size()) {
		errorCode = SystemUiController::MissingProcessId;
		errMsg = "Invalid processId of the calling process";
		return processId;
	}

	ApplicationDescription* desc = ApplicationDescription::fromJsonString(appDescString.c_str());
	if(!desc) {
		errorCode = SystemUiController::InternalError;
		errMsg = "Error getting ApplicationDescription for the app to be launched";
		return processId;
	}

	errorCode = SystemUiController::NoErr;

	// always create a new card if the system wants a modal card to be created, no relaunching.
	std::string url = desc->entryPoint();
	processId = processIdFactory();

	// if we want to create a headless app, the type is Window::Type_None else it Window::Type_ModalChildWindowCard
	if(true == isHeadless)
		winType = Window::Type_None;

	// launch the app
	WebAppManager::instance()->onLaunchUrlChild(url.c_str(),
												winType,
												appDescString.c_str(),
												processId.c_str(),
												params.c_str(), launchingAppId.c_str(), launchingProcId.c_str(), errorCode, isHeadless);

	g_debug("Launched Id %s", processId.c_str() );

	delete desc;
	return processId;
}

/*
	Return the process list. This will be 
*/
std::vector<ProcessInfo>	ProcessManager::list( bool includeSystemApps )
{
	std::vector<ProcessInfo> list;
	
	std::list<const ProcessBase*> running = WebAppManager::instance()->runningApps();
	for( std::list<const ProcessBase*>::iterator it=running.begin(); it != running.end(); it++ )
	{
		if( (*it)->appId().size() || (!(*it)->appId().size() && includeSystemApps ) )
			list.push_back( ProcessInfo( (*it)->processId(), (*it)->appId() ) );
	}
	
	return list;
}

bool ProcessManager::getProcIdsOfApp(const std::string& appId,std::vector<std::string>& procIdList) const {
	
	std::list<const ProcessBase*> running = WebAppManager::instance()->runningApps();
	std::vector<std::string>::size_type s = procIdList.size();
	for( std::list<const ProcessBase*>::iterator it=running.begin(); it != running.end(); it++ )
	{
		if(QString::fromStdString(appId) == (*it)->appId()) {
			procIdList.push_back((*it)->processId().toStdString());
		}
	}
	
	if (s < procIdList.size())
		return true;
	
	return false;
}

void ProcessManager::handleEvent(sptr<Event> event) {

	Event * pRawEvnt = event.get();
	
	ProcessManagerEvent * pPMEvent = static_cast<ProcessManagerEvent *>(pRawEvnt);
	
	if (pPMEvent == NULL)
		return;				//cast error? wrong event type passed in????
	
	pPMEvent->action(this);
	
}

/*
 * this one is static and makes a parameter string suitable for passing to a process/app that is going to be launched
 * TODO: need more versions of this function, in case more complicated parameter passing needs to be implemented.
 * I had the idea of doing this with a vararg version of this function but will hold off on that for now given the potential
 * platform issues with varargs
 */
std::string ProcessManager::createParameterJSONString(const std::string& target) {
	
	return (std::string("{ \"target\":\"")+target+std::string("\"}"));
}

std::string ProcessManager::createParameterJSONString(const char * pcstrTarget) {
	
	if (pcstrTarget == NULL)
		return (std::string("{ \"target\":\"\"}"));
	
	return (std::string("{ \"target\":\"")+std::string(pcstrTarget)+std::string("\"}"));
}

