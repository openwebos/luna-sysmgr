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
#include "icongeometrysettings.h"
#include <qglobal.h>

static const char* kSettingsFile = "/etc/palm/launcher3/launcher_icon_geom_settings.conf";
static const char* kSettingsFilePlatform = "/etc/palm/launcher3/launcher_icon_geom_settings-platform.conf";

IconGeometrySettings* IconGeometrySettings::s_instance = 0;

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


IconGeometrySettings::IconGeometrySettings()
:
  useAbsoluteGeom(true)
, absoluteGeomSizePx(128,128)
, decoratorEdgeOffsetPx(6,6)
, labelVerticalSpacingPx(5)
, useAbsoluteLabelBoxGeom(true)
, labelBoxAbsoluteGeomSizePx(100,40)
, labelBoxProportionToGeom(1.0,0.5)
, useAbsoluteFrameGeom(true)
, frameBoxAbsoluteGeomSizePx(128,128)
, frameBoxProportionToGeom(0.8,0.8)
, useAbsoluteMainIconGeom(true)
, mainIconBoxAbsoluteGeomSizePx(64,64)
, mainIconBoxProportionToGeom(0.5,0.5)
, useAbsoluteRemoveDeleteDecoratorGeom(true)
, removeDeleteDecoratorBoxAbsoluteGeomSizePx(32,32)
, removeDeleteDecoratorBoxProportionToGeom(0.2,0.2)
, mainIconOffsetFromGeomOriginPx(0,-13)
, frameOffsetFromGeomOriginPx(0,0)
, feedbackOffsetFromGeomOriginPx(0,-13)
, useAbsoluteRemoveDeleteDecoratorOffsetFromGeomOrigin(true)
, removeDeleteDecoratorOffsetFromGeomOriginPx(-50,-47)
, useAbsoluteInstallStatusDecoratorGeom(true)
, installStatusDecoratorBoxAbsoluteGeomSizePx(32,32)
, installStatusDecoratorBoxProportionToGeom(0.2,0.2)
, useAbsoluteInstallStatusDecoratorOffsetFromGeomOrigin(true)
, installStatusDecoratorOffsetFromGeomOriginPx(50,-50)
, labelFontSizePx(14)
, labelFontColor(QColor(Qt::white))
, labelFontEmbolden(true)
, useAlignmentGeom(false)
, alignmentGeomSizePx(128,128)
{
	s_instance = this;

	readSettings(kSettingsFile);
	readSettings(kSettingsFilePlatform);

	verify();
}

IconGeometrySettings::~IconGeometrySettings()
{
    s_instance = 0;
}

void IconGeometrySettings::readSettings(const char* filePath)
{
    GKeyFile* keyfile = g_key_file_new();
    qint32 v = INT_MIN;
    qreal vf = 0.0;
	if (!g_key_file_load_from_file(keyfile, filePath,
								   G_KEY_FILE_NONE, NULL)) {
		goto Done;
	}

	KEY_BOOLEAN("Main","UseAbsoluteGeom",useAbsoluteGeom);
	KEYS_SIZE("Main","AbsoluteGeomSizeWidthPx",
								"AbsoluteGeomSizeHeightPx",
								absoluteGeomSizePx);

	KEY_BOOLEAN("Main","UseAbsoluteFrameGeom",useAbsoluteFrameGeom);
	KEYS_SIZE("Main",		"FrameAbsoluteGeomSizeWidthPx",
							"FrameAbsoluteGeomSizeHeightPx",
							frameBoxAbsoluteGeomSizePx);
	KEYS_SIZEF("Main",		"FrameBoxHorizontalProportionToTotalGeom",
							"FrameBoxVerticalProportionToTotalGeom",
							frameBoxProportionToGeom);

	KEY_BOOLEAN("Main","UseAbsoluteMainIconGeom",useAbsoluteMainIconGeom);
	KEYS_SIZE("Main",		"MainIconAbsoluteGeomSizeWidthPx",
							"MainIconAbsoluteGeomSizeHeightPx",
							mainIconBoxAbsoluteGeomSizePx);
	KEYS_SIZEF("Main",		"MainIconBoxHorizontalProportionToTotalGeom",
							"MainIconBoxVerticalProportionToTotalGeom",
							mainIconBoxProportionToGeom);

	KEY_BOOLEAN("Main","UseAlignmentGeom",useAlignmentGeom);
	KEYS_SIZE("Main",		"AlignmentAbsoluteGeomSizeWidthPx",
								"AlignmentAbsoluteGeomSizeHeightPx",
								alignmentGeomSizePx);

	KEY_BOOLEAN("Main","UseAbsoluteRemoveDeleteDecoratorGeom",useAbsoluteRemoveDeleteDecoratorGeom);
	KEYS_SIZE("Main",		"RemoveDeleteDecoratorBoxAbsoluteGeomWidthPx",
							"RemoveDeleteDecoratorBoxAbsoluteGeomHeightPx",
							removeDeleteDecoratorBoxAbsoluteGeomSizePx);
	KEYS_SIZEF("Main",		"RemoveDeleteDecoratorBoxHorizontalProportionToTotalGeom",
							"RemoveDeleteDecoratorBoxVerticalProportionToTotalGeom",
							removeDeleteDecoratorBoxProportionToGeom);
	KEY_BOOLEAN("Main","UseAbsoluteRemoveDeleteDecoratorOffsetFromGeomOrigin",useAbsoluteRemoveDeleteDecoratorOffsetFromGeomOrigin);
	KEYS_POINT("Main", "RemoveDeleteDecoratorHorizontalOffsetFromGeomOrigin",
					   "RemoveDeleteDecoratorVerticalOffsetFromGeomOrigin",
					   removeDeleteDecoratorOffsetFromGeomOriginPx);

	KEY_BOOLEAN("Main","UseAbsoluteInstallStatusDecoratorGeom",useAbsoluteInstallStatusDecoratorGeom);
	KEYS_SIZE("Main",		"InstallStatusDecoratorBoxAbsoluteGeomWidthPx",
							"InstallStatusDecoratorBoxAbsoluteGeomHeightPx",
							installStatusDecoratorBoxAbsoluteGeomSizePx);
	KEYS_SIZEF("Main",		"InstallStatusDecoratorBoxHorizontalProportionToTotalGeom",
							"InstallStatusDecoratorBoxVerticalProportionToTotalGeom",
							installStatusDecoratorBoxProportionToGeom);
	KEY_BOOLEAN("Main","UseAbsoluteInstallStatusDecoratorOffsetFromGeomOrigin",useAbsoluteInstallStatusDecoratorOffsetFromGeomOrigin);
	KEYS_POINT("Main", "InstallStatusDecoratorHorizontalOffsetFromGeomOrigin",
					   "InstallStatusDecoratorVerticalOffsetFromGeomOrigin",
					   installStatusDecoratorOffsetFromGeomOriginPx);

	KEY_BOOLEAN("Label",	"UseAbsoluteLabelBoxGeom",useAbsoluteLabelBoxGeom);
	KEYS_SIZE("Label",		"LabelBoxAbsoluteGeomWidthPx",
							"LabelBoxAbsoluteGeomHeightPx",
							labelBoxAbsoluteGeomSizePx);
	KEYS_SIZEF("Label",		"LabelBoxHorizontalProportionToTotalGeom",
							"LabelBoxVerticalProportionToTotalGeom",
							labelBoxProportionToGeom);

	KEY_UINTEGER("Label","LabelVerticalSpacingFromMainIcon",labelVerticalSpacingPx);

	KEY_UINTEGER("Label","FontSizePx",labelFontSizePx);
	KEY_COLOR("Label","FontColor",labelFontColor);
	KEY_BOOLEAN("Label","FontUsesBold",labelFontEmbolden);

	KEYS_SIZE("Decorators",	"EdgeOffsetHorizontalPx",
							"EdgeOffsetVerticalPx",
							decoratorEdgeOffsetPx);

	KEYS_POINT("Main","MainIconHorizontalOffsetFromGeomOriginPx",
					"MainIconVerticalOffsetFromGeomOriginPx",
					mainIconOffsetFromGeomOriginPx);

	KEYS_POINT("Main","FrameHorizontalOffsetFromGeomOriginPx",
					 "FrameVerticalOffsetFromGeomOriginPx",
					 frameOffsetFromGeomOriginPx);

Done:

	if (keyfile) {
		g_key_file_free(keyfile);
	}
}

void IconGeometrySettings::IconGeometrySettings::verify()
{
	//check and correct any inconsistencies in settings
}
