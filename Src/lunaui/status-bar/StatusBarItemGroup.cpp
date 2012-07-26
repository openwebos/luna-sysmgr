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



#include "StatusBarItemGroup.h"
#include "Settings.h"
#include "StatusBarItem.h"
#include <QPainter>
#include "AnimationSettings.h"
#include "stdio.h"

#define ITEM_SPACING     5
#define GROUP_RIGHT_PADDING    3
#define MENU_OVERLAY_OPACITY 1.0


StatusBarItemGroup::StatusBarItemGroup(int height, bool hasArrow, bool showSeparator, Alignment align)
	:  StatusBarItem(align)
	, m_height(height)
	, m_hasArrow(hasArrow)
	, m_arrowAnimProg(0.0)
	, m_active(false)
	, m_actionable(false)
	, m_mouseDown(false)
	, m_activeBkgPix(0)
//	, m_pressedBkgPix(0)
	, m_arrowPix(0)
	, m_separatorPix(0)
	, m_opacityAnimPtr(0)
	, m_overlayAnimPtr(0)
	, m_overlayOpacity(0.0)
	, m_menuObj(0)
{
	Settings* settings = Settings::LunaSettings();
	std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";

	if(m_hasArrow) {
		std::string filePath = statusBarImagesPath + "menu-arrow.png";
		m_arrowPix = new QPixmap(filePath.c_str());
	} else {
		m_arrowAnimProg = 1.0;
	}

	if(showSeparator && (align != StatusBarItem::AlignCenter)) {
		std::string filePath = statusBarImagesPath + "status-bar-separator.png";
		m_separatorPix = new QPixmap(filePath.c_str());
	}

	layout();
}

StatusBarItemGroup::~StatusBarItemGroup()
{
	if(m_arrowPix)
		delete m_arrowPix;

	if(m_separatorPix)
		delete m_separatorPix;

	if(m_activeBkgPix)
		delete m_activeBkgPix;

//	if(m_pressedBkgPix)
//		delete m_pressedBkgPix;
}

void StatusBarItemGroup::setHeight(int h)
{
	m_height = h;
	layout();
}

QRectF StatusBarItemGroup::boundingRect() const
{
	return m_bounds;
}

void StatusBarItemGroup::addItem(StatusBarItem* item)
{
	if(!item)
		return;

	item->setParentItem(this);

	m_items.append(item);

	connect(item,SIGNAL(signalBoundingRectChanged()), this, SLOT(slotChildBoundingRectChanged()));

	layout();
}

void StatusBarItemGroup::setActionable(bool actionable)
{
	if(m_actionable == actionable)
		return;

	m_actionable = actionable;

	if(Settings::LunaSettings()->tabletUi) {
		if(m_actionable && !m_activeBkgPix) {
			Settings* settings = Settings::LunaSettings();
			std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";

			std::string filePath = statusBarImagesPath + "status-bar-menu-dropdown-tab.png";
			m_activeBkgPix = new QPixmap(filePath.c_str());
		}

//		if(m_actionable && !m_pressedBkgPix) {
//			Settings* settings = Settings::LunaSettings();
//			std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";
//
//			std::string filePath = statusBarImagesPath + "status-bar-menu-dropdown-tab-pressed.png";
//			m_pressedBkgPix = new QPixmap(filePath.c_str());
//		}
	}

	// make sure the arrow is visible, but only when the item is accepting actions
	if(m_hasArrow) {
		if(!m_arrowFadeAnimPtr.isNull()) {
			m_arrowFadeAnimPtr->stop();
			delete m_arrowFadeAnimPtr;
		}

		if((m_actionable && (m_arrowAnimProg < 1.0)) || (!m_actionable && (m_arrowAnimProg > 0.0))) {
			m_arrowFadeAnimPtr = new QPropertyAnimation();
			m_arrowFadeAnimPtr->setPropertyName("arrowAnimProgress");
			m_arrowFadeAnimPtr->setEasingCurve(AS_CURVE(statusBarArrowSlideCurve));
			m_arrowFadeAnimPtr->setTargetObject(this);
			m_arrowFadeAnimPtr->setDuration(AS(statusBarArrowSlideDuration));

			if(m_actionable && (m_arrowAnimProg < 1.0)) {
				// animate arrow in
				m_arrowFadeAnimPtr->setEndValue(1.0);
			} else {
				// animate arrow out
				m_arrowFadeAnimPtr->setEndValue(0.0);
			}

			m_arrowFadeAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);
		}
	}
}

void StatusBarItemGroup::setArrowAnimProgress(qreal prog)
{
	m_arrowAnimProg = prog;
	layout();
}

void StatusBarItemGroup::setOverlayOpacity(qreal opacity)
{
	m_overlayOpacity = opacity;
	update();
}


void StatusBarItemGroup::actionTriggered()
{
	if(m_active)
		Q_EMIT signalActionTriggered(false);
	else
		Q_EMIT signalActionTriggered(true);
}

void StatusBarItemGroup::show()
{
	if(!m_opacityAnimPtr.isNull()) {
		m_opacityAnimPtr->stop();
		delete m_opacityAnimPtr;
	}

	setVisible(true);

	if(opacity() == 1.0)
		return;

	m_active = false;

	m_opacityAnimPtr = new QPropertyAnimation();
	m_opacityAnimPtr->setPropertyName("opacity");
	m_opacityAnimPtr->setEasingCurve(AS_CURVE(statusBarTabFadeCurve));
	m_opacityAnimPtr->setTargetObject(this);
	m_opacityAnimPtr->setDuration(AS(statusBarTabFadeDuration));
	m_opacityAnimPtr->setEndValue(1.0);
	connect(m_opacityAnimPtr, SIGNAL(finished()), SLOT(slotFadeAnimationFinished()));
	m_opacityAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);
}

void StatusBarItemGroup::hide()
{
	if(!m_opacityAnimPtr.isNull()) {
		m_opacityAnimPtr->stop();
		delete m_opacityAnimPtr;
	}

	if(!isVisible()) {
		setVisible(false);
		return;
	}

	m_opacityAnimPtr = new QPropertyAnimation();
	m_opacityAnimPtr->setPropertyName("opacity");
	m_opacityAnimPtr->setEasingCurve(AS_CURVE(statusBarTabFadeCurve));
	m_opacityAnimPtr->setTargetObject(this);
	m_opacityAnimPtr->setDuration(AS(statusBarTabFadeDuration));
	m_opacityAnimPtr->setEndValue(0.0);
	connect(m_opacityAnimPtr, SIGNAL(finished()), SLOT(slotFadeAnimationFinished()));
	m_opacityAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);
}

void StatusBarItemGroup::slotFadeAnimationFinished()
{
	if(opacity() == 0.0) {
		setVisible(false);
		m_active = false;
	}
}

void StatusBarItemGroup::setMenuObject(QGraphicsObject* item)
{
	m_menuObj = item;
}

void StatusBarItemGroup::activate()
{
	m_active = true;

	if(!m_overlayAnimPtr.isNull()) {
		m_overlayAnimPtr->stop();
		delete m_overlayAnimPtr;
	}

	m_overlayAnimPtr = new QPropertyAnimation();
	m_overlayAnimPtr->setPropertyName("overlayOpacity");
	m_overlayAnimPtr->setEasingCurve(AS_CURVE(statusBarMenuFadeCurve));
	m_overlayAnimPtr->setTargetObject(this);
	m_overlayAnimPtr->setDuration(AS(statusBarMenuFadeDuration));

	m_overlayAnimPtr->setEndValue(MENU_OVERLAY_OPACITY);

	connect(m_overlayAnimPtr, SIGNAL(valueChanged(const QVariant&)), this,
			SLOT(slotOverlayAnimValueChanged(const QVariant&)));

	if(m_menuObj)
		m_menuObj->setVisible(true);

	if(m_menuObj)
		m_menuObj->setOpacity(0.0);

	m_overlayAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);

	update();

	Q_EMIT signalActivated(this);
}

void StatusBarItemGroup::deactivate()
{
	m_active = false;

	if(!m_overlayAnimPtr.isNull()) {
		m_overlayAnimPtr->stop();
		delete m_overlayAnimPtr;
	}

	m_overlayAnimPtr = new QPropertyAnimation();
	m_overlayAnimPtr->setPropertyName("overlayOpacity");
	m_overlayAnimPtr->setEasingCurve(AS_CURVE(statusBarMenuFadeCurve));
	m_overlayAnimPtr->setTargetObject(this);
	m_overlayAnimPtr->setDuration(AS(statusBarMenuFadeDuration));

	m_overlayAnimPtr->setEndValue(0.0);

	connect(m_overlayAnimPtr, SIGNAL(valueChanged(const QVariant&)), this,
			SLOT(slotOverlayAnimValueChanged(const QVariant&)));
	connect(m_overlayAnimPtr, SIGNAL(finished()), SLOT(slotOverlayAnimationFinished()));

	m_overlayAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);

	update();
}

void StatusBarItemGroup::slotOverlayAnimValueChanged(const QVariant& value)
{
	qreal opacity = value.toReal();
	if(m_menuObj)
		m_menuObj->setOpacity(opacity);
}

void StatusBarItemGroup::slotOverlayAnimationFinished()
{
	if(m_menuObj)
		m_menuObj->setVisible(false);
}

void StatusBarItemGroup::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if(!m_actionable || !isVisible()) {
		event->ignore();
	} else {
		event->accept();
		m_mouseDown = true;
//		update();
	}
}

void StatusBarItemGroup::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{

}

void StatusBarItemGroup::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{

}

void StatusBarItemGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if(m_actionable && m_mouseDown) {
		actionTriggered();
	}
	m_mouseDown = false;
//	update();
}


int StatusBarItemGroup::separatorWidth()
{
	if(m_separatorPix)
		return m_separatorPix->width();
	else
		return 0;
}

void StatusBarItemGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	qreal opacity = painter->opacity();
	QRectF tabRect;

	static const int margin = 11;

	if(m_alignment == AlignRight && m_separatorPix) {
		tabRect = m_bounds.adjusted(m_separatorPix->width() - margin, 0, margin, 0);
	} else if(m_alignment == AlignLeft && m_separatorPix) {
		tabRect = m_bounds.adjusted(-margin, 0, margin - m_separatorPix->width(), 0);
	} else {
		tabRect = m_bounds.adjusted(margin, 0, margin, 0);
	}

	if(m_activeBkgPix && (m_overlayOpacity > 0.0)) {
		painter->setOpacity(opacity * m_overlayOpacity);

		painter->drawPixmap(tabRect.x(), tabRect.y(), margin, tabRect.height(),
							*m_activeBkgPix, 0, 0, margin, m_activeBkgPix->height());
		painter->drawPixmap(tabRect.x() + margin, tabRect.y(), tabRect.width() - 2*margin, tabRect.height(),
							*m_activeBkgPix, margin, 0, m_activeBkgPix->width() - 2*margin, m_activeBkgPix->height());
		painter->drawPixmap(tabRect.x() + tabRect.width() - margin, tabRect.y(), margin, tabRect.height(),
							*m_activeBkgPix, m_activeBkgPix->width() - margin, 0, margin, m_activeBkgPix->height());
	}

//	if(m_mouseDown && m_pressedBkgPix) {
//		painter->drawPixmap(tabRect.x(), tabRect.y(), margin, tabRect.height(),
//							*m_pressedBkgPix, 0, 0, margin, m_pressedBkgPix->height());
//		painter->drawPixmap(tabRect.x() + margin, tabRect.y(), tabRect.width() - 2*margin, tabRect.height(),
//							*m_pressedBkgPix, margin, 0, m_pressedBkgPix->width() - 2*margin, m_pressedBkgPix->height());
//		painter->drawPixmap(tabRect.x() + tabRect.width() - margin, tabRect.y(), margin, tabRect.height(),
//							*m_pressedBkgPix, m_pressedBkgPix->width() - margin, 0, margin, m_pressedBkgPix->height());
//	}

	if(m_hasArrow && m_arrowPix && !m_arrowPix->isNull() && (m_arrowAnimProg > 0.0)) {
		painter->setOpacity(m_arrowAnimProg * opacity);
		if(m_alignment == AlignRight) {
			painter->drawPixmap(-m_arrowPix->width() - ARROW_SPACING, -m_arrowPix->height()/2, *m_arrowPix);
		} else if(m_alignment == AlignLeft) {
			int separator = 0;
			if(m_separatorPix && !m_separatorPix->isNull()) {
				separator = m_separatorPix->width();
			}
			painter->drawPixmap(m_bounds.width() - m_arrowPix->width() - separator - ARROW_SPACING, -m_arrowPix->height()/2, *m_arrowPix);
		} else {
			painter->drawPixmap(m_bounds.width()/2 - m_arrowPix->width() - ARROW_SPACING, -m_arrowPix->height()/2, *m_arrowPix);
		}
		painter->setOpacity(opacity);
	}

	if((m_alignment != StatusBarItem::AlignCenter) && m_separatorPix && !m_separatorPix->isNull() && (m_arrowAnimProg > 0.0)) {
		painter->setOpacity(m_arrowAnimProg * opacity * (1 - m_overlayOpacity));
		if(m_alignment == AlignRight) {
			// separator on the LEFT
			painter->drawPixmap(-m_bounds.width(), -m_separatorPix->height()/2, *m_separatorPix);
		} else if(m_alignment == AlignLeft) {
			// separator on the RIGHT
			painter->drawPixmap(m_bounds.width() - m_separatorPix->width(), -m_separatorPix->height()/2, *m_separatorPix);
		}
		painter->setOpacity(opacity);
	}
}

void StatusBarItemGroup::layout()
{
	if(m_alignment == AlignRight) {
		layoutRight();
	} else if(m_alignment == AlignLeft) {
		layoutLeft();
	} else {
		layoutCenter();
	}
}

void StatusBarItemGroup::layoutCenter()
{
	int width=0;
	int currRight;
	QRect rect;

	if(m_hasArrow && m_arrowPix && !m_arrowPix->isNull() && (m_arrowAnimProg > 0.0)) {
		width = m_arrowPix->width() + 2.0 * ARROW_SPACING;
	}

	for(int x = 0; x < m_items.size(); x++) {
		QGraphicsObject* item = m_items.at(x);
		if(item) {
			width += item->boundingRect().width() + ITEM_SPACING;
		}
	}

	rect = QRect(-width/2, -m_height/2, width, m_height);

	currRight = width/2;

	if(m_hasArrow && m_arrowPix && !m_arrowPix->isNull() && (m_arrowAnimProg > 0.0)) {
		currRight -=m_arrowPix->width() + 2.0 * ARROW_SPACING;
	}

	for(int x = 0; x < m_items.size(); x++) {
		StatusBarItem* item = m_items.at(x);

		if(item) {
			StatusBarItem::Alignment align = item->alignment();

			if(align == AlignRight) {
				item->setPos(currRight, 0);
			} else if(align == AlignLeft) {
				item->setPos(currRight - item->boundingRect().width(), 0);
			} else {
				item->setPos(currRight - item->boundingRect().width()/2, 0);
			}

			currRight -= item->boundingRect().width() + ITEM_SPACING;
		}
	}

	if(rect != m_bounds) {
		prepareGeometryChange();
		m_bounds = rect;
		Q_EMIT signalBoundingRectChanged();
	}

	update();
}

void StatusBarItemGroup::layoutRight()
{
	int width=0;
	int currRight;
	QRect rect;

	if(m_hasArrow && m_arrowPix && !m_arrowPix->isNull() && (m_arrowAnimProg > 0.0)) {
		width = m_arrowPix->width() + 2.0 * ARROW_SPACING;
	}
	else {
		width = GROUP_RIGHT_PADDING;
	}

	if(m_separatorPix && !m_separatorPix->isNull()) {
		width += m_separatorPix->width(); // + ITEM_SPACING;
	}

	for(int x = 0; x < m_items.size(); x++) {
		StatusBarItem* item = m_items.at(x);
		if(item) {
			if(x > 0)
				width += ITEM_SPACING;
			width += item->boundingRect().width();
		}
	}

	rect = QRect(-width, -m_height/2, width, m_height);

	currRight = 0;
	if (!m_hasArrow)
		currRight = - GROUP_RIGHT_PADDING;

	if(m_hasArrow && m_arrowPix && !m_arrowPix->isNull() && (m_arrowAnimProg > 0.0)) {
		currRight -= m_arrowPix->width() + 2.0 * ARROW_SPACING;
	}

	for(int x = 0; x < m_items.size(); x++) {
		StatusBarItem* item = m_items.at(x);

		if(item) {
			StatusBarItem::Alignment align = item->alignment();

			if(x > 0)
				currRight -= ITEM_SPACING;

			if(align == AlignRight) {
				item->setPos(currRight, 0);
			} else if(align == AlignLeft) {
				item->setPos(currRight - item->boundingRect().width(), 0);
			} else {
				item->setPos(currRight - item->boundingRect().width()/2, 0);
			}

			currRight -= item->boundingRect().width();
		}
	}

	if(rect != m_bounds) {
		prepareGeometryChange();
		m_bounds = rect;
		Q_EMIT signalBoundingRectChanged();
	}

	update();
}

void StatusBarItemGroup::layoutLeft()
{
	int width=0;
	int currLeft;
	QRect rect;

	if(m_hasArrow && m_arrowPix && !m_arrowPix->isNull() && (m_arrowAnimProg > 0.0)) {
		width = m_arrowPix->width() + ARROW_SPACING;
	}

	if(m_separatorPix && !m_separatorPix->isNull()) {
		width += m_separatorPix->width(); // + ITEM_SPACING;
	}

	for(int x = 0; x < m_items.size(); x++) {
		StatusBarItem* item = m_items.at(x);
		if(item) {
			width += item->boundingRect().width() + ITEM_SPACING;
		}
	}

	rect = QRect(0, -m_height/2, width, m_height);

	currLeft = 0;

	for(int x = 0; x < m_items.size(); x++) {
		StatusBarItem* item = m_items.at(x);

		if(item) {
			StatusBarItem::Alignment align = item->alignment();

			if(x > 0)
				currLeft += ITEM_SPACING;

			if(align == AlignLeft) {
				item->setPos(currLeft, 0);
			} else if(align == AlignRight) {
				item->setPos(currLeft + item->boundingRect().width(), 0);
			} else {
				item->setPos(currLeft + item->boundingRect().width()/2, 0);
			}

			currLeft += item->boundingRect().width();
		}
	}

	if(rect != m_bounds) {
		prepareGeometryChange();
		m_bounds = rect;
		Q_EMIT signalBoundingRectChanged();
	}

	update();
}

void StatusBarItemGroup::slotChildBoundingRectChanged()
{
	layout();
}

