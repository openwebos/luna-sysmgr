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




#include "Common.h"

#include <QPainter>

#include "DockWebApp.h"
#include "Logging.h"
#include "Settings.h"
#include "WebAppManager.h"
#include "WebAppFactory.h"
#include "SysMgrWebBridge.h"
#include "Window.h"
#include "WindowContentTransitionRunner.h"
#include "Time.h"
#include "RemoteWindowData.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <glib.h>
#include <string>
#include <PIpcBuffer.h>
#include <PIpcChannel.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>


DockWebApp::DockWebApp(Window::Type winType, PIpcChannel *channel)
	: CardWebApp(winType, channel)
{
	
	m_appBufWidth = m_width;
	m_appBufHeight = m_height;
	
	m_windowWidth = m_width;
	m_windowHeight = m_height;
	
	m_setWindowWidth = m_windowWidth;
	m_setWindowHeight = m_windowHeight;
	
	m_enableFullScreen = false;	
	
	init();
	
	setOrientation(WebAppManager::instance()->orientation());
}

DockWebApp::~DockWebApp()
{
}

void DockWebApp::setVisibleDimensions(int width, int height)
{
	// override this so we are always full screen
	m_channel->sendAsyncMessage(new ViewHost_SetVisibleDimensions(routingId(), m_width, m_height));
}

void DockWebApp::resizeWindowForOrientation(Event::Orientation orient)
{
/*
	if (!m_allowsOrientationChange)
		return;

	if (m_fixedOrientation != Event::Orientation_Invalid) {
		// already there?
		if (m_orientation == m_fixedOrientation)
			return;

		// this should already be true. but won't hurt
		orient = m_fixedOrientation;
	}

	if (orient == m_orientation)
		return;

	Event::Orientation oldOrientation = m_orientation;
	m_orientation = orient;
	
	switch (orient) {
	case (Event::Orientation_Left):
	case (Event::Orientation_Right): {
		// Full screen in this mode
		m_windowWidth = m_height;
		m_windowHeight = m_width - Settings::LunaSettings()->positiveSpaceTopPadding;
		m_appBufWidth = m_height;
		m_appBufHeight = m_width;
		break;
	}
	case (Event::Orientation_Down):
	case (Event::Orientation_Up):
	default: {
		m_windowWidth = m_width;
		m_windowHeight = m_height - Settings::LunaSettings()->positiveSpaceTopPadding;
		m_appBufWidth = m_width;
		m_appBufHeight = m_height;
		break;
	}
	}

	m_page->webkitView()->resize(m_windowWidth, m_windowHeight);

	if (m_orientation == Event::Orientation_Up && !m_enableFullScreen) {
		setVisibleDimensions(m_width, m_height - Settings::LunaSettings()->positiveSpaceTopPadding);
	}
	else {
		setVisibleDimensions(m_width, m_height);
	}	
	

	// Force a full paint onto a temporary surface
	PGSurface* toSceneSurface = PGSurface::create(m_width, m_height, false);
	PGContext* toSceneContext = PGContext::create();
	toSceneContext->setSurface(toSceneSurface);

	// Force a full app repaint (not clipped to window dimensions)
	int tx = 0;
	int ty = 0;
	int tw = m_appBufWidth;
	int th = m_appBufHeight;
	m_paintRect.setRect(tx, ty, tw, th);
	paint(toSceneContext, toSceneSurface,
		  tx, ty, tw, th, tx, ty, tw, th);

	// Draw rounded corners only if window is not a full-screen window
	if ((m_windowWidth * m_windowHeight) < uint32_t(m_width * m_height))
		drawCornerWindows(toSceneContext);
	
	// Now we can kick off the animation
	int currAngleForAnim = angleForOrientation(oldOrientation) - angleForOrientation(m_orientation);
	if (currAngleForAnim > 180)
		currAngleForAnim = -90;
	else if (currAngleForAnim < -180)
		currAngleForAnim = 90;
	int targAngleForAnim = 0;

	updateWindowProperties();

	PGContext* gc = m_data->renderingContext();
	
	WindowContentTransitionRunner::instance()->runRotateTransition(this,
																   toSceneSurface,
																   gc,
																   currAngleForAnim,
																   targAngleForAnim);

	toSceneSurface->releaseRef();
	toSceneContext->releaseRef();
	
	animationFinished();
*/
}






