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

#include <stdio.h>
#include <strings.h>
#include <string>
#include <sys/ipc.h>
#include <cjson/json.h>

#include <palmwebview.h>
#include <palmwebglobal.h>
#include <palmwebpage.h>

#include "AlertWebApp.h"
#include "CardWebApp.h"
#include "DeviceInfo.h"
#include "Event.h"
#include "KeyboardMapping.h"
#include "JsSysObject.h"
#include "JsSysObjectAnimationRunner.h"
#include "Localization.h"
#include "Logging.h"
#include "Preferences.h"
#include "Settings.h"
#include "Utils.h"
#include "WebAppFactory.h"
#include "WebAppManager.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebPageClient.h"
#include "Window.h"
#include "Time.h"
#include "BannerMessageEventFactory.h"
#include "NewContentIndicatorEventFactory.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include <openssl/blowfish.h>

#if defined(HAS_NYX)
#include "NyxSensorConnector.h"
static NYXOrientationSensorConnector* sOrientationSensor = 0;
#endif

static const char* sLogChannel = "JsSysObject";

static bool s_initialized = false;

enum PropId {
	PropLaunchParams = 0,
	PropHasAlphaHole,
	PropLocale,
	PropLocaleRegion,
	PropTimeFormat,
	PropTimeZone,
	PropIsMinimal,
	PropIdentifier,
	PropVersion,
	PropScreenOrientation,
	PropWindowOrientation,
	PropSpecifiedWindowOrientation,
	PropVideoOrientation,
	PropDeviceInfo,
	PropIsActivated,
	PropActivityId,
	PropPhoneRegion
};

enum MethodId {
	MethodAddBannerMessage = 0,
	MethodRemoveBannerMessage,
	MethodClearBannerMessages,
	MethodPlaySoundNotification,
	MethodSimulateMouseClick,
	MethodPaste,
	MethodCopiedToClipboard,
	MethodPastedFromClipboard,
	MethodSetWindowOrientation,
	MethodRunTextIndexer,
	MethodPrepareSceneTransition,
	MethodRunSceneTransition,
	MethodCancelSceneTransition,
	MethodRunCrossAppTransition,
	MethodCrossAppSceneActive,
	MethodCancelCrossAppScene,
	MethodEncrypt,
	MethodDecrypt,
	MethodShutdown,
	MethodMarkFirstUseDone,
	MethodEnableFullScreenMode,
	MethodActivate,
	MethodDeactivate,
	MethodStagePreparing,
	MethodStageReady,
	MethodSetAlertSound,
	MethodReceivePageUpDownInLandscape,
	MethodShow,
	MethodHide,
	MethodEnableDockMode,
	MethodGetLocalizedString,
	MethodAddNewContentIndicator,
	MethodRemoveNewContentIndicator,
	MethodRunAnimationLoop,
	MethodSetActiveBannerWindowWidth,
	MethodCancelVibrations,
	MethodSetWindowProperties,
	MethodAddActiveCallBanner,
	MethodRemoveActiveCallBanner,
	MethodUpdateActiveCallBanner,
	MethodApplyLaunchFeedback,
	MethodLauncherReady,
	MethodGetDeviceKeys,
	MethodRepaint,
	MethodHideSpellingWidget,
	MethodPrintFrame,
	MethodEditorFocused,
	MethodAllowResizeOnPositiveSpaceChange,
	MethodKeepAlive,
    MethodUseSimulatedMouseClicks,
    MethodHandleTapAndHoldEvent,
    MethodSetManualKeyboardEnabled,
    MethodKeyboardShow,
    MethodKeyboardHide
};

static const char* s_propIdNames[] = {
	"launchParams",
	"hasAlphaHole",
	"locale",
	"localeRegion",
	"timeFormat",
	"timeZone",
	"isMinimal",
	"identifier",
	"version",
	"screenOrientation",
	"windowOrientation",
	"specifiedWindowOrientation",
	"videoOrientation",
	"deviceInfo",
	"isActivated",
	"activityId",
	"phoneRegion"
};

static const char* s_methodIdNames[] = {
	"addBannerMessage",
	"removeBannerMessage",
	"clearBannerMessages",
	"playSoundNotification",
	"simulateMouseClick",
	"paste",
	"copiedToClipboard",
	"pastedFromClipboard",
	"setWindowOrientation",
	"runTextIndexer",
	"prepareSceneTransition",
	"runSceneTransition",
	"cancelSceneTransition",
	"runCrossAppTransition",
	"crossAppSceneActive",
	"cancelCrossAppScene",
	"encrypt",
	"decrypt",
	"shutdown",
	"markFirstUseDone",
	"enableFullScreenMode",
	"activate",
	"deactivate",
	"stagePreparing",
	"stageReady",
	"setAlertSound",
	"receivePageUpDownInLandscape",
	"show",
	"hide",
	"enableDockMode",
	"getLocalizedString",
	"addNewContentIndicator",
	"removeNewContentIndicator",
	"runAnimationLoop",
	"setActiveBannerWindowWidth",
	"cancelVibrations",
	"setWindowProperties",
	"addActiveCallBanner",
	"removeActiveCallBanner",
	"updateActiveCallBanner",
	"applyLaunchFeedback",
	"launcherReady",
	"getDeviceKeys",
	"repaint",
	"hideSpellingWidget",
	"printFrame",
	"editorFocused",
	"allowResizeOnPositiveSpaceChange",
	"keepAlive",
    "useSimulatedMouseClicks",
    "handleTapAndHoldEvent",
    "setManualKeyboardEnabled",
    "keyboardShow",
    "keyboardHide"
};

static const int kNumProps = sizeof(s_propIdNames)/sizeof(const char*);
static NPIdentifier s_propIds[kNumProps];

static const int kNumMethods = sizeof(s_methodIdNames)/sizeof(const char*);
static NPIdentifier s_methodIds[kNumMethods];

static char* npStringToString(const NPString& str)
{
    char* s = (char*) malloc(str.UTF8Length + 1);
    s[str.UTF8Length] = 0;
    if (str.UTF8Length > 0)
        memcpy(s, str.UTF8Characters, str.UTF8Length);

    return s;
}

static std::string npStringToStlString(const NPString& str)
{
    std::string s;
    if (str.UTF8Length > 0) {
        s.resize(str.UTF8Length);
        for (unsigned int i = 0; i < str.UTF8Length; i++)
            s[i] = str.UTF8Characters[i];
    }

    return s;
}

static void stringToNPString(const char* str, NPVariant* result)
{
    char* s = ::strdup(str);
    STRINGZ_TO_NPVARIANT(s, *result);
}

static const char* nameForOrientation(Event::Orientation o)
{
    switch (o) {
    case (Event::Orientation_Down): return "down";
    case (Event::Orientation_Left): return "left";
    case (Event::Orientation_Right): return "right";
    default: break;
    }

    return "up";
}

static bool npVariantIsNumber(NPVariant arg)
{
    return NPVARIANT_IS_DOUBLE(arg) || NPVARIANT_IS_INT32(arg);
}

static uint32_t npVariantToNumber(NPVariant arg)
{
    if (NPVARIANT_IS_INT32(arg))
        return NPVARIANT_TO_INT32(arg);

    if (NPVARIANT_IS_DOUBLE(arg))
        return NPVARIANT_TO_DOUBLE(arg);

    return 0;
}

static double npVariantToDouble(NPVariant arg)
{
    if (NPVARIANT_IS_DOUBLE(arg))
        return NPVARIANT_TO_DOUBLE(arg);

    if (NPVARIANT_IS_INT32(arg)) {
        // Returning the uint32 value as a double can result in a loss of
        // precision if that value is casted to a signed int.  Use
        // npVariantToNumber() if a) the argument is an int32 and b) it
        // can be negative.
        return NPVARIANT_TO_INT32(arg);
    }

    return 0.0;
}

static inline WebPage* framePage(WebFrame* frame)
{
	if (frame && frame->page())
		return frame->page();
	return 0;
}

static inline WebPageClient* framePageClient(WebFrame* frame)
{
	if (frame && frame->page() && frame->page()->client())
		return frame->page()->client();

	return 0;
}


static OrientationEvent::Orientation getOrientation() 
{
    OrientationEvent::Orientation orientation = OrientationEvent::Orientation_Invalid;

    if (G_UNLIKELY(!sOrientationSensor))
    {
        sOrientationSensor = static_cast<HALOrientationSensorConnector *>(HALConnectorBase::getSensor(HALConnectorBase::SensorOrientation));
    }

    orientation = sOrientationSensor->getOrientation();

    return orientation;
}


JsSysObject::JsSysObject(NPP npp)
    : m_npp(npp)
{
    luna_log(sLogChannel, " ");
    m_browserFuncs = 0;
    m_frame = 0;
}

JsSysObject::~JsSysObject()
{
    luna_log(sLogChannel, " ");

    if (m_frame)
        m_frame->jsObjectDeleted();
}

void JsSysObject::release()
{
    luna_log(sLogChannel, " ");
    m_browserFuncs->releaseobject(this);
}

void JsSysObject::setBrowserFuncs(NPNetscapeFuncs* browserFuncs)
{
    luna_log(sLogChannel, " ");
    m_browserFuncs = browserFuncs;

    if (!s_initialized) {
        m_browserFuncs->getstringidentifiers(s_propIdNames, kNumProps, s_propIds);
        m_browserFuncs->getstringidentifiers(s_methodIdNames, kNumMethods, s_methodIds);
        s_initialized = true;
    }

    // get properties map --------------------------------------------

#undef KEY
#undef VAL
#undef KEY_VAL
#define KEY(x) s_propIds[Prop##x]
#define VAL(x) &JsSysObject::prop##x
#define KEY_VAL(x) m_getPropPtrMap[KEY(x)] = VAL(x)

    KEY_VAL(LaunchParams);
    KEY_VAL(HasAlphaHole);
    KEY_VAL(Locale);
    KEY_VAL(LocaleRegion);
    KEY_VAL(TimeFormat);
    KEY_VAL(TimeZone);
    KEY_VAL(IsMinimal);
    KEY_VAL(Identifier);
    KEY_VAL(Version);
    KEY_VAL(ScreenOrientation);
    KEY_VAL(WindowOrientation);
    KEY_VAL(SpecifiedWindowOrientation);
	KEY_VAL(VideoOrientation);
    KEY_VAL(DeviceInfo);
    KEY_VAL(IsActivated);
    KEY_VAL(ActivityId);
    KEY_VAL(PhoneRegion);

    luna_assert(m_getPropPtrMap.size() == (unsigned int) kNumProps);

    // set properties map --------------------------------------------

#undef KEY
#undef VAL
#undef KEY_VAL
#define KEY(x) s_propIds[Prop##x]
#define VAL(x) &JsSysObject::setProp##x
#define KEY_VAL(x) m_setPropPtrMap[KEY(x)] = VAL(x)

    KEY_VAL(WindowOrientation);
    KEY_VAL(HasAlphaHole);

    // method map ---------------------------------------------------

#undef KEY
#undef VAL
#undef KEY_VAL
#define KEY(x) s_methodIds[Method##x]
#define VAL(x) &JsSysObject::method##x
#define KEY_VAL(x) m_methodPtrMap[KEY(x)] = VAL(x)

	KEY_VAL(AddBannerMessage);
	KEY_VAL(RemoveBannerMessage);
	KEY_VAL(ClearBannerMessages);
	KEY_VAL(PlaySoundNotification);
	KEY_VAL(SimulateMouseClick);
	KEY_VAL(Paste);
	KEY_VAL(CopiedToClipboard);
	KEY_VAL(PastedFromClipboard);
	KEY_VAL(SetWindowOrientation);
	KEY_VAL(RunTextIndexer);
	KEY_VAL(PrepareSceneTransition);
	KEY_VAL(RunSceneTransition);
	KEY_VAL(CancelSceneTransition);
	KEY_VAL(RunCrossAppTransition);
	KEY_VAL(CrossAppSceneActive);
	KEY_VAL(CancelCrossAppScene);
	KEY_VAL(Encrypt);
	KEY_VAL(Decrypt);
	KEY_VAL(Shutdown);
	KEY_VAL(MarkFirstUseDone);
	KEY_VAL(EnableFullScreenMode);
	KEY_VAL(Activate);
	KEY_VAL(Deactivate);
	KEY_VAL(StagePreparing);
	KEY_VAL(StageReady);
	KEY_VAL(SetAlertSound);
	KEY_VAL(ReceivePageUpDownInLandscape);
	KEY_VAL(EnableDockMode);
	KEY_VAL(GetLocalizedString);
	KEY_VAL(Hide);
	KEY_VAL(Show);
	KEY_VAL(AddNewContentIndicator);
	KEY_VAL(RemoveNewContentIndicator);
	KEY_VAL(RunAnimationLoop);
	KEY_VAL(SetActiveBannerWindowWidth);
	KEY_VAL(CancelVibrations);
	KEY_VAL(SetWindowProperties );
	KEY_VAL(AddActiveCallBanner);
	KEY_VAL(RemoveActiveCallBanner);
	KEY_VAL(UpdateActiveCallBanner);
	KEY_VAL(ApplyLaunchFeedback);
	KEY_VAL(LauncherReady);
	KEY_VAL(GetDeviceKeys);
	KEY_VAL(Repaint);
	KEY_VAL(HideSpellingWidget);
	KEY_VAL(PrintFrame);
	KEY_VAL(EditorFocused);
	KEY_VAL(AllowResizeOnPositiveSpaceChange);
	KEY_VAL(KeepAlive);
    KEY_VAL(UseSimulatedMouseClicks);
    KEY_VAL(HandleTapAndHoldEvent);
    KEY_VAL(SetManualKeyboardEnabled);
    KEY_VAL(KeyboardShow);
    KEY_VAL(KeyboardHide);

    luna_assert(m_methodPtrMap.size() == (unsigned int) kNumMethods);
}

void JsSysObject::setFrame(WebFrame* frame)
{
    m_frame = frame;
}

void JsSysObject::setLaunchParams(const std::string& params)
{
    std::string p = params;
    if (!p.empty()) {
        json_object* root = json_tokener_parse(p.c_str());
        if (root && !is_error(root) && json_object_get_type(root) == json_type_object) {
            int keyValueCount = 0;
            json_object_object_foreach(root, key, val) {
                keyValueCount++;
            }

            json_object_put(root);

            if (keyValueCount == 0)
                p = std::string();
        }
    }

    m_launchParams = p;
}

void JsSysObject::invalidate()
{
    luna_log(sLogChannel, " ");
}

bool JsSysObject::hasMethod(NPIdentifier name)
{
    return (m_methodPtrMap.find(name) != m_methodPtrMap.end());
}

bool JsSysObject::invoke(NPIdentifier name, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

    MethodPtrMap::const_iterator it = m_methodPtrMap.find(name);
    if (it == m_methodPtrMap.end())
        return false;

    return (this->*(it->second))(args, argCount, result);
}

bool JsSysObject::invokeDefault(const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");
    return false;
}

bool JsSysObject::hasProperty(NPIdentifier name)
{
    //luna_log(sLogChannel, " ");

    return (m_getPropPtrMap.find(name) != m_getPropPtrMap.end());
}

bool JsSysObject::getProperty(NPIdentifier name, NPVariant *result)
{
    GetPropPtrMap::const_iterator it = m_getPropPtrMap.find(name);
    if (it == m_getPropPtrMap.end())
        return false;

    return (this->*(it->second))(result);
}

bool JsSysObject::setProperty(NPIdentifier name, const NPVariant *value)
{
    luna_log(sLogChannel, " ");

    SetPropPtrMap::const_iterator it = m_setPropPtrMap.find(name);
    if (it == m_setPropPtrMap.end())
        return false;

    return (this->*(it->second))(value);
}

bool JsSysObject::removeProperty(NPIdentifier name)
{
    luna_log(sLogChannel, " ");
    return false;
}

bool JsSysObject::enumerate(NPIdentifier **value, uint32_t *count)
{
    luna_log(sLogChannel, " ");
    return false;
}

bool JsSysObject::construct(const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");
    return false;
}

bool JsSysObject::propLaunchParams(NPVariant *result)
{
    luna_log(sLogChannel, " ");

    if (m_launchParams.empty())
        stringToNPString("", result);
    else
        stringToNPString(m_launchParams.c_str(), result);

    return true;
}

bool JsSysObject::propDeviceInfo(NPVariant* result)
{
    luna_log(sLogChannel, " ");

    std::string deviceInfo;

    if (WebPageClient* pageClient = framePageClient(m_frame)) {
        WebAppBase* app = static_cast<WebAppBase*>(pageClient);
        if (app->isCardApp()) {
            deviceInfo = DeviceInfo::instance()->jsonString(static_cast<CardWebApp*>(app)->windowType());
        } else {
        	deviceInfo = DeviceInfo::instance()->jsonString();
        }
    } else {
    	deviceInfo = DeviceInfo::instance()->jsonString();
    }

    if (deviceInfo.empty())
        stringToNPString("{}", result);
    else
        stringToNPString(deviceInfo.c_str(), result);

    return true;
}

bool JsSysObject::propHasAlphaHole(NPVariant *result)
{
    luna_log(sLogChannel, " ");

    BOOLEAN_TO_NPVARIANT(false, *result);

    return true;
}

bool JsSysObject::propIsMinimal(NPVariant* result)
{
    BOOLEAN_TO_NPVARIANT(Settings::LunaSettings()->uiType == Settings::UI_MINIMAL, *result);

    return true;

}

bool JsSysObject::propLocale(NPVariant *result)
{
    luna_log(sLogChannel, " ");

    // Access is thread-safe
    std::string locale = Preferences::instance()->locale();

    if (locale.empty())
        stringToNPString("", result);
    else
        stringToNPString(locale.c_str(), result);

    return true;
}

bool JsSysObject::propLocaleRegion(NPVariant* result)
{
    luna_log(sLogChannel, " ");

    // Access is thread-safe
    std::string localeRegion = Preferences::instance()->localeRegion();

    if (localeRegion.empty())
        stringToNPString("", result);
    else
        stringToNPString(localeRegion.c_str(), result);

    return true;
}

bool JsSysObject::propPhoneRegion(NPVariant* result)
{
	luna_log(sLogChannel, " ");

	// Access is thread-safe
	std::string phoneRegion = Preferences::instance()->phoneRegion();

	if (phoneRegion.empty())
		stringToNPString("", result);
	else
		stringToNPString(phoneRegion.c_str(), result);

	return true;
}

bool JsSysObject::propTimeFormat(NPVariant* result)
{
    //luna_log(sLogChannel, " ");

    // Access is thread-safe
    std::string timeFormat = Preferences::instance()->timeFormat();

    if (timeFormat.empty())
        stringToNPString("", result);
    else
        stringToNPString(timeFormat.c_str(), result);

    return true;
}

bool JsSysObject::propTimeZone(NPVariant* result)
{
    //luna_log(sLogChannel, " ");

    std::string timeZone = WebAppManager::instance()->getTimeZone();

    if (timeZone.empty())
        stringToNPString("", result);
    else
        stringToNPString(timeZone.c_str(), result);

    return true;
}

bool JsSysObject::propIdentifier(NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

    std::string id = m_frame->getIdentifier();

    stringToNPString(id.c_str(), result);

    return true;
}

bool JsSysObject::propVersion(NPVariant* result)
{
    luna_log(sLogChannel, " ");

	std::string version = Palm::WebGlobal::version();
    stringToNPString(version.c_str(), result);

    return true;
}

bool JsSysObject::propScreenOrientation(NPVariant* result)
{
    luna_log(sLogChannel, " ");

    // Enyo uses screenOrientation for windowOrientation so we need
    // to pretend to do the same
    //const char* orientStr = nameForOrientation(static_cast<Event::Orientation>(getOrientation()));
    const char* orientStr = nameForOrientation(WebAppManager::instance()->orientation());
    stringToNPString(orientStr, result);
    return true;
}

static Event::Orientation mapScreenOrientationToWindowOrientation(Event::Orientation aOrientation)
{
    Event::Orientation retValue = aOrientation;

    if (0 == Settings::LunaSettings()->homeButtonOrientationAngle) // For Opal device
    {
        switch(aOrientation)
        {
            case Event::Orientation_Right:
            {
                retValue = Event::Orientation_Left;
                break;
            }

            case Event::Orientation_Left:
            {
                retValue = Event::Orientation_Right;
                break;
            }

            default:
                break;
        }
    }
    // For Topaz devices
    else if ((270 == Settings::LunaSettings()->homeButtonOrientationAngle) ||
             (-90 == Settings::LunaSettings()->homeButtonOrientationAngle))
    {
        switch(aOrientation)
        {
            case Event::Orientation_Up:
            {
                retValue = Event::Orientation_Right;
                break;
            }

            case Event::Orientation_Right:
            {
                retValue = Event::Orientation_Up;
                break;
            }

            case Event::Orientation_Left:
            {
                retValue = Event::Orientation_Down;
                break;
            }

            case Event::Orientation_Down:
            {
                retValue = Event::Orientation_Left;
                break;
            }

            default:
                break;
        }
    }

    return retValue;
}

bool JsSysObject::propWindowOrientation(NPVariant* result)
{
    luna_log(sLogChannel, " ");

    Event::Orientation orient = Event::Orientation_Up;

    if (WebPageClient* pageClient = framePageClient(m_frame)) {
        WebAppBase* app = static_cast<WebAppBase*>(pageClient);
        if (app->isCardApp()) {
            orient = static_cast<CardWebApp*>(app)->orientation();
            orient = mapScreenOrientationToWindowOrientation(orient);
        }
    }

    const char* orientStr = nameForOrientation(orient);
    stringToNPString(orientStr, result);
    return true;
}

bool JsSysObject::propSpecifiedWindowOrientation(NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (m_specifiedWindowOrientation.empty())
        result->type = NPVariantType_Null;
    else
        stringToNPString(m_specifiedWindowOrientation.c_str(), result);

    return true;
}

bool JsSysObject::propVideoOrientation(NPVariant* result)
{
    luna_log(sLogChannel, " ");

	Event::Orientation orient = Event::Orientation_Up;
	
	switch (Settings::LunaSettings()->homeButtonOrientationAngle) {
	case 90:
        orient = Event::Orientation_Left;
		break;
	case 180:
		orient = Event::Orientation_Down;
		break;
    case 270:// For Topaz
	case -90:
        orient = Event::Orientation_Right;
		break;
	case 0:
	default:
		break;
	}
    
    const char* orientStr = nameForOrientation(orient);
    stringToNPString(orientStr, result);

	return true;
}

bool JsSysObject::propIsActivated(NPVariant* result)
{
    luna_log(sLogChannel, " ");

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isWindowed())
        return false;

    WindowedWebApp* windowedApp = static_cast<WindowedWebApp*>(app);
    BOOLEAN_TO_NPVARIANT(windowedApp->isFocused(), *result);

    return true;
}

bool JsSysObject::propActivityId(NPVariant* result)
{
    luna_log(sLogChannel, " ");

	WebPage* page = framePage(m_frame);
    if (!page)
        return false;

    int id = page->activityId();
    if (id < 0)
        VOID_TO_NPVARIANT(*result);
    else
        INT32_TO_NPVARIANT(id, *result);

    return true;
}

bool JsSysObject::setPropWindowOrientation(const NPVariant* value)
{
    luna_log(sLogChannel, " ");

    if (!value || !NPVARIANT_IS_STRING(*value)) {
        return false;
    }

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
		return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp()) {
        return false;
    }

    char* orientStr = npStringToString(NPVARIANT_TO_STRING(*value));
    if (!orientStr) {
        return false;
    }

    luna_log(sLogChannel, "%s", orientStr);

    m_specifiedWindowOrientation = orientStr;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);

    if (strcasecmp(orientStr, "free") == 0) {

        cardApp->setFixedOrientation(Event::Orientation_Invalid);
        cardApp->setAllowOrientationChange(true);
        cardApp->setOrientation(cardApp->appOrientationFor(WebAppManager::instance()->orientation()));

        cardApp->setAllowOrientationChange(true);
    }
    else {

        Event::Orientation orientation;

        if (strcasecmp(orientStr, "up") == 0)
            orientation = Event::Orientation_Up;
        else if (strcasecmp(orientStr, "down") == 0)
            orientation = Event::Orientation_Down;
        else if (strcasecmp(orientStr, "left") == 0)
            orientation = Event::Orientation_Left;
        else if (strcasecmp(orientStr, "right") == 0)
            orientation = Event::Orientation_Right;
        else if (strcasecmp(orientStr, "landscape") == 0)
            orientation = Event::Orientation_Landscape;
        else if (strcasecmp(orientStr, "portrait") == 0)
            orientation = Event::Orientation_Portrait;
        else {
            free(orientStr);
            return false;
        }

        cardApp->setFixedOrientation(orientation);
    }

    free(orientStr);
    return true;

}

bool JsSysObject::setPropHasAlphaHole(const NPVariant *value)
{
    luna_log(sLogChannel, " ");

    return true;
}

bool JsSysObject::methodAddBannerMessage(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

	if (!m_frame)
		return false;

    // Args are:
    // Mandatory: msg (string), launchParams (string),
    // Optional:  icon (string), soundClass (string), soundFile (string), duration (double), doNoSuppress (boolean)
    if (argCount < 2)
        return false;

    // All args need to be strings
    for (unsigned int i = 0; i < 2; i++) {
        if (!NPVARIANT_IS_STRING(args[i]))
            return false;
    }

    std::string msg = npStringToStlString(NPVARIANT_TO_STRING(args[0]));
    std::string launchParams = npStringToStlString(NPVARIANT_TO_STRING(args[1]));

    std::string icon;
    if (argCount >= 3 && NPVARIANT_IS_STRING(args[2]))
        icon = npStringToStlString(NPVARIANT_TO_STRING(args[2]));

    std::string soundClass;
    if (argCount >= 4 && NPVARIANT_IS_STRING(args[3]))
        soundClass = npStringToStlString(NPVARIANT_TO_STRING(args[3]));

    std::string soundFile;
    if (argCount >= 5 && NPVARIANT_IS_STRING(args[4]))
        soundFile = npStringToStlString(NPVARIANT_TO_STRING(args[4]));

    int duration = -1;
    if (argCount >= 6 && npVariantIsNumber(args[5]))
        duration = npVariantToDouble(args[5]);

    bool doNotSuppress = false;
    if (argCount >= 7 && NPVARIANT_IS_BOOLEAN(args[6]))
        doNotSuppress = NPVARIANT_TO_BOOLEAN(args[6]);

    BannerMessageEvent* e = BannerMessageEventFactory::createAddMessageEvent(m_frame->appId(),
                                                                             msg, launchParams,
                                                                             icon, soundClass, soundFile,
                                                                             duration, doNotSuppress);

    if (!e)
        return false;

    // Send the message ID back to the JS side
    g_debug("%s: Banner message created with id: %s", __PRETTY_FUNCTION__, e->msgId.c_str());
    stringToNPString(e->msgId.c_str(), result);

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));

    return true;
}

bool JsSysObject::methodRemoveBannerMessage(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

    if (argCount < 1)
        return false;

    if (!NPVARIANT_IS_STRING(args[0]))
        return false;

    std::string msgId = npStringToStlString(NPVARIANT_TO_STRING(args[0]));

    BannerMessageEvent* e = BannerMessageEventFactory::createRemoveMessageEvent(m_frame->appId(),
																				msgId);

    if (!e)
        return false;

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));

    return true;
}

bool JsSysObject::methodClearBannerMessages(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

    BannerMessageEvent* e = BannerMessageEventFactory::createClearMessagesEvent(m_frame->appId());

    if (!e)
        return false;

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));

    return true;
}

bool JsSysObject::methodPlaySoundNotification(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

    if (argCount < 1)
        return false;

    if (!NPVARIANT_IS_STRING(args[0]))
        return false;

    std::string soundClass = npStringToStlString(NPVARIANT_TO_STRING(args[0]));
    std::string soundFile;
    if (argCount >= 2 && NPVARIANT_IS_STRING(args[1]))
        soundFile = npStringToStlString(NPVARIANT_TO_STRING(args[1]));

    int duration = -1;
    if (argCount >= 3 && npVariantIsNumber(args[2]))
        duration = npVariantToDouble(args[2]);

    bool wakeupScreen = false;
    if (argCount >= 4 && NPVARIANT_IS_BOOLEAN(args[3]))
        wakeupScreen = NPVARIANT_TO_BOOLEAN(args[3]);


    BannerMessageEvent* e = BannerMessageEventFactory::createPlaySoundEvent(m_frame->appId(),
																			soundClass, soundFile,
																			duration, wakeupScreen);

    if (!e)
        return false;

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));

    return true;
}

bool JsSysObject::methodSimulateMouseClick(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

	if (WebAppManager::instance()->inSimulatedMouseEvent()) {
		g_warning("simulated mouse click called in middle of a simulated mouse event. Bailing...");
		return false;
	}

	WebPageClient* pageClient = framePageClient(m_frame);
	
    if (!pageClient || argCount < 3 )
        return false;

    int page_x = (int) npVariantToDouble(args[0]);
    int page_y = (int) npVariantToDouble(args[1]);
    bool mouseDown = NPVARIANT_TO_BOOLEAN(args[2]);

    /*
    // Note : this code is not correct as it calls directly into webkit
    // (this thread of execution reaching this point from inside webkit)
    if( mouseDown )
        m_page->webkitView()->mouseEvent(Palm::MouseDown, page_x, page_y, 1);
    else
        m_page->webkitView()->mouseEvent(Palm::MouseUp, page_x, page_y, 0);
    */

	// The events are now oriented wrt the card orientation even when the display
	// rotates and so this is not needed (DFISH-29659)
	/*														
    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (app->isCardApp()) {

        CardWebApp* cardApp = static_cast<CardWebApp*>(app);

        Event::Orientation orient = cardApp->orientation();
        switch (orient) {
        case (Event::Orientation_Left): {

            int tmp = page_x;
            page_x = cardApp->windowHeight() - page_y;
            page_y = tmp;
            break;
        }
        case (Event::Orientation_Right): {

            int tmp = page_x;
            page_x = page_y;
            page_y = cardApp->windowWidth() - tmp;
            break;
        }
        case (Event::Orientation_Down): {

            page_x = cardApp->windowWidth() - page_x;
            page_y = cardApp->windowHeight() - page_y;
        }
        default:
            break;
        }
    }
    */

    if( mouseDown )
    {
        sptr<Event> e = new Event;
        e->x = page_x;
        e->y = page_y;
        e->type = Event::PenDown;
        e->button = Event::Left;
        e->modifiers = 0;
        e->setMainFinger(true);
        e->simulated = true;
        //e->clickCount = 1;
        int winKey = pageClient->getKey();
        WebAppManager::instance()->inputEvent(winKey,e);
    }
    else
    {
        sptr<Event> e = new Event;
        e->x = page_x;
        e->y = page_y;
        e->type = Event::PenUp;
        e->button = Event::Left;
        e->modifiers = 0;
        e->setMainFinger(true);
        e->simulated = true;
        int winKey = pageClient->getKey();
        WebAppManager::instance()->inputEvent(winKey,e);
    }

    return true;
}

bool JsSysObject::methodPaste(const NPVariant* /* args */, uint32_t /* argCount */, NPVariant * /*result*/)
{
    // Request SysMgr to emmit a Paste command to the active window
    WebAppManager::instance()->sendAsyncMessage(new ViewHost_PasteToActiveWindow());

    return true;
}

bool JsSysObject::methodCopiedToClipboard(const NPVariant* /* args */, uint32_t /* argCount */, NPVariant * /*result*/)
{
    if (!m_frame)
        return false;

    WebAppManager::instance()->copiedToClipboard(m_frame->appId());

    return true;
}

bool JsSysObject::methodPastedFromClipboard(const NPVariant* /* args */, uint32_t /* argCount */, NPVariant * /*result*/)
{
    if (!m_frame)
        return false;

    WebAppManager::instance()->pastedFromClipboard(m_frame->appId());

    return true;
}

bool JsSysObject::methodSetWindowOrientation(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

    if (argCount < 1 || !NPVARIANT_IS_STRING(args[0]))
        return false;

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    char* orientStr = npStringToString(NPVARIANT_TO_STRING(args[0]));
    if (!orientStr)
        return false;

    m_specifiedWindowOrientation = orientStr;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);

    if (strcasecmp(orientStr, "free") == 0) {

        cardApp->setFixedOrientation(Event::Orientation_Invalid);
        cardApp->setAllowOrientationChange(true);
        cardApp->setOrientation(WebAppManager::instance()->orientation());
    }
    else {

        Event::Orientation orientation;

        if (strcasecmp(orientStr, "up") == 0)
            orientation = Event::Orientation_Up;
        else if (strcasecmp(orientStr, "down") == 0)
            orientation = Event::Orientation_Down;
        else if (strcasecmp(orientStr, "left") == 0)
            orientation = Event::Orientation_Left;
        else if (strcasecmp(orientStr, "right") == 0)
            orientation = Event::Orientation_Right;
        else if (strcasecmp(orientStr, "landscape") == 0)
            orientation = Event::Orientation_Landscape;
        else if (strcasecmp(orientStr, "portrait") == 0)
            orientation = Event::Orientation_Portrait;
        else {
            free(orientStr);
            return false;
        }

        cardApp->setFixedOrientation(orientation);
    }

    free(orientStr);
    return true;
}

bool JsSysObject::methodRunTextIndexer( const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " " );

    if( argCount < 1 )
        return false;

    if( !NPVARIANT_IS_STRING(args[0]) )
        return false;

    char* srcStr = npStringToString(NPVARIANT_TO_STRING(args[0]));
    if( !srcStr )
        return false;

    unsigned int flags = Palm::TextIndexer_Default;
    if (argCount > 1 && NPVARIANT_IS_OBJECT(args[1])) {

        NPObject* obj = NPVARIANT_TO_OBJECT(args[1]);
        if (obj) {

            NPIdentifier fieldId = m_browserFuncs->getstringidentifier ("phoneNumber");
            NPVariant fieldValue;
            if (m_browserFuncs->getproperty(m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN(fieldValue) ) {

                if (NPVARIANT_TO_BOOLEAN(fieldValue))
                    flags |= Palm::TextIndexer_PhoneNumber;
                else
                    flags &= ~Palm::TextIndexer_PhoneNumber;
            }

            fieldId = m_browserFuncs->getstringidentifier ("emailAddress");
            if (m_browserFuncs->getproperty(m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN(fieldValue) ) {

                if (NPVARIANT_TO_BOOLEAN(fieldValue))
                    flags |= Palm::TextIndexer_EmailAddress;
                else
                    flags &= ~Palm::TextIndexer_EmailAddress;
            }

            fieldId = m_browserFuncs->getstringidentifier ("webLink");
            if (m_browserFuncs->getproperty(m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN(fieldValue) ) {

                if (NPVARIANT_TO_BOOLEAN(fieldValue))
                    flags |= Palm::TextIndexer_WebLink;
                else
                    flags &= ~Palm::TextIndexer_WebLink;
            }

            fieldId = m_browserFuncs->getstringidentifier ("schemalessWebLink");
            if (m_browserFuncs->getproperty(m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN(fieldValue) ) {

                if (NPVARIANT_TO_BOOLEAN(fieldValue))
                    flags |= Palm::TextIndexer_SchemalessWebLink;
                else
                    flags &= ~Palm::TextIndexer_SchemalessWebLink;
            }

            fieldId = m_browserFuncs->getstringidentifier ("emoticon");
            if (m_browserFuncs->getproperty(m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN(fieldValue) ) {

                if (NPVARIANT_TO_BOOLEAN(fieldValue))
                    flags |= Palm::TextIndexer_Emoticon;
                else
                    flags &= ~Palm::TextIndexer_Emoticon;
            }
        }
    }
    luna_log(sLogChannel, "indexing with the following flags %04X", flags);
    std::string markedUp = Palm::WebGlobal::runTextIndexerOnHtml( std::string(srcStr), flags );
    stringToNPString(markedUp.c_str(), result);

    free(srcStr);
    return true;
}

bool JsSysObject::methodPrepareSceneTransition(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    static_cast<CardWebApp*>(app)->prepareSceneTransition();

    return true;
}

bool JsSysObject::methodRunSceneTransition(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    if (argCount < 2 || !NPVARIANT_IS_STRING(args[0]) || !NPVARIANT_IS_BOOLEAN(args[1]))
        return false;

    char* transitionType = npStringToString(NPVARIANT_TO_STRING(args[0]));
    bool isPop = NPVARIANT_TO_BOOLEAN(args[1]);

    static_cast<CardWebApp*>(app)->runSceneTransition(transitionType, isPop);

    free(transitionType);

    return true;
}

bool JsSysObject::methodCancelSceneTransition(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    static_cast<CardWebApp*>(app)->cancelSceneTransition();

    return true;
}

bool JsSysObject::methodRunCrossAppTransition(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    if (argCount < 1 || !NPVARIANT_IS_BOOLEAN(args[0]))
        return false;

    bool isPop = NPVARIANT_TO_BOOLEAN(args[0]);

    static_cast<CardWebApp*>(app)->runCrossAppTransition(isPop);

    return true;
}

bool JsSysObject::methodCrossAppSceneActive(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
        return false;

    luna_log(sLogChannel, "%s, %s",  m_frame->appId().c_str(), m_frame->processId().c_str());

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    static_cast<CardWebApp*>(app)->crossAppSceneActive();

    return true;
}

bool JsSysObject::methodCancelCrossAppScene(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " ");

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    static_cast<CardWebApp*>(app)->cancelCrossAppScene();

    return true;
}

// Blowfish encrypt
bool JsSysObject::methodEncrypt(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " " );

    if( argCount < 2 )
        return false;

    if( !NPVARIANT_IS_STRING(args[0]) || !NPVARIANT_IS_STRING(args[1]) )
        return false;

    char* key = npStringToString(NPVARIANT_TO_STRING(args[0]));
    if( !key )
        return false;
    char* str = npStringToString(NPVARIANT_TO_STRING(args[1]));
    if( !str ) {
        free(key);
        return false;
    }

    char* encryptedString=0;

    bool r = false;//encryptString( str, key, &encryptedString );

    free(key);
    free(str);

    if( !r ) {
        return false;
    }

    stringToNPString(encryptedString, result);
    delete[] encryptedString;

    return true;
}

// Blowfish decrypt
bool JsSysObject::methodDecrypt(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    luna_log(sLogChannel, " " );

    if( argCount < 2 )
        return false;

    if( !NPVARIANT_IS_STRING(args[0]) || !NPVARIANT_IS_STRING(args[1]) )
        return false;

    char* key = npStringToString(NPVARIANT_TO_STRING(args[0]));
    if( !key ) {
        return false;
    }
    char* b64 = npStringToString(NPVARIANT_TO_STRING(args[1]));
    if( !b64 ) {
        free(key);
        return false;
    }

    char* cleartextString=0;

    bool r = false;//decryptString( (const char*)b64, key, &cleartextString );

    free(b64);
    free(key);

    if( !r )
        return false;

    stringToNPString(cleartextString, result);
    delete[] cleartextString;

    return true;
}

bool JsSysObject::methodMarkFirstUseDone(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
	markFirstUseDone();
	return true;    
}

bool JsSysObject::methodShutdown(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    std::string appId = m_frame ?  m_frame->appId() : std::string();

    if ((Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) ||
        (appId == "com.palm.systemui")) {

		markFirstUseDone();
        exit(0);
    }

    return true;
}

bool JsSysObject::methodEnableFullScreenMode(const NPVariant* args, uint32_t argCount, NPVariant *result)
{
    sptr<Event> e = new Event;

    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    if (argCount < 1 || !NPVARIANT_IS_BOOLEAN(args[0]))
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);
    cardApp->enableFullScreenMode(NPVARIANT_TO_BOOLEAN(args[0]));
    return true;
}

bool JsSysObject::methodActivate(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    pageClient->focus();

    return true;
}

bool JsSysObject::methodDeactivate(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    pageClient->unfocus();

    return true;
}

bool JsSysObject::methodStagePreparing(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    app->stagePreparing();

    return true;
}

bool JsSysObject::methodStageReady(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPage* page = framePage(m_frame);
	WebPageClient* pageClient = framePageClient(m_frame);
	
	if (!page)
        return false;
	
    if (!pageClient) {
        // delay stageReady until the client is ready
        page->setStageReadyPending(true);
        return false;
    }

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    app->stageReady();

    return true;
}

bool JsSysObject::methodSetAlertSound(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isAlertApp()) {
        luna_warn(sLogChannel, "Not an alert web app");
        return false;
    }

    AlertWebApp* alertApp = static_cast<AlertWebApp*>(app);
    if (!alertApp->getKey()) {
        luna_warn(sLogChannel, "no window for alert app");
        return false;
    }

    if (argCount < 1) {
        luna_warn(sLogChannel, "num args incorrect: expected at least 1. got: %d", argCount);
        return false;
    }

    std::string soundClass;
    std::string sound;

    if (argCount == 1) {

        if (!NPVARIANT_IS_STRING(args[0])) {
            luna_warn(sLogChannel, "args[0] not a string");
            return false;
        }

        char* str = npStringToString(NPVARIANT_TO_STRING(args[0]));
        soundClass = str;
        free(str);
    }
    else if (argCount == 2) {

        if (!NPVARIANT_IS_STRING(args[0]) ||
            !NPVARIANT_IS_STRING(args[1])) {
            luna_warn(sLogChannel, "args[0] or args[1] not a string");
            return false;
        }

        char* str0 = npStringToString(NPVARIANT_TO_STRING(args[0]));
        char* str1 = npStringToString(NPVARIANT_TO_STRING(args[1]));
        soundClass = str0;
        sound = str1;
        free(str0);
        free(str1);
    }

    luna_log(sLogChannel, "soundclass: %s, sound: %s", soundClass.c_str(), sound.c_str());

    alertApp->setSoundParams(sound, soundClass);

    return true;
}

bool JsSysObject::methodReceivePageUpDownInLandscape(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    if (argCount < 1)
        return false;

    if (!NPVARIANT_IS_BOOLEAN(args[0]))
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);
    cardApp->receivePageUpDownInLandscape(NPVARIANT_TO_BOOLEAN(args[0]));

    return true;
}

bool JsSysObject::methodShow(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPage* page = framePage(m_frame);
	if (!page)
        return false;

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
		return false;

	static_cast<WebAppBase*>(pageClient)->thawFromCache();

	return true;
}

bool JsSysObject::methodHide(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    // Park this card -- we'll "close" it and the WebPage will automatically
    // get parked as long as this appID is marked as keep-alive.
    pageClient->close();

    return true;
}

bool JsSysObject::methodEnableDockMode(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	luna_log(sLogChannel, " ");

	if (!m_frame)
		return false;

	// only allow this call from the systemui app
	if (m_frame->appId() != "com.palm.systemui") {
		luna_warn(sLogChannel, "%s not permitted to call this function", m_frame->appId().c_str());
		return false;
	}

	if (argCount != 1 || !NPVARIANT_IS_BOOLEAN(args[0]))
		return false;

	bool enabled = NPVARIANT_TO_BOOLEAN(args[0]);

	WebAppManager::instance()->sendAsyncMessage(new ViewHost_EnableDockMode(enabled));

	return true;
}

bool JsSysObject::methodGetLocalizedString(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

    // only allow this call from the memory app
    if (m_frame->appId() != "com.palm.memory") {
        luna_warn(sLogChannel, "%s not permitted to call this function", m_frame->appId().c_str());
        return false;
    }

    if (argCount < 1 || !NPVARIANT_IS_STRING(args[0]))
        return false;

    char* text = npStringToString(NPVARIANT_TO_STRING(args[0]));
    std::string textLocalized = LOCALIZED(text);
    free(text);

    stringToNPString(textLocalized.c_str(), result);

    return true;
}

bool JsSysObject::methodAddNewContentIndicator(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

	WebPage* page = framePage(m_frame);
	if (!page)
		return false;

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!(app->isAlertApp() || app->isDashboardApp())) {
        g_warning ("not adding new content indicator as calling app is not an alert or a dashboard");
        return false;
    }

	std::string appId = m_frame->appId();
	if (appId.empty())
		return false;

    NewContentIndicatorEvent* e = NewContentIndicatorEventFactory::createAddEvent(appId);
    if (!e)
        return false;

    luna_log(sLogChannel, "New Content Indicator created with id: %s", e->requestId.c_str());
    page->addNewContentRequestId (e->requestId);
    stringToNPString(e->requestId.c_str(), result);

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_NewContentEvent(NewContentIndicatorEventWrapper(e)));

    return true;
}

bool JsSysObject::methodRemoveNewContentIndicator(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (argCount < 1)
        return false;

	if (!m_frame || !m_frame->page())
		return false;

	std::string appId = m_frame->appId();
	if (appId.empty())
		return false;
	
    if (!NPVARIANT_IS_STRING(args[0]))
        return false;

    char* requestId = npStringToString(NPVARIANT_TO_STRING(args[0]));
    if (!requestId)
        return false;

	
    luna_log(sLogChannel, "Remove Content Indicator  with id: %s", requestId);
    NewContentIndicatorEvent* e = NewContentIndicatorEventFactory::createRemoveEvent(appId, requestId);
	m_frame->page()->removeNewContentRequestId (std::string (requestId));

    free(requestId);

    if (!e)
        return false;

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_NewContentEvent(NewContentIndicatorEventWrapper(e)));

    return true;
}

bool JsSysObject::methodRunAnimationLoop(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    if (argCount < 7)
        return false;

    if (!NPVARIANT_IS_OBJECT(args[0]) || // obj
        !NPVARIANT_IS_STRING(args[1]) || // onStep
        !NPVARIANT_IS_STRING(args[2]) || // onComplete
        !NPVARIANT_IS_STRING(args[3]) || // animation curve
        !npVariantIsNumber(args[4]) || // duration
        !npVariantIsNumber(args[5]) || // initial value
        !npVariantIsNumber(args[6]))   // final value
        return false;

    char* onStepCallbackName = npStringToString(NPVARIANT_TO_STRING(args[1]));
    if (!onStepCallbackName)
        return false;

    char* onCompleteCallbackName = npStringToString(NPVARIANT_TO_STRING(args[2]));
    if (!onCompleteCallbackName) {
        free(onStepCallbackName);
        return false;
    }

    char* animationCurveStr = npStringToString(NPVARIANT_TO_STRING(args[3]));
    if (!animationCurveStr) {
        free(onStepCallbackName);
        free(onCompleteCallbackName);
        return false;
    }

    NPIdentifier onStepCallbackId = m_browserFuncs->getstringidentifier(onStepCallbackName);
    free(onStepCallbackName);

    NPIdentifier onCompleteCallbackId = m_browserFuncs->getstringidentifier(onCompleteCallbackName);
    free(onCompleteCallbackName);

    std::string animationCurve = animationCurveStr;
    free(animationCurveStr);

    NPObject* domObj = NPVARIANT_TO_OBJECT(args[0]);
    m_browserFuncs->retainobject(domObj);

    double duration = npVariantToDouble(args[4]);
    double initialValue = npVariantToDouble(args[5]);
    double finalValue = npVariantToDouble(args[6]);

    JsSysObjectAnimationRunner::instance()->run(static_cast<CardWebApp*>(app), domObj,
                                                m_browserFuncs, m_npp,
                                                onStepCallbackId, onCompleteCallbackId,
                                                animationCurve, duration, initialValue, finalValue);

    m_browserFuncs->releaseobject(domObj);

    return true;
}

bool JsSysObject::methodSetActiveBannerWindowWidth(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    return true;
}

bool JsSysObject::methodCancelVibrations(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_CancelVibrations());

    return true;
}

bool JsSysObject::methodSetWindowProperties(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " " );

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
        return false;

    if (argCount < 1)
        return false;

    if (NPVARIANT_IS_OBJECT (*args)) {
        NPObject* obj = NPVARIANT_TO_OBJECT (*args);
        WindowProperties props;

        if (!obj)
            return true;

        // check for screen timeout
        NPIdentifier fieldId = m_browserFuncs->getstringidentifier ("blockScreenTimeout");
        NPVariant	fieldValue;
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setBlockScreenTimeout (NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        // check for subtle light bar: OLD API
        fieldId = m_browserFuncs->getstringidentifier ("setSubtleLightbar");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setSubtleLightbar (NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        // check for subtle light bar: NEW API
        fieldId = m_browserFuncs->getstringidentifier ("subtleLightbar");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setSubtleLightbar (NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        // check for fast accelerometer
        fieldId = m_browserFuncs->getstringidentifier("fastAccelerometer");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            WebPage* page = framePage(m_frame);
            if (page)
            {
                page->fastAccelerometerOn(NPVARIANT_TO_BOOLEAN (fieldValue));
            }
        }

        fieldId = m_browserFuncs->getstringidentifier("activeTouchpanel");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setActiveTouchpanel(NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        fieldId = m_browserFuncs->getstringidentifier("alsDisabled");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setAlsDisabled(NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        fieldId = m_browserFuncs->getstringidentifier("enableCompass");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setCompassEvents(NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        fieldId = m_browserFuncs->getstringidentifier("enableGyro");
		if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
			props.setAllowGyroEvents(NPVARIANT_TO_BOOLEAN (fieldValue));
		}

        // check for Suppress Banner Messages
        fieldId = m_browserFuncs->getstringidentifier("suppressBannerMessages");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setSuppressBannerMessages(NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        fieldId = m_browserFuncs->getstringidentifier("suppressGestures");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setSuppressGestures(NPVARIANT_TO_BOOLEAN (fieldValue));
        }

        fieldId = m_browserFuncs->getstringidentifier("webosDragMode");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_STRING (fieldValue)) {
        	std::string mode = npStringToStlString(NPVARIANT_TO_STRING (fieldValue));
            props.setDashboardManualDragMode(0 == mode.compare("manual"));
        }

        fieldId = m_browserFuncs->getstringidentifier("statusBarColor");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_INT32 (fieldValue)) {
            props.setStatusBarColor(NPVARIANT_TO_INT32 (fieldValue));
        }

        fieldId = m_browserFuncs->getstringidentifier("rotationLockMaximized");
        if (m_browserFuncs->getproperty (m_npp, obj, fieldId, &fieldValue) && NPVARIANT_IS_BOOLEAN (fieldValue)) {
            props.setRotationLockMaximized(NPVARIANT_TO_BOOLEAN (fieldValue));
        }

		WebAppManager::instance()->setAppWindowProperties(pageClient->getKey(), props);
    }
    return true;
}


bool JsSysObject::methodAddActiveCallBanner(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

#ifndef TARGET_DESKTOP
    if (m_frame->appId() != "com.palm.app.phone")
        return false;
#endif

    // Args are:
    // Mandatory: icon (string) msg (string) time (uint)
    if (argCount < 3)
        return false;

    // args 1 and 2 need to be strings , arg 3 needs to be an int
    if ( (!NPVARIANT_IS_STRING(args[0])) || (!NPVARIANT_IS_STRING(args[1])) || (!npVariantIsNumber(args[2])) )
    {
        return false;
    }

    std::string icon = npStringToStlString(NPVARIANT_TO_STRING(args[0]));
    std::string message = npStringToStlString(NPVARIANT_TO_STRING(args[1]));
    uint32_t timeStart = (uint32_t)(npVariantToDouble(args[2])/1000);

    ActiveCallBannerEvent* e = ActiveCallBannerEventFactory::createAddEvent(message, icon, timeStart);
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_ActiveCallBannerEvent(ActiveCallBannerEventWrapper(e)));

    g_message("%s: Active Call Banner created?: %d", __PRETTY_FUNCTION__, (e != 0));
    BOOLEAN_TO_NPVARIANT((e != 0), *result);

    return true;
}

bool JsSysObject::methodRemoveActiveCallBanner(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

#ifndef TARGET_DESKTOP
    if (m_frame->appId() != "com.palm.app.phone")
        return false;
#endif

    ActiveCallBannerEvent* e = ActiveCallBannerEventFactory::createRemoveEvent();
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_ActiveCallBannerEvent(ActiveCallBannerEventWrapper(e)));

    g_message("%s: Active Call Banner deleted?: %d", __PRETTY_FUNCTION__, (e != 0));
    return true;
}

bool JsSysObject::methodUpdateActiveCallBanner(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame)
        return false;

#ifndef TARGET_DESKTOP
    if (m_frame->appId() != "com.palm.app.phone")
        return false;
#endif

    // Args are:
    // Mandatory: icon (string) msg (string) time (uint)
    if (argCount < 3)
        return false;

    // args 1 and 2 need to be strings , arg 3 needs to be an int
    if ( (!NPVARIANT_IS_STRING(args[0])) || (!NPVARIANT_IS_STRING(args[1])) || (!npVariantIsNumber(args[2])) )
    {
        return false;
    }

    std::string icon = npStringToStlString(NPVARIANT_TO_STRING(args[0]));
    std::string message = npStringToStlString(NPVARIANT_TO_STRING(args[1]));
    uint32_t timeStart = (uint32_t)(npVariantToDouble(args[2])/1000);

    ActiveCallBannerEvent* e = ActiveCallBannerEventFactory::createUpdateEvent(message, icon, timeStart);
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_ActiveCallBannerEvent(ActiveCallBannerEventWrapper(e)));

    g_message("%s: Active Call Banner updated?: %d", __PRETTY_FUNCTION__, (e != 0));
    BOOLEAN_TO_NPVARIANT((e != 0), *result);

    return true;
}

bool JsSysObject::methodApplyLaunchFeedback(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

	WebPageClient* pageClient = framePageClient(m_frame);
	if (!pageClient)
		return false;

    if (m_frame->appId() != "com.palm.launcher")
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isWindowed())
        return false;

    // Args are:
    // Mandatory: offsetX, offsetY
    if (argCount < 2)
        return false;

    if (!npVariantIsNumber(args[0]) ||
        !npVariantIsNumber(args[1]))
        return false;

    static_cast<WindowedWebApp*>(app)->applyLaunchFeedback((int) npVariantToDouble(args[0]),
                                                           (int) npVariantToDouble(args[1]));

    return true;
}

bool JsSysObject::methodLauncherReady(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    luna_log(sLogChannel, " ");

    if (!m_frame || m_frame->appId() != "com.palm.launcher")
        return false;

    g_message("%s: launcher has marked boot as finished", __PRETTY_FUNCTION__);

    WebAppManager::instance()->markUniversalSearchReady();

    return true;
}

bool JsSysObject::methodGetDeviceKeys(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    // Args are:
    // Mandatory: key (int)
    if (argCount < 1) {
        luna_log(sLogChannel, "Invalid argCount. Expected 3, Got %d\n", argCount);
        return false;
    }

    if (!npVariantIsNumber(args[0])) {
        luna_log(sLogChannel, "Invalid arg types");
        return false;
    }

    int key = (int) npVariantToDouble(args[0]);

    KeyMapType details = getDetailsForKey(key);
    json_object* json = json_object_new_object();
    const int tempLen = 4;
    char temp[tempLen + 1];

    if (details.normal != Event::Key_Null)
        sprintf(temp, "%c", details.normal);
    else
        strcpy(temp, "");

    json_object_object_add(json, (char*) "normal", json_object_new_string(temp));

    if (details.shift != Event::Key_Null)
        sprintf(temp, "%c", details.shift);
    else
        strcpy(temp, "");

    json_object_object_add(json, (char*) "shift", json_object_new_string(temp));

    if (details.opt != Event::Key_Null)
        sprintf(temp, "%c", details.opt);
    else
        strcpy(temp, "");

    json_object_object_add(json, (char*) "opt", json_object_new_string(temp));

    stringToNPString(json_object_to_json_string(json), result);

    json_object_put(json);

    return true;
}

bool JsSysObject::methodRepaint(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    if (m_frame->appId() != "com.palm.launcher")
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isWindowed())
        return false;

    WindowedWebApp* winApp = static_cast<WindowedWebApp*>(app);
    winApp->invalidate();
    winApp->paint(false);

    return true;
}

bool JsSysObject::methodHideSpellingWidget(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	WebPage* page = framePage(m_frame);
    if (!page)
        return false;

	page->hideSpellingWidget();
    return true;
}

bool JsSysObject::methodPrintFrame(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    g_debug("%s: argCount = %d", __PRETTY_FUNCTION__, argCount);

	WebPage* page = framePage(m_frame);
    if (!page || !page->webkitView())
        return false;

    if (argCount < 6)
        return false;

    if (!NPVARIANT_IS_STRING(args[0]) ||  /* frameName */
        !npVariantIsNumber(args[1]) ||    /* lpsJobId */
        !npVariantIsNumber(args[2]) ||    /* widthPx */
        !npVariantIsNumber(args[3]) ||    /* heightPx */
        !npVariantIsNumber(args[4]) ||    /* printDpi */
        !NPVARIANT_IS_BOOLEAN(args[5]))   /* landscape */
        return false;

    bool reverseOrder = false;
    if (argCount > 6 && NPVARIANT_IS_BOOLEAN(args[6]))
        reverseOrder = NPVARIANT_TO_BOOLEAN(args[6]);

//DangL.-coverity-investigation:  should do we need to use malloc here:
    
    char* tmpstr = npStringToString(NPVARIANT_TO_STRING(args[0]));
    
    g_debug("[JsSysObject::methodPrintFrame] frameName = %s", tmpstr);
    //g_debug("[JsSysObject::methodPrintFrame] frameName = %s", npStringToString(NPVARIANT_TO_STRING(args[0])));


    page->webkitView()->print(tmpstr, /* frameName */
    //page->webkitView()->print(npStringToString(NPVARIANT_TO_STRING(args[0])), /* frameName */
                                (int)npVariantToNumber(args[1]),  /* lpsJobId */
                                (int)npVariantToNumber(args[2]),  /* printableWidth */
                                (int)npVariantToNumber(args[3]),  /* printableHeight */
                                (int)npVariantToNumber(args[4]),  /* printDpi */
                                NPVARIANT_TO_BOOLEAN(args[5]),    /* landscape */
                                reverseOrder                      /* reverseOrder */
                                );
    free(tmpstr);
    return true;
}

bool JsSysObject::methodEditorFocused(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	g_debug("%s: argCount %d", __FUNCTION__, argCount);

	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
		return false;

	if (argCount < 2)
		return false;

	if (!NPVARIANT_IS_BOOLEAN(args[0]) ||  /* focus state */
		!NPVARIANT_IS_INT32(args[1]) ||	   /* field type */
		!NPVARIANT_IS_INT32(args[2]))	 /* field actions */
		return false;

	WebAppBase* app = static_cast<WebAppBase*>(pageClient);
	if (!app->isCardApp())
		return false;
	
	bool focused = NPVARIANT_TO_BOOLEAN(args[0]);
	int fieldType = NPVARIANT_TO_INT32(args[1]);
	int fieldActions =	NPVARIANT_TO_INT32(args[2]);

	g_debug("%s: focused %d, type %d, actions %d", __FUNCTION__, focused, fieldType, fieldActions);

	PalmIME::EditorState editorState(static_cast<PalmIME::FieldType>(fieldType), static_cast<PalmIME::FieldAction>(fieldActions));

	app->setExplicitEditorFocus(focused, editorState);

	return true;
}

bool JsSysObject::methodAllowResizeOnPositiveSpaceChange(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

	if (argCount < 1)
		return false;

	if (!NPVARIANT_IS_BOOLEAN(args[0]))
		return false;

	WebAppBase* app = static_cast<WebAppBase*>(pageClient);
	if (!app->isCardApp())
		return false;
	
	bool allowResize = NPVARIANT_TO_BOOLEAN(args[0]);

	g_message("%s: %d", __FUNCTION__, allowResize);

	static_cast<CardWebApp*>(app)->allowResizeOnPositiveSpaceChange(allowResize);
	return true;
}

void JsSysObject::markFirstUseDone()
{
	g_message("[%s]: DEBUG: staring markFirstUseDone.", __PRETTY_FUNCTION__);
	// For first-use mode, touch a marker file on the filesystem
	if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {
		g_mkdir_with_parents(Settings::LunaSettings()->lunaPrefsPath.c_str(), 0755);
		FILE* f = fopen((Settings::LunaSettings()->lunaPrefsPath + "/ran-first-use").c_str(), "w");
		fclose(f);

		g_message("[%s]: DEBUG: emitting first-use-finished", __PRETTY_FUNCTION__);
		::system("/sbin/initctl emit first-use-finished");
	}
	g_message("[%s]: DEBUG: leaving markFirstUseDone.", __PRETTY_FUNCTION__);
}

bool JsSysObject::methodKeepAlive(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    if (argCount < 1)
        return false;

    if (!NPVARIANT_IS_BOOLEAN(args[0]))
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    const std::set<std::string>& appsToKeepAlive = Settings::LunaSettings()->appsToKeepAlive;
    if (appsToKeepAlive.find(m_frame->appId()) == appsToKeepAlive.end())
        return false;

    bool keepAlive = NPVARIANT_TO_BOOLEAN(args[0]); 
    app->setKeepAlive(keepAlive);

    return true;
}

bool JsSysObject::methodUseSimulatedMouseClicks(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;

    WebPage* page = framePage(m_frame);
    if (!page || !page->webkitView())
        return false;

    if (argCount < 1)
        return false;

    if (!NPVARIANT_IS_BOOLEAN(args[0]))
        return false;

    page->webkitView()->usesSimulatedMouseClicks(NPVARIANT_TO_BOOLEAN(args[0]));

    return true;
}

bool JsSysObject::methodHandleTapAndHoldEvent(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    WebPage* page = framePage(m_frame);
    if (!page || !page->webkitView())
        return false;

    if (argCount < 2)
        return false;

    if (!NPVARIANT_IS_INT32(args[0]) || !NPVARIANT_IS_INT32(args[1]))
        return false;

    // Disconnect the API since clipboard should work in any app not just enyo apps.
    // page->webkitView()->mouseHoldEvent(NPVARIANT_TO_INT32(args[0]), NPVARIANT_TO_INT32(args[1]));
    return true;
}

bool JsSysObject::methodSetManualKeyboardEnabled(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;
	
    if (argCount < 1)
        return false;

    if (!NPVARIANT_IS_BOOLEAN(args[0]))
        return false;

    bool enabled = NPVARIANT_TO_BOOLEAN(args[0]);

	g_debug("%s: enabled: %d", __PRETTY_FUNCTION__, enabled);

	app->setManualEditorFocusEnabled(enabled);

    return true;
}

bool JsSysObject::methodKeyboardShow(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;
	
    if (argCount < 1)
        return false;

    if (!NPVARIANT_IS_INT32(args[0]))
        return false;

    int fieldType = NPVARIANT_TO_INT32(args[0]);

    g_debug("%s: type %d", __PRETTY_FUNCTION__, fieldType);

    PalmIME::EditorState editorState(static_cast<PalmIME::FieldType>(fieldType));
    app->setManualEditorFocus(true, editorState);

    return true;
}

bool JsSysObject::methodKeyboardHide(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return false;

	WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return false;
	
    g_debug("%s", __PRETTY_FUNCTION__);

    PalmIME::EditorState editorState;
    app->setManualEditorFocus(false, editorState);

    return true;
}


