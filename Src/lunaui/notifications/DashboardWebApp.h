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




#ifndef DASHBOARDWEBAPP_H
#define DASHBOARDWEBAPP_H

#include "Common.h"

#include "WindowedWebApp.h"

class DashboardWebApp : public WindowedWebApp
{
public:

	DashboardWebApp(PIpcChannel *channel);
	~DashboardWebApp();

	virtual bool isDashboardApp() const { return true; }

	virtual void attach(SysMgrWebBridge* page);
	
private:

	DashboardWebApp& operator=(const DashboardWebApp&);
	DashboardWebApp(const DashboardWebApp&);
};

#endif /* DASHBOARDWEBAPP_H */
