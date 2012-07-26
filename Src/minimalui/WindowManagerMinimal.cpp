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

#include "WindowManagerMinimal.h"

#include "AlertWindow.h"
#include "CardWindow.h"
#include "HostBase.h"
#include "Logging.h"
#include "SystemUiController.h"
#include "WebAppMgrProxy.h"
#include "WindowServer.h"
#include "Utils.h"

static const int kTopLeftWindowIndex     = 0;
static const int kTopRightWindowIndex    = 1;
static const int kBottomLeftWindowIndex  = 2;
static const int kBottomRightWindowIndex = 3;

WindowManagerMinimal::WindowManagerMinimal(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_activeWin(0)
	, m_statusBar(0)
	, m_cardContainer(0)
	, m_alertContainer(0)
{
	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChangeFinished(const QRect&)), 
			this, SLOT(slotPositiveSpaceChanged(const QRect&)));

	connect(SystemUiController::instance(), SIGNAL(signalUiRotationAboutToStart()),
			this, SLOT(slotUiRotationAboutToStart()));
	connect(SystemUiController::instance(), SIGNAL(signalUiRotationCompleted()),
			this, SLOT(slotUiRotationCompleted()));

}

WindowManagerMinimal::~WindowManagerMinimal()
{
}

void WindowManagerMinimal::init()
{
	m_statusBar = new StatusBar(StatusBar::TypeFirstUse, boundingRect().width(), Settings::LunaSettings()->positiveSpaceTopPadding);
	if (m_statusBar) {
		SystemUiController::instance()->setStatusBar (m_statusBar);
		m_statusBar->init();
		m_statusBar->setParentItem(this);
		m_statusBar->setPos(0, boundingRect().y() + m_statusBar->boundingRect().height() / 2);
		m_statusBar->setZValue(100);
	}

	m_cardContainer = new GraphicsItemContainer(boundingRect().width(), boundingRect().height() - m_statusBar->boundingRect().height());
	m_alertContainer = new GraphicsItemContainer(boundingRect().width(), boundingRect().height() - m_statusBar->boundingRect().height());

	m_cardContainer->setParentItem(this);
	m_alertContainer->setParentItem(this);

	m_alertContainer->setPos(boundingRect().center().x(), boundingRect().center().y() + m_statusBar->boundingRect().height()/2);
	m_cardContainer->setPos(boundingRect().center());
}

void WindowManagerMinimal::prepareAddWindow(Window* win)
{
	// NO-OP
}

void WindowManagerMinimal::addWindow(Window* win)
{
	switch (win->type()) {
	case Window::Type_StatusBar:
		// NO-OP
		break;
	case Window::Type_Card:
	case Window::Type_Emergency:
	default:
		addCardWindow(win);
		break;
	}
}

void WindowManagerMinimal::removeWindow(Window* win)
{
	switch (win->type()) {
	case Window::Type_StatusBar:
		// NO-OP
		break;
	case Window::Type_Card:
	case Window::Type_Emergency:
	default:
		removeCardWindow(win);
		break;
	}    
}

void WindowManagerMinimal::focusWindow(Window* win)
{
	switch (win->type()) {
	case Window::Type_StatusBar:
		// NO-OP
		break;
	case Window::Type_Card:
	case Window::Type_Emergency:
	default:
		focusCardWindow(win);
		break;
	}    
}

void WindowManagerMinimal::slotPositiveSpaceChanged(const QRect& r)
{
	m_positiveSpace = r;

	//m_cardContainer->setPos(r.center());
	m_cardContainer->resize(r.width(),r.height());

	if (m_activeWin) {
		CardWindow* cardWin = static_cast<CardWindow*>(m_activeWin);
		resizeCardWindow(cardWin, m_positiveSpace.width(), m_positiveSpace.height());
		cardWin->positiveSpaceChanged(m_positiveSpace);
	}

}

void WindowManagerMinimal::slotUiRotationAboutToStart()
{
	if (m_activeWin) {
		SystemUiController::instance()->setDirectRenderingForWindow (SystemUiController::CARD_WINDOW_MANAGER,
				static_cast<CardWindow*>(m_activeWin), false, false);
	}
}

void WindowManagerMinimal::slotUiRotationCompleted()
{
	if (m_activeWin) {
		SystemUiController::instance()->setDirectRenderingForWindow (SystemUiController::CARD_WINDOW_MANAGER,
				static_cast<CardWindow*>(m_activeWin), true, false);
	}
}

void WindowManagerMinimal::addCardWindow(Window* win)
{
	if (m_cardArray.contains(win))
		return;

	win->setParentItem(m_cardContainer);
	m_cardArray.append(win);
		
	win->setPos(m_cardContainer->boundingRect().center().x(), m_cardContainer->boundingRect().center().y() + m_statusBar->boundingRect().height()/2);

	if (!m_activeWin) {

		m_activeWin = win;

		static_cast<CardWindow*>(m_activeWin)->focusEvent(true);
		static_cast<CardWindow*>(m_activeWin)->setMaximized(true);
	
		SystemUiController::instance()->setCardWindowMaximized(true);
		SystemUiController::instance()->setActiveCardWindow(m_activeWin);
		SystemUiController::instance()->setMaximizedCardWindow(m_activeWin);
		m_statusBar->setMaximizedAppTitle (true, m_activeWin->appDescription()->menuName().c_str());

		SystemUiController::instance()->setDirectRenderingForWindow (SystemUiController::CARD_WINDOW_MANAGER,
				static_cast<CardWindow*>(m_activeWin), true, false);
		resizeCardWindow(static_cast<CardWindow*>(m_activeWin),
						 m_positiveSpace.width(), m_positiveSpace.height());
	}
	else {
		raiseChild(m_activeWin);
	}
}

void WindowManagerMinimal::addAlertWindow(Window* win)
{
	if (m_alertArray.contains(win))
		return;

	win->setParentItem(m_alertContainer);
	m_alertArray.append(win);

	if (m_alertArray.size() != 1) {
		// Another alert is currently active. Nothing to do
		return;
	}
	
	int h = win->boundingRect().height();
	const HostInfo& hostInfo = HostBase::instance()->getInfo();
	win->setPos (0, hostInfo.displayHeight - h);

	h = hostInfo.displayHeight - Settings::LunaSettings()->positiveSpaceTopPadding - h;

	QRect r = QRect(m_positiveSpace.x(), m_positiveSpace.y(),
					m_positiveSpace.width(), h);
	slotPositiveSpaceChanged(r);
}

void WindowManagerMinimal::removeCardWindow(Window* win)
{
	// This can happen if the window was not added to this WM
	if (!m_cardArray.contains(win)) {
		delete win;
		return;
	}

	m_cardArray.removeAll(win);

	if (m_cardArray.empty())
		m_activeWin = 0;
	else {
		m_activeWin = m_cardArray.first();
	}

	if (m_activeWin) {
		raiseChild(m_activeWin);
		static_cast<CardWindow*>(m_activeWin)->focusEvent(true);
		SystemUiController::instance()->setDirectRenderingForWindow (SystemUiController::CARD_WINDOW_MANAGER,
				static_cast<CardWindow*>(m_activeWin), true, false);
		resizeCardWindow(static_cast<CardWindow*>(m_activeWin), m_positiveSpace.width(), m_positiveSpace.height());	
		m_statusBar->setMaximizedAppTitle (true, m_activeWin->appDescription()->menuName().c_str());
	}

	SystemUiController::instance()->setActiveCardWindow(m_activeWin);	
	SystemUiController::instance()->setMaximizedCardWindow(m_activeWin);

	delete win;
}

void WindowManagerMinimal::removeAlertWindow(Window* win)
{
	if (!m_alertArray.contains(win)) {
		delete win;
		return;
	}

	bool isCurrentAlert = (m_alertArray.front() == win);

	m_alertArray.removeAll(win);
	delete win;

	if (!isCurrentAlert)
		return;

	if (m_alertArray.empty()) {
		const HostInfo& hostInfo = HostBase::instance()->getInfo();
		int h = hostInfo.displayHeight - Settings::LunaSettings()->positiveSpaceTopPadding;
		QRect r = QRect(m_positiveSpace.x(), m_positiveSpace.y(),
						m_positiveSpace.width(), h);
		slotPositiveSpaceChanged(r);

		return;
	}

	win = m_alertArray.front();

	int h = win->boundingRect().height();
	const HostInfo& hostInfo = HostBase::instance()->getInfo();
	win->setPos (0, hostInfo.displayHeight - h);

	h = hostInfo.displayHeight - Settings::LunaSettings()->positiveSpaceTopPadding - h;

	QRect r = QRect(m_positiveSpace.x(), m_positiveSpace.y(),
					m_positiveSpace.width(), h);
	slotPositiveSpaceChanged(r);
}

void WindowManagerMinimal::focusCardWindow(Window* win)
{
	if (!m_cardArray.contains(win)) {

		win->setParentItem(m_cardContainer);
		m_cardArray.append(win);
		
		win->setPos(m_cardContainer->boundingRect().center().x(), m_cardContainer->boundingRect().center().y() + m_statusBar->boundingRect().height()/2);
	}
	else if (m_activeWin == win) {
		return;
	}
	
	if (m_activeWin)
		SystemUiController::instance()->setDirectRenderingForWindow (SystemUiController::CARD_WINDOW_MANAGER,
				static_cast<CardWindow*>(m_activeWin), false, false);
	m_activeWin = win;

	raiseChild(win);

	static_cast<CardWindow*>(m_activeWin)->focusEvent(true);
	static_cast<CardWindow*>(m_activeWin)->setMaximized(true);
	SystemUiController::instance()->setDirectRenderingForWindow (SystemUiController::CARD_WINDOW_MANAGER,
		static_cast<CardWindow*>(m_activeWin), false, false);
	resizeCardWindow(static_cast<CardWindow*>(m_activeWin), m_positiveSpace.width(), m_positiveSpace.height());
	m_statusBar->setMaximizedAppTitle (true, m_activeWin->appDescription()->menuName().c_str());
	
	SystemUiController::instance()->setCardWindowMaximized(true);
	SystemUiController::instance()->setActiveCardWindow(m_activeWin);
	SystemUiController::instance()->setMaximizedCardWindow(m_activeWin);
}

void WindowManagerMinimal::focusAlertWindow(Window* win)
{
    
}

void WindowManagerMinimal::resize (int width, int height)
{
	WindowManagerBase::resize(width, height);

	if(m_statusBar) {
		m_statusBar->resize(boundingRect().width(), Settings::LunaSettings()->positiveSpaceTopPadding);
		m_statusBar->setPos(0, boundingRect().y() + m_statusBar->boundingRect().height() / 2);
		m_statusBar->update();
	}

	m_cardContainer->resize (width, height);
	m_cardContainer->setPos(boundingRect().center());

	m_positiveSpace.setWidth (width);
	m_positiveSpace.setHeight (height - m_statusBar->boundingRect().height());

	if (m_activeWin) {
		CardWindow* cardWin = static_cast<CardWindow*>(m_activeWin);
		cardWin->resizeWindowBufferEvent(width, height, m_positiveSpace);
		cardWin->setPos(m_cardContainer->boundingRect().center().x(), m_cardContainer->boundingRect().center().y() + m_statusBar->boundingRect().height()/2);
	}

	Q_FOREACH(Window* win, m_cardArray) {
		if (win == m_activeWin)
			continue;

        static_cast<CardWindow*>(win)->resizeWindowBufferEvent(width, height, m_positiveSpace);
		win->setPos(m_cardContainer->boundingRect().center().x(), m_cardContainer->boundingRect().center().y() + m_statusBar->boundingRect().height()/2);
	}
}

void WindowManagerMinimal::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPainter::CompositionMode previous = painter->compositionMode();
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

	painter->fillRect (boundingRect(), QColor (0, 0, 0, 0xff));

	painter->setCompositionMode(previous);

}

void WindowManagerMinimal::resizeCardWindow(CardWindow* win, int width, int height)
{
	if (win->allowResizeOnPositiveSpaceChange())
		win->resizeEvent(m_positiveSpace.width(), m_positiveSpace.height());
	else
		win->adjustForPositiveSpaceSize(m_positiveSpace.width(), m_positiveSpace.height());
}
