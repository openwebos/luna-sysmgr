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




#ifndef STATUSBARNOTIFICATIONAREA_H
#define STATUSBARNOTIFICATIONAREA_H

#include "StatusBar.h"
#include "StatusBarItem.h"
#include "StatusBarIcon.h"
#include "BannerMessageHandler.h"
#include "VariantAnimation.h"

#include <QGraphicsObject>
#include <QPixmap>

class DashboardWindowManager;
class DashboardWindow;


class StatusBarNotificationIcon : public StatusBarIcon
{
	Q_OBJECT

public:
	StatusBarNotificationIcon(StatusBarIconContainer* parent, DashboardWindow* ownerWindow);
	~StatusBarNotificationIcon();

	DashboardWindow* ownerWindow() const { return m_ownerWindow; }

private:
	void setVisible(bool visible);

	QPixmap          m_iconImg;
	DashboardWindow* m_ownerWindow;
};



class StatusBarNotificationArea : public StatusBarItem
                                , public StatusBarIconContainer
                                , public BannerMessageView
{
	Q_OBJECT

public:
	StatusBarNotificationArea();
	~StatusBarNotificationArea();

	int width() const { return m_bounds.width(); }
	int height() const { return m_bounds.height(); }

	QRectF boundingRect() const; // This item is Right Aligned (The position  of the icon is the position of the RIGHT EDGE of the bounding rect)

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void init(DashboardWindowManager* dwm);

	void updateBoundingRect(bool forceRepaint=false);
	void iconRemoved(StatusBarIcon* icon);

	void setMaxWidth(int maxWidth);
	void setMaxHeight(int maxHeight);

	int  maxWidth()  { return m_maxWidth; }
	int  maxHeight() { return m_maxHeight; }

	bool handleBannerMsgTap();

	void registerBannerView() { m_bmHandler->registerView(this); }
	void unregisterBannerView() { m_bmHandler->unregisterView (this); }

	int  bmViewGetWidth() const;
	int  bmIsViewFullyExpanded() const;
	void bmViewUpdated();
	void bmViewMessageCountUpdated(int count);
	void bmViewShowingNonSuppressibleMessage(bool showing);

	void bmViewAddActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time);
	void bmViewRemoveActiveCallBanner();
	void bmViewUpdateActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time);


Q_SIGNALS:

	void signalNotificationArealVisibilityChanged(bool visible);
	void signalBannerMessageActivated();

private Q_SLOTS:
	void slotDashboardWindowAdded(DashboardWindow* w);
	void slotDashboardWindowsRemoved(DashboardWindow* w);

private:

	void setIconsShown(bool shown);
	void animValueChanged(const QVariant& value);
	int m_maxWidth;
	int m_maxHeight;
	int m_croppedWidth;
	DashboardWindowManager* m_dwm;
	QVector<StatusBarNotificationIcon*>  m_icons; // items order from right (0) to left

	QPixmap* m_arrowPix;
	bool m_showArrow;

	BannerMessageHandler* m_bmHandler;
	int m_bannerMsgCount;
	typedef VariantAnimation<StatusBarNotificationArea> tIconsFadeAnim;
	QPointer<tIconsFadeAnim> m_iconsAnimPtr;
	qreal m_iconsOpacity;

};



#endif /* STATUSBARNOTIFICATIONAREA_H */
