/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef __DockWebApp_h__
#define __DockWebApp_h__

#include "Common.h"

#include "WindowedWebApp.h"
#include "CardWebApp.h"

class PIpcChannel;
class QPainter;

class DockWebApp : public CardWebApp
{
public:

	DockWebApp(Window::Type winType, PIpcChannel *channel);
	~DockWebApp( );

	virtual bool isCardApp() const { return true; } // $$$
	virtual void setOrientation(Event::Orientation orient) { }
	virtual void enableFullScreenMode(bool enable) { }

protected:
	virtual void setVisibleDimensions(int width, int height);
	void resizeWindowForOrientation(Event::Orientation orient);
	
private:
	
	DockWebApp& operator=( const DockWebApp& );
	DockWebApp( const DockWebApp& );
};

#endif


