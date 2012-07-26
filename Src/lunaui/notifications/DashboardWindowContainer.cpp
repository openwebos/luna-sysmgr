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




#include "Common.h"

#include "DashboardWindowContainer.h"

#include <QPropertyAnimation>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QTapGesture>
#include <SysMgrDefs.h>
#include <QPropertyAnimation>

#include "DashboardWindow.h"
#include "DashboardWindowManager.h"
#include "FlickGesture.h"
#include "HostBase.h"
#include "Settings.h"
#include "SystemUiController.h"
#include "Time.h"
#include "Utils.h"
#include "WebAppMgrProxy.h"
#include "AnimationSettings.h"
#include "DockModeWindowManager.h"
#include "WindowServerLuna.h"
#include "CardDropShadowEffect.h"

static const qreal kMaxWindowsToDisplay = 5.5; // max height will be 5 1/2 windows
const int DashboardWindowContainer::sDashboardWindowHeight = 52;
const int DashboardWindowContainer::sDashboardBadgeWidth = 50;

DashboardWindowContainer::DashboardWindowContainer(DashboardWindowManager* wm, int width, int height)
	: GraphicsItemContainer(width, height, GraphicsItemContainer::SolidRectBackground)
	, m_wm(wm)
	, m_trackingMouseDirection(false)
	, m_vertLockedMovement(true)
	, m_isViewPortAnimationInProgress(false)
	, m_isWindowBeingDeleted(false)
	, m_animateVisibleViewportHeight(false)
	, m_IndexOfDeletedItem(-1)
	, m_FlickDirection(Ignore)
	, m_contentsHeight(0)
	, m_viewportHeight(0)
	, m_scrollBottom(0)
	, m_itemsDeleted(0)
	, m_verticalMouseMoveInProgress(0)
	, m_menuSeparatorHeight(0)
	, m_maskDisplayStatus(ShowNoMasks)
	, m_tabletTopMask(NULL)
	, m_tabletbottomMask(NULL)
	, m_tabletBackground(NULL)
	, m_tabletArrowUp(NULL)
	, m_tabletArrowDown(NULL)
	, m_menuSwipeBkg(NULL)
	, m_menuSwipeHighlight(NULL)
	, m_itemSeparator(NULL)
	, m_heightAnimation(this, &DashboardWindowContainer::heightAnimationValueChanged)
	, m_dashboardManualDrag(false)
{

	setObjectName("DashboardWindowContainer");
	m_isMenu = m_wm->isOverlay();

	grabGesture(Qt::TapGesture);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
		
	connect(&m_anim, SIGNAL(finished()), SLOT(slotProcessAnimationComplete()));
	connect(&m_deleteAnim, SIGNAL(finished()), SLOT(slotDeleteAnimationFinished()));

	// Initialize the pixmaps for use.
	initPixmaps();

	// Determine where to apply the padding depending on where we are running.
	if(m_isMenu) {
		m_FlickDirection = Ignore;
		m_maskDisplayStatus = ShowNoMasks;
		m_DashboardTopPadding = 0;
		m_operation = Invalid;
		m_BottomMaskHeightCorrection = 7;
		setAcceptTouchEvents(true);
	}
	else {
		m_BottomMaskHeightCorrection = 10;
		m_DashboardTopPadding = 10;
		SystemUiController* suc = SystemUiController::instance();

		connect(suc, SIGNAL(signalNegativeSpaceAboutToChange(const QRect&, bool, bool)),
				SLOT(slotNegativeSpaceAboutToChange(const QRect&, bool, bool)));
		connect(suc, SIGNAL(signalNegativeSpaceChanged(const QRect&)),
				SLOT(slotNegativeSpaceChanged(const QRect&)));
		connect(suc, SIGNAL(signalNegativeSpaceChangeFinished(const QRect&)),
				SLOT(slotNegativeSpaceChangeFinished(const QRect&)));
	}

	if(m_isMenu) {
		m_heightAnimation.setEasingCurve(AS_CURVE(dashboardSnapCurve));
		m_heightAnimation.setDuration(AS(dashboardSnapDuration));
		connect(&m_heightAnimation, SIGNAL(finished()), SIGNAL(signalContainerSizeChanged()));
	}
}

DashboardWindowContainer::~DashboardWindowContainer()
{
	if(NULL != m_tabletBackground)
		delete m_tabletBackground;

	if(NULL != m_tabletTopMask)
		delete m_tabletTopMask;

	if(NULL != m_tabletbottomMask)
		delete m_tabletbottomMask;

	if(NULL != m_tabletArrowUp)
		delete m_tabletArrowUp;

	if(NULL != m_tabletArrowDown)
		delete m_tabletArrowDown;

	if(NULL != m_menuSwipeBkg)
		delete m_menuSwipeBkg;

	if(NULL != m_menuSwipeHighlight)
		delete m_menuSwipeHighlight;

	if(NULL != m_itemSeparator)
		delete m_itemSeparator;
}

int DashboardWindowContainer::getMaximumHeightForMenu() const
{
	return ((int)(kMaxWindowsToDisplay * (qreal)sDashboardWindowHeight)) + ((kMaxWindowsToDisplay - 1) * m_menuSeparatorHeight);
}

void DashboardWindowContainer::resetLocalState(bool forceReset)
{
	m_itemsDeleted = 0;
	m_scrollBottom = 0;
	m_IndexOfDeletedItem = 0;
	m_isWindowBeingDeleted = false;
	m_animateVisibleViewportHeight = false;
	m_FlickDirection = Ignore;
	m_maskDisplayStatus = ShowNoMasks;

	if(true == forceReset) {
		m_viewportHeight = 0;
	}
}

void DashboardWindowContainer::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	m_anim.stop();
	m_anim.clear();

	m_trackingMouseDirection = true;
	m_vertLockedMovement = true;
	m_seenFlick = false;
	m_dashboardManualDrag = false;

	event->accept();

	m_draggedWindow.clear();

	DashboardWindow* w;
	Q_FOREACH(w, m_items) {
		if (m_pendingDeleteItems.contains(w))
			continue;		
		QRectF r = w->boundingRect();
		r.moveCenter(w->pos());
		if (r.contains(event->pos())) {
			m_draggedWindow = w;
			// check if this is a grouped notification window
			if(w->isManualDragWindow()) {
				if(w->x() == (m_isMenu ? w->boundingRect().width()/2 : 0)) { // check if the window is at its 'resting' position
					int winX = event->pos().x() + (m_isMenu ? 0 : w->boundingRect().width()/2);
					int winY = event->pos().y() - w->pos().y() + w->boundingRect().height()/2;
					if(winX > sDashboardBadgeWidth) {
						m_dashboardManualDrag = true;

						// send the mouse down to the web app side
						Event ev;
						ev.type = Event::PenDown;
						ev.setMainFinger(true);
						ev.x = winX;
						ev.y = winY;
						ev.clickCount = 1;
						ev.modifiers = Event::modifiersFromQt(event->modifiers());
						ev.time = Time::curSysTimeMs();

						w->inputEvent(&ev);
					}
				}
			}
			break;
		}
	}

	// Don't drag a pending delete item
	if (DashboardWindow* w = m_draggedWindow.data()) {
		if (m_pendingDeleteItems.contains(w))
			m_draggedWindow.clear();		
	}
}

void DashboardWindowContainer::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();

	if (G_UNLIKELY(m_trackingMouseDirection)) {

		int deltaX = event->pos().x() - event->buttonDownPos(Qt::LeftButton).x();
		int deltaY = event->pos().y() - event->buttonDownPos(Qt::LeftButton).y();

		if ((deltaX * deltaX + deltaY * deltaY) <
			Settings::LunaSettings()->tapRadiusSquared)
			return;

		m_trackingMouseDirection = false;

		if (abs(deltaX) > abs(deltaY)) {
			m_vertLockedMovement = false;
			Q_EMIT signalItemDragState(true);
		} else if(m_dashboardManualDrag) {
			QGraphicsSceneMouseEvent ev;
			ev.setCanceled(true);
			mouseReleaseEvent(&ev);
			m_dashboardManualDrag = false;
		}
	}

	if(m_isMenu && m_vertLockedMovement)
		return;

	if (m_vertLockedMovement) {

		// Set the flag that a vertical mouse move is in progress, so that we can change the # of items getting painted.
		if(false == m_verticalMouseMoveInProgress) 
			m_verticalMouseMoveInProgress = true;

		int deltaY = event->pos().y() - event->lastPos().y();
		if (deltaY == 0) {
			return;
		}

		setScrollBottom(m_scrollBottom - deltaY);

		// 	Set the direction of the flick
		if(Ignore == m_FlickDirection) {
			if(deltaY > 0) {
				m_FlickDirection = FlickDown;
			}
			else {
				m_FlickDirection = FlickUp;
			}
		}

		showOrHideMasks();

		return;
	}

	
	if (DashboardWindow* w = m_draggedWindow.data()) {
		if(!m_dashboardManualDrag) {
			// draw the window
			int deltaX = event->pos().x() - event->lastPos().x();
			if (deltaX == 0)
				return;

			w->setPos(MAX(w->boundingRect().width()/2, w->pos().x() + deltaX), w->pos().y());
		} else {
			// send the mouse events to the window
			int winX = event->pos().x() + (m_isMenu ? 0 : w->boundingRect().width()/2);
			int winY = event->pos().y() - w->pos().y() + w->boundingRect().height()/2;

			// send the mouse down to the web app side
			Event ev;
			ev.type = Event::PenMove;
			ev.setMainFinger(true);
			ev.x = winX;
			ev.y = winY;
			ev.clickCount = 1;
			ev.modifiers = Event::modifiersFromQt(event->modifiers());
			ev.time = Time::curSysTimeMs();

			w->inputEvent(&ev);
		}
	}
}

void DashboardWindowContainer::mouseWasGrabbedByParent()
{
	// fake a mouse cancel
	QGraphicsSceneMouseEvent ev;
	ev.setCanceled(true);
	mouseReleaseEvent(&ev);
}

void DashboardWindowContainer::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();

	Q_EMIT signalItemDragState(false);

	if (m_seenFlick) {
		m_seenFlick = false;
		m_dashboardManualDrag = false;
		return;
	}

	if (DashboardWindow* w = m_draggedWindow.data()) {

		if(!m_dashboardManualDrag) {
			if (event->canceled() || w->persistent())
				goto Done;

			// A window was being dragged around. Did it move too far?
			int width = w->boundingRect().width();
			if(!m_isMenu) {
				if (abs(w->pos().x()) > width/4) {
					triggerItemDelete(w);
					// Restoring of other items will occur when the delete animation finished
					return;
				}
			} else {
				if (abs(w->pos().x() - width/2) > width/4) {
					triggerItemDelete(w);
					// Restoring of other items will occur when the delete animation finished
					return;
				}
			}
		} else {
			// send the mouse event to the window
			int winX = event->pos().x() + (m_isMenu ? 0 : w->boundingRect().width()/2);
			int winY = event->pos().y() - w->pos().y() + w->boundingRect().height()/2;

			Event ev;
			if (event->canceled())
				ev.type = Event::PenCancel;
			else
				ev.type = Event::PenUp;
			ev.setMainFinger(true);
			ev.x = winX;
			ev.y = winY;
			ev.clickCount = 1;
			ev.modifiers = Event::modifiersFromQt(event->modifiers());
			ev.time = Time::curSysTimeMs();

			w->inputEvent(&ev);
		}

		m_draggedWindow.clear();
	}

Done:
	if(!m_isMenu)
		restoreNonDeletedItems();
	else {
		// If we are flicking down, then we always need to snap back up to the first window, so there is no need to recalculate the new scroll bottom. Same is the case if we have 5 windows
		restoreNonDeletedItems(false);
	}
}

bool DashboardWindowContainer::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {
		QGesture* g = static_cast<QGestureEvent*>(event)->gesture(Qt::TapGesture);
		if (g) {
			QTapGesture* tap = static_cast<QTapGesture*>(g);
			if (tap->state() == Qt::GestureFinished) {
				handleTap(mapFromScene(tap->position()));
			}
			
			return true;
		}

		if(!m_isMenu) {
			g = static_cast<QGestureEvent*>(event)->gesture((Qt::GestureType) SysMgrGestureFlick);
			if (g) {
				FlickGesture* flick = static_cast<FlickGesture*>(g);
				if (flick->state() == Qt::GestureFinished) {
					bool flick_delete = abs(flick->velocity().x()) > abs(flick->velocity().y());

					if (flick_delete) {
						if (DashboardWindow* w = m_draggedWindow.data()) {
							m_draggedWindow.clear();
							if (!w->persistent()) {
								m_seenFlick = true;
								triggerItemDelete(w);
							}
						}
					}
				}
				return true;
			}
		}
	} else if (event->type() == QEvent::TouchBegin) {
		return true;
	} else if (event->type() == QEvent::TouchUpdate) {
		return true;
	} else if (event->type() == QEvent::TouchEnd){
		return true;
	}

	return QGraphicsObject::sceneEvent(event);        
}

void DashboardWindowContainer::animateResize(int width, int height)
{
	if(m_heightAnimation.state() != QVariantAnimation::Stopped)
		m_heightAnimation.stop();

	if(!isVisible()) {
		// not visible, so do not animate
		resize(width, height);
		Q_EMIT signalContainerSizeChanged();
	}
	else {
		resize(width, boundingRect().height());

		m_heightAnimation.setStartValue((int)(boundingRect().height()));
		m_heightAnimation.setEndValue(height);
		m_heightAnimation.start();
	}
}

void DashboardWindowContainer::heightAnimationValueChanged( const QVariant & value)
{
	if(m_heightAnimation.state() != QVariantAnimation::Stopped) {
		int height = value.toInt();

		resize(boundingRect().width(),height);
		Q_EMIT signalContainerSizeChanged();
	}
}

QVariant DashboardWindowContainer::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if (change == ItemVisibleHasChanged) {

		bool val = value.toBool();
		if (!val) {

			m_anim.stop();
			m_anim.clear();

			if (!m_pendingDeleteItems.empty()) {

				m_deleteAnim.stop();
				m_deleteAnim.clear();

				DashboardWindow* w;
				Q_FOREACH(w, m_pendingDeleteItems) {
					m_items.removeOne(w);
					Q_EMIT signalWindowsRemoved(w);
					++m_itemsDeleted;
					if (w->removed()) {
						// Window has already been "removed". Just delete it
						delete w;
						continue;
					}
					m_deletedItems.insert(w);
				}

				if(m_isMenu) {
					if(m_itemsDeleted > 0) {
						animateResize(m_contentWidth, (m_items.size() * sDashboardWindowHeight + (m_items.size()-1) * m_menuSeparatorHeight));
					}
				}

				// Determine whether single/multiple windows were removed.
				if(m_itemsDeleted > 1) {
					m_operation = MultipleWindowsRemoved;
				}
				else {
					m_operation = SingleWindowRemoved;
				}

				m_pendingDeleteItems.clear();
				calculateScrollProperties();
				
				if (m_items.empty()) {
					Q_EMIT signalEmpty();
					resetLocalState(true);
				}
			}
		}
	}
	return GraphicsItemContainer::itemChange(change, value);
}

// On the tablet we reverse the order in which we show the dashboard windows. So, we need to provide a different way to set the scroll bottom when the user scrolls the dashboard windows on the screen.
void DashboardWindowContainer::layoutAllWindowsInMenu()
{
	if(!m_isMenu)
		return;

	int y = 0, delta = 0;

	y = boundingRect().y();
	y += sDashboardWindowHeight / 2;

	// Layout all items, excluding deleted items
	for (int i = m_items.count() - 1; i >= 0; i--) {
		if (!m_pendingDeleteItems.contains(m_items[i])) {
			m_items[i]->setPos(m_items[i]->boundingRect().width()/2, y);
			y += sDashboardWindowHeight + m_menuSeparatorHeight;
		}
	}
}



void DashboardWindowContainer::setScrollBottom(int newBottom)
{
	if(m_isMenu)
		return;

	m_scrollBottom = newBottom;
	int y = boundingRect().y() + m_viewportHeight;

	y -= sDashboardWindowHeight / 2;
	y -= m_scrollBottom;

	// Layout all items, excluding deleted items
	for (int i = m_items.count() - 1; i >= 0; i--) {
		if (!m_pendingDeleteItems.contains(m_items[i])) {
			m_items[i]->setPos(0, y);
			y -= sDashboardWindowHeight;
		}
	}
}

void DashboardWindowContainer::resizeWindowsEventSync(int w)
{
	DashboardWindow* dw = 0;
	for (int i = 0; i < m_items.size(); ++i) {
		dw = m_items.at(i);
		if (dw) {
			dw->resizeEventSync(w, dw->initialHeight());
			setPosTopLeft(dw, 0, 0);
		}
	}

	calculateScrollProperties();
}

QRectF DashboardWindowContainer::boundingRect() const
{
	if(!m_isMenu)
		return QRectF(-m_contentWidth/2, -m_contentHeight/2, m_contentWidth, m_contentHeight);
	else {
		// when inside a QML menu, this item is top left aligned (to match the coordinate system used by QML)
		return QRectF(0, 0, m_contentWidth, m_contentHeight);
	}
}

void DashboardWindowContainer::addWindow(DashboardWindow* win)
{
	if(!m_isMenu) {
		win->resizeEventSync(SystemUiController::instance()->currentUiWidth(), sDashboardWindowHeight);
	}
	else {
		// $$$ remove resize for menu
		win->resizeEventSync(m_contentWidth, sDashboardWindowHeight);
	}

	update();
	
	win->setParentItem(this);
	m_items.push_back(win);

	Q_EMIT signalWindowAdded(win);

	// if we are running on a tablet, update the height
	if(m_isMenu) {
		animateResize(m_contentWidth, (m_items.size() * sDashboardWindowHeight + (m_items.size()-1) * m_menuSeparatorHeight));
		m_operation = SingleWindowAdded;
	}

	calculateScrollProperties();

	setPosTopLeft(win, 0, 0);

	// Reset scroll bottom
	m_scrollBottom = 0;
   	
	if(!m_isMenu)
		win->setPos(0, m_scrollBottom + sDashboardWindowHeight);
	else {
		win->setPos(win->boundingRect().width()/2, m_scrollBottom - sDashboardWindowHeight);
	}

	restoreNonDeletedItems(false);

	if (!isVisible()) {
		// if dashboard is not open  send a defocus event to the window
		WebAppMgrProxy::instance()->focusEvent(win, false);
	} else {
		// Send a focus event to window
		WebAppMgrProxy::instance()->focusEvent(win, true);
	}
}

void DashboardWindowContainer::removeWindow(DashboardWindow* w)
{
	// Item has already been animated for delete
	if (m_deletedItems.contains(w)) {
		m_deletedItems.remove(w);
		delete w;
		return;
	}

	// This happens if a window is removed before being added to this window
	if (!m_items.contains(w)) {
		delete w;
		return;
	}

	if (!isVisible()) {
		m_pendingDeleteItems.remove(w);
		m_items.removeOne(w);

		Q_EMIT signalWindowsRemoved(w);

		if(m_isMenu) {
			animateResize(m_contentWidth, (m_items.size() * sDashboardWindowHeight + (m_items.size()-1) * m_menuSeparatorHeight));
			m_operation = SingleWindowRemoved;
		}

		if (m_items.empty()) {
			Q_EMIT signalEmpty();
			resetLocalState(true);
		}

		delete w;

		calculateScrollProperties();

		return;
	}

	// indicate that once the animation is done it should be ok to go ahead and delete this window
	w->setRemoved();

	if (m_pendingDeleteItems.contains(w)) {
		return;
	}

	m_pendingDeleteItems.insert(w);

	raiseChild(w);
	QPropertyAnimation* a = new QPropertyAnimation(w, "pos");
	a->setEndValue(QPointF(w->pos().x() + 1.5 * w->boundingRect().width(),
						   w->pos().y()));
	a->setDuration(AS(dashboardDeleteDuration));
	a->setEasingCurve(AS_CURVE(dashboardDeleteCurve));
	
	m_deleteAnim.addAnimation(a);
	m_deleteAnim.start();
}

bool DashboardWindowContainer::empty() const
{
	return m_items.empty();    
}

void DashboardWindowContainer::focusAllWindows(bool focus)
{
	WebAppMgrProxy* wm = WebAppMgrProxy::instance();
	DashboardWindow* w;
	Q_FOREACH(w, m_items)
		wm->focusEvent(w, focus);
}

void DashboardWindowContainer::slotDeleteAnimationFinished()
{
	m_deleteAnim.clear();
	
	DashboardWindow* w;
	Q_FOREACH(w, m_pendingDeleteItems) {
		m_items.removeOne(w);
		++m_itemsDeleted;
		Q_EMIT signalWindowsRemoved(w);
		if (w->removed()) {
			// window already closed by Webkit, so delete it
			delete w;
		} else {
			// put it on the list of deleted windows until we receive a removeWindow for it
			m_deletedItems.insert(w);
		}
	}

	m_pendingDeleteItems.clear();

	// Recalculate the scroll props
	if(m_isMenu) {
		if(0 != m_itemsDeleted) {
			animateResize(m_contentWidth, (m_items.size() * sDashboardWindowHeight + (m_items.size()-1) * m_menuSeparatorHeight));
			if(m_itemsDeleted > 1) {
				m_operation = MultipleWindowsRemoved;
			}
			else {
				m_operation = SingleWindowRemoved;
			}
		}
	}

	calculateScrollProperties();

	// calculateScrollProperties() changes the Y position of the container, causing the windows to get shifted and this causes the animation to shift up and then move downwards.
	if(m_isMenu) {
		animateWindowsToFinalDestinationInMenu(boundingRect().y());
	}

	if (m_items.empty()) {
		Q_EMIT signalEmpty();
		resetLocalState(true);
		return;
	}

	if (!isVisible())
		return;

	restoreNonDeletedItems();
}

void DashboardWindowContainer::slotProcessAnimationComplete()
{
	if(!m_isMenu) {
		// if we were showing any masks continue showing them or hide them
		switch(m_maskDisplayStatus) {
		case ShowNoMasks:
		case TopMaskOnly:
			// Check if we still need to show the top mask
			if((m_scrollBottom + m_contentsHeight - m_viewportHeight) <= 0) {
				m_maskDisplayStatus = ShowNoMasks;
			}
			break;
		case BottomMaskOnly:
			// We were showing the bottom mask only. This happens if there are more items than we are showing currently.
			if(m_scrollBottom > 0) {
				if(m_contentsHeight == m_viewportHeight) {
					m_maskDisplayStatus = ShowNoMasks;
				}
				else {
					m_maskDisplayStatus = ShowBothMasks;
				}
			}
			break;
		case ShowBothMasks:
			// Top Mask must be made invisible when scrolling stops. The only case is to check if we need to enable bottom mask
			if((m_scrollBottom + m_contentsHeight - m_viewportHeight) > 0) {
				if(m_scrollBottom < 0) {
					m_maskDisplayStatus = ShowBothMasks;
				}
				else {
					m_maskDisplayStatus = TopMaskOnly;
				}
			}
			else {
				if(m_scrollBottom < 0) {
					m_maskDisplayStatus = ShowBothMasks;
				}
				else {
					m_maskDisplayStatus = BottomMaskOnly;
				}
			}
		}
	}

	m_verticalMouseMoveInProgress = false;
	m_FlickDirection = Ignore;
	m_isWindowBeingDeleted = false;
	m_operation = Invalid;

	if(ShowNoMasks != m_maskDisplayStatus)
		m_maskDisplayStatus = ShowNoMasks;
}

// Will be called ONLY by the phone version [not the tablet version]
void DashboardWindowContainer::animateWindowsToFinalDestination(int yCoOrd)
{
	// Cannot be called when not running as a phone
	if(m_isMenu)
		return;

	// clear any existing animation if in progress
	m_anim.clear();

	// Initialize the starting position
	int y = yCoOrd;
	y -= sDashboardWindowHeight / 2;
	y -= m_scrollBottom;

	DashboardWindow* w;

	for (int i = m_items.count() - 1; i >= 0; i--) {
		w = m_items[i];
		if (m_pendingDeleteItems.contains(w))
			continue;

		QPropertyAnimation* a = new QPropertyAnimation(w, "pos");
		a->setEndValue(QPointF(0, y));
		a->setEasingCurve(AS_CURVE(dashboardSnapCurve));
		a->setDuration(AS(dashboardSnapDuration));
		m_anim.addAnimation(a);
		y -= sDashboardWindowHeight;
	}

	// start the animation
	m_anim.start();
}

void DashboardWindowContainer::animateWindowsToFinalDestinationInMenu(int topCoord)
{
	if(!m_isMenu)
		return;

	// clear any existing animation if in progress
	m_anim.stop();
	m_anim.clear();

	// Initialize the starting position
	int y = topCoord;
	y += sDashboardWindowHeight / 2;

	DashboardWindow* w;

	for (int i = m_items.count() - 1; i >= 0; i--) {
		w = m_items[i];
		if (m_pendingDeleteItems.contains(w))
			continue;

		QPropertyAnimation* a = new QPropertyAnimation(w, "pos");
		a->setEndValue(QPointF(w->boundingRect().width()/2, y));
		a->setEasingCurve(AS_CURVE(dashboardSnapCurve));
		a->setDuration(AS(dashboardSnapDuration));
		m_anim.addAnimation(a);
		y += sDashboardWindowHeight + m_menuSeparatorHeight;
	}

	// start the animation
	m_anim.start();
}

// sets teh position of the dashboard windows without animating them
void DashboardWindowContainer::setWindowsToFinalDestinationInMenu(int topCoord)
{
	if(!m_isMenu)
		return;

	// clear any existing animation if in progress
	m_anim.stop();
	m_anim.clear();

	// Initialize the starting position
	int y = topCoord;
	y += sDashboardWindowHeight / 2;

	DashboardWindow* w;

	for (int i = m_items.count() - 1; i >= 0; i--) {
		w = m_items[i];
		if (m_pendingDeleteItems.contains(w))
			continue;

		w->setPos(QPointF(w->boundingRect().width()/2, y));
		y += sDashboardWindowHeight + m_menuSeparatorHeight;
	}
}

void DashboardWindowContainer::restoreNonDeletedItems(bool recalcScrollBottom)
{
	if (m_items.empty())
		return;

	// Call an appropriate function depending on whether we are running on tablet/phone
	if(m_isMenu) {
		m_scrollBottom = 0;

		if (isVisible()) {
			animateWindowsToFinalDestinationInMenu(boundingRect().y());
		} else {
			setWindowsToFinalDestinationInMenu(boundingRect().y());
		}
	} else {
		int y = boundingRect().y() + m_viewportHeight;

		if (recalcScrollBottom) {

			int deltaY = 9999999; // artificially large number
			DashboardWindow* windowClosestToBottomEdge = 0;

			for (int i = m_items.count() - 1; i >= 0; i--) {
				int dy = abs(m_items[i]->pos().y() - y);
				if (dy < deltaY) {
					deltaY = dy;
					windowClosestToBottomEdge = m_items[i];
				}
			}

			m_scrollBottom = 0;
			for (int i = m_items.count() - 1; i >= 0; i--) {
				if (m_items[i] == windowClosestToBottomEdge)
					break;
				m_scrollBottom -= sDashboardWindowHeight;
			}
		}

		// Also, make sure that there is no empty space left in the viewport
		m_scrollBottom = MAX(m_scrollBottom, m_viewportHeight - m_contentsHeight);

		// Make sure scrollBottom is a multiple of dashboard window heights
		m_scrollBottom = (m_scrollBottom / sDashboardWindowHeight) * sDashboardWindowHeight;

		animateWindowsToFinalDestination(boundingRect().y() + m_viewportHeight);
	}
}

void DashboardWindowContainer::calculateScrollProperties()
{
	int actualDisplayHeight = 0;
	int itemCount = 0;

	if(m_isMenu) {

		itemCount = m_items.size();

		actualDisplayHeight = itemCount * DashboardWindowContainer::sDashboardWindowHeight  + (itemCount-1) * m_menuSeparatorHeight;

		m_itemsDeleted = 0;
		m_operation = Invalid;
	}

	m_contentsHeight = (m_items.size() * sDashboardWindowHeight + (m_items.size()-1) * m_menuSeparatorHeight);

	if (!m_items.isEmpty())
		m_contentsHeight += m_DashboardTopPadding;

	int newViewportHeight = MIN(m_contentsHeight, SystemUiController::instance()->currentUiHeight() -
								SystemUiController::instance()->minimumPositiveSpaceBottom());

	if (newViewportHeight != m_viewportHeight) {
		m_viewportHeight = newViewportHeight;
		if (isVisible())
			Q_EMIT signalViewportHeightChanged();
    }
}

void DashboardWindowContainer::slotNegativeSpaceAboutToChange(const QRect& r, bool, bool screenResizing)
{
	if (!isVisible())
		return;
}

void DashboardWindowContainer::slotNegativeSpaceChanged(const QRect& r)
{
	if (!isVisible() || m_isMenu)
		return;

	showOrHideMasks();
}

void DashboardWindowContainer::slotNegativeSpaceChangeFinished(const QRect& r)
{
	if (!isVisible())
		return;
}

void DashboardWindowContainer::setMaskVisibility(bool& topMask, bool& bottomMask)
{
	if(m_isMenu)
		return;

	topMask = bottomMask = false;

	// determine bottom mask visibility
	if (m_scrollBottom < 0)
		bottomMask = true;
	else
		bottomMask = false;

	// determine top mask visibility
	if ((m_scrollBottom + m_contentsHeight - m_viewportHeight) > 0)
		topMask = true;
	else
		topMask = false;
}

// determine if the masks need to be made visible.
void DashboardWindowContainer::showOrHideMasks()
{
	bool showTopMask = false, showBottomMask = false;

	// get which masks need to be made visible
	setMaskVisibility(showTopMask, showBottomMask);

	if(true == showTopMask && true == showBottomMask)
		m_maskDisplayStatus = ShowBothMasks;
	else if(true == showTopMask && false == showBottomMask)
		m_maskDisplayStatus = TopMaskOnly;
	else if(false == showTopMask && true == showBottomMask)
		m_maskDisplayStatus = BottomMaskOnly;
	else if(false == showTopMask && false == showBottomMask)
		m_maskDisplayStatus = ShowNoMasks;
}

void DashboardWindowContainer::sendClickToDashboardWindow(int num, QPointF tap, bool whileLocked)
{
	DashboardWindow* w = m_items[num];
	
	if(!w || m_pendingDeleteItems.contains(w))
		return;
	
	QRectF r = w->boundingRect();
	
	if (r.contains(tap)) {
		tap -= w->boundingRect().topLeft();

		if (whileLocked && !w->clickableWhenLocked()) 
			return;

		// FIXME: Move to Qt mouse event

		Event evtDown;
		evtDown.type = Event::PenDown;
		evtDown.x = tap.x();
		evtDown.y = tap.y();
		evtDown.z = 0;
		evtDown.key = Event::Key_Null;
		evtDown.button = Event::Left;
		evtDown.modifiers = 0;
		evtDown.time = Time::curTimeMs();
		evtDown.clickCount = 1;

		WebAppMgrProxy::instance()->inputEvent(w, &evtDown);
			
		Event evtUp;
		evtUp.type = Event::PenUp;
		evtUp.x = tap.x();
		evtUp.y = tap.y();
		evtUp.z = 0;
		evtUp.key = Event::Key_Null;
		evtUp.button = Event::Left;
		evtUp.modifiers = 0;
		evtUp.time = Time::curTimeMs();
		evtUp.clickCount = 0;

		WebAppMgrProxy::instance()->inputEvent(w, &evtUp);    
		
		return;
	}
}

void DashboardWindowContainer::handleTap(const QPointF& pos)
{
	DashboardWindow* w;
	Q_FOREACH(w, m_items) {
		if (m_pendingDeleteItems.contains(w))
			continue;		
		QRectF r = w->boundingRect();
		r.moveCenter(w->pos());
		if (r.contains(pos)) {
			QPointF p = w->mapFromParent(pos);
			p -= w->boundingRect().topLeft();
		
			// FIXME: Move to Qt mouse event

			Event evtDown;
			evtDown.type = Event::PenDown;
			evtDown.x = p.x();
			evtDown.y = p.y();
			evtDown.z = 0;
			evtDown.key = Event::Key_Null;
			evtDown.button = Event::Left;
			evtDown.modifiers = 0;
			evtDown.time = Time::curTimeMs();
			evtDown.clickCount = 1;

			WebAppMgrProxy::instance()->inputEvent(w, &evtDown);
				
			Event evtUp;
			evtUp.type = Event::PenUp;
			evtUp.x = p.x();
			evtUp.y = p.y();
			evtUp.z = 0;
			evtUp.key = Event::Key_Null;
			evtUp.button = Event::Left;
			evtUp.modifiers = 0;
			evtUp.time = Time::curTimeMs();
			evtUp.clickCount = 0;

			WebAppMgrProxy::instance()->inputEvent(w, &evtUp);    
			
			return;
		}
	}
}

void DashboardWindowContainer::triggerItemDelete(DashboardWindow* win)
{
	if (m_pendingDeleteItems.contains(win))
		return;

	// Set the flag that the window is being deleted.
	m_isWindowBeingDeleted = true;

	// Get the index of the window we are deleting.
	m_IndexOfDeletedItem = m_items.indexOf(win, 0);

	// If we deleted the window with index 0 and we have <=5 windows, we have to animate in the new viewport height and not just set it to the final value.
	m_animateVisibleViewportHeight = true;

	WebAppMgrProxy::instance()->closeWindow(win);
	m_pendingDeleteItems.insert(win);

	raiseChild(win);

	int width = win->boundingRect().width();
	
	QPropertyAnimation* a = new QPropertyAnimation(win, "pos");
	a->setEndValue(QPointF((win->pos().x() > (m_isMenu ? win->boundingRect().width()/2 : 0)) ?
						   (win->pos().x() + 1.5 * width) :
						   (win->pos().x() - 1.5 * width),
						   win->pos().y()));
	a->setDuration(AS(dashboardDeleteDuration));
	a->setEasingCurve(AS_CURVE(dashboardDeleteCurve));
			
	m_deleteAnim.addAnimation(a);
	m_deleteAnim.start();
}

// Initiate the PixMaps to be drawn
void DashboardWindowContainer::initPixmaps()
{
	if(!m_isMenu) {
		// Initialize normal Dashboard Container pixmaps

		// Create the top pixMap
		std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/dashboard-mask-top.png";
		if(NULL == m_tabletTopMask) {
			m_tabletTopMask = new QPixmap(filePath.c_str(), 0, Qt::AutoColor);
		}

		// Create the bottom pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/dashboard-mask-bottom.png";
		if(NULL == m_tabletbottomMask) {
			m_tabletbottomMask = new QPixmap(filePath.c_str(), NULL, Qt::AutoColor);
		}

	} else {
		// Initialize Menu pixmaps

		// Create the background pixMap
		std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-dropdown-bg.png";
		if(NULL == m_tabletTopMask) {
			m_tabletBackground = new QPixmap(filePath.c_str());
		}

		// Create the top pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-dropdown-scrollfade-top.png";
		if(NULL == m_tabletTopMask) {
			m_tabletTopMask = new QPixmap(filePath.c_str(), 0, Qt::AutoColor);
		}

		// Create the bottom pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-dropdown-scrollfade-bottom.png";
		if(NULL == m_tabletbottomMask) {
			m_tabletbottomMask = new QPixmap(filePath.c_str(), NULL, Qt::AutoColor);
		}

		// Create the arrowUp pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-arrow-up.png";
		if(NULL == m_tabletArrowUp) {
			m_tabletArrowUp = new QPixmap(filePath.c_str(), NULL, Qt::AutoColor);
		}

		// Create the arrowDown pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-arrow-down.png";
		if(NULL == m_tabletArrowDown) {
			m_tabletArrowDown = new QPixmap(filePath.c_str(), NULL, Qt::AutoColor);
		}

		// Create the arrowDown pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-dropdown-swipe-bg.png";
		if(NULL == m_menuSwipeBkg) {
			m_menuSwipeBkg = new QPixmap(filePath.c_str(), NULL, Qt::AutoColor);
		}

		// Create the arrowDown pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-dropdown-swipe-highlight.png";
		if(NULL == m_menuSwipeHighlight) {
			m_menuSwipeHighlight = new QPixmap(filePath.c_str(), NULL, Qt::AutoColor);
		}

		// Create the arrowDown pixMap
		filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-divider.png";
		if(NULL == m_itemSeparator) {
			m_itemSeparator = new QPixmap(filePath.c_str(), NULL, Qt::AutoColor);
		}

		if(NULL != m_itemSeparator) {
			m_menuSeparatorHeight = m_itemSeparator->height();
		}
	}
}

void DashboardWindowContainer::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if(m_isMenu) {
		// redirect the painting to the Menu painting function
		paintInsideMenu(painter);
		return;
	}

	int itemsToDisplay = 0, startIndex = 0, endIndex = 0, curSize = m_items.size();
	QRect bRect = boundingRect().toRect();

	if (m_items.empty()) {
		return;
	}

	QPainter::CompositionMode previous = painter->compositionMode();
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

	startIndex = 0;
	endIndex = curSize;

	painter->fillRect(bRect.x(), bRect.y(), bRect.width(), bRect.height(),  Qt::black);
	painter->setClipRect(bRect.x(), bRect.y(), bRect.width(), bRect.height());

	for(int i = startIndex;;) {
		DashboardWindow* w = m_items.at(i);
		QPoint p = w->pos().toPoint();

		const QPixmap* pix = w->acquireScreenPixmap();
		if (!pix) {
			g_critical("%s: Failed to acquireScreenPixmap", __PRETTY_FUNCTION__);
			continue;
		}

		p -= QPoint(pix->width() / 2, pix->height() / 2);

		painter->drawPixmap(p, *pix);

		// Check if we need to continue or if we are done painting
		++i;
		if(i < endIndex)
			continue;
		else
			break;
	}

	// Draw the masks if needed
	if(ShowNoMasks != m_maskDisplayStatus){
		switch(m_maskDisplayStatus) {
			case ShowBothMasks:
			case TopMaskOnly:
				painter->drawTiledPixmap(bRect.x(), bRect.y(), bRect.width(), m_tabletTopMask->height(), *m_tabletTopMask, 0, 0);
				if(TopMaskOnly == m_maskDisplayStatus)
					break;
			case BottomMaskOnly:
				// If we have more items than we are displaying, set the y co-ord correctly
				if(m_contentsHeight == m_viewportHeight) {
					painter->drawTiledPixmap(bRect.x(), (bRect.y() + (m_items.size() * 52)), bRect.width(), m_tabletbottomMask->height(), *m_tabletbottomMask, 0, 0);
				}
				else {
					painter->drawTiledPixmap(bRect.x(), (bRect.y() + m_viewportHeight - m_BottomMaskHeightCorrection), bRect.width(), m_tabletbottomMask->height(), *m_tabletbottomMask, 0, 0);
				}
				break;
			default:
				break;
		}
	}

	// restore the painter
	painter->setCompositionMode(previous);
	painter->setClipPath(QPainterPath(), Qt::NoClip);
	painter->setClipping(false);
}

void DashboardWindowContainer::paintInsideMenu(QPainter* painter)
{
	int itemsToDisplay = 0, startIndex = 0, endIndex = 0, curSize = m_items.size();
	QRect bRect = boundingRect().toRect();
	QRect shade;

	if (m_items.empty()) {
		return;
	}

	QPainter::CompositionMode previous = painter->compositionMode();
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

	endIndex = 0;
	startIndex = curSize - 1;

	bool clipWasEnabled = painter->hasClipping();
	QPainterPath oldPath = painter->clipPath();

	painter->setClipRect(bRect.x(), bRect.y(), bRect.width(), bRect.height(), Qt::IntersectClip);

	// paint the dashboard windows
	for(int i = startIndex;;) {
		DashboardWindow* w = m_items.at(i);
		QPoint p = w->pos().toPoint();
		QRectF  wRect = w->boundingRect();
		bool separator = m_itemSeparator && ((i != startIndex) || (p.y() > wRect.height()/2));

		const QPixmap* pix = w->acquireScreenPixmap();
		if (!pix) {
			g_critical("%s: Failed to acquireScreenPixmap", __PRETTY_FUNCTION__);
			continue;
		}

		p -= QPoint(pix->width() / 2, pix->height() / 2);

		painter->drawPixmap(p, *pix);

		// if it is not the first window, paint the item divider above the window
		if(G_LIKELY(separator)) {
			painter->drawPixmap(p.x(), p.y() - m_menuSeparatorHeight, pix->width(), m_menuSeparatorHeight, *m_itemSeparator);
		}

		// if the window has been moved from its default position, then paint the background assets
		if(m_menuSwipeBkg && m_menuSwipeHighlight) {
			if(w->pos().x() > wRect.width()/2) {
				shade.setRect(0,
							  p.y() - (separator ? m_menuSeparatorHeight : 0),
							  MIN(w->pos().x() - wRect.width()/2, wRect.width()),
							  wRect.height() + (separator ? m_menuSeparatorHeight : 0));

				paintHoriz3Tile(painter, m_menuSwipeBkg,
								shade.x(), shade.y(), shade.width(), shade.height(),
								MIN(shade.width(), 5), MIN(shade.width(), 5));

				painter->drawPixmap(shade.right() - m_menuSwipeHighlight->width() + 1, shade.top(), m_menuSwipeHighlight->width(), shade.height(),
						            *m_menuSwipeHighlight, 0, 0, m_menuSwipeHighlight->width(), m_menuSwipeHighlight->height());
			} else if (w->pos().x() < wRect.width()/2) {
				shade.setRect(MAX(0, w->pos().x() + wRect.width()/2),
							  p.y() - (separator ? m_menuSeparatorHeight : 0),
							  MIN(wRect.width()/2 - w->pos().x(), wRect.width()),
							  wRect.height() + (separator ? m_menuSeparatorHeight : 0));

				paintHoriz3Tile(painter, m_menuSwipeBkg,
								shade.x(), shade.y(), shade.width(), shade.height(),
								MIN(shade.width(), 5), MIN(shade.width(), 5));

				painter->drawPixmap(shade.left(), shade.top(), m_menuSwipeHighlight->width(), shade.height(),
						            *m_menuSwipeHighlight, 0, 0, m_menuSwipeHighlight->width(), m_menuSwipeHighlight->height());
			}
		}

		if((i == startIndex) && (w->pos().y() > wRect.height()/2)) {
			// for for the first window, check if it is not at the top (if the previous first window was deleted), and if so paint a shadow on the top
			paintHoriz3Tile(painter, m_menuSwipeBkg,
							0, 0, boundingRect().width(), w->pos().y() - wRect.height()/2 - m_menuSeparatorHeight,
							5, 5);
		} else if((i == endIndex) && ((w->pos().y() + wRect.height()/2) < boundingRect().bottom())) {
			// for for the last window, check if it is not at the bottom (if the previous last window was deleted), and if so paint a shadow on the bottom
			int bottomStart = w->pos().y() + wRect.height()/2;
			paintHoriz3Tile(painter, m_menuSwipeBkg,
							0, bottomStart, boundingRect().width(), boundingRect().bottom() - bottomStart,
							5, 5);
		}

		// move the index on to the next item
		--i;

		if(i >= endIndex) {
			// check if there is a gap between the current item and the next one, and fill the gap with a shade

			int endThis = (w->pos().y() + wRect.height()/2);
			int startNext = m_items.at(i)->pos().y() - m_items.at(i)->boundingRect().height()/2 - m_menuSeparatorHeight;
			if(startNext > endThis) {
				// fill the gap with a shadow
				paintHoriz3Tile(painter, m_menuSwipeBkg,
								0, endThis, boundingRect().width(), startNext - endThis,
								5, 5);
			}
			// we still have more items to paint
			continue;
		} else {
			// we are done
			break;
		}
	}

	// restore the painter
	painter->setCompositionMode(previous);
	painter->setClipPath(oldPath, clipWasEnabled ? Qt::ReplaceClip : Qt::NoClip);
	painter->setClipping(clipWasEnabled);
}

void DashboardWindowContainer::paintHoriz3Tile(QPainter* painter, QPixmap* maskImg, int x, int y, int width, int height, int leftOffset, int rightOffset)
{
	if(!painter || !maskImg || (width <= 0) || (height <= 0))
		return;

	int leftWidth = MIN((int)((qreal)width / 2 + 0.5), leftOffset);
	int rightWidth = MIN((int)((qreal)width / 2 - 0.5), rightOffset);

	// Draw the mask as a horizontal 3-tile image
	painter->drawPixmap(x, y, leftWidth, height,
			            *maskImg, 0, 0, leftWidth, maskImg->height());

	if((width - leftWidth - rightWidth) > 0) {
		painter->drawPixmap(x + leftWidth, y, width - leftWidth - rightWidth, height,
							*maskImg, leftWidth, 0, maskImg->width() - leftWidth - rightWidth, maskImg->height());
	}

	painter->drawPixmap(x + width - rightWidth, y, rightWidth, height,
			            *maskImg, maskImg->width() - rightWidth, 0, rightWidth, maskImg->height());

}
