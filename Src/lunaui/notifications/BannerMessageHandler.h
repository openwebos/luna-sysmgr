/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef BANNERMESSAGEHANDLER_H
#define BANNERMESSAGEHANDLER_H

#include "Common.h"

#include <string>
#include <QObject>

#include "sptr.h"
#include "SoundPlayerPool.h"

class QPainter;

class BannerMessage;
class BannerMessageEvent;
class BannerMessageHandlerPriv;

class ActiveCallBannerEvent;

class BannerMessageView
{
public:
	enum ScrollType {
		VerticalScroll = 0,
		HorizontalScroll,
		StatusBarScroll,   // in this mode the Banner Message width chage, but the status bar is responsible for scrolling and clipping
		NoScroll
	};

	BannerMessageView(ScrollType type) { m_scrollType = type; }
	virtual ~BannerMessageView() {}

	ScrollType scrollType() { return m_scrollType; }

	virtual int  bmViewGetWidth() const = 0;
	virtual int  bmIsViewFullyExpanded() const { return false; }
	virtual void bmViewUpdated() = 0;
	virtual void bmViewMessageCountUpdated(int count) = 0;
	virtual void bmViewShowingNonSuppressibleMessage(bool showing) = 0;

	virtual void bmViewAddActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time) = 0;
	virtual void bmViewRemoveActiveCallBanner() = 0;
	virtual void bmViewUpdateActiveCallBanner(const std::string& msg, const std::string& icon, uint32_t time) = 0;

	ScrollType m_scrollType;
};

class BannerMessageHandler : public QObject
{
	Q_OBJECT
	
public:

	static BannerMessageHandler* instance();

	void registerView(BannerMessageView* view);

	void unregisterView(BannerMessageView* view);

	int viewHeight() const;

	qreal getViewContentWidth(const BannerMessageView* view);

	void drawView(QPainter* painter, const BannerMessageView* view);

	void handleBannerMessageEvent(BannerMessageEvent* e);

	void handleActiveCallBannerEvent(ActiveCallBannerEvent* e);
	
	void addMessage(const std::string& appId,
					const std::string& msgId,
					const std::string& msg,
					const std::string& launchParams,
					const std::string& icon,
					const std::string& soundClass,
					const std::string& soundFile,
					int duration,
					bool doNotSuppress);

	void removeMessage(const std::string& appId,
					   const std::string& msgId);

	void clearMessages(const std::string& appId);

	void activateCurrentMessage();

	sptr<SoundPlayer> playSound(const std::string& appId, const std::string& soundClass,
								const std::string& soundFile, int duration, bool loop=false,
								bool wakeupScreen=false);
	
private:

	BannerMessageHandler();
	~BannerMessageHandler();

	void generateIcon(BannerMessage* m);
	void setupState(BannerMessage* m, int viewWidth);
	void playSound(BannerMessage* m);

	void paintBanner(QPainter* p, BannerMessage* m);

	int msgCount() const;

	bool isDefaultNotificationSound(const std::string& filePath) const;
	bool isDefaultAlertSound(const std::string& filePath) const;
	std::string pulseAudioSinkName(const std::string& streamClass) const;

Q_SIGNALS:

	void signalHideBanner();

private Q_SLOTS:

	void aboutToShowBanner();
	void aboutToHideBanner();
	void bannerPositionChanged();
	void bannerStateMachineFinished();

private:

	BannerMessageHandlerPriv* d;
	friend class BannerMessageHandlerPriv;
};

#endif /* BANNERMESSAGEHANDLER_H */
