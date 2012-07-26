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




#ifndef JSSYSOBJECTANIMATIONRUNNER_H
#define JSSYSOBJECTANIMATIONRUNNER_H

#include "Common.h"

#include <string>
#include <glib.h>

#include <npupp.h>
#include <npapi.h>

class CardWebApp;

class JsSysObjectAnimationRunner
{
public:

	static JsSysObjectAnimationRunner* instance();

	void run(CardWebApp* app, NPObject* domObj,
			 NPNetscapeFuncs* browserFuncs, NPP npp,
			 NPIdentifier onStepCallbackId,
			 NPIdentifier onCompleteCallbackId,
			 const std::string& animationCurve,
			 double duration, double initialValue, double finalValue);

private:

	enum AnimationType {
		Linear,
		EaseIn,
		EaseOut,
		EaseInOut
	};

	JsSysObjectAnimationRunner();
	~JsSysObjectAnimationRunner();

	void runLoop();
	bool timerTicked();
	static gboolean sourceCallback(gpointer arg);	

private:

	GMainContext* m_mainCtxt;
	GMainLoop* m_mainLoop;

	CardWebApp* m_app;
	NPObject* m_domObj;
	NPNetscapeFuncs* m_browserFuncs;
	NPP m_npp;
	NPIdentifier m_onStepCallbackId;
	NPIdentifier m_onCompleteCallbackId;
	std::string m_animationCurve;
	double m_duration;
	double m_initialValue;
	double m_finalValue;

	AnimationType m_animationType;
	double m_currentValue;
	int m_numSteps;
	int m_currStep;
};

#endif /* JSSYSOBJECTANIMATIONRUNNER_H */
