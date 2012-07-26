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

#include "layoutsettings.h"

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <cstdlib>

#include "page.h"

static const char* kSettingsFile = "/etc/palm/launcher3/layoutSettings.conf";
static const char* kSettingsFilePlatform = "/etc/palm/launcher3/layoutSettings-platform.conf";

#if 0

#define SETTINGS_TRACE(...) \
do { \
    fprintf(stdout, "layoutSettings:: " ); \
    fprintf(stdout, __VA_ARGS__); \
} while (0)

#else

#define SETTINGS_TRACE(...) (void)0

#endif


unsigned long _MemStringToBytes( const char* ptr );

LayoutSettings* LayoutSettings::s_settings = 0;

LayoutSettings::LayoutSettings()
:
  launcherSizePctScreenRelative(QSizeF(1.0,1.0))
, autoPageSize(true)
, pageSizePctLauncherRelative(QSizeF(1.0,0.95))
, pageVerticalBorderActivationAreaSizePx(QSize(20,20))
, pageHorizontalBorderActivationAreaSizePx(QSize(50,50))
, pageTopBorderActivationTimeoutMs(1000)
, pageBottomBorderActivationTimeoutMs(1000)
, pageLeftBorderActivationTimeoutMs(1000)
, pageRightBorderActivationTimeoutMs(1000)

, tabBarUseAbsoluteSize(true)
, tabBarSizePctLauncherRelative(QSizeF(1.0,0.05))
, tabBarHeightAbsolute(50)
, tabBarTabFontSizePx(16)
, tabBarTabFontEmbolden(true)
, tabBarSelectedTabFontColor(Qt::white)
, tabBarUnSelectedTabFontColor(QColor("#C8C8C8"))
, tabTextVerticalPosAdjust(0)

, doneButtonFontSizePx(15)
, doneButtonFontEmbolden(true)
, doneButtonFontColor(Qt::white)
, doneButtonTextVerticalPosAdjust(0)
, doneButtonPositionAdjust(QPoint(12,0))

, quickLaunchBarUseAbsoluteSize(true)
, quickLaunchBarSizePctScreenRelative(QSizeF(1.0,0.15))
, quickLaunchBarHeightAbsolute(100)
, quickLaunchBarLauncherAccessButtonOffsetPx(0,20) // for now, only the Y of this offset is being used
, quickLaunchItemAreaOffsetPx(0,65)
, quickLaunchMaxItems(5)

, centerUiVerticalOffset(0)

, searchPillWidth(588)
, searchPillInnerIconRightOffset(15)
, searchPillTopOffsetFromStatusBar(9)
, searchPillInnerTextAdjust(QPoint(0,-1))

, iconVisibleLocationOffsetPx(QPoint(0,-10))

, openglHatesOddNumbers(false)
{
	load(kSettingsFile);
	load(kSettingsFilePlatform);
	postLoad();

}

LayoutSettings::~LayoutSettings()
{
}

#define KEY_STRING(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=(const char*)_vs; g_free(_vs); }\
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

#define KEY_MEMORY_STRING(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=::_MemStringToBytes((const char*)_vs); g_free(_vs); }\
	else g_error_free(_error); \
}

#define KEY_BOOLEAN(cat,name,var) \
{\
	gboolean _vb;\
	GError* _error = 0;\
	_vb=g_key_file_get_boolean(keyfile,cat,name,&_error);\
	if( !_error ) { var=_vb; }\
	else g_error_free(_error); \
}

#define KEY_INTEGER(cat,name,var) \
{\
	int _v;\
	GError* _error = 0;\
	_v=g_key_file_get_integer(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_DOUBLE(cat,name,var) \
{\
	double _v;\
	GError* _error = 0;\
	_v=g_key_file_get_double(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEYS_SIZEF(cat,namew,nameh,var) \
{ \
	qreal _v; \
	GError * _error = 0;\
	_v = (qreal)g_key_file_get_double(keyfile,cat,namew,&_error);\
	if (!_error) { \
		var.setWidth(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
	_error = 0; \
	_v = (qreal)g_key_file_get_double(keyfile,cat,nameh,&_error);\
	if (!_error) { \
		var.setHeight(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
}

#define KEYS_SIZE(cat,namew,nameh,var) \
{ \
	qint32 _v; \
	GError * _error = 0;\
	_v = (qint32)g_key_file_get_integer(keyfile,cat,namew,&_error);\
	if (!_error) { \
		var.setWidth(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
	_error = 0; \
	_v = (qint32)g_key_file_get_integer(keyfile,cat,nameh,&_error);\
	if (!_error) { \
		var.setHeight(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
}

#define KEYS_POINT(cat,namew,nameh,var) \
{ \
	qint32 _v; \
	GError * _error = 0;\
	_v = (qint32)g_key_file_get_integer(keyfile,cat,namew,&_error);\
	if (!_error) { \
		var.setX(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
	_error = 0; \
	_v = (qint32)g_key_file_get_integer(keyfile,cat,nameh,&_error);\
	if (!_error) { \
		var.setY(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
}

#define KEY_COLOR(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs )  \
		{ 	var=QColor((const char*)_vs); \
			if (!var.isValid()) { var = QColor(Qt::white);} \
			g_free(_vs); \
		}\
	else g_error_free(_error); \
}


void LayoutSettings::load(const char* settingsFile)
{
	GKeyFile* keyfile;
	GKeyFileFlags flags;
	GError* error = 0;

	keyfile = g_key_file_new();
	if(!keyfile)
		return;
	flags = GKeyFileFlags( G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	if( !g_key_file_load_from_file( keyfile, settingsFile, flags, &error ) )
	{
		g_key_file_free( keyfile );
		if (error) g_error_free(error);
		return ;
	}

	KEYS_SIZEF("Main", "launcherWidthPctScreenRelative","launcherHeighPctScreenRelative",launcherSizePctScreenRelative);
	KEY_INTEGER("Main","centerUiVerticalOffset",centerUiVerticalOffset);
	KEY_INTEGER("Main","searchPillWidth",searchPillWidth);
	KEY_INTEGER("Main","searchPillInnerIconRightOffset",searchPillInnerIconRightOffset);
	KEY_INTEGER("Main","searchPillTopOffsetFromStatusBar",searchPillTopOffsetFromStatusBar);
	KEYS_POINT("Main","searchPillInnerTextHorizAdjust","searchPillInnerTextVertAdjust",searchPillInnerTextAdjust);
	KEY_BOOLEAN("Main","openglHatesOddNumbers",openglHatesOddNumbers);

	KEY_BOOLEAN("Tabs","tabBarUseAbsoluteSize",tabBarUseAbsoluteSize);
	KEYS_SIZEF("Tabs", "tabBarWidthPctLauncherRelative","tabBarHeightPctLauncherRelative",tabBarSizePctLauncherRelative);
	KEY_INTEGER("Tabs","tabBarHeightAbsolute",tabBarHeightAbsolute);
	KEY_INTEGER("Tabs","tabBarTabFontSizePx",tabBarTabFontSizePx);
	KEY_BOOLEAN("Tabs","tabBarTabFontEmbolden",tabBarTabFontEmbolden);
	KEY_COLOR("Tabs","tabBarSelectedTabFontColor",tabBarSelectedTabFontColor);
	KEY_COLOR("Tabs","tabBarUnSelectedTabFontColor",tabBarUnSelectedTabFontColor);
	KEY_INTEGER("Tabs","tabTextVerticalPosAdjust",tabTextVerticalPosAdjust);

	KEY_INTEGER("DoneButton","doneButtonFontSizePx",doneButtonFontSizePx);
	KEY_BOOLEAN("DoneButton","doneButtonFontEmbolden",doneButtonFontEmbolden);
	KEY_COLOR("DoneButton","doneButtonFontColor",doneButtonFontColor);
	KEY_INTEGER("DoneButton","doneButtonTextVerticalPosAdjust",doneButtonTextVerticalPosAdjust);
	KEYS_POINT("DoneButton",
			"doneButtonHorizontalPositionAdjustPx",
			"doneButtonVerticalPositionAdjustPx",
			doneButtonPositionAdjust);
	KEYS_SIZEF("Pages","pageWidthPctLauncherRelative","pageHeightPctLauncherRelative",pageSizePctLauncherRelative);
	KEY_BOOLEAN("Pages","autoPageSize",autoPageSize);

	KEYS_SIZE("Pages","pageLeftBorderActivationSizePx","pageRightBorderActivationSizePx",pageHorizontalBorderActivationAreaSizePx);
	KEYS_SIZE("Pages","pageTopActivationSizePx","pageBottomActivationSizePx",pageVerticalBorderActivationAreaSizePx);

	KEY_INTEGER("Pages","pageTopBorderActivationTimeoutMs",pageTopBorderActivationTimeoutMs);
	KEY_INTEGER("Pages","pageBottomBorderActivationTimeoutMs",pageBottomBorderActivationTimeoutMs);
	KEY_INTEGER("Pages","pageLeftBorderActivationTimeoutMs",pageLeftBorderActivationTimeoutMs);
	KEY_INTEGER("Pages","pageRightBorderActivationTimeoutMs",pageRightBorderActivationTimeoutMs);

	KEY_BOOLEAN("QuickLaunch","quickLaunchBarUseAbsoluteSize",quickLaunchBarUseAbsoluteSize);
	KEYS_SIZEF("QuickLaunch","quickLaunchBarWidthPctScreenRelative","quickLaunchBarHeightPctScreenRelative",quickLaunchBarSizePctScreenRelative);
	KEY_INTEGER("QuickLaunch","quickLaunchBarHeightAbsolute",quickLaunchBarHeightAbsolute);
	KEYS_POINT("QuickLaunch","quickLaunchBarLauncherAccessButtonHorizontalOffsetPx",
							"quickLaunchBarLauncherAccessButtonVerticalOffsetPx",
							quickLaunchBarLauncherAccessButtonOffsetPx);
	KEYS_POINT("QuickLaunch","quickLaunchItemAreaHorizontalOffsetPx",
							"quickLaunchItemAreaVerticalOffsetPx",
							quickLaunchItemAreaOffsetPx);
	KEY_INTEGER("QuickLaunch","quickLaunchMaxItems",quickLaunchMaxItems);

	KEYS_POINT("Icons","iconVisibleLocationHorizontalOffsetPx","iconVisibleLocationVerticalOffsetPx",iconVisibleLocationOffsetPx);

	g_key_file_free( keyfile );
}

void LayoutSettings::postLoad()
{
}

// Expands "1MB" --> 1048576, "2k" --> 2048, etc.
unsigned long _MemStringToBytes( const char* ptr )
{
	char number[32];
	unsigned long r = 0;
	const char* s= ptr;

	while( *ptr && !isalnum(*ptr) ) // skip whitespace
		ptr++;
	s=ptr;

	while( isdigit(*ptr) )
		ptr++;

	strncpy( number, s, (size_t)(ptr-s) );
	number[ptr-s]=0;

	r = (unsigned long)atol(number);
	switch(*ptr)
	{
	case 'M':
		r *= 1024 * 1024; break;
	case 'k':
	case 'K':
		r *= 1024 ; break;
	}

	return r;
}

