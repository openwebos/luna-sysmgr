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




#ifndef JSSYSOBJECT_H
#define JSSYSOBJECT_H

#include "Common.h"

#include <string>
#include <map>

#include <npupp.h>
#include <npapi.h>

class WebFrame;

class JsSysObject : public NPObject
{
public:

	JsSysObject(NPP npp);
	~JsSysObject();

	void release();

	void setLaunchParams(const std::string& params);
	
public:
	
	void setBrowserFuncs(NPNetscapeFuncs* browserFuncs);
	void setFrame(WebFrame* frame);

	void invalidate();
	bool hasMethod(NPIdentifier name);
	bool invoke(NPIdentifier name, const NPVariant *args, uint32_t argCount, NPVariant *result);
	bool invokeDefault(const NPVariant *args, uint32_t argCount, NPVariant *result);
	bool hasProperty(NPIdentifier name);
	bool getProperty(NPIdentifier name, NPVariant *result);
	bool setProperty(NPIdentifier name,  const NPVariant *value);
	bool removeProperty(NPIdentifier name);
	bool enumerate(NPIdentifier **value, uint32_t *count);
	bool construct(const NPVariant *args, uint32_t argCount, NPVariant *result);

private:

	bool propLaunchParams(NPVariant *result);
	bool propHasAlphaHole(NPVariant *result);
	bool propLocale(NPVariant *result);
	bool propLocaleRegion(NPVariant* result);
	bool propPhoneRegion(NPVariant* result);
	bool propTimeFormat(NPVariant* result);
	bool propTimeZone(NPVariant* result);
	bool propIsMinimal(NPVariant* result);
	bool propIdentifier(NPVariant* result);
	bool propVersion(NPVariant* result);
	bool propScreenOrientation(NPVariant* result);
	bool propWindowOrientation(NPVariant* result);
	bool propSpecifiedWindowOrientation(NPVariant* result);
	bool propVideoOrientation(NPVariant* result);
	bool propDeviceInfo(NPVariant* result);
	bool propIsActivated(NPVariant* result);
	bool propActivityId(NPVariant* result);

	bool setPropHasAlphaHole(const NPVariant *value);
	bool setPropWindowOrientation(const NPVariant* value);
	
	bool methodAddBannerMessage(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodRemoveBannerMessage(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodClearBannerMessages(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodPlaySoundNotification(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodSimulateMouseClick(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodPaste(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodCopiedToClipboard(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodPastedFromClipboard(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodSetWindowOrientation(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodRunTextIndexer( const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodPrepareSceneTransition( const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodRunSceneTransition(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodCancelSceneTransition(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodRunCrossAppTransition(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodCrossAppSceneActive(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodCancelCrossAppScene(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodEncrypt(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodDecrypt(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodMarkFirstUseDone(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodShutdown(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodEnableFullScreenMode(const NPVariant* args, uint32_t argCount, NPVariant *result);
	bool methodActivate(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodDeactivate(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodStagePreparing(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodStageReady(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodSetAlertSound(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodReceivePageUpDownInLandscape(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodShow(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodHide(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodEnableDockMode(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodGetLocalizedString(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodAddNewContentIndicator(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodRemoveNewContentIndicator(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodRunAnimationLoop(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodSetActiveBannerWindowWidth(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodCancelVibrations(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodSetWindowProperties(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodAddActiveCallBanner(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodRemoveActiveCallBanner(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodUpdateActiveCallBanner(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodApplyLaunchFeedback(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodLauncherReady(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodGetDeviceKeys(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodRepaint(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodHideSpellingWidget(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodPrintFrame(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodEditorFocused(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodAllowResizeOnPositiveSpaceChange(const NPVariant* args, uint32_t argCount, NPVariant* result);
	bool methodKeepAlive(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool methodUseSimulatedMouseClicks(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool methodHandleTapAndHoldEvent(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool methodSetManualKeyboardEnabled(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool methodKeyboardShow(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool methodKeyboardHide(const NPVariant* args, uint32_t argCount, NPVariant* result);

private:
	
	typedef bool (JsSysObject::*getPropPtr)(NPVariant* result);
	typedef bool (JsSysObject::*setPropPtr)(const NPVariant* value);
	typedef bool (JsSysObject::*methodPtr)(const NPVariant* args, uint32_t argCount, NPVariant* result);

	typedef std::map<NPIdentifier, getPropPtr> GetPropPtrMap;
	typedef std::map<NPIdentifier, setPropPtr> SetPropPtrMap;
	typedef std::map<NPIdentifier, methodPtr>  MethodPtrMap;

	void markFirstUseDone();

	NPP              m_npp;
	NPNetscapeFuncs* m_browserFuncs;

	GetPropPtrMap    m_getPropPtrMap;
	SetPropPtrMap    m_setPropPtrMap;
	MethodPtrMap     m_methodPtrMap;

	WebFrame*        m_frame;
	std::string      m_launchParams;
	std::string      m_specifiedWindowOrientation;
};



#endif /* JSSYSOBJECT_H */
