/**
 * @file
 * 
 * Holds information about processes launched by LunaSysMgr
 *
 * @author Hewlett-Packard Development Company, L.P.
 * @author tyrok1
 *
 * @section LICENSE
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
 */




#ifndef __ProcessBase_h__
#define __ProcessBase_h__

#include "Common.h"

#include <QString>

/**
 * Holds information about a process launched by LunaSysMgr
 * 
 * This is very close to being just a structure.  There is no
 * additional functionality in this class other than the storage
 * and retrieval of information about a process.
 */
class ProcessBase
{
	public:
		/**
		 * Construct a ProcessBase
		 * 
		 * Make sure you explicitly set all relevant
		 * information about this process.  This
		 * constructor does not zero it out.
		 * 
		 * @todo Initialize variables to prevent invalid values and reading of previously-stored-to memory.
		 */
		ProcessBase() { }
		
		/**
		 * Clean up
		 * 
		 * Doesn't do anything - just defined here
		 * so it is declared as virtual so the
		 * correct destructor is called for derived
		 * classes.
		 */
		virtual ~ProcessBase() { }
		
		/**
		 * Set the Process ID for the process associated with this instance
		 * 
		 * Just stores it - doesn't change anything
		 * else.
		 * 
		 * @param	inId			New process ID to store.
		 */
		virtual void		setProcessId( const QString& inId ) { m_procId=inId; }
		
		/**
		 * Get the previously-stored Process ID for the process associated with this instance
		 * 
		 * Just retrieves it - has no intelligence
		 * to pull the process ID from anywhere.
		 * 
		 * @note Make sure you've previously stored an process ID before attempting to retrieve it.
		 * 
		 * @return				Previously-stored process ID.
		 */
		const QString&	processId() const { return m_procId; }

		/**
		 * Set the app ID for the process associated with this instance
		 * 
		 * Just stores it - doesn't change anything
		 * else.
		 * 
		 * @param	inId			New app ID to store.
		 */
		virtual void		setAppId( const QString& inId ) { m_appId=inId; }

		/**
		 * Get the previously-stored app ID for the process associated with this instance
		 * 
		 * Just retrieves it - has no intelligence
		 * to pull the app ID from anywhere.
		 * 
		 * @note Make sure you've previously stored an app ID before attempting to retrieve it.
		 * 
		 * @return				Previously-stored app ID.
		 */
		const QString&	appId() const { return m_appId; }

		/**
		 * Set the app ID for the process that launched the one associated with this instance
		 * 
		 * Just stores it - doesn't change anything
		 * else.
		 * 
		 * @param	inId			New app ID to store.
		 */
		void				setLaunchingAppId(const QString& id) { m_launchingAppId=id; }

		/**
		 * Get the previously-stored app ID of the parent process for the process associated with this instance
		 * 
		 * Just retrieves it - has no intelligence
		 * to pull the app ID from anywhere.
		 * 
		 * @note Make sure you've previously stored an app ID before attempting to retrieve it.
		 * 
		 * @return				Previously-stored app ID.
		 */
		const QString&  launchingAppId() const { return m_launchingAppId; }

		/**
		 * Set the Process ID for the process which launched the one associated with this instance
		 * 
		 * Just stores it - doesn't change anything
		 * else.
		 * 
		 * @param	inId			New process ID to store.
		 */
		void 				setLaunchingProcessId(const QString& id) { m_launchingProcId = id; }

		/**
		 * Get the previously-stored Process ID for the process that launched the one associated with this instance
		 * 
		 * Just retrieves it - has no intelligence
		 * to pull the process ID from anywhere.
		 * 
		 * @note Make sure you've previously stored an process ID before attempting to retrieve it.
		 * 
		 * @return				Previously-stored process ID.
		 */
		const QString&	launchingProcessId() const { return m_launchingProcId; }
	
	private:	
		/**
		 * Process ID of the process associated with this class instance
		 */
		QString			m_procId;

		/**
		 * App ID of the process associated with this class instance
		 */
		QString			m_appId;

		/**
		 * App ID of the process that launched the one associated with this class instance
		 */
		QString			m_launchingAppId;
		
		/**
		 * Process ID of the process that launched the one associated with this class instance
		 */
		QString 		m_launchingProcId;
};


#endif

