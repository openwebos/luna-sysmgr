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

#include <QGesture>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include "ActiveCallBanner.h"
#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "BannerWindow.h"
#include "DashboardWindow.h"
#include "DashboardWindowContainer.h"
#include "DashboardWindowManager.h"
#include "HostBase.h"
#include "Logging.h"
#include "Preferences.h"
#include "Settings.h"
#include "Time.h"
#include "Utils.h"
#include "WebAppMgrProxy.h"
#include "WindowServer.h"
#include "SystemUiController.h"
#include "SystemService.h"


BannerWindow::BannerWindow(DashboardWindowManager* wm, int width, int height)
	: BannerMessageView(BannerMessageView::VerticalScroll)
 	, m_wm(wm)
	, m_activeCallBanner(0)
	, m_bmHandler(BannerMessageHandler::instance())
	, m_width(width)
	, m_height(height)
	, m_msgCount(0)
	, m_ignoreGestureUpEvent(false)
{
	setObjectName("BannerWindow");
	if(!m_wm->isOverlay()) {

		grabGesture(Qt::TapGesture);

		m_bmHandler->registerView(this);
	}
}

BannerWindow::~BannerWindow()
{
	ungrabGesture(Qt::TapGesture);
}

QRectF BannerWindow::boundingRect() const
{
	return QRectF(-m_width/2, -m_height/2, m_width, m_height);
}

void BannerWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(m_wm->isOverlay())
		return; 

	painter->fillRect(boundingRect(), Qt::black);

	// Don't draw banner message if active banner is up
	if ((m_activeCallBanner == 0) && m_msgCount) {
		int tx = boundingRect().x();
		int ty = boundingRect().y();
		painter->translate(tx, ty);
		m_bmHandler->drawView(painter, this);
		painter->translate(-tx, -ty);
	}
	else if (!m_wm->dashboardWindowContainer()->empty()) {
		int x = 0;
		int y = 0;

		if(m_wm->isOverlay())
			x = m_width/3;
		else
			x = m_width/2;

		QList<DashboardWindow*> windows = m_wm->dashboardWindowContainer()->windows();
		int iconMaxSize = Settings::LunaSettings()->positiveSpaceBottomPadding;
		
		for (int i = windows.count() - 1; i >= 0; i--) {

			DashboardWindow* w = windows[i];
			QPixmap icon = w->icon();
			if (icon.isNull()) {
				continue;
			}

			int iconHeight = (icon.height() > iconMaxSize) ? iconMaxSize : icon.height();
			int iconWidth = (iconHeight == icon.height()) ? icon.width() : ((icon.width() * iconHeight) / icon.height());

			painter->drawPixmap(x - iconWidth, y - iconHeight/2, iconWidth, iconHeight, icon);
			x -= iconWidth;
		}
	}
}

bool BannerWindow::handleTap(const QPointF &tapHotspot)
{
	if (m_activeCallBanner && m_activeCallBanner->boundingRect().contains(m_activeCallBanner->mapFromScene(tapHotspot))) {
		m_activeCallBanner->handleTap();
		return true;
	}
	
	// Showing banner messages?
	if (m_msgCount) {
		BannerMessageHandler::instance()->activateCurrentMessage();
		return true;
	}

	return false;
}

void BannerWindow::resize(int width, int height)
{
	// TODO: Even though this paints the dashboard windows, I think we'll let its
	// geometry change handle updating those
	m_width = width;
	m_height = height;
	prepareGeometryChange();
}

void BannerWindow::contentUpdated()
{
#ifndef FIX_FOR_QT	
	SystemUiController::instance()->notifyBannerAboutToUpdate(boundingRect());
#endif
	update();
}

void BannerWindow::bmViewAddActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time)
{
	if (m_activeCallBanner)
		return;

	m_activeCallBanner = new ActiveCallBanner(boundingRect().width(), boundingRect().height(),
											  icon, msg, time);

	m_wm->setBannerHasContent(true);

	m_activeCallBanner->setParentItem(this);
	setPosTopLeft(m_activeCallBanner, 0, 0);
}

void BannerWindow::bmViewRemoveActiveCallBanner()
{
	if (!m_activeCallBanner)
		return;

	delete m_activeCallBanner;
	m_activeCallBanner = 0;

	if (!m_msgCount) {
		m_wm->setBannerHasContent(false);
	}

	update();
}

void BannerWindow::bmViewUpdateActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time)
{
	if (!m_activeCallBanner)
		return;

	m_activeCallBanner->updateProperties(time, icon, msg);
}

int BannerWindow::bmViewGetWidth() const
{
	return m_width;
}

void BannerWindow::bmViewUpdated()
{
	update();    
}

void BannerWindow::bmViewMessageCountUpdated(int count)
{
	m_msgCount = count;
	if (m_msgCount)
		m_wm->setBannerHasContent(true);
	else if (m_activeCallBanner == 0)
		m_wm->setBannerHasContent(false);
}

bool BannerWindow::sceneEvent(QEvent* event)
{
	if(m_wm->isOverlay())
		return QGraphicsObject::sceneEvent(event);    

	switch (event->type()) {
	case QEvent::GestureOverride: {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		break;
	}
	case QEvent::Gesture: {
		QGesture* t = static_cast<QGestureEvent*>(event)->gesture(Qt::TapGesture);
		if (t) {
			QTapGesture* tap = static_cast<QTapGesture*>(t);
			if (tap->state() == Qt::GestureFinished) {
				if (!m_wm->isInDockModeAnimation() && !handleTap(tap->hotSpot())) {
					if(!m_wm->isOverlay()) {
						m_wm->openDashboard();
					}
					else {
						if(false == m_ignoreGestureUpEvent) {
							m_wm->openDashboard();
						}
						else {
							resetIgnoreGestureUpEvent();
						}
					}
				}
			}
		}
		break;
	}
	default:
		break;
	}

	return QGraphicsObject::sceneEvent(event);    
}

void BannerWindow::bmViewShowingNonSuppressibleMessage(bool val)
{
	// NO-OP    
}
