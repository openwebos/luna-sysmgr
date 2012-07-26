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
#include "iconlayoutsettings.h"
#include "Localization.h"
#include "QtUtils.h"
#include <qglobal.h>

//TODO: CRITICAL - fix file paths
static const char* kSettingsFile = "/etc/palm/launcher3/launcher_icon_layoutsettings.conf";
static const char* kSettingsFilePlatform = "/etc/palm/launcher3/launcher_icon_layout_settings-platform.conf";

IconLayoutSettings* IconLayoutSettings::s_instance = 0;

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


IconLayoutSettings::IconLayoutSettings()
:
 alphabetlayout_interAlphaRowSpaceInPixels(10)
, alphabetlayout_intraAlphaRowSpaceInPixels(10)
, alphabetlayout_maxIconsPerRow(7)
, alphabetlayout_iconHorizSpaceAdjustInPixels(0)
, alphabetlayout_rowLeftMarginInPixels(25)
, alphabetlayout_rowTopMarginInPixels(10)
, alphabetlayout_useFixedIconCellSize(true)
, alphabetlayout_fixedIconCellSize(QSize(128,128))
, alphabetlayout_rowDividerLeftOffsetPx(10)
, alphabetlayout_rowDividerTopOffsetPx(15)
, reorderablelayout_interRowSpaceInPixels(10)
, reorderablelayout_maxIconsPerRow(6)
, reorderablelayout_iconHorizSpaceAdjustInPixels(0)
, reorderablelayout_useFixedIconCellSize(true)
, reorderablelayout_fixedIconCellSize(128,128)
, reorderablelayout_rowLeftMarginInPixels(50)
, reorderablelayout_rowTopMarginInPixels(20)
, reorderablelayout_emptyPageText(fromStdUtf8(LOCALIZED("Tap and hold any app to drag it to this page.")))
, reorderablelayout_emptyPageTextBoxSize(800,200)
, reorderablelayout_emptyPageTextFontSizePx(24)
, reorderablelayout_emptyPageTextFontEmbolden(true)
, reorderablelayout_emptyPageTextFontColor(Qt::white)

, reorderablelayout_emptyPageTextOffsetFromCenter(0,-100)
, reorderablelayout_emptyPageIconOffsetFromCenter(0,0)
{
	s_instance = this;

	readSettings(kSettingsFile);
	readSettings(kSettingsFilePlatform);
}

IconLayoutSettings::~IconLayoutSettings()
{
    s_instance = 0;
}

void IconLayoutSettings::readSettings(const char* filePath)
{
    GKeyFile* keyfile = g_key_file_new();
    qint32 v = INT_MIN;
    qreal vf = 0.0;
	if (!g_key_file_load_from_file(keyfile, filePath,
								   G_KEY_FILE_NONE, NULL)) {
		goto Done;
	}
	KEY_UINTEGER("AlphabetLayout","InterAlphaRowSpaceInPixels",alphabetlayout_interAlphaRowSpaceInPixels);
	KEY_UINTEGER("AlphabetLayout","IntraAlphaRowSpaceInPixels",alphabetlayout_intraAlphaRowSpaceInPixels);
	KEY_UINTEGER("AlphabetLayout","MaxIconsPerRow",alphabetlayout_maxIconsPerRow);
	KEY_INTEGER("AlphabetLayout","IconHorizontalSpaceAdjustInPixels",alphabetlayout_iconHorizSpaceAdjustInPixels);
	KEY_UINTEGER("AlphabetLayout","RowLeftMarginInPixels",alphabetlayout_rowLeftMarginInPixels);
	KEY_UINTEGER("ReorderableLayout","RowTopMarginInPixels",alphabetlayout_rowTopMarginInPixels);

	KEY_BOOLEAN("AlphabetLayout","UseFixedIconCellSize",alphabetlayout_useFixedIconCellSize);
	KEYS_SIZE("AlphabetLayout","FixedIconCellWidth","FixedIconCellHeight",alphabetlayout_fixedIconCellSize);
	KEY_INTEGER("AlphabetLayout","RowDividerLeftOffsetPx",alphabetlayout_rowDividerLeftOffsetPx);
	KEY_INTEGER("AlphabetLayout","RowDividerTopOffsetPx",alphabetlayout_rowDividerTopOffsetPx);

	KEY_UINTEGER("ReorderableLayout","InterRowSpaceInPixels",reorderablelayout_interRowSpaceInPixels);
	KEY_UINTEGER("ReorderableLayout","MaxIconsPerRow",reorderablelayout_maxIconsPerRow);
	KEY_INTEGER("ReorderableLayout","IconHorizontalSpaceAdjustInPixels",reorderablelayout_iconHorizSpaceAdjustInPixels);

	KEY_BOOLEAN("ReorderableLayout","UseFixedIconCellSize",reorderablelayout_useFixedIconCellSize);
	KEYS_SIZE("ReorderableLayout","FixedIconCellWidth","FixedIconCellHeight",reorderablelayout_fixedIconCellSize);
	KEY_UINTEGER("ReorderableLayout","RowLeftMarginInPixels",reorderablelayout_rowLeftMarginInPixels);
	KEY_UINTEGER("ReorderableLayout","RowTopMarginInPixels",reorderablelayout_rowTopMarginInPixels);

	KEY_QSTRING("ReorderableLayout","EmptyText",reorderablelayout_emptyPageText);
	KEYS_SIZE("ReorderableLayout",
			"EmptyPageTextBoxWidthPx",
			"EmptyPageTextBoxHeightPx",
			reorderablelayout_emptyPageTextBoxSize);
	KEYS_POINT("ReorderableLayout",
			"EmptyPageTextBoxCenterHorizontalOffsetPx",
			"EmptyPageTextBoxCenterVerticalOffsetPx",
			reorderablelayout_emptyPageTextOffsetFromCenter);
	KEYS_POINT("ReorderableLayout",
			"EmptyPageIconCenterHorizontalOffsetPx",
			"EmptyPageIconCenterVerticalOffsetPx",
			reorderablelayout_emptyPageIconOffsetFromCenter);

	KEY_UINTEGER("ReorderableLayout",
			"EmptyPageTextFontSizePx",
			reorderablelayout_emptyPageTextFontSizePx);
	KEY_BOOLEAN("ReorderableLayout",
			"EmptyPageTextFontEmbolden",
			reorderablelayout_emptyPageTextFontEmbolden);
	KEY_COLOR("ReorderableLayout",
			"EmptyPageTextColor",
			reorderablelayout_emptyPageTextFontColor);
Done:

	if (keyfile) {
		g_key_file_free(keyfile);
	}
}

void IconLayoutSettings::verify()
{
	//TODO: IMPLEMENT
	// this is intended to make sure all specified settings are valid and consistent
	// if they aren't then they are to be set to valid, or at least SAFE defaults

}
