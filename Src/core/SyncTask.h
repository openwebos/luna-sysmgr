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




#ifndef SYNCTASK_H
#define SYNCTASK_H

#include "Common.h"

#include "TaskBase.h"

class SyncTask : public TaskBase
{
public:

	SyncTask();
	SyncTask(GMainContext* ctxt);
	virtual ~SyncTask();

	virtual void run();
	virtual void quit();

private:

	virtual void handleEvent(sptr<Event> event) { /* NO-OP */ }
	
	bool destroyMainLoop;
};

#endif /* SYNCTASK_H */
