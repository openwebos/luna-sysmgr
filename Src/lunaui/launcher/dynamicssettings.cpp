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

#include "dynamicssettings.h"
#include <QString>

static const char* kSettingsFile = "/etc/palm/launcher3/dynamicsSettings.conf";
static const char* kSettingsFilePlatform = "/etc/palm/launcher3/dynamicsSettingss-platform.conf";

DynamicsSettings* DynamicsSettings::s_instance = 0;

#define KEY_UINTEGER(cat,name,var) \
{\
		quint32 _v;\
		GError* _error = 0;\
		_v=(quint32)g_key_file_get_integer(keyfile,cat,name,&_error);\
		if( !_error ) { var=_v; }\
		else { g_error_free(_error); }\
}

#define KEY_DOUBLE(cat,name,var) \
{\
	double _v;\
	GError* _error = 0;\
	_v=g_key_file_get_double(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_QREAL(cat,name,var) \
{\
	qreal _v;\
	GError* _error = 0;\
	_v=(qreal)g_key_file_get_double(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_CURVE(cat,name,var)									\
	{															\
		QEasingCurve::Type _v;													\
		GError* _error = 0;										\
		_v=(QEasingCurve::Type)g_key_file_get_integer(keyfile,cat,name,&_error);	\
		if( !_error ) { var=_v; }								\
		else { g_error_free(_error); }							\
	}

#define KEY_BOOLEAN(cat,name,var) \
{\
	gboolean _vb;\
	GError* _error = 0;\
	_vb=g_key_file_get_boolean(keyfile,cat,name,&_error);\
	if( !_error ) { var=_vb; }\
	else g_error_free(_error); \
}

#define KEY_QSTRING(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=QString((const char*)_vs); g_free(_vs); }\
	else g_error_free(_error); \
}

DynamicsSettings::DynamicsSettings()
:	snapbackAnimTime(250)
,	snapbackAnimCurve(QEasingCurve::InQuad)
, 	iconReorderSampleRate(4)
,	maxVelocityForSampling(1)
, 	timeNormalizationUnit(1)
, 	distanceMagFactor(3)
, 	iconReorderIconMoveAnimTime(300)
,	iconReorderIconMoveAnimCurve(QEasingCurve::InQuad)
,	alwaysWaitForIconReorderAnimToFinish(true)
,	iconReorderTrackedIconUseDistanceBasedAnimTime(true)
,	iconReorderTrackedIconMinAnimTime(200)
,	alphaIconMoveTrackedIconUseAnimation(true)
,	alphaIconMoveTrackedIconAnimType(IconAnimationType::Fade)
,	alphaIconMoveTrackedIconAnimCurve(QEasingCurve::InQuad)
,	alphaIconMoveTrackedIconAnimTime(300)
, 	pageScrollDelayMs(800)		//includes pageScrollAnimTime, below (- probably some error,so in reality, this is really min delay)
,   pageScrollAmount(150)
,	pageScrollAnimTime(300)
,	pagePanForIconMoveDelayMs(1500)
, 	iconInstallModeOpacity(0.5)
,  	appInfoDialogFadeInTime(400)
, 	appInfoDialogFadeOutTime(600)
,   launchFeedbackTimeout(3000)
{
	s_instance = this;

	readSettings(kSettingsFile);
	readSettings(kSettingsFilePlatform);
}

DynamicsSettings::~DynamicsSettings()
{
    s_instance = 0;
}

void DynamicsSettings::readSettings(const char* filePath)
{
	QString alphaTrAnimTypeName;
    GKeyFile* keyfile = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile, filePath,
								   G_KEY_FILE_NONE, NULL)) {
		goto Done;
	}

	KEY_UINTEGER("Animations","IconReorderIconMoveAnimTime",iconReorderIconMoveAnimTime);
	KEY_CURVE("Animations","IconReorderIconMoveAnimCurve",iconReorderIconMoveAnimCurve);
	
	KEY_UINTEGER("UserControl","IconReorderSampleRate",iconReorderSampleRate);
	if (iconReorderSampleRate == 0)
	{
		iconReorderSampleRate = 1;
	}

	KEY_BOOLEAN("Animations","AlwaysWaitForIconReorderAnimToFinish",alwaysWaitForIconReorderAnimToFinish);
	KEY_BOOLEAN("Animations","TrackedIconUseDistanceBasedAnimTime",iconReorderTrackedIconUseDistanceBasedAnimTime);
	KEY_UINTEGER("Animations","TrackedIconMinAnimTime",iconReorderTrackedIconMinAnimTime);

	KEY_BOOLEAN("Animations","UseAnimationForAlphaTrackedIconCancel",alphaIconMoveTrackedIconUseAnimation);
	KEY_QSTRING("Animations","AlphaTrackedIconCancelAnimationType",alphaTrAnimTypeName);
	if (!alphaTrAnimTypeName.isEmpty())
	{
		alphaIconMoveTrackedIconAnimType = animName2Type(alphaTrAnimTypeName);
	}
	KEY_UINTEGER("UserControl","PageScrollUnderIconMoveDelayMs",pageScrollDelayMs);
	KEY_UINTEGER("UserControl","PageVerticalAutoScrollAmountPx",pageScrollAmount);
	KEY_UINTEGER("Animations","PageVerticalAutoScrollAnimTime",pageScrollAnimTime);

	KEY_QREAL("IconRender","IconInstallModeOpacity",iconInstallModeOpacity);

	KEY_UINTEGER("Animations","AppInfoDialogFadeInAnimTime",appInfoDialogFadeInTime);
	KEY_UINTEGER("Animations","AppInfoDialogFadeOutAnimTime",appInfoDialogFadeOutTime);

	KEY_UINTEGER("UserControl","IconFeedbackTimeout", launchFeedbackTimeout);

	KEY_UINTEGER("UserControl","MaxVelocityForSampling",maxVelocityForSampling);
	KEY_UINTEGER("UserControl","VelocityTimeNormalizationUnit",timeNormalizationUnit);
	KEY_UINTEGER("UserControl","VelocityDistanceMagFactor",distanceMagFactor);

	KEY_UINTEGER("UserControl","PagePanForIconMoveDelayMs",pagePanForIconMoveDelayMs);
Done:

	if (keyfile) {
		g_key_file_free(keyfile);
	}
}

void DynamicsSettings::verify()
{
	//TODO: IMPLEMENT (unfinished)
	if (alphaIconMoveTrackedIconAnimType == IconAnimationType::INVALID)
	{
		alphaIconMoveTrackedIconAnimType = IconAnimationType::Fade;
	}
	iconInstallModeOpacity = qBound((qreal)0.0,(qreal)iconInstallModeOpacity,(qreal)1.0);
}

//static
IconAnimationType::Enum DynamicsSettings::animName2Type(const QString& name)
{
	//TODO: new types as needed
	return IconAnimationType::Fade;
}

//static
QString DynamicsSettings::animType2Name(IconAnimationType::Enum a)
{
	//TODO: new types as needed
	return QString("fade");
}
