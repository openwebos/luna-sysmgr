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




#ifndef __ProcessManager_h__
#define __ProcessManager_h__

#include "Common.h"

#include <vector>
#include <QString>
#include "SyncTask.h"

class ApplicationDescription;

class ProcessInfo
{
	public:
		ProcessInfo( const QString& inProcId, const QString& inAppId )
		: processId(inProcId)
		, appId(inAppId) { }
		~ProcessInfo( ) { }
		
		QString				processId;
		QString				appId;
};


class ProcessManager : public SyncTask
{
	public:
		static ProcessManager*		instance();
		virtual ~ProcessManager();
		
		std::string					launchBootTimeApp(const std::string& appDescString);
		void						launch_threadsafe(const std::string& appId,
													  const std::string& params,
													  const std::string& launchingAppId,
													  const std::string& launchingProcId);
		
		std::string					launch( const std::string& appDescString,
											const std::string& params,
											const std::string& launchingAppId,
										    const std::string& launchingProcId,
											std::string& errMsg);
		std::string					launchModal(const std::string& appDescString,
												const std::string& params,
												const std::string& launchingAppId,
												const std::string& launchingProcId,
												std::string& errMsg,
												int& errCode,
												bool isHeadless,
												bool isParentPdk);
		std::vector<ProcessInfo>	list( bool includeSystemApps = false );
		std::string					processIdFactory();
		bool						getProcIdsOfApp(const std::string& appId,std::vector<std::string>& procIdList) const;		//pass in vector to fill in with proc ids. return false if none found 

		static std::string			createParameterJSONString(const std::string& target);
		static std::string			createParameterJSONString(const char * pcstrTarget);
	private:
		
		virtual void handleEvent(sptr<Event> event);
		ProcessManager();
};


#endif // __ProcessManager_h__

