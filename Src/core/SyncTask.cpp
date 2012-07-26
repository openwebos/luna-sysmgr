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

#include "SyncTask.h"
#include "HostBase.h"
#include "MutexLocker.h"

SyncTask::SyncTask()
{
	if (m_mainLoop)
		return;

	m_mainLoop = HostBase::instance()->mainLoop();
	m_masterTimer = HostBase::instance()->masterTimer();
	destroyMainLoop = false;
}

SyncTask::SyncTask(GMainContext* ctxt)
{
	m_mainCtxt = ctxt;
	m_mainLoop = g_main_loop_new(m_mainCtxt, FALSE);
	m_masterTimer = new SingletonTimer(m_mainLoop);
	destroyMainLoop = true;
}

SyncTask::~SyncTask()
{
    quit();

    if(destroyMainLoop) {
		delete m_masterTimer;
		g_main_loop_unref(m_mainLoop);
		g_main_context_unref(m_mainCtxt);
    }
}

void SyncTask::run()
{
}

void SyncTask::quit()
{
	// probably should do more. remove any sources we added
}
