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




#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include "Common.h"

#include <string>
#include <list>
#include <glib.h>
#include <pthread.h>

#include "sptr.h"

/**
 * these need to be define since qemux86 still uses gcc 4.2.1 
 */
#define BOOST_NO_TYPEID
#define BOOST_NO_RTTI

#include <media/LunaConnector.h>
#include <media/MediaClient.h>
#include <media/MediaPlayer.h>

#include "Timer.h"

using namespace media;
using namespace boost;
using namespace std;

/**
 * Simple media playback class to demonstrate a possible way to connect up to the
 * luna bus and use a MediaPlayer object.
 */
class SoundPlayer : public RefCounted, public LunaConnector {

	/*
	 * This listener merely forwards notifications to SoundPlayer object
	 * Can't use multiple inheritance, because it's refcounted via boost::shared_ptr
	 */
	class SoundPlayerMediaPlayerChangeListener : public MediaPlayerChangeListener
	{
	public:
		SoundPlayerMediaPlayerChangeListener(SoundPlayer * player) : m_soundPlayer(player) {}

		void	disconnect()					{ m_soundPlayer = 0; }

		virtual void currentTimeChanged()		{ if (m_soundPlayer) m_soundPlayer->currentTimeChanged(); }
		virtual void eosChanged()				{ if (m_soundPlayer) m_soundPlayer->eosChanged(); }
		virtual void sourceChanged()			{ if (m_soundPlayer) m_soundPlayer->sourceChanged(); }
		virtual void errorChanged()				{ if (m_soundPlayer) m_soundPlayer->errorChanged(); }
		virtual void extendedErrorChanged()		{ if (m_soundPlayer) m_soundPlayer->extendedErrorChanged(); }

	private:
		SoundPlayer *	m_soundPlayer;
	};

	enum EState
	{
		// Created, but nothing happened yet
		eState_Init,

		// Waiting for connection
		eState_Connecting,

		// Connected, waiting for a request
		eState_Connected,

		// Play request sent, but not playing yet
		eState_PlayPending,

		// Playing. Audio is flowing out...
		eState_Playing,

		// Stop request issued. Waiting for completion
		eState_Closing,

		// Done: we're all done successfully (you can try to reuse us)
		eState_Finished,

		// Dead: we're finished (successful or not), and don't try to use us again!
		eState_Dead
	};

public:

    /**
     * Construct a player to play the media content at the given URI.
     */
	SoundPlayer();
	~SoundPlayer();

	void 		play(const std::string& filePath, const std::string& streamClass, bool repeat, int duration);
	void 		stop();

	bool 		dead() const { return m_state == eState_Dead; }

	static int m_numInstances; // to keep count of number of instantiations

protected:

	/**
	 * LunaConnector callback methods
	 */

    virtual LSPalmService *	connectToBus();
    virtual void 			connected();

    /**
     * MediaPlayerChangeListener callbacks methods
     */

	void			currentTimeChanged();
	void			eosChanged();
	void			sourceChanged();
	void			errorChanged();
	void			extendedErrorChanged();

private:
	/**
	 * Internal implementation methods
	 */
	bool			healthCheck();
	void			checkForNextStep();
	void			setState(EState state);
	const char *	getStateName();
	guint64			currentTime();
	void			startTimer();
	string			getURI();
	void			onError();

	LSHandle *		getServiceHandle();

private:
    shared_ptr<MediaPlayer> m_player;
	LSPalmService*			m_serviceHandle;
	string					m_filePath;
	string					m_streamClass;
	AudioStreamClass		m_audioStreamClass;
	bool					m_repeat;
	int						m_retries;
	float					m_duration;
	EState					m_state;
	guint64					m_lastPlayingTime;
	int						m_fakeBackupRingtoneCount;
	boost::shared_ptr<MediaPlayerChangeListener>	m_mediaPlayerChangeListener;

	Timer<SoundPlayer>		m_timer;
	int						m_activityID;

	static int				m_activityCount;
};

#endif /* SOUNDPLAYER_H */
