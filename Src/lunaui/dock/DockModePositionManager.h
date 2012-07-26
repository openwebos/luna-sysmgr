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




#ifndef __DockModePositionManager_h__
#define __DockModePositionManager_h__

#include "Common.h"

#include <string>
#include <vector>

class DockModePositionManager
{
	public:
		static DockModePositionManager* instance();
		
		void	addEnabledApp(const std::string& appId);
		void	removeEnabledApp(const std::string& appId);

		void 	addAppForPuck (const std::string& puckId, const std::string& appId);
		bool	load();
		void	clear();

		bool	getLaunchPointIds(std::vector<std::string>& outIds);
			
	private:
		DockModePositionManager();
		~DockModePositionManager();

		bool	save();
		
		std::vector<std::string>	m_appIds;

		std::map<std::string, std::string> m_appForPuck;
		bool	m_loadFailed;
};



#endif // DockModePositionManager

