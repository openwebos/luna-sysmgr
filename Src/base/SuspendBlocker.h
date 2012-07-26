/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef SUSPENDBLOCKER_H
#define SUSPENDBLOCKER_H

#include "Common.h"

#include <glib.h>
#include <lunaservice.h>

class SuspendBlockerBase
{
public:

	SuspendBlockerBase(GMainLoop* mainLoop);
	~SuspendBlockerBase();

protected:

	virtual bool allowSuspend() = 0;
	virtual void setSuspended(bool) = 0;

private:

	void setupService(LSHandle*& service, GMainLoop* loop);
	void callService(LSHandle* service, LSFilterFunc callback,
					 const char* url, const char* message);
	void registerSuspendRequest();
	void registerPrepareSuspend();
	
	static bool cbSuspendRequest(LSHandle* sh, LSMessage* message, void* ctx);
	static bool cbPrepareSuspend(LSHandle* sh, LSMessage* message, void* ctx);
	static bool cbResume(LSHandle* sh, LSMessage* message, void* ctx);
	static bool cbPowerdUp(LSHandle* sh, LSMessage* message, void* ctx);
	static bool cbIdentify(LSHandle* sh, LSMessage* message, void* ctx);

private:

	char* m_name;
	char* m_id;
	
	GMainLoop* m_mainLoop;
	LSHandle* m_service;

	LSHandle* m_nestedService;
	GMainContext* m_nestedCtxt;
	GMainLoop* m_nestedLoop;

	bool m_registeredSuspendRequest;
	bool m_registeredPrepareSuspend;

private:

	SuspendBlockerBase(const SuspendBlockerBase&);
	SuspendBlockerBase& operator=(const SuspendBlockerBase&);
};

template<class Target>
class SuspendBlocker : public SuspendBlockerBase
{
public:

	typedef bool (Target::*AllowSuspendFunction)();
	typedef void (Target::*SetSuspendedFunction)(bool);

	SuspendBlocker(GMainLoop* loop, Target* target, AllowSuspendFunction f1, SetSuspendedFunction f2)
		: SuspendBlockerBase(loop)
		, m_target(target)
		, m_allowSuspendFunction(f1)
		, m_setSuspendedFunction(f2) {}

private:

	virtual bool allowSuspend() { return (m_target->*m_allowSuspendFunction)(); }
	virtual void setSuspended(bool isSuspended) { (m_target->*m_setSuspendedFunction)(isSuspended); }

private:

	Target* m_target;
	AllowSuspendFunction m_allowSuspendFunction;
	SetSuspendedFunction m_setSuspendedFunction;
};


#endif /* SUSPENDBLOCKER_H */
