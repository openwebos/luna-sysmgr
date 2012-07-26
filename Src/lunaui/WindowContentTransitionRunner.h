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




#ifndef WINDOWCONTENTTRANSITIONRUNNER_H
#define WINDOWCONTENTTRANSITIONRUNNER_H

#include "Common.h"

#include <glib.h>

class CardWebApp;
class PGSurface;
class PGContext;
class PIpcChannel;
class PIpcBuffer;

class WindowContentTransitionRunner
{
public:

	static WindowContentTransitionRunner* instance();

	void runRotateTransition(CardWebApp* app,
							 PGSurface* toSurface,
							 PGContext* dstContext,
							 int currAngle,
							 int targAngle);							 

private:

	WindowContentTransitionRunner();
	~WindowContentTransitionRunner();

	void runLoop();
	bool timerTicked();
	static gboolean sourceCallback(gpointer arg);

	bool rotateTick();
	
private:

	enum TransitionType {
		Transition_Invalid,
		Transition_Rotate
	};

	struct AnimObject {
		AnimObject() : currX(0), currY(0), currR(0), currB(0), currA(0),
					   targX(0), targY(0), targR(0), targB(0), targA(0) {}
		int currX, currY, currR, currB, currA;
		int targX, targY, targR, targB, targA;
	};
		
	GMainLoop* m_mainLoop;
	GMainContext* m_mainCtxt;

	TransitionType m_transitionType;

	CardWebApp* m_app;
	PIpcChannel* m_channel;
	PIpcBuffer* m_ipcBuffer;
	PGContext* m_dstContext;
	
	AnimObject m_fromSceneAnimObject;
	AnimObject m_toSceneAnimObject;

	PGSurface* m_fromSceneSurface;
	PGSurface* m_toSceneSurface;
	bool m_transitionIsPop;

	int m_currRotateAngle;
	int m_targRotateAngle;
};

#endif // WINDOWCONTENTTRANSITIONRUNNER_H
