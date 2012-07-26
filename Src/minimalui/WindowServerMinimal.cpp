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

#include "WindowServerMinimal.h"

#include "CoreNaviManager.h"
#include "MetaKeyManager.h"
#include "SystemUiController.h"
#include "WindowManagerMinimal.h"
#include "InputWindowManager.h"
#include "WebAppMgrProxy.h"

WindowServerMinimal::WindowServerMinimal()
{
	// Trigger the initial layout for the WMs
	m_minimalWM = new WindowManagerMinimal(m_screenWidth, m_screenHeight);
	if (Settings::LunaSettings()->virtualKeyboardEnabled)
		m_inputWindowMgr = new InputWindowManager(m_screenWidth, m_screenHeight);

	m_minimalWM->init();
	if (m_inputWindowMgr)
		m_inputWindowMgr->init();

	m_minimalWM->setParentItem (&m_uiRootItem);
	if (m_inputWindowMgr) {
		m_inputWindowMgr->setParentItem(&m_uiRootItem);
		m_inputWindowMgr->setPos (m_uiRootItem.x(), m_uiRootItem.y());
	}

	SystemUiController::instance()->setUiRootItemPtr(&m_uiRootItem);

	m_uiRootItem.setPos(SystemUiController::instance()->currentUiWidth()/2, SystemUiController::instance()->currentUiHeight()/2);

	SystemUiController::instance()->init();

	scene()->addItem(&m_uiRootItem);
}

WindowServerMinimal::~WindowServerMinimal()
{
    
}

WindowManagerBase* WindowServerMinimal::windowManagerForWindow(Window* wm) const
{
	return m_minimalWM;
}

bool WindowServerMinimal::sysmgrEventFilters(QEvent* event)
{
	if (handleEvent(event))
		return true;

	if (m_displayMgr->handleEvent(event)) {
		return true;
	}

	if (m_inputMgr->handleEvent(event)) {
		return true;
	}

	if (m_coreNaviMgr->handleEvent(event)) {
		return true;
	}

	if (m_metaKeyMgr->handleEvent(event)) {
		return true;
	}
	
	if (SystemUiController::instance()->handleEvent(event)) {
		return true;
	}

    return false;
}

void WindowServerMinimal::prepareAddWindow(Window* win)
{
    m_minimalWM->prepareAddWindow(win);
}

void WindowServerMinimal::addWindow(Window* win)
{
    m_minimalWM->addWindow(win);    
}

void WindowServerMinimal::removeWindow(Window* win)
{
    m_minimalWM->removeWindow(win);
}

void WindowServerMinimal::focusWindow(Window* win)
{
    m_minimalWM->focusWindow(win);
}

bool WindowServerMinimal::okToResizeUi(bool ignorePendingRequests)
{
	if(m_inRotationAnimation != Rotation_NoAnimation)
		return false;
	if(!ignorePendingRequests && !m_pendingFlipRequests.empty())
		return false;
	if (m_minimalWM && !m_minimalWM->okToResize())
		return false;
	if (m_inputWindowMgr && !m_inputWindowMgr->okToResize())
		return false;

	return true;
}
void WindowServerMinimal::resizeWindowManagers (int width, int height)
{
	WebAppMgrProxy::instance()->uiDimensionsChanged(width, height);

	m_uiRootItem.setBoundingRect (QRectF (-SystemUiController::instance()->currentUiWidth()/2, -SystemUiController::instance()->currentUiHeight()/2,
				SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight()));

	// clean up any windows still pending a resize
	m_pendingFlipRequests.clear();

	if(m_minimalWM)
		m_minimalWM->resize (width, height);

	if(m_inputWindowMgr)
		m_inputWindowMgr->resize (width, height);

}

QRectF WindowServerMinimal::mapRectToRoot(const QGraphicsItem* item, const QRectF& rect) const
{
    return (item ? item->mapRectToItem(&m_uiRootItem, rect) : rect);
}

QPixmap* WindowServerMinimal::takeScreenShot()
{
	QPixmap* pix = new QPixmap(m_screenWidth, m_screenHeight);
	QPainter painter(pix);
	render(&painter);
	painter.end();
	return pix;
}
