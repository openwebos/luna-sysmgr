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




#ifndef BANNERWINDOW_H
#define BANNERWINDOW_H

#include "Common.h"

#include <string>

#include <QGraphicsObject>

#include "BannerMessageHandler.h"
#include "Window.h"

class DashboardWindowManager;
class ActiveCallBanner;

class BannerWindow : public QGraphicsObject,
					 public BannerMessageView
{
public:
	
	BannerWindow(DashboardWindowManager* wm, int width, int height);
	virtual ~BannerWindow();

	virtual QRectF boundingRect() const;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void contentUpdated();
	inline void resetIgnoreGestureUpEvent() {
		m_ignoreGestureUpEvent = !m_ignoreGestureUpEvent;
	}

	void resize(int width, int height);

private:

	bool sceneEvent(QEvent* event);
	bool handleTap(const QPointF &tapHotspot);

	virtual int  bmViewGetWidth() const;
	virtual void bmViewUpdated();
	virtual void bmViewMessageCountUpdated(int count);

	virtual void bmViewAddActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time);
	virtual void bmViewRemoveActiveCallBanner();
	virtual void bmViewUpdateActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time);
	virtual void bmViewShowingNonSuppressibleMessage(bool val);

	void sendClickToActiveBannerWindow(Window* win, int x, int y);

	DashboardWindowManager* m_wm;
	ActiveCallBanner* m_activeCallBanner;
	BannerMessageHandler* m_bmHandler;
	int m_width;
	int m_height;
	int m_msgCount;
	bool m_ignoreGestureUpEvent;
};

#endif /* BANNERWINDOW_H */
