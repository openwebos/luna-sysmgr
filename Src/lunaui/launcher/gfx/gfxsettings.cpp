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

#include "gfxsettings.h"

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <cstdlib>

static const char* kSettingsFile = "/etc/palm/launcher3/graphics_settings.conf";
static const char* kSettingsFilePlatform = "/etc/palm/launcher3/graphics_settings-platform.conf";

#if 0

#define SETTINGS_TRACE(...) \
do { \
    fprintf(stdout, "graphicsSettings:: " ); \
    fprintf(stdout, __VA_ARGS__); \
} while (0)

#else

#define SETTINGS_TRACE(...) (void)0

#endif


unsigned long _MemStringToBytes( const char* ptr );

GraphicsSettings* GraphicsSettings::s_settings = 0;

#define DEFAULT_MAX_SIZE	10485760
// 10MB

GraphicsSettings::GraphicsSettings()
: totalCacheSizeLimitInBytes(DEFAULT_MAX_SIZE)
, atlasPagesExemptFromSizeLimit(true)
, allowedScaleUpPercentage(10)
, allowedScaleDownPercentage(5)
, allowedAsymmetricScalePercentage(5)
, forceSquarifyUsesMaxBoundSquare(false)
, dbg_dumpFunctionsWriteableDirectory("/tmp/diui_debug/")
, graphicsAssetBaseDirectory("/usr/palm/sysmgr/images/launcher3/")
, dbg_graphicsAssetBaseDirectory("/home/harvey/graphics_assets/dfishlauncher/test/images/")
, maxPixSize(1024,1024)			//the standard opengl max texture size is 1024x1024
, maxVCamPixSize(1024,1024)
, useFixedVCamPixSize(false)
, fixedVCamPixSize(1024,1024)
, goggleSize(240,160)
, vCamHorizontalFlip(false)
#if defined(TARGET_DEVICE)
, vCamVerticalFlip(true)
#else
, vCamVerticalFlip(false)
#endif
, dbgSaveVcamOutput(true)
, jupocInnerSpacing(2,2)
{
	load(kSettingsFile);
	load(kSettingsFilePlatform);
	postLoad();

}

GraphicsSettings::~GraphicsSettings()
{
}

#define KEY_STRING(cat,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,#var,&_error);\
	if( !_error && _vs ) { var=(const char*)_vs; g_free(_vs); }\
	else g_error_free(_error); \
}

#define KEY_STRING_EX(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=(const char*)_vs; g_free(_vs); }\
	else g_error_free(_error); \
}

#define KEY_MEMORY_STRING(cat,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,#var,&_error);\
	if( !_error && _vs ) { var=::__MemStringToBytes((const char*)_vs); g_free(_vs); }\
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

#define KEY_DOUBLE(cat,var) \
{\
	double _v;\
	GError* _error = 0;\
	_v=g_key_file_get_double(keyfile,cat,#var,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_UINTEGER(cat, var) \
{\
	quint32 _v; \
	GError* _error = 0; \
	_v=(quint32)g_key_file_get_integer(keyfile,cat,#var,&_error); \
	if( !_error ) { var=_v; } \
	else { g_error_free(_error); } \
}

#define KEY_UINTEGER_EX(cat,name,var) \
{\
	quint32 _v;\
	GError* _error = 0;\
	_v=(quint32)g_key_file_get_integer(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_INTEGER(cat, var) \
{\
	qint32 _v; \
	GError* _error = 0; \
	_v=(qint32)g_key_file_get_integer(keyfile,cat,#var,&_error); \
	if( !_error ) { var=_v; } \
	else { g_error_free(_error); } \
}

#define KEY_INTEGER_EX(cat,name,var) \
{\
	qint32 _v;\
	GError* _error = 0;\
	_v=(qint32)g_key_file_get_integer(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
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

void  GraphicsSettings::reload()
{
	load(m_settingsFile.toAscii().constData());
}

void GraphicsSettings::load(const char* settingsFile)
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

	m_settingsFile = QString(settingsFile);

	std::string tmps;

	KEY_UINTEGER(	"MemoryManagement",		totalCacheSizeLimitInBytes);
	KEY_BOOLEAN(	"MemoryManagement",		"atlasPagesExemptFromSizeLimit",atlasPagesExemptFromSizeLimit);
	KEY_UINTEGER(	"MemoryManagement",		allowedScaleUpPercentage);
	KEY_UINTEGER(	"MemoryManagement",		allowedScaleDownPercentage);
	KEY_BOOLEAN(	"MemoryManagement",		"forceSquarifyUsesMaxBoundSquare",forceSquarifyUsesMaxBoundSquare);

	tmps.clear();
	KEY_STRING_EX(		"Debug",	"dbg_dumpFunctionsWriteableDirectory",			tmps);
	if (!tmps.empty())
	{
		dbg_dumpFunctionsWriteableDirectory = QString::fromStdString(tmps);
	}
	tmps.clear();
	KEY_STRING_EX(		"Debug",	"dbg_graphicsAssetBaseDirectory",			tmps);
	if (!tmps.empty())
	{
		dbg_graphicsAssetBaseDirectory = QString::fromStdString(tmps);
	}
	tmps.clear();
	KEY_STRING_EX(		"FileLocations", "graphicsAssetBaseDirectory",		tmps);
	if (!tmps.empty())
	{
		graphicsAssetBaseDirectory = QString::fromStdString(tmps);
	}

	KEYS_SIZE(			"Limits", "maxPixmapWidth",	"maxPixmapHeight",		maxPixSize);

	KEYS_SIZE(			"Limits", "maxVCamPixWidth",	"maxVCamPixHeight",		maxVCamPixSize);

	KEY_BOOLEAN(		"Limits", "useFixedVCamPixSize",useFixedVCamPixSize);
	KEYS_SIZE(			"Limits", "fixedVCamPixWidth",	"fixedVCamPixHeight",		fixedVCamPixSize);

	KEYS_SIZE(			"Toys", "goggleSizeWidth",	"goggleSizeHeight",		goggleSize);

	KEY_BOOLEAN(		"Main", "vCamHorizontalFlip",	vCamHorizontalFlip);
	KEY_BOOLEAN(		"Debug", "dbgSaveVcamOutput" , dbgSaveVcamOutput);

	KEYS_SIZE(			"JUPOC", "JUPOCInnerSpacingWidth","JUPOCInnerSpacingHeight", jupocInnerSpacing);

	g_key_file_free( keyfile );
}

void GraphicsSettings::postLoad()
{
	g_mkdir_with_parents(dbg_dumpFunctionsWriteableDirectory.toUtf8().constData(),0755);
}

// Expands "1MB" --> 1048576, "2k" --> 2048, etc.
unsigned long __MemStringToBytes( const char* ptr )
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

