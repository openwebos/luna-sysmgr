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




/* ========================================================================================
 * Easing equations from Robert Penner. Copyright notice follows:
 * 
 * TERMS OF USE - EASING EQUATIONS
 *
 * Open source under the BSD License.
 *
 * Copyright © 2001 Robert Penner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the names of contributors may be used
 *     to endorse or promote products derived from this software without specific
 *     prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 *  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 *  OF SUCH DAMAGE.
 * ======================================================================================== */

#include "Common.h"

#include "JsSysObjectAnimationRunner.h"

#include "CardWebApp.h"

static const int kStepIntervalMs = 50;
static const double kMaxDurationSec = 1.0;

JsSysObjectAnimationRunner* JsSysObjectAnimationRunner::instance()
{
	static JsSysObjectAnimationRunner* s_instance = 0;
	if (G_UNLIKELY(s_instance == 0))
		s_instance = new JsSysObjectAnimationRunner;

	return s_instance;    
}

JsSysObjectAnimationRunner::JsSysObjectAnimationRunner()
	: m_mainCtxt(0)
	, m_mainLoop(0)
	, m_app(0)
	, m_domObj(0)
	, m_browserFuncs(0)
{
	m_mainCtxt = g_main_context_new();
	m_mainLoop = g_main_loop_new(m_mainCtxt, TRUE);    
}

JsSysObjectAnimationRunner::~JsSysObjectAnimationRunner()
{
    // not reached
}

void JsSysObjectAnimationRunner::run(CardWebApp* app, NPObject* domObj,
									 NPNetscapeFuncs* browserFuncs, NPP npp,
									 NPIdentifier onStepCallbackId,
									 NPIdentifier onCompleteCallbackId,
									 const std::string& animationCurve,
									 double duration, double initialValue, double finalValue)
{
	// Recursion catcher
	static bool s_alreadyHere = false;
	if (s_alreadyHere)
		return;

	s_alreadyHere = true;
	
    m_app = app;
	m_domObj = domObj;
	m_browserFuncs = browserFuncs;
	m_npp = npp;
	m_onStepCallbackId = onStepCallbackId;
	m_onCompleteCallbackId = onCompleteCallbackId;
	m_animationCurve = animationCurve;
	m_duration = MIN(duration, kMaxDurationSec);
	m_initialValue = initialValue;
	m_finalValue = finalValue;

	m_currentValue = initialValue;
	m_numSteps = (int) ((m_duration * 1000) / kStepIntervalMs);
	m_currStep = 0;

	if (m_animationCurve == "easeIn")
		m_animationType = EaseIn;
	else if (m_animationCurve == "easeOut")
		m_animationType = EaseOut;
	else if (m_animationCurve == "easeInOut")
		m_animationType = EaseInOut;
	else
		m_animationType = Linear;

	runLoop();

	NPVariant jsResult;
	bool r = m_browserFuncs->invoke(m_npp, m_domObj, onCompleteCallbackId, 0, 0, &jsResult);
	if (r)
		m_browserFuncs->releasevariantvalue(&jsResult);
	
	m_app = 0;
	m_domObj = 0;
	m_browserFuncs = 0;

	s_alreadyHere = false;
}

void JsSysObjectAnimationRunner::runLoop()
{
	GSource* src = g_timeout_source_new(kStepIntervalMs);
	g_source_set_callback(src, JsSysObjectAnimationRunner::sourceCallback, this, NULL);
	g_source_attach(src, m_mainCtxt);
	g_source_unref(src);

	g_main_loop_run(m_mainLoop);
}

gboolean JsSysObjectAnimationRunner::sourceCallback(gpointer arg)
{
	JsSysObjectAnimationRunner* runner = (JsSysObjectAnimationRunner*) arg;
	bool ret = runner->timerTicked();
	if (!ret)
		g_main_loop_quit(runner->m_mainLoop);

	return ret;    
}

static double easeIn(double time, double initial, double change, double duration)  {
	time /= duration;
	return change * time * time * time + initial;
}

static double easeOut(double time, double initial, double change, double duration)  {
	time = time / duration - 1;
	return change * ((time * time * time) + 1) + initial;
}

static double easeInOut(double time, double initial, double change, double duration)  {
	time /= duration / 2;
	if (time < 1)
		return change/2 * time * time * time + initial;
	else {
		time -= 2;
		return change / 2 * ((time * time * time) + 2) + initial;
	}
}

static double easeLinear(double time, double initial, double change, double duration)  {
	time /= duration;
	return change * time + initial;
}

bool JsSysObjectAnimationRunner::timerTicked()
{
	if (m_currStep >= m_numSteps)
		return false;

	//m_currentValue += (m_finalValue - m_initialValue) / m_numSteps;
	switch (m_animationType) {
	case (EaseIn):
		m_currentValue = easeIn(m_currStep, m_initialValue, (m_finalValue - m_initialValue),
								m_numSteps);
		break;
	case (EaseOut):
		m_currentValue = easeOut(m_currStep, m_initialValue, (m_finalValue - m_initialValue),
								m_numSteps);
		break;
	case (EaseInOut):
		m_currentValue = easeInOut(m_currStep, m_initialValue, (m_finalValue - m_initialValue),
								   m_numSteps);
		break;
	case (Linear):
	default:
		m_currStep = easeLinear(m_currStep, m_initialValue, (m_finalValue - m_initialValue),
								m_numSteps);
		break;
	}

	m_currStep++;
	
	NPVariant jsResult;
	NPVariant arg;
	DOUBLE_TO_NPVARIANT(m_currentValue, arg);
	
	bool r = m_browserFuncs->invoke(m_npp, m_domObj, m_onStepCallbackId, &arg, 1, &jsResult);	
	if (r) {
		m_browserFuncs->releasevariantvalue(&jsResult);
	}

	m_app->invalidate();
	m_app->paint(true);
	
	return true; 
}
