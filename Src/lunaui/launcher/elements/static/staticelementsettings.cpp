/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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
#include "staticelementsettings.h"
#include <qglobal.h>

static const char* kSettingsFile = "/etc/palm/launcher3/static_element_settings.conf";
static const char* kSettingsFilePlatform = "/etc/palm/launcher3/static_element_settings-platform.conf";

StaticElementSettings* StaticElementSettings::s_instance = 0;

#define KEY_UINTEGER(cat,name,var) \
{\
		quint32 _v;\
		GError* _error = 0;\
		_v=(quint32)g_key_file_get_integer(keyfile,cat,name,&_error);\
		if( !_error ) { var=_v; }\
		else { g_error_free(_error); }\
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


StaticElementSettings::StaticElementSettings()
:
  horizLabeledDivider_useAlignmentGeom(false)
, horizLabeledDivider_alignmentHeightPx(10)
, horizLabeledDivider_labelToPixmapSpacingPx(4)
, horizLabeledDivider_labelFontSizePx(16)
, horizLabeledDivider_labelFontColor(QColor("#999999"))
, horizLabeledDivider_labelFontEmbolden(true)
{
	s_instance = this;

	readSettings(kSettingsFile);
	readSettings(kSettingsFilePlatform);

	verify();
}

StaticElementSettings::~StaticElementSettings()
{
    s_instance = 0;
}

void StaticElementSettings::readSettings(const char* filePath)
{
    GKeyFile* keyfile = g_key_file_new();
    qint32 v = INT_MIN;
    qreal vf = 0.0;
	if (!g_key_file_load_from_file(keyfile, filePath,
								   G_KEY_FILE_NONE, NULL)) {
		goto Done;
	}


	KEY_BOOLEAN("HorizontalLabeledDivider",		"UseAlignmentGeom",
													horizLabeledDivider_useAlignmentGeom);
	KEY_UINTEGER("HorizontalLabeledDivider",	"AlignmentAbsoluteGeomSizeHeightPx",
													horizLabeledDivider_alignmentHeightPx);

Done:

	if (keyfile) {
		g_key_file_free(keyfile);
	}
}

void StaticElementSettings::verify()
{
	//check and correct any inconsistencies in settings
}
