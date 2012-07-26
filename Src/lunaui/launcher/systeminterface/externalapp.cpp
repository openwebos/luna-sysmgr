/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#include  "externalapp.h"
#include <glib.h>

namespace DimensionsSystemInterface
{

ExternalApp::ExternalApp(ExternalAppType::Enum type)
: QObject()
, m_type(type)
, m_stateBeingRemoved(false)
, m_stateBeingUpdated(false)
, m_stateFailed(false)
{
	m_uid = QUuid::createUuid();
}

//virtual
ExternalApp::~ExternalApp()
{
}

//virtual
QUuid ExternalApp::uid() const
{
	return m_uid;
}

//virtual
ExternalAppType::Enum ExternalApp::type() const
{
	return m_type;
}

//virtual
bool ExternalApp::isUpdating() const
{
	return m_stateBeingUpdated;
}

//virtual
bool ExternalApp::isInRemoval() const
{
	return m_stateBeingRemoved;
}

//virtual
bool ExternalApp::isReady() const
{
	//meaning, ok to launch, etc
//	bool v = !(m_stateBeingUpdated || m_stateBeingRemoved || m_stateFailed);
//	g_warning("%s: updated = %s , removed = %s , failed = %s --> %s",__FUNCTION__,
//			( m_stateBeingUpdated ? "true" : "false") ,( m_stateBeingRemoved  ? "true" : "false"), ( m_stateFailed ? "true" : "false"), (v ? "true" : "false"));
	return !(m_stateBeingUpdated || m_stateBeingRemoved || m_stateFailed);
}

//virtual
void ExternalApp::setReady()
{
	m_stateBeingUpdated = false;
	m_stateBeingRemoved = false;
	m_stateFailed = false;
}

//virtual
bool ExternalApp::isFailed() const
{
	return m_stateFailed;
}

} //end namespace
