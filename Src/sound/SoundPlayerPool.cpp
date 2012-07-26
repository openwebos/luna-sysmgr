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

#include "SoundPlayerPool.h"
#include <algorithm>
#include <cjson/json.h>

#include "HostBase.h"
#include "Preferences.h"

static SoundPlayerPool* s_instance = 0;
static const int kMaxPooledPlayers = 0;
static const int kMaxPlayers = 5;

SoundPlayerPool* SoundPlayerPool::instance()
{
    if (!s_instance)
		new SoundPlayerPool;

	return s_instance;
}

SoundPlayerPool::SoundPlayerPool()
	: m_purgeTimer(HostBase::instance()->masterTimer(), this, &SoundPlayerPool::purgeTimerFired)
{
    s_instance = this;

	bool ret;
	LSError error;
	LSErrorInit(&error);

	ret = LSRegister(NULL, &m_lsHandle, &error);
	if (!ret) {
		g_warning("Failed to register handler: %s", error.message);
		LSErrorFree(&error);
		return;
	}

	ret = LSGmainAttach(m_lsHandle, HostBase::instance()->mainLoop(), &error);
	if (!ret) {
		g_warning("Failed to attach service to main loop: %s", error.message);
		LSErrorFree(&error);
		return;
	}
}

SoundPlayerPool::~SoundPlayerPool()
{
	s_instance = 0;
	
	// no-op    
}

sptr<SoundPlayer> SoundPlayerPool::play(const std::string& filePath,
										const std::string& streamClass,
										bool repeat, int duration)
{
	sptr<SoundPlayer> player = getPooledPlayer();
	if (!player.get()) {
		if (SoundPlayer::m_numInstances >= kMaxPlayers) {
			g_warning ("Exceeded maximum instances of sound players %d, ignoring request", kMaxPlayers);
			return 0;
            }
		
		player = new SoundPlayer;
	}
	
	// Add to list of active players
	m_activePlayers.push_back(player);

	player->play(filePath, streamClass, repeat, duration);

	return player;
}

void SoundPlayerPool::stop(sptr<SoundPlayer> player)
{
    if (player.get())
        player->stop();

	// once the player is asynchronously closed, it will add
	// itself to the pool of dormant clients
}

void SoundPlayerPool::playFeedback(const std::string& name,
								   const std::string& sinkName)
{
	if (sinkName.empty() && !Preferences::instance()->playFeedbackSounds())
		return;
	
	bool ret;
	LSError error;
	LSErrorInit(&error);

	if (!m_lsHandle)
		return;

	json_object* json = json_object_new_object();
	json_object_object_add(json, "name", json_object_new_string(name.c_str()));

	if (!sinkName.empty())
		json_object_object_add(json, "sink", json_object_new_string(sinkName.c_str()));

	ret = LSCall(m_lsHandle, "palm://com.palm.audio/systemsounds/playFeedback",
				 json_object_to_json_string(json), NULL, NULL, NULL, &error);
	if (!ret) {
		g_warning("Failed in playFeedback call: %s", error.message);
		LSErrorFree(&error);
	}
	json_object_put(json);
}

void SoundPlayerPool::queueFinishedPlayer(sptr<SoundPlayer> player)
{
	// Add to list of finished players
	bool foundInFinishedPlayers = false;
	for (PlayerList::const_iterator it = m_finishedPlayers.begin();
		it != m_finishedPlayers.end(); ++it) {
		if (it->get() == player.get()) {
			foundInFinishedPlayers = true;
			break;
		}
	}

	if (!foundInFinishedPlayers)
		m_finishedPlayers.push_back(player);

	// Remove from list of active players
	for (PlayerList::iterator it = m_activePlayers.begin(); it != m_activePlayers.end(); ++it) {
		if (it->get() == player.get()) {
			m_activePlayers.erase(it);
			break;
		}
	}
		
	if (!m_purgeTimer.running())
		m_purgeTimer.start(0);
}

bool SoundPlayerPool::purgeTimerFired()
{
	PlayerList::iterator it;
	
	// First delete all dead players
	it = m_finishedPlayers.begin();
	while (it != m_finishedPlayers.end()) {
		
		if ((*it)->dead())
			it = m_finishedPlayers.erase(it);
		else
			++it;
	}

	int count = m_finishedPlayers.size();
	it = m_finishedPlayers.begin();
	while (it != m_finishedPlayers.end()) {

		if (count <= kMaxPooledPlayers)
			break;

		it = m_finishedPlayers.erase(it);
	}
	
    return false;
}

sptr<SoundPlayer> SoundPlayerPool::getPooledPlayer()
{
/*	
	PlayerList::iterator it = m_finishedPlayers.begin();
	while (it != m_finishedPlayers.end()) {

		sptr<SoundPlayer> player = (*it);
		// explicitly remove from dormant set
		it = m_finishedPlayers.erase(it);

		if (player->dead())
			continue;

		return player;
	}

	return 0;
*/

	// No pooling: mediaserver cannot handle back to back playbacks
	// on the same client very well
	return 0;
}
