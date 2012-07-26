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




#include <QTimer>

#include "AppDirectRenderingArbitrator.h"
#include "HostBase.h"
#include "SystemUiController.h"

static AppDirectRenderingArbitrator* s_instance = 0;

AppDirectRenderingArbitrator::AppDirectRenderingArbitrator()
{
    s_instance = this;

	m_suspended = false;

	m_layerTurnOffTimer = new QTimer;
	m_layerTurnOffTimer->setSingleShot(true);
	m_layerTurnOffTimer->setInterval(500);
	connect(m_layerTurnOffTimer, SIGNAL(timeout()), SLOT(slotLayerTurnOffTimerTimeout()));
}

void AppDirectRenderingArbitrator::setLayerEnabled(void* client, bool enabled)
{
	if (!s_instance)
		new AppDirectRenderingArbitrator();

	s_instance->layerEnabled(client, enabled);
}

void AppDirectRenderingArbitrator::layerEnabled(void* client, bool enabled)
{
	g_message("AppDirectRenderingArbitrator::layerEnabled %s",
			  enabled ? "true" : "false");
	
	if (enabled) {

		if (m_clients.contains(client))
			return;

		m_clients.insert(client);
		m_layerTurnOffTimer->stop();
		if (!m_suspended)
			HostBase::instance()->setAppDirectRenderingLayerEnabled(true);
	}
	else {

		if (!m_clients.contains(client))
			return;

		m_clients.remove(client);
		if (m_clients.isEmpty() && !SystemUiController::instance()->isUiRotating())
			m_layerTurnOffTimer->start();
	}

	g_message("AppDirectRenderingArbitrator::layerEnabled enableCount: %d",
			  m_clients.size());
}

void AppDirectRenderingArbitrator::slotLayerTurnOffTimerTimeout()
{
	HostBase::instance()->setAppDirectRenderingLayerEnabled(false);

	if (m_suspended)
		HostBase::instance()->setRenderingLayerEnabled(false);		
}

void AppDirectRenderingArbitrator::suspend()
{
	if (!s_instance)
		new AppDirectRenderingArbitrator();

	s_instance->suspendLayer();
}

void AppDirectRenderingArbitrator::resume()
{
	if (!s_instance)
		new AppDirectRenderingArbitrator();

	s_instance->resumeLayer();
}

void AppDirectRenderingArbitrator::suspendLayer()
{
	if (m_suspended)
		return;

	g_message("%s", __PRETTY_FUNCTION__);

	m_suspended = true;
	m_layerTurnOffTimer->stop();
	m_layerTurnOffTimer->start();
}

void AppDirectRenderingArbitrator::resumeLayer()
{
	if (!m_suspended)
		return;

	g_message("%s", __PRETTY_FUNCTION__);
	
    m_suspended = false;

	HostBase::instance()->setRenderingLayerEnabled(true);		
	
	if (!m_clients.isEmpty()) {
		m_layerTurnOffTimer->stop();		
		HostBase::instance()->setAppDirectRenderingLayerEnabled(true);
	}
}
