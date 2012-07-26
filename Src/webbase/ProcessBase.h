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




#ifndef __ProcessBase_h__
#define __ProcessBase_h__

#include "Common.h"

#include <QString>


class ProcessBase
{
	public:
		ProcessBase() { }
		virtual ~ProcessBase() { }
		
		virtual void		setProcessId( const QString& inId ) { m_procId=inId; }
		const QString&	processId() const { return m_procId; }
		virtual void		setAppId( const QString& inId ) { m_appId=inId; }
		const QString&	appId() const { return m_appId; }
		void				setLaunchingAppId(const QString& id) { m_launchingAppId=id; }
		const QString&  launchingAppId() const { return m_launchingAppId; }
		void 				setLaunchingProcessId(const QString& id) { m_launchingProcId = id; }
		const QString&	launchingProcessId() const { return m_launchingProcId; }
	
	private:	
		QString			m_procId;
		QString			m_appId;
		QString			m_launchingAppId;
		QString 		m_launchingProcId;
};


#endif

