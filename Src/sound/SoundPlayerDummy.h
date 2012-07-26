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




#ifndef SOUNDPLAYERDUMMY_H
#define SOUNDPLAYERDUMMY_H

#include "Common.h"

#include <string>

#include "sptr.h"

/**
 * Simple media playback class to demonstrate a possible way to connect up to the
 * luna bus and use a MediaPlayer object.
 */
class SoundPlayer : public RefCounted
{
public:

    /**
     * Construct a player to play the media content at the given URI.
     */
	SoundPlayer();
	~SoundPlayer();

	void 		play(const std::string& filePath, const std::string& streamClass, bool repeat, int duration);
	void 		stop();

	bool 		dead() const { return true; }

	static int m_numInstances; // to keep count of number of instantiations
};	


#endif /* SOUNDPLAYERDUMMY_H */
