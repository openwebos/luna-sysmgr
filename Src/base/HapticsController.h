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




#ifndef __HAPTICS_CONTROLLER_H__
#define __HAPTICS_CONTROLLER_H__
#include "Common.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <lunaservice.h>
#include <glib.h>
#include "HostBase.h"

#define VIBE_ERROR(x) (x != 0)

void *vibetonz_thread(void*);

class HapticsController
{
public:
	static HapticsController *instance();
	
	virtual ~HapticsController() { }
	
	virtual int vibrate(int period,int duration) {return -1;}
	virtual int vibrate(const char *name) { return -1;}
	virtual int vibrateWithAudioFeedback(const char *name, const char *audiosample) { return -1; };
	virtual int cancel(int id) {return -1;}
	virtual void cancelAll() {}
	void addMapping(const char *key,void *value) {
		g_hash_table_insert(m_mappingTable,(void*)key,value);
	}
	int getAndRemoveMapping(const char *key) {
		int id;
		id = (int)g_hash_table_lookup(m_mappingTable,(void*)key);
		g_hash_table_remove(m_mappingTable,(void*)key);
		return id;
	}
	void startService();
protected:
	HapticsController();
	LSHandle *m_service;
private:
	static HapticsController *s_instance;
	GHashTable *m_mappingTable;
};

#endif
