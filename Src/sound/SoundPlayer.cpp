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

#include "SoundPlayer.h"

#include <map>
#include <algorithm>
#include <sys/prctl.h>

#include "DisplayManager.h"
#include "HostBase.h"
#include "SoundPlayerPool.h"
#include "Utils.h"
#include "Logging.h"

#define TRACE_DEBUG_THIS_FILE  0
#define USE_COLOR_PRINTF       1

#if TRACE_DEBUG_THIS_FILE
	#if USE_COLOR_PRINTF
		#define COLORESCAPE     "\033["
		#define RESETCOLOR      COLORESCAPE "0m"
		#define BOLDCOLOR       COLORESCAPE "1m"
		#define REDOVERBLACK    COLORESCAPE "1;31m"
		#define BLUEOVERBLACK   COLORESCAPE "1;34m"
		#define YELLOWOVERBLACK COLORESCAPE "1;33m"
	#else
		#define COLORESCAPE
		#define RESETCOLOR
		#define BOLDCOLOR
		#define REDOVERBLACK
		#define BLUEOVERBLACK
		#define YELLOWOVERBLACK
	#endif
	#define PRINTF(fmt, args...) printf("%s: " fmt "\n", getTimeStamp(), ## args)
	#define PRINTF_RED(fmt, args...) printf(REDOVERBLACK "%s: " fmt RESETCOLOR "\n", getTimeStamp(), ## args)
	#define PRINTF_BLUE(fmt, args...) printf(BLUEOVERBLACK "%s: " fmt RESETCOLOR "\n", getTimeStamp(), ## args)
	#define PRINTF_YELLOW(fmt, args...) printf(YELLOWOVERBLACK "%s: " fmt RESETCOLOR "\n", getTimeStamp(), ## args)
	#define PRINTF_BOLD(fmt, args...) printf(BOLDCOLOR "%s: " fmt RESETCOLOR "\n", getTimeStamp(), ## args)

	#define DEBUG PRINTF
	#define MESSAGE PRINTF_BLUE
	#define WARNING PRINTF_YELLOW
	#define CRITICAL PRINTF_RED

static const char * getTimeStamp()
{
	static bool sInited = false;
	static struct timespec		sLogStartSeconds = { 0 };
	static struct tm			sLogStartTime = { 0 };
	if (!sInited)
	{
		time_t now = ::time(0);
		::clock_gettime(CLOCK_MONOTONIC, &sLogStartSeconds);
		::localtime_r(&now, &sLogStartTime);
		sInited = true;
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
	static char sTimeStamp[128];
	::sprintf(sTimeStamp, "%02d:%02d:%02d.%03d", hr, min, sec, ms);
	return sTimeStamp;
}

#else
	#define PRINTF(fmt, args...) (void)0
	#define PRINTF_RED(fmt, args...) (void)0
	#define PRINTF_BLUE(fmt, args...) (void)0
	#define PRINTF_YELLOW(fmt, args...) (void)0
	#define PRINTF_BOLD(fmt, args...) (void)0

	#define DEBUG g_debug
	#define MESSAGE g_message
	#define WARNING g_warning
	#define CRITICAL g_critical
#endif

int SoundPlayer::m_numInstances = 0;
int SoundPlayer::m_activityCount = 0;

#ifdef BOOST_NO_EXCEPTIONS
#include <exception>
void boost::throw_exception(std::exception const&)
{ abort(); }	//this function is supposed to never return, so abort() seems appropriate.
#endif

/**
 * Constructor: register proxy listener
 */
SoundPlayer::SoundPlayer()
	: m_serviceHandle(0)
	, m_retries(0)
	, m_state(eState_Init)
	, m_fakeBackupRingtoneCount(0)
	, m_timer(HostBase::instance()->masterTimer(), this, &SoundPlayer::healthCheck)
	, m_activityID(0)
{
	PRINTF_BLUE("SoundPlayer::SoundPlayer(%d)", getpid());
	m_mediaPlayerChangeListener = boost::shared_ptr<MediaPlayerChangeListener>(new SoundPlayerMediaPlayerChangeListener(this));
}

/**
 * Destructor: Stop anything we have going & disconnect proxy listener
 */
SoundPlayer::~SoundPlayer()
{
	PRINTF_BLUE("SoundPlayer::~SoundPlayer(%s)", getStateName());
	if (m_state == eState_PlayPending || m_state == eState_Playing)
		stop();

	if (m_player.get()) {
		m_player->removeMediaPlayerChangeListener(m_mediaPlayerChangeListener);
		m_player.reset((media::MediaPlayer*) 0);
	}
	
	// Important: Make sure to call this before the service handle destuction
	if (m_mediaPlayerChangeListener.get())
		m_mediaPlayerChangeListener.reset((media::MediaPlayerChangeListener*)0);

	if (m_serviceHandle) {
		LSError lserror;
		LSErrorInit(&lserror);
		if (!LSUnregisterPalmService(m_serviceHandle, &lserror)) {
	        LSErrorPrint (&lserror, stderr);
	        LSErrorFree (&lserror);
		}

		m_serviceHandle = 0;
	}
}

/**
 * Client request entry point
 */
void SoundPlayer::play(const std::string& filePath, const std::string& streamClass, bool repeat, int duration)
{
	WARNING("SoundPlayer::play(%s): %p: filePath: %s, streamClass: %s, repeat: %d, duration: %d",
			  getStateName(), this, filePath.c_str(), streamClass.c_str(), repeat, duration);

	if (m_state == eState_Dead)
	{
		CRITICAL("SoundPlayer::play(%s): This player can not be reused!", getStateName());
		return;
	}

	if (m_state != eState_Init)
		stop();

	// recycling? move state machine back
	if (m_state == eState_Finished)
		setState(eState_Connected);

	m_filePath = filePath;
	m_streamClass = streamClass;
	m_audioStreamClass = kAudioStreamMedia;

	if (!m_streamClass.empty())
	{
		if (m_streamClass == "ringtones")
			m_audioStreamClass = kAudioStreamRingtone;

		else if (m_streamClass == "alarm")
			m_audioStreamClass = kAudioStreamAlarm;

		else if (m_streamClass == "alerts")
			m_audioStreamClass = kAudioStreamAlert;

		else if (m_streamClass == "notifications")
			m_audioStreamClass = kAudioStreamNotification;

		else if (m_streamClass == "calendar")
			m_audioStreamClass = kAudioStreamCalendar;

		else if (m_streamClass == "media")
			m_audioStreamClass = kAudioStreamMedia;

		else if (m_streamClass == "voicedial")
			m_audioStreamClass = kAudioStreamVoicedial;

		else if (m_streamClass == "flash")
			m_audioStreamClass = kAudioStreamFlash;

		else if (m_streamClass == "navigation")
			m_audioStreamClass = kAudioStreamNavigation;

		else if (m_streamClass == "defaultapp")
			m_audioStreamClass = kAudioStreamDefaultapp;

		else if (m_streamClass == "feedback")
			m_audioStreamClass = kAudioStreamFeedback;

		else
		{
			WARNING("SoundPlayer::play: unrecognized media class '%s' for '%s'. Will use media class.", streamClass.c_str(), filePath.c_str());
			m_audioStreamClass = kAudioStreamMedia;
		}
	}

	m_repeat = repeat;
	m_duration = float(duration) / 1000.f;	// integer ms to float seconds
	checkForNextStep();
}

/**
 * Client side cancel point. Also used internally.
 */
void SoundPlayer::stop()
{
	PRINTF_BLUE("SoundPlayer::stop(%s)", getStateName());
	if (m_state == eState_PlayPending || m_state == eState_Playing)
	{
		if (VERIFY(m_player))
		{
			m_player->pause();
			m_player->unload();
		}
		setState(eState_Closing);
	}
	else
	{	// not sure why we were not playing: don't take chance...
		WARNING("SoundPlayer::stop(%s): unexpected stop request. Declaring SoundPlayer object dead.", getStateName());
		setState(eState_Dead);
	}
	m_filePath.clear();
	m_streamClass.clear();
	m_lastPlayingTime = currentTime();
	m_fakeBackupRingtoneCount = 0;
	if (m_activityID)
	{
	    gchar *  request = g_strdup_printf("{\"id\":\"com.palm.sysmgr.soundplayer.%i\"}", m_activityID);

	    LSError lserror;
	    LSErrorInit(&lserror);
	    bool activityStartRequestSuccess = LSCall(getServiceHandle(), "palm://com.palm.power/com/palm/power/activityEnd", request, NULL, NULL, NULL, &lserror);
	    if (!activityStartRequestSuccess)
	    {
	        CRITICAL("SoundPlayer::stop: activity '%s' End failed", request);
	        LSErrorPrint (&lserror, stderr);
	        LSErrorFree (&lserror);
	    }
	    else
	    	DEBUG("SoundPlayer::stop: ended activity '%s'...", request);

	    g_free(request);
	    m_activityID = 0;
	}
}

/**
 * Connect to mediaserver using lunaservice.
 */
LSPalmService* SoundPlayer::connectToBus()
{
	PRINTF_BLUE("SoundPlayer::connectToBus(%s)", getStateName());
	VERIFY(m_state == eState_Connecting);
	setState(eState_Connecting);

	if (m_serviceHandle)
		return m_serviceHandle;

	bool result = true;
	LSError lserror;
	LSErrorInit(&lserror);

	result = LSRegisterPalmService(NULL, &m_serviceHandle, &lserror);
	if (!result)
	{
		CRITICAL("SoundPlayer::connectToBus: Could not register SoundPlayer");
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
		// Make next health check fail ASAP...
		m_lastPlayingTime = 0;
		return NULL;
	}

	result = LSGmainAttachPalmService (m_serviceHandle, HostBase::instance()->mainLoop(), &lserror);
	if (!result)
	{
		CRITICAL("SoundPlayer::connectToBus: Could not attach SoundPlayer to main loop");
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
		// Make next health check fail ASAP...
		m_lastPlayingTime = 0;
		return NULL;
	}
	return m_serviceHandle;
}

/**
 * This simply loads the media source URI and plays it.
 */
void SoundPlayer::connected() {
	PRINTF_BLUE("SoundPlayer::connected(%s): loading '%s', as '%s'", getStateName(), m_filePath.c_str(), m_streamClass.c_str());
	if (VERIFY(m_state == eState_Connecting))
		setState(eState_Connected);
	checkForNextStep();
}

/**
 * Playback is moving
 */
void SoundPlayer::currentTimeChanged()
{
	if (VERIFY(m_player))
	{
		PRINTF_BOLD("SoundPlayer::currentTimeChanged(%s): %f sec", getStateName(), m_player->getCurrentTime());
		if (m_state == eState_PlayPending || m_state == eState_Playing)
		{
			setState(eState_Playing);
			m_lastPlayingTime = currentTime();
			if (!m_repeat && m_duration > 0 && m_player->getCurrentTime() >= m_duration)
				stop();
		}
	}
}

/**
 * The end of the stream has been reached (i.e. we played the entire stream).
 */
void SoundPlayer::eosChanged()
{
    if (VERIFY(m_player) && m_player->getEos()) {
    	PRINTF_BLUE("SoundPlayer::eosChanged(%s): EOS", getStateName());
    	if (m_state == eState_PlayPending || m_state == eState_Playing)
    	{
			if (m_repeat)
			{
				m_player->seek(0);
				m_player->play();
			}
			else
				stop();
    	}
    }
}

/**
 * The object we are playing was changed.
 */
void SoundPlayer::sourceChanged()
{
	if (!VERIFY(m_player))
		return;
	PRINTF_BLUE("SoundPlayer::sourceChanged(%s): '%s'", getStateName(), m_player->getSource().c_str());
	if (m_player->getSource().empty() && m_state == eState_Closing)
	{
		setState(eState_Connected);
		checkForNextStep();
	}
}

/**
 * Errors occur asynchronously.  If an error occurs, this will be called.
 */
void SoundPlayer::errorChanged()
{
	if (!VERIFY(m_player))
		return;
    if (m_player->getError() != NO_ERROR)
    {
    	CRITICAL("SoundPlayer::errorChanged(%s): %d", getStateName(), m_player->getError());
    	onError();
    }
}

/**
 * Errors occur asynchronously.  If an error occurs, this will be called.
 */
void SoundPlayer::extendedErrorChanged()
{
	if (!VERIFY(m_player))
		return;
    if (!m_player->getExtendedError().empty())
    {
    	CRITICAL("SoundPlayer::extendedErrorChanged(%s): '%s'", getStateName(), m_player->getExtendedError().c_str());
    	onError();
    }
}

/**
 * In case of error, try again
 */
void SoundPlayer::onError()
{
	if (VERIFY(m_player) && eState_PlayPending && m_retries++ < 3)
	{
		m_player->unload();
		m_player->load(getURI(), m_audioStreamClass);
		m_player->seek(0);
		m_player->play();
	}
}

/**
 * Set the state of the player
 */
void SoundPlayer::setState(EState state)
{
	// dead is dead...
	if (m_state != eState_Dead)
	{
		if (state != m_state)
		{
			if ((state == eState_Dead || state == eState_Finished) && (m_state != eState_Dead && m_state != eState_Finished))
			{
				m_timer.stop();
				SoundPlayerPool::instance()->queueFinishedPlayer(this);
			}
		}
		m_state = state;
	}
}

/**
 * Check if we can advance the state machine by loading or playing
 */
void SoundPlayer::checkForNextStep()
{
	if (m_state == eState_Init)
	{
		if (!m_filePath.empty() && VERIFY(m_player == 0))
		{
			setState(eState_Connecting);
			startTimer();
			m_player = media::MediaClient::createLunaMediaPlayer(*this);
			m_player->addMediaPlayerChangeListener(m_mediaPlayerChangeListener);
		}
	}
	else if (m_state == eState_Connected)
	{
		if (m_filePath.empty())
		{	// if we had any problem, don't recycle this player...
			if (m_retries == 0)
				setState(eState_Finished);
			else
				setState(eState_Dead);
		}
		else if (VERIFY(m_player))
		{
			setState(eState_PlayPending);
			startTimer();
			m_player->load(getURI(), m_audioStreamClass);
			m_player->play();
		}
	}
}

/**
 * Simply start the healthCheck timer with a single refresh setting
 * Also start activity if necessary
 */
void SoundPlayer::startTimer()
{
	if (!m_timer.running())
	{
		m_timer.start(1000, false);
		m_activityID = ++m_activityCount;

	    gchar * request = g_strdup_printf("{\"id\":\"com.palm.sysmgr.soundplayer.%i\",\"duration_ms\":60000}", m_activityID);

	    LSError lserror;
	    LSErrorInit(&lserror);
	    bool activityStartRequestSuccess = LSCall(getServiceHandle(), "palm://com.palm.power/com/palm/power/activityStart", request, NULL, NULL, NULL, &lserror);
	    if (!activityStartRequestSuccess)
	    {
	    	CRITICAL("SoundPlayer::startTimer: activity '%s' Start failed", request);
	        LSErrorPrint (&lserror, stderr);
	        LSErrorFree (&lserror);
	    }
	    else
	    	DEBUG("SoundPlayer::startTimer: booked activity '%s'...", request);

	    g_free(request);
	}
	m_lastPlayingTime = currentTime();
}

static const guint64 cConnectDelay = 8000;
static const guint64 cLoadDelay = 3000;
static const guint64 cPlayDelay = 1000;
static const guint64 cCloseDelay = 2000;

/**
 * Check that we aren't stuck waiting, while loading or playing...
 */
bool SoundPlayer::healthCheck()
{
	DEBUG("SoundPlayer::healthCheck(%s, %llu ms)", getStateName(), currentTime() - m_lastPlayingTime);
	if (!m_filePath.empty() &&		// we are supposed to play something
		(	(m_state == eState_Connecting && m_lastPlayingTime + cConnectDelay < currentTime()) ||	// but we can't connect, or
			(m_state == eState_PlayPending && m_lastPlayingTime + cLoadDelay < currentTime())   ||	// we can't load the file, or
			(m_state == eState_Playing && m_lastPlayingTime + cPlayDelay < currentTime())))			// we can't play the file
	{
		if (m_fakeBackupRingtoneCount == 0)
		{	// log failure, when it first happens
			CRITICAL("SoundPlayer::healthCheck(%s): can't play '%s' as %s for %llu ms", getStateName(),
					m_filePath.c_str(), m_streamClass.c_str(), currentTime() - m_lastPlayingTime);
			m_retries++;	// we failed to play. Remember this, so that we won't recycle this player.
		}
		if (m_audioStreamClass == kAudioStreamRingtone || m_audioStreamClass == kAudioStreamAlarm)
		{	// back ringtone for ringtones & alarms
			if (m_fakeBackupRingtoneCount == 0)
				WARNING("SoundPlayer::healthCheck: using backup ringtone using alert notifications!");
			// Don't use alarm & ringtone sinks, because they trigger a ringtone vibration, which we are faking here!
			if ((m_fakeBackupRingtoneCount++ % 5) < 3)
				SoundPlayerPool::instance()->playFeedback("sysmgr_notification", "palerts");	// let's hope audiod & pulse are ok!
		}
		else if (m_audioStreamClass == kAudioStreamNotification)
		{
			WARNING("SoundPlayer::healthCheck: using default notification instead!");
			stop();
			SoundPlayerPool::instance()->playFeedback("sysmgr_alert", "palerts");	// let's hope audiod & pulse are ok!
		}
		else if (m_audioStreamClass == kAudioStreamCalendar)
		{
			WARNING("SoundPlayer::healthCheck: using default notification instead!");
			stop();
			SoundPlayerPool::instance()->playFeedback("sysmgr_alert", "pcalendar");	// let's hope audiod & pulse are ok!
		}
		else if (m_audioStreamClass == kAudioStreamAlert)
		{
			WARNING("SoundPlayer::healthCheck: using default alert instead!");
			stop();
			SoundPlayerPool::instance()->playFeedback("sysmgr_alert", "palerts");	// let's hope audiod & pulse are ok!
		}
	}
	else if (m_state == eState_Closing && m_lastPlayingTime + cCloseDelay < currentTime())
	{
		WARNING("SoundPlayer::healthCheck(%s): can't close for %llu ms. Declaring SoundPlayer object dead.", getStateName(), currentTime() - m_lastPlayingTime);
		setState(eState_Dead);
	}
	else
	{
		m_fakeBackupRingtoneCount = 0;
	}

	return true;	// return true to repeat
}

/**
 * Return URI based on file name or URI
 */
string SoundPlayer::getURI()
{
	string	uri;
	if (m_filePath.size() > 0 && m_filePath[0] == '/')
		uri = "file://" + m_filePath;
	else
		uri = m_filePath;
	return uri;
}

/**
 * Name of current state for tracing purposes
 */
const char * SoundPlayer::getStateName()
{
	switch (m_state)
	{
	case eState_Init:			return "Init";
	case eState_Connecting:		return "Connecting";
	case eState_Connected:		return "Connected";
	case eState_PlayPending:	return "Play Pending";
	case eState_Playing:		return "Playing";
	case eState_Closing:		return "Closing";
	case eState_Finished:		return "Finished";
	case eState_Dead:			return "Dead";
	}
	return "< ??? State ??? >";
}

/**
 * Current time in ms
 */
guint64 SoundPlayer::currentTime()
{
	struct timespec now;
	::clock_gettime(CLOCK_MONOTONIC, &now);
	return guint64(now.tv_sec) * 1000ULL + guint64(now.tv_nsec) / 1000000ULL;
}

/**
 * Minor utility function
 */
LSHandle * SoundPlayer::getServiceHandle()
{
	return SoundPlayerPool::instance()->m_lsHandle;
}
