/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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

#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string>

#include <lunaservice.h>
#include <cjson/json.h>

#include "HostBase.h"
#include "Settings.h"
#include "Window.h"
#include "SystemUiController.h"
#include "AnimationSettings.h"
#include "DockModeWindowManager.h"
#include "DockModeLaunchPoint.h"
#include "DockModeAppMenuContainer.h"
#include "WindowServerLuna.h"
#include "Localization.h"
#include "ApplicationDescription.h"
#include "LaunchPoint.h"
#include "Preferences.h"
#include "Utils.h"
#include "StatusBar.h"
#include "QtUtils.h"

#include <QPixmap>
#include <QFont>
#include <QGesture>
#include <QGestureEvent>

#define MAX_ITEMS_VISIBLE (5.5)
#define ITEM_HEIGHT (70)


static int kMenuItemHeight = 70;

// ------------------------- Class Declarations -----------------------------------

class DockModeMenuItem : public QGraphicsObject 
{
public:
	DockModeMenuItem (DockModeLaunchPoint* dlp, DockModeAppMenuContainer* parent);
	~DockModeMenuItem();

	QRectF boundingRect() const;
	void setPressed (bool pressed) { m_pressed = pressed; update(); }
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	DockModeLaunchPoint*  launchPoint() const { return m_dockLp; }

	void resize (int screenWidth, int screenHeight);
	bool sceneEvent (QEvent* event);
	QString title() const { return m_title; }

	void setAsLast (bool last);
private:
	DockModeAppMenuContainer* m_parent;
	DockModeLaunchPoint* m_dockLp;
	QPixmap		m_icon;
	QString		m_title;
	QRectF		m_bounds;
	int		m_iconSize;
	int		m_padding;
	bool		m_pressed;
	bool		m_isLast;

	int 		kSeparatorHeight;
};

// ------------------------- DockModeMenuItem -------------------------------------

DockModeMenuItem::DockModeMenuItem (DockModeLaunchPoint* dlp, DockModeAppMenuContainer* parent)
	: m_dockLp (dlp), m_parent (parent), m_padding (10), m_pressed (false), m_iconSize (48), m_isLast (true)
{
	m_icon = QPixmap (m_dockLp->launchPoint()->iconPath().c_str());
	if (m_icon.isNull())
		g_warning ("DockModeMenuItem: unable to load icon at %s", m_dockLp->launchPoint()->iconPath().c_str());

	m_title = fromStdUtf8(m_dockLp->launchPoint()->appDesc()->dockModeTitle());

	m_bounds = QRectF (0, 0, parent->boundingRect().width(), ITEM_HEIGHT);

	kSeparatorHeight = 0;

	grabGesture (Qt::TapGesture);
}

DockModeMenuItem::~DockModeMenuItem()
{
	ungrabGesture (Qt::TapGesture);
}

QRectF DockModeMenuItem::boundingRect() const
{
	return m_bounds;
}

void DockModeMenuItem::setAsLast (bool isLast)
{
	if (m_isLast != isLast) {
		m_isLast = isLast;
		if (isLast) {
			m_bounds.setHeight (m_bounds.height()-kSeparatorHeight);
			kSeparatorHeight = 0;
		}
		else  {
			kSeparatorHeight = m_parent->m_menuItemSeparator.height();
			m_bounds.setHeight (m_bounds.height() + kSeparatorHeight);
		}
		update(); 
	}

}

void DockModeMenuItem::resize (int displayWidth, int displayHeight)
{
	m_bounds.setWidth (displayWidth);
	prepareGeometryChange();
	update();
}

bool DockModeMenuItem::sceneEvent (QEvent* event) 
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g) {
			event->accept();
			return true;
		}
	}

	if (event->type() == QEvent::Gesture) {
		QGestureEvent* gevt = static_cast<QGestureEvent*>(event);
		if (gevt) {
			QTapGesture* tap = (QTapGesture*) gevt->gesture (Qt::TapGesture);
			if (tap && tap->state() == Qt::GestureFinished) {
				QPointF position = gevt->mapToGraphicsScene(tap->hotSpot());
				m_parent->appSelected (m_dockLp);
				gevt->accept();
				return true;
			}
		}
	}
	return QGraphicsObject::sceneEvent(event);
}

void DockModeMenuItem::paint (QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if (m_pressed) {
		painter->drawPixmap (QRect (0,0, m_bounds.width(), m_bounds.height()), m_parent->m_menuItemBgSelected);
	}

	painter->drawPixmap (QRect (m_padding, (int)m_bounds.height()/2 - m_iconSize/2 - kSeparatorHeight/2, m_iconSize, m_iconSize), m_icon);

	QFont oldFont = painter->font();
	QPen oldPen = painter->pen();

	QFont newFont = m_parent->font();
	newFont.setPixelSize (18);

	painter->setFont (newFont);
	painter->setPen (Qt::white);
	painter->drawText (QRectF (m_padding + m_iconSize + m_padding, 0,
				m_bounds.width() - (m_padding + m_icon.width() + m_padding), 
				m_bounds.height() - kSeparatorHeight),
			Qt::AlignLeft | Qt::AlignVCenter, m_title);
	painter->setFont (oldFont);
	painter->setPen (oldPen);
	if (!m_isLast && !m_parent->m_menuItemSeparator.isNull()) {
		painter->drawPixmap (QRect (m_bounds.left(), m_bounds.height() - kSeparatorHeight, m_bounds.width(), kSeparatorHeight), m_parent->m_menuItemSeparator);
	}
}

// ------------------------- DockModeAppMenuContainer -------------------------------------
//

DockModeAppMenuContainer::DockModeAppMenuContainer (DockModeMenuManager* parent, int width, int height)
	: m_menuHeight (0.0), m_penDown (false), m_currentItemIndex (-1), m_maxHeight (0)
{
	setFlag (QGraphicsItem::ItemClipsToShape);
	setFlag (QGraphicsItem::ItemClipsChildrenToShape);
	setFlag (QGraphicsItem::ItemHasNoContents);

	m_bounds = QRectF (0, 0, width, 0);

	std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-selection-gradient-default.png";
	m_menuItemBgSelected = QPixmap (filePath.c_str());
	if (m_menuItemBgSelected.isNull())
		g_warning ("Unable to load viewport highlighted background pixmap at %s", filePath.c_str());

	filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/menu-divider.png";
	m_menuItemSeparator = QPixmap (filePath.c_str());
	if (m_menuItemSeparator.isNull())
		g_warning ("Unable to load menu divider pixmap at %s", filePath.c_str());


	const char* fontName = Settings::LunaSettings()->fontDockMode.c_str();
	m_font = QFont (fontName, 21);
	m_font.setBold(true);

	m_maxHeight = MAX_ITEMS_VISIBLE * kMenuItemHeight;
}

DockModeAppMenuContainer::~DockModeAppMenuContainer()
{
	int i = 0;
	for (i = 0; i < m_menuItems.size(); ++i)
		delete m_menuItems[i];
	m_menuItems.clear();

}

QRectF DockModeAppMenuContainer::boundingRect() const 
{ 
	return m_bounds; 
}

void DockModeAppMenuContainer::resize (int screenWidth, int screenHeight)
{
	m_bounds.setWidth (screenWidth);

	int i = 0;
	for (i = 0; i < m_menuItems.size(); ++i)
		m_menuItems[i]->resize (screenWidth, screenHeight);

	prepareGeometryChange();
	update();
}


void DockModeAppMenuContainer::addMenuItem (DockModeLaunchPoint* dlp)
{
	DockModeMenuItem* menuItem = new DockModeMenuItem (dlp, this);
	menuItem->setPos (0, m_menuHeight);
	menuItem->setParentItem (this);
	m_menuHeight += menuItem->boundingRect().height();
	m_bounds.setHeight (m_menuHeight);
	if (!m_menuItems.empty())
		m_menuItems.last()->setAsLast(false);

	m_menuItems.append (menuItem);
	m_menuItemMap[dlp] = menuItem;
	if (!m_menuItems.empty())
		m_menuItems.last()->setAsLast(true);

	Q_EMIT signalContainerSizeChanged();
	prepareGeometryChange();
	update();

}

void DockModeAppMenuContainer::mousePressEvent (QGraphicsSceneMouseEvent* mouseEvent)
{
	int i = 0;
	for (i = 0; i < m_menuItems.size(); ++i) {
		QRectF itemBoundingRect = m_menuItems[i]->boundingRect().translated (m_menuItems[i]->pos());
		if (itemBoundingRect.contains (mouseEvent->pos())) {
			m_menuItems[i]->setPressed(true);
			m_currentItemIndex = i;
			break;
		}
	}
	if (i == m_menuItems.size()) {
		g_warning ("%s: No item contains this point (%f, %f)!!!", __PRETTY_FUNCTION__, pos().x(), pos().y());
	}

	m_penDown = true;
	m_penDownPos = mouseEvent->pos();
	mouseEvent->accept();
	update();
}

void DockModeAppMenuContainer::mouseMoveEvent (QGraphicsSceneMouseEvent* mouseEvent)
{
}

void DockModeAppMenuContainer::mouseReleaseEvent (QGraphicsSceneMouseEvent* mouseEvent)
{
	if (m_currentItemIndex != -1)
		m_menuItems[m_currentItemIndex]->setPressed(false);
	m_currentItemIndex = -1;
	m_penDown = false;

	mouseEvent->accept();
	update();
}

void DockModeAppMenuContainer::mouseWasGrabbedByParent()
{
	if (m_currentItemIndex != -1)
		m_menuItems[m_currentItemIndex]->setPressed(false);
	m_currentItemIndex = -1;
	m_penDown = false;
	update();
}

void DockModeAppMenuContainer::removeMenuItem (DockModeLaunchPoint* dlp)
{
	bool adjustPos = false;
	int adjustHeight = 0;
	DockModeMenuItem* removedItem = NULL;
	for (int i = 0; i < m_menuItems.size(); ++i) {
		if (m_menuItems[i]->launchPoint() == dlp) {
			removedItem = m_menuItems[i];
			adjustHeight = m_menuItems[i]->boundingRect().height();
			adjustPos = true;
			continue;
		}
		if (adjustPos) {
			m_menuItems[i]->moveBy (0, -adjustHeight);
			// move each following entry up by the height of the deleted item
		}
	}
	m_menuItems.remove(removedItem);
	m_menuItemMap.erase(dlp);

	if (!m_menuItems.empty())
		m_menuItems.last()->setAsLast(true);

	// recalculate memu height after removal of entry since item heights are variable
	// depending on whether they are the last item or not (inclusion / exclusion of separator)
	m_menuHeight = 0;
	for (int i = 0; i < m_menuItems.size(); ++i) {
		m_menuHeight += m_menuItems[i]->boundingRect().height();
	}

	m_bounds.setHeight (m_menuHeight);
	delete removedItem;

	Q_EMIT signalContainerSizeChanged();
	prepareGeometryChange();
	update();

}

void DockModeAppMenuContainer::appSelected (DockModeLaunchPoint* dlp)
{
	Q_EMIT signalDockModeAppSelected (dlp);
}

