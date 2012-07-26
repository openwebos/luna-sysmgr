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




#ifndef __CORENAVIMANAGER_H__
#define __CORENAVIMANAGER_H__

#include "Common.h"

#include "HostBase.h"
#include "CoreNaviLeds.h"
#include "DisplayManager.h"
#include "SystemUiController.h"
#include "AmbientLightSensor.h"

#include <QObject>
#include <QEvent>

class CoreNaviManager : public QObject
{
	Q_OBJECT

public:

	static CoreNaviManager* instance ();	

	~CoreNaviManager();

	bool handleEvent(sptr<Event> e);
	bool handleEvent(QEvent *event);

	/* these are for led patterns when the display is off */
	void	addStandbyRequest (const std::string& appId, const std::string& requestId);
	void	removeStandbyRequest (const std::string& appId, const std::string& requestId);
	void	clearStandbyRequests (const std::string& appId);
	void	clearAllStandbyRequests ();
	int		numStandbyRequests() const;

	void    updateBrightness (int brightness);
	void	setSubtleLightbar (bool enable);

	bool    isSubtleLightbar() const { return m_isSubtleLightbar; }

	/* timer function */
	bool    restoreLightbar();
	bool    seesawLightbar();
	bool    swipeLeftLightbar();
	bool    swipeRightLightbar();

private Q_SLOTS:

	void    slotDisplayStateChange (int displayState);
	void    slotLockStateChange (int lockState, int displayEvent);
	void    slotCardWindowMinimized();
	void    slotCardWindowMaximized();
	void	slotFocusMaximizedCardWindow(bool enable);

private:

	CoreNaviManager();

	static	CoreNaviManager* m_instance; /* singleton instance */

	void setMetaGlow(bool enable);
	bool useLightbar();

	void renderGesture(int key);
	void renderGestureOnLightbar(int key);

	bool m_centerGlow;
	bool m_allGlow;
	int  m_brightness;
	bool m_isSubtleLightbar;
	int  m_subtleModeBrightness;
	int  m_animationSpeed;

	bool m_metaKeyDown;
	Timer<CoreNaviManager> m_lightbarRestoreTimer;
	Timer<CoreNaviManager> m_lightbarHoldTimer;
	Timer<CoreNaviManager> m_lightbarSwipeLeftTimer;
	Timer<CoreNaviManager> m_lightbarSwipeRightTimer;

	CoreNaviLeds*		m_leds;
	DisplayManager*		m_displayManager;
	AmbientLightSensor*	m_ambientLightSensor;
	SystemUiController* m_systemUiController;

	bool	m_hasLightBar; // this means no core navi button
	int		m_lightbarMaxBrightness;
	int		m_lightbarDimBrightness;
	int		m_lightbarDarkBrightness;

	void    lightbarOn (int brightness = -1);
	void    lightbarOff();

	// throbber related functionality
	struct LedRequest {
		std::string appId;
		std::string requestId;
		// const int priority; 

		LedRequest (const std::string& appId, const std::string& requestId);
	};

	void	stopStandbyLeds();
	void 	startStandbyLeds();

	typedef std::list<LedRequest> LedRequestList;
	LedRequestList	m_ledRequestList;

	bool m_isStandbyLedsActive;

	int		m_throbberBrightness;
	float	m_throbberDimBrightness;
};

#endif

