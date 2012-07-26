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



#ifndef PRE_KEYMAP_H
#define PRE_KEYMAP_H

#include <palmimedefines.h>

#include <qlist.h>
#include <qrect.h>
#include <qstring.h>

class QFile;

namespace Pre_Keyboard {

// We're back to using Qt::Key definitions. This typedef allows us to change our mind...
typedef Qt::Key UKey;

// Constants defined to make keyboard definitions more readable (and more stable as we change the values they point to...)
// Note that except for the Euro sign, they are only meant to represent a key and are never used as unicode characters for display purposes...

const UKey cKey_None = UKey(0);						// nothing...
const UKey cKey_Euro = UKey(0x20ac);				// unicode "EURO SIGN" €
const UKey cKey_SymbolPicker = Qt::Key_Control;		// not used in virtual keyboard: that's to trigger Webkit's symbol picker
const UKey cKey_Symbol = Qt::Key_Alt;				// "new" virtual keyboard symbol key

// special keys not mapping to already existing Qt keys: use unique value used only internaly.
const UKey cKey_ToggleSuggestions = UKey(0x01200200);
const UKey cKey_ToggleLanguage = UKey(0x01200201);
const UKey cKey_SwitchToQwerty = UKey(0x01200202);
const UKey cKey_SwitchToAzerty = UKey(0x01200203);
const UKey cKey_SwitchToQwertz = UKey(0x01200204);
const UKey cKey_ShowXT9Regions = UKey(0x01200209);
const UKey cKey_CreateDefaultKeyboards = UKey(0x0120020D);
const UKey cKey_ClearDefaultKeyboards = UKey(0x0120020E);
const UKey cKey_Hide = UKey(0x0120020F);
const UKey cKey_ShowKeymapRegions = UKey(0x01200210);
const UKey cKey_StartStopRecording = UKey(0x01200211);

const UKey cKey_ToggleSoundFeedback = UKey(0x01200216);

// Keys that correspond to some text entry (label & entered text are equal)
const UKey cKey_DotCom = UKey(0x01200300);
const UKey cKey_DotOrg = UKey(0x01200301);
const UKey cKey_DotNet = UKey(0x01200302);
const UKey cKey_DotEdu = UKey(0x01200303);
const UKey cKey_DotGov = UKey(0x01200304);
const UKey cKey_DotCoUK = UKey(0x01200305);
const UKey cKey_DotDe = UKey(0x01200306);
const UKey cKey_DotFr = UKey(0x01200307);
const UKey cKey_DotUs = UKey(0x01200308);
const UKey cKey_ColonSlashSlash = UKey(0x01200309);
const UKey cKey_HTTPColonSlashSlash = UKey(0x0120030A);
const UKey cKey_HTTPSColonSlashSlash = UKey(0x0120030B);
const UKey cKey_Emoticon_Frown = UKey(0x0120030C);
const UKey cKey_Emoticon_Cry = UKey(0x0120030D);
const UKey cKey_Emoticon_Smile = UKey(0x0120030E);
const UKey cKey_Emoticon_Wink = UKey(0x0120030F);
const UKey cKey_Emoticon_Yuck = UKey(0x01200310);
const UKey cKey_Emoticon_Gasp = UKey(0x01200311);
const UKey cKey_Emoticon_Heart = UKey(0x01200312);
const UKey cKey_Emoticon_Options = UKey(0x01200313);

// Keys used for keyboard/language selections
const UKey cKey_KeyboardComboChoice_First = UKey(0x01200400);
const UKey cKey_KeyboardComboChoice_Last = UKey(0x012004ff);

// helper: can a UKey code be used as a Qt::Key and/or as an acceptable unicode character?
inline bool UKeyIsUnicodeQtKey(UKey ukey)		{ return ukey >= ' ' && ukey < Qt::Key_Escape; }
inline bool UKeyIsFunctionKey(UKey ukey)		{ return ukey >= Qt::Key_Escape; }
inline bool UKeyIsTextShortcutKey(UKey ukey)	{ return ukey >= 0x01200300 && ukey <= 0x012003FF; }
inline bool UKeyIsKeyboardComboKey(UKey ukey)	{ return ukey >= cKey_KeyboardComboChoice_First && ukey <= cKey_KeyboardComboChoice_Last;}
inline bool UKeyIsEmoticonKey(UKey ukey)		{ return ukey >= cKey_Emoticon_Frown && ukey <= cKey_Emoticon_Options; }

const QPoint cOutside(-1, -1);		// special value meaning representing "outside of keyboard", or "no key".

class PreKeymap
{
public:
	enum {
		cKeymapRows = 4,
		cKeymapColumns = 10,
	};

	enum EShiftMode
	{
		eShiftMode_Undefined = -1,

		eShiftMode_Off = 0,
		eShiftMode_Once,
		eShiftMode_CapsLock
	};

	enum ESymbolMode
	{
		eSymbolMode_Undefined = -1,

		eSymbolMode_Off = 0,
		eSymbolMode_Lock
	};

	enum ELayoutPage {
		eLayoutPage_plain = 0,
		eLayoutPage_Alternate,

		eLayoutPageCount = 2
	};

	enum ETabAction {
		eTabAction_Tab = 0,
		eTabAction_Next,
		eTabAction_Previous
	};

	typedef const UKey constUKeyArray[];

	struct WKey {
		void set(UKey key, float weight = 1, const UKey * extended = NULL)
		{
			m_weight = weight, m_key = key, m_altkey = key, m_extended = extended;
		}
		void set(UKey key, UKey altkey, float weight = 1, const UKey * extended = NULL)
		{
			m_weight = weight, m_key = key, m_altkey = altkey, m_extended = extended;
		}
		void hide()
		{
			m_weight = 0, m_key = cKey_None, m_altkey = cKey_None, m_extended = NULL;
		}
		bool operator !=(const WKey & rhs)
		{
			return m_weight != rhs.m_weight || m_key != rhs.m_weight || m_altkey != rhs.m_altkey || m_extended != rhs.m_extended;
		}

		float			m_weight;
		UKey			m_key;
		UKey			m_altkey;
		const UKey *	m_extended;
	};

	typedef WKey	Layout[cKeymapRows][cKeymapColumns];
	typedef float	HLimits[cKeymapRows][cKeymapColumns];
	typedef float	VLimits[cKeymapRows];

	struct LayoutFamily {

		LayoutFamily(const char * name, const char * defaultLanguage, uint16_t primaryID, uint16_t secondaryID, Layout * layout);

		const WKey &	wkey(int x, int y) const						{ return (*m_layout)[y][x]; }
		WKey &			writable_wkey(int x, int y)	 const				{ return (*m_layout)[y][x]; }
		UKey			key(int x, int y, ELayoutPage page) const		{ return (page == eLayoutPage_plain) ? wkey(x, y).m_key : wkey(x, y).m_altkey; }
		float			weight(int x, int y) const						{ return wkey(x, y).m_weight; }

		const char *	m_name;
		const char *	m_defaultLanguage;
		uint16_t		m_primaryID;
		uint16_t		m_secondaryID;

		Layout *		m_layout;

		// self registration of layout families. Start with first, iterate until nextFamily is null.
		const LayoutFamily *		m_nextFamily;

		static const LayoutFamily * findLayoutFamily(const char * name, bool returnNullNotDefaultIfNotFound = true);

		static const LayoutFamily * s_firstFamily;

	};

	PreKeymap();

	void				setRect(int x, int y, int w, int h)		{ m_rect.setRect(x, y, w, h); m_limitsDirty = true; }
	const QRect &		rect() const							{ return m_rect; }
	void				setRowHeight(int rowIndex, int height);

	QPoint				pointToKeyboard(const QPoint & location);							// convert screen coordinate in keyboard coordinate
	int					keyboardToKeyZone(QPoint keyboardCoordinate, QRect & outZone);		// convert keyboard coordinate to rect of the key

	// The following functions that return a bool return true when the layout effectively changed (and you probably need to update your display)
	bool				setLayoutFamily(const LayoutFamily * layoutFamily);
	const LayoutFamily*	layoutFamily() const					{ return m_layoutFamily; }
	bool				setLanguageName(const std::string & name);		// if empty string, hide language key, otherwise show it.
	void				keyboardCombosChanged();						// called when available keyboard combos change
	QList<const char *>	getLayoutList();
	const char *		getLayoutDefaultLanguage(const char * layoutName);

	bool				setShiftMode(EShiftMode shiftMode);
	EShiftMode			shiftMode() const						{ return m_shiftMode; }
	bool				setSymbolMode(ESymbolMode symbolMode);
	ESymbolMode			symbolMode() const						{ return m_symbolMode; }
	bool				setShiftKeyDown(bool shiftKeyDown);
	bool				setSymbolKeyDown(bool symbolKeyDown);
	bool				setEditorState(const PalmIME::EditorState & editorState);
	const PalmIME::EditorState &	editorState() const			{ return m_editorState; }
	bool				setAutoCap(bool autoCap);

	bool				isSymbolActive() const					{ return ((m_symbolMode == eSymbolMode_Lock) ? 1 : 0) + (m_symbolDown ? 1 : 0) == 1; }
	bool				isShiftActive() const					{ return ((m_shiftMode == eShiftMode_Once) ? 1 : 0) + (m_shiftDown ? 1 : 0) == 1; }
	bool				isShiftDown() const						{ return m_shiftDown; }
	bool				isSymbolDown() const					{ return m_symbolDown; }
	bool				isCapActive() const						{ return (m_shiftDown && m_shiftMode == eShiftMode_Off) || (!m_shiftDown && m_shiftMode != eShiftMode_Off); }
	bool				isCapOrAutoCapActive() const			{ return m_autoCap || isCapActive(); }
	bool				isAutoCapActive() const					{ return m_autoCap; }

	UKey				map(QPoint p)							{ return map(p.x(), p.y()); }
	UKey				map(int x, int y);
	UKey				map(QPoint p, ELayoutPage page)			{ return map(p.x(), p.y(), page); }
	UKey				map(int x, int y, ELayoutPage page)		{ return isValidLocation(x, y) ? m_layoutFamily->key(x, y, page) : cKey_None; }
	quint32				getPage() const							{ return m_layoutPage; }

	ETabAction			tabAction() const;

	const char *		layoutName()							{ return m_layoutFamily->m_name; }
	uint16_t			primaryKeyboardID()						{ return m_layoutFamily->m_primaryID; }
	uint16_t			secondaryKeyboardID()					{ return m_layoutFamily->m_secondaryID; }

	bool				generateKeyboardLayout(const char * fullPath);

	static inline bool	isValidLocation(int x, int y)			{ return x >= 0 && x < cKeymapColumns && y >= 0 && y < cKeymapRows; }
	static inline bool	isValidLocation(QPoint location)		{ return isValidLocation(location.x(), location.y()); }

	const UKey *		getExtendedChars(QPoint keyboardCoordinate);			// MAY RETURN NULL!
	QString				getKeyDisplayString(UKey key, bool logging = false);	// for display purposes. NOT ALL KEYS can be handled that way! "logging" gives you more...
	bool				showEmoticonsAsGraphics()				{ return true; /*m_editorState.flags & PalmIME::FieldFlags_Emoticons*/ }

	int					updateLimits();

private:
	EShiftMode			m_shiftMode;
	ESymbolMode			m_symbolMode;
	bool				m_shiftDown;
	bool				m_symbolDown;
	bool				m_autoCap;
	bool				m_numLock;							// in number & phone fields, when numbers need shift.
	PalmIME::EditorState m_editorState;
	const LayoutFamily * m_layoutFamily;
	ELayoutPage			m_layoutPage;
	QRect				m_rect;
	int					m_rowHeight[cKeymapRows];
	HLimits				m_hlimits;
	VLimits				m_vlimits;
	bool				m_limitsDirty;
	int					m_limitsVersion;

	QString				m_languageName;

	bool				updateMapping();					// true if layout changed
	void				updateLanguageKey();

	int					xCenterOfKey(int touchX, int x, int y, float weight);
	int					yCenterOfRow(int y);
};

}; // namespace Pre_Keyboard

#endif // PRE_KEYMAP_H
