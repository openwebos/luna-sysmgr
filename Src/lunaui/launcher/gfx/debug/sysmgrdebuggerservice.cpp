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




#include "sysmgrdebuggerservice.h"
#include <cjson/json.h>
#include <glib.h>
#include "lunaservice.h"
#include "HostBase.h"
#include <QDebug>
#include "qtjsonabstract.h"

#define ccstr(s) (s.toUtf8().data())
#define cstr(s) (s.toUtf8().constData())

QPointer<SysmgrDebuggerService> SysmgrDebuggerService::s_qp_instance = 0;

bool _createPixPagerDebugger(LSHandle* lshandle,LSMessage *message, void *user_data)
{
	return (SysmgrDebuggerService::service() ? SysmgrDebuggerService::service()->createPixPagerDebugger(lshandle,message,user_data) : false);
}

bool _test0(LSHandle* lshandle,LSMessage *message, void *user_data)
{
	const char* str = LSMessageGetPayload(message);
	if( !str )
		return false;

	QtJsonAbstract * j = QtJsonAbstract::jsonParse(QString(str));
	if (j)
	{
		QList<QPair<QString,QVariant> > allContents = j->extractAll();
		for (QtJsonAbstract::ContentsListIterator it = allContents.begin();
				it != allContents.end();++it)
		{
			//qDebug() << "key = " << it->first;
//			if ((QMetaType::Type)(it->second.userType()) == qMetaTypeId<QtJsonAbstractVariant>())
				//qDebug() << "\tval = " << it->second.value<QtJsonAbstractVariant>();
//			else
				//qDebug() << "\tval = " << it->second << " , usertype =  " << (QMetaType::Type)(it->second.userType()) << " , mytype = " << qMetaTypeId<QtJsonAbstractVariant>();
		}
		QtJsonAbstract::throwAway(j);

		QtJsonAbstract * reply = QtJsonAbstract::create();
		reply->add("returnValue",true);

		LSError lserror;
		LSErrorInit(&lserror);

		if (!LSMessageReply( lshandle, message,ccstr(reply->toString()), &lserror )) {
			LSErrorPrint (&lserror, stderr);
			LSErrorFree(&lserror);
		}
		QtJsonAbstract::throwAway(reply);
	}
	else
	{
		QtJsonAbstract * reply = QtJsonAbstract::create();
		reply->add("returnValue",true);

		LSError lserror;
		LSErrorInit(&lserror);

		if (!LSMessageReply( lshandle, message,ccstr(reply->toString()), &lserror )) {
			LSErrorPrint (&lserror, stderr);
			LSErrorFree(&lserror);
		}
		QtJsonAbstract::throwAway(reply);
	}

	return true;
}

static LSMethod s_public_methods[]  = {
		{ "test0", _test0 },
		{ "createPixPagerDebugger", _createPixPagerDebugger },
		{ 0, 0 }
};

static LSMethod s_private_methods[] = {};

//static
SysmgrDebuggerService * SysmgrDebuggerService::service()
{
	if (s_qp_instance.isNull())
	{
		s_qp_instance = new SysmgrDebuggerService();
		if (s_qp_instance->startup() == false)
		{
			//get rid of the instance, it didn't start up correctly
			delete s_qp_instance;
		}
	}
	return s_qp_instance;
}

SysmgrDebuggerService::~SysmgrDebuggerService()
{
	(void)shutdown();
}

bool SysmgrDebuggerService::startup()
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result;

	if (m_p_service)
	{
		qWarning() << __FUNCTION__ << " will exit since it seems the service handle is already active; shutdown first" ;
		return true;
	}

	GMainLoop *mainLoop = HostBase::instance()->mainLoop();

	result = LSRegisterPalmService("com.palm.sysmgrdebugger", &m_p_service, &lserror);
	if (!result)
	{
		qWarning() << __FUNCTION__ << " failed" ;
		LSErrorFree(&lserror);
		m_p_service = 0;
		return false;
	}

	result = LSPalmServiceRegisterCategory( m_p_service, "/", s_public_methods, s_private_methods,
			NULL, NULL, &lserror);
	if (!result)
	{
		qWarning() << __FUNCTION__ << " failed" ;
		LSErrorFree(&lserror);
		(void)shutdown();
		return false;
	}

	result = LSGmainAttachPalmService(m_p_service, mainLoop, &lserror);
	if (!result)
	{
		qWarning() << __FUNCTION__ << " failed" ;
		LSErrorFree(&lserror);
		(void)shutdown();
		return false;
	}

	//qDebug() << __FUNCTION__ << " Ok" ;
	return true;
}

bool SysmgrDebuggerService::shutdown()
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result;

	result = LSUnregisterPalmService(m_p_service, &lserror);
	if (!result)
	{
		qWarning() << __FUNCTION__ << " failed - the service handle may have just been leaked" ;
		LSErrorFree(&lserror);
	}
	m_p_service = 0;
	return result;
}

bool SysmgrDebuggerService::createPixPagerDebugger(LSHandle* p_lshandle,LSMessage *p_message, void * p_userdata)
{
	if (!m_qp_pixPagerDebugger.isNull())
	{

	}
	return true;
}

