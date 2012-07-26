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
#include <glib.h>

#include <cjson/json.h>

#include "ApplicationDescription.h"
#include "ApplicationStatus.h"
#include "LaunchPoint.h"
#include "Utils.h"
#include "QtUtils.h"
#include "Settings.h"
#include "MimeSystem.h"
#include "ApplicationManager.h"
#include <QMetaMethod>
#include <QMetaObject>
#include "Preferences.h"

ApplicationDescription::ApplicationDescription()
    : m_splashIconName()
    , m_splashBackgroundName()
    , m_launchInNewGroup(false)
	, m_tapToShareSupported(false)
{
	m_isHeadLess = false;
	m_hasTransparentWindows = false;
	m_entryPoint = "index.html";
	m_miniIconName = "miniicon.png";
	m_category = "";
	m_version = std::string("1.0"); // default version
	m_executionLock = false;
	m_flaggedForRemoval = false;
	m_isRemovable = false;
	m_isUserHideable = false;
	m_progress = 0;
	m_isVisible	= true;
	m_hardwareFeaturesNeeded = HardwareFeaturesNeeded_None;
	m_type = Type_Web;
	m_status = Status_Ready;
	m_hasAccounts = false;
	m_dockMode = false;
	m_appSize = 0;
	m_fsBlockSize = 0;
	m_runtimeMemoryRequired = 0;
	m_universalSearchJsonStr = "";
	m_pBuiltin_launcher = 0;
	m_requestedWindowOrientation = "";
}

ApplicationDescription::~ApplicationDescription()
{
    for (LaunchPointList::const_iterator it = m_launchPoints.begin();
		 it != m_launchPoints.end(); ++it) {
		delete (*it);
	}
    if (m_pBuiltin_launcher)
    	delete m_pBuiltin_launcher;
}

ApplicationDescription* ApplicationDescription::fromFile(const std::string& filePath, const std::string& folderPath)
{
	bool success = false;
	ApplicationDescription* appDesc = 0;
	char* jsonStr = 0;
	const gchar* palmAppDirPrefix = "/usr/palm/applications/";
	std::vector<MimeRegInfo> extractedMimeTypes;
	std::string launchParams;
	
	std::string builtinEntrypt;
	std::string builtinArgs;

	jsonStr = readFile(filePath.c_str());
	if (!jsonStr || !g_utf8_validate(jsonStr, -1, NULL))
	{
		return 0;
	}

	struct json_object* root=0;
	struct json_object* label=0;
	
	std::string title, icon, dirPath;
	gchar* dirPathCStr;

	dirPathCStr = g_path_get_dirname(filePath.c_str());
	dirPath = dirPathCStr;
	dirPath += "/";
	g_free(dirPathCStr);
	
	root = json_tokener_parse( jsonStr );
	if( !root || is_error( root ) )
	{
		g_warning("%s: Failed to parse '%s' into a JSON string", __FUNCTION__, filePath.c_str() );
		goto Done;
	}
	
	appDesc = new ApplicationDescription();

	appDesc->m_folderPath = folderPath;

	// ID: mandatory
	label = json_object_object_get(root, "id");
	if( label && !is_error(label) )
	{
		appDesc->m_id = json_object_get_string(label);
	}
	else
	{
		g_warning("%s: App %s does not have an ID", __FUNCTION__, filePath.c_str() );
		goto Done;
	}
	

	// MAIN: optional
	label = json_object_object_get(root, "main");
	if( label && !is_error(label) )
	{
		appDesc->m_entryPoint = json_object_get_string(label);
	}
	else
	{
		appDesc->m_entryPoint = "index.html";
	}
	
	if (!strstr(appDesc->m_entryPoint.c_str(), "://"))
		appDesc->m_entryPoint = std::string("file://") + dirPath + appDesc->m_entryPoint;

	
	// TITLE: mandatory
	label = json_object_object_get(root, "title");
	if( label && !is_error(label) )
	{
		appDesc->m_title = json_object_get_string(label);
	}
	else
	{
		g_warning("%s: App %s does not have a title",__FUNCTION__, filePath.c_str() );
		goto Done;
	}

	// SHORT NAME: optional
	label = json_object_object_get(root,"appmenu");
	if ( label && !is_error(label))
	{
		appDesc->m_appmenuName = json_object_get_string(label);
	}
	else
		appDesc->m_appmenuName = appDesc->m_title;
	
	// KEYWORDS: optional
	label = json_object_object_get(root,"keywords");
	if ( label && !is_error(label)) {
		appDesc->m_keywords.addKeywords(label);
	}
	
	//MIME HANDLING REGISTRATIONS: optional
	label = json_object_object_get(root,"mimeTypes");
	if ( label && !is_error(label)) {
		if (utilExtractMimeTypes(label,extractedMimeTypes)) {
			//found some!
			for (std::vector<MimeRegInfo>::iterator it = extractedMimeTypes.begin();
				it != extractedMimeTypes.end();
				++it)
			{
				if ((*it).mimeType.size()) {
					// ADD BY MIME TYPE.  The extension that is appropriate for this mimeType will be automatically filled in into "extension" if successful
					if (MimeSystem::instance()->addResourceHandler((*it).extension,(*it).mimeType,!((*it).stream),appDesc->id(),NULL,false) > 0)
						appDesc->m_mimeTypes.push_back(ResourceHandler((*it).extension,(*it).mimeType,appDesc->id(),(*it).stream));			//success adding to mime system, so add it to this app descriptor for bookeeping purposes
				}
				else if ((*it).extension.size()) {
					// ADD BY EXTENSION... count on the extension->mime mapping to already exist, or this will fail
					if (MimeSystem::instance()->addResourceHandler((*it).extension,!((*it).stream),appDesc->id(),NULL,false) > 0) {
						//get the mime type
						MimeSystem::instance()->getMimeTypeByExtension((*it).extension,(*it).mimeType);
						appDesc->m_mimeTypes.push_back(ResourceHandler((*it).extension,(*it).mimeType,appDesc->id(),(*it).stream));
					}
				}
				else if ((*it).scheme.size()) {			//TODO: fix this so it's more robust; it should check if the way the appinfo file specified the scheme is in fact a valid "scheme form" regexp and if not, make it one
					// ADD REDIRECT: THIS IS A SCHEME or "COMMAND" FORM.... (e.g. "tel://")
					(*it).scheme = std::string("^")+(*it).scheme+std::string(":");
					if (MimeSystem::instance()->addRedirectHandler((*it).scheme,appDesc->id(),NULL,true,false) > 0) {
						appDesc->m_redirectTypes.push_back(RedirectHandler((*it).scheme,appDesc->id(),true));
					}
				}
				else if ((*it).urlPattern.size()) {
					// ADD REDIRECT: THIS IS A PURE REDIRECT FORM... (e.g. "^[^:]+://www.youtube.com/watch\\?v="
					if (MimeSystem::instance()->addRedirectHandler((*it).urlPattern,appDesc->id(),NULL,false,false) > 0) {
						appDesc->m_redirectTypes.push_back(RedirectHandler((*it).urlPattern,appDesc->id(),false));
					}
				}
			}
		}
	}
	
	// ICON: we have a default if this is not present.
	label = json_object_object_get(root, "icon");
	if( label && !is_error(label) )
	{
		icon = dirPath + json_object_get_string(label);
	}
	else 
		icon = dirPath + "icon.png";

	// Optional parameters
	success = true;

	// Type: optional (defaults to Type_Web)
	label = json_object_object_get(root, "type");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		if (strncmp(json_object_get_string(label), "game", 4) == 0)
			appDesc->m_type = Type_Native;
		else if (strncmp(json_object_get_string(label), "pdk", 3) == 0)
			appDesc->m_type = Type_PDK;
		else if (strncmp(json_object_get_string(label), "qt", 2) == 0)
			appDesc->m_type = Type_Qt;
		else if (strncmp(json_object_get_string(label), "sysmgrbuiltin" , 13 ) == 0)
			appDesc->m_type = Type_SysmgrBuiltin;
		else
			appDesc->m_type = Type_Web;
	}

	// SPLASH ICON: optional (Used for loading/splash screen for cards)
	label = json_object_object_get(root, "splashicon");
	if (label && !is_error(label)) {
		appDesc->m_splashIconName = dirPath + json_object_get_string(label);
	}
	// SPLASH BACKGROUND: optional (Used for loading/splash screen for cards)
	label = json_object_object_get(root, "splashBackground");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		appDesc->m_splashBackgroundName = dirPath + json_object_get_string(label);
	}
	else {
		label = json_object_object_get(root, "splashbackground");
		if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
			appDesc->m_splashBackgroundName = dirPath + json_object_get_string(label);
		}
	}

	// MINI ICON: optional (Used for notification banner area)
	label = json_object_object_get(root, "miniicon");
	if( label && !is_error(label) )
	{
		appDesc->m_miniIconName = json_object_get_string(label);
	}
	else
		appDesc->m_miniIconName = "miniicon.png";

	appDesc->m_miniIconName = dirPath + appDesc->m_miniIconName;

	// LAUNCH IN NEW GROUP: optional (Used to prevent app from launching in current card stack)
	label = json_object_object_get(root, "launchinnewgroup");
	if( label && !is_error(label) )
	{
		appDesc->m_launchInNewGroup = json_object_get_boolean(label);
	}
	else
		appDesc->m_launchInNewGroup = false;

	// CATEGORY: optional
	label = json_object_object_get(root, "category");
	if( label && !is_error(label) )
	{
		appDesc->m_category = json_object_get_string(label);
	}

	// VENDOR: optional
	label = json_object_object_get(root, "vendor");
	if( label && !is_error(label) )
	{
		appDesc->m_vendorName = json_object_get_string(label);
	}
	else if (g_str_has_prefix(dirPath.c_str(), palmAppDirPrefix)) {
		appDesc->m_vendorName = "Palm, Inc.";
	}
	
	// VENDOR URL: optional
	label = json_object_object_get(root, "vendorurl");
	if( label && !is_error(label) )
	{
		appDesc->m_vendorUrl = json_object_get_string(label);
	}

	// SIZE: optional
	label = json_object_object_get(root, "appsize");
	if( label && !is_error(label) )
	{
		appDesc->m_appSize = (unsigned int) json_object_get_int(label);
	}
	
	// RUNTIME MEMORY REQUIRED: optional
	label = json_object_object_get(root, "requiredMemory");
	if( label && !is_error(label) )
	{
		appDesc->m_runtimeMemoryRequired = (unsigned int) json_object_get_int(label);
		//json_object_put( label );
	}
	
	// HEADLESS: optional
	label = json_object_object_get(root, "noWindow");
	if( label && !is_error(label) )
	{
		appDesc->m_isHeadLess = (strcasecmp( json_object_get_string(label), "true") == 0);
	}

	//VISIBLE: optional* by default the launch icons are visible...set to false in the json and they won't show in the
	//launcher screen
	label = json_object_object_get(root, "visible");
	if( label && !is_error(label) )
	{
		if (json_object_is_type(label,json_type_string))
			appDesc->m_isVisible = (strcasecmp( json_object_get_string(label), "true") == 0);
		else
			appDesc->m_isVisible = json_object_get_boolean(label);
	}

	// TRANSPARENT: optional
	label = json_object_object_get(root, "transparent");
	if( label && !is_error(label) )
	{
		appDesc->m_hasTransparentWindows = (strcasecmp( json_object_get_string(label), "true") == 0);
	}

	// VERSION: optional?
	label = json_object_object_get(root, "version");
	if (label && !is_error(label)) {
		appDesc->m_version = json_object_get_string(label);
	}
	
	// additional attributes, like http proxy
    label = json_object_object_get(root, "attributes");
    if (label && !is_error(label)) {
        appDesc->m_attributes = json_object_get_string(label);
    }

	// REMOVABLE: optional
	label = json_object_object_get(root, "removable");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		// Any appinfo.json can set removable to true. But if you want to set removable to false you better be a trusted palm application
        // NOTE: we should always be able to trust the removable flag set in the appinfo
		appDesc->m_isRemovable = json_object_get_boolean(label);
	   	g_debug("%s: App %s is %s because of appinfo.json",__FUNCTION__, appDesc->m_id.c_str(), appDesc->m_isRemovable ? "removable" : "non-removable");
	}
    else {
        // apps in ROM are never removable
	    appDesc->m_isRemovable = !(folderPath.find("/usr") == 0);
        g_debug("%s: App %s is %s by default",__FUNCTION__, appDesc->m_id.c_str(), appDesc->m_isRemovable ? "removable" : "non-removable");
    }

	// DOCK ENABLED: optional (defines if this app can provide a Dock mode stage)
	label = json_object_object_get(root, "exhibitionMode");
	if (!label || is_error (label)) // maintaining backward compatibility
		label = json_object_object_get(root, "dockMode");

	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean))
	{
		appDesc->m_dockMode = json_object_get_boolean(label);
		if(appDesc->m_dockMode) {
			// read the optional Dock mode parameters
			struct json_object *dockOptions=0, *dockLabel=0;
						
			// DOCK Mode options: (optional)
			dockOptions = json_object_object_get(root, "exhibitionModeOptions");
			if (!dockOptions || is_error (dockOptions)) // maintaining backward compatibility
				dockOptions = json_object_object_get(root, "dockModeOptions");

			if (dockOptions && !is_error(dockOptions)) {
				dockLabel = json_object_object_get(dockOptions, "title");
				if (dockLabel && !is_error(dockLabel) && json_object_is_type(dockLabel, json_type_string)) {
					appDesc->m_dockModeTitle = json_object_get_string(dockLabel);
				}
				else {
					appDesc->m_dockModeTitle = appDesc->m_appmenuName;
				}
			}
			else {
				appDesc->m_dockModeTitle = appDesc->m_appmenuName;
			}
		}
	}

	// Hardware features needed: optional
	label = json_object_object_get(root, "hardwareFeaturesNeeded");
	if (label && !is_error(label) && json_object_is_type(label, json_type_array)) {

		for (int i = 0; i < json_object_array_length(label); i++) {

			struct json_object* entry = json_object_array_get_idx(label, i);
			if (!entry  || is_error(entry))
				continue;

			if (!json_object_is_type(entry, json_type_string))
				continue;

			const char* str = json_object_get_string(entry);

			if (strncasecmp(str, "wifi", 4) == 0)
				appDesc->m_hardwareFeaturesNeeded |= HardwareFeaturesNeeded_Wifi;
			else if (strncasecmp(str, "bluetooth", 9) == 0)
				appDesc->m_hardwareFeaturesNeeded |= HardwareFeaturesNeeded_Bluetooth;
			else if (strncasecmp(str, "compass", 7) == 0)
				appDesc->m_hardwareFeaturesNeeded |= HardwareFeaturesNeeded_Compass;
			else if (strncasecmp(str, "accelerometer", 13) == 0)
				appDesc->m_hardwareFeaturesNeeded |= HardwareFeaturesNeeded_Accelerometer;
		}
	}
	
	//Universal Search JSON objct: optional
	label = json_object_object_get(root, "universalSearch");
	if(label && !is_error(label)) {
		appDesc->m_universalSearchJsonStr = json_object_to_json_string(label);
	}

	// Services JSON array: optional
	label = json_object_object_get(root, "services");
	if (label && !is_error(label))
		appDesc->m_servicesJsonStr = json_object_to_json_string(label);

	// Accounts JSON array: optional
	label = json_object_object_get(root, "accounts");
	if (label && !is_error(label))
		appDesc->m_accountsJsonStr = json_object_to_json_string(label);

	// Launch params: optional
	label = json_object_object_get(root, "params");
	if (label && !is_error(label)) {
		if (appDesc->m_type == Type_Qt)
            launchParams = json_object_get_string(label);
        else
    		launchParams = json_object_to_json_string(label);
    }

	// Tap to Share Supported: optional
	label = json_object_object_get(root, "tapToShareSupported");
	if (label && !is_error(label)) {
		appDesc->m_tapToShareSupported = json_object_get_boolean(label);
	}

	// Requested Window Orientation: optional
	label = json_object_object_get(root, "requestedWindowOrientation");
	if( label && !is_error(label) && json_object_is_type(label, json_type_string))
	{
		appDesc->m_requestedWindowOrientation = json_object_get_string(label);
	}


	//check to see if it's a sysmgr-builtin
	if (appDesc->m_type == Type_SysmgrBuiltin)
	{
		//must have an entrypoint
		label = json_object_object_get(root,"entrypoint");
		if ((!label) || is_error(label))
		{
			g_warning("%s: App %s of type SysmgrBuiltin doesn't name an entrypoint",__FUNCTION__,appDesc->m_id.c_str());
			success = false;
			goto Done;
		}
		builtinEntrypt = json_object_get_string(label);
		label = json_object_object_get(root,"args");
		if (label && !is_error(label))
			builtinArgs = json_object_get_string(label);
		else
			builtinArgs = "";

		//update fields that can be localized   ...won't be done automatically because of sysmgr builtins being in a special location
		appDesc->updateSysmgrBuiltinWithLocalization();

		//try and create a launch helper
		if (appDesc->initSysmgrBuiltIn(ApplicationManager::instance(),builtinEntrypt,builtinArgs) == false)
		{
			//failed...something was specified wrong
			g_warning("%s: App %s cannot be formed into a sysmgrbuiltin: entry = [%s] , args = [%s]",
					__FUNCTION__,appDesc->m_id.c_str(),builtinEntrypt.c_str(),builtinArgs.c_str());
			success = false;
			goto Done;
		}
	}
Done:

	if( root && !is_error(root) )json_object_put(root);

	delete [] jsonStr;

	if (!success) {
		delete appDesc;
		return 0;
	}

	// Default launchpoint (with empty params)
	LaunchPoint * defaultLp = new LaunchPoint(appDesc,
			  appDesc->m_id,
			  appDesc->m_id + "_default",
			  appDesc->m_title, appDesc->m_appmenuName, icon, launchParams,appDesc->m_isRemovable);
	defaultLp->setAsDefault();
	appDesc->m_launchPoints.push_back(defaultLp);
	
	return appDesc;
}

ApplicationDescription* ApplicationDescription::fromApplicationStatus(const ApplicationStatus& appStatus, bool isUpdating)
{
	ApplicationDescription* appDesc = new ApplicationDescription();

	appDesc->m_id = appStatus.id;
	appDesc->m_version = appStatus.version;
	appDesc->m_vendorName = appStatus.vendor;
	appDesc->m_vendorUrl = appStatus.vendorUrl;
	appDesc->m_progress = appStatus.progress;
	appDesc->m_appmenuName = appStatus.title;
	
	if (appStatus.state == ApplicationStatus::State_Failed)
		appDesc->m_status = Status_Failed;
	else
		appDesc->m_status = isUpdating ? Status_Updating : Status_Installing;

	LaunchPoint* defaultLP = new LaunchPoint(appDesc,
			appDesc->m_id,
			appDesc->m_id + "_default",
			appStatus.title,
			appDesc->m_appmenuName,
			appStatus.iconPath,
			"",
			true);
	defaultLP->setAsDefault();
	appDesc->m_launchPoints.push_back(defaultLP);
	appDesc->setRemovable(true);

	return appDesc;
}


ApplicationDescription* ApplicationDescription::fromJsonString(const char* jsonStr)
{
	ApplicationDescription* appDesc = new ApplicationDescription();

	struct json_object* root=0;
	
	root = json_tokener_parse( jsonStr );
	if( !root || is_error( root ) )
	{
		fprintf( stderr, "ApplicationDescription::fromJsonString: Failed to parse string into a JSON string.\n" );
		return 0;
	}

	bool success = true;
	
	success &= extractFromJson(root, "id", appDesc->m_id);
	success &= extractFromJson(root, "category", appDesc->m_category);
	success &= extractFromJson(root, "main", appDesc->m_entryPoint);
	success &= extractFromJson(root, "version", appDesc->m_version);
	success &= extractFromJson(root, "splashicon", appDesc->m_splashIconName);
	success &= extractFromJson(root, "splashBackground", appDesc->m_splashBackgroundName);
	success &= extractFromJson(root, "miniicon", appDesc->m_miniIconName);
	success &= extractFromJson(root, "folderPath", appDesc->m_folderPath);
	success &= extractFromJson(root, "attributes", appDesc->m_attributes);
	success &= extractFromJson(root, "vendor", appDesc->m_vendorName);
	success &= extractFromJson(root, "vendorUrl", appDesc->m_vendorUrl);
	success &= extractFromJson(root, "appmenu", appDesc->m_appmenuName);

	success &= extractFromJson(root, "noWindow", appDesc->m_isHeadLess);
	success &= extractFromJson(root, "transparent", appDesc->m_hasTransparentWindows);
	success &= extractFromJson(root, "removable", appDesc->m_isRemovable);
	success &= extractFromJson(root, "userHideable", appDesc->m_isUserHideable);
	success &= extractFromJson(root, "visible", appDesc->m_isVisible);
	success &= extractFromJson(root, "launchinnewgroup", appDesc->m_launchInNewGroup);
	success &= extractFromJson(root, "requestedWindowOrientation", appDesc->m_requestedWindowOrientation);
	success &= extractFromJson(root, "tapToShareSupported", appDesc->m_tapToShareSupported);

	
	int temp;
	success &= extractFromJson(root, "hardwareFeaturesNeeded", temp);
	appDesc->m_hardwareFeaturesNeeded = (uint32_t)temp;

	success &= extractFromJson(root, "type", temp);
	appDesc->m_type = (Type)temp;
	
	success &= extractFromJson(root, "size", temp);
	appDesc->m_appSize = temp;
	
	success &= extractFromJson(root, "runtimeMemoryRequired", temp);
	appDesc->m_runtimeMemoryRequired = (unsigned int)temp;
	
	if( root && !is_error(root) )json_object_put(root);

	if(!success) {
		fprintf(stderr,"ApplicationDescription::fromJsonString : error decodeing app description JSON string.\n" );
		delete appDesc;
		return 0;
	}
	
	return appDesc;
}

ApplicationDescription* ApplicationDescription::fromNativeDockApp(const std::string& id, 
		const std::string& title, const std::string& version,
		const std::string& splashIcon, const std::string& splashBackgroundName,
		const std::string& miniicon, const std::string& vendor, const std::string& vendorUrl,
		const std::string& appmenu)
{
	ApplicationDescription* appDesc = new ApplicationDescription();

	appDesc->m_id = id;
	appDesc->m_title = title;
	appDesc->m_version = version;
	appDesc->m_splashIconName = splashIcon;
	appDesc->m_splashBackgroundName = splashBackgroundName;
	appDesc->m_miniIconName = miniicon;
	appDesc->m_vendorName = vendor;
	appDesc->m_vendorUrl = vendorUrl;
	appDesc->m_appmenuName = appmenu;
	appDesc->m_dockModeTitle = appmenu;

	appDesc->m_isHeadLess = false;
	appDesc->m_hasTransparentWindows = false;
	appDesc->m_isRemovable = false;
	appDesc->m_isUserHideable = true;
	appDesc->m_isVisible = true;
	appDesc->m_hardwareFeaturesNeeded = false;
	appDesc->m_type = Type_SysmgrBuiltin;

	return appDesc;

}

//static 
std::string ApplicationDescription::versionFromFile(const std::string& filePath, const std::string& folderPath)
{
	
	char* jsonStr = 0;
	jsonStr = readFile(filePath.c_str());
	if (!jsonStr || !g_utf8_validate(jsonStr, -1, NULL))
	{
		return "";
	}

	struct json_object* root=0;
	struct json_object* label=0;

	std::string version ="1.0";			///default from constructor
	
	root = json_tokener_parse( jsonStr );
	if( !root || is_error( root ) )
	{
		g_warning("%s: Failed to parse '%s' into a JSON string.\n",__FUNCTION__,filePath.c_str() );
		return version;
	}
	
	label = json_object_object_get(root, "version");
	if (label && !is_error(label)) {
		version = json_object_get_string(label);
	}
		
	json_object_put(root);
	return version;
	
}

const std::list<ResourceHandler>& ApplicationDescription::mimeTypes() const
{
    return m_mimeTypes;
}

const std::list<RedirectHandler>& ApplicationDescription::redirectTypes() const
{
	return m_redirectTypes;
}

const LaunchPointList& ApplicationDescription::launchPoints() const
{
    return m_launchPoints;
}

void ApplicationDescription::launchPoints(LaunchPointList& launchPointList) {		//copy version of launchPoints(). Useful for add/remove of stuff from the list

	launchPointList = m_launchPoints;
}

void ApplicationDescription::addLaunchPoint(LaunchPoint* lp)
{
	//to be sure that default launchpoints' "removable" field is synchronized with 
	//the "removable" field of the app (at least when it is being added), check consistency
	if (lp->isDefault())
		lp->setRemovable(this->m_isRemovable);
	
	m_launchPoints.push_back(lp);
}

const LaunchPoint* ApplicationDescription::findLaunchPoint(const std::string& lpId)
{
	for (LaunchPointList::const_iterator it = m_launchPoints.begin();
		 it != m_launchPoints.end(); ++it) {
		if (lpId == (*it)->launchPointId())
			return (*it);
	}

	return 0;
}

const LaunchPoint* ApplicationDescription::getDefaultLaunchPoint() const {

	for (LaunchPointList::const_iterator it = m_launchPoints.begin();
			 it != m_launchPoints.end(); ++it) {
			if ((*it)->isDefault())
				return (*it);
		}

		return 0;
}

//NOTE: unsafe to call directly! Launcher depends on getting events about these being gone.
void ApplicationDescription::removeLaunchPoint(const LaunchPoint* lp)
{
    m_launchPoints.remove(lp);
	delete lp;
}

json_object* ApplicationDescription::toJSON() const
{
	const LaunchPoint* defaultLP = m_launchPoints.front();

	json_object* json = json_object_new_object();
	json_object_object_add(json, (char*) "id",   json_object_new_string((char*) m_id.c_str()));
	json_object_object_add(json, (char*) "main", json_object_new_string((char*) m_entryPoint.c_str()));
	json_object_object_add(json, (char*) "version", json_object_new_string(m_version.c_str()));
	json_object_object_add(json, (char*) "category", json_object_new_string((char*) m_category.c_str()));
	json_object_object_add(json, (char*) "title", json_object_new_string((char*) defaultLP->title().c_str()));
	json_object_object_add(json, (char*) "appmenu",json_object_new_string((char*) defaultLP->menuName().c_str()));
	json_object_object_add(json, (char*) "vendor", json_object_new_string((char*) m_vendorName.c_str()));
	json_object_object_add(json, (char*) "vendorUrl", json_object_new_string((char*) m_vendorUrl.c_str()));
	json_object_object_add(json, (char*) "size", json_object_new_int((int)m_appSize));
	
	json_object_object_add(json, (char*) "icon", json_object_new_string((char*) defaultLP->iconPath().c_str()));
	json_object_object_add(json, (char*) "removable",json_object_new_boolean(this->isRemovable()));	//use isRemovable() instead of m_isRemovable in case "removability" logic changes
	json_object_object_add(json, (char*) "userInstalled",json_object_new_boolean(this->isRemovable() && !this->isUserHideable()));
	json_object_object_add(json, (char*) "hasAccounts",json_object_new_boolean(this->hasAccounts()));

	if(!m_universalSearchJsonStr.empty()) {
		json_object* universalSearch = json_tokener_parse(m_universalSearchJsonStr.c_str());
		if(universalSearch && !is_error(universalSearch)) {
			json_object_object_add(json, (char*) "universalSearch",universalSearch);
		}
	}

	if (!m_servicesJsonStr.empty())
		json_object_object_add(json, (char*) "services", json_object_new_string(m_servicesJsonStr.c_str()));

	if (!m_accountsJsonStr.empty())
		json_object_object_add(json, (char*) "accounts", json_object_new_string(m_accountsJsonStr.c_str()));

	json_object_object_add(json, (char*) "tapToShareSupported",json_object_new_boolean(this->tapToShareSupported()));	//use isRemovable() instead of m_isRemovable in case "removability" logic changes

	if (m_dockMode) {
		json_object_object_add(json, (char*) "dockMode", json_object_new_boolean(true));
		json_object_object_add(json, (char*) "exhibitionMode", json_object_new_boolean(true));
		json_object_object_add(json, (char*) "exhibitionModeTitle", json_object_new_string((char*) dockModeTitle().c_str()));
	}
	
	return json;
}

std::string ApplicationDescription::toString() const
{
	json_object * j = toJSON();
	std::string s = json_object_to_json_string(j);
	json_object_put(j);
	return s;
}

//DANGER DANGER! try not to mess with this
bool ApplicationDescription::setRemovable(bool v)
{
	bool pv=m_isRemovable;
	m_isRemovable=v;
	//mark the default launchpoint to match
	LaunchPoint* defaultLP = const_cast<LaunchPoint *>(getDefaultLaunchPoint());
	if (defaultLP) {
		defaultLP->setRemovable(v);
	}
	return pv;
}

bool ApplicationDescription::setVisible(bool v)
{
	bool pv=m_isVisible;
	m_isVisible=v;
	return pv;
}

bool ApplicationDescription::operator==(const ApplicationDescription& cmp) const {
	return (m_id == cmp.id());
}
bool ApplicationDescription::operator!=(const ApplicationDescription& cmp) const {
	return (m_id != cmp.id());
}

//TODO: this should eventually just be the == operator. However, for now, allow operator== to be a weaker comparison
//whereas this function will compare every *info* field (not counting internal fields like execution lock, etc)
bool ApplicationDescription::strictCompare(const ApplicationDescription& cmp) const {
	
	if (m_id != cmp.id())
		return false;
	if (m_category != cmp.m_category)
		return false;
	if (m_entryPoint != cmp.m_entryPoint)
		return false;
	if (cmp.m_isRemovable && (m_version != cmp.m_version))
		return false;
	if (m_mimeTypes !=  cmp.m_mimeTypes)
		return false;
	if (m_redirectTypes != cmp.m_redirectTypes)
		return false;
	if (m_folderPath !=  cmp.m_folderPath)
		return false;
	if (m_vendorName != cmp.m_vendorName)
		return false;
	if (m_vendorUrl != cmp.m_vendorUrl)
		return false;	
	if (m_isHeadLess != cmp.m_isHeadLess)
		return false;
	if (m_hasTransparentWindows != cmp.m_hasTransparentWindows)
		return false;
	if (m_isVisible != cmp.m_isVisible)
		return false;
	if (m_appSize != cmp.m_appSize)
		return false;
	if (m_tapToShareSupported != cmp.m_tapToShareSupported)
		return false;
	
	return true;
}

void ApplicationDescription::update(const ApplicationStatus& appStatus, bool isUpdating)
{
	// for now, just update progress and icon
	m_progress = appStatus.progress;
	
	if (appStatus.state == ApplicationStatus::State_Failed)
		m_status = Status_Failed;
	else
		m_status = isUpdating ? Status_Updating : Status_Installing;

	const LaunchPoint* lp = getDefaultLaunchPoint();
	if (appStatus.iconPath != lp->iconPath()) {
		const_cast<LaunchPoint*>(lp)->updateIconPath(appStatus.iconPath);
	}
}

//TODO: this is a poor-man's assignment operator (operator=). Should probably be rewritten as such
//TODO: won't update the icon; add that.
//returns > 0 on success
int ApplicationDescription::update(const ApplicationDescription& appDesc) 
{
	m_category = appDesc.m_category;
	m_entryPoint = appDesc.m_entryPoint;
	m_version = appDesc.m_version;
	m_folderPath = appDesc.m_folderPath;
	m_vendorName = appDesc.m_vendorName;
	m_vendorUrl = appDesc.m_vendorUrl;
	m_isHeadLess = appDesc.m_isHeadLess;
	m_hasTransparentWindows = appDesc.m_hasTransparentWindows;
	m_isVisible = appDesc.m_isVisible;
	m_appSize = appDesc.m_appSize;
	m_runtimeMemoryRequired = appDesc.m_runtimeMemoryRequired;
	m_miniIconName = appDesc.m_miniIconName;
	m_tapToShareSupported = appDesc.m_tapToShareSupported;
	m_universalSearchJsonStr = appDesc.m_universalSearchJsonStr;
	m_servicesJsonStr = appDesc.m_servicesJsonStr;
	m_accountsJsonStr = appDesc.m_accountsJsonStr;
	m_dockMode = appDesc.m_dockMode;
	m_dockModeTitle = appDesc.m_dockModeTitle;

	const LaunchPoint* lp = getDefaultLaunchPoint();
	const LaunchPoint* nlp = appDesc.getDefaultLaunchPoint();
	if ((lp) && (nlp))
	{
		if (!nlp->iconPath().empty()) //guard a little bit...you don't know where nlp has been.
		{
			const_cast<LaunchPoint*>(lp)->updateIconPath(nlp->iconPath());
		}
		if ((!nlp->title().empty()) && (nlp->title() != lp->title()))
		{
			const_cast<LaunchPoint*>(lp)->updateTitle(nlp->title());
		}
	}
	//WARNING...doesn't propagate changes automatically to launcher, or things that are holding a ref to this...
	//				currently this is done in the callers of update()
	// TODO: update m_keywords
	// TODO: update defaultLP->title
	return 1;
}

bool ApplicationDescription::doesMatchKeywordExact(const gchar* keyword) const
{
	return m_keywords.hasMatch(keyword, true);
}

bool ApplicationDescription::doesMatchKeywordPartial(const gchar* keyword) const
{
	return m_keywords.hasMatch(keyword, false);
}


/*
 * return >0 on success, 0 on error
 * 
 * rather than directly adding to the m_mimeTypes map, get the mime types as a list in case further processing should be done on them here
 * 
 * the object type in the vector is a pair such that:
 * pair.first = the mime type string (e.g. "application/binary")
 * pair.second = "streamable" flag....  true = this type should not be downloaded first (pass url directly to app), false = download first
 * 
 * TODO: "streamable" should be "shouldDownload" or something that better signifies intent of this flag
 * (but streamable is what is used now in the command-resource-handlers file)
 * 
 */
//static
int ApplicationDescription::utilExtractMimeTypes(struct json_object * jsonMimeTypeArray,std::vector<MimeRegInfo>& extractedMimeTypes)
{
	if (jsonMimeTypeArray == NULL || !json_object_is_type(jsonMimeTypeArray, json_type_array))
		return 0;

	json_object * label;
	
	/*
	"mimeTypes": [ 	{"mime":<string> , "stream":<bool> },
					{ "extension":<string> , "stream":<bool> },
					{ "urlPattern":<string;regexp> },
					{ "scheme":<string;regexp> }
				]
	*/
	
	int rc=0;
	for (int listIdx=0;listIdx<json_object_array_length(jsonMimeTypeArray);++listIdx) {
		struct json_object * jsonListObject = json_object_array_get_idx(jsonMimeTypeArray,listIdx);
		if (jsonListObject == NULL)
			continue;
		if (json_object_is_type(jsonListObject,json_type_object) == false)
			continue;
		MimeRegInfo mri;
		bool r = false;
		r |= extractFromJson(jsonListObject,"mime",mri.mimeType);
		r |= extractFromJson(jsonListObject,"extension",mri.extension);
		r |= extractFromJson(jsonListObject,"urlPattern",mri.urlPattern);
		r |= extractFromJson(jsonListObject,"scheme",mri.scheme);
		
		if ((label = JsonGetObject(jsonListObject,"stream")) == NULL)
			mri.stream = false;
		else
			mri.stream = json_object_get_boolean(label);
		
		if (r) {
			extractedMimeTypes.push_back(mri);
			++rc;
		}
	}

	return rc;
		
}


void ApplicationDescription::getAppDescriptionString(std::string &descString) const
{
	// Compose json string from the app description object  -------------------------------
	// This will only include the integer and string fields of App Description

	json_object* json = json_object_new_object();

	json_object_object_add(json, (char*) "id",   json_object_new_string((char*) m_id.c_str()));
	json_object_object_add(json, (char*) "category",   json_object_new_string((char*) m_category.c_str()));
	json_object_object_add(json, (char*) "main",   json_object_new_string((char*) m_entryPoint.c_str()));
	json_object_object_add(json, (char*) "version",   json_object_new_string((char*) m_version.c_str()));
	json_object_object_add(json, (char*) "noWindow",   json_object_new_boolean(m_isHeadLess));
	json_object_object_add(json, (char*) "splashicon", json_object_new_string((char*) m_splashIconName.c_str()));
	json_object_object_add(json, (char*) "splashBackground", json_object_new_string((char*) m_splashBackgroundName.c_str()));
	json_object_object_add(json, (char*) "miniicon", json_object_new_string((char*) m_miniIconName.c_str()));
	json_object_object_add(json, (char*) "transparent",   json_object_new_boolean(m_hasTransparentWindows));
	json_object_object_add(json, (char*) "folderPath", json_object_new_string((char*) m_folderPath.c_str()));
	json_object_object_add(json, (char*) "removable",   json_object_new_boolean(m_isRemovable));
	json_object_object_add(json, (char*) "userHideable",   json_object_new_boolean(m_isUserHideable));
	json_object_object_add(json, (char*) "visible",   json_object_new_boolean(m_isVisible));
	json_object_object_add(json, (char*) "hardwareFeaturesNeeded",   json_object_new_int(m_hardwareFeaturesNeeded));
	json_object_object_add(json, (char*) "type",   json_object_new_int(m_type));
	json_object_object_add(json, (char*) "attributes", json_object_new_string((char*) m_attributes.c_str()));
	json_object_object_add(json, (char*) "vendor", json_object_new_string((char*) m_vendorName.c_str()));
	json_object_object_add(json, (char*) "vendorUrl", json_object_new_string((char*) m_vendorUrl.c_str()));
	json_object_object_add(json, (char*) "size", json_object_new_int((int)m_appSize));
	json_object_object_add(json, (char*) "runtimeMemoryRequired", json_object_new_int((int)m_runtimeMemoryRequired));
	json_object_object_add(json, (char*) "appmenu",json_object_new_string((char*) m_appmenuName.c_str()));
	json_object_object_add(json, (char*) "launchinnewgroup", json_object_new_boolean(m_launchInNewGroup));
	json_object_object_add(json, (char*) "requestedWindowOrientation", json_object_new_string((char*) m_requestedWindowOrientation.c_str()));
	json_object_object_add(json, (char*) "tapToShareSupported",   json_object_new_boolean(m_tapToShareSupported));

	if (m_dockMode) {
		json_object_object_add(json, (char*) "dockMode", json_object_new_boolean(true));
		json_object_object_add(json, (char*) "exhibitionMode", json_object_new_boolean(true));
	}

	descString = json_object_to_json_string(json);

	json_object_put(json);
}

void desaturate(QImage &img)
{
	int length = img.byteCount();
	uchar* data = img.bits();

	int avg;

	for (int i = 0; i < length; i += 4) {
		avg = (data[i] + data[i+1] + data[i+2]) / 3;
		data[i]   = avg;
		data[i+1] = avg;
		data[i+2] = avg;
	}
}

QPixmap ApplicationDescription::miniIcon() const
{
	QPixmap pix(m_miniIconName.c_str());
	if (!pix.isNull()) {
		return pix;
	}

	// if there is no mini-icon, we will scale and desaturate the regular app icon
	// to get one

#ifndef FIX_FOR_QT
	m_miniIconName = std::string("*default");	//invalid filename so it'll never be confused with a "real" icon file
#endif
	int miniIconSize = Settings::LunaSettings()->positiveSpaceBottomPadding;

	const LaunchPoint* lp = m_launchPoints.front();

	QImage img = QImage(qFromUtf8Stl(lp->iconPath())).scaled(miniIconSize, miniIconSize,
		                Qt::IgnoreAspectRatio,
		                Qt::SmoothTransformation);
	desaturate(img);
	pix = QPixmap::fromImage(img);

	return pix;
}

void ApplicationDescription::startSysmgrBuiltIn(const std::string& jsonArgsString) const
{
	if (m_pBuiltin_launcher == NULL)
		return;
	m_pBuiltin_launcher->launch(jsonArgsString);
}

void ApplicationDescription::startSysmgrBuiltIn() const
{
	if (m_pBuiltin_launcher == NULL)
		return;
	m_pBuiltin_launcher->launch();
}

void ApplicationDescription::startSysmgrBuiltInNoArgs() const
{
	if (m_pBuiltin_launcher == NULL)
		return;
	m_pBuiltin_launcher->launchNoArgs();
}


bool ApplicationDescription::initSysmgrBuiltIn(QObject * pReceiver,const std::string& entrypt,const std::string& args)
{
	if (pReceiver == NULL)
		return false;

	const QMetaObject* metaObject = pReceiver->metaObject();
	bool v = false;
	for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
	{
		if (QString(metaObject->method(i).signature()).contains(QString::fromStdString(entrypt)))
		{
			//try and connect
			m_pBuiltin_launcher = new SysmgrBuiltinLaunchHelper(args);
			QString normSig = QMetaObject::normalizedSignature("signalEntry(const std::string&)");
			int idxSignal = m_pBuiltin_launcher->metaObject()->indexOfSignal(normSig.toAscii().data());
			int idxSlot = metaObject->indexOfSlot(metaObject->method(i).signature());
			v = QMetaObject::connect(m_pBuiltin_launcher,idxSignal,
							pReceiver,idxSlot);
			break;
		}
	}
	return v;
}

bool ApplicationDescription::securityChecksVerified()
{
    if (m_folderPath.length() <= m_id.length() || strcmp(m_folderPath.c_str() + m_folderPath.length() - m_id.length(), m_id.c_str()) != 0) {
        // we have some known exceptions...
		if ((m_id == "com.palm.app.bluetooth" && m_folderPath == "/usr/palm/applications/com.palm.app.bluetoothtab") ||
				(m_id == "com.palm.calculator" && m_folderPath == "/media/cryptofs/apps/usr/palm/applications/com.palm.app.calculator") ||
				(m_id == "com.palm.mojo-systemui" && m_folderPath == "/usr/palm/applications/mojo-systemui"))
        {
            return true;
        }
        g_critical("ApplicationDescription::securityChecksVerified: '%s' in '%s' has the id '%s' that doesn't match the folder name!", m_title.c_str(), m_folderPath.c_str(), m_id.c_str());
        return false;
    }
    return true;
}

void ApplicationDescription::updateSysmgrBuiltinWithLocalization()
{
	/*
	 * Needed due to localization assumptions about app locations.
	 * Localization for sysmgr builtins will be in the sysmgr localization dir, and some of it will be wrong (like icon location)
	 * So this is intended to scan and update the fields that can be localized
	 * Whenever the fields allowed in the special sysmgr builtin appinfo.json change (i.e. the schema of the file), this
	 * function should be revisited and checked and updated if necessary
	 *
	 * This will not change the default launchpoint, so it has be called before that gets created
	 */

	if (m_type != Type_SysmgrBuiltin)
		return;			//not a valid app descriptor to call this fn on

	// Do we have a locale setting
	std::string locale = Preferences::instance()->locale();

	// Look for the language/region specific appinfo.json

	if (locale.empty())
		return;			//nothing to do

	//sysmgr always seems to have lang_region style directories, so this is all that need to be checked
	std::string filePath = Settings::LunaSettings()->lunaSystemLocalePath
							+ std::string("/") + locale + std::string("/sysapps/") + m_id;

	char * jsonStr = readFile(filePath.c_str());
	if (!jsonStr || !g_utf8_validate(jsonStr, -1, NULL))
		return;		// probably no file there

	struct json_object* root=0;
	struct json_object* label=0;

	//the only 2 that are localizable in the special builtin appinfo.json
	root = json_tokener_parse( jsonStr );
	if( !root || is_error( root ) )
	{
		g_warning("%s: Failed to parse '%s' into a JSON string", __FUNCTION__, filePath.c_str() );
		delete [] jsonStr;
		return;
	}

	// TITLE
	label = json_object_object_get(root, "title");
	if( label && !is_error(label) )
	{
		m_title = json_object_get_string(label);
	}
	else
	{
		g_warning("%s: App %s does not have a localized title",__FUNCTION__, filePath.c_str() );
	}

	//CATEGORY
	label = json_object_object_get(root, "category");
	if( label && !is_error(label) )
	{
		m_category = json_object_get_string(label);

	}
	else
	{
		g_warning("%s: App %s does not have a localized category",__FUNCTION__, filePath.c_str() );
	}

	json_object_put(root);
	delete [] jsonStr;

}

