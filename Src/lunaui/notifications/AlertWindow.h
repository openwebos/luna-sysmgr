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




#ifndef ALERTWINDOW_H
#define ALERTWINDOW_H

#include "Common.h"

#include "sptr.h"
#include "Window.h"
#include "HostWindow.h"
#include <Event.h>

class QGraphicsSceneMouseEvent;

class SoundPlayer;
class PIpcMessage;

class AlertWindow : public HostWindow
{
public:

	AlertWindow(Type type, int width, int height, bool hasAlpha);
	AlertWindow(Type type, HostWindowData* window, IpcClientHost* clientHost=0);
	virtual ~AlertWindow();

	virtual void onMessageReceived(const PIpcMessage& msg);
	void onSetContentRect(int left, int right, int top, int bottom);
	
	void setSoundParams(const std::string& fileName, const std::string& soundClass);
	void setContentRect(const QRect& r);
	QRect contentRect() const; // 0,0 is top left corner

	void activate();
	void deactivate();

	bool isIncomingCallAlert() const;

	void setTransient(bool transient) { m_isTransientAlert = transient; }
	bool isTransientAlert() { return m_isTransientAlert; }

protected:

    virtual void onUpdateWindowRegion(int x, int y, int w, int h);
    virtual void onUpdateFullWindow();
	
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	
private:

	void extractSoundParams();

    /*!
     * Convenience function to send touch events to the WebAppManager process
     */
    void sendInputEvent(QGraphicsSceneMouseEvent* event, Event::Type type, int clickCount = 0);

	sptr<SoundPlayer> m_player;
	std::string m_filePath;
	std::string m_soundClass;
	std::string m_setFilePath;
	std::string m_setSoundClass;
	QRect m_contentRect;
	bool m_playedSound;
	bool m_isTransientAlert;
};	

#endif /* ALERTWINDOW_H */
