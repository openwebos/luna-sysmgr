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

#include "HostBase.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(TARGET_DESKTOP)
	#include "HostQtDesktop.h"
#elif defined(TARGET_DEVICE)
	#include "HostArm.cpp"
	#ifdef MACHINE_CASTLE
		#include "HostArmCastle.cpp"
	#elif defined(MACHINE_PIXIE)
		#include "HostArmPixie.cpp"
	#elif defined(MACHINE_WINDSOR)
		#include "HostArmWindsor.cpp"
	#elif defined(MACHINE_BROADWAY)
		#include "HostArmBroadway.cpp"
	#elif defined(MACHINE_MANTARAY)
		#include "HostArmMantaray.cpp"
	#elif defined(MACHINE_CHILE)
		#include "HostArmChile.cpp"
	#elif defined(MACHINE_TOPAZ)
		#include "HostArmTopaz.cpp"
	#elif defined(MACHINE_OPAL)
		#include "HostArmOpal.cpp"
	#elif defined(MACHINE_WINDSORNOT)
		#include "HostArmWindsorNot.cpp"
	#else
		#include "HostArmUnknown.cpp"
	#endif
#elif defined(TARGET_EMULATOR)
	#include "HostArm.cpp"
	#include "HostArmQemu.cpp"
#endif

static HostBase* sInstance = 0;


bool HostBase::hostIsQemu() {

	FILE* f = ::fopen("/proc/cpuinfo", "r");
	if (!f)
		return false;

	bool isQemu = false;

	char* line = 0;
	size_t lineLen = 0;
	ssize_t readLen;

	while ((readLen = ::getline(&line, &lineLen, f)) != -1) {
		if (strstr(line, "ARM-Versatile") != 0) {
			isQemu = true;
			printf("Host Platform is QEMU\n");
			break;
		}
	}

	if (line)
		free(line);

	fclose(f);

	return isQemu;
}

HostBase* HostBase::instance()
{
    if (!sInstance) {
#if defined(TARGET_DESKTOP)
		new HostQtDesktop;
#elif defined(TARGET_EMULATOR)
		new HostArmQemu;
#elif defined(TARGET_DEVICE)
	#ifdef MACHINE_CASTLE
		new HostArmCastle;
	#elif defined(MACHINE_PIXIE)
		new HostArmPixie;
	#elif defined (MACHINE_WINDSOR)
		new HostArmWindsor;
	#elif defined(MACHINE_BROADWAY)
		new HostArmBroadway;
	#elif defined (MACHINE_MANTARAY)
		new HostArmMantaray;
	#elif defined(MACHINE_CHILE)
		new HostArmChile;
	#elif defined(MACHINE_TOPAZ)
		new HostArmTopaz;
	#elif defined(MACHINE_OPAL)
		new HostArmOpal;
	#elif defined(MACHINE_WINDSORNOT)
		new HostArmWindsorNot;
	#else
		new HostArmUnknown;
	#endif
#endif
	}

	return sInstance;
}

HostBase::HostBase() 
	: m_metaKeyDown(false)
    , m_orientation(OrientationEvent::Orientation_Up)
	, m_turboModeSubscriptions(0)
{
	sInstance = this;
	memset(&m_info, 0, sizeof(HostInfo));

	m_mainCtxt = g_main_context_default();

	// This mainloop is a dummy. It's sole purpose is to provide a mechanism to get to
	// the GMainContext for compatibility with luna-service and other glib based 3rd party
	// libraries
	m_mainLoop = g_main_loop_new(m_mainCtxt, TRUE);

	m_masterTimer = new SingletonTimer(m_mainLoop);
}

HostBase::~HostBase()
{
	quit();

	g_main_loop_unref(m_mainLoop);
	g_main_context_unref(m_mainCtxt);

    sInstance = 0;
}

void HostBase::run()
{
}

void HostBase::quit()
{
}

unsigned short HostBase::translateKeyWithMeta( unsigned short key,
												bool /*withShift*/, bool /*withAlt*/ )
{
	return key;
}

bool HostBase::hasAltKey(Qt::KeyboardModifiers modifiers)
{
	return modifiers & Qt::AltModifier;
}

void HostBase::setOrientation(OrientationEvent::Orientation o)
{
	switch (o) {
    case OrientationEvent::Orientation_Up:
		m_orientation = o;
		m_trans.reset();
		m_trans.rotate(0);
		break;
    case OrientationEvent::Orientation_Down:
		m_orientation = o;
		m_trans.reset();
		m_trans.rotate(180);
		break;
    case OrientationEvent::Orientation_Right:
		m_orientation = o;
		m_trans.reset();
        m_trans.rotate(90);
		break;
    case OrientationEvent::Orientation_Left:
		m_orientation = o;
		m_trans.reset();
        m_trans.rotate(270);
		break;
	default:
		break;
	}
}

void HostBase::turboModeSubscription(bool add)
{
	if (add) {
		m_turboModeSubscriptions++;
		if (m_turboModeSubscriptions == 1)
			turboMode(true);
	}
	else {
		m_turboModeSubscriptions--;
		if (m_turboModeSubscriptions == 0)
			turboMode(false);
	}
}
