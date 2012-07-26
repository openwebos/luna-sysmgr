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


#include "TabletKeymap.h"

#include "KeyLocationRecorder.h"
#include "Localization.h"
#include "Logging.h"
#include "QtUtils.h"
#include "Utils.h"
#include "VirtualKeyboardPreferences.h"

#include <QFile>
#include <qdebug.h>

#include <pbnjson.hpp>

namespace Tablet_Keyboard {

#define KEY_1(w, k) { w, k, k, NULL }

#define NOKEY_1 { 0, cKey_None, cKey_None, NULL }
#define NOKEY_2 NOKEY_1, NOKEY_1
#define NOKEY_3 NOKEY_2, NOKEY_1
#define NOKEY_4 NOKEY_3, NOKEY_1
#define NOKEY_5 NOKEY_4, NOKEY_1
#define NOKEY_6 NOKEY_5, NOKEY_1

#define KEY_2(w, k, a) { w, k, a, NULL }
#define KEY_3(w, k, a, e) { w, k, a, e }

#define SPACE_SIZE 5

static UKey	sLanguageChoices_Extended[cKey_KeyboardComboChoice_Last - cKey_KeyboardComboChoice_First + 2] = { cKey_None };

static TabletKeymap::constUKeyArray sA_extended = { Qt::Key_A, Qt::Key_Agrave, Qt::Key_Aacute, Qt::Key_Acircumflex, Qt::Key_Atilde, Qt::Key_Adiaeresis, Qt::Key_Aring, UKey(0x00E6) /* LATIN SMALL LETTER AE æ */, UKey(0x00AA) /* FEMININE ORDINAL INDICATOR ª */, cKey_None };
static TabletKeymap::constUKeyArray sC_extended = { Qt::Key_C, Qt::Key_Ccedilla, UKey(0x0107) /* LATIN SMALL LETTER C WITH ACUTE ć */, Qt::Key_copyright, Qt::Key_cent, cKey_None };
static TabletKeymap::constUKeyArray sD_extended = { Qt::Key_D, UKey(0x00F0) /* LATIN SMALL LETTER ETH ð */, UKey(0x2020) /* DAGGER † */, UKey(0x2021) /* 	DOUBLE DAGGER ‡ */, cKey_None };
static TabletKeymap::constUKeyArray sE_extended = { Qt::Key_E, Qt::Key_Egrave, Qt::Key_Eacute, Qt::Key_Ecircumflex, Qt::Key_Ediaeresis, UKey(0x0119) /* LATIN SMALL LETTER E WITH OGONEK ę */, UKey(0x0113) /* LATIN SMALL LETTER E WITH MACRON ē */, cKey_None };
static TabletKeymap::constUKeyArray sG_extended = { Qt::Key_G, UKey(0x011F) /* LATIN SMALL LETTER G WITH BREVE ğ */, cKey_None };
static TabletKeymap::constUKeyArray sI_extended = { Qt::Key_I, Qt::Key_Igrave, Qt::Key_Iacute, Qt::Key_Icircumflex, Qt::Key_Idiaeresis, UKey(0x0130) /* LATIN CAPITAL LETTER I WITH DOT ABOVE İ  */, UKey(0x0131) /* LATIN SMALL LETTER DOTLESS I ı */, cKey_None };
static TabletKeymap::constUKeyArray sL_extended = { Qt::Key_L, UKey(0x00141) /* LATIN CAPITAL LETTER L WITH STROKE Ł */, cKey_None };
static TabletKeymap::constUKeyArray sM_extended = { Qt::Key_M, UKey(0x00B5) /* MICRO SIGN µ */, cKey_None };
static TabletKeymap::constUKeyArray sN_extended = { Qt::Key_N, UKey(0x00F1) /* LATIN SMALL LETTER N WITH TILDE ñ */, UKey(0x0144) /* LATIN SMALL LETTER N WITH ACUTE ń */, cKey_None };
static TabletKeymap::constUKeyArray sO_extended = { Qt::Key_O, Qt::Key_Ograve, Qt::Key_Oacute, Qt::Key_Ocircumflex, Qt::Key_Otilde, Qt::Key_Odiaeresis, Qt::Key_Ooblique, UKey(0x0151) /* LATIN SMALL LETTER O WITH DOUBLE ACUTE ő */,
											  UKey(0x0153) /* latin small letter œ */, UKey(0x00BA) /* MASCULINE ORDINAL INDICATOR º */, UKey(0x03C9) /* GREEK SMALL LETTER OMEGA ω */, cKey_None };
static TabletKeymap::constUKeyArray sP_extended = { Qt::Key_P /*, UKey(0x00B6) / * PILCROW SIGN ¶ */, UKey(0x00A7) /* SECTION SIGN § */, UKey(0x03C0) /* GREEK SMALL LETTER PI π */, cKey_None };
static TabletKeymap::constUKeyArray sR_extended = { Qt::Key_R, Qt::Key_registered, cKey_None };
static TabletKeymap::constUKeyArray sS_extended = { Qt::Key_S, UKey(0x0161) /* LATIN SMALL LETTER S WITH CARON š */, UKey(0x015E) /* LATIN CAPITAL LETTER S WITH CEDILLA ş */, Qt::Key_ssharp, UKey(0x03C3) /* GREEK SMALL LETTER SIGMA σ */, cKey_None };
static TabletKeymap::constUKeyArray sT_extended = { Qt::Key_T, UKey(0x2122) /* TRADE MARK SIGN ™ */, Qt::Key_THORN, cKey_None };
static TabletKeymap::constUKeyArray sU_extended = { Qt::Key_U, Qt::Key_Ugrave, Qt::Key_Uacute, Qt::Key_Ucircumflex, Qt::Key_Udiaeresis, UKey(0x0171) /* LATIN SMALL LETTER U WITH DOUBLE ACUTE ű */, cKey_None };
static TabletKeymap::constUKeyArray sY_extended = { Qt::Key_Y, Qt::Key_Yacute, Qt::Key_ydiaeresis, cKey_None };
static TabletKeymap::constUKeyArray sZ_extended = { Qt::Key_Z, UKey(0x017E) /* LATIN SMALL LETTER Z WITH CARON ž */, UKey(0x017A) /* LATIN SMALL LETTER Z WITH ACUTE ź */, UKey(0x017C) /* LATIN SMALL LETTER Z WITH DOT ABOVE ż */, cKey_None };

static TabletKeymap::constUKeyArray sQwerty1_extended = { Qt::Key_1, Qt::Key_Exclam, UKey(0x00B9) /* SUPERSCRIPT ONE ¹ */, UKey(0x00BC) /* VULGAR FRACTION ONE QUARTER ¼ */, UKey(0x00BD) /* VULGAR FRACTION ONE HALF ½ */, Qt::Key_exclamdown, cKey_None };
static TabletKeymap::constUKeyArray sQwerty2_extended = { Qt::Key_2, Qt::Key_At, UKey(0x00B2) /* SUPERSCRIPT TWO ² */, cKey_None };
static TabletKeymap::constUKeyArray sQwerty3_extended = { Qt::Key_3, Qt::Key_NumberSign, UKey(0x00B3) /* SUPERSCRIPT THREE ³ */, UKey(0x00BE) /* VULGAR FRACTION THREE QUARTERS ¾ */, cKey_None };
static TabletKeymap::constUKeyArray sQwerty4_extended = { Qt::Key_4, Qt::Key_Dollar, cKey_Euro, Qt::Key_sterling, Qt::Key_yen, Qt::Key_cent, UKey(0x00A4) /* CURRENCY SIGN ¤ */, cKey_None };
static TabletKeymap::constUKeyArray sQwerty5_extended = { Qt::Key_5, Qt::Key_Percent, UKey(0x2030) /* PER MILLE SIGN ‰ */, cKey_None };
static TabletKeymap::constUKeyArray sQwerty6_extended = { Qt::Key_6, Qt::Key_AsciiCircum, cKey_None };
static TabletKeymap::constUKeyArray sQwerty7_extended = { Qt::Key_7, Qt::Key_Ampersand, cKey_None };
static TabletKeymap::constUKeyArray sQwerty8_extended = { Qt::Key_8, Qt::Key_Asterisk, cKey_None };
static TabletKeymap::constUKeyArray sQwerty9_extended = { Qt::Key_9, Qt::Key_ParenLeft, Qt::Key_BracketLeft, Qt::Key_BraceLeft, cKey_None };
static TabletKeymap::constUKeyArray sQwerty0_extended = { Qt::Key_0, Qt::Key_ParenRight, Qt::Key_BracketRight, Qt::Key_BraceRight, cKey_None };

static TabletKeymap::constUKeyArray sHide_extended = { cKey_Resize_Tiny, cKey_Resize_Small, cKey_Resize_Default, cKey_Resize_Large, cKey_None };

/* UKey(0x2039) SINGLE LEFT-POINTING ANGLE QUOTATION MARK ‹ */
/* UKey(0x203A) SINGLE RIGHT-POINTING ANGLE QUOTATION MARK › */
/* UKey(0x00B7) MIDDLE DOT · */
/* UKey(0x201A) SINGLE LOW-9 QUOTATION MARK ‚ */
/* UKey(0x201E) DOUBLE LOW-9 QUOTATION MARK „ */
/* UKey(0x201B) SINGLE HIGH-REVERSED-9 QUOTATION MARK ‛ */
/* UKey(0x201F) DOUBLE HIGH-REVERSED-9 QUOTATION MARK ‟ */

static TabletKeymap::constUKeyArray sSingleAndDoubleQuote_extended = { Qt::Key_Apostrophe, Qt::Key_QuoteDbl, UKey(0x0060) /* GRAVE ACCENT ` */,
																 UKey(0x2018) /* LEFT SINGLE QUOTATION MARK ‘ */, UKey(0x2019) /* RIGHT SINGLE QUOTATION MARK ’ */,
																 UKey(0x201C) /* LEFT DOUBLE QUOTATION MARK “ */, UKey(0x201D) /* RIGHT DOUBLE QUOTATION MARK ” */,
																 UKey(0x00AB) /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK « */, UKey(0x00BB) /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK » */, cKey_None };
static TabletKeymap::constUKeyArray sPeriodQuestion_extended = { Qt::Key_Period, Qt::Key_Question, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, Qt::Key_questiondown, cKey_None };
static TabletKeymap::constUKeyArray sMinusUnderscore_extended = { Qt::Key_Minus, Qt::Key_Underscore, UKey(0x00B1) /* PLUS-MINUS SIGN ± */, UKey(0x00AC) /* NOT SIGN ¬ */, cKey_None };
static TabletKeymap::constUKeyArray sCommaSlash_extended = { Qt::Key_Comma, Qt::Key_Slash, Qt::Key_Backslash, cKey_None };

static TabletKeymap::constUKeyArray sURL_extended = { cKey_HTTPColonSlashSlash, cKey_HTTPSColonSlashSlash, cKey_WWW, cKey_None };
static TabletKeymap::constUKeyArray sQwerty_DotCom_Extended = { cKey_DotCom, cKey_DotNet, cKey_DotOrg, cKey_DotEdu, cKey_None };
static TabletKeymap::constUKeyArray sQwertz_DotCom_Extended = { cKey_DotCom, cKey_DotDe, cKey_DotNet, cKey_DotOrg, cKey_DotEdu, cKey_None };
static TabletKeymap::constUKeyArray sAzerty_DotCom_Extended = { cKey_DotCom, cKey_DotFr, cKey_DotNet, cKey_DotOrg, cKey_DotEdu, cKey_None };

// turn off backdoors for shipping version
#if SHIPPING_VERSION && !defined(TARGET_DESKTOP)
	#define sToggleLanguage_extended NULL
	#define sOptions NULL
#else
	static TabletKeymap::constUKeyArray sToggleLanguage_extended = { cKey_SwitchToQwerty, cKey_SwitchToAzerty, cKey_SwitchToQwertz, cKey_CreateDefaultKeyboards, cKey_ClearDefaultKeyboards, cKey_None };
	static TabletKeymap::constUKeyArray sOptions = { cKey_ToggleSuggestions, cKey_ShowXT9Regions, cKey_ShowKeymapRegions, cKey_StartStopRecording, cKey_ToggleSoundFeedback, cKey_None };
#endif

#define QWERTY_NUMBERS_10(w)		{ w, Qt::Key_1,			Qt::Key_Exclam,							sQwerty1_extended },\
									{ w, Qt::Key_2,			Qt::Key_At,								sQwerty2_extended },\
									{ w, Qt::Key_3,			Qt::Key_NumberSign,						sQwerty3_extended },\
									{ w, Qt::Key_4,			Qt::Key_Dollar,							sQwerty4_extended },\
									{ w, Qt::Key_5,			Qt::Key_Percent,						sQwerty5_extended },\
									{ w, Qt::Key_6,	 		Qt::Key_AsciiCircum,					sQwerty6_extended },\
									{ w, Qt::Key_7,			Qt::Key_Ampersand,						sQwerty7_extended },\
									{ w, Qt::Key_8,			Qt::Key_Asterisk,						sQwerty8_extended },\
									{ w, Qt::Key_9,			Qt::Key_ParenLeft,						sQwerty9_extended },\
									{ w, Qt::Key_0,			Qt::Key_ParenRight,						sQwerty0_extended }

#define QWERTY_TOP_10(w)			{ w, Qt::Key_Q,			Qt::Key_QuoteLeft,						NULL },\
									{ w, Qt::Key_W,			Qt::Key_AsciiTilde,						NULL },\
									{ w, Qt::Key_E,			cKey_Euro,								sE_extended },\
									{ w, Qt::Key_R,			Qt::Key_sterling,						sR_extended },\
									{ w, Qt::Key_T,			Qt::Key_Backslash,						sT_extended },\
									{ w, Qt::Key_Y,	 		Qt::Key_Bar,							sY_extended },\
									{ w, Qt::Key_U,			Qt::Key_BraceLeft,						sU_extended },\
									{ w, Qt::Key_I,			Qt::Key_BraceRight,						sI_extended },\
									{ w, Qt::Key_O,			Qt::Key_BracketLeft,					sO_extended },\
									{ w, Qt::Key_P,			Qt::Key_BracketRight,					sP_extended }

#define QWERTY_MID_9(w)				{ w, Qt::Key_A,			Qt::Key_Less,							sA_extended },\
									{ w, Qt::Key_S,			Qt::Key_Greater,						sS_extended },\
									{ w, Qt::Key_D,			Qt::Key_Equal,							sD_extended },\
									{ w, Qt::Key_F,			Qt::Key_Plus,							NULL },\
									{ w, Qt::Key_G,			UKey(0x00D7) /* multiplication sign */,	sG_extended },\
									{ w, Qt::Key_H,			UKey(0x00F7) /* division sign */,		NULL },\
									{ w, Qt::Key_J,			Qt::Key_degree,							NULL },\
									{ w, Qt::Key_K,			Qt::Key_Semicolon,						NULL },\
									{ w, Qt::Key_L,			Qt::Key_Colon,							sL_extended }

#define QWERTY_LOW_9(w)				{ w, Qt::Key_Z,			cKey_Emoticon_Smile,					sZ_extended },\
									{ w, Qt::Key_X,			cKey_Emoticon_Wink,						sOptions },\
									{ w, Qt::Key_C,			cKey_Emoticon_Frown,					sC_extended },\
									{ w, Qt::Key_V,			cKey_Emoticon_Cry,						NULL },\
									{ w, Qt::Key_B,			cKey_Emoticon_Yuck,						sToggleLanguage_extended },\
									{ w, Qt::Key_N,			cKey_Emoticon_Gasp,						sN_extended },\
									{ w, Qt::Key_M,			cKey_Emoticon_Heart,					sM_extended },\
									{ w, Qt::Key_Comma,		Qt::Key_Slash,							sCommaSlash_extended },\
									{ w, Qt::Key_Period,	Qt::Key_Question,						sPeriodQuestion_extended }

#define QWERTY_BOTTOM_ROW_DEFAULT \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(SPACE_SIZE, Qt::Key_Space),\
									NOKEY_1,\
									KEY_3(1, Qt::Key_Apostrophe, Qt::Key_QuoteDbl, sSingleAndDoubleQuote_extended),\
									KEY_3(1, Qt::Key_Minus, Qt::Key_Underscore, sMinusUnderscore_extended),\
									KEY_3(1, cKey_Hide, cKey_Hide, sHide_extended),\
									NOKEY_3

#define QWERTY_BOTTOM_ROW_URL \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									KEY_3(1, Qt::Key_Slash, Qt::Key_Slash, sURL_extended),\
									KEY_1(SPACE_SIZE - 2, Qt::Key_Space),\
									KEY_3(1, cKey_DotCom, cKey_DotCom, sQwerty_DotCom_Extended),\
									KEY_3(1, Qt::Key_Apostrophe, Qt::Key_QuoteDbl, sSingleAndDoubleQuote_extended),\
									KEY_3(1, Qt::Key_Minus, Qt::Key_Underscore, sMinusUnderscore_extended),\
									KEY_3(1, cKey_Hide, cKey_Hide, sHide_extended),\
									NOKEY_3

#define QWERTY_BOTTOM_ROW_EMAIL \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									KEY_1(1, Qt::Key_At),\
									KEY_1(SPACE_SIZE - 2, Qt::Key_Space),\
									KEY_3(1, cKey_DotCom, cKey_DotCom, sQwerty_DotCom_Extended),\
									KEY_3(1, Qt::Key_Apostrophe, Qt::Key_QuoteDbl, sSingleAndDoubleQuote_extended),\
									KEY_3(1, Qt::Key_Minus, Qt::Key_Underscore, sMinusUnderscore_extended),\
									KEY_3(1, cKey_Hide, cKey_Hide, sHide_extended),\
									NOKEY_3

static TabletKeymap::Layout sQwerty = {
	{ KEY_2(-0.5, Qt::Key_Q, Qt::Key_BracketLeft), QWERTY_NUMBERS_10(1), KEY_1(-0.5, Qt::Key_Backspace) },
	{ QWERTY_TOP_10(1), KEY_1(1, Qt::Key_Backspace), NOKEY_1 },
	{ KEY_2(-0.5, Qt::Key_A, Qt::Key_Less), QWERTY_MID_9(1), KEY_1(1.5, Qt::Key_Return), NOKEY_1 },
	{ KEY_1(1, Qt::Key_Shift), QWERTY_LOW_9(1), KEY_1(1, Qt::Key_Shift), NOKEY_1 },
	{ QWERTY_BOTTOM_ROW_DEFAULT },
};

static TabletKeymap::LayoutRow sQwertyBottomRow_default = { QWERTY_BOTTOM_ROW_DEFAULT };
static TabletKeymap::LayoutRow sQwertyBottomRow_url = { QWERTY_BOTTOM_ROW_URL };
static TabletKeymap::LayoutRow sQwertyBottomRow_email = { QWERTY_BOTTOM_ROW_EMAIL };

static TabletKeymap::constUKeyArray sQwertz1_extended = { Qt::Key_1, Qt::Key_Exclam, UKey(0x00B9) /* SUPERSCRIPT ONE ¹ */, UKey(0x00BC) /* VULGAR FRACTION ONE QUARTER ¼ */, UKey(0x00BD) /* VULGAR FRACTION ONE HALF ½ */, Qt::Key_exclamdown, cKey_None };
static TabletKeymap::constUKeyArray sQwertz2_extended = { Qt::Key_2, Qt::Key_QuoteDbl, UKey(0x00B2) /* SUPERSCRIPT TWO ² */, UKey(0x201D) /* RIGHT DOUBLE QUOTATION MARK ” */, UKey(0x201E) /* DOUBLE LOW-9 QUOTATION MARK „ */, UKey(0x201C) /* LEFT DOUBLE QUOTATION MARK “ */, UKey(0x00AB) /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK « */, UKey(0x00BB) /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK » */, cKey_None };
static TabletKeymap::constUKeyArray sQwertz3_extended = { Qt::Key_3, Qt::Key_At, UKey(0x00B3) /* SUPERSCRIPT THREE ³ */, UKey(0x00BE) /* VULGAR FRACTION THREE QUARTERS ¾ */, cKey_None };
static TabletKeymap::constUKeyArray sQwertz4_extended = { Qt::Key_4, Qt::Key_Dollar, cKey_Euro, Qt::Key_sterling, Qt::Key_yen, Qt::Key_cent, UKey(0x00A4) /* CURRENCY SIGN ¤ */, cKey_None };
static TabletKeymap::constUKeyArray sQwertz5_extended = { Qt::Key_5, Qt::Key_Percent, UKey(0x2030) /* PER MILLE SIGN ‰ */, cKey_None };
static TabletKeymap::constUKeyArray sQwertz6_extended = { Qt::Key_6, Qt::Key_Ampersand, cKey_None };
static TabletKeymap::constUKeyArray sQwertz7_extended = { Qt::Key_7, Qt::Key_Slash, Qt::Key_Backslash, cKey_None };
static TabletKeymap::constUKeyArray sQwertz8_extended = { Qt::Key_8, Qt::Key_ParenLeft, Qt::Key_BracketLeft, Qt::Key_BraceLeft, cKey_None };
static TabletKeymap::constUKeyArray sQwertz9_extended = { Qt::Key_9, Qt::Key_ParenRight, Qt::Key_BracketRight, Qt::Key_BraceRight, cKey_None };
static TabletKeymap::constUKeyArray sQwertz0_extended = { Qt::Key_0, Qt::Key_Equal, cKey_None };

static TabletKeymap::constUKeyArray sCommaSemiColon_extended = { Qt::Key_Comma, Qt::Key_Semicolon, cKey_None };
static TabletKeymap::constUKeyArray sPeriodColon_extended = { Qt::Key_Period, Qt::Key_Colon, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, cKey_None };
static TabletKeymap::constUKeyArray sSSharpQuestion_extended = { Qt::Key_ssharp, Qt::Key_Question, Qt::Key_questiondown, cKey_None };
static TabletKeymap::constUKeyArray sMinusApostrophe_extended = { Qt::Key_Minus, Qt::Key_Apostrophe, UKey(0x00B1) /* PLUS-MINUS SIGN ± */, UKey(0x00AC) /* NOT SIGN ¬ */, UKey(0x0060) /* GRAVE ACCENT ` */, UKey(0x201A) /* SINGLE LOW-9 QUOTATION MARK ‚ */, UKey(0x2018) /* LEFT SINGLE QUOTATION MARK ‘ */, UKey(0x2019) /* RIGHT SINGLE QUOTATION MARK ’ */, cKey_None };

#define QWERTZ_NUMBERS_10(w)		{ w, Qt::Key_1,			Qt::Key_Exclam,							sQwertz1_extended },\
									{ w, Qt::Key_2,			Qt::Key_QuoteDbl,						sQwertz2_extended },\
									{ w, Qt::Key_3,			Qt::Key_At,								sQwertz3_extended },\
									{ w, Qt::Key_4,			Qt::Key_Dollar,							sQwertz4_extended },\
									{ w, Qt::Key_5,			Qt::Key_Percent,						sQwertz5_extended },\
									{ w, Qt::Key_6,	 		Qt::Key_Ampersand,						sQwertz6_extended },\
									{ w, Qt::Key_7,			Qt::Key_Slash,							sQwertz7_extended },\
									{ w, Qt::Key_8,			Qt::Key_ParenLeft,						sQwertz8_extended },\
									{ w, Qt::Key_9,			Qt::Key_ParenRight,						sQwertz9_extended },\
									{ w, Qt::Key_0,			Qt::Key_Equal,							sQwertz0_extended }

#define QWERTZ_TOP_10(w)			{ w, Qt::Key_Q,			Qt::Key_QuoteLeft,						NULL },\
									{ w, Qt::Key_W,			Qt::Key_AsciiTilde,						NULL },\
									{ w, Qt::Key_E,			cKey_Euro,								sE_extended },\
									{ w, Qt::Key_R,			Qt::Key_AsciiCircum,					sR_extended },\
									{ w, Qt::Key_T,			Qt::Key_Backslash,						sT_extended },\
									{ w, Qt::Key_Z,	 		Qt::Key_Bar,							sZ_extended },\
									{ w, Qt::Key_U,			Qt::Key_BraceLeft,						sU_extended },\
									{ w, Qt::Key_I,			Qt::Key_BraceRight,						sI_extended },\
									{ w, Qt::Key_O,			Qt::Key_BracketLeft,					sO_extended },\
									{ w, Qt::Key_P,			Qt::Key_BracketRight,					sP_extended }

#define QWERTZ_MID_9(w)				{ w, Qt::Key_A,			Qt::Key_Less,							sA_extended },\
									{ w, Qt::Key_S,			Qt::Key_Greater,						sS_extended },\
									{ w, Qt::Key_D,			Qt::Key_Underscore,						sD_extended },\
									{ w, Qt::Key_F,			Qt::Key_Plus,							NULL },\
									{ w, Qt::Key_G,			UKey(0x00D7) /* multiplication sign */,	sG_extended },\
									{ w, Qt::Key_H,			UKey(0x00F7) /* division sign */,		NULL },\
									{ w, Qt::Key_J,			Qt::Key_degree,							NULL },\
									{ w, Qt::Key_K,			Qt::Key_Asterisk,						NULL },\
									{ w, Qt::Key_L,			Qt::Key_NumberSign,						sL_extended }

#define QWERTZ_LOW_9(w)				{ w, Qt::Key_Y,			cKey_Emoticon_Smile,					sY_extended },\
									{ w, Qt::Key_X,			cKey_Emoticon_Wink,						sOptions },\
									{ w, Qt::Key_C,			cKey_Emoticon_Frown,					sC_extended },\
									{ w, Qt::Key_V,			cKey_Emoticon_Cry,						NULL },\
									{ w, Qt::Key_B,			cKey_Emoticon_Yuck,						sToggleLanguage_extended },\
									{ w, Qt::Key_N,			cKey_Emoticon_Gasp,						sN_extended },\
									{ w, Qt::Key_M,			cKey_Emoticon_Heart,					sM_extended },\
									{ w, Qt::Key_Comma, 	Qt::Key_Semicolon,						sCommaSemiColon_extended },\
									{ w, Qt::Key_Period,	Qt::Key_Colon,							sPeriodColon_extended }

#define QWERTZ_BOTTOM_ROW_DEFAULT \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(SPACE_SIZE, Qt::Key_Space),\
									NOKEY_1,\
									KEY_3(1, Qt::Key_ssharp, Qt::Key_Question, sSSharpQuestion_extended),\
									KEY_3(1, Qt::Key_Minus, Qt::Key_Apostrophe, sMinusApostrophe_extended),\
									KEY_3(1, cKey_Hide, cKey_Hide, sHide_extended),\
									NOKEY_3

#define QWERTZ_BOTTOM_ROW_URL \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									KEY_1(1, Qt::Key_Slash),\
									KEY_1(SPACE_SIZE - 2, Qt::Key_Space),\
									KEY_3(1, cKey_DotCom, cKey_DotCom, sQwertz_DotCom_Extended),\
									KEY_3(1, Qt::Key_ssharp, Qt::Key_Question, sSSharpQuestion_extended),\
									KEY_3(1, Qt::Key_Minus, Qt::Key_Apostrophe, sMinusApostrophe_extended),\
									KEY_3(1, cKey_Hide, cKey_Hide, sHide_extended),\
									NOKEY_3

#define QWERTZ_BOTTOM_ROW_EMAIL \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									KEY_1(1, Qt::Key_At),\
									KEY_1(SPACE_SIZE - 2, Qt::Key_Space),\
									KEY_3(1, cKey_DotCom, cKey_DotCom, sQwertz_DotCom_Extended),\
									KEY_3(1, Qt::Key_ssharp, Qt::Key_Question, sSSharpQuestion_extended),\
									KEY_3(1, Qt::Key_Minus, Qt::Key_Apostrophe, sMinusApostrophe_extended),\
									KEY_3(1, cKey_Hide, cKey_Hide, sHide_extended),\
									NOKEY_3

static TabletKeymap::Layout sQwertz = {
	{ KEY_2(-0.5, Qt::Key_Q, Qt::Key_BracketLeft), QWERTZ_NUMBERS_10(1), KEY_1(-0.5, Qt::Key_Backspace) },
	{ QWERTZ_TOP_10(1), KEY_1(1, Qt::Key_Backspace), NOKEY_1 },
	{ KEY_2(-0.5, Qt::Key_A, Qt::Key_Less), QWERTZ_MID_9(1), KEY_1(1.5, Qt::Key_Return), NOKEY_1 },
	{ KEY_1(1, Qt::Key_Shift), QWERTZ_LOW_9(1), KEY_1(1, Qt::Key_Shift), NOKEY_1 },
	{ QWERTZ_BOTTOM_ROW_DEFAULT },
};

static TabletKeymap::LayoutRow sQwertzBottomRow_default = { QWERTZ_BOTTOM_ROW_DEFAULT };
static TabletKeymap::LayoutRow sQwertzBottomRow_url = { QWERTZ_BOTTOM_ROW_URL };
static TabletKeymap::LayoutRow sQwertzBottomRow_email = { QWERTZ_BOTTOM_ROW_EMAIL };

static TabletKeymap::constUKeyArray sAzert1_extended = { Qt::Key_1, Qt::Key_Ampersand, UKey(0x00B9) /* SUPERSCRIPT ONE ¹ */, UKey(0x00BC) /* VULGAR FRACTION ONE QUARTER ¼ */, UKey(0x00BD) /* VULGAR FRACTION ONE HALF ½ */, cKey_None };
static TabletKeymap::constUKeyArray sAzert2_extended = { Qt::Key_2, Qt::Key_Eacute, UKey(0x00B2) /* SUPERSCRIPT TWO ² */, cKey_None };
static TabletKeymap::constUKeyArray sAzert3_extended = { Qt::Key_3, Qt::Key_QuoteDbl, UKey(0x00B3) /* SUPERSCRIPT THREE ³ */, UKey(0x00BE) /* VULGAR FRACTION THREE QUARTERS ¾ */,
												   UKey(0x201C) /* LEFT DOUBLE QUOTATION MARK “ */, UKey(0x201D) /* RIGHT DOUBLE QUOTATION MARK ” */,
												   UKey(0x00AB) /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK « */, UKey(0x00BB) /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK » */, cKey_None };
static TabletKeymap::constUKeyArray sAzert4_extended = { Qt::Key_4, Qt::Key_Apostrophe, UKey(0x2018) /* LEFT SINGLE QUOTATION MARK ‘ */, UKey(0x2019) /* RIGHT SINGLE QUOTATION MARK ’ */, cKey_None };
static TabletKeymap::constUKeyArray sAzert5_extended = { Qt::Key_5, Qt::Key_ParenLeft, Qt::Key_BracketLeft, Qt::Key_BraceLeft, cKey_None };
static TabletKeymap::constUKeyArray sAzert6_extended = { Qt::Key_6, Qt::Key_Minus, UKey(0x00B1) /* PLUS-MINUS SIGN ± */, UKey(0x00AC) /* NOT SIGN ¬ */, cKey_None };
static TabletKeymap::constUKeyArray sAzert7_extended = { Qt::Key_7, Qt::Key_Egrave, Qt::Key_QuoteLeft, cKey_None };
static TabletKeymap::constUKeyArray sAzert8_extended = { Qt::Key_8, Qt::Key_ParenRight, Qt::Key_BracketRight, Qt::Key_BraceRight, cKey_None };
static TabletKeymap::constUKeyArray sAzert9_extended = { Qt::Key_9, Qt::Key_Ccedilla, Qt::Key_cent, Qt::Key_Dollar, cKey_Euro, Qt::Key_sterling, Qt::Key_yen, UKey(0x00A4) /* CURRENCY SIGN ¤ */, cKey_None };
static TabletKeymap::constUKeyArray sAzert0_extended = { Qt::Key_0, Qt::Key_Agrave, Qt::Key_Percent, UKey(0x2030) /* PER MILLE SIGN ‰ */, cKey_None };

static TabletKeymap::constUKeyArray sCommaQuestion_extended = { Qt::Key_Comma, Qt::Key_Question, Qt::Key_questiondown, cKey_None };
static TabletKeymap::constUKeyArray sSemicolonPeriod_extended = { Qt::Key_Semicolon, Qt::Key_Period, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, cKey_None };
static TabletKeymap::constUKeyArray sPeriodSemicolon_extended = { Qt::Key_Period, Qt::Key_Semicolon, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, cKey_None };
static TabletKeymap::constUKeyArray sColonSlash_extended = { Qt::Key_Colon, Qt::Key_Slash, Qt::Key_Backslash, cKey_None };
static TabletKeymap::constUKeyArray sAtUnderscore_extended = { Qt::Key_At, Qt::Key_Underscore, cKey_None };
static TabletKeymap::constUKeyArray sExclamAsterisk_extended = { Qt::Key_Exclam, Qt::Key_Asterisk, Qt::Key_exclamdown, cKey_None };

#define AZERTY_NUMBERS_10(w)		{ w, Qt::Key_Ampersand,		Qt::Key_1,									sAzert1_extended },\
									{ w, Qt::Key_Eacute,		Qt::Key_2,									sAzert2_extended },\
									{ w, Qt::Key_QuoteDbl,		Qt::Key_3,									sAzert3_extended },\
									{ w, Qt::Key_Apostrophe,	Qt::Key_4,									sAzert4_extended },\
									{ w, Qt::Key_ParenLeft,		Qt::Key_5,									sAzert5_extended },\
									{ w, Qt::Key_Minus,			Qt::Key_6,									sAzert6_extended },\
									{ w, Qt::Key_Egrave,		Qt::Key_7,									sAzert7_extended },\
									{ w, Qt::Key_ParenRight,	Qt::Key_8,									sAzert8_extended },\
									{ w, Qt::Key_Ccedilla,		Qt::Key_9,									sAzert9_extended },\
									{ w, Qt::Key_Agrave,		Qt::Key_0,									sAzert0_extended }

#define AZERTY_TOP_10(w)			{ w, Qt::Key_A,				Qt::Key_AsciiTilde,							sA_extended },\
									{ w, Qt::Key_Z,				Qt::Key_NumberSign,							sZ_extended },\
									{ w, Qt::Key_E,				cKey_Euro,									sE_extended },\
									{ w, Qt::Key_R,				Qt::Key_Dollar,								sR_extended },\
									{ w, Qt::Key_T,				Qt::Key_Backslash,							sT_extended },\
									{ w, Qt::Key_Y, 			Qt::Key_Bar,								sY_extended },\
									{ w, Qt::Key_U,				Qt::Key_BraceLeft,							sU_extended },\
									{ w, Qt::Key_I,				Qt::Key_BraceRight,							sI_extended },\
									{ w, Qt::Key_O,				Qt::Key_BracketLeft,						sO_extended },\
									{ w, Qt::Key_P,				Qt::Key_BracketRight,						sP_extended }

#define AZERTY_MID_10(w)			{ w, Qt::Key_Q,				Qt::Key_Less,								NULL },\
									{ w, Qt::Key_S,				Qt::Key_Greater,							sS_extended },\
									{ w, Qt::Key_D,				Qt::Key_Equal,								sD_extended },\
									{ w, Qt::Key_F,				Qt::Key_Plus,								NULL },\
									{ w, Qt::Key_G,				UKey(0x00D7) /* multiplication sign */,		sG_extended },\
									{ w, Qt::Key_H,				UKey(0x00F7) /* division sign */,			NULL },\
									{ w, Qt::Key_J,				Qt::Key_Percent,							NULL },\
									{ w, Qt::Key_K,				Qt::Key_degree,								NULL },\
									{ w, Qt::Key_L,				Qt::Key_diaeresis,							sL_extended },\
									{ w, Qt::Key_M,				Qt::Key_AsciiCircum,						sM_extended }

#define AZERTY_LOW_9(w)				{ w, Qt::Key_W,				cKey_Emoticon_Smile,						NULL  },\
									{ w, Qt::Key_X,				cKey_Emoticon_Wink,							sOptions },\
									{ w, Qt::Key_C,				cKey_Emoticon_Frown,						sC_extended },\
									{ w, Qt::Key_V,				cKey_Emoticon_Cry,							NULL },\
									{ w, Qt::Key_B,				cKey_Emoticon_Yuck,							sToggleLanguage_extended },\
									{ w, Qt::Key_N,				cKey_Emoticon_Gasp,							sN_extended },\
									{ w, Qt::Key_Comma,			Qt::Key_Question,							sCommaQuestion_extended },\
									{ w, Qt::Key_Period,		Qt::Key_Semicolon,							sPeriodSemicolon_extended },\
									{ w, Qt::Key_Colon,			Qt::Key_Slash,								sColonSlash_extended }

#define AZERTY_BOTTOM_DEFAULT \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(SPACE_SIZE, Qt::Key_Space),\
									NOKEY_1,\
									{ 1, Qt::Key_At,			Qt::Key_Underscore,							sAtUnderscore_extended },\
									{ 1, Qt::Key_Exclam,		Qt::Key_Asterisk,							sExclamAsterisk_extended },\
									{ 1.5, cKey_Hide,			cKey_Hide,									sHide_extended },\
									NOKEY_3

#define AZERTY_BOTTOM_URL \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									KEY_1(1, Qt::Key_Slash),\
									KEY_1(SPACE_SIZE - 2, Qt::Key_Space),\
									KEY_3(1, cKey_DotCom, cKey_DotCom, sAzerty_DotCom_Extended),\
									{ 1, Qt::Key_At,			Qt::Key_Underscore,							sAtUnderscore_extended },\
									{ 1, Qt::Key_Exclam,		Qt::Key_Asterisk,							sExclamAsterisk_extended },\
									{ 1.5, cKey_Hide,			cKey_Hide,									sHide_extended },\
									NOKEY_3

#define AZERTY_BOTTOM_EMAIL \
									KEY_1(1, Qt::Key_Tab),\
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(SPACE_SIZE - 1, Qt::Key_Space),\
									KEY_3(1, cKey_DotCom, cKey_DotCom, sAzerty_DotCom_Extended),\
									{ 1, Qt::Key_At,			Qt::Key_Underscore,							sAtUnderscore_extended },\
									{ 1, Qt::Key_Exclam,		Qt::Key_Asterisk,							sExclamAsterisk_extended },\
									{ 1.5, cKey_Hide,			cKey_Hide,									sHide_extended },\
									NOKEY_3

static TabletKeymap::Layout sAzerty = {
	{ KEY_2(-0.5, Qt::Key_Ampersand, Qt::Key_1), AZERTY_NUMBERS_10(1), KEY_1(-1, Qt::Key_Backspace) },
	{ AZERTY_TOP_10(1), KEY_1(1.5, Qt::Key_Backspace), NOKEY_1 },
	{ KEY_2(-0.5, Qt::Key_Q, Qt::Key_Less), AZERTY_MID_10(1), KEY_1(1, Qt::Key_Return) },
	{ KEY_1(1, Qt::Key_Shift), AZERTY_LOW_9(1), KEY_1(1.5, Qt::Key_Shift), NOKEY_1 },
	{ AZERTY_BOTTOM_DEFAULT },
};

static TabletKeymap::LayoutRow sAzertyBottomRow_default = { AZERTY_BOTTOM_DEFAULT };
static TabletKeymap::LayoutRow sAzertyBottomRow_url = { AZERTY_BOTTOM_URL };
static TabletKeymap::LayoutRow sAzertyBottomRow_email = { AZERTY_BOTTOM_EMAIL };

static TabletKeymap::LayoutFamily sQwertyFamily("qwerty", "en", IME_KBD_LANG_English, IME_KBD_SEC_REGQwerty,
                                                "+ = [  ]" /* Spaces are "Unicode Character 'HAIR SPACE' (U+200A) ' ' " */, "Q w y" /* Spaces are "Unicode Character 'HAIR SPACE' (U+200A) ' ' " */,
                                                0, 1, 10, 2, false, sQwerty, sQwertyBottomRow_default, sQwertyBottomRow_url, sQwertyBottomRow_email);
static TabletKeymap::LayoutFamily sQwertzFamily("qwertz", "de", IME_KBD_LANG_German, IME_KBD_SEC_REGQwerty,
                                                "+ ~ [  ]" /* Spaces are "Unicode Character 'HAIR SPACE' (U+200A) ' ' " */, "Q w z" /* Spaces are "Unicode Character 'HAIR SPACE' (U+200A) ' ' " */,
                                                0, 1, 10, 2, false, sQwertz, sQwertzBottomRow_default, sQwertzBottomRow_url, sQwertzBottomRow_email);
static TabletKeymap::LayoutFamily sAzertyFamily("azerty", "fr", IME_KBD_LANG_French, IME_KBD_SEC_REGQwerty,
                                                "+ = [  ]" /* Spaces are "Unicode Character 'HAIR SPACE' (U+200A) ' ' " */, "A z y" /* Spaces are "Unicode Character 'HAIR SPACE' (U+200A) ' ' " */,
                                                0, 1, 11, 2, true, sAzerty, sAzertyBottomRow_default, sAzertyBottomRow_url, sAzertyBottomRow_email);

const TabletKeymap::LayoutFamily * TabletKeymap::LayoutFamily::s_firstFamily = NULL;

TabletKeymap::LayoutFamily::LayoutFamily(const char * name, const char * defaultLanguage, uint16_t primaryID, uint16_t secondaryID,
										 const char * symbolKeyLabel, const char * noLanguageKeyLabel, int tab_x, int symbol_x, int return_x, int return_y, bool needNumLock, Layout & layout,
										 LayoutRow & defaultBottomRow, LayoutRow & urlBottomRow, LayoutRow & emailBottomRow) :
	m_name(name), m_defaultLanguage(defaultLanguage), m_primaryID(primaryID), m_secondaryID(secondaryID), m_symbolKeyLabel(symbolKeyLabel), m_noLanguageKeyLabel(noLanguageKeyLabel),
	m_tab_x(tab_x), m_symbol_x(symbol_x), m_return_x(return_x), m_return_y(return_y), m_needNumLock(needNumLock), m_cachedGlyphsCount(0), m_layout(layout),
	m_defaultBottomRow(defaultBottomRow), m_urlBottomRow(urlBottomRow), m_emailBottomRow(emailBottomRow)
{
	m_nextFamily = s_firstFamily;
	s_firstFamily = this;
}

const TabletKeymap::LayoutFamily * TabletKeymap::LayoutFamily::findLayoutFamily(const char * name, bool returnNullNotDefaultIfNotFound)
{
	const LayoutFamily * family = s_firstFamily;
	while (family)
	{
		if (strcasecmp(family->m_name, name) == 0)
			return family;
		else
			family = family->m_nextFamily;
	}
	if (returnNullNotDefaultIfNotFound)
		return NULL;
	family = &sQwertyFamily;
	g_warning("LayoutFamily::findLayoutFamily: '%s' not found, returning '%s' by default.", name, family->m_name);
	return family;
}

TabletKeymap::TabletKeymap() : m_shiftMode(TabletKeymap::eShiftMode_Off), m_symbolMode(eSymbolMode_Off), m_shiftDown(false), m_symbolDown(false), m_autoCap(false), m_numLock(false),
	m_layoutFamily(&sQwertyFamily), m_layoutPage(eLayoutPage_plain), m_limitsDirty(true), m_limitsVersion(0)
{
	for (int r = 0; r < cKeymapRows; ++r)
		m_rowHeight[r] = 1;
}

QList<const char *> TabletKeymap::getLayoutList()
{
	QList<const char *> list;
	const LayoutFamily * family = LayoutFamily::s_firstFamily;
	while (family)
	{
		list.push_back(family->m_name);
		family = family->m_nextFamily;
	}
	return list;
}

const char * TabletKeymap::getLayoutDefaultLanguage(const char * layoutName)
{
	const TabletKeymap::LayoutFamily * family = LayoutFamily::findLayoutFamily(layoutName);
	if (family)
		return family->m_defaultLanguage;
	return NULL;
}

void TabletKeymap::setRowHeight(int rowIndex, int height)
{
	if (VERIFY(rowIndex >= 0 && rowIndex < cKeymapRows))
		m_rowHeight[rowIndex] = height;
}

bool TabletKeymap::setLayoutFamily(const LayoutFamily * layoutFamily)
{
	if (m_layoutFamily != layoutFamily)
	{
		m_layoutFamily = layoutFamily;
		updateLanguageKey();
		setEditorState(m_editorState);
		m_limitsDirty = true;
		resetCachedGlyphsCount();
		return true;
	}
	return false;
}

bool TabletKeymap::setLanguageName(const std::string & name)
{
	QString displayName = getLanguageDisplayName(name, m_layoutFamily);
	if (displayName != m_languageName)
	{
		m_languageName = displayName;
		if (updateLanguageKey())
			m_limitsDirty = true;
		return true;
	}
	return false;
}

QString TabletKeymap::getLanguageDisplayName(const std::string & languageName, const LayoutFamily * layoutFamily)
{
	QString name;
	if (::strcasecmp(languageName.c_str(), "none") == 0)
		name = QString::fromUtf8(layoutFamily->m_noLanguageKeyLabel);
	else
	{
		if (languageName.length() > 0)
			name += QChar(languageName[0]).toUpper();
		if (languageName.length() > 1)
			name += QChar(languageName[1]).toLower();
		if (languageName.length() > 2 && languageName[2] != '-')
			for (size_t k = 2; k < languageName.size(); ++k)
				name += QChar(languageName[k]).toLower();
	}
	return name;
}

void TabletKeymap::keyboardCombosChanged()
{
	VirtualKeyboardPreferences & prefs = VirtualKeyboardPreferences::instance();
	int count = qMin<int>(G_N_ELEMENTS(sLanguageChoices_Extended), prefs.getKeyboardComboCount());
	for (int k = 0; k < count; k++)
		sLanguageChoices_Extended[k] = (UKey) (cKey_KeyboardComboChoice_First + k);
	sLanguageChoices_Extended[count] = cKey_None;
}

inline float fabs(float f) { return f >= 0.f ? f : -f; }

bool TabletKeymap::updateLanguageKey(LayoutRow * bottomRow)
{
	if (bottomRow == NULL)
		bottomRow = &m_layoutFamily->m_layout[cKeymapRows - 1];
	WKey & symbol = (*bottomRow)[m_layoutFamily->m_symbol_x];
	WKey & language = (*bottomRow)[m_layoutFamily->m_symbol_x + 1];
	int symbolWeightBefore = symbol.m_weight;
	int languageWeightBefore = language.m_weight;
	if (!m_languageName.isEmpty())
	{
		symbol.m_weight = 1;
		language.set(cKey_ToggleLanguage, 1, sLanguageChoices_Extended);
	}
	else
	{
		symbol.m_weight = 2;
		language.hide();
	}
	if (symbolWeightBefore != symbol.m_weight || languageWeightBefore != language.m_weight)
		return true;
	return false;
}

int TabletKeymap::updateLimits()
{
	if (m_limitsDirty)
	{
		int rectWidth = m_rect.width();
		if (rectWidth > 0)
		{
			for (int y = 0; y < cKeymapRows; ++y)
			{
				float width = 0.0001f;	// handle possible rounding errors by nudging up
				for (int x = 0; x < cKeymapColumns; ++x)
				{
					width += fabs(m_layoutFamily->weight(x, y));
					m_hlimits[y][x] = width;
				}
				for (int x = 0; x < cKeymapColumns; ++x)
				{
					m_hlimits[y][x] = float(m_hlimits[y][x] * rectWidth) / width;
				}
			}
		}
		int rectHeight = m_rect.height();
		if (rectHeight > 0)
		{
			float height = 0.0001f;	// handle possible rounding errors by nudging up
			for (int y = 0; y < cKeymapRows; ++y)
			{
				height += m_rowHeight[y];
				m_vlimits[y] = height;
			}
			for (int y = 0; y < cKeymapRows; ++y)
			{
				m_vlimits[y] = float(m_vlimits[y] * rectHeight) / height;
			}
		}
		m_limitsDirty = false;
		++m_limitsVersion;
	}
	return m_limitsVersion;
}

bool TabletKeymap::setShiftMode(TabletKeymap::EShiftMode shiftMode)
{
	if (m_shiftMode != shiftMode)
	{
		m_shiftMode = shiftMode;
		updateMapping();
		return true;
	}
	return false;
}

bool TabletKeymap::setAutoCap(bool autoCap)
{
	if (autoCap != m_autoCap)
	{
		m_autoCap = autoCap;
		return true;
	}
	return false;
}

bool TabletKeymap::setSymbolMode(ESymbolMode symbolMode)
{
	if (m_symbolMode != symbolMode)
	{
		m_symbolMode = symbolMode;
		updateMapping();
		return true;
	}
	return false;
}

bool TabletKeymap::setShiftKeyDown(bool shiftKeyDown)
{
	if (shiftKeyDown != m_shiftDown)
	{
		m_shiftDown = shiftKeyDown;
		return true;
	}
	return false;
}

bool TabletKeymap::setSymbolKeyDown(bool symbolKeyDown)
{
	m_symbolDown = symbolKeyDown;
	return updateMapping();
}

#define UPDATE_KEYS(x, y, plain, alt) { WKey & wkey = (*m_layoutFamily->m_layout)[y][x]; if (wkey.m_key != plain) { wkey.m_key = plain; layoutChanged = true; } if (wkey.m_altkey != alt) { wkey.m_altkey = alt; layoutChanged = true; } }

bool TabletKeymap::setEditorState(const PalmIME::EditorState & editorState)
{
	bool	layoutChanged = false;
	bool	weightChanged = false;
	bool	numLock = false;
	LayoutRow * newBottomRow = &m_layoutFamily->m_defaultBottomRow;

	if (!(m_editorState == editorState))
	{
		layoutChanged = true;
		m_editorState = editorState;
		m_editorState.enterKeyLabel[sizeof(m_editorState.enterKeyLabel) - 1] = 0;	// certify null-termination
		m_custom_Enter = QString::fromUtf8(m_editorState.enterKeyLabel);
	}

	switch (m_editorState.type)
	{
	case PalmIME::FieldType_Email:
		newBottomRow = &m_layoutFamily->m_emailBottomRow;
		break;
	case PalmIME::FieldType_URL:
		newBottomRow = &m_layoutFamily->m_urlBottomRow;
		break;
	default:
	case PalmIME::FieldType_Text:
	case PalmIME::FieldType_Password:
	case PalmIME::FieldType_Search:
	case PalmIME::FieldType_Range:
	case PalmIME::FieldType_Color:
		break;
	case PalmIME::FieldType_Phone:
	case PalmIME::FieldType_Number:
		if (m_layoutFamily->m_needNumLock)
			numLock = true;
		break;
	}

	updateLanguageKey(newBottomRow);
	const int lastRow = cKeymapRows - 1;
	LayoutRow & bottomRow = m_layoutFamily->m_layout[lastRow];

	for (int x = 0; x < cKeymapColumns; ++x)
	{
		WKey & wkey = bottomRow[x];
		const WKey & newWkey = (*newBottomRow)[x];
		if (wkey.m_weight != newWkey.m_weight)
			weightChanged = true;
		else if (wkey.m_key != newWkey.m_key || wkey.m_altkey != newWkey.m_altkey || wkey.m_extended != newWkey.m_extended)
			layoutChanged = true;
		else
			continue;
		wkey = newWkey;
	}

	if (weightChanged)
		m_limitsDirty = true;

	if (numLock != m_numLock)
	{
		m_numLock = numLock;
		layoutChanged = true;
	}

	m_localized__Enter		= fromStdUtf8(LOCALIZED("Enter"));
	m_localized__Tab		= fromStdUtf8(LOCALIZED("Tab"));
	m_localized__Next		= fromStdUtf8(LOCALIZED("Next"));
	m_localized__Previous	= fromStdUtf8(LOCALIZED("Prev"));

	return layoutChanged || weightChanged;
}

bool TabletKeymap::updateMapping()
{
	ELayoutPage	newPage = !isSymbolActive() ? eLayoutPage_plain : eLayoutPage_Alternate;
	if (newPage != m_layoutPage)
	{
		m_layoutPage = newPage;
		return true;
	}
	return false;
}

int TabletKeymap::keyboardToKeyZone(QPoint keyboardCoordinate, QRect & outZone)
{
	int count = 0;
	if (isValidLocation(keyboardCoordinate))
	{
		updateLimits();

		int x = keyboardCoordinate.x();
		int y = keyboardCoordinate.y();

		int left = m_rect.left() + (x > 0 ? m_hlimits[y][x - 1] : 0);
		int right = m_rect.left() + m_hlimits[y][x] - 1;

		int bottom = m_rect.top() + m_vlimits[y] - 1;
		int top = m_rect.top() + (y > 0 ? m_vlimits[y - 1] : 0);

		outZone.setCoords(left, top, right, bottom);

		if (right > left)
			count = m_layoutFamily->weight(x, y) < 0 ? -1 : 1;
	}
	return count;
}

UKey TabletKeymap::map(int x, int y)
{
	if (cResizeHandleCoord == QPoint(x, y))
		return cKey_ResizeHandle;
	if (!isValidLocation(x, y))
		return cKey_None;
	const WKey & wkey = m_layoutFamily->wkey(x, y);
	UKey key = wkey.m_key;
	if ((m_numLock || (m_layoutFamily->m_needNumLock && m_shiftMode == eShiftMode_CapsLock)) && wkey.m_altkey >= Qt::Key_0 && wkey.m_altkey <= Qt::Key_9)
	{
		if (!isShiftActive())
			key = wkey.m_altkey;
	}
	else
	{
#if 0 // experiment to get arrow keys when shift & option are down. Doesn't work because shift is held down, extending selection...
        if (m_shiftDown && m_symbolDown)
		{
			switch (key)
			{
			case Qt::Key_Y:	return Qt::Key_Up;
			case Qt::Key_B:	return Qt::Key_Down;
			case Qt::Key_G: return Qt::Key_Left;
			case Qt::Key_H: return Qt::Key_Right;
			case Qt::Key_T: return Qt::Key_Home;
			case Qt::Key_V: return Qt::Key_End;
			case Qt::Key_U: return Qt::Key_PageUp;
			case Qt::Key_N: return Qt::Key_PageDown;
			default:
				break;
			}
		}
#endif
		// for letters, use alternate layout when symbol is active, for non-letter, use alternate layout when shift is active
		if ((key >= Qt::Key_A && key <= Qt::Key_Z) ? m_layoutPage == eLayoutPage_Alternate : isShiftActive())
			key = wkey.m_altkey;
	}
	return key;
}

int TabletKeymap::xCenterOfKey(int touchX, int x, int y, float weight)
{
	int leftSide = (x > 0) ? m_hlimits[y][x - 1] : 0;
	int rightSide = m_hlimits[y][x];
	int center = (leftSide + rightSide) / 2;
	if (weight > 1)
	{
		int radius = (rightSide - leftSide) / (weight * 2);
		if (touchX < center)
		{
			int leftMost = leftSide + radius;
			if (touchX < leftMost)
				center = leftMost;
			else
				center = touchX;
		}
		else
		{
			int rightMost = rightSide - radius;
			if (touchX > rightMost)
				center = rightMost;
			else
				center = touchX;
		}
	}
	//g_debug("TouchX: %d, x: %d, y: %d, left: %d, right: %d, center: %d, radius: %g -> %d", touchX, x, y, leftSide, rightSide, (leftSide + rightSide) / 2, (rightSide - leftSide) / (weight * 2), center);
	return center;
}

int TabletKeymap::yCenterOfRow(int y, UKey key)
{	// slightly reduce the effective height of the top & lowest rows, by moving their centers further away from the closest inner row
	const int cReduceFactorTopRow = 4;		// 1 = most neutral factor: top row tallest, higher factors reduce its effective height
	const int cReduceFactorBottomRow = 3;	// 1 = most neutral factor: bottom row tallest, higher factors reduce its effective height
	const int cReduceFactorBottomRowSpace = 2;	// 1 = most neutral factor: bottom row tallest, higher factors reduce its effective height
	int top_y = y > 0 ? m_vlimits[y - 1] : 0;
	int lower_y = m_vlimits[y];
	if (y == 0)
		return lower_y / cReduceFactorTopRow;
	else if (y < cKeymapRows - 1)
		return (top_y + lower_y) / 2;
	else if (key == Qt::Key_Space)
		return (top_y + (cReduceFactorBottomRowSpace - 1) * lower_y) / cReduceFactorBottomRowSpace;
	return (top_y + (cReduceFactorBottomRow - 1) * lower_y) / cReduceFactorBottomRow;
}

inline int square(int x)										{ return x * x; }
inline int close_distance(int x, int distance)					{ return (x < 0 ? -x : x) <= distance; }

inline float adjusted_weight(const TabletKeymap::WKey & wkey)
{
    if (wkey.m_weight < 1)
        return 0.3;
    switch ((int)wkey.m_key)
	{
    case cKey_Hide:				return 0.4;
    case Qt::Key_Tab:			return 0.7;
    case Qt::Key_Space:         return 0.9;
	//case cKey_ToggleLanguage:	return 0.8;
	default:
		;
	}
#if 1
	return 1.0;
#else
	if (symbolActive || wkey.m_key < Qt::Key_A || wkey.m_key > Qt::Key_Z)
		return 1.0;
    return 1.2;
#endif
}

QPoint TabletKeymap::pointToKeyboard(const QPoint & location, bool useDiamondOptimizations)
{
	updateLimits();
	int locy = location.y() - m_rect.top() + 1;
#if RESIZE_HANDLES
	if (locy * 3 < m_vlimits[0] * 2 && (location.x() <= m_hlimits[0][0] || location.x() >= m_hlimits[0][cKeymapColumns-2]))
		return cResizeHandleCoord;
#endif
	int y = 0;
	while (locy > m_vlimits[y] && ++y < cKeymapRows)
		;
	if (y < cKeymapRows)
	{
		int locx = location.x() + 1;
		int x = 0;
		while (locx > m_hlimits[y][x] && ++x < cKeymapColumns)
			;
		if (x < cKeymapColumns)
		{
			bool changed = false;
			const WKey & wkey = m_layoutFamily->wkey(x, y);
			if (useDiamondOptimizations)
			{	// try to improve accuracy by looking if the touch point is closer to some other key above or below...
				int center_y = yCenterOfRow(y, wkey.m_key);	// vertical center of found key
				int min = (m_vlimits[y] - (y > 0 ? m_vlimits[y - 1] : 0)) / 10;
				int oy = -1;
				if (y > 0 && locy < center_y - min)
					oy = y - 1;		// pressed the upper part of the key and there is a row above
				else if (y < cKeymapRows - 1 && locy > center_y + min)
					oy = y + 1;		// pressed the lower part of the key and there is a row below
				if (oy >= 0)	// there is a possible better match above or below, on the oy row
				{
					int ox = x;
					while (ox > 0 && locx < m_hlimits[oy][ox])
						--ox;
					while (locx > m_hlimits[oy][ox] && ++ox < cKeymapColumns)
						;
					if (ox < cKeymapColumns)
					{
						int center_x = xCenterOfKey(locx, x, y, wkey.m_weight);								// horizontal center of first found key
						const WKey & owkey = m_layoutFamily->wkey(ox, oy);
						int center_ox = xCenterOfKey(locx, ox, oy, owkey.m_weight);							// horizontal center of other candidate
						int center_oy = yCenterOfRow(oy, owkey.m_key);										// vertical center of other candidate
                        int first_d = square(locy - center_y);                          					// "distance" between tap location & first found key
                        int o_d = square(locy - center_oy);                         						// "distance" between tap location & other candidate
//                        g_debug("Key: %s, %d-%d, cx: %d, cy: %d", getKeyDisplayString(wkey.m_key, true).toUtf8().data(), x, y, center_x, center_y);
//                        g_debug("OKy: %s, %d-%d, cx: %d, cy: %d", getKeyDisplayString(owkey.m_key, true).toUtf8().data(), ox, oy, center_ox, center_oy);
                        if (!close_distance(center_x - center_ox, 2))
                        {
                            first_d += square(locx - center_x);
                            o_d += square(locx - center_ox);
                        }
						bool use_o = o_d * adjusted_weight(wkey) < first_d * adjusted_weight(owkey);
						if (use_o)
							x = ox, y = oy, changed = true;
					}
				}
			}
			if (!changed && wkey.m_weight < 0)
			{ // "invisible" key. Look for the visible neighbor that has the same key...
				for (int xo = (x == 0) ? 0 : x - 1; xo <= x + 1 && xo < cKeymapColumns; ++xo)
					for (int yo = (y == 0) ? 0 : y - 1; yo <= y + 1 && yo < cKeymapRows; ++yo)
						if ((x != xo || y != yo) && m_layoutFamily->wkey(xo, yo).m_key == wkey.m_key)
						{
							x = xo; y = yo; xo = cKeymapColumns; yo = cKeymapRows;	// update x & y, then exit both loops
						}
			}
			//g_debug("%dx%d -> %s", x, y, QString(m_layoutFamily->key(x, y, m_layoutPage)).toUtf8().data());
			return QPoint(x, y);
		}
	}
	return cOutside;
}

QString TabletKeymap::getXKeys(int locX, int k_x, int k_y)
{
	QString keys;
	UKey key = map(k_x, k_y);
	if (UKeyIsUnicodeQtKey(key) && key != Qt::Key_Space)
		keys += QChar(key).toLower();
	if (locX < xCenterOfKey(locX, k_x, k_y, 1))
	{
		if (k_x > 0)
		{
			UKey okey = map(k_x - 1, k_y);
			if (okey != key && UKeyIsUnicodeQtKey(okey) && okey != Qt::Key_Space)
				keys += QChar(okey).toLower();
		}
	}
	else
	{
		if (k_x < cKeymapColumns - 1)
		{
			UKey okey = map(k_x + 1, k_y);
			if (okey != key && UKeyIsUnicodeQtKey(okey) && okey != Qt::Key_Space)
				keys += QChar(okey).toLower();
		}
	}

	return keys;
}

std::string TabletKeymap::pointToKeys(const QPoint & location)
{
	QString	keys;
	updateLimits();
	int locy = location.y() + 1;
	int y = 0;
	while (locy > m_vlimits[y] && ++y < cKeymapRows)
		;
	if (y < cKeymapRows)
	{
		int locx = location.x() + 1;
		int x = 0;
		while (locx > m_hlimits[y][x] && ++x < cKeymapColumns)
			;
		if (x < cKeymapColumns)
		{
			keys += getXKeys(locx,  x, y);

			int center_y = yCenterOfRow(y, keys.size() > 0 ? UKey(keys[0].unicode()) : cKey_None);	// vertical center of found key
			int oy = -1;
			if (locy < center_y)
			{
				if (y > 0)
					oy = y - 1;		// pressed the upper part of the key and there is a row above
			}
			else if (y < cKeymapRows)
				oy = y + 1;		// pressed the lower part of the key and there is a row below
			if (oy >= 0)	// there is a possible better match above or below, on the oy row
			{
				int ox = x;
				while (ox > 0 && locx < m_hlimits[oy][ox])
					--ox;
				while (locx > m_hlimits[oy][ox] && ++ox < cKeymapColumns)
					;
				if (ox < cKeymapColumns)
					keys += getXKeys(locx, ox, oy);
			}
		}
	}
	return keys.toUtf8().data();	// convert to utf8
}

bool TabletKeymap::generateKeyboardLayout(const char * fullPath)
{
    if (rect().width() <= 0 || rect().height() <= 0)
        return false;
	QFile file(fullPath);
	//QFile keys(QString(fullPath) + ".keys");
	if (VERIFY(file.open(QIODevice::WriteOnly))/* && VERIFY(keys.open(QIODevice::WriteOnly))*/)
	{
		updateLimits();
        file.write("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\n");
        file.write(string_printf("<keyboard primaryId=\"0x%02X\" secondaryId=\"0x%02X\" defaultLayoutWidth=\"%d\" defaultLayoutHeight=\"%d\">\n\n",
                m_layoutFamily->m_primaryID, m_layoutFamily->m_secondaryID >> 8, rect().width(), rect().height()).c_str());
		file.write("<area conditionValue=\"0\">\n");
		QRect	r;
		for (int y = 0; y < cKeymapRows; ++y)
			for (int x = 0; x < cKeymapColumns; ++x)
			{
				const WKey & wkey = m_layoutFamily->wkey(x, y);
				UKey key = wkey.m_key;
				if (UKeyIsUnicodeQtKey(key) && keyboardToKeyZone(QPoint(x, y), r) > 0)
				{
					r.translate(-rect().left(), -rect().top());
#if 1
					r.adjust(6, 6, -6, -6);
#else
					const int cMaxWidth = 2;
					if (r.width() > cMaxWidth && r.height() > cMaxWidth)
					{
						QPoint center = r.center();
						r.setLeft(center.x() - cMaxWidth / 2);
						r.setRight(center.x() + cMaxWidth / 2);
						r.setTop(center.y() - cMaxWidth / 2);
						r.setBottom(center.y() + cMaxWidth / 2);
					}
#endif
					QString text(key);
					switch (key)
					{
					case Qt::Key_Ampersand:     text = "&amp;";     break;
					case Qt::Key_Less:          text = "&lt;";      break;
					case Qt::Key_Greater:       text = "&gt;";      break;
					case Qt::Key_QuoteDbl:      text = "&quot;";    break;
					//case Qt::Key_Apostrophe:    text = "&apos;";    break;
					default:                                        break;
					}
					if (key == Qt::Key_Space) {
						file.write(string_printf("<key keyLabel=\" \" keyType=\"function\" keyName=\"ET9KEY_SPACE\" keyLeft=\"%ddp\" keyTop=\"%ddp\" keyWidth=\"%ddp\" keyHeight=\"%ddp\" />\n",
												 r.left(), r.top(), r.width(), r.height()).c_str());
					} else {
						file.write(string_printf("<key keyLabel=\"%s\" keyType=\"%s\" keyLeft=\"%ddp\" keyTop=\"%ddp\" keyWidth=\"%ddp\" keyHeight=\"%ddp\" />\n",
							  text.toUtf8().data(), key < 256 && isalpha(key) ? "regional" : "nonRegional",
							  r.left(), r.top(), r.width(), r.height()).c_str());
					}
					//QPoint center = r.center();
					//keys.write(string_printf("%d %d %d\n", key, center.x(), center.y()).c_str());
				}
			}
		file.write("</area>\n\n");
        file.write("</keyboard>\n\n");
        file.close();
	}
	return true;
}

std::string TabletKeymap::getKeyboardLayoutAsJson()
{
	pbnjson::JValue layout = pbnjson::Object();
	layout.put("layout", m_layoutFamily->m_name);
	layout.put("width", rect().width());
	layout.put("height", rect().height());
	pbnjson::JValue keys = pbnjson::Array();
	QRect	r;
	updateLimits();
	for (int y = 0; y < cKeymapRows; ++y)
		for (int x = 0; x < cKeymapColumns; ++x)
		{
			if (keyboardToKeyZone(QPoint(x, y), r) > 0)
			{
				QPoint center = r.center();
				const WKey & wkey = m_layoutFamily->wkey(x, y);
				UKey key = wkey.m_key;
				pbnjson::JValue jkey = pbnjson::Object();
				jkey.put("label", (const char *) getKeyDisplayString(key, true).toUtf8().data());
				jkey.put("shift", false);
				jkey.put("symbol", false);
				jkey.put("x", center.x());
				jkey.put("y", center.y());
				keys.append(jkey);
				UKey altKey = wkey.m_altkey;
				if (altKey != cKey_None && key != altKey)
				{
					pbnjson::JValue jkey = pbnjson::Object();
					jkey.put("label", (const char *) getKeyDisplayString(altKey, true).toUtf8().data());
					if (key >= Qt::Key_A && key <= Qt::Key_Z)
					{
						jkey.put("shift", false);
						jkey.put("symbol", true);
					}
					else
					{
						jkey.put("shift", true);
						jkey.put("symbol", false);
					}
					jkey.put("x", center.x());
					jkey.put("y", center.y());
					keys.append(jkey);
				}
			}
		}
	layout.put("keys", keys);

	std::string	layoutString;
	pbnjson::JGenerator().toString(layout, pbnjson::JSchemaFragment("{}"), layoutString);

	return layoutString;
}

const UKey * TabletKeymap::getExtendedChars(QPoint keyboardCoordinate)
{
	if (isValidLocation(keyboardCoordinate))
	{
		const WKey & wkey = m_layoutFamily->wkey(keyboardCoordinate.x(), keyboardCoordinate.y());
		if (wkey.m_key < Qt::Key_A || wkey.m_key > Qt::Key_Z || !isSymbolActive())
			return wkey.m_extended;
	}
	return NULL;
}

TabletKeymap::ETabAction TabletKeymap::tabAction() const
{
	return eTabAction_Tab;	// killing any variable Tab key behavior. Tab key always says 'Tab' for now...

	int actions = m_editorState.actions & (PalmIME::FieldAction_Next | PalmIME::FieldAction_Previous);
	if (actions == 0)
		return eTabAction_Tab;
	else if (actions == PalmIME::FieldAction_Next)	// only next
		return eTabAction_Next;
	else if (actions == PalmIME::FieldAction_Previous || m_shiftDown)	// only previous or both & shift down
		return eTabAction_Previous;
	else
		return eTabAction_Next;	// only left option...
}

QString TabletKeymap::getKeyDisplayString(UKey key, bool logging)
{
	if (UKeyIsFunctionKey(key))
	{
		if (UKeyIsKeyboardComboKey(key))
		{
			int index = key - cKey_KeyboardComboChoice_First;
			VirtualKeyboardPreferences & prefs = VirtualKeyboardPreferences::instance();
			if (VERIFY(index >= 0 && index < prefs.getKeyboardComboCount()))
				return getLanguageDisplayName(prefs.getkeyboardCombo(index).language, LayoutFamily::findLayoutFamily(prefs.getkeyboardCombo(index).layout.c_str(), false));
			return NULL;
		}

		switch ((int)key)
		{
		case Qt::Key_Return:							return (m_custom_Enter.isEmpty()) ? m_localized__Enter : m_custom_Enter;
		case Qt::Key_Tab:
		{
			switch (tabAction())
			{
			case eTabAction_Next:						return m_localized__Next;
			case eTabAction_Previous:					return m_localized__Previous;
			case eTabAction_Tab:
			default:									return m_localized__Tab;
			}
		}
		case cKey_Resize_Tiny:							return logging ? "<XS>" : "XS";
		case cKey_Resize_Small:							return logging ? "<S>" : "S";
		case cKey_Resize_Default:						return logging ? "<M>" : "M";
		case cKey_Resize_Large:							return logging ? "<L>" : "L";
		case cKey_Emoticon_Frown:						return ":-(";
		case cKey_Emoticon_Cry:							return ":'(";
		case cKey_Emoticon_Smile:						return ":-)";
		case cKey_Emoticon_Wink:						return ";-)";
		case cKey_Emoticon_Yuck:						return ":-P";
		case cKey_Emoticon_Gasp:						return ":-O";
		case cKey_Emoticon_Heart:						return "<3";
		case cKey_Symbol:								return  QString::fromUtf8((symbolMode() == TabletKeymap::eSymbolMode_Lock) ? "A B C"/* Spaces are "Unicode Character 'HAIR SPACE' (U+200A) ' ' " */ : m_layoutFamily->m_symbolKeyLabel);
		case cKey_DotCom:								return ".com";
		case cKey_DotCoUK:								return ".co.uk";
		case cKey_DotOrg:								return ".org";
		case cKey_DotDe:								return ".de";
		case cKey_DotEdu:								return ".edu";
		case cKey_DotFr:								return ".fr";
		case cKey_DotGov:								return ".gov";
		case cKey_DotNet:								return ".net";
		case cKey_DotUs:								return ".us";
		case cKey_WWW:									return "www.";
		case cKey_HTTPColonSlashSlash:					return "http://";
		case cKey_HTTPSColonSlashSlash:					return "https://";
		case Qt::Key_Left:								return QChar(0x2190) /* ← */;
		case Qt::Key_Right:								return QChar(0x2192) /* → */;
		case Qt::Key_Up:								return QChar(0x2191) /* ↑ */;
		case Qt::Key_Down:								return QChar(0x2193) /* ↓ */;
		case Qt::Key_Home:								return QChar(0x21f1) /* ⇱ */;
		case Qt::Key_End:								return QChar(0x21f2) /* ⇲ */;
		case Qt::Key_PageUp:							return QChar(0x21de) /* ⇞ */;
		case Qt::Key_PageDown:							return QChar(0x21df) /* ⇟ */;
		case cKey_ToggleSuggestions:					return "XT9";
		case cKey_ShowXT9Regions:						return "XT9 Regs";
		case cKey_ShowKeymapRegions:					return "Regions";
		case cKey_ToggleLanguage:						return m_languageName;
		case cKey_CreateDefaultKeyboards:				return "Prefs";
		case cKey_ClearDefaultKeyboards:				return "Clear";
		case cKey_SwitchToQwerty:						return "QWERTY";
		case cKey_SwitchToAzerty:						return "AZERTY";
		case cKey_SwitchToQwertz:						return "QWERTZ";
		case cKey_StartStopRecording:					return KeyLocationRecorder::instance().isRecording() ? "Stop" : "Rec";
		case cKey_ToggleSoundFeedback:					return VirtualKeyboardPreferences::instance().getTapSounds() ? "Mute" : "Sound";
		case cKey_SymbolPicker:							return "Sym";
		case Qt::Key_Shift:								return logging ? "Shift" : QString();
		case Qt::Key_AltGr:								return logging ? "AltGr" : QString();
		case cKey_Hide:									return logging ? "Hide" : QString();
		case Qt::Key_Backspace:							return logging ? "Backspace" : QString();
		default:			return QString();
		}
	}
	return isCapOrAutoCapActive() ? QChar(key).toUpper() : QChar(key).toLower();
}

}; // namespace Tablet_Keyboard
