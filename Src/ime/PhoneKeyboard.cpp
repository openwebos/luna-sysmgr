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



#include "PhoneKeyboard.h"

#include "KeyLocationRecorder.h"
#include "PalmIMEHelpers.h"
#include "Settings.h"
#include "SingletonTimer.h"
#include "Utils.h"
#include "VirtualKeyboardPreferences.h"

#include <QDebug>
#include <QFile>
#include <QApplication>
#include <stdlib.h>
#include <glib.h>

#include <sys/times.h>

namespace Phone_Keyboard {

/**
 * temporary XML filename
 */
#define IME_KDB_XML_FILENAME      "/tmp/kdb.xml"

#define CURRENT_TIME SingletonTimer::currentTime()

//	#define DOUBLE_TAP_DURATION Settings::LunaSettings()->tapDoubleClickDuration
#define DOUBLE_TAP_DURATION 500

#define DEBUG_TOUCH 0

inline bool KeyCap_TwoVertical(const QPoint & keyCoord, UKey key)
{
	return !UKeyIsFunctionKey(key) && (key < Qt::Key_A || key > Qt::Key_Z);
}

const int cFirstRepeatDelay = 350;
const int cFirstRepeatLongDelay = 750;
const int cLetterDeleteRepeatDelay = 120;
const int cWordDeleteRepeatDelay = 275;
const uint64_t cWordDeleteDelay = cFirstRepeatDelay + 1500;

const QPainter::RenderHints cRenderHints = QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing;

// constants used to draw the popup for extended keys
const int cPopupFontSize = 22;
const int cPopupLeftSide = 11;
const int cPopupRightSide = 10;
const int cPopupSide = 20;
const int cPopupPointerStart = 37;
const int cPopupPointerWidth = 25;
const int cPopupTopToKey = 10;
const int cPopupSingleLineMax = 5;	// if more extended chars that this, break-up in two lines
const int cPressedTranslateH = 0;
const int cPressedTranslateV = 0;

static QFont sFont("Prelude");
static QFont sPopoutFont("Prelude", 32);

static QString sElipsis(QChar(0x2026 /* … */));
const int cElipsisFontSize = 14;

const QColor cActiveColor(0xd2, 0xd2, 0xd2);
const QColor cActiveColor_back(0xd2, 0xd2, 0xd2);
const QColor cDisabledColor(0x80, 0x80, 0x80);
const QColor cDisabledColor_back(0x80, 0x80, 0x80);
const QColor cFunctionColor(0xd2, 0xd2, 0xd2);
const QColor cFunctionColor_back(0xd2, 0xd2, 0xd2);
const QColor cBlueColor(75, 151, 222);
const QColor cBlueColor_back(255, 255, 255);
const QColor cPopoutTextColor(20, 20, 20);
const QColor cPopoutTextColor_back(0xe2, 0xe2, 0xe2);

class PhoneKeyboardFactory : public VirtualKeyboardFactory
{
public:
	PhoneKeyboardFactory() : VirtualKeyboardFactory("Phone Keyboard")		{}
	InputMethod *	create(IMEDataInterface * dataInterface)				{ return new PhoneKeyboard(dataInterface); }
	EVirtualKeyboardSupport	getSupport(int maxWidth, int maxHeight)
	{
//		return eVirtualKeyboardSupport_Preferred_SizeAndLocale;		// force phone keyboard for testing!
		if (maxWidth < 1024 && maxHeight < 1024)
			return eVirtualKeyboardSupport_Preferred_Size;
		return eVirtualKeyboardSupport_Poor;
	}
};

static PhoneKeyboardFactory sPhoneKeyboardFactory;

static gboolean keyboard_idle(gpointer)
{
	PhoneKeyboard * keyboard = PhoneKeyboard::getExistingInstance();
	if (keyboard)
		return keyboard->idle();
	return false;
}

typedef DoubleDrawRendererT<GlyphSpec> DoubleDrawRenderer;

PhoneKeyboard * PhoneKeyboard::s_instance = NULL;

PhoneKeyboard::PhoneKeyboard(IMEDataInterface * dataInterface) : VirtualKeyboard(dataInterface),
	m_shiftDown(false),
	m_symbolDown(false),
	m_lastShiftTime(0),
	m_lastUnlockTime(0),
	m_keyboardTopPading(0),
	m_requestedHeight(-1),
	m_9tileCorner(22, 22),
	m_keyboardBackgound(NULL),
	m_keyboardLimitsVersion(0),
	m_keyboardDirty(true),
	m_candidateBar(m_keymap, m_IMEDataInterface),
	m_candidateBarLayoutOutdated(true),
	m_generatedKeymapLayout(NULL),
	m_timer(this), m_repeatKey(cOutside), m_repeatStartTime(0),
	m_extendedKeys(NULL),
	m_extendedKeyShown(cKey_None),
	m_shortcutsHandler(dataInterface),
	m_showPopupKeys(true),
	m_idleInit(false),
	m_backspace("icon-delete.png"),
	m_shift("icon-shift.png"),
	m_shift_on("icon-shift-on.png"),
	m_shift_lock("icon-shift-lock.png"),
	m_hide("icon-hide-keyboard.png"),
	m_emoticon_frown("/usr/palm/emoticons/emoticon-frown.png"),
	m_emoticon_cry("/usr/palm/emoticons/emoticon-cry.png"),
	m_emoticon_smile("/usr/palm/emoticons/emoticon-smile.png"),
	m_emoticon_wink("/usr/palm/emoticons/emoticon-wink.png"),
	m_emoticon_yuck("/usr/palm/emoticons/emoticon-yuck.png"),
	m_emoticon_gasp("/usr/palm/emoticons/emoticon-gasp.png"),
	m_emoticon_heart("/usr/palm/emoticons/emoticon-heart.png"),
	m_background("keyboard-bg.png"),
	m_white_key("key-white.png"),
	m_gray_key("key-gray.png"),
	m_black_key("key-black.png"),
	m_shift_on_key("key-shift-on.png"),
	m_shift_lock_key("key-shift-lock.png"),
	m_popup("popup-bg.png"),
	m_popup_2("popup-bg-2.png"),
	m_popup_key("popup-key.png"),
	m_glyphCache(600, 800)
{
	if (VERIFY(s_instance == NULL))
		s_instance = this;

	Q_ASSERT(m_IMEDataInterface);

	IMEPixmap::setDefaultLocation("keyboard-phone");

	for (int r = 0; r < PhoneKeymap::cKeymapRows; ++r)
		m_keymap.setRowHeight(r, m_white_key.height() / 2);

	m_presetHeight[0] = 377;	// portrait
	m_presetHeight[1] = 260;	// landscape

	connect(&m_IMEDataInterface->m_availableSpace, SIGNAL(valueChanged(const QRect &)), SLOT(availableSpaceChanged(const QRect &)));
	connect(&m_IMEDataInterface->m_visible, SIGNAL(valueChanged(const bool &)), SLOT(visibleChanged(const bool &)));
	connect(&m_IMEDataInterface->m_editorState, SIGNAL(valueChanged(const PalmIME::EditorState &)), SLOT(editorStateChanged(const PalmIME::EditorState &)));
	connect(&m_IMEDataInterface->m_autoCap, SIGNAL(valueChanged(const bool &)), SLOT(autoCapChanged(const bool &)));
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(repeatChar()));

	m_candidateBar.font().setPixelSize(24);

	connect(&m_candidateBar, SIGNAL(needsRedraw()), SLOT(triggerRepaint()));
	connect(&m_candidateBar, SIGNAL(resized()), SLOT(candidateBarResized()));

	// init size
	VirtualKeyboardPreferences::instance().applyInitSettings(this);
}

PhoneKeyboard::~PhoneKeyboard()
{
	if (VERIFY(s_instance == this))
		s_instance = NULL;
}

void PhoneKeyboard::editorStateChanged(const PalmIME::EditorState & state)
{
	bool layoutChanged = false;

	if (m_keymap.symbolMode() == PhoneKeymap::eSymbolMode_Lock)
		if (m_keymap.setSymbolMode(PhoneKeymap::eSymbolMode_Off))
			layoutChanged = true;

	if (m_keymap.setEditorState(state))
		layoutChanged = true;

	m_candidateBar.setEditorState(state);

	if (layoutChanged)
		keyboardLayoutChanged();

	m_shortcutsHandler.resetEditor(state);
}

void PhoneKeyboard::autoCapChanged(const bool & autoCap)
{
	if (m_keymap.setAutoCap(autoCap))
		keyboardLayoutChanged();
}

void PhoneKeyboard::setShiftMode(PhoneKeymap::EShiftMode shiftMode)
{
	if (m_keymap.setShiftMode(shiftMode))
		keyboardLayoutChanged();
}

void PhoneKeyboard::setSymbolMode(PhoneKeymap::ESymbolMode symbolMode)
{
	if (m_keymap.setSymbolMode(symbolMode))
		keyboardLayoutChanged();
}

void PhoneKeyboard::setKeyboardCombo(const std::string & layoutName, const std::string & languageName, bool showLanguageKey)
{
	const PhoneKeymap::LayoutFamily * layoutFamily = PhoneKeymap::LayoutFamily::findLayoutFamily(layoutName.c_str(), false);	// get default if not found
	bool changed = false;

	if (m_keymap.setLayoutFamily(layoutFamily))
	{
		changed = true;
		KeyLocationRecorder::instance().keyboardSizeChanged(m_keymap.layoutName(), m_keymap.rect());
	}
	syncKeymap();

	if (m_keymap.setLanguageName(showLanguageKey ? languageName : ""))
		changed = true;

	m_candidateBar.setLanguage(languageName);

	if (changed)
		keyboardLayoutChanged();
}

void PhoneKeyboard::syncKeymap()
{
	if (m_keymap.layoutFamily() != m_generatedKeymapLayout)
	{
		if (VERIFY(m_keymap.generateKeyboardLayout(IME_KDB_XML_FILENAME)))
			m_generatedKeymapLayout = m_keymap.layoutFamily();
		else
			m_generatedKeymapLayout = NULL;
		m_candidateBarLayoutOutdated = true;
	}
	if (m_candidateBarLayoutOutdated && m_generatedKeymapLayout)
	{
		if (m_candidateBar.loadKeyboardLayoutFile(IME_KDB_XML_FILENAME, m_generatedKeymapLayout->m_primaryID, m_generatedKeymapLayout->m_secondaryID))
			m_candidateBarLayoutOutdated = false;
	}
}

void PhoneKeyboard::showSuggestions(bool show)
{
	m_candidateBar.setEnabled(show);
	if (show)
	{
		m_candidateBarLayoutOutdated = true;
		syncKeymap();
		keyboardLayoutChanged();
		VirtualKeyboardPreferences::instance().activateCombo();	// for language update...
	}
}

void PhoneKeyboard::visibleChanged(const bool & visible)
{
	m_candidateBar.clearCandidates();
	if (visible)
	{
		setKeyboardHeight(m_requestedHeight);
	}
	else
	{
		m_keymap.setSymbolMode(PhoneKeymap::eSymbolMode_Off);
		m_keymap.setShiftMode(PhoneKeymap::eShiftMode_Off);
		clearExtendedkeys();
	}
}

bool PhoneKeyboard::setBoolOption(const std::string & optionName, bool value)
{
	if (optionName == "suggestions")
	{
		showSuggestions(value);
	}
	else if (optionName == "popupkeys")
	{
		m_showPopupKeys = value;
		triggerRepaint();
	}
	else
	{
		g_warning("PhoneKeyboard::setBoolOption: \"%s\" is not supported.", optionName.c_str());
		return false;
	}
	return true;
}

bool PhoneKeyboard::setIntOption(const std::string & optionName, int value)
{
	g_warning("PhoneKeyboard::setIntOption: \"%s\" is not supported.", optionName.c_str());
	return false;
}

bool PhoneKeyboard::getValue(const std::string & name, std::string & outValue)
{
	if (name == "height")
	{
		outValue = string_printf("%d", m_requestedHeight);
		return true;
	}
	else if (name == "keyboard_layout")
	{
		outValue = m_keymap.getKeyboardLayoutAsJson();
		return true;
	}
	else if (name == "autocap")
	{
		outValue = m_keymap.isAutoCapActive() ? "1" : "0";
		return true;
	}
	return false;
}

void PhoneKeyboard::requestSize(int size)
{
	requestHeight(m_presetHeight[inLandscapeOrientation()]);
}

void PhoneKeyboard::requestHeight(int height)
{
	m_requestedHeight = height;
	setKeyboardHeight(height);
	if (height > 0)
		queueIdlePrerendering();
}

void PhoneKeyboard::changePresetHeightForSize(int size, int height)
{
	bool landscape = (size != 0);
	m_presetHeight[landscape] = qBound<int>(10, height, 2 * m_background.height());
	if (landscape == inLandscapeOrientation())
		requestHeight(height);
}

void PhoneKeyboard::availableSpaceChanged(const QRect & size)
{
	m_candidateBar.commit();
	m_extendedKeys = NULL;
	m_keymap.setRect(0, 0, 0, 0);
	m_candidateBar.frame().setRect(0, 0, 0, 0);
	m_keyboardTopPading = 0;

	// use the height preset for that orientation
	m_requestedHeight = m_presetHeight[inLandscapeOrientation()];
	setKeyboardHeight(m_requestedHeight);

	queueIdlePrerendering();
}

void PhoneKeyboard::setKeyboardHeight(int height, bool notify)
{
	const QRect & availableSpace = m_IMEDataInterface->m_availableSpace.get();
	int width = availableSpace.width();
	int screenHeight = availableSpace.height();
	height = qBound<int>(50, height, screenHeight - 28);
	if (VERIFY(height > 0))
	{
		m_keyboardDirty = true;

		// assets give us "ideal" non-scaled sizes. Proportionaly adjust m_keyboardTopPading
		int fullHeight = m_background.height();
		int fullKeymapHeight = PhoneKeymap::cKeymapRows * m_white_key.height() / 2;
		if (fullHeight < fullKeymapHeight)
			fullHeight = fullKeymapHeight;	// if background shorter than assets, stretch background!
		int keymapHeight = height * fullKeymapHeight / fullHeight;
		m_keyboardTopPading = height - keymapHeight;
		if (m_keyboardTopPading < 0)
			m_keyboardTopPading = 0;
		// PhoneKeymap pushed at the bottom of the available space
		m_keymap.setRect(0, availableSpace.height() - keymapHeight, width, keymapHeight);

		if (notify)
		{
			keyboardLayoutChanged();
			KeyLocationRecorder::instance().keyboardSizeChanged(m_keymap.layoutName(), m_keymap.rect());
		}

		int candidateBarHeight = m_candidateBar.enabled() ? m_white_key.height() / 2 : 0;
		m_candidateBar.frame().setRect(0, m_keymap.rect().top() - candidateBarHeight - m_keyboardTopPading, width, candidateBarHeight);
		//g_debug("PhoneKeyboard::setKeyboardHeight: %d pixels of height, Setting keymap rect to: %d, %d, %dx%d, candidateBar: %d", height, m_keymap.rect().left(), m_keymap.rect().top(), m_keymap.rect().width(), m_keymap.rect().height(), candidateBarHeight);

		m_9tileCorner.m_trimV = 0;
		m_9tileCorner.m_trimH = 0;
//		if (availableSpace.width() < 480)
//			m_9tileCorner.m_trimH = 4;
//		else if (availableSpace.width() == 480)
//			m_9tileCorner.m_trimH = 3;
//		else if (keymapHeight >= fullKeymapHeight)
//			m_9tileCorner.m_trimH = 0;
//		else
//		{
//			m_9tileCorner.m_trimH = (fullKeymapHeight - keymapHeight) / 40;
//			if (m_9tileCorner.m_trimH > 3)
//				m_9tileCorner.m_trimH = 3;
//		}
		//g_critical("9Tile Shrink: %g-%g (%d)", m_9tileCorner.m_trimH, m_9tileCorner.m_trimV, fullKeymapHeight - keymapHeight);
	}
	else
		g_debug("PhoneKeyboard::setKeyboardHeight: FAILED! height: %d, requestedHeight: %d, portrait: %d, landscape %d, background height: %d, keyboard height: %d, available: %d-%d %dx%d.", height, m_requestedHeight,
				  m_presetHeight[0], m_presetHeight[1], m_background.height(),
				  m_keymap.rect().height() + m_keyboardTopPading + m_candidateBar.frame().height(), availableSpace.x(), availableSpace.y(), availableSpace.width(), availableSpace.height());
	if (notify)
		m_IMEDataInterface->m_keyboardHeight.set(m_keymap.rect().height() + m_keyboardTopPading + m_candidateBar.frame().height());
}

void PhoneKeyboard::keyboardLayoutChanged()
{
	if (!m_keyboardDirty && m_IMEDataInterface->m_visible.get())
	{
		m_keyboardDirty = true;
		triggerRepaint();
	}
	m_candidateBar.updateKeyboardLayout(m_keymap.layoutName(), m_keymap.getPage(), m_keymap.rect(), m_keymap.isShiftActive(), m_keymap.isCapsLocked(), m_keymap.isAutoCapActive());
}

void PhoneKeyboard::clearExtendedkeys()
{
	if (m_extendedKeys)
	{
		m_extendedKeys = 0;
		m_IMEDataInterface->m_hitRegion.set(QRegion());
		if (m_IMEDataInterface->m_visible.get())
			triggerRepaint();
	}
	else if (!m_IMEDataInterface->m_hitRegion.get().isEmpty())
		m_IMEDataInterface->m_hitRegion.set(QRegion());		// defensive...
	triggerRepaint();
}

void PhoneKeyboard::releaseTouch(int id)
{
	Touch &		touch = m_touches[id];
#if DEBUG_TOUCH
	g_debug("PhoneKeyboard::releaseTouch: '%s', consumed: %d, visible: %d", QString(m_keymap.map(touch.m_keyCoordinate)).toUtf8().data(), touch.m_consumed, touch.m_visible);
#endif
	if (m_candidateBar.endTrace(id))
	{
		triggerRepaint();
	}
	else if (m_extendedKeys)
	{
		UKey key;
		if (!pointToExtendedPopup(touch.m_lastPosition, key))
		{
			key = m_keymap.map(touch.m_keyCoordinate);
			if (key == Qt::Key_Shift || key == cKey_Symbol)
				handleKey(key, touch.m_lastPosition);
			else if (!setExtendedKeys(touch.m_keyCoordinate, true) && !touch.m_consumed)
				clearExtendedkeys();
			else
				triggerRepaint();
		}
		else
		{
			if (key != cKey_None)
			{
				g_debug("Extended character selected: %s", QString(m_keymap.getKeyDisplayString(key, true)).toUtf8().data());
				handleKey(key, QPointF());
			}
			clearExtendedkeys();
		}
	}
	else if (touch.m_inCandidateBar)
	{
		m_candidateBar.releaseTouch(touch.m_lastPosition.x() - touch.m_firstPosition.x());
	}
	else if (m_keymap.isValidLocation(touch.m_keyCoordinate))
	{
		bool	sendKey = touch.m_visible && !touch.m_consumed;
		UKey	key = m_keymap.map(touch.m_keyCoordinate);
		if (key == cKey_Symbol)
			setSymbolKeyDown(false);
		else if (key == Qt::Key_Shift)
			setShiftKeyDown(false);
		else
		{
			touch.m_visible = false;	// the key is no longer considered pressed...
			touch.m_consumed = true;
			if (m_shiftDown || m_symbolDown)
			{	// we are sending the key, which means we are "using" the shift or symbol keypress: when these are released, they are NOT sent out/used again.
				for (std::map<int, Touch>::iterator iter = m_touches.begin(); iter != m_touches.end(); ++iter)
				{
					if (iter->first != id)
					{
						Touch & touch = iter->second;
						UKey key = m_keymap.map(touch.m_keyCoordinate);
						if (key == cKey_Symbol || key == Qt::Key_Shift)
							touch.m_consumed = true;
					}
				}
			}
		}
		if (sendKey)
			handleKey(key, touch.m_lastPosition);
		triggerRepaint();
		if (touch.m_keyCoordinate == m_repeatKey)
			stopRepeat();
	}
}

inline int Min(int a, int b) { return a < b ? a : b; }
inline int MinMax(int min, int v, int max) { return v < min ? min : (v < max ? v : max); }

void PhoneKeyboard::updateTouch(int id, QPointF position)
{
	uint64_t	now = CURRENT_TIME;
	QPointF		touchPosition(position.x(), position.y() - m_keymap.rect().top());
	UKey		extendedKey;
	QPoint		keyCoordinate = (!pointToExtendedPopup(touchPosition, extendedKey) && position.y() > m_keymap.rect().top() - m_keyboardTopPading) ? m_keymap.pointToKeyboard(position.toPoint()) : cOutside;
	bool		newTouch = m_touches.find(id) == m_touches.end();
	Touch &		touch = m_touches[id];
	bool		inCandidatebar = m_candidateBar.frame().contains(position.x(), position.y());
	UKey		newKey = m_keymap.map(keyCoordinate);
	if (newTouch)
	{
		touch.m_firstPosition = touchPosition;
		touch.m_lastPosition = touchPosition;
		touch.m_inCandidateBar = inCandidatebar;
	}
	//g_debug("Touch bar: %d, %gx%g", inCandidatebar, position.x(), position.y());
	if (extendedKey != cKey_None)
	{
		if (newTouch)
			makeSound(extendedKey);
		if (extendedKey != m_extendedKeyShown || (touch.m_visible && touch.m_keyCoordinate != keyCoordinate))
			triggerRepaint();
	}
	else if (!m_extendedKeys && newTouch && m_touches.size() == 1 && QChar(newKey).isLetter() && m_candidateBar.tracePoint(touchPosition.toPoint(), newKey, id, true))
	{
	}
	else if (!m_extendedKeys && !newTouch && m_candidateBar.tracePoint(touchPosition.toPoint(), newKey, id, false))
	{
		stopRepeat();
	}
	else if (newTouch ||
		(!touch.m_inCandidateBar && touch.m_keyCoordinate != keyCoordinate) ||
		(touch.m_inCandidateBar && inCandidatebar))
	{
		triggerRepaint();
		if (touch.m_inCandidateBar)
		{
			m_candidateBar.setScrollOffset(m_candidateBar.scrollOffset() + position.x() - touch.m_lastPosition.x());
			//g_debug("Candidate bar offset: %d", m_candidateBar.scrollOffset());
		}
		else
		{
#if DEBUG_TOUCH
			g_debug("%s key '%s', consumed: %d, visible: %d", newTouch ? "New" : "Moved", m_keymap.getKeyDisplayString(newKey, true).toUtf8().data(), touch.m_consumed, touch.m_visible);
#endif
			if (touch.m_visible && !touch.m_consumed)
			{
				if (keyCoordinate != m_repeatKey)
				{
					if (newTouch && newKey == cKey_Emoticon_Options)
					{
						if (!setExtendedKeys(keyCoordinate, true))
							m_extendedKeys = NULL;
						touch.m_consumed = true;
						stopRepeat();
					}
					else if (newTouch && (canRepeat(newKey) || m_keymap.getExtendedChars(keyCoordinate) || (newKey == cKey_Hide && m_touches.size() == 1)))
					{
						m_timer.start(newKey == cKey_Hide || m_candidateBar.isTraceActive() ? cFirstRepeatLongDelay : cFirstRepeatDelay);
						m_repeatKey = keyCoordinate;
						m_repeatStartTime = CURRENT_TIME;
					}
					else
						stopRepeat();
				}
			}
			if (newTouch)
			{	// send pressed keys not already sent out...
				makeSound(newKey);
				for (std::map<int, Touch>::iterator iter = m_touches.begin(); iter != m_touches.end(); ++iter)
				{
					if (iter->first != id && !touch.m_inCandidateBar)
					{
						Touch & othertouch = iter->second;
						if (othertouch.m_visible)
						{
							UKey key = m_keymap.map(othertouch.m_keyCoordinate);
							if (key != cKey_Symbol && key != Qt::Key_Shift && key != cKey_Hide && !othertouch.m_consumed)
							{
#if DEBUG_TOUCH
								g_debug("Consumming pressed key '%s', consumed: %d, visible: %d", m_keymap.getKeyDisplayString(key, true).toUtf8().data(), touch.m_consumed, touch.m_visible);
#endif
								handleKey(key, othertouch.m_lastPosition);
								othertouch.m_visible = false;
							}
							othertouch.m_consumed = true;
						}
					}
				}
			}
			if (touch.m_visible && ((newKey == cKey_Symbol && !m_extendedKeys && setSymbolKeyDown(true)) || (newKey == Qt::Key_Shift && setShiftKeyDown(true))))
			{
				if (m_extendedKeys)
					touch.m_consumed = true;
			}
		}
	}
	touch.m_keyCoordinate = keyCoordinate;
	if (m_extendedKeys && touch.m_visible != (extendedKey == cKey_None))
	{	// show keyboard key when NOT on the extended bar
		touch.m_visible = !touch.m_visible;
		triggerRepaint();
	}
	touch.m_lastPosition = touchPosition;
	touch.m_lastTouchTime = now;
}

void PhoneKeyboard::handleKey(UKey key, QPointF where)
{
	//g_debug("PhoneKeyboard::handleKey: '%s'", QString(key).toUtf8().data());
	PhoneKeymap::EShiftMode	shiftMode = m_keymap.shiftMode();
	PhoneKeymap::ESymbolMode	symbolMode = m_keymap.symbolMode();
	bool consumeMode = false;
	bool commit = false;
	bool sendKey = true;
	Qt::Key qtkey = Qt::Key_unknown;
	if (UKeyIsUnicodeQtKey(key))
	{
		qtkey = Qt::Key(key);	// "normal" case: UKey is also a valid Qt::Key
		if (m_candidateBar.enabled())
		{
			if (QChar(key).isLetter())
				sendKey = !m_candidateBar.keyboardTap(where, key);
			else
				commit = true;
		}
		else
		{
			sendKey = true;
		}
	}
	else if (UKeyIsTextShortcutKey(key))
	{
		commit = true;
		qtkey = key;
	}
	else if (UKeyIsKeyboardComboKey(key))
	{
		int index = key - cKey_KeyboardComboChoice_First;
		VirtualKeyboardPreferences::instance().selectKeyboardCombo(index);
	}
	else
	{
		switch ((int)key)
		{
		case Qt::Key_Backspace:
			qtkey = Qt::Key_Backspace;
			sendKey = !m_candidateBar.backspace(m_keymap.isShiftDown());
			break;
		case Qt::Key_Return:
			qtkey = key;
			commit = true;
			break;
		case cKey_SymbolPicker:
			qtkey = Qt::Key_Control;
			break;
		case cKey_Symbol:
			if (m_extendedKeys)
				clearExtendedkeys();
			else if (m_keymap.symbolMode() == PhoneKeymap::eSymbolMode_Lock)
				symbolMode = PhoneKeymap::eSymbolMode_Off;
			else
				symbolMode = PhoneKeymap::eSymbolMode_Lock, shiftMode = PhoneKeymap::eShiftMode_Off;
			break;
		case Qt::Key_Shift:
		{
			uint64_t now = CURRENT_TIME;
			if (m_lastUnlockTime + DOUBLE_TAP_DURATION > now)
			{	// quick tap after unlocking: eat that tap, and next tap is like nothing happened before...
				m_lastUnlockTime = 0;
				now = 0;
			}
			else if (m_lastShiftTime + DOUBLE_TAP_DURATION > now)
				shiftMode = PhoneKeymap::eShiftMode_CapsLock;
			else if (shiftMode == PhoneKeymap::eShiftMode_CapsLock)
			{
				shiftMode = PhoneKeymap::eShiftMode_Off;
				m_lastUnlockTime = now;
			}
			else if (shiftMode == PhoneKeymap::eShiftMode_Off)
				shiftMode = PhoneKeymap::eShiftMode_Once;
			else
				shiftMode = PhoneKeymap::eShiftMode_Off;
			m_lastShiftTime = now;
			autoCapChanged(false);
			break;
		}
		case cKey_Hide:
			m_IMEDataInterface->requestHide();
			break;
		case cKey_ToggleSuggestions:
			showSuggestions(!m_candidateBar.enabled());
			break;
		case cKey_ShowXT9Regions:
		{
			PerfMonitor regionMonitor("showXT9Regions");
			m_candidateBar.drawXT9Regions(m_keyboardBackgound, m_keyboardTopPading);
			triggerRepaint();
			break;
		}
		case cKey_ShowKeymapRegions:
			showKeymapRegions();
			triggerRepaint();
			break;
		case cKey_SwitchToQwerty:
			VirtualKeyboardPreferences::instance().selectLayoutCombo("qwerty");
			break;
		case cKey_SwitchToAzerty:
			VirtualKeyboardPreferences::instance().selectLayoutCombo("azerty");
			break;
		case cKey_SwitchToQwertz:
			VirtualKeyboardPreferences::instance().selectLayoutCombo("qwertz");
			break;
		case cKey_StartStopRecording:
		{
			bool	wasRecording = KeyLocationRecorder::instance().isRecording();
			KeyLocationRecorder::instance().startStop(m_keymap.layoutName(), m_keymap.rect());
			if (KeyLocationRecorder::instance().isRecording())
			{
                sendKeyDownUp(Qt::Key_Space, Qt::NoModifier);
                sendKeyDownUp(Qt::Key_Backspace, Qt::NoModifier);
			}
			break;
		}
		case cKey_ToggleLanguage:
			VirtualKeyboardPreferences::instance().selectNextKeyboardCombo();
			break;
		case cKey_CreateDefaultKeyboards:
			VirtualKeyboardPreferences::instance().createDefaultKeyboards();
			break;
		case cKey_ClearDefaultKeyboards:
			VirtualKeyboardPreferences::instance().clearDefaultDeyboards();
			break;
		case cKey_ToggleSoundFeedback:
			VirtualKeyboardPreferences::instance().setTapSounds(!VirtualKeyboardPreferences::instance().getTapSounds());
			break;
		case Qt::Key_Left:
			qtkey = Qt::Key_Left; // used to navigate the cursor left
			break;
		case Qt::Key_Right:
			qtkey = Qt::Key_Right; // used to navigate the cursor right
			break;
		case Qt::Key_Tab:
			switch (m_keymap.tabAction())
			{
			case PhoneKeymap::eTabAction_Next:
				m_IMEDataInterface->performEditorAction(PalmIME::FieldAction_Next);
				break;
			case PhoneKeymap::eTabAction_Previous:
				m_IMEDataInterface->performEditorAction(PalmIME::FieldAction_Previous);
				break;
			case PhoneKeymap::eTabAction_Tab:
			default:
				qtkey = Qt::Key_Tab;
			}
			break;
		default:
			break;
		}
	}
	if (qtkey != Qt::Key_unknown)
	{
		if (commit)
			m_candidateBar.commit();
		consumeMode = true;
		if (sendKey)
		{
			if (KeyLocationRecorder::instance().isRecording())
				KeyLocationRecorder::instance().record(m_keymap.getKeyDisplayString(key, true), where.toPoint());
			if (UKeyIsTextShortcutKey(key))
			{
				m_IMEDataInterface->commitText(m_keymap.getKeyDisplayString(key).toUtf8().data());
				m_shortcutsHandler.resetEditor();
			}
			else if (m_shortcutsHandler.filterKey(qtkey))
			{
				if (UKeyIsFunctionKey(qtkey))
				{
					if (qtkey == Qt::Key_Tab)
						m_IMEDataInterface->commitText("\t");
					else
						sendKeyDownUp(qtkey, m_keymap.isShiftDown() ? Qt::ShiftModifier : Qt::NoModifier);
				}
				else if (qtkey > 0 && qtkey < 128)
					sendKeyDownUp(qtkey, m_keymap.isCapActive() ? Qt::ShiftModifier : Qt::NoModifier);	// send as basic keystroke
				else if (m_keymap.isCapActive())
					sendKeyDownUp((Qt::Key) QChar(qtkey).toUpper().unicode(), Qt::ShiftModifier);
				else
					sendKeyDownUp((Qt::Key) QChar(qtkey).toLower().unicode(), Qt::NoModifier);
			}
		}
		else
			m_shortcutsHandler.resetEditor();
		if (qtkey == Qt::Key_Space || qtkey == Qt::Key_Return)
			symbolMode = PhoneKeymap::eSymbolMode_Off;
	}
	if (consumeMode)
	{
		if (m_keymap.shiftMode() == PhoneKeymap::eShiftMode_Once)
			shiftMode = PhoneKeymap::eShiftMode_Off;
	}
	if (m_keymap.shiftMode() != shiftMode)
		setShiftMode(shiftMode);
	if (m_keymap.symbolMode() != symbolMode)
		setSymbolMode(symbolMode);
}

void PhoneKeyboard::sendKeyDownUp(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    if (m_IMEDataInterface) {
        m_IMEDataInterface->sendKeyEvent(QEvent::KeyPress, key, modifiers);
        m_IMEDataInterface->sendKeyEvent(QEvent::KeyRelease, key, modifiers);
    }
}

inline const char * touchPointState(Qt::TouchPointState state)
{
	switch (state)
	{
	case Qt::TouchPointPressed:		return "pressed";
	case Qt::TouchPointMoved:		return "moved";
	case Qt::TouchPointStationary:	return "stationary";
	case Qt::TouchPointReleased:	return "released";
	default:						return "<unknown>";
	}
}

void PhoneKeyboard::touchEvent(const QTouchEvent& te)
{
	if (m_IMEDataInterface)
	{
		const QList<QTouchEvent::TouchPoint> & touchPoints = te.touchPoints();
#if DEBUG_TOUCH
		std::string	str;
		for (QList<QTouchEvent::TouchPoint>::ConstIterator iter = touchPoints.constBegin(); iter != touchPoints.constEnd(); ++iter)
		{
			const QTouchEvent::TouchPoint & touchPoint = *iter;
			QPoint keyPos = m_keymap.pointToKeyboard(touchPoint.pos().toPoint());
			Qt::Key key = m_keymap.map(keyPos);
			::append_format(str, "   Id: %d, location: %gx%g %s, Key: %dx%d = '%s'.\n", touchPoint.id(), touchPoint.pos().x(), touchPoint.pos().y(), touchPointState(touchPoint.state()), keyPos.x(), keyPos.y(), m_keymap.getKeyDisplayString(key, true).toUtf8().data());
		}
		g_debug("TouchEvent: \n%s", str.c_str());
#endif
		if (m_IMEDataInterface->m_visible.get())
		{
			// handle new presses after handling release & moves
			bool	presses = false;
			for (QList<QTouchEvent::TouchPoint>::ConstIterator iter = touchPoints.constBegin(); iter != touchPoints.constEnd(); ++iter)
			{
				const QTouchEvent::TouchPoint & touchPoint = *iter;
				Qt::TouchPointState state = touchPoint.state();
				if (state == Qt::TouchPointReleased)
				{
					releaseTouch(touchPoint.id());
					m_touches.erase(touchPoint.id());
				}
				else if (state == Qt::TouchPointMoved)
					updateTouch(touchPoint.id(), touchPoint.pos());
				else if (state == Qt::TouchPointPressed)
					presses = true;
			}
			if (presses)
			{
				for (QList<QTouchEvent::TouchPoint>::ConstIterator iter = touchPoints.constBegin(); iter != touchPoints.constEnd(); ++iter)
				{
					const QTouchEvent::TouchPoint & touchPoint = *iter;
					if (touchPoint.state() == Qt::TouchPointPressed)
						updateTouch(touchPoint.id(), touchPoint.pos());
				}
			}
		}
		else
			g_warning("TabletKeyboard::touchEvent: hidden (probably being hidden...), so we will ignore these touches.");
		// everything is released: make sure we have nothing left in our records...
		if (te.type() == QEvent::TouchEnd)
		{
			if (m_touches.size() > 0)
			{
				if (m_IMEDataInterface->m_visible.get())
					g_critical("Clearing %u non-finished touches!", m_touches.size());
				for (QList<QTouchEvent::TouchPoint>::ConstIterator iter = touchPoints.constBegin(); iter != touchPoints.constEnd(); ++iter)
					m_candidateBar.endTrace(iter->id());
				m_touches.clear();
			}
			stopRepeat();
			setShiftKeyDown(false);
			setSymbolKeyDown(false);
		}
	}
}

void PhoneKeyboard::tapEvent(const QPoint& tapPt)
{
#if DEBUG_TOUCH
	g_debug("tapEvent: %d, %d", tapPt.x(), tapPt.y());
#endif
	m_candidateBar.tapEvent(tapPt);
}

void PhoneKeyboard::screenEdgeFlickEvent()
{
	// Mark all touches as consumed
	for (std::map<int, Touch>::iterator iter = m_touches.begin(); iter != m_touches.end(); ++iter) {
		iter->second.m_consumed = true;
	}
}

void PhoneKeyboard::repeatChar()
{
	if (PhoneKeymap::isValidLocation(m_repeatKey))
	{
		UKey key = m_keymap.map(m_repeatKey);
		if (canRepeat(key))
		{
			makeSound(key);
			bool wordDelete = m_keymap.isShiftDown() || (CURRENT_TIME - m_repeatStartTime > cWordDeleteDelay);
			if (key == Qt::Key_Backspace)
				sendKeyDownUp(Qt::Key_Backspace, wordDelete ? Qt::ShiftModifier : Qt::NoModifier);
			else
				sendKeyDownUp(Qt::Key(key), m_keymap.isCapActive() ? Qt::ShiftModifier : Qt::NoModifier);
			int repeatInterval = wordDelete ? cWordDeleteRepeatDelay : cLetterDeleteRepeatDelay;
			if (m_timer.interval() != repeatInterval)
				m_timer.setInterval(repeatInterval);
		}
		else
		{
			if (setExtendedKeys(m_repeatKey))
			{
				for (std::map<int, Touch>::iterator iter = m_touches.begin(); iter != m_touches.end(); ++iter)
				{
					Touch & touch = iter->second;
					if (touch.m_keyCoordinate == m_repeatKey)
						touch.m_consumed = true;
				}
			}
			stopRepeat();
		}
	}
	else
		stopRepeat();
}

bool PhoneKeyboard::setExtendedKeys(QPoint keyCoord, bool cancelIfSame)
{
	const UKey * newExtended = m_keymap.getExtendedChars(keyCoord);
	if (cancelIfSame && newExtended == m_extendedKeys)
		return false;
	m_extendedKeys = newExtended;
	if (m_extendedKeys)
	{
		int cellCount, lineCount, lineLength;
		getExtendedPopupSpec(cellCount, lineCount, lineLength);
		IMEPixmap & popup = (lineCount > 1) ? m_popup_2 : m_popup;
		m_extendedKeyShown = cKey_None;
		m_keymap.keyboardToKeyZone(keyCoord, m_extendedKeysFrame);
		m_extendedKeysPointer = m_extendedKeysFrame.left() + m_extendedKeysFrame.width() / 2;
		m_extendedKeysFrame.translate(0, -popup.height() + 10);
		int width = cPopupLeftSide + cPopupRightSide + lineLength * m_popup_key.width();
		m_extendedKeysFrame.setLeft(m_extendedKeysPointer - m_popup_key.width() / 2 - cPopupLeftSide);
		m_extendedKeysFrame.setWidth(width);
		m_extendedKeysFrame.setHeight(popup.height());
		if (m_extendedKeysFrame.left() < 0)
			m_extendedKeysFrame.moveLeft(0);
		else if (m_extendedKeysFrame.right() > m_keymap.rect().right())
			m_extendedKeysFrame.translate(m_keymap.rect().right() - m_extendedKeysFrame.right(), 0);
		if (m_extendedKeysFrame.isValid())
		{
			m_IMEDataInterface->m_hitRegion.set(QRegion(m_IMEDataInterface->m_availableSpace.get()));
			triggerRepaint();
		}
		return true;
	}
	else
		m_IMEDataInterface->m_hitRegion.set(QRegion());
	return false;
}

bool PhoneKeyboard::pointToExtendedPopup(QPointF position, UKey & outKey)
{
	outKey = cKey_None;
	if (m_extendedKeys && m_extendedKeysFrame.contains(position.x(), position.y() + m_keymap.rect().top()))
	{
		QPoint	where = position.toPoint() - m_extendedKeysFrame.topLeft() - QPoint(cPopupLeftSide, -m_keymap.rect().top() + cPopupTopToKey);
		int cellCount, lineCount, lineLength;
		getExtendedPopupSpec(cellCount, lineCount, lineLength);
		int x = qMin<int>(where.x() / m_popup_key.width(), lineLength - 1);
		int y = where.y() / (m_popup_key.height() / 2);
		int index = (y == 0) ? x : x + lineLength;
		if (index <= cellCount)
			outKey = m_extendedKeys[index];
		return true;
	}
	return false;
}

void PhoneKeyboard::getExtendedPopupSpec(int & outCellCount, int & outLineCount, int & outLineLength)
{
	outCellCount = 0;
	if (m_extendedKeys)
		while (m_extendedKeys[outCellCount] != cKey_None)
			++outCellCount;
	outLineCount = (outCellCount > cPopupSingleLineMax) ? 2 : 1;
	outLineLength = (outCellCount + outLineCount - 1) / outLineCount;
}

bool PhoneKeyboard::canRepeat(UKey key) const
{
	return (key == Qt::Key_Space || key == Qt::Key_Backspace || key == Qt::Key_Left || key == Qt::Key_Right);
}

void PhoneKeyboard::stopRepeat()
{
	m_timer.stop();
	m_repeatKey = cOutside;
	m_repeatStartTime = 0;
}

void PhoneKeyboard::showKeymapRegions()
{
	PerfMonitor regionMonitor("showKeymapRegions");
	m_keyboardBackgound->fill(QColor(0, 0, 0));
	QRect		frame = m_keymap.rect();
	QPainter	painter(m_keyboardBackgound);
	int			y_offset = frame.top() - m_keyboardTopPading;
	ColorMap	colorMap;
	for (int x = 0; x < frame.width(); ++x)
		for (int y = 0; y < m_keyboardTopPading + frame.height(); ++y)
		{
			QPoint	keycoord = m_keymap.pointToKeyboard(QPoint(x, y_offset + y));
			if (keycoord != cOutside)
			{
				painter.setPen(colorMap[QChar(m_keymap.map(keycoord)).unicode()]);	// create or reuse random color for this character
				painter.drawPoint(x, y);
			}
		}
	m_keyboardDirty = true;
}

void PhoneKeyboard::paint(QPainter & painter)
{
	PerfMonitor perf("PhoneKeyboard::paint");
	m_candidateBar.paint(painter, cBlueColor);
	const QRect & keymapRect = m_keymap.rect();
	QRect	keyboardFrame(keymapRect.left(), keymapRect.top() - m_keyboardTopPading, keymapRect.width(), keymapRect.height() + m_keyboardTopPading);
	if (updateBackground())
		perf.trace("background rebuilt");
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.drawPixmap(QPointF(keyboardFrame.left(), keyboardFrame.top()), *m_keyboardBackgound);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	perf.trace("Draw background");
	DoubleDrawRenderer				doubleDrawRenderer;
	CachedGlyphRenderer<GlyphSpec>	renderer(painter, m_glyphCache, doubleDrawRenderer, PhoneKeymap::cKeymapColumns * (PhoneKeymap::cKeymapRows + 1));
	for (int y = 0; y < PhoneKeymap::cKeymapRows; ++y)
	{
		for (int x = 0; x < PhoneKeymap::cKeymapColumns; ++x)
		{
			QPoint	keyCoord(x, y);
			UKey plainKey = m_keymap.map(x, y, PhoneKeymap::eLayoutPage_plain);
			QRect r;
			int count = m_keymap.keyboardToKeyZone(keyCoord, r);
			if (count > 0 && plainKey != cKey_None)
			{
				UKey key = m_keymap.map(x, y);
//				if (key == Qt::Key_Shift)
//					drawKeyBackground(painter, r, keyCoord, key, false, count);
				drawKeyCap(&painter, renderer, r, keyCoord, key, eUse_unpressed);
			}
		}
	}
	bool extendedKeysShown = m_extendedKeys && m_extendedKeysFrame.isValid();
	QRect	r;
	if (extendedKeysShown)
	{
		for (int y = 0; y < PhoneKeymap::cKeymapRows; ++y)	// draw caps second (faster to split)
		{
			for (int x = 0; x < PhoneKeymap::cKeymapColumns; ++x)
			{
				if (m_keymap.getExtendedChars(QPoint(x, y)) && m_keymap.keyboardToKeyZone(QPoint(x, y), r) > 0)
				{
					r.setWidth(r.width() - 9 + m_9tileCorner.m_trimH); r.setHeight(r.height() - 9 + m_9tileCorner.m_trimV);
					renderer.render(r, GlyphSpec(sElipsis, cElipsisFontSize, false, cActiveColor, cActiveColor_back), sFont, Qt::AlignRight | Qt::AlignBottom);
				}
			}
		}
	}
	renderer.flush();
	perf.trace("Draw labels");
	UKey	extendedKey = cKey_None;
	for (std::map<int, Touch>::iterator iter = m_touches.begin(); iter != m_touches.end(); ++iter)
	{
		Touch & touch = iter->second;
		if (!pointToExtendedPopup(touch.m_lastPosition, extendedKey))
		{
			if (touch.m_visible)
			{
				int count = m_keymap.keyboardToKeyZone(touch.m_keyCoordinate, r);
				if (count > 0)
				{
					UKey key = m_keymap.map(touch.m_keyCoordinate);
					if (key != cKey_None)
					{
						painter.setClipRect(r);
						painter.drawPixmap(r.left(), keyboardFrame.top(), r.width(), keyboardFrame.height(), m_background.pixmap());
						painter.setClipping(false);
						drawKeyBackground(painter, r, touch.m_keyCoordinate, key, true, count);
						drawKeyCap(&painter, renderer, r, touch.m_keyCoordinate, key, eUse_pressed);
						if (extendedKeysShown && m_keymap.getExtendedChars(touch.m_keyCoordinate))
						{
							QRect	elipsisRect(r.left() + cPressedTranslateH, r.top() + cPressedTranslateV, r.width() - 9 + m_9tileCorner.m_trimH, r.height() - 9 + m_9tileCorner.m_trimV);
							renderer.render(elipsisRect, GlyphSpec(sElipsis, cElipsisFontSize, false, cActiveColor, cActiveColor_back), sFont, Qt::AlignRight | Qt::AlignBottom);
						}
						if (!m_extendedKeys && m_showPopupKeys && key != Qt::Key_Shift && key != cKey_Symbol && key != Qt::Key_Space && key != Qt::Key_Return && key != Qt::Key_Backspace)
						{
							QPoint	topLeft((r.left() + r.right() - m_popup.width()) / 2, r.top() - m_popup.height());
							painter.drawPixmap(topLeft, m_popup);
							QRect	destRect(topLeft + QPoint((m_popup.width() - m_popup_key.width()) / 2, cPopupTopToKey), QSize(m_popup_key.width(), m_popup_key.height() / 2));
							painter.drawPixmap(destRect.topLeft(), m_popup_key, QRect(0, m_popup_key.height() / 2, destRect.width(), destRect.height()));
							drawKeyCap(&painter, renderer, destRect, touch.m_keyCoordinate, key, eUse_preview);
						}
						//g_debug("'%s' drawn pressed, consumed: %d", QString(key).toUtf8().data(), touch.m_consumed);
					}
				}
			}
		}
	}
	renderer.flush();
	m_candidateBar.paintTrace(painter, keyboardFrame.top() + m_keyboardTopPading, cBlueColor, 4);
	if (extendedKeysShown)
	{
		renderer.flush();
		painter.setFont(sFont);
		int cellCount, lineCount, lineLength;
		getExtendedPopupSpec(cellCount, lineCount, lineLength);
		IMEPixmap & popup = (lineCount > 1) ? m_popup_2 : m_popup;
		QRect	r(m_extendedKeysFrame);
		int left = r.left() + cPopupSide;
		int right = r.right() - cPopupSide + 1;
		painter.drawPixmap(r.left(), r.top(), popup.pixmap(), 0, 0, cPopupSide, popup.height());
		painter.drawPixmap(right, r.top(), popup.pixmap(), popup.width() - cPopupSide, 0, cPopupSide, popup.height());

		int pointerLeft = m_extendedKeysPointer - cPopupPointerWidth / 2;
		int pointerRight = pointerLeft + cPopupPointerWidth;
		if (left < pointerLeft)
			painter.drawPixmap(left, r.top(), pointerLeft - left, popup.height(), popup.pixmap(), cPopupSide, 0, 1, popup.height());
		if (pointerRight < right)
			painter.drawPixmap(pointerRight, r.top(), right - pointerRight, popup.height(), popup.pixmap(), cPopupSide, 0, 1, popup.height());
		painter.drawPixmap(pointerLeft, r.top(), popup.pixmap(), cPopupPointerStart, 0, cPopupPointerWidth, popup.height());
		r.translate(cPopupLeftSide, cPopupTopToKey);
		UKey key;
		for (int k = 0; (key = m_extendedKeys[k]) != cKey_None; ++k)
		{
			if (k < lineLength)
				painter.drawPixmap(r.left() + k * m_popup_key.width(), r.top(), m_popup_key.pixmap(),
								   0, (extendedKey == key) ? m_popup_key.height() / 2 : 0, m_popup_key.width(), m_popup_key.height() / 2);
			else
				painter.drawPixmap(r.left() + (k - lineLength) * m_popup_key.width(), r.top() + m_popup_2.height() - m_popup.height(), m_popup_key.pixmap(),
								   0, (extendedKey == key) ? m_popup_key.height() / 2 : 0, m_popup_key.width(), m_popup_key.height() / 2);
		}
		r.setWidth(m_popup_key.width() - 3);
		r.setHeight(m_popup_key.height() / 2 - 2);
		QRect	cell(r);
		for (int k = 0; (key = m_extendedKeys[k]) != cKey_None; ++k)
		{
			if (k < lineLength)
				cell.moveTopLeft(QPoint(r.left() + k * m_popup_key.width(), r.top()));
			else
				cell.moveTopLeft(QPoint(r.left() + (k - lineLength) * m_popup_key.width(), r.top() + m_popup_2.height() - m_popup.height()));
			QPixmap	*	pix = (UKeyIsEmoticonKey(key) && m_keymap.showEmoticonsAsGraphics()) ? getPixmapForKey(key) : NULL;
			if (pix)
			{
				drawCenteredPixmap(painter, *pix, cell);
			}
			else
			{
				QString	text = m_keymap.getKeyDisplayString(key);
				int fontSize = (text.length() < 6) ? cPopupFontSize : cPopupFontSize - 8;
				if (sFont.pixelSize() != fontSize)
				{
					sFont.setPixelSize(fontSize);
					painter.setFont(sFont);
				}
				renderer.render(cell, GlyphSpec(text, fontSize, false, cPopoutTextColor, cPopoutTextColor_back), sFont);
			}
		}
	}
	m_extendedKeyShown = extendedKey;
#if VKB_SHOW_GLYPH_CACHE
	painter.setPen(QColor(255, 0, 0)); painter.drawRect(QRect(QPoint(0, 0), m_glyphCache.pixmap().size())); painter.drawPixmap(0, 0, m_glyphCache.pixmap());
#endif
#if VKB_FORCE_FPS
	triggerRepaint();
#endif
	if (renderer.getCacheMissCount() > 0 && m_keymap.getCachedGlyphsCount() < 3)
		queueIdlePrerendering();
}

bool PhoneKeyboard::updateBackground()
{
	if (!m_keyboardBackgound || m_keyboardDirty)
	{
		QRect	keymapFrame(m_keymap.rect());
		int width = keymapFrame.width();
		int usedHeight = keymapFrame.height() + m_keyboardTopPading;
		QRect	keyboardFrame(keymapFrame.left(), keymapFrame.top() - m_keyboardTopPading, width, usedHeight);
		if (!m_keyboardBackgound || m_keyboardBackgound->width() != width || m_keyboardBackgound->height() != usedHeight)
		{
			PixmapCache::instance().dispose(m_keyboardBackgound);
			m_keyboardBackgound = PixmapCache::instance().get(width, usedHeight);
		}
		if (m_keymap.updateLimits() != m_keyboardLimitsVersion)
		{
			//g_critical("Rebuilding BACKGROUND");
			m_keyboardLimitsVersion = m_keymap.updateLimits();
			QPainter	offscreenPainter(m_keyboardBackgound);
			//m_keyboardBackgound->fill(QColor(255, 0, 0));
			offscreenPainter.drawPixmap(QRect(0, 0, width, usedHeight), m_background.pixmap());
			offscreenPainter.translate(0, -keyboardFrame.top());
			offscreenPainter.setRenderHints(cRenderHints, true);
			m_nineTileSprites.reserve(true);
			for (int y = 0; y < PhoneKeymap::cKeymapRows; ++y)
			{
				for (int x = 0; x < PhoneKeymap::cKeymapColumns; ++x)
				{
					QPoint	keyCoord(x, y);
					UKey key = m_keymap.map(x, y);
					QRect r;
					int count = m_keymap.keyboardToKeyZone(keyCoord, r);
					if (count > 0)
					{
						QSize size = r.size();
						if (key == Qt::Key_Shift)
						{
							m_nineTileSprites.reserve(size, count, m_shift_on_key);
							m_nineTileSprites.reserve(size, count, m_shift_lock_key);
							m_nineTileSprites.reserve(size, count, m_black_key);
						}
						else
							m_nineTileSprites.reserve(size, count, getKeyBackground(keyCoord, key));
					}
				}
			}
			m_nineTileSprites.reserve(false);
			for (int y = 0; y < PhoneKeymap::cKeymapRows; ++y)
			{
				for (int x = 0; x < PhoneKeymap::cKeymapColumns; ++x)
				{
					UKey plainKey = m_keymap.map(x, y, PhoneKeymap::eLayoutPage_plain);
					//if (plainKey != Qt::Key_Shift)
					{
						QPoint	keyCoord(x, y);
						QRect r;
						int count = m_keymap.keyboardToKeyZone(keyCoord, r);
						if (count > 0)
							drawKeyBackground(offscreenPainter, r, keyCoord, plainKey, false, count);
					}
				}
			}
		}
		m_keyboardDirty = false;
		return true;
	}
	return false;
}

QPixmap & PhoneKeyboard::getKeyBackground(const QPoint & keyCoord, UKey key)
{
	/*
	if (key == Qt::Key_Shift)
	{
		switch (m_keymap.shiftMode())
		{
		case PhoneKeymap::eShiftMode_CapsLock: return m_shift_lock_key;
		case PhoneKeymap::eShiftMode_Once: return m_shift_on_key;
		default:
			return m_black_key;
		}
	}
	else
	*/
		return selectFromKeyType<QPixmap &>(m_keymap.map(keyCoord, PhoneKeymap::eLayoutPage_plain), m_white_key, m_black_key, m_gray_key);
}

QPixmap * PhoneKeyboard::getPixmapForKey(UKey key)
{
	switch ((int)key)
	{
	case Qt::Key_Shift:
		switch (m_keymap.shiftMode())
		{
		case PhoneKeymap::eShiftMode_Once:		return &m_shift_on.pixmap();	break;
		case PhoneKeymap::eShiftMode_CapsLock:	return &m_shift_lock.pixmap();	break;
		case PhoneKeymap::eShiftMode_Off:
		default:
		 return m_keymap.isAutoCapActive() ?  &m_shift_on.pixmap() : &m_shift.pixmap();
		}
		break;
	case Qt::Key_Backspace:		return &m_backspace.pixmap();		break;
	case cKey_Hide:				return &m_hide.pixmap();			break;
	case cKey_Emoticon_Frown:	return &m_emoticon_frown.pixmap();	break;
	case cKey_Emoticon_Cry:		return &m_emoticon_cry.pixmap();	break;
	case cKey_Emoticon_Options:
	case cKey_Emoticon_Smile:	return &m_emoticon_smile.pixmap();	break;
	case cKey_Emoticon_Wink:	return &m_emoticon_wink.pixmap();	break;
	case cKey_Emoticon_Yuck:	return &m_emoticon_yuck.pixmap();	break;
	case cKey_Emoticon_Gasp:	return &m_emoticon_gasp.pixmap();	break;
	case cKey_Emoticon_Heart:	return &m_emoticon_heart.pixmap();	break;
	default: /* NOP */;
	}
	return NULL;
}

void PhoneKeyboard::drawCenteredPixmap(QPainter & painter, QPixmap & pixmap, const QRect & location)
{
	if (pixmap.height() > location.height() || pixmap.width() > location.width())
	{
		//g_debug("TabletKeyboard::drawKeyCap shrinking \"%s\" by %d pixels", m_keymap.getKeyDisplayString(key, true).toUtf8().data(), location.height() - pixmap.height());
		painter.setRenderHints(cRenderHints, true);
		if (pixmap.height() * location.width() > location.height() * pixmap.width())
		{
			int targetWidth = location.height() * pixmap.width() / pixmap.height();
			painter.drawPixmap(location.left() + (location.width() - targetWidth) / 2, location.top(), targetWidth, location.height(), pixmap);
		}
		else
		{
			int targetHeight = location.width() * pixmap.height() / pixmap.width();
			painter.drawPixmap(location.left(), location.top() + (location.height() - targetHeight) / 2, location.width(), targetHeight, pixmap);
		}
	}
	else
		painter.drawPixmap((int) location.left() + (location.width() - pixmap.width()) / 2, (int) location.top() + (location.height() - pixmap.height()) / 2, pixmap);
}

inline bool boostSize(QChar c)
{
	ushort ci = c.unicode();
	return ci == '.' || ci == ',' || ci == ';' || ci == ':' || ci == '\'' || ci == '"';
}

inline bool boostSize(QString s)
{
	return s.size() == 1 && boostSize(s[0]);
}

//inline int font_size(const QString & text, const QColor & color, int baseSize, int percent)
//{
//	if (color != cActiveColor)
//		return baseSize * percent / 100;
//	return boostSize(text) ? baseSize + 2 : baseSize;
//}

#define font_size(text, color, baseSize, percent) ((color != activeColor) ? (baseSize * percent / 100) : (boostSize(text) ? baseSize + 2 : baseSize))

void PhoneKeyboard::drawKeyCap(QPainter * painter, GlyphRenderer<GlyphSpec> & renderer, QRect location, const QPoint & keyCoord, UKey key, EUse use)
{
	location.setBottom(location.bottom() - 4);
//	if (pressed)
//		location.translate(cPressedTranslateH, cPressedTranslateV);
	QString		text, altText;
	bool		twoHorizontal = false;
	bool		twoVertical = false;
	bool		useTwo = false;
	QColor		activeColor = useWhite(use) ? cActiveColor : cPopoutTextColor;
	QColor		activeColor_back = useWhite(use) ? cActiveColor_back : cPopoutTextColor_back;
	QColor		mainCharColor = activeColor;
	QColor		mainCharColor_back = activeColor_back;
	QColor		altCharColor = cDisabledColor;
	QColor		altCharColor_back = cDisabledColor_back;
	bool		capitalize = m_keymap.isCapOrAutoCapActive();
//	bool		capitalize = key >= Qt::Key_A && key <= Qt::Key_Z;
	if (key == Qt::Key_Space)
		text = m_candidateBar.autoSelectCandidate();
	else if (UKeyIsUnicodeQtKey(key))
	{	// key is also a unicode character...
		UKey plain = m_keymap.map(keyCoord, PhoneKeymap::eLayoutPage_plain);
		UKey alt = m_keymap.map(keyCoord, PhoneKeymap::eLayoutPage_Alternate);
		if (plain != alt && alt != cKey_None)
		{
			useTwo = twoVertical = KeyCap_TwoVertical(keyCoord, plain);
			if (twoVertical)
			{
				if (key == plain)
				{
					text = capitalize ? QChar(plain) : QChar(plain).toLower();
					altText = QChar(alt).toLower();
				}
				else
				{
					mainCharColor = cDisabledColor;
					mainCharColor_back = cDisabledColor_back;
					altCharColor = activeColor;
					altCharColor_back = activeColor_back;
					text = QChar(plain).toLower();
					altText = capitalize ? QChar(alt) : QChar(alt).toLower();
				}
			}
			else
				text = capitalize ? QChar(key) : QChar(key).toLower();
		}
		else
			text = capitalize ? QChar(key) : QChar(key).toLower();
	}
	else if (((UKeyIsEmoticonKey(key) && m_keymap.showEmoticonsAsGraphics()) || (text = m_keymap.getKeyDisplayString(key)).size() == 0))
	{
		if (painter)
		{
			QPixmap	*	pix = getPixmapForKey(key);
			if (pix)
			{
				int cPixMargin = UKeyIsEmoticonKey(key) ? 8 : 2;
				location.adjust(cPixMargin, cPixMargin, - cPixMargin, - cPixMargin);
				drawCenteredPixmap(*painter, *pix, location);
			}
		}
	}
	if (text.size() > 0)
	{
		sFont.setBold(useExtraLarge(use));
		bool forceAlignHCenter = false;		// if too tight, center text for better looking results
		int height = location.height();
		int fontSize = useExtraLarge(use) ? 32 : 24;
		int centerOffset = 1;
		if (useTwo && use == eUse_preview)
			twoHorizontal = true, centerOffset = 2;
		if (height / 2 < fontSize)
			fontSize = (height + 1) / 2 + (useExtraLarge(use) ? 4 : 0);
		if (text.size() > 1)
		{
			if (!useExtraLarge(use))
				sFont.setBold(UKeyIsFunctionKey(key) && !UKeyIsTextShortcutKey(key));
			fontSize = qMin<int>(fontSize, 22);
			sFont.setPixelSize(fontSize);
			int gap;
			while ((gap = QFontMetrics(sFont).width(text) + 16 - location.width()) > 0) {
				forceAlignHCenter = true;
				int reduction = gap / text.length();
				if (reduction < 1)
					reduction = 1;
				//g_debug("font size %d, Width = %d, gap = %d, reduction = %d", fontSize, location.width(), gap, reduction);
				fontSize -= reduction;
				sFont.setPixelSize(fontSize);
			};
			if (gap > -8)
				forceAlignHCenter = true;
			//g_debug("Using font size %d, Width = %d, text width = %d", fontSize, location.width(), QFontMetrics(sFont).width(text));
		}
		if (twoHorizontal)
		{
			if (mainCharColor == activeColor)
				location.adjust(4, 1, -5, 1);
			else
				location.adjust(5, 1, -4, 1);
			if (inLandscapeOrientation() == false)
				fontSize -= 1;
			QRect	rect(location.left() + location.width() / 2 - centerOffset, location.top(), location.width() / 2, location.height());
			renderer.render(rect, GlyphSpec(text, font_size(text, mainCharColor, fontSize, 75), sFont.bold(), mainCharColor, mainCharColor_back), sFont);
			rect.moveLeft(location.left() + centerOffset);
			renderer.render(rect, GlyphSpec(altText, font_size(altText, altCharColor, fontSize, 75), sFont.bold(), altCharColor, altCharColor_back), sFont);
		}
		else if (twoVertical)
		{
			int boxheight = location.height() / 3;
			QRect	rect(location.left(), location.bottom() - boxheight - 10 + (boostSize(text) ? -2 : 0), location.width(), boxheight);
			renderer.render(rect, GlyphSpec(text, font_size(text, mainCharColor, fontSize, 75), sFont.bold(), mainCharColor, mainCharColor_back), sFont);
			rect.moveTop(location.top() + 10);
			renderer.render(rect, GlyphSpec(altText, font_size(altText, altCharColor, fontSize, 75), sFont.bold(), altCharColor, altCharColor_back), sFont);
		}
		else
		{
/*
			if (key == Qt::Key_Return)
			{ // Smaller, bottom right corner...
				location.setHeight((location.height()) * 90 / 100);
				if (forceAlignHCenter)
					renderer.render(location, GlyphSpec(text, qMin<int>(height, fontSize - 2), sFont.bold(), cFunctionColor, cFunctionColor_back), sFont, Qt::AlignBottom | Qt::AlignHCenter);
				else
				{
					location.setWidth(location.width() * 85 / 100 + m_9tileCorner.m_trimH);
					renderer.render(location, GlyphSpec(text, qMin<int>(height, fontSize - 2), sFont.bold(), cFunctionColor, cFunctionColor_back), sFont, Qt::AlignBottom | Qt::AlignRight);
				}
			}
			else
*/
			{
				int size = qMin<int>(height, fontSize);
				if (key == cKey_ToggleLanguage && text.endsWith('-'))	// special case: strike the language key if the language ends with '-'
				{
					QString		realText = text.left(text.size() - 1);
					renderer.render(location, GlyphSpec(realText, size, sFont.bold(), mainCharColor, cFunctionColor_back), sFont);
					if (painter)
					{
						renderer.flush();
						painter->setPen(QPen(QBrush(cActiveColor), 4, Qt::SolidLine, Qt::RoundCap));
						int width = QFontMetrics(sFont).width(realText) / 2 + 4;
						int cx = (location.left() + location.right()) / 2;
						int cy = (location.top() + location.bottom()) / 2;
						painter->drawLine(cx - width, cy, cx + width, cy);
					}
				}
				else if (key == Qt::Key_Space)
					renderer.renderNow(location, GlyphSpec(text, size, sFont.bold(), mainCharColor, cFunctionColor_back), sFont);
				else
					renderer.render(location, GlyphSpec(text, size, sFont.bold(), mainCharColor, cFunctionColor_back), sFont);
			}
		}
		sFont.setBold(false);
	}
}

bool PhoneKeyboard::setShiftKeyDown(bool shiftKeyDown)
{
	if (m_keymap.setShiftKeyDown(shiftKeyDown))
	{
//		if (m_IMEDataInterface)
//			m_IMEDataInterface->sendKeyEvent(shiftKeyDown ? QEvent::KeyPress : QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier);
		keyboardLayoutChanged();
		return true;
	}
	return false;
}

bool PhoneKeyboard::setSymbolKeyDown(bool symbolKeyDown)
{
	if (m_keymap.setSymbolKeyDown(symbolKeyDown))
	{
		keyboardLayoutChanged();
		return true;
	}
	return false;
}

void PhoneKeyboard::makeSound(UKey key)
{
	if (VirtualKeyboardPreferences::instance().getTapSounds() && key != cKey_None)
		m_IMEDataInterface->keyDownAudioFeedback(key);
}

void PhoneKeyboard::queueIdlePrerendering()
{
	if (!m_idleInit)
	{
		m_idleInit = true;
		g_idle_add_full(G_PRIORITY_LOW, keyboard_idle, NULL, NULL);
	}
}

bool PhoneKeyboard::idle()
{	// there is only ever one PhoneKeyboard. Using statics to avoid exposing everywhere variables only used here
	static int	sCount = 0;
	static bool	sInitExtendedGlyphs = true;

	if (m_IMEDataInterface->isUIAnimationActive())
		return true;

	if (sCount < IMEPixmap::count())
	{
		IMEPixmap::load(sCount++);
		return true;
	}

	int index = sCount++ - IMEPixmap::count();
	int stateIndex = index / PhoneKeymap::cKeymapRows;
	int y = index % PhoneKeymap::cKeymapRows;

	if (stateIndex < 3)
	{	// pre-rendering all keyboards states for the current size, one row at a time...
		DoubleDrawRenderer				renderer;
		GlyphCachePopulator<GlyphSpec>	populator(m_glyphCache, renderer);

		bool						shiftDown = m_keymap.isShiftDown();
		bool						symbolDown = m_keymap.isSymbolDown();
		bool						autoCapActive = m_keymap.isAutoCapActive();
		PhoneKeymap::EShiftMode	shiftMode = m_keymap.shiftMode();
		PhoneKeymap::ESymbolMode	symbolMode = m_keymap.symbolMode();

		m_keymap.setShiftKeyDown(false);
		m_keymap.setSymbolKeyDown(false);
		m_keymap.setAutoCap(false);

		if (stateIndex == 0)
		{
			m_keymap.setShiftMode(PhoneKeymap::eShiftMode_Off);
			m_keymap.setSymbolMode(PhoneKeymap::eSymbolMode_Off);
		}
		else if (stateIndex == 1)
		{
			m_keymap.setShiftMode(PhoneKeymap::eShiftMode_Once);
			m_keymap.setSymbolMode(PhoneKeymap::eSymbolMode_Off);
		}
		else
		{
			m_keymap.setShiftMode(PhoneKeymap::eShiftMode_CapsLock);
			m_keymap.setSymbolMode(PhoneKeymap::eSymbolMode_Lock);
		}

		std::string msg = string_printf("PhoneKeyboard pre-render (%dx%d): shift %d, symbol %d, autoCap %d, index=%d, y=%d", m_keymap.rect().width(), m_keymap.rect().height(), m_keymap.isShiftActive(), m_keymap.isSymbolActive(), m_keymap.isCapOrAutoCapActive(), stateIndex, y);
		PerfMonitor perf(msg.c_str());
		//g_debug("%s", msg.c_str());
		for (int x = 0; x < PhoneKeymap::cKeymapColumns; ++x)
		{
			QPoint	keyCoord(x, y);
			UKey key = m_keymap.map(x, y);
			QRect r;
			int count = m_keymap.keyboardToKeyZone(keyCoord, r);
			r.moveTo(0, 0);
			if (count > 0 && key != Qt::Key_Space && key != cKey_None)
			{
				drawKeyCap(NULL, populator, r, keyCoord, key, eUse_unpressed);
				drawKeyCap(NULL, populator, r, keyCoord, key, eUse_pressed);
				drawKeyCap(NULL, populator, QRect(QPoint(), m_popup_key.size()), keyCoord, key, eUse_preview);
				drawKeyCap(NULL, populator, QRect(QPoint(), m_popup_key.size()), keyCoord, key, eUse_extended);
			}
		}

		m_keymap.setShiftKeyDown(shiftDown);
		m_keymap.setSymbolKeyDown(symbolDown);
		m_keymap.setAutoCap(autoCapActive);
		m_keymap.setShiftMode(shiftMode);
		m_keymap.setSymbolMode(symbolMode);

		return true;
	}
	else if (stateIndex == 0 && y == 0)
	{	// pre-rendering background, with 9-tiled keys
		updateBackground();
		return true;
	}
	else if (sInitExtendedGlyphs)
	{	// pre-render extended chars, but only once per run (they are always shown at the same size & same color)
		DoubleDrawRenderer				renderer;
		GlyphCachePopulator<GlyphSpec>	populator(m_glyphCache, renderer);

		static int x = -1;
		static int y = -1;
		static const UKey * extendedChars = NULL;
		static int extendedIndex = 0;
		if (x < 0 || y < 0 || x >= PhoneKeymap::cKeymapColumns || y >= PhoneKeymap::cKeymapRows)
		{
			x = y = 0;
			extendedChars = m_keymap.getExtendedChars(QPoint(x, y));
			extendedIndex = 0;
		}
		uint64_t timeLimit = CURRENT_TIME + 10;	// process 10ms max
		do {
			if (extendedChars)
			{
				//g_debug("pre-render %dx%d %s...", x, y, QString(QChar(extendedChars[extendedIndex])).toUtf8().data());
				if (UKeyIsUnicodeQtKey(extendedChars[extendedIndex]))
				{
					populator.render(QRect(QPoint(), m_popup_key.size()), GlyphSpec(QString(QChar(extendedChars[extendedIndex]).toLower()), cPopupFontSize, false, cPopoutTextColor, cPopoutTextColor_back), sFont);
					populator.render(QRect(QPoint(), m_popup_key.size()), GlyphSpec(QString(QChar(extendedChars[extendedIndex]).toUpper()), cPopupFontSize, false, cPopoutTextColor, cPopoutTextColor_back), sFont);
				}
				if (!extendedChars[++extendedIndex])
					extendedChars = NULL;
			}
			if (!extendedChars)
			{
				if (++x >= PhoneKeymap::cKeymapColumns)
				{
					x = 0;
					if (++y >= PhoneKeymap::cKeymapRows)
						break;
				}
				extendedChars = m_keymap.getExtendedChars(QPoint(x, y));
				extendedIndex = 0;
			}
			if (CURRENT_TIME > timeLimit)
				return true;
		} while (true);
		sInitExtendedGlyphs = false;

		// cache elipsis...
		populator.render(QRect(0, 0, 20, 20), GlyphSpec(sElipsis, cElipsisFontSize, false, cActiveColor, cActiveColor_back), sFont);
	}

	m_keymap.incCachedGlyphs();
	sCount = IMEPixmap::count();
	m_idleInit = false;
	//g_debug("PhoneKeyboard background init complete!");

#if 0
#if defined(TARGET_DEVICE)
	m_glyphCache.pixmap().toImage().save("/media/internal/glyphcache.png");
#else
	m_glyphCache.pixmap().toImage().save(QString(getenv("HOME")) + "/Desktop/glyphcache.png");
#endif
#endif

	return false;
}

}; // namespace Phone_Keyboard
