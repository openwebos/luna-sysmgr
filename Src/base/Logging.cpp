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

#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <cstring>
#include <map>
#include <errno.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#include "Logging.h"
#include "MutexLocker.h"
#include "Settings.h"

static GStaticMutex s_mutex       = G_STATIC_MUTEX_INIT;
static bool         s_initialized = false;
static GHashTable*  s_channelHash = 0;

#ifdef ENABLE_TRACING

__thread int gGlobalLogIndent = -1;

SysMgrTracer::SysMgrTracer(const char* function, const char* file, int line)
	: m_indentSpaces(2)
{
	gGlobalLogIndent++;
	g_message("%*s-> %s (%s %d)", gGlobalLogIndent * m_indentSpaces, "", function, file, line);
	m_function = function;
}

SysMgrTracer::~SysMgrTracer()
{
	g_message("%*s<- %s", gGlobalLogIndent * m_indentSpaces, "", m_function); 	
	gGlobalLogIndent--;
}
#endif

bool LunaChannelEnabled(const char* channel)
{
	if (!channel)
		return false;

	bool ret = false;
	
	g_static_mutex_lock(&s_mutex);
		
	if (!s_initialized) {

		s_initialized = true;
		int index = 0;

		s_channelHash = ::g_hash_table_new(g_str_hash, g_str_equal);
		
		const char* env = ::getenv("LUNA_LOGGING");
		if (!env)
			goto Done;
		
		gchar** splitStr = ::g_strsplit(env, ",", 0);
		if (!splitStr)
			goto Done;

		while (splitStr[index]) {
			char* key = ::g_strdup(splitStr[index]);
			key = g_strstrip(key);
			g_hash_table_insert(s_channelHash, key, (gpointer)0x1);
			index++;
		}

		::g_strfreev(splitStr);
	}

	if (g_hash_table_lookup(s_channelHash, channel))
		ret = true;

 Done:	

	g_static_mutex_unlock(&s_mutex);

	return ret;
}

LunaLogContext syslogContextGlobal()
{
#if !defined(TARGET_DESKTOP)

	static PmLogContext sGlobalContext = 0;
	if (G_UNLIKELY(sGlobalContext == 0))
		PmLogGetContext("LunaSysMgr", &sGlobalContext);
	return sGlobalContext;
	
#else

	return 0;

#endif
}

LunaLogContext syslogContextJavascript()
{
#if !defined(TARGET_DESKTOP)

	static PmLogContext sGlobalContextJS = 0;
	if (G_UNLIKELY(sGlobalContextJS == 0))
		PmLogGetContext("LunaSysMgrJS", &sGlobalContextJS);
	return sGlobalContextJS;
	
#else

	return 0;

#endif
}

#if !defined(TARGET_DESKTOP)
PmLogLevel GLogToPmLogLevel(GLogLevelFlags flags)
{
	switch (flags & G_LOG_LEVEL_MASK) {
	case G_LOG_LEVEL_ERROR:
		return kPmLogLevel_Error;
	case G_LOG_LEVEL_CRITICAL:
		return kPmLogLevel_Critical;
	case G_LOG_LEVEL_WARNING:
		return kPmLogLevel_Warning;
	case G_LOG_LEVEL_MESSAGE:
		return kPmLogLevel_Notice;
	case G_LOG_LEVEL_DEBUG:
		return kPmLogLevel_Debug;
	case G_LOG_LEVEL_INFO:
	default:
		return kPmLogLevel_Info;
	}

	// Not reached
	return kPmLogLevel_Info;
}
#endif

static Mutex slogFilter_mutex;
static std::string sLogIndent;
static GAsyncQueue* sLogAsyncQueue = 0;
static GThread* sLogThread = 0;

LogIndent::LogIndent(const char * indent) : mIndent(indent)
{
	MutexLocker		lock(&slogFilter_mutex);
	sLogIndent += indent;
}

LogIndent::~LogIndent()
{
	MutexLocker		lock(&slogFilter_mutex);
	sLogIndent.resize(sLogIndent.size() - ::strlen(mIndent));
}

static const char * logLevelName(GLogLevelFlags logLevel)
{
	const char * name = "unknown";
	switch (logLevel & G_LOG_LEVEL_MASK) {
		 case G_LOG_LEVEL_ERROR:
			 name = "ERROR";
			 break;
		 case G_LOG_LEVEL_CRITICAL:
			 name = "CRITICAL";
			 break;
		 case G_LOG_LEVEL_WARNING:
			 name = "warning";
			 break;
		 case G_LOG_LEVEL_MESSAGE:
			 name = "message";
			 break;
		 case G_LOG_LEVEL_DEBUG:
			 name = "debug";
			 break;
		 case G_LOG_LEVEL_INFO:
		 default:
			 name = "info";
			 break;
	 }
	return name;
}


static gpointer PrvLogThread(gpointer arg)
{
	::prctl(PR_SET_NAME, "Logging", 0, 0, 0);
	::setpriority(PRIO_PROCESS, ::getpid(), 5);

	while (true) {
		char* buf = (char*) g_async_queue_pop(sLogAsyncQueue);

		GLogLevelFlags logLevel;
		gchar* message;

		logLevel = *((GLogLevelFlags*)buf);
		message = buf + sizeof(logLevel);

		luna_syslog(syslogContextGlobal(), logLevel, message);

		free(buf);
	}

	return 0;
}

static void PrvCreateLogThread()
{
	sLogAsyncQueue = g_async_queue_new();
	sLogThread = g_thread_create(PrvLogThread, 0, false, NULL);
}

static void PrvLogAtForkPrepare()
{
}

static void PrvLogAtForkParent()
{
}

static void PrvLogAtForkChild()
{
	// At fork, reset the global variables, so that a new thread and async queue
	// is created for every process context
	sLogAsyncQueue = 0;
	sLogThread = 0;
}

void logInit()
{
	Settings* settings = Settings::LunaSettings();
	if (!settings->logger_useTerminal && settings->logger_useSyslog) {
		pthread_atfork(PrvLogAtForkPrepare, PrvLogAtForkParent, PrvLogAtForkChild);
		PrvCreateLogThread();
	}
}

void logFilter(const gchar *log_domain, GLogLevelFlags logLevel, const gchar *message, gpointer unused_data)
{
	Settings* settings = Settings::LunaSettings();
	if (logLevel > settings->logger_level || message == 0 || *message == 0)
		return;

	if (!settings->logger_useTerminal)
	{
		if (settings->logger_useSyslog) {

			if (G_LIKELY(sLogThread)) {

				int msgSize = strlen(message) + 1;
				int bufSize = sizeof(logLevel) + msgSize;
				char* buffer = (char*) malloc(bufSize);
				*((GLogLevelFlags*)buffer) = logLevel;
				memcpy(buffer + sizeof(logLevel), message, msgSize);

				g_async_queue_push(sLogAsyncQueue, buffer);
			}
			else {

				luna_syslog(syslogContextGlobal(), logLevel, message);
			}
		}
		else
			g_log_default_handler(log_domain, logLevel, message, unused_data);
	}
	else
	{
		MutexLocker					lock(&slogFilter_mutex);	// race protection only needed for terminal and file logging

		static struct timespec		sLogStartSeconds = { 0 };
		static struct tm			sLogStartTime = { 0 };
		const char *				indent = sLogIndent.c_str();	// we own slogFilter_mutex, so we're ok cache this value while we do
		if (sLogStartSeconds.tv_sec == 0 && sLogStartSeconds.tv_nsec == 0)
		{
			time_t now = ::time(0);
			::clock_gettime(CLOCK_MONOTONIC, &sLogStartSeconds);
			::localtime_r(&now, &sLogStartTime);
			char startTime[64];
			::asctime_r(&sLogStartTime, startTime);
			::fprintf(stdout, "Sysmgr starting at %s", startTime);
		}
		struct timespec now;
		::clock_gettime(CLOCK_MONOTONIC, &now);
		int ms = (now.tv_nsec - sLogStartSeconds.tv_nsec) / 1000000;
		int sec = sLogStartTime.tm_sec + int (now.tv_sec - sLogStartSeconds.tv_sec);
		if (ms < 0)
		{
			ms += 1000;
			--sec;
		}
		int min = sLogStartTime.tm_min + sec / 60;
		int hr = sLogStartTime.tm_hour + min / 60;
		min = min % 60;
		sec = sec % 60;
		char timeStamp[128];
		char levelName = *logLevelName(logLevel);	// just use one letter
		const char * format = g_ascii_isupper(levelName) ? "%02d:%02d:%02d.%03d*%c*" : "%02d:%02d:%02d.%03d %c ";
		size_t len = ::snprintf(timeStamp, 128, format, hr, min, sec, ms, levelName);
		if (len >= G_N_ELEMENTS(timeStamp))
		{
			len = G_N_ELEMENTS(timeStamp) - 1;
			timeStamp[len] = 0;
		}
		len = ::strlen(message);
		bool	needLF = false;
		if (len < 1 || message[len - 1] != '\n')
			needLF = true;

//	#define BLACK 		0
//	#define RED			1
//	#define GREEN		2
//	#define YELLOW		3
//	#define BLUE		4
//	#define MAGENTA		5
//	#define CYAN		6
//	#define	WHITE		7

//	foreground			30 + color
//	background			40 + color

//	#define RESET		0
//	#define BRIGHT 		1
//	#define DIM			2
//	#define UNDERLINE 	3
//	#define BLINK		4
//	#define REVERSE		7
//	#define HIDDEN		8

#define COLORESCAPE		"\033["

#define RESETCOLOR		COLORESCAPE "0m"

#define BOLDCOLOR		COLORESCAPE "1m"
#define REDOVERBLACK	COLORESCAPE "1;31m"
#define BLUEOVERBLACK	COLORESCAPE "1;34m"
#define YELLOWOVERBLACK	COLORESCAPE "1;33m"

		FILE * output = g_ascii_isupper(levelName) ? stderr : stdout;
		if (levelName ==  'd' || !settings->logger_useColor)
		{
			if (needLF)
				::fprintf(output, "%s%s(%d) %s%s\n", timeStamp, indent, getpid(), indent, message);
			else
				::fprintf(output, "%s%s(%d) %s%s", timeStamp, indent, getpid(), indent, message);
		}
		else if (levelName ==  'w')
		{
			if (needLF)
				::fprintf(output, YELLOWOVERBLACK "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
			else
				::fprintf(output, YELLOWOVERBLACK "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
		}
		else if (levelName ==  'm')
		{
			if (needLF)
				::fprintf(output, BLUEOVERBLACK "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
			else
				::fprintf(output, BLUEOVERBLACK "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
		}
		else if (g_ascii_isupper(levelName))
		{
			if (needLF)
				::fprintf(output, REDOVERBLACK "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
			else
				::fprintf(stderr, REDOVERBLACK "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
		}
		else
		{
			if (needLF)
				::fprintf(output, BOLDCOLOR "%s%s(%d) %s%s" RESETCOLOR "\n", timeStamp, indent, getpid(), indent, message);
			else
				::fprintf(output, BOLDCOLOR "%s%s(%d) %s%s" RESETCOLOR, timeStamp, indent, getpid(), indent, message);
		}
		::fflush(stdout);
	}
}

// In case a VERIFY fails repeatedly, we do not want to fill-up the log files...
// The strategy is that after a number of systematic reports, we require a minimum delay between reports.
// This delay grows as the same error keep occurring, up to a max delay.

class Location
{
public:
	Location(const gchar * file, int line) : mFile(file), mLine(line) {}

	bool operator<(const Location & rhs) const	{ return mFile < rhs.mFile || (mFile == rhs.mFile && mLine < rhs.mLine); }

private:
	const gchar *	mFile;
	int 			mLine;
};

struct Details
{
	Details() : mCount(0), mLastReportedOccurenceTime(0), mTimeGap(2) {}

	int			mCount;
	__time_t	mLastReportedOccurenceTime;
	__time_t	mTimeGap;
};

static void traceContext(GLogLevelFlags logLevel)
{
	char	localBuffer[256];
	int		err = errno;
	const char *	errorName = strerror_r(err, localBuffer, G_N_ELEMENTS(localBuffer));
	g_log(G_LOG_DOMAIN, logLevel, "FYI, errno is: %d '%s'", err, errorName);
}

static Mutex sLog_and_filter_mutex;

// log a failure, but filter out repeated occurrences to avoid filling-in the logs
static void log_and_filter(GLogLevelFlags logLevel, const char * name, const char * error, const char * description, const char * function, const char * file, int line)
{
	MutexLocker	lock(&sLog_and_filter_mutex);

	static std::map<Location, Details>	sFailureLog;
	Details	& details = sFailureLog[Location(file, line)];
	struct timespec	now;
	::clock_gettime(CLOCK_MONOTONIC, &now);
	const int cMaxLogAll = 20;
	const __time_t cMaxGap = 60 * 5; // 5 minutes
	if (++details.mCount < cMaxLogAll)
	{
		g_log(G_LOG_DOMAIN, logLevel, "%s: \"%s\" %sin %s:%d, in %s()", name, error, description, file, line, function);
		traceContext(logLevel);
		details.mLastReportedOccurenceTime = now.tv_sec;
	}
	else if (details.mCount == cMaxLogAll)
	{
		g_log(G_LOG_DOMAIN, logLevel, "%s: \"%s\" %sin %s:%d, in %s(), for the %dth time. *** We might NOT report every occurrence anymore! ***", name, error, description, file, line, function, cMaxLogAll);
		traceContext(logLevel);
		details.mLastReportedOccurenceTime = now.tv_sec;
	}
	else
	{
		if (details.mLastReportedOccurenceTime + details.mTimeGap <= now.tv_sec)
		{
			g_log(G_LOG_DOMAIN, logLevel, "%s: \"%s\" %sin %s:%d, in %s(), for the %dth time.", name, error, description, file, line, function, details.mCount);
			traceContext(logLevel);
			details.mLastReportedOccurenceTime = now.tv_sec;
			// increase reporting gap between two failures by 50%, up to a max, that sets the minimum reporting frequency
			details.mTimeGap += details.mTimeGap / 2;
			if (details.mTimeGap > cMaxGap)
				details.mTimeGap = cMaxGap;
		}
	}
}

void logFailure(const gchar * message, const gchar * file, int line, const gchar * function)
{
	log_and_filter(G_LOG_LEVEL_CRITICAL, "Failure", message, "", function, file, line);
}

void logFailedVerify(const gchar * test, const gchar * file, int line, const gchar * function)
{
	log_and_filter(G_LOG_LEVEL_CRITICAL, "Failed verify", test, "is false ", function, file, line);
}

void logCheck(const gchar * test, const gchar * file, int line, const gchar * function)
{
	log_and_filter(G_LOG_LEVEL_WARNING, "Failed check", test, "is false ", function, file, line);
}
