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


#include "PreKeymap.h"

#include "KeyLocationRecorder.h"
#include "Localization.h"
#include "Logging.h"
#include "Utils.h"
#include "VirtualKeyboardPreferences.h"

#include <QFile>
#include <qdebug.h>

namespace Pre_Keyboard {

#define KEY_1(w, k) { w, k, k, NULL }

#define NOKEY_1 { 0, cKey_None, cKey_None, NULL }
#define NOKEY_2 NOKEY_1, NOKEY_1
#define NOKEY_3 NOKEY_2, NOKEY_1
#define NOKEY_4 NOKEY_3, NOKEY_1
#define NOKEY_5 NOKEY_4, NOKEY_1
#define NOKEY_6 NOKEY_5, NOKEY_1

#define KEY_2(w, k, a) { w, k, a, NULL }
#define KEY_3(w, k, a, e) { w, k, a, e }

#define QWERTY_TOP_10(w)			{ w, Qt::Key_Q,			Qt::Key_Slash,				NULL },\
									{ w, Qt::Key_W,			Qt::Key_Plus,				NULL },\
									{ w, Qt::Key_E,			Qt::Key_1,					NULL },\
									{ w, Qt::Key_R,			Qt::Key_2,					NULL },\
									{ w, Qt::Key_T,			Qt::Key_3,					NULL },\
									{ w, Qt::Key_Y,	 		Qt::Key_ParenLeft,			NULL },\
									{ w, Qt::Key_U,			Qt::Key_ParenRight,			NULL },\
									{ w, Qt::Key_I,			Qt::Key_Percent,			NULL },\
									{ w, Qt::Key_O,			Qt::Key_QuoteDbl,			NULL },\
									{ w, Qt::Key_P,			Qt::Key_Equal,				NULL }

#define QWERTY_MID_9(w)				{ w, Qt::Key_A,			Qt::Key_Ampersand,			NULL },\
									{ w, Qt::Key_S,			Qt::Key_Minus,				NULL },\
									{ w, Qt::Key_D,			Qt::Key_4,					NULL },\
									{ w, Qt::Key_F,			Qt::Key_5,					NULL },\
									{ w, Qt::Key_G,			Qt::Key_6,					NULL },\
									{ w, Qt::Key_H,			Qt::Key_Dollar,				NULL },\
									{ w, Qt::Key_J,			Qt::Key_Exclam,				NULL },\
									{ w, Qt::Key_K,			Qt::Key_Colon,				NULL },\
									{ w, Qt::Key_L,			Qt::Key_Apostrophe,			NULL }

#define QWERTY_LOW_7(w)				{ w, Qt::Key_Z,			Qt::Key_Asterisk,			NULL },\
									{ w, Qt::Key_X,			Qt::Key_7,					NULL },\
									{ w, Qt::Key_C,			Qt::Key_8,					NULL },\
									{ w, Qt::Key_V,			Qt::Key_9,					NULL },\
									{ w, Qt::Key_B,			Qt::Key_NumberSign,			NULL },\
									{ w, Qt::Key_N,			Qt::Key_Question,			NULL },\
									{ w, Qt::Key_M,			Qt::Key_Semicolon,			NULL }

#define BOTTOM_ROW KEY_1(1, cKey_None), KEY_1(1.5, Qt::Key_Shift), KEY_2(1, Qt::Key_At, Qt::Key_0), KEY_1(3, Qt::Key_Space), KEY_1(1, Qt::Key_Period), KEY_1(2.5, cKey_None), NOKEY_4

static PreKeymap::Layout sQwerty = {
	{ QWERTY_TOP_10(1) },
	{ QWERTY_MID_9(1), KEY_1(1, Qt::Key_Backspace) },
	{ KEY_1(1, Qt::Key_Shift), QWERTY_LOW_7(1), KEY_2(1, Qt::Key_Comma, Qt::Key_Underscore), KEY_1(1, Qt::Key_Return) },
	{ BOTTOM_ROW }
};

#define QWERTZ_TOP_10(w)			{ w, Qt::Key_Q,			Qt::Key_Slash,				NULL },\
									{ w, Qt::Key_W,			Qt::Key_Plus,				NULL },\
									{ w, Qt::Key_E,			Qt::Key_1,					NULL },\
									{ w, Qt::Key_R,			Qt::Key_2,					NULL },\
									{ w, Qt::Key_T,			Qt::Key_3,					NULL },\
									{ w, Qt::Key_Z,	 		Qt::Key_ParenLeft,			NULL },\
									{ w, Qt::Key_U,			Qt::Key_ParenRight,			NULL },\
									{ w, Qt::Key_I,			Qt::Key_Percent,			NULL },\
									{ w, Qt::Key_O,			Qt::Key_QuoteDbl,			NULL },\
									{ w, Qt::Key_P,			Qt::Key_Equal,				NULL }

#define QWERTZ_MID_9(w)				{ w, Qt::Key_A,			Qt::Key_Ampersand,			NULL },\
									{ w, Qt::Key_S,			Qt::Key_Minus,				NULL },\
									{ w, Qt::Key_D,			Qt::Key_4,					NULL },\
									{ w, Qt::Key_F,			Qt::Key_5,					NULL },\
									{ w, Qt::Key_G,			Qt::Key_6,					NULL },\
									{ w, Qt::Key_H,			cKey_Euro,					NULL },\
									{ w, Qt::Key_J,			Qt::Key_Exclam,				NULL },\
									{ w, Qt::Key_K,			Qt::Key_Colon,				NULL },\
									{ w, Qt::Key_L,			Qt::Key_Apostrophe,			NULL }

#define QWERTZ_LOW_7(w)				{ w, Qt::Key_Y,			Qt::Key_Asterisk,			NULL },\
									{ w, Qt::Key_X,			Qt::Key_7,					NULL },\
									{ w, Qt::Key_C,			Qt::Key_8,					NULL },\
									{ w, Qt::Key_V,			Qt::Key_9,					NULL },\
									{ w, Qt::Key_B,			Qt::Key_NumberSign,			NULL },\
									{ w, Qt::Key_N,			Qt::Key_Question,			NULL },\
									{ w, Qt::Key_M,			Qt::Key_Semicolon,			NULL }

static PreKeymap::Layout sQwertz = {
	{ QWERTZ_TOP_10(1) },
	{ QWERTZ_MID_9(1), KEY_1(1, Qt::Key_Backspace) },
	{ KEY_1(1, Qt::Key_Shift), QWERTZ_LOW_7(1), KEY_2(1, Qt::Key_Comma, Qt::Key_Underscore), KEY_1(1, Qt::Key_Return) },
	{ BOTTOM_ROW }
};

#define AZERTY_TOP_10(w)			{ w, Qt::Key_A,			Qt::Key_Agrave,				NULL },\
									{ w, Qt::Key_Z,			Qt::Key_Egrave,				NULL },\
									{ w, Qt::Key_E,			Qt::Key_1,					NULL },\
									{ w, Qt::Key_R,			Qt::Key_2,					NULL },\
									{ w, Qt::Key_T,			Qt::Key_3,					NULL },\
									{ w, Qt::Key_Y, 		Qt::Key_ParenLeft,			NULL },\
									{ w, Qt::Key_U,			Qt::Key_ParenRight,			NULL },\
									{ w, Qt::Key_I,			Qt::Key_Slash,				NULL },\
									{ w, Qt::Key_O,			Qt::Key_QuoteDbl,			NULL },\
									{ w, Qt::Key_P,			Qt::Key_Plus,				NULL }

#define AZERTY_MID_10(w)			{ w, Qt::Key_Q,			Qt::Key_Eacute,				NULL },\
									{ w, Qt::Key_S,			Qt::Key_Ccedilla,			NULL },\
									{ w, Qt::Key_D,			Qt::Key_4,					NULL },\
									{ w, Qt::Key_F,			Qt::Key_5,					NULL },\
									{ w, Qt::Key_G,			Qt::Key_6,					NULL },\
									{ w, Qt::Key_H,			cKey_Euro,					NULL },\
									{ w, Qt::Key_J,			Qt::Key_Exclam,				NULL },\
									{ w, Qt::Key_K,			Qt::Key_Colon,				NULL },\
									{ w, Qt::Key_L,			Qt::Key_Apostrophe,			NULL },\
									{ w, Qt::Key_M,			Qt::Key_Minus,				NULL }

#define AZERTY_LOW_6(w)				{ w, Qt::Key_W,			Qt::Key_Asterisk,			NULL },\
									{ w, Qt::Key_X,			Qt::Key_7,					NULL },\
									{ w, Qt::Key_C,			Qt::Key_8,					NULL },\
									{ w, Qt::Key_V,			Qt::Key_9,					NULL },\
									{ w, Qt::Key_B,			Qt::Key_NumberSign,			NULL },\
									{ w, Qt::Key_N,			Qt::Key_Question,			NULL }

static PreKeymap::Layout sAzerty = {
	{ AZERTY_TOP_10(1) },
	{ AZERTY_MID_10(1) },
	{ KEY_1(1, Qt::Key_Shift), AZERTY_LOW_6(1), KEY_2(1, Qt::Key_Comma, Qt::Key_Semicolon), KEY_1(1, Qt::Key_Backspace), KEY_1(1, Qt::Key_Return) },
	{ BOTTOM_ROW }
};

static PreKeymap::LayoutFamily sQwertyFamily("qwerty", "en", IME_KBD_LANG_English, IME_KBD_SEC_PhonePad, &sQwerty);
static PreKeymap::LayoutFamily sQwertzFamily("qwertz", "de", IME_KBD_LANG_German, IME_KBD_SEC_PhonePad, &sQwertz);
static PreKeymap::LayoutFamily sAzertyFamily("azerty", "fr", IME_KBD_LANG_French, IME_KBD_SEC_PhonePad, &sAzerty);

const PreKeymap::LayoutFamily * PreKeymap::LayoutFamily::s_firstFamily = NULL;

PreKeymap::LayoutFamily::LayoutFamily(const char * name, const char * defaultLanguage, uint16_t primaryID, uint16_t secondaryID, Layout * layout) :
	m_name(name), m_defaultLanguage(defaultLanguage), m_primaryID(primaryID), m_secondaryID(secondaryID), m_layout(layout)
{
	m_nextFamily = s_firstFamily;
	s_firstFamily = this;
}

const PreKeymap::LayoutFamily * PreKeymap::LayoutFamily::findLayoutFamily(const char * name, bool returnNullNotDefaultIfNotFound)
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

PreKeymap::PreKeymap() : m_shiftMode(PreKeymap::eShiftMode_Off), m_symbolMode(eSymbolMode_Off), m_shiftDown(false), m_symbolDown(false), m_autoCap(false), m_numLock(false),
	m_layoutFamily(&sQwertyFamily), m_layoutPage(eLayoutPage_plain), m_limitsDirty(true), m_limitsVersion(0)
{
	for (int r = 0; r < cKeymapRows; ++r)
		m_rowHeight[r] = 1;
	updateLanguageKey();
}

QList<const char *> PreKeymap::getLayoutList()
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

const char * PreKeymap::getLayoutDefaultLanguage(const char * layoutName)
{
	const PreKeymap::LayoutFamily * family = LayoutFamily::findLayoutFamily(layoutName);
	if (family)
		return family->m_defaultLanguage;
	return NULL;
}

void PreKeymap::setRowHeight(int rowIndex, int height)
{
	if (VERIFY(rowIndex >= 0 && rowIndex < cKeymapRows))
		m_rowHeight[rowIndex] = height;
}

bool PreKeymap::setLayoutFamily(const LayoutFamily * layoutFamily)
{
	if (m_layoutFamily != layoutFamily)
	{
		m_layoutFamily = layoutFamily;
		updateLanguageKey();
		setEditorState(m_editorState);
		m_limitsDirty = true;
		return true;
	}
	return false;
}

bool PreKeymap::setLanguageName(const std::string & name)
{
	QString shortName;
	if (name.length() > 0)
		shortName += QChar(name[0]).toUpper();
	if (name.length() > 1)
		shortName += QChar(name[1]).toLower();
	if (shortName != m_languageName)
	{
		m_languageName = shortName;
		updateLanguageKey();
		return true;
	}
	return false;
}

void PreKeymap::keyboardCombosChanged()
{
}

inline float fabs(float f) { return f >= 0.f ? f : -f; }

void PreKeymap::updateLanguageKey()
{
}

int PreKeymap::updateLimits()
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

bool PreKeymap::setShiftMode(PreKeymap::EShiftMode shiftMode)
{
	if (m_shiftMode != shiftMode)
	{
		m_shiftMode = shiftMode;
		updateMapping();
		return true;
	}
	return false;
}

bool PreKeymap::setAutoCap(bool autoCap)
{
	if (autoCap != m_autoCap)
	{
		m_autoCap = autoCap;
		return true;
	}
	return false;
}

bool PreKeymap::setSymbolMode(ESymbolMode symbolMode)
{
	if (m_symbolMode != symbolMode)
	{
		m_symbolMode = symbolMode;
		updateMapping();
		return true;
	}
	return false;
}

bool PreKeymap::setShiftKeyDown(bool shiftKeyDown)
{
	if (shiftKeyDown != m_shiftDown)
	{
		m_shiftDown = shiftKeyDown;
		return true;
	}
	return false;
}

bool PreKeymap::setSymbolKeyDown(bool symbolKeyDown)
{
	m_symbolDown = symbolKeyDown;
	return updateMapping();
}

#define UPDATE_KEYS(x, y, plain, alt) { WKey & wkey = (*m_layoutFamily->m_layout)[y][x]; if (wkey.m_key != plain) { wkey.m_key = plain; layoutChanged = true; } if (wkey.m_altkey != alt) { wkey.m_altkey = alt; layoutChanged = true; } }

bool PreKeymap::setEditorState(const PalmIME::EditorState & editorState)
{
	m_editorState = editorState;

	return false;
}

bool PreKeymap::updateMapping()
{
	ELayoutPage	newPage = !isSymbolActive() ? eLayoutPage_plain : eLayoutPage_Alternate;
	if (newPage != m_layoutPage)
	{
		m_layoutPage = newPage;
		return true;
	}
	return false;
}

int PreKeymap::keyboardToKeyZone(QPoint keyboardCoordinate, QRect & outZone)
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

UKey PreKeymap::map(int x, int y)
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
		// for letters, use alternate layout when symbol is active, for non-letter, use alternate layout when shift is active
		if ((key >= Qt::Key_A && key <= Qt::Key_Z) ? m_layoutPage == eLayoutPage_Alternate : isShiftActive())
			key = wkey.m_altkey;
	}
	return key;
}

int PreKeymap::xCenterOfKey(int touchX, int x, int y, float weight)
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

int PreKeymap::yCenterOfRow(int y)
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

QPoint PreKeymap::pointToKeyboard(const QPoint & location)
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

bool PreKeymap::generateKeyboardLayout(const char * fullPath)
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
					UKey altKey = wkey.m_altkey;
					std::string	alt;
					if (altKey != cKey_None && altKey != key && (altKey < Qt::Key_A || altKey > Qt::Key_Z))
						alt = string_printf(" key-codes-attribute=\"%d\"", altKey);
					file.write(string_printf("<key keyLabel=\"%s\" keyType=\"%s\" keyLeft=\"%ddp\" keyTop=\"%ddp\" keyWidth=\"%ddp\" keyHeight=\"%ddp\"%s />\n",
						  text.toUtf8().data(), key < 256 && isalpha(key) ? "regional" : "nonRegional",
						  r.left(), r.top(), r.width(), r.height(), alt.c_str()).c_str());
				}
			}
		file.write("</area>\n\n");
		file.write("</keyboard>\n\n");
		file.close();
	}
	return true;
}

const UKey * PreKeymap::getExtendedChars(QPoint keyboardCoordinate)
{
	if (isValidLocation(keyboardCoordinate))
	{
		const WKey & wkey = m_layoutFamily->wkey(keyboardCoordinate.x(), keyboardCoordinate.y());
		if (wkey.m_key < Qt::Key_A || wkey.m_key > Qt::Key_Z || !isSymbolActive())
			return wkey.m_extended;
	}
	return NULL;
}

PreKeymap::ETabAction PreKeymap::tabAction() const
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

QString PreKeymap::getKeyDisplayString(UKey key, bool logging)
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

		switch (key)
		{
		case Qt::Key_Return:							return "Enter";
		case Qt::Key_Tab:
		{
			switch (tabAction())
			{
			case eTabAction_Next:						return "Next";
			case eTabAction_Previous:					return "Prev";
			case eTabAction_Tab:
			default:									return "Tab";
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
		case cKey_Symbol:								return  (symbolMode() == PreKeymap::eSymbolMode_Lock) ? "ABC" : "123";
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
		case cKey_ToggleSuggestions:					return "Suggest";
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

}; // namespace Pre_Keyboard
