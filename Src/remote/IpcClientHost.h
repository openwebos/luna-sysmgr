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




#ifndef IPCCLIENTHOST_H
#define IPCCLIENTHOST_H

#include "Common.h"

#include <string>
#include <map>
#include <set>
#include <glib.h>

#include <PIpcChannelListener.h>
#include "Window.h"

#include <QObject>

class PIpcChannel;
class PIpcBuffer;
class SysMgrKeyEvent;

class IpcClientHost : public QObject, public PIpcChannelListener
{
	Q_OBJECT
public:

	IpcClientHost();
	IpcClientHost(int pid, const std::string& name, PIpcChannel* channel);
	virtual ~IpcClientHost();

	void windowDeleted(Window* w);
	
	virtual void relaunch();
	virtual void closeWindow(Window* w);

    virtual void replaceWindowKey(Window* win, int oldKey, int newKey);
    
    int pid() const { return m_pid; }
	std::string name() const { return m_name; }	

protected:

    virtual void onMessageReceived(const PIpcMessage& msg);
    virtual void onDisconnected();

    virtual void onReturnedInputEvent(const SysMgrKeyEvent& event);
    virtual void onPrepareAddWindow(int key, int type, int width, int height);
    virtual void onAddWindow(int key);
    virtual void onRemoveWindow(int key);
    virtual void onSetWindowProperties(int key, const std::string& winProps);
    virtual void onFocusWindow(int key);
    virtual void onUnfocusWindow(int key);
    


	virtual Window* findWindow(int key) const;

protected Q_SLOTS:

	void slotMaximizedCardWindowChanged(Window* w);

	
protected:

	int m_pid;
	int m_processPriority;
	std::string m_name;

	typedef std::map<int, Window*> WindowMap;
	typedef std::set<Window*> WindowSet;
	WindowMap m_winMap;
	WindowSet m_winSet;
	WindowSet m_closedWinSet;

	bool m_clearing;
	GSource* m_idleDestroySrc;

private:

	IpcClientHost(const IpcClientHost&);
	IpcClientHost& operator=(const IpcClientHost&);

	static gboolean idleDestroyCallback(gpointer arg);
};


#endif /* IPCCLIENTHOST_H */
