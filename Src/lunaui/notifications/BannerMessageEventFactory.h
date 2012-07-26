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




#ifndef BANNERMESSAGEEVENTFACTORY_H
#define BANNERMESSAGEEVENTFACTORY_H

#include "Common.h"

#include <BannerMessageEvent.h>
#include <ActiveCallBannerEvent.h>

#include <time.h>
#include <glib.h>

#include <string>
#include <stdint.h>

class BannerMessageEventFactory
{
public:

	static BannerMessageEvent* createAddMessageEvent(const std::string& _appId,
													 const std::string& _msg,
													 const std::string& _launchParams,
													 const std::string& _icon,
													 const std::string& _soundClass,
													 const std::string& _soundFile,
													 int _duration,
													 bool _doNotSuppress) {
		if (_appId.empty() || _msg.empty() || _launchParams.empty())
			return 0;

		BannerMessageEvent* e = new BannerMessageEvent;
		e->msgType = BannerMessageEvent::Add;
		e->appId = _appId;
		e->msg = _msg;
		e->launchParams = _launchParams;
		e->icon = _icon;
		e->soundClass = _soundClass;
		e->soundFile = _soundFile;
		e->duration = _duration;
		e->doNotSuppress = _doNotSuppress;

		// Create a "unique" ID for the msg
		struct timespec time;
		clock_gettime(CLOCK_MONOTONIC, &time);
		double t = time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0;

		gchar* id = g_strdup_printf("%lf", t);
		e->msgId = id;
		g_free(id);

		return e;
	}

	static BannerMessageEvent* createRemoveMessageEvent(const std::string& _appId,
														const std::string& _msgId) {
		
		if (_appId.empty() || _msgId.empty())
			return 0;

		BannerMessageEvent* e = new BannerMessageEvent;
		e->msgType = BannerMessageEvent::Remove;
		e->appId = _appId;
		e->msgId = _msgId;

		return e;
	};

	static BannerMessageEvent* createClearMessagesEvent(const std::string& _appId) {
		
		if (_appId.empty())
			return 0;

		BannerMessageEvent* e = new BannerMessageEvent;
		e->msgType = BannerMessageEvent::Clear;
		e->appId = _appId;

		return e;
	};	
	
	static BannerMessageEvent* createPlaySoundEvent(const std::string& _appId,
													const std::string& _soundClass,
													const std::string& _soundFile,
													int _duration, bool _wakeupScreen=false) {
		if (_appId.empty() || _soundClass.empty())
			return 0;

		BannerMessageEvent* e = new BannerMessageEvent;
		e->msgType = BannerMessageEvent::PlaySound;
		e->appId = _appId;
		e->soundClass = _soundClass;
		e->soundFile = _soundFile;
		e->duration = _duration;
		e->wakeupScreen = _wakeupScreen;

		return e;
	}
};	
	
class ActiveCallBannerEventFactory
{
public:

	static ActiveCallBannerEvent* createAddEvent(const std::string& _msg,
												 const std::string& _icon,
												 uint32_t _time) {
		if (_msg.empty() || BannerActive)
			return 0;

		ActiveCallBannerEvent* e = new ActiveCallBannerEvent;
		e->type = ActiveCallBannerEvent::Add;
		e->msg = _msg;
		e->icon = _icon;
		e->time = _time;

		BannerActive = true;

		return e;
	}

	static ActiveCallBannerEvent* createRemoveEvent() {
		
		ActiveCallBannerEvent* e = new ActiveCallBannerEvent;
		e->type = ActiveCallBannerEvent::Remove;

		BannerActive = false;

		return e;
	};

	static ActiveCallBannerEvent* createUpdateEvent(const std::string& _msg,
													const std::string& _icon,
													uint32_t _time) {
		
		if (_msg.empty())
			return 0;

		ActiveCallBannerEvent* e = new ActiveCallBannerEvent;
		e->type = ActiveCallBannerEvent::Update;
		e->msg = _msg;
		e->icon = _icon;
		e->time = _time;

		return e;
	};

private:

	// only allow a single active call banner to exist at any time
	static bool BannerActive;
};

#endif /* BANNERMESSAGEEVENTFACTORY_H */
