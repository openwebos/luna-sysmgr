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




#ifndef SOUNDPLAYERPOOL_H
#define SOUNDPLAYERPOOL_H

#include "Common.h"

#include <list>
#include <string>
#include <lunaservice.h>

#include "sptr.h"
#include "Timer.h"

#if defined(TARGET_DESKTOP) || defined(TARGET_EMULATOR)
#include "SoundPlayerDummy.h"
#else
#include "SoundPlayer.h"
#endif

class SoundPlayerPool
{
public:

	static SoundPlayerPool* instance();

	sptr<SoundPlayer> play(const std::string& filePath,
						   const std::string& streamClass,
						   bool repeat, int duration);
	void stop(sptr<SoundPlayer> player);

	void playFeedback(const std::string& name, const std::string& sinkName=std::string());

private:

	SoundPlayerPool();
	~SoundPlayerPool();

	void queueFinishedPlayer(sptr<SoundPlayer> player);
	bool purgeTimerFired();
	sptr<SoundPlayer> getPooledPlayer();

private:

	typedef std::list<sptr<SoundPlayer> > PlayerList;

	PlayerList m_activePlayers;
	PlayerList m_finishedPlayers;
	LSHandle* m_lsHandle;
	Timer<SoundPlayerPool> m_purgeTimer;

	friend class SoundPlayer;
};

#endif /* SOUNDPLAYERPOOL_H */
