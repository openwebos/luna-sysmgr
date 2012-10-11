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




#include "KeyLocationRecorder.h"
#include "BannerMessageHandler.h"
#include "BannerMessageEventFactory.h"

#include <time.h>
#include <QString>
#include "Utils.h"
#include "Logging.h"
#include <sys/stat.h>

#if defined(TARGET_DEVICE)
#define PATH_PREFIX "/media/internal"
#else
#define PATH_PREFIX getenv("HOME")
#endif

static std::string sAppID = "LunaSysMgr/KeyLocationRecorder";

KeyLocationRecorder::KeyLocationRecorder() : m_file(NULL)
{
}

KeyLocationRecorder & KeyLocationRecorder::instance()
{
	static KeyLocationRecorder sInstance;
	return sInstance;
}

void KeyLocationRecorder::startStop(const char * layoutName, const QRect & keymapRect)
{
	std::string	msg;
	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
		msg = "Virtual Keyboard Recording Stopped!";
	}
	else
	{
		struct tm times;
		time_t now = ::time(0);
		::localtime_r(&now, &times);
		std::string name = string_printf("%s/keyLocationRecordings/keys%02d-%02d_%02dh%02dm%02ds.txt", PATH_PREFIX,
										 times.tm_mon + 1, times.tm_mday, times.tm_hour, times.tm_min, times.tm_sec);
		mkdir(string_printf("%s/keyLocationRecordings", PATH_PREFIX).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		m_file = fopen(name.c_str(), "w");
		keyboardSizeChanged(layoutName, keymapRect);
		msg = "Virtual Keyboard Recording Started!";
	}
	if (!m_lastMessageID.empty())
	{
		BannerMessageEvent* e = BannerMessageEventFactory::createRemoveMessageEvent(sAppID, m_lastMessageID);
		BannerMessageHandler::instance()->handleBannerMessageEvent(e);
		m_lastMessageID.clear();
		delete e;
	}
	if (!msg.empty())
	{
		BannerMessageEvent* e = BannerMessageEventFactory::createAddMessageEvent(sAppID, msg, "{ }", "", "", "", -1, false);
		m_lastMessageID = e->msgId;
		BannerMessageHandler::instance()->handleBannerMessageEvent(e);
		delete e;
	}
}

void KeyLocationRecorder::keyboardSizeChanged(const char * layoutName, const QRect & keymapRect)
{
	if (m_file)
		fprintf(m_file, "Layout: %s, Dimension: %dx%d\n", layoutName, keymapRect.width(), keymapRect.height());
}

void KeyLocationRecorder::record(const QString & text, const QPoint & where, const QString & altText)
{
	if (m_file)
	{
		if (text == altText || altText.isEmpty())
			fprintf(m_file, "%4d x %3d %s\n", where.x(), where.y(), text.toUtf8().data());
		else
			fprintf(m_file, "%4d x %3d %s ===> %s\n", where.x(), where.y(), text.toUtf8().data(), altText.toUtf8().data());
		fflush(m_file);
	}
}


