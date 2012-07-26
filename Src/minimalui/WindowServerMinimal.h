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




#ifndef WINDOWSERVERMINIMAL_H
#define WINDOWSERVERMINIMAL_H

#include "Common.h"

#include "WindowServer.h"

class WindowManagerBase;

class WindowServerMinimal : public WindowServer
{
public:

	WindowServerMinimal();
	virtual ~WindowServerMinimal();

	virtual void prepareAddWindow(Window* win);
	virtual void addWindow(Window* win);
	virtual void removeWindow(Window* win);
	virtual void focusWindow(Window* win);

	virtual WindowManagerBase* windowManagerForWindow(Window* wm) const;

	virtual bool okToResizeUi(bool ignorePendingRequests = false);
	virtual void resizeWindowManagers(int width, int height);

	virtual QRectF mapRectToRoot(const QGraphicsItem* item, const QRectF& rect) const;
	virtual QPixmap* takeScreenShot();
protected:

	virtual bool sysmgrEventFilters(QEvent* event);

private:

	WindowManagerBase* m_minimalWM;
};	

#endif /* WINDOWSERVERMINIMAL_H */
