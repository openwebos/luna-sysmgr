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




#include "StatusBarNotificationArea.h"
#include "Settings.h"
#include "Preferences.h"
#include "DashboardWindowManager.h"
#include "DashboardWindowContainer.h"
#include "DashboardWindow.h"

#include <QPainter>

#define FADED_ICONS_OPACITY 0.0

StatusBarNotificationIcon::StatusBarNotificationIcon(StatusBarIconContainer* parent, DashboardWindow* ownerWindow)
	: StatusBarIcon(parent)
	, m_ownerWindow(ownerWindow)
{
	if(m_ownerWindow) {
		if(!m_ownerWindow->icon().isNull()) {
			m_iconImg = m_ownerWindow->icon();
			setImage(&m_iconImg);
		}
	}
}

StatusBarNotificationIcon::~StatusBarNotificationIcon()
{

}

void StatusBarNotificationIcon::setVisible(bool visible)
{
	m_visible = visible;
	if(!visible) {
		// notifications get deleted when they are done animating away and set to not visible
		m_parent->iconRemoved(this);
	}
}

// =============================================================================


StatusBarNotificationArea::StatusBarNotificationArea()
	:  StatusBarItem(AlignRight, true) // Right Aligned item
	, BannerMessageView(BannerMessageView::StatusBarScroll)
	, m_dwm(0)
	, m_maxWidth(320)
	, m_maxHeight(28)
	, m_croppedWidth(0)
	, m_bmHandler(BannerMessageHandler::instance())
	, m_bannerMsgCount(0)
	, m_iconsOpacity(1.0)
	, m_arrowPix(0)
	, m_showArrow(true)
{
}

StatusBarNotificationArea::~StatusBarNotificationArea()
{
	if(m_arrowPix)
		delete m_arrowPix;
}

void StatusBarNotificationArea::init(DashboardWindowManager* dwm)
{
	m_bounds = QRect(-40, -14, 40, 28);

	Settings* settings = Settings::LunaSettings();
	std::string filePath = settings->lunaSystemResourcesPath + "/statusBar/menu-arrow.png";
	m_arrowPix = new QPixmap(filePath.c_str());

	if(m_arrowPix && !m_arrowPix->isNull()) {
		m_maxWidth += m_arrowPix->width() + 2 * ARROW_SPACING;
		updateBoundingRect();
	}

	m_dwm = dwm;
	if(m_dwm) {
		connect(m_dwm->dashboardWindowContainer(), SIGNAL(signalWindowAdded(DashboardWindow*)), this,
				SLOT(slotDashboardWindowAdded(DashboardWindow*)));
		connect(m_dwm->dashboardWindowContainer(), SIGNAL(signalWindowsRemoved(DashboardWindow*)), this,
				SLOT(slotDashboardWindowsRemoved(DashboardWindow*)));

	}
}


QRectF StatusBarNotificationArea::boundingRect() const
{
	return m_bounds;
}

void StatusBarNotificationArea::iconRemoved(StatusBarIcon* icon)
{
	if(icon) {
		for(int x = 0; x < m_icons.size(); x++) {
			if(m_icons.at(x) == icon) {
				StatusBarNotificationIcon* notif = m_icons[x];
				m_icons.remove(x);
				notif->deleteLater();
				break;
			}
		}

		if((m_icons.size() == 0) && (m_bannerMsgCount)) {
			m_showArrow = false;
		}

		updateBoundingRect();

		if((m_icons.size() == 0) && (m_bannerMsgCount == 0)) {
			Q_EMIT signalNotificationArealVisibilityChanged(false);
		}
	}
}

void StatusBarNotificationArea::setMaxWidth(int maxWidth)
{
	m_maxWidth = maxWidth;

	if(m_arrowPix && !m_arrowPix->isNull()) {
		m_maxWidth += m_arrowPix->width() + 2 * ARROW_SPACING;
	}

	updateBoundingRect();
}

void StatusBarNotificationArea::setMaxHeight(int maxHeight)
{
	m_maxHeight = maxHeight;
	update();
}

// This item is Right Aligned (The position  of the icon is the position of the RIGHT EDGE of the bounding rect)
void StatusBarNotificationArea::updateBoundingRect(bool forceRepaint)
{
	QRectF rect, bannerRect;
	int cropped = 0;

	int width=0, height=0;

	if(m_showArrow && m_arrowPix && !m_arrowPix->isNull()) {
		width += m_arrowPix->width() + 2 * ARROW_SPACING;
	}

	for(int x = 0; x < m_icons.size(); x++) {
		if(m_icons.at(x)->isVisible()) {
			width += m_icons.at(x)->boundingRect().width() + (ICON_SPACING * m_icons.at(x)->visiblePortion());
			if(height < m_icons.at(x)->boundingRect().height())
				height = m_icons.at(x)->boundingRect().height();
		}
	}
	if((m_maxWidth >= 0) && (width > m_maxWidth)) {
		cropped = width - m_maxWidth;
		width = m_maxWidth;
	}
	else {
		cropped = 0;
	}

	rect = QRect(-width, -height/2, width, height);

	if(m_bannerMsgCount) {
		int bannerWidth = m_bmHandler->getViewContentWidth(this);
		int bannerHeight = m_bmHandler->viewHeight();

		if (bannerWidth > m_maxWidth)
			bannerWidth = m_maxWidth;

		bannerRect = QRect(-bannerWidth, -bannerHeight/2, bannerWidth, bannerHeight);
		rect = rect.united(bannerRect);
	}

	if(rect != m_bounds) {
		prepareGeometryChange();
		m_bounds = rect;
		Q_EMIT signalBoundingRectChanged();
	} else if(cropped != m_croppedWidth) {
		m_croppedWidth = cropped;
		update();
	} else if(forceRepaint) {
		update();
	}
}

void StatusBarNotificationArea::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPoint centerRight;
	qreal opacity = painter->opacity();
	int rightEdge = 0;

	if(m_croppedWidth <= 0) {
		centerRight = QPoint(0, 0);
	} else {
		centerRight = QPoint(m_croppedWidth, 0);
	}

	if(m_iconsOpacity < 1.0)
		painter->setOpacity(m_iconsOpacity * opacity);

	if(m_arrowPix && !m_arrowPix->isNull()) {
		painter->drawPixmap(-m_arrowPix->width() - ARROW_SPACING, -m_arrowPix->height()/2, *m_arrowPix);
		rightEdge = -(m_arrowPix->width() + 2 * ARROW_SPACING);
		centerRight.setX(centerRight.x() + rightEdge);
	}

	for(int x = 0; x < m_icons.size(); x++) {
		if(m_icons.at(x)->isVisible()) {
			if(centerRight.x() > rightEdge) {
				if((centerRight.x() - m_icons.at(x)->boundingRect().width()) < rightEdge) // paint partial
					m_icons.at(x)->paint(painter, centerRight, m_icons.at(x)->boundingRect().width() - (centerRight.x() - rightEdge));
			} else {
				m_icons.at(x)->paint(painter, centerRight);
			}
			centerRight.setX(centerRight.x() - (m_icons.at(x)->boundingRect().width() + (ICON_SPACING * m_icons.at(x)->visiblePortion())));
		}
	}

	if(m_bannerMsgCount) {
		int tx = 0;
		int ty = m_bounds.y();
		// must translate the painter to have (0,0) as the top right corner for the Banner Handler drawing to work correctly
		painter->translate(tx, ty);
		m_bmHandler->drawView(painter, this);
		painter->translate(-tx, -ty);
	}

	painter->setOpacity(opacity);
}

void StatusBarNotificationArea::slotDashboardWindowAdded(DashboardWindow* w)
{
	if(w) {
		bool hidden = false;
		m_showArrow = true;

		if((m_icons.size() == 0) && (m_bannerMsgCount == 0))
			hidden = true;

		StatusBarNotificationIcon* notif = new StatusBarNotificationIcon(this, w);
		m_icons.append(notif);
		notif->show();

		if(hidden) {
			m_iconsOpacity = 1.0;
			Q_EMIT signalNotificationArealVisibilityChanged(true);
		}
	}
}

void StatusBarNotificationArea::slotDashboardWindowsRemoved(DashboardWindow* w)
{
	for(int x = 0; x < m_icons.size(); x++) {
		if(m_icons.at(x)->ownerWindow() == w) {
			m_icons[x]->hide();
			break;
		}
	}
}

bool StatusBarNotificationArea::handleBannerMsgTap()
{
	if(m_bannerMsgCount && m_bmHandler) {
		m_bmHandler->activateCurrentMessage();
		return true;
	}

	return false;
}

int  StatusBarNotificationArea::bmViewGetWidth() const
{
	return MAX(m_maxWidth, m_bounds.width());
}

int  StatusBarNotificationArea::bmIsViewFullyExpanded() const
{
	return (m_maxWidth == m_bounds.width());
}

void StatusBarNotificationArea::bmViewUpdated()
{
	if(m_bannerMsgCount)
		updateBoundingRect();
	update();
}

void StatusBarNotificationArea::bmViewMessageCountUpdated(int count)
{
	bool hidden = false;

	if((m_icons.size() == 0) && (m_bannerMsgCount == 0))
		hidden = true;

	if((m_bannerMsgCount == 0) && (count != 0)) {
		Q_EMIT signalBannerMessageActivated();
	}

	m_bannerMsgCount = count;
	updateBoundingRect();

	if((m_bannerMsgCount != 0) && (m_iconsOpacity > FADED_ICONS_OPACITY)) {
		if(hidden) {
			m_iconsOpacity = FADED_ICONS_OPACITY;
		} else {
			setIconsShown(false);
		}
	} else if((m_bannerMsgCount == 0) && (m_iconsOpacity < 1.0)) {
		if(hidden) {
			m_iconsOpacity = 1.0;
		} else if(m_icons.size() > 0) {
			setIconsShown(true);
		}
	}

	if(m_bannerMsgCount && hidden) {
		m_showArrow = false;
		Q_EMIT signalNotificationArealVisibilityChanged(true);
	} else if(m_bannerMsgCount == 0) {
		if(m_icons.size() == 0) {
			Q_EMIT signalNotificationArealVisibilityChanged(false);
		}
	}

	update();
}

void StatusBarNotificationArea::bmViewShowingNonSuppressibleMessage(bool showing)
{

}


void StatusBarNotificationArea::bmViewAddActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time)
{

}

void StatusBarNotificationArea::bmViewRemoveActiveCallBanner()
{

}

void StatusBarNotificationArea::bmViewUpdateActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time)
{

}

void StatusBarNotificationArea::setIconsShown(bool shown)
{
	if(shown && (m_iconsOpacity == 1.0) && !m_iconsAnimPtr.isNull())
		return;

	if(!shown && (m_iconsOpacity == FADED_ICONS_OPACITY) && !m_iconsAnimPtr.isNull())
		return;

	if(!m_iconsAnimPtr.isNull()) {
		m_iconsAnimPtr->stop();
		m_iconsAnimPtr = NULL;
	}


	if(shown) {
		m_iconsAnimPtr = new tIconsFadeAnim(this, &StatusBarNotificationArea::animValueChanged);
		m_iconsAnimPtr->setEasingCurve(QEasingCurve::Linear);
		m_iconsAnimPtr->setDuration(300);
		m_iconsAnimPtr->setStartValue((int)(m_iconsOpacity * 1000));
		m_iconsAnimPtr->setEndValue(1000);

		m_iconsAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);
	} else {
		m_iconsAnimPtr = new tIconsFadeAnim(this, &StatusBarNotificationArea::animValueChanged);
		m_iconsAnimPtr->setEasingCurve(QEasingCurve::Linear);
		m_iconsAnimPtr->setDuration(300);
		m_iconsAnimPtr->setStartValue((int)(m_iconsOpacity * 1000));
		m_iconsAnimPtr->setEndValue((int)(FADED_ICONS_OPACITY * 1000));

		m_iconsAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);
	}
}

void StatusBarNotificationArea::animValueChanged(const QVariant& value)
{
	m_iconsOpacity = (qreal)(((qreal)(value.toInt())) / 1000.0);
	update();
}

