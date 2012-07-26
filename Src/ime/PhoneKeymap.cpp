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


#include "PhoneKeymap.h"

#include "PhoneKeymap.h"

#include "KeyLocationRecorder.h"
#include "Localization.h"
#include "Logging.h"
#include "QtUtils.h"
#include "Utils.h"
#include "VirtualKeyboardPreferences.h"

#include <QFile>
#include <qdebug.h>

#include <pbnjson.hpp>

namespace Phone_Keyboard {

#define KEY_1(w, k) { w, k, k, NULL, NULL }

#define NOKEY_1 { 0, cKey_None, cKey_None, NULL, NULL }
#define NOKEY_2 NOKEY_1, NOKEY_1
#define NOKEY_3 NOKEY_2, NOKEY_1
#define NOKEY_4 NOKEY_3, NOKEY_1
#define NOKEY_5 NOKEY_4, NOKEY_1
#define NOKEY_6 NOKEY_5, NOKEY_1

#define KEY_2(w, k, a) { w, k, a, NULL, NULL }
#define KEY_3(w, k, a, e) { w, k, a, e, NULL }
#define KEY_4(w, k, a, e, ae) { w, k, a, e, ae }

#define SPACE_KEY_WEIGHT 4

static UKey	sLanguageChoices_Extended[cKey_KeyboardComboChoice_Last - cKey_KeyboardComboChoice_First + 2] = { cKey_None };

static PhoneKeymap::constUKeyArray sA_extended = { Qt::Key_A, Qt::Key_Agrave, Qt::Key_Aacute, Qt::Key_Acircumflex, Qt::Key_Atilde, Qt::Key_Adiaeresis, Qt::Key_Aring, UKey(0x00E6) /* LATIN SMALL LETTER AE æ */, UKey(0x00AA) /* FEMININE ORDINAL INDICATOR ª */, cKey_None };
static PhoneKeymap::constUKeyArray sC_extended = { Qt::Key_C, Qt::Key_Ccedilla, UKey(0x0107) /* LATIN SMALL LETTER C WITH ACUTE ć */, Qt::Key_copyright, Qt::Key_cent, cKey_None };
static PhoneKeymap::constUKeyArray sD_extended = { Qt::Key_D, UKey(0x00F0) /* LATIN SMALL LETTER ETH ð */, UKey(0x2020) /* DAGGER † */, UKey(0x2021) /* 	DOUBLE DAGGER ‡ */, cKey_None };
static PhoneKeymap::constUKeyArray sE_extended = { Qt::Key_E, Qt::Key_Egrave, Qt::Key_Eacute, Qt::Key_Ecircumflex, Qt::Key_Ediaeresis, UKey(0x0119) /* LATIN SMALL LETTER E WITH OGONEK ę */, UKey(0x0113) /* LATIN SMALL LETTER E WITH MACRON ē */, cKey_None };
static PhoneKeymap::constUKeyArray sG_extended = { Qt::Key_G, UKey(0x011F) /* LATIN SMALL LETTER G WITH BREVE ğ */, cKey_None };
static PhoneKeymap::constUKeyArray sI_extended = { Qt::Key_I, Qt::Key_Igrave, Qt::Key_Iacute, Qt::Key_Icircumflex, Qt::Key_Idiaeresis, UKey(0x0130) /* LATIN CAPITAL LETTER I WITH DOT ABOVE İ  */, UKey(0x0131) /* LATIN SMALL LETTER DOTLESS I ı */, cKey_None };
static PhoneKeymap::constUKeyArray sL_extended = { Qt::Key_L, UKey(0x00141) /* LATIN CAPITAL LETTER L WITH STROKE Ł */, cKey_None };
static PhoneKeymap::constUKeyArray sM_extended = { Qt::Key_M, UKey(0x00B5) /* MICRO SIGN µ */, cKey_None };
static PhoneKeymap::constUKeyArray sN_extended = { Qt::Key_N, UKey(0x00F1) /* LATIN SMALL LETTER N WITH TILDE ñ */, UKey(0x0144) /* LATIN SMALL LETTER N WITH ACUTE ń */, cKey_None };
static PhoneKeymap::constUKeyArray sO_extended = { Qt::Key_O, Qt::Key_Ograve, Qt::Key_Oacute, Qt::Key_Ocircumflex, Qt::Key_Otilde, Qt::Key_Odiaeresis, Qt::Key_Ooblique, UKey(0x0151) /* LATIN SMALL LETTER O WITH DOUBLE ACUTE ő */,
											  UKey(0x0153) /* latin small letter œ */, UKey(0x00BA) /* MASCULINE ORDINAL INDICATOR º */, UKey(0x03C9) /* GREEK SMALL LETTER OMEGA ω */, cKey_None };
static PhoneKeymap::constUKeyArray sP_extended = { Qt::Key_P /*, UKey(0x00B6) / * PILCROW SIGN ¶ */, UKey(0x00A7) /* SECTION SIGN § */, UKey(0x03C0) /* GREEK SMALL LETTER PI π */, cKey_None };
static PhoneKeymap::constUKeyArray sR_extended = { Qt::Key_R, Qt::Key_registered, cKey_None };
static PhoneKeymap::constUKeyArray sS_extended = { Qt::Key_S, UKey(0x0161) /* LATIN SMALL LETTER S WITH CARON š */, UKey(0x015E) /* LATIN CAPITAL LETTER S WITH CEDILLA ş */, Qt::Key_ssharp, UKey(0x03C3) /* GREEK SMALL LETTER SIGMA σ */, cKey_None };
static PhoneKeymap::constUKeyArray sT_extended = { Qt::Key_T, UKey(0x2122) /* TRADE MARK SIGN ™ */, Qt::Key_THORN, cKey_None };
static PhoneKeymap::constUKeyArray sU_extended = { Qt::Key_U, Qt::Key_Ugrave, Qt::Key_Uacute, Qt::Key_Ucircumflex, Qt::Key_Udiaeresis, UKey(0x0171) /* LATIN SMALL LETTER U WITH DOUBLE ACUTE ű */, cKey_None };
static PhoneKeymap::constUKeyArray sY_extended = { Qt::Key_Y, Qt::Key_Yacute, Qt::Key_ydiaeresis, cKey_None };
static PhoneKeymap::constUKeyArray sZ_extended = { Qt::Key_Z, UKey(0x017E) /* LATIN SMALL LETTER Z WITH CARON ž */, UKey(0x017A) /* LATIN SMALL LETTER Z WITH ACUTE ź */, UKey(0x017C) /* LATIN SMALL LETTER Z WITH DOT ABOVE ż */, cKey_None };

static PhoneKeymap::constUKeyArray sQwerty1_extended = { Qt::Key_1, Qt::Key_Exclam, UKey(0x00BC) /* VULGAR FRACTION ONE QUARTER ¼ */, UKey(0x00BD) /* VULGAR FRACTION ONE HALF ½ */, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty2_extended = { Qt::Key_2, Qt::Key_At, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty3_extended = { Qt::Key_3, Qt::Key_NumberSign, UKey(0x00BE) /* VULGAR FRACTION THREE QUARTERS ¾ */, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty4_extended = { Qt::Key_4, Qt::Key_Dollar, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty5_extended = { Qt::Key_5, Qt::Key_Percent, UKey(0x2030) /* PER MILLE SIGN ‰ */, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty6_extended = { Qt::Key_6, Qt::Key_AsciiCircum, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty7_extended = { Qt::Key_7, Qt::Key_Ampersand, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty8_extended = { Qt::Key_8, Qt::Key_Asterisk, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty9_extended = { Qt::Key_9, Qt::Key_ParenLeft, cKey_None };
static PhoneKeymap::constUKeyArray sQwerty0_extended = { Qt::Key_0, Qt::Key_ParenRight, cKey_None };

static PhoneKeymap::constUKeyArray sQwertyExclam_extended = { Qt::Key_Exclam, Qt::Key_AsciiTilde, Qt::Key_exclamdown, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyAt_extended = { Qt::Key_At, Qt::Key_copyright, Qt::Key_registered, UKey(0x2122) /* TRADE MARK SIGN ™ */, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyNumberSign_extended = { Qt::Key_NumberSign, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyDollar_extended = { Qt::Key_Dollar, Qt::Key_sterling, Qt::Key_yen, cKey_Euro, Qt::Key_currency, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyPercent_extended = { Qt::Key_Percent, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyAmpersand_extended = { Qt::Key_Ampersand, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyAsterisk_extended = { Qt::Key_Asterisk, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyParenLeft_extended = { Qt::Key_ParenLeft, Qt::Key_BracketLeft, Qt::Key_BraceLeft, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyParenRight_extended = { Qt::Key_ParenRight, Qt::Key_BracketRight, Qt::Key_BraceRight, cKey_None };

static PhoneKeymap::constUKeyArray sQwertySemicolon_extended = { Qt::Key_Semicolon, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyColon_extended = { Qt::Key_Colon, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyEqual_extended = { Qt::Key_Equal, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyPlus_extended = { Qt::Key_Plus, UKey(0x00D7) /* multiplication sign */, UKey(0x00F7) /* division sign */, UKey(0x00B1) /* PLUS-MINUS SIGN ± */, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyMinus_extended = { Qt::Key_Minus, Qt::Key_Underscore, UKey(0x00AC) /* NOT SIGN ¬ */, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyApostrophe_extended = { Qt::Key_Apostrophe, UKey(0x0060) /* GRAVE ACCENT ` */, UKey(0x2018) /* LEFT SINGLE QUOTATION MARK ‘ */, UKey(0x2019) /* RIGHT SINGLE QUOTATION MARK ’ */, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyQuoteDbl_extended = { Qt::Key_QuoteDbl, UKey(0x201C) /* LEFT DOUBLE QUOTATION MARK “ */, UKey(0x201D) /* RIGHT DOUBLE QUOTATION MARK ” */,
																UKey(0x00AB) /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK « */, UKey(0x00BB) /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK » */, cKey_None };

static PhoneKeymap::constUKeyArray sQwertyCommaSlash_extended = { Qt::Key_Comma, Qt::Key_Slash, Qt::Key_Backslash, Qt::Key_Bar, cKey_None };
static PhoneKeymap::constUKeyArray sQwertyPeriodQuestion_extended = { Qt::Key_Period, Qt::Key_Question, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, Qt::Key_questiondown, cKey_None };
//static PhoneKeymap::constUKeyArray sQwerty_extended = { Qt::Key_, cKey_None };
//static PhoneKeymap::constUKeyArray sQwerty_extended = { Qt::Key_, cKey_None };


static PhoneKeymap::constUKeyArray sEmoticons_extended = { cKey_Emoticon_Smile, cKey_Emoticon_Wink, cKey_Emoticon_Frown, cKey_Emoticon_Cry, cKey_Emoticon_Yuck, cKey_Emoticon_Gasp, cKey_Emoticon_Heart, cKey_None };


/* UKey(0x2039) SINGLE LEFT-POINTING ANGLE QUOTATION MARK ‹ */
/* UKey(0x203A) SINGLE RIGHT-POINTING ANGLE QUOTATION MARK › */
/* UKey(0x00B7) MIDDLE DOT · */
/* UKey(0x201A) SINGLE LOW-9 QUOTATION MARK ‚ */
/* UKey(0x201E) DOUBLE LOW-9 QUOTATION MARK „ */
/* UKey(0x201B) SINGLE HIGH-REVERSED-9 QUOTATION MARK ‛ */
/* UKey(0x201F) DOUBLE HIGH-REVERSED-9 QUOTATION MARK ‟ */

static PhoneKeymap::constUKeyArray sSingleAndDoubleQuote_extended = { Qt::Key_Apostrophe, Qt::Key_QuoteDbl, UKey(0x0060) /* GRAVE ACCENT ` */,
																 UKey(0x2018) /* LEFT SINGLE QUOTATION MARK ‘ */, UKey(0x2019) /* RIGHT SINGLE QUOTATION MARK ’ */,
																 UKey(0x201C) /* LEFT DOUBLE QUOTATION MARK “ */, UKey(0x201D) /* RIGHT DOUBLE QUOTATION MARK ” */,
																 UKey(0x00AB) /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK « */, UKey(0x00BB) /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK » */, cKey_None };
static PhoneKeymap::constUKeyArray sPeriodQuestion_extended = { Qt::Key_Period, Qt::Key_Question, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, Qt::Key_questiondown, cKey_None };
static PhoneKeymap::constUKeyArray sMinusUnderscore_extended = { Qt::Key_Minus, Qt::Key_Underscore, UKey(0x00B1) /* PLUS-MINUS SIGN ± */, UKey(0x00AC) /* NOT SIGN ¬ */, cKey_None };
static PhoneKeymap::constUKeyArray sCommaSlash_extended = { Qt::Key_Comma, Qt::Key_Slash, Qt::Key_Backslash, cKey_None };

static PhoneKeymap::constUKeyArray sDotCom_Extended = { cKey_DotCom, cKey_DotNet, cKey_DotEdu, cKey_DotOrg, cKey_DotCoUK, cKey_None };

// turn off backdoors for shipping version
#if SHIPPING_VERSION
	#define sToggleLanguage_extended NULL
	#define sOptions NULL
#else
	static PhoneKeymap::constUKeyArray sOptions = { cKey_ToggleSuggestions, cKey_ShowXT9Regions, cKey_ShowKeymapRegions, cKey_StartStopRecording, cKey_ToggleSoundFeedback, cKey_None };
	static PhoneKeymap::constUKeyArray sToggleLanguage_extended = { cKey_SwitchToQwerty, cKey_SwitchToAzerty, cKey_SwitchToQwertz, cKey_CreateDefaultKeyboards, cKey_ClearDefaultKeyboards, cKey_None };
#endif

#define QWERTY_TOP_10(w)			{ w, Qt::Key_Q,			Qt::Key_1,					NULL,						sQwerty1_extended },\
									{ w, Qt::Key_W,			Qt::Key_2,					NULL,						sQwerty2_extended },\
									{ w, Qt::Key_E,			Qt::Key_3,					sE_extended,				sQwerty3_extended },\
									{ w, Qt::Key_R,			Qt::Key_4,					sR_extended,				sQwerty4_extended },\
									{ w, Qt::Key_T,			Qt::Key_5,					sT_extended,				sQwerty5_extended },\
									{ w, Qt::Key_Y,	 		Qt::Key_6,					sY_extended,				sQwerty6_extended },\
									{ w, Qt::Key_U,			Qt::Key_7,					sU_extended,				sQwerty7_extended },\
									{ w, Qt::Key_I,			Qt::Key_8,					sI_extended,				sQwerty8_extended },\
									{ w, Qt::Key_O,			Qt::Key_9,					sO_extended,				sQwerty9_extended },\
									{ w, Qt::Key_P,			Qt::Key_0,					sP_extended,				sQwerty0_extended }

#define QWERTY_MID_9(w)				{ w, Qt::Key_A,			Qt::Key_Exclam,				sA_extended,				sQwertyExclam_extended },\
									{ w, Qt::Key_S,			Qt::Key_At,					sS_extended,				sQwertyAt_extended },\
									{ w, Qt::Key_D,			Qt::Key_NumberSign,			sD_extended,				sQwertyNumberSign_extended },\
									{ w, Qt::Key_F,			Qt::Key_Dollar,				NULL,						sQwertyDollar_extended },\
									{ w, Qt::Key_G,			Qt::Key_Percent,			sG_extended,				sQwertyPercent_extended },\
									{ w, Qt::Key_H,			Qt::Key_Ampersand,			NULL,						sQwertyAmpersand_extended },\
									{ w, Qt::Key_J,			Qt::Key_Asterisk,			NULL,						sQwertyAsterisk_extended },\
									{ w, Qt::Key_K,			Qt::Key_ParenLeft,			NULL,						sQwertyParenLeft_extended },\
									{ w, Qt::Key_L,			Qt::Key_ParenRight,			sL_extended,				sQwertyParenRight_extended }

#define QWERTY_LOW_7(w)				{ w, Qt::Key_Z,			Qt::Key_Semicolon,			sZ_extended,				sQwertySemicolon_extended },\
									{ w, Qt::Key_X,			Qt::Key_Colon,				sOptions,					sQwertyColon_extended },\
									{ w, Qt::Key_C,			Qt::Key_Equal,				sC_extended,				sQwertyEqual_extended },\
									{ w, Qt::Key_V,			Qt::Key_Plus,				NULL,						sQwertyPlus_extended },\
									{ w, Qt::Key_B,			Qt::Key_Minus,				sToggleLanguage_extended,	sQwertyMinus_extended },\
									{ w, Qt::Key_N,			Qt::Key_Apostrophe,			sN_extended,				sQwertyApostrophe_extended },\
									{ w, Qt::Key_M,			Qt::Key_QuoteDbl,			sM_extended,				sQwertyQuoteDbl_extended }

#define QWERT_BOTTOM_ROW \
									KEY_1(1.5, cKey_Symbol),\
									NOKEY_1,\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(SPACE_KEY_WEIGHT, Qt::Key_Space),\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(1.5, Qt::Key_Return),\
									NOKEY_4

static PhoneKeymap::Layout sQwerty = {
	{ QWERTY_TOP_10(1), NOKEY_2 },
	{ KEY_4(-0.5, Qt::Key_A, Qt::Key_Less, sA_extended, sQwertyExclam_extended), QWERTY_MID_9(1), KEY_4(-0.5, Qt::Key_L, Qt::Key_ParenRight, sL_extended, sQwertyParenRight_extended), NOKEY_1 },
	{ KEY_4(1.25, Qt::Key_Shift, cKey_ToggleLanguage, NULL, sLanguageChoices_Extended), KEY_4(-0.25, Qt::Key_Z, Qt::Key_Semicolon, sZ_extended, sQwertySemicolon_extended), QWERTY_LOW_7(1), KEY_1(-0.25, Qt::Key_Backspace), KEY_1(1.25, Qt::Key_Backspace), NOKEY_1 },
	{ QWERT_BOTTOM_ROW },
};

//static PhoneKeymap::constUKeyArray sQwertz1_extended = { Qt::Key_1, Qt::Key_Exclam, UKey(0x00B9) /* SUPERSCRIPT ONE ¹ */, UKey(0x00BC) /* VULGAR FRACTION ONE QUARTER ¼ */, UKey(0x00BD) /* VULGAR FRACTION ONE HALF ½ */, Qt::Key_exclamdown, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz2_extended = { Qt::Key_2, Qt::Key_QuoteDbl, UKey(0x00B2) /* SUPERSCRIPT TWO ² */, UKey(0x201C) /* LEFT DOUBLE QUOTATION MARK “ */, UKey(0x201D) /* RIGHT DOUBLE QUOTATION MARK ” */, UKey(0x00AB) /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK « */, UKey(0x00BB) /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK » */, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz3_extended = { Qt::Key_3, Qt::Key_At, UKey(0x00B3) /* SUPERSCRIPT THREE ³ */, UKey(0x00BE) /* VULGAR FRACTION THREE QUARTERS ¾ */, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz4_extended = { Qt::Key_4, Qt::Key_Dollar, cKey_Euro, Qt::Key_sterling, Qt::Key_yen, Qt::Key_cent, UKey(0x00A4) /* CURRENCY SIGN ¤ */, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz5_extended = { Qt::Key_5, Qt::Key_Percent, UKey(0x2030) /* PER MILLE SIGN ‰ */, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz6_extended = { Qt::Key_6, Qt::Key_Ampersand, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz7_extended = { Qt::Key_7, Qt::Key_Slash, Qt::Key_Backslash, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz8_extended = { Qt::Key_8, Qt::Key_ParenLeft, Qt::Key_BracketLeft, Qt::Key_BraceLeft, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz9_extended = { Qt::Key_9, Qt::Key_ParenRight, Qt::Key_BracketRight, Qt::Key_BraceRight, cKey_None };
//static PhoneKeymap::constUKeyArray sQwertz0_extended = { Qt::Key_0, Qt::Key_Equal, cKey_None };

static PhoneKeymap::constUKeyArray sCommaSemiColon_extended = { Qt::Key_Comma, Qt::Key_Semicolon, cKey_None };
static PhoneKeymap::constUKeyArray sPeriodColon_extended = { Qt::Key_Period, Qt::Key_Colon, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, cKey_None };
static PhoneKeymap::constUKeyArray sSSharpQuestion_extended = { Qt::Key_ssharp, Qt::Key_Question, Qt::Key_questiondown, cKey_None };
static PhoneKeymap::constUKeyArray sMinusApostrophe_extended = { Qt::Key_Minus, Qt::Key_Apostrophe, UKey(0x00B1) /* PLUS-MINUS SIGN ± */, UKey(0x00AC) /* NOT SIGN ¬ */, UKey(0x0060) /* GRAVE ACCENT ` */, UKey(0x2018) /* LEFT SINGLE QUOTATION MARK ‘ */, UKey(0x2019) /* RIGHT SINGLE QUOTATION MARK ’ */, cKey_None };

#define QWERTZ_TOP_10(w)			{ w, Qt::Key_Q,			Qt::Key_1,					NULL,						NULL },\
									{ w, Qt::Key_W,			Qt::Key_2,					NULL,						NULL },\
									{ w, Qt::Key_E,			Qt::Key_3,					sE_extended,				NULL },\
									{ w, Qt::Key_R,			Qt::Key_4,					sR_extended,				NULL },\
									{ w, Qt::Key_T,			Qt::Key_5,					sT_extended,				NULL },\
									{ w, Qt::Key_Z,	 		Qt::Key_6,					sZ_extended,				NULL },\
									{ w, Qt::Key_U,			Qt::Key_7,					sU_extended,				NULL },\
									{ w, Qt::Key_I,			Qt::Key_8,					sI_extended,				NULL },\
									{ w, Qt::Key_O,			Qt::Key_9,					sO_extended,				NULL },\
									{ w, Qt::Key_P,			Qt::Key_0,					sP_extended,				NULL }

#define QWERTZ_MID_9(w)				{ w, Qt::Key_A,			Qt::Key_Exclam,				sA_extended,				NULL },\
									{ w, Qt::Key_S,			Qt::Key_At,					sS_extended,				NULL },\
									{ w, Qt::Key_D,			Qt::Key_NumberSign,			sD_extended,				NULL },\
									{ w, Qt::Key_F,			Qt::Key_Dollar,				NULL,						NULL },\
									{ w, Qt::Key_G,			Qt::Key_Percent,			sG_extended,				NULL },\
									{ w, Qt::Key_H,			Qt::Key_Ampersand,			NULL,						NULL },\
									{ w, Qt::Key_J,			Qt::Key_Asterisk,			NULL,						NULL },\
									{ w, Qt::Key_K,			Qt::Key_ParenLeft,			NULL,						NULL },\
									{ w, Qt::Key_L,			Qt::Key_ParenRight,			sL_extended,				NULL }

#define QWERTZ_LOW_7(w)				{ w, Qt::Key_Y,			Qt::Key_Semicolon,			sY_extended,				NULL },\
									{ w, Qt::Key_X,			Qt::Key_Colon,				sOptions,					NULL },\
									{ w, Qt::Key_C,			Qt::Key_Equal,				sC_extended,				NULL },\
									{ w, Qt::Key_V,			Qt::Key_Plus,				NULL,						NULL },\
									{ w, Qt::Key_B,			Qt::Key_Minus,				sToggleLanguage_extended,	NULL },\
									{ w, Qt::Key_N,			Qt::Key_Apostrophe,			sN_extended,				NULL },\
									{ w, Qt::Key_M,			Qt::Key_QuoteDbl,			sM_extended,				NULL }

static PhoneKeymap::Layout sQwertz = {
	{ QWERTZ_TOP_10(1), NOKEY_2 },
	{ KEY_3(-0.5, Qt::Key_A, Qt::Key_Exclam, sA_extended), QWERTZ_MID_9(1), KEY_3(-0.5, Qt::Key_L, Qt::Key_ParenRight, sL_extended), NOKEY_1 },
	{ KEY_4(1.25, Qt::Key_Shift, cKey_ToggleLanguage, NULL, sLanguageChoices_Extended), KEY_4(-0.25, Qt::Key_Y, Qt::Key_Semicolon, sY_extended, NULL), QWERTZ_LOW_7(1), KEY_1(-0.25, Qt::Key_Backspace), KEY_1(1.25, Qt::Key_Backspace), NOKEY_1 },
	{ QWERT_BOTTOM_ROW },
};

//static PhoneKeymap::constUKeyArray sAzert1_extended = { Qt::Key_1, Qt::Key_Ampersand, UKey(0x00B9) /* SUPERSCRIPT ONE ¹ */, UKey(0x00BC) /* VULGAR FRACTION ONE QUARTER ¼ */, UKey(0x00BD) /* VULGAR FRACTION ONE HALF ½ */, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert2_extended = { Qt::Key_2, Qt::Key_Eacute, UKey(0x00B2) /* SUPERSCRIPT TWO ² */, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert3_extended = { Qt::Key_3, Qt::Key_QuoteDbl, UKey(0x00B3) /* SUPERSCRIPT THREE ³ */, UKey(0x00BE) /* VULGAR FRACTION THREE QUARTERS ¾ */,
//												   UKey(0x201C) /* LEFT DOUBLE QUOTATION MARK “ */, UKey(0x201D) /* RIGHT DOUBLE QUOTATION MARK ” */,
//												   UKey(0x00AB) /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK « */, UKey(0x00BB) /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK » */, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert4_extended = { Qt::Key_4, Qt::Key_Apostrophe, UKey(0x2018) /* LEFT SINGLE QUOTATION MARK ‘ */, UKey(0x2019) /* RIGHT SINGLE QUOTATION MARK ’ */, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert5_extended = { Qt::Key_5, Qt::Key_ParenLeft, Qt::Key_BracketLeft, Qt::Key_BraceLeft, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert6_extended = { Qt::Key_6, Qt::Key_Minus, UKey(0x00B1) /* PLUS-MINUS SIGN ± */, UKey(0x00AC) /* NOT SIGN ¬ */, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert7_extended = { Qt::Key_7, Qt::Key_Egrave, Qt::Key_QuoteLeft, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert8_extended = { Qt::Key_8, Qt::Key_ParenRight, Qt::Key_BracketRight, Qt::Key_BraceRight, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert9_extended = { Qt::Key_9, Qt::Key_Ccedilla, Qt::Key_cent, Qt::Key_Dollar, cKey_Euro, Qt::Key_sterling, Qt::Key_yen, UKey(0x00A4) /* CURRENCY SIGN ¤ */, cKey_None };
//static PhoneKeymap::constUKeyArray sAzert0_extended = { Qt::Key_0, Qt::Key_Agrave, Qt::Key_Percent, UKey(0x2030) /* PER MILLE SIGN ‰ */, cKey_None };

static PhoneKeymap::constUKeyArray sCommaQuestion_extended = { Qt::Key_Comma, Qt::Key_Question, Qt::Key_questiondown, cKey_None };
static PhoneKeymap::constUKeyArray sPeriodExclamation_extended = { Qt::Key_Period, Qt::Key_Exclam, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, Qt::Key_exclamdown, cKey_None };
static PhoneKeymap::constUKeyArray sSemicolonPeriod_extended = { Qt::Key_Semicolon, Qt::Key_Period, UKey(0x2022) /* BULLET • */, UKey(0x2026) /* HORIZONTAL ELLIPSIS … */, cKey_None };
static PhoneKeymap::constUKeyArray sColonSlash_extended = { Qt::Key_Colon, Qt::Key_Slash, Qt::Key_Backslash, cKey_None };
static PhoneKeymap::constUKeyArray sAtUnderscore_extended = { Qt::Key_At, Qt::Key_Underscore, cKey_None };
static PhoneKeymap::constUKeyArray sExclamAsterisk_extended = { Qt::Key_Exclam, Qt::Key_Asterisk, Qt::Key_exclamdown, cKey_None };

#define AZERTY_TOP_10(w)			{ w, Qt::Key_A,				Qt::Key_1,				sA_extended,				NULL },\
									{ w, Qt::Key_Z,				Qt::Key_2,				NULL,						NULL },\
									{ w, Qt::Key_E,				Qt::Key_3,				sE_extended,				NULL },\
									{ w, Qt::Key_R,				Qt::Key_4,				sR_extended },\
									{ w, Qt::Key_T,				Qt::Key_5,				sT_extended,				NULL },\
									{ w, Qt::Key_Y, 			Qt::Key_6,				sY_extended,				NULL },\
									{ w, Qt::Key_U,				Qt::Key_7,				sU_extended,				NULL },\
									{ w, Qt::Key_I,				Qt::Key_8,				sI_extended,				NULL },\
									{ w, Qt::Key_O,				Qt::Key_9,				sO_extended,				NULL },\
									{ w, Qt::Key_P,				Qt::Key_0,				sP_extended,				NULL }

#define AZERTY_MID_10(w)			{ w, Qt::Key_Q,				Qt::Key_Exclam,			NULL,						NULL },\
									{ w, Qt::Key_S,				Qt::Key_At,				sS_extended,				NULL },\
									{ w, Qt::Key_D,				Qt::Key_NumberSign,		sD_extended,				NULL },\
									{ w, Qt::Key_F,				Qt::Key_Dollar,			NULL,						NULL },\
									{ w, Qt::Key_G,				Qt::Key_Percent,		sG_extended,				NULL },\
									{ w, Qt::Key_H,				Qt::Key_Ampersand,		NULL,						NULL },\
									{ w, Qt::Key_J,				Qt::Key_Asterisk,		NULL,						NULL },\
									{ w, Qt::Key_K,				Qt::Key_ParenLeft,		NULL,						NULL },\
									{ w, Qt::Key_L,				Qt::Key_ParenRight,		sL_extended,				NULL },\
									{ w, Qt::Key_M,				Qt::Key_AsciiCircum,	sM_extended,				NULL }

#define AZERTY_LOW_6(w)				{ w, Qt::Key_W,				Qt::Key_Semicolon,		NULL,						NULL },\
									{ w, Qt::Key_X,				Qt::Key_Colon,			sOptions,					NULL },\
									{ w, Qt::Key_C,				Qt::Key_Equal,			sC_extended,				NULL },\
									{ w, Qt::Key_V,				Qt::Key_Plus,			NULL,						NULL },\
									{ w, Qt::Key_B,				Qt::Key_Minus,			sToggleLanguage_extended,	NULL },\
									{ w, Qt::Key_N,				Qt::Key_Apostrophe,		sN_extended,				NULL }

#define AZERTY_BOTTOM \
									KEY_1(2, cKey_Symbol),\
									NOKEY_1,\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(SPACE_KEY_WEIGHT, Qt::Key_Space),\
									NOKEY_1,\
									NOKEY_1,\
									KEY_1(2, Qt::Key_Return),\
									NOKEY_4

static PhoneKeymap::Layout sAzerty = {
	{ AZERTY_TOP_10(1), KEY_2(-0.5, Qt::Key_P, Qt::Key_BracketRight), NOKEY_1 },
	{ KEY_2(-0.5, Qt::Key_Q, Qt::Key_Less), AZERTY_MID_10(1), NOKEY_1 },
	{ KEY_4(1.25, Qt::Key_Shift, cKey_ToggleLanguage, NULL, sLanguageChoices_Extended), KEY_4(-0.25, Qt::Key_Shift, cKey_ToggleLanguage, NULL, sLanguageChoices_Extended), AZERTY_LOW_6(1), KEY_4(1.5, Qt::Key_Apostrophe, Qt::Key_At, NULL, NULL), KEY_1(-0.25, Qt::Key_Backspace), KEY_1(1.25, Qt::Key_Backspace) },
	{ AZERTY_BOTTOM },
};

static PhoneKeymap::constUKeyArray sURL_extended = { cKey_ColonSlashSlash, cKey_HTTPColonSlashSlash, cKey_HTTPSColonSlashSlash, cKey_None };

const PhoneKeymap::WKey cWKey_Hidden = { 0, cKey_None, cKey_None, NULL, NULL };
const PhoneKeymap::WKey cWKey_CommaSlash = { 1.5, Qt::Key_Comma, Qt::Key_Slash, sCommaSlash_extended };
const PhoneKeymap::WKey cWKey_PeriodQuestion = { 1.5, Qt::Key_Period, Qt::Key_Question, sQwertyPeriodQuestion_extended };
const PhoneKeymap::WKey cWKey_LanguageEmoticons = { 1, cKey_ToggleLanguage, cKey_Emoticon_Options, sLanguageChoices_Extended, sEmoticons_extended };
const PhoneKeymap::WKey cWKey_Emoticons = { 1.5, cKey_Emoticon_Options, cKey_Emoticon_Options, sEmoticons_extended, sEmoticons_extended };
const PhoneKeymap::WKey cWKey_MorePopup = { 1.5, cKey_MorePopup, cKey_MorePopup, NULL, NULL };
const PhoneKeymap::WKey cWKey_MinusUnderscore = { 1, Qt::Key_Minus, Qt::Key_Underscore, NULL, NULL };
const PhoneKeymap::WKey cWKey_At = { 1, Qt::Key_At, Qt::Key_At, NULL, NULL };
const PhoneKeymap::WKey cWKey_Underscore = { 1, Qt::Key_Underscore, Qt::Key_Underscore, NULL, NULL };
const PhoneKeymap::WKey cWKey_Slash = { 1, Qt::Key_Slash, Qt::Key_Slash, NULL, NULL };
const PhoneKeymap::WKey cWKey_DotCom = { 1, cKey_DotCom, cKey_DotCom, sDotCom_Extended, NULL };
const PhoneKeymap::WKey cWKey_Colon = { 1, Qt::Key_Colon, Qt::Key_Colon, sURL_extended, NULL };
const PhoneKeymap::WKey cWKey_https = { 1, cKey_HTTPSColonSlashSlash, cKey_HTTPSColonSlashSlash, sURL_extended, NULL };
const PhoneKeymap::WKey cWKey_CommaQuestion = { 1.5, Qt::Key_Comma, Qt::Key_Question, sCommaQuestion_extended, NULL };
const PhoneKeymap::WKey cWKey_PeriodExclamation = { 1.5, Qt::Key_Period, Qt::Key_Exclam, sPeriodExclamation_extended, NULL };

const PhoneKeymap::CustomKeys cCustom_QWERT_plain(cWKey_CommaSlash, cWKey_Hidden, cWKey_Hidden, cWKey_PeriodQuestion);
const PhoneKeymap::CustomKeys cCustom_QWERT_symbol(cWKey_Emoticons, cWKey_Hidden, cWKey_Hidden, cWKey_MorePopup);
const PhoneKeymap::CustomKeys cCustom_QWERT_email(cWKey_CommaSlash, cWKey_At, cWKey_DotCom, cWKey_PeriodQuestion);
const PhoneKeymap::CustomKeys cCustom_QWERT_url(cWKey_Slash, cWKey_Colon, cWKey_DotCom, cWKey_PeriodQuestion);

const PhoneKeymap::CustomKeys cCustom_AZERTY_plain(cWKey_CommaQuestion, cWKey_Hidden, cWKey_Hidden, cWKey_PeriodExclamation);
const PhoneKeymap::CustomKeys cCustom_AZERTY_symbol(cWKey_Emoticons, cWKey_Hidden, cWKey_Hidden, cWKey_MorePopup);
const PhoneKeymap::CustomKeys cCustom_AZERTY_email(cWKey_CommaQuestion, cWKey_At, cWKey_DotCom, cWKey_PeriodExclamation);
const PhoneKeymap::CustomKeys cCustom_AZERTY_url(cWKey_Slash, cWKey_Colon, cWKey_DotCom, cWKey_PeriodExclamation);

static PhoneKeymap::LayoutFamily sQwertyFamily("qwerty", "en", IME_KBD_LANG_English, IME_KBD_SEC_REGQwerty, 0, 4, 7, 3, 1.5, 1.5, cCustom_QWERT_plain, cCustom_QWERT_symbol, cCustom_QWERT_email, cCustom_QWERT_url, false, &sQwerty);
static PhoneKeymap::LayoutFamily sQwertzFamily("qwertz", "de", IME_KBD_LANG_German, IME_KBD_SEC_REGQwerty, 0, 4, 7, 3, 1.5, 1.5, cCustom_QWERT_plain, cCustom_QWERT_symbol, cCustom_QWERT_email, cCustom_QWERT_url, false, &sQwertz);
static PhoneKeymap::LayoutFamily sAzertyFamily("azerty", "fr", IME_KBD_LANG_French, IME_KBD_SEC_REGQwerty, 0, 4, 7, 3, 1.5, 2, cCustom_AZERTY_plain, cCustom_AZERTY_symbol, cCustom_AZERTY_email, cCustom_AZERTY_url, false, &sAzerty);

const PhoneKeymap::LayoutFamily * PhoneKeymap::LayoutFamily::s_firstFamily = NULL;

PhoneKeymap::LayoutFamily::LayoutFamily(const char * name, const char * defaultLanguage, uint16_t primaryID, uint16_t secondaryID,
								   int symbol_x, int space_x, int return_x, int return_y, float return_key_weight, float symbol_key_weight,
								   const CustomKeys & plainKeys, const CustomKeys & symbolKeys, const CustomKeys & emailKeys, const CustomKeys & urlKeys, bool needNumLock, Layout * layout) :
	m_name(name), m_defaultLanguage(defaultLanguage), m_primaryID(primaryID), m_secondaryID(secondaryID),
	m_symbol_x(symbol_x), m_space_x(space_x), m_return_x(return_x), m_return_y(return_y), m_return_key_weight(return_key_weight), m_symbol_key_weight(symbol_key_weight),
	m_plainKeys(plainKeys), m_symbolKeys(symbolKeys), m_emailKeys(emailKeys), m_urlKeys(urlKeys), m_needNumLock(needNumLock), m_cachedGlyphsCount(0), m_layout(layout)
{
	m_nextFamily = s_firstFamily;
	s_firstFamily = this;
}

const PhoneKeymap::LayoutFamily * PhoneKeymap::LayoutFamily::findLayoutFamily(const char * name, bool returnNullNotDefaultIfNotFound)
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

PhoneKeymap::PhoneKeymap() : m_shiftMode(PhoneKeymap::eShiftMode_Off), m_symbolMode(eSymbolMode_Off), m_shiftDown(false), m_symbolDown(false), m_autoCap(false), m_numLock(false),
	m_layoutFamily(&sQwertyFamily), m_layoutPage(eLayoutPage_plain), m_limitsDirty(true), m_limitsVersion(0)
{
	for (int r = 0; r < cKeymapRows; ++r)
		m_rowHeight[r] = 1;
}

QList<const char *> PhoneKeymap::getLayoutList()
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

const char * PhoneKeymap::getLayoutDefaultLanguage(const char * layoutName)
{
	const PhoneKeymap::LayoutFamily * family = LayoutFamily::findLayoutFamily(layoutName);
	if (family)
		return family->m_defaultLanguage;
	return NULL;
}

void PhoneKeymap::setRowHeight(int rowIndex, int height)
{
	if (VERIFY(rowIndex >= 0 && rowIndex < cKeymapRows))
		m_rowHeight[rowIndex] = height;
}

bool PhoneKeymap::setLayoutFamily(const LayoutFamily * layoutFamily)
{
	if (m_layoutFamily != layoutFamily)
	{
		m_layoutFamily = layoutFamily;
		setEditorState(m_editorState, false);
		m_limitsDirty = true;
		return true;
	}
	return false;
}

bool PhoneKeymap::setLanguageName(const std::string & name)
{
	QString shortName;
	if (::strcasecmp(name.c_str(), "none") == 0)
	{
		if (m_languageName.size())
			shortName = m_languageName + '-';	// mark this as a disabled language
		else
			shortName = "En-";
	}
	else
	{
		if (name.length() > 0)
			shortName += QChar(name[0]).toUpper();
		if (name.length() > 1)
			shortName += QChar(name[1]).toLower();
		if (name.length() > 2 && name[2] != '-')
			for (size_t k = 2; k < name.size(); ++k)
				shortName += QChar(name[k]).toLower();
	}
	if (shortName != m_languageName)
	{
		m_languageName = shortName;
		updateSymbolKey();
		return true;
	}
	return false;
}

void PhoneKeymap::keyboardCombosChanged()
{
	VirtualKeyboardPreferences & prefs = VirtualKeyboardPreferences::instance();
	int count = qMin<int>(G_N_ELEMENTS(sLanguageChoices_Extended), prefs.getKeyboardComboCount());
	for (int k = 0; k < count; k++)
		sLanguageChoices_Extended[k] = (UKey) (cKey_KeyboardComboChoice_First + k);
	sLanguageChoices_Extended[count] = cKey_None;
}

inline float fabs(float f) { return f >= 0.f ? f : -f; }

bool PhoneKeymap::updateSymbolKey()
{
	return false;
}

int PhoneKeymap::updateLimits()
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

bool PhoneKeymap::setShiftMode(PhoneKeymap::EShiftMode shiftMode)
{
	if (m_shiftMode != shiftMode)
	{
		m_shiftMode = shiftMode;
		updateMapping();
		return true;
	}
	return false;
}

bool PhoneKeymap::setAutoCap(bool autoCap)
{
	if (autoCap != m_autoCap)
	{
		m_autoCap = autoCap;
		return true;
	}
	return false;
}

bool PhoneKeymap::setSymbolMode(ESymbolMode symbolMode)
{
	if (m_symbolMode != symbolMode)
	{
		m_symbolMode = symbolMode;
		updateMapping();
		return true;
	}
	return false;
}

bool PhoneKeymap::setShiftKeyDown(bool shiftKeyDown)
{
	if (shiftKeyDown != m_shiftDown)
	{
		m_shiftDown = shiftKeyDown;
		return true;
	}
	return false;
}

bool PhoneKeymap::setSymbolKeyDown(bool symbolKeyDown)
{
	m_symbolDown = symbolKeyDown;
	return updateMapping();
}

#define UPDATE_KEY(key, value) if (key != value) { if (key.m_weight != value.m_weight) weightChanged = true; else layoutChanged = true; key = value; }

bool PhoneKeymap::setEditorState(const PalmIME::EditorState & editorState, bool updateTranslations)
{
	bool	layoutChanged = false;
	bool	weightChanged = false;
	bool	numLock = false;
	const CustomKeys *	customKeys = isSymbolActive() ? &m_layoutFamily->m_symbolKeys : &m_layoutFamily->m_plainKeys;

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
		customKeys = &m_layoutFamily->m_emailKeys;
		break;
	case PalmIME::FieldType_URL:
		customKeys = &m_layoutFamily->m_urlKeys;
		break;
	case PalmIME::FieldType_Phone:
	case PalmIME::FieldType_Number:
		if (m_layoutFamily->m_needNumLock)
			numLock = true;
		break;
	case PalmIME::FieldType_Text:
	case PalmIME::FieldType_Password:
	case PalmIME::FieldType_Search:
	case PalmIME::FieldType_Range:
	case PalmIME::FieldType_Color:
	default:
		break;
	}

	if (updateSymbolKey())
		weightChanged = true;

	const int lastRow = cKeymapRows - 1;

	float newSpaceWeight = SPACE_KEY_WEIGHT - customKeys->m_leftSpace.m_weight - customKeys->m_rightSpace.m_weight;
	float & currentSpaceWeight = m_layoutFamily->writable_wkey(m_layoutFamily->m_space_x, lastRow).m_weight;
	if (currentSpaceWeight != newSpaceWeight)
		currentSpaceWeight = newSpaceWeight, weightChanged = true;

	WKey & beforeSpace = m_layoutFamily->writable_wkey(m_layoutFamily->m_space_x - 2, lastRow);
	UPDATE_KEY(beforeSpace, customKeys->m_beforeSpace);

	WKey & leftSpace = m_layoutFamily->writable_wkey(m_layoutFamily->m_space_x - 1, lastRow);
	UPDATE_KEY(leftSpace, customKeys->m_leftSpace);

	WKey & rightSpace = m_layoutFamily->writable_wkey(m_layoutFamily->m_space_x + 1, lastRow);
	UPDATE_KEY(rightSpace, customKeys->m_rightSpace);

	WKey & afterSpace = m_layoutFamily->writable_wkey(m_layoutFamily->m_space_x + 2, lastRow);
	UPDATE_KEY(afterSpace, customKeys->m_afterSpace);

	if (weightChanged)
		m_limitsDirty = true;

	if (numLock != m_numLock)
	{
		m_numLock = numLock;
		layoutChanged = true;
	}

	if (updateTranslations)
	{
		m_localized__Enter		= fromStdUtf8(LOCALIZED("Enter"));
		m_localized__Tab		= fromStdUtf8(LOCALIZED("Tab"));
		m_localized__Next		= fromStdUtf8(LOCALIZED("Next"));
		m_localized__Previous	= fromStdUtf8(LOCALIZED("Prev"));
	}

	return layoutChanged || weightChanged;
}

bool PhoneKeymap::updateMapping()
{
	ELayoutPage	newPage = !isSymbolActive() ? eLayoutPage_plain : eLayoutPage_Alternate;
	if (newPage != m_layoutPage)
	{
		m_layoutPage = newPage;
		setEditorState(m_editorState, false);
		return true;
	}
	return false;
}

int PhoneKeymap::keyboardToKeyZone(QPoint keyboardCoordinate, QRect & outZone)
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

UKey PhoneKeymap::map(int x, int y)
{
	if (!isValidLocation(x, y))
		return cKey_None;
	const WKey & wkey = m_layoutFamily->wkey(x, y);
	UKey key = wkey.m_key;
	if (m_numLock && wkey.m_altkey >= Qt::Key_0 && wkey.m_altkey <= Qt::Key_9)
	{
		if (!isShiftActive())
			key = wkey.m_altkey;
	}
	else
	{
		// for letters & function keys, use alternate layout when symbol is active, for non-letter, use alternate layout when shift is active
		if (((key >= Qt::Key_A && key <= Qt::Key_Z) || UKeyIsFunctionKey(key)) ? m_layoutPage == eLayoutPage_Alternate : isShiftActive())
			key = wkey.m_altkey;
	}
	return key;
}

int PhoneKeymap::xCenterOfKey(int touchX, int x, int y, float weight)
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
	return center;
}

int PhoneKeymap::yCenterOfRow(int y)
{	// slightly reduce the effective height of the top & lowest rows, by moving their centers further away from the closest inner row
	const int cReduceFactorTopRow = 4;		// 1 = most neutral factor: top row tallest, higher factors reduce its effective height
	const int cReduceFactorBottomRow = 3;	// 1 = most neutral factor: bottom row tallest, higher factors reduce its effective height
	int top_y = y > 0 ? m_vlimits[y - 1] : 0;
	int lower_y = m_vlimits[y];
	if (y == 0)
		return lower_y / cReduceFactorTopRow;
	else if (y < cKeymapRows - 1)
		return (top_y + lower_y) / 2;
	else
		return (top_y + (cReduceFactorBottomRow - 1) * lower_y) / cReduceFactorBottomRow;
}

inline int square(int x)										{ return x * x; }

QPoint PhoneKeymap::pointToKeyboard(const QPoint & location)
{
	updateLimits();
	int locy = location.y() - m_rect.top() + 1;
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
#if 1 // try to improve accuracy by looking if the touch point is closer to some other key above or below...
			int center_y = yCenterOfRow(y);	// vertical center of found key
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
					float o_weight = m_layoutFamily->weight(ox, oy);
					int center_ox = xCenterOfKey(locx, ox, oy, o_weight);								// horizontal center of other candidate
					int center_oy = yCenterOfRow(oy);													// vertical center of other candidate
					int first_d = square(locy - center_y) + square(locx - center_x);					// "distance" between tap location & first found key
					int o_d = square(locy - center_oy) + square(locx - center_ox);						// "distance" between tap location & other candidate
					bool use_o = false;
					if (o_weight < 1 && wkey.m_weight >= 1)
						use_o = 3 * o_d < first_d;
					else if (wkey.m_weight < 1 && o_weight >= 1)
						use_o = o_d < 3 * first_d;
					else
						use_o = o_d < first_d;
					if (use_o)
						x = ox, y = oy, changed = true;
				}
			}
#endif
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

std::string PhoneKeymap::pointToKeys(const QPoint & point)
{
	return "";
}

bool PhoneKeymap::generateKeyboardLayout(const char * fullPath)
{
	if (rect().width() <= 0 || rect().height() <= 0)
		return false;
	QFile file(fullPath);
	if (VERIFY(file.open(QIODevice::WriteOnly)))
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
				if (UKeyIsUnicodeQtKey(key) && key != Qt::Key_Space && keyboardToKeyZone(QPoint(x, y), r) > 0)
				{
					r.translate(-rect().left(), -rect().top());
					r.adjust(6, 6, -6, -6);
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
					file.write(string_printf("<key keyLabel=\"%s\" keyType=\"%s\" keyLeft=\"%ddp\" keyTop=\"%ddp\" keyWidth=\"%ddp\" keyHeight=\"%ddp\" />\n",
						  text.toUtf8().data(), key < 256 && isalpha(key) ? "regional" : "nonRegional",
						  r.left(), r.top(), r.width(), r.height()).c_str());
				}
			}
		file.write("</area>\n\n");
		file.write("</keyboard>\n\n");
		file.close();
	}
	return true;
}

std::string PhoneKeymap::getKeyboardLayoutAsJson()
{
	pbnjson::JValue layout = pbnjson::Object();
	layout.put("layout", m_layoutFamily->m_name);
	layout.put("width", rect().width());
	layout.put("height", rect().height());
	pbnjson::JValue keys = pbnjson::Array();
	QRect	r;
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

const UKey * PhoneKeymap::getExtendedChars(QPoint keyboardCoordinate)
{
	if (isValidLocation(keyboardCoordinate))
	{
		const WKey & wkey = m_layoutFamily->wkey(keyboardCoordinate.x(), keyboardCoordinate.y());
		if ((wkey.m_key >= Qt::Key_A && wkey.m_key <= Qt::Key_Z) || UKeyIsFunctionKey(wkey.m_key))
			return (m_layoutPage == eLayoutPage_Alternate && wkey.m_key != wkey.m_altkey) ? wkey.m_altExtended : wkey.m_extended;
		else
			return wkey.m_extended;
	}
	return NULL;
}

PhoneKeymap::ETabAction PhoneKeymap::tabAction() const
{
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

QString PhoneKeymap::getKeyDisplayString(UKey key, bool logging)
{
	if (UKeyIsFunctionKey(key))
	{
		if (UKeyIsKeyboardComboKey(key))
		{
			int index = key - cKey_KeyboardComboChoice_First;
			VirtualKeyboardPreferences & prefs = VirtualKeyboardPreferences::instance();
			if (VERIFY(index >= 0 && index < prefs.getKeyboardComboCount()))
				return prefs.getkeyboardCombo(index).language.c_str();
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
		case cKey_Emoticon_Options:						return ":)";
		case cKey_Emoticon_Frown:						return ":-(";
		case cKey_Emoticon_Cry:							return ":'(";
		case cKey_Emoticon_Smile:						return ":-)";
		case cKey_Emoticon_Wink:						return ";-)";
		case cKey_Emoticon_Yuck:						return ":-P";
		case cKey_Emoticon_Gasp:						return ":-O";
		case cKey_Emoticon_Heart:						return "<3";
		case cKey_Symbol:								return  (symbolMode() == PhoneKeymap::eSymbolMode_Lock) ? "ABC" : "123";
		case cKey_DotCom:								return ".com";
		case cKey_DotCoUK:								return ".co.uk";
		case cKey_DotOrg:								return ".org";
		case cKey_DotDe:								return ".de";
		case cKey_DotEdu:								return ".edu";
		case cKey_DotFr:								return ".fr";
		case cKey_DotGov:								return ".gov";
		case cKey_DotNet:								return ".net";
		case cKey_DotUs:								return ".us";
		case cKey_ColonSlashSlash:						return "://";
		case cKey_HTTPColonSlashSlash:					return "http://";
		case cKey_HTTPSColonSlashSlash:					return "https://";
		case Qt::Key_Left:								return QString(QChar(0x2190) /* left arrow */) + "A";
		case Qt::Key_Right:								return QString("A") + QChar(0x2192) /* right arrow */;
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
		case cKey_MorePopup:							return "...";
		default:			return QString();
		}
	}
	return isCapOrAutoCapActive() ? QChar(key).toUpper() : QChar(key).toLower();
}

}; // namespace Phone_Keyboard
