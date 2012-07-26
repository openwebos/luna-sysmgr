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




#ifndef LOGGING_H
#define LOGGING_H

#include "Common.h"

#include <stdio.h>
#include <glib.h>

#if !defined(TARGET_DESKTOP)
#include <PmLogLib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool LunaChannelEnabled(const char* channel);

#ifdef ENABLE_TRACING

class SysMgrTracer
{
public:
	SysMgrTracer(const char* function, const char* file, int line);
	virtual ~SysMgrTracer();

private:
	const char* m_function;
	const int m_indentSpaces;
};

#define SYSMGR_TRACE()	SysMgrTracer __sysmgrtracer(__PRETTY_FUNCTION__, __FILE__, __LINE__)

#else
#define SYSMGR_TRACE()
#endif  // ENABLE_TRACING

#ifndef NO_LOGGING

#define luna_log(channel, ...)                                              \
do {                                                                                \
    if (LunaChannelEnabled(channel)) {                                              \
	   fprintf(stdout, "LOG<%s>:(%s:%d) ", channel, __PRETTY_FUNCTION__, __LINE__);  \
       fprintf(stdout, __VA_ARGS__);                                        \
       fprintf(stdout, "\n");                                                       \
    }                                                                               \
} while(0)

#define luna_warn(channel, ...)                                             \
do {                                                                                \
    if (LunaChannelEnabled(channel)) {                                              \
	   fprintf(stdout, "WARN<%s>:(%s:%d) ", channel, __PRETTY_FUNCTION__, __LINE__); \
       fprintf(stdout, __VA_ARGS__);                                        \
       fprintf(stdout, "\n");                                                       \
    }                                                                               \
} while(0)

#else // NO_LOGGING

#define luna_log(channel, ...) (void)0
#define luna_warn(channel, ...) (void)0

#endif // NO_LOGGING

#define luna_critical(channel, ...)                                         \
do {                                                                                \
   fprintf(stdout, "CRITICAL<%s>:(%s:%d) ", channel, __PRETTY_FUNCTION__, __LINE__); \
   fprintf(stdout, __VA_ARGS__);                                            \
   fprintf(stdout, "\n");                                                           \
} while(0)

#define luna_assert(val)						\
	do {										\
		if (G_LIKELY(val)) {}					\
		else {									\
			g_critical("FATAL:(%s:%d) %s", __PRETTY_FUNCTION__, __LINE__, #val); \
			int* p = 0;													\
			*p = 0;														\
		}																\
	} while(0)															\

		
		
// Logging routed to PmLogLib on device

#if defined(TARGET_DESKTOP)

#define LunaLogContext void*

#define luna_syslog(context, level, message)	\
	g_log_default_handler(0, level, message, 0)

#define luna_syslogV(context, level,  ...)	\
	g_log(0, level, __VA_ARGS__)

#else

PmLogLevel GLogToPmLogLevel(GLogLevelFlags flags);

#define LunaLogContext PmLogContext

#define luna_syslog(context, level, message)						\
	PmLogPrint((PmLogContext) context, GLogToPmLogLevel(level), "%s", message)

#define luna_syslogV(context, level, ...)						\
	PmLogPrint((PmLogContext) context, GLogToPmLogLevel(level), __VA_ARGS__)

#endif

LunaLogContext syslogContextGlobal();
LunaLogContext syslogContextJavascript();

void logInit();
void logFilter(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer unused_data);

/// Test macro that will make a critical log entry if the test fails
#define VERIFY(t) (G_LIKELY(t) || (logFailedVerify(#t, __FILE__, __LINE__, __FUNCTION__), false))

/// Test macro that will make a warning log entry if the test fails
#define CHECK(t) (G_LIKELY(t) || (logCheck(#t, __FILE__, __LINE__, __FUNCTION__), false))

/// Direct critical message to put in the logs with file & line number, with filtering of repeats
#define FAILURE(m) logFailure(m, __FILE__, __LINE__, __FUNCTION__)

#define SHOULD_NOT_REACH_HERE FAILURE("This line should never be reached")

/// Functions that implement the macros above. You probably don't want to call them directly...
void logFailedVerify(const gchar * test, const gchar * file, int line, const gchar * function);
void logFailure(const gchar * message, const gchar * file, int line, const gchar * function);
void logCheck(const gchar * message, const gchar * file, int line, const gchar * function);

#ifdef __cplusplus
}

/// Helper class for log presentation purposes. Indents the logs with the text passed as long as the object exists.
class LogIndent {
public:
	LogIndent(const char * indent);
	~LogIndent();

private:
	const char *	mIndent;
};

#endif

#endif /* LOGGING_H */
