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




#ifndef TABLET_KEYBOARD_H
#define TABLET_KEYBOARD_H

#include <qpixmap.h>
#include <qtimer.h>

#include <map>
#include <stdint.h>

#include "VirtualKeyboard.h"
#include "TabletKeymap.h"
#include "IMEPixmap.h"
#include "GlyphCache.h"
#include "ShortcutsHandler.h"
#include "CandidateBar.h"

namespace Tablet_Keyboard {

/*
						 m_keymap.rect.width()
<----------- m_IMEDataInterface->m_availableSpace.get().width() ----------->
|--------------------------------------------------------------------------|
|                                                                          |
|                                                                          |
|                                                                          |
|                                                                          |
|                            Application Space                             |
|                                                                          |
|                                                                          |
|                                                                          |
|                                                                          |
|----^---------------------------------------------------------------------|--^
|    | m_keyboardTopPading  (extra space above keys)                       |  |
|----V--^------------------------------------------------------------------|  |
|       |                                                                  |  |
|       |                                                                  |  |  requestHeight(), requestedHeight(), setKeyboardHeight() affect this dimension!
|       | m_keymap.rect.height()                                           |  |
|       |                                                                  |  |                  Space used by the keyboard, as seen by the user.
|       |                                                                  |  |
|-------V------------------------------------------------------------------|--V

*/

// specs type used in GlyphCache<T> for the tablet
struct GlyphSpec {
	GlyphSpec(QString string = QString(), int fontSize = 0, bool bold = false, const QColor & frontColor = QColor(), const QColor & backColor = QColor()) : m_string(string), m_fontSize(fontSize), m_bold(bold), m_frontColor(frontColor), m_backColor(backColor) {}

	bool operator<(const GlyphSpec & rhs) const
	{
		if (m_fontSize < rhs.m_fontSize)
			return true;
		if (m_fontSize == rhs.m_fontSize)
		{
			int stringCompare = m_string.compare(rhs.m_string);
			return (stringCompare < 0 || (stringCompare == 0 && m_frontColor.rgba() < rhs.m_frontColor.rgba()));
		}
		return false;
	}

	void applyFontSettings(QFont & font) const
	{
		font.setPixelSize(m_fontSize);
		font.setBold(m_bold);
	}

	QString		description() const
	{
		return QString("'") + m_string + "', font size: " + QString::number(m_fontSize) + ", " + (m_bold ? "bold" : "plain");
	}

	QString		m_string;
	int			m_fontSize;
	bool		m_bold;
	QColor		m_frontColor;
	QColor		m_backColor;
};

class TabletKeyboard : public VirtualKeyboard
{
	Q_OBJECT

	struct Touch
	{
		Touch() : m_lastTouchTime(0), m_keyCoordinate(-1, -1), m_visible(true), m_consumed(false), m_inCandidateBar(false) {}

		uint64_t	m_lastTouchTime;
		QPointF		m_lastPosition;		// coordinate in pixels in keymap area
		QPointF		m_firstPosition;	// coordinate in pixels in keymap area
		QPoint		m_keyCoordinate;	// coordinate in key layout (1, 1) is second key from the left of second row...
		bool		m_visible;
		bool		m_consumed;
		bool		m_inCandidateBar;
	};

	template <class T> T selectFromKeyType(UKey key, T letter, T functionKeys, T otherKeys)
	{
		return UKeyIsFunctionKey(key) && !UKeyIsTextShortcutKey(key) ? functionKeys : (((key >= Qt::Key_A && key <= Qt::Key_Z) || key == Qt::Key_Space) ? letter : otherKeys);
	}

public:
	TabletKeyboard(IMEDataInterface * dataInterface);
	virtual ~TabletKeyboard();

	// For debug purposes, to implement luna-send command on an existing instance
	static TabletKeyboard * getExistingInstance()		{ return s_instance; }

	void	requestSize(int size);
	void	requestHeight(int height);
	void	changePresetHeightForSize(int size, int height);
	bool	setBoolOption(const std::string & optionName, bool value);
	bool	setIntOption(const std::string & optionName, int value);
	bool	getValue(const std::string & name, std::string & outValue);
	void	setKeyboardCombo(const std::string & layoutName, const std::string & languageName, bool showLanguageKey);
	void	keyboardCombosChanged()					{ m_keymap.keyboardCombosChanged(); }	// called by VirtualKeyboardPreferences when combos change

	QList<const char *>	getLayoutNameList()			{ return m_keymap.getLayoutList(); }
	const char *		getLayoutDefaultLanguage(const char * layoutName)		{ return m_keymap.getLayoutDefaultLanguage(layoutName); }

	bool	inLandscapeOrientation() const			{ const QRect & frame = m_IMEDataInterface->m_availableSpace.get(); return frame.width() >= frame.height(); }

	bool	idle();

public Q_SLOTS:
	// slots for IMEDataInterface signals
	void	availableSpaceChanged(const QRect & size);
	void	visibleChanged(const bool & visible);
	void	editorStateChanged(const PalmIME::EditorState & state);
	void	autoCapChanged(const bool & autoCap);

	// slots for candidate bar
	void	triggerRepaint()						{ m_IMEDataInterface->invalidateRect(m_keymap.rect()); }
	void	candidateBarResized()					{ availableSpaceChanged(m_IMEDataInterface->m_availableSpace.get()); }

public:
	virtual void touchEvent(const QTouchEvent& te);
	virtual void paint(QPainter& painter);
	virtual void tapEvent(const QPoint& tapPt);
	virtual void screenEdgeFlickEvent();

protected:
	void	updateTouch(int id, QPointF position);
	void	releaseTouch(int id);
	void	drawKeyBackground(QPainter & painter, QRect & location, const QPoint & keyCoord, UKey key, bool pressed, int count)
	{
		m_nineTileSprites.draw(painter, location, getKeyBackground(keyCoord, key), pressed, count, m_9tileCorner);
	}

	QPixmap &	getKeyBackground(const QPoint & keyCoord, UKey key);
	void	draw9Tile(QPainter & painter, QRect & location, QPixmap & pixmap, bool pressed);
	void	drawKeyCap(QPainter * painter, GlyphRenderer<GlyphSpec> & renderer, QRect location, const QPoint & keyCoord, UKey key, bool pressed);
	bool	updateBackground();
	void	keyboardLayoutChanged();
	void	handleKey(UKey key, QPointF where);
    void    sendKeyDownUp(Qt::Key key, Qt::KeyboardModifiers modifiers);

	void	setShiftMode(TabletKeymap::EShiftMode shiftMode);
	void	setSymbolMode(TabletKeymap::ESymbolMode symbolMode);
	bool	setShiftKeyDown(bool shiftKeyDown);
	bool	setSymbolKeyDown(bool symbolKeyDown);

	bool	canRepeat(UKey key) const;
	void	stopRepeat();

	void	showSuggestions(bool show);
	void	showKeymapRegions();

	int		getKeyboardHeight(UKey ukey);

public:
	bool	setExtendedKeys(QPoint keyCoord, bool cancelIfSame = false);
	void	clearExtendedkeys();
	bool	pointToExtendedPopup(QPointF position, UKey & outKey);
	void	getExtendedPopupSpec(int & outCellCount, int & outLineCount, int & outLineLength);

	void	setKeyboardHeight(int height, bool notify = true);
	void	syncKeymap();

	void	queueIdlePrerendering();

	void	makeSound(UKey key);

protected Q_SLOTS:
	void	repeatChar();

private:
	static TabletKeyboard *	s_instance;			// only valid while a keyboard exists.

	std::map<int, Touch>	m_touches;			// where the user touches the screen, only while touching in key-coordinate (0, 0) = Q, (0, 1) = W...
	bool				m_shiftDown;
	bool				m_symbolDown;
	uint64_t			m_lastShiftTime;
	uint64_t			m_lastUnlockTime;

	bool				m_resizeMode;
	int					m_keyboardTopPading;
	int					m_requestedHeight;
	NineTileCorner		m_9tileCorner;

	TabletKeymap		m_keymap;				// current keymap.
	QPixmap	*			m_keyboardBackgound;	// current keyboard, cached version using the assets below without text rendering nor shift keys.
	int					m_keyboardLimitsVersion;// version of m_keymap limits used to build m_keyboard & m_keyboardBackground
	bool				m_keyboardDirty;		// does m_keyboard need to be rebuilt before drawing onscreen?
	int					m_presetHeight[cKey_Resize_Last - cKey_Resize_First + 1];	// height in pixels for each size. Index 0 is the smallest, last index the largest

	CANDIDATEBAR		m_candidateBar;
	bool				m_candidateBarLayoutOutdated;
	const TabletKeymap::LayoutFamily * m_generatedKeymapLayout;

	QTimer				m_timer;				// repeat timer.
	QPoint				m_repeatKey;			// key being repeated.
	uint64_t			m_repeatStartTime;		// when the repeated key was first pressed

	const UKey *		m_extendedKeys;			// set when a popout with extended keys is being shown
	QRect				m_extendedKeysFrame;	// frame of the currently displayed extended keys, if any. Undefined value when m_extendedKeys is NULL.
	int					m_extendedKeysPointer;	// where the extended keys popup pointer to the key should be (X coordinate)
	UKey				m_extendedKeyShown;

	ShortcutsHandler	m_shortcutsHandler;

	bool				m_diamondOptimization;

	bool				m_idleInit;

	// keyboard special key assets.
	IMEPixmap			m_backspace;
	IMEPixmap			m_shift;
	IMEPixmap			m_shift_on;
	IMEPixmap			m_shift_lock;
	IMEPixmap			m_hide;

	IMEPixmap			m_emoticon_frown;
	IMEPixmap			m_emoticon_cry;
	IMEPixmap			m_emoticon_smile;
	IMEPixmap			m_emoticon_wink;
	IMEPixmap			m_emoticon_yuck;
	IMEPixmap			m_emoticon_gasp;
	IMEPixmap			m_emoticon_heart;

	IMEPixmap			m_emoticon_frown_small;
	IMEPixmap			m_emoticon_cry_small;
	IMEPixmap			m_emoticon_smile_small;
	IMEPixmap			m_emoticon_wink_small;
	IMEPixmap			m_emoticon_yuck_small;
	IMEPixmap			m_emoticon_gasp_small;
	IMEPixmap			m_emoticon_heart_small;

	// keyboard background assets.
	IMEPixmap			m_background;
	IMEPixmap			m_drag_handle;
	IMEPixmap			m_drag_highlight;
	IMEPixmap			m_white_key;
	IMEPixmap			m_gray_key;
	IMEPixmap			m_black_key;
	IMEPixmap			m_shift_on_key;
	IMEPixmap			m_shift_lock_key;
	IMEPixmap			m_short_gray_key;

	// popup assets.
	IMEPixmap			m_popup;
	IMEPixmap			m_popup_2;
	IMEPixmap			m_popup_key;

	NineTileSprites		m_nineTileSprites;
	GlyphCache<GlyphSpec>	m_glyphCache;
};

}; // namespace TabletKeyboard

#endif

