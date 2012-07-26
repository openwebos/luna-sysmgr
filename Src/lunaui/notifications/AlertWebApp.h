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




#ifndef ALERTWEBAPP_H
#define ALERTWEBAPP_H

#include "Common.h"

#include "WindowedWebApp.h"

class AlertWebApp : public WindowedWebApp
{
public:

	AlertWebApp(const QString& appId, int width, int height,
				Window::Type type, PIpcChannel *channel);
	virtual ~AlertWebApp();

	virtual bool isAlertApp() const { return true; }

	virtual void attach(SysMgrWebBridge* page);
	
	virtual void setSoundParams(const QString& fileName, const QString& soundClass);

	virtual void setOrientation(Event::Orientation orient);
	
protected:
	virtual void stageReady();	

private:

	virtual void loadFinished();
    virtual void paint();
	virtual void focus();

	void updateContentRect();
    int constraintHeight(int h);

	void callMojoScreenOrientationChange();
	
	// private copy constructor
	AlertWebApp& operator=(const AlertWebApp&);
	AlertWebApp(const AlertWebApp&);

	void startPowerdActivity();
	void stopPowerdActivity();
	bool m_isPowerdActivityRunning;
};

#endif /* ALERTWEBAPP_H */
