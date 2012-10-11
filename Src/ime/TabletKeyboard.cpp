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


#include "TabletKeyboard.h"

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

namespace Tablet_Keyboard {
/**
 * temporary XML filename
 */
#define IME_KDB_XML_FILENAME      "/tmp/kdb.xml"


#define CURRENT_TIME SingletonTimer::currentTime()

//	#define DOUBLE_TAP_DURATION Settings::LunaSettings()->tapDoubleClickDuration
#define DOUBLE_TAP_DURATION 500

#define DEBUG_TOUCH 0

inline bool KeyCap_TwoHorizontal(const QPoint & keyCoord, UKey key)
{
	return keyCoord.y() == 0 && !UKeyIsFunctionKey(key);
}

inline bool KeyCap_TwoVertical(const QPoint & keyCoord, UKey key)
{
	return !UKeyIsFunctionKey(key) && keyCoord.y() > 0 && (key < Qt::Key_A || key > Qt::Key_Z);
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

static QFont sFont("Prelude");

static QString sElipsis(QChar(0x2026 /* … */));
const int cElipsisFontSize = 14;

const QColor cActiveColor(20, 20, 20);
const QColor cActiveColor_back(0xe2, 0xe2, 0xe2);
const QColor cDisabledColor(100, 100, 100);
const QColor cDisabledColor_back(200, 200, 200);
const QColor cFunctionColor(0xd2, 0xd2, 0xd2);
const QColor cFunctionColor_back(0, 0, 0);
const QColor cBlueColor(75, 151, 222);
const QColor cBlueColor_back(255, 255, 255);

class TabletKeyboardFactory : public VirtualKeyboardFactory
{
public:
	TabletKeyboardFactory() : VirtualKeyboardFactory("Tablet Keyboard")		{}
	InputMethod *	create(IMEDataInterface * dataInterface)				{ return new TabletKeyboard(dataInterface); }
	EVirtualKeyboardSupport	getSupport(int maxWidth, int maxHeight)
	{
		if (maxWidth >= 1024 || maxHeight >= 1024)
			return eVirtualKeyboardSupport_Preferred_Size;
		return eVirtualKeyboardSupport_Poor;
	}
};

static TabletKeyboardFactory sTabletKeyboardFactory;

static gboolean keyboard_idle(gpointer)
{
	TabletKeyboard * keyboard = TabletKeyboard::getExistingInstance();
	if (keyboard)
		return keyboard->idle();
	return false;
}

typedef DoubleDrawRendererT<GlyphSpec> DoubleDrawRenderer;

TabletKeyboard * TabletKeyboard::s_instance = NULL;

TabletKeyboard::TabletKeyboard(IMEDataInterface * dataInterface) : VirtualKeyboard(dataInterface),
	m_shiftDown(false),
	m_symbolDown(false),
	m_resizeMode(false),
	m_lastShiftTime(0),
	m_lastUnlockTime(0),
	m_keyboardTopPading(0),
	m_requestedHeight(-1),
	m_9tileCorner(13, 13),
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
	m_diamondOptimization(true),
	m_idleInit(false),
	m_backspace("icon-delete.png"),
	m_shift("icon-shift.png"),
	m_shift_on("icon-shift-on.png"),
	m_shift_lock("icon-shift-lock.png"),
	m_hide("icon-hide-keyboard.png"),
	m_emoticon_frown("/usr/palm/emoticons-1.5x/emoticon-frown.png"),
	m_emoticon_cry("/usr/palm/emoticons-1.5x/emoticon-cry.png"),
	m_emoticon_smile("/usr/palm/emoticons-1.5x/emoticon-smile.png"),
	m_emoticon_wink("/usr/palm/emoticons-1.5x/emoticon-wink.png"),
	m_emoticon_yuck("/usr/palm/emoticons-1.5x/emoticon-yuck.png"),
	m_emoticon_gasp("/usr/palm/emoticons-1.5x/emoticon-gasp.png"),
	m_emoticon_heart("/usr/palm/emoticons-1.5x/emoticon-heart.png"),
	m_emoticon_frown_small("/usr/palm/emoticons/emoticon-frown.png"),
	m_emoticon_cry_small("/usr/palm/emoticons/emoticon-cry.png"),
	m_emoticon_smile_small("/usr/palm/emoticons/emoticon-smile.png"),
	m_emoticon_wink_small("/usr/palm/emoticons/emoticon-wink.png"),
	m_emoticon_yuck_small("/usr/palm/emoticons/emoticon-yuck.png"),
	m_emoticon_gasp_small("/usr/palm/emoticons/emoticon-gasp.png"),
	m_emoticon_heart_small("/usr/palm/emoticons/emoticon-heart.png"),
	m_background("keyboard-bg.png"),
	m_drag_handle("drag-handle.png"),
	m_drag_highlight("drag-highlight.png"),
	m_white_key("key-white.png"),
	m_gray_key("key-gray.png"),
	m_short_gray_key("key-gray-short.png"),
	m_black_key("key-black.png"),
	m_shift_on_key("key-shift-on.png"),
	m_shift_lock_key("key-shift-lock.png"),
	m_popup("popup-bg.png"),
	m_popup_2("popup-bg-2.png"),
	m_popup_key("popup-key.png"),
	m_glyphCache(440, 800)
{
	if (VERIFY(s_instance == NULL))
		s_instance = this;

	Q_ASSERT(m_IMEDataInterface);

	IMEPixmap::setDefaultLocation("keyboard-tablet");

	m_keymap.setRowHeight(0, m_short_gray_key.height() / 2);
	for (int r = 1; r < TabletKeymap::cKeymapRows; ++r)
		m_keymap.setRowHeight(r, m_white_key.height() / 2);

	m_presetHeight[cKey_Resize_Tiny - cKey_Resize_First] = 243;
	m_presetHeight[cKey_Resize_Small - cKey_Resize_First] = (340 + 243) / 2;
	m_presetHeight[cKey_Resize_Default - cKey_Resize_First] = m_background.height();
	m_presetHeight[cKey_Resize_Large - cKey_Resize_First] = 393;

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

TabletKeyboard::~TabletKeyboard()
{
	if (VERIFY(s_instance == this))
		s_instance = NULL;
}

void TabletKeyboard::editorStateChanged(const PalmIME::EditorState & state)
{
	bool layoutChanged = false;

	if (m_keymap.symbolMode() == TabletKeymap::eSymbolMode_Lock)
		if (m_keymap.setSymbolMode(TabletKeymap::eSymbolMode_Off))
			layoutChanged = true;

	if (m_keymap.setEditorState(state))
		layoutChanged = true;

	m_candidateBar.setEditorState(state);

	if (layoutChanged)
		keyboardLayoutChanged();

	m_shortcutsHandler.resetEditor(state);
}

void TabletKeyboard::autoCapChanged(const bool & autoCap)
{
	if (m_keymap.setAutoCap(autoCap))
		keyboardLayoutChanged();
}

void TabletKeyboard::setShiftMode(TabletKeymap::EShiftMode shiftMode)
{
	if (m_keymap.setShiftMode(shiftMode))
		keyboardLayoutChanged();
}

void TabletKeyboard::setSymbolMode(TabletKeymap::ESymbolMode symbolMode)
{
	if (m_keymap.setSymbolMode(symbolMode))
		keyboardLayoutChanged();
}

void TabletKeyboard::setKeyboardCombo(const std::string & layoutName, const std::string & languageName, bool showLanguageKey)
{
	const TabletKeymap::LayoutFamily * layoutFamily = TabletKeymap::LayoutFamily::findLayoutFamily(layoutName.c_str(), false);	// get default if not found
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

void TabletKeyboard::syncKeymap()
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

void TabletKeyboard::showSuggestions(bool show)
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

void TabletKeyboard::visibleChanged(const bool & visible)
{
	m_candidateBar.clearCandidates();
	if (visible)
	{
		setKeyboardHeight(m_requestedHeight);
	}
	else
	{
		m_keymap.setSymbolMode(TabletKeymap::eSymbolMode_Off);
		m_keymap.setShiftMode(TabletKeymap::eShiftMode_Off);
		clearExtendedkeys();
	}
}

bool TabletKeyboard::setBoolOption(const std::string & optionName, bool value)
{
	if (optionName == "suggestions")
	{
		showSuggestions(value);
	}
	else if (optionName == "diamonds")
	{
		m_diamondOptimization = value;
	}
	else
	{
		g_warning("TabletKeyboard::setBoolOption: \"%s\" is not supported.", optionName.c_str());
		return false;
	}
	return true;
}

bool TabletKeyboard::setIntOption(const std::string & optionName, int value)
{
	g_warning("TabletKeyboard::setIntOption: \"%s\" is not supported.", optionName.c_str());
	return false;
}

bool TabletKeyboard::getValue(const std::string & name, std::string & outValue)
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

int	TabletKeyboard::getKeyboardHeight(UKey ukey)
{
	ukey = qBound<UKey>(cKey_Resize_First, ukey, cKey_Resize_Last);
	return m_presetHeight[ukey - cKey_Resize_First];
}

void TabletKeyboard::requestSize(int size)
{
	requestHeight(getKeyboardHeight(UKey(cKey_Resize_Default + size)));
	m_keymap.resetCachedGlyphsCount();
}

void TabletKeyboard::requestHeight(int height)
{
	if (VERIFY(height > 30))
	{
		m_requestedHeight = height;
		setKeyboardHeight(height);
		if (height > 0 && m_keymap.getCachedGlyphsCount() == 0)
			queueIdlePrerendering();
	}
}

void TabletKeyboard::changePresetHeightForSize(int size, int height)
{
	int sizeIndex = qBound<int>(0, cKey_Resize_Default - cKey_Resize_First + size, cKey_Resize_Last - cKey_Resize_First);
	m_presetHeight[sizeIndex] = qBound<int>(10, height, 2 * m_background.height());
}

void TabletKeyboard::availableSpaceChanged(const QRect & size)
{
	m_candidateBar.commit();
	m_extendedKeys = NULL;
	m_keymap.setRect(0, 0, 0, 0);
	m_candidateBar.frame().setRect(0, 0, 0, 0);
	m_keyboardTopPading = 0;

	// we occupy 100% of width, stretching the keys as necessary. From that, infer the height...
	int	width = size.width();
	int height = m_requestedHeight;
	if (height < 0)	// no height was set yet for this orientation: calculate default value.
	{
		if (width == 1024)	// native assets
		{
			height = m_background.height();
		}
		else
		{
			height = width * 200 / 320;	// arbitrary proportions...
			if (size.height() < 480)
				height = 160;	// shrink height for Pixie & Broadway, and some horizontal layout
			if (height > 300)
				height = 300;
		}
	}
	setKeyboardHeight(height);

	if (width > 0 && height > 0 && m_keymap.getCachedGlyphsCount() == 0)
		queueIdlePrerendering();
}

void TabletKeyboard::setKeyboardHeight(int height, bool notify)
{
	const QRect & availableSpace = m_IMEDataInterface->m_availableSpace.get();
	int width = availableSpace.width();
	int maxHeight = availableSpace.height() - 28;
	if (!VERIFY(height <= maxHeight))
		height = maxHeight;
	if (VERIFY(height > 0))
	{
		m_keyboardDirty = true;

		// assets give us "ideal" non-scaled sizes. Proportionaly adjust m_keyboardTopPading
		int fullHeight = m_background.height();
		int fullKeymapHeight = (m_short_gray_key.height() + (TabletKeymap::cKeymapRows - 1) * m_white_key.height()) / 2;
		if (fullHeight < fullKeymapHeight)
			fullHeight = fullKeymapHeight;	// if background shorter than assets, stretch background!
		int keymapHeight = height * fullKeymapHeight / fullHeight;
		m_keyboardTopPading = height - keymapHeight;
		if (m_keyboardTopPading < 0)
			m_keyboardTopPading = 0;
		// TabletKeymap pushed at the bottom of the available space
		m_keymap.setRect(0, availableSpace.height() - keymapHeight, width, keymapHeight);

		if (notify)
		{
			keyboardLayoutChanged();
			KeyLocationRecorder::instance().keyboardSizeChanged(m_keymap.layoutName(), m_keymap.rect());
		}

		int candidateBarHeight = m_candidateBar.enabled() ? m_short_gray_key.height() / 2 : 0;
		m_candidateBar.frame().setRect(0, m_keymap.rect().top() - candidateBarHeight - m_keyboardTopPading, width, candidateBarHeight);
		//g_debug("TabletKeyboard::setKeyboardHeight: Setting rect to: %d, %d, %dx%d, candidateBar: %d", m_keymap.rect().left(), m_keymap.rect().top(), m_keymap.rect().width(), m_keymap.rect().height(), candidateBarHeight);

		qreal trim = 0;
		if (availableSpace.width() < 480)
			trim = 5;
		else if (availableSpace.width() == 480)
			trim = 4;
		else if (keymapHeight >= fullKeymapHeight)
			trim = 0;
		else
		{
			trim = (fullKeymapHeight - keymapHeight) / 40;
			if (trim > 4)
				trim = 4;
		}
		m_9tileCorner.m_trimH = m_9tileCorner.m_trimV = trim;
		//g_critical("9Tile Shrink: %g (%d)", trim, fullKeymapHeight - keymapHeight);
	}
	else
		g_debug("TabletKeyboard::setKeyboardHeight: FAILED! height: %d, requestedHeight: %d, sizes: %d, %d, %d, %d, background height: %d, keyboard height: %d, available: %d-%d %dx%d.", height, m_requestedHeight,
				  getKeyboardHeight(cKey_Resize_Tiny), getKeyboardHeight(cKey_Resize_Small), getKeyboardHeight(cKey_Resize_Default), getKeyboardHeight(cKey_Resize_Large), m_background.height(),
				  m_keymap.rect().height() + m_keyboardTopPading + m_candidateBar.frame().height(), availableSpace.x(), availableSpace.y(), availableSpace.width(), availableSpace.height());
	if (notify)
		m_IMEDataInterface->m_keyboardHeight.set(m_keymap.rect().height() + m_keyboardTopPading + m_candidateBar.frame().height());
}

void TabletKeyboard::keyboardLayoutChanged()
{
	if (!m_keyboardDirty && m_IMEDataInterface->m_visible.get())
	{
		m_keyboardDirty = true;
		triggerRepaint();
	}
	m_candidateBar.updateKeyboardLayout(m_keymap.layoutName(), m_keymap.getPage(), m_keymap.rect(), m_keymap.isShiftActive(), m_keymap.isCapsLocked(), m_keymap.isAutoCapActive());
}

void TabletKeyboard::clearExtendedkeys()
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

void TabletKeyboard::releaseTouch(int id)
{
	Touch &		touch = m_touches[id];
#if DEBUG_TOUCH
	g_debug("TabletKeyboard::releaseTouch: '%s', consumed: %d, visible: %d", m_keymap.getKeyDisplayString(m_keymap.map(touch.m_keyCoordinate), true).toUtf8().data(), touch.m_consumed, touch.m_visible);
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

void TabletKeyboard::updateTouch(int id, QPointF position)
{
	uint64_t	now = CURRENT_TIME;
	QPointF		touchPosition(position.x(), position.y() - m_keymap.rect().top());
	UKey		extendedKey;
	QPoint		keyCoordinate = (!pointToExtendedPopup(touchPosition, extendedKey) && position.y() > m_keymap.rect().top() - m_keyboardTopPading) ? m_keymap.pointToKeyboard(position.toPoint(), m_diamondOptimization) : cOutside;
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
	if (m_resizeMode)
	{
		if (!touch.m_consumed)
		{
#if 0
			int deltaY = touch.m_lastPosition.y() - position.y();
			touch.m_lastPosition = position;
			int newHeight = m_keymap.rect().height() + m_keyboardTopPading + deltaY;
			int screenHeight = m_IMEDataInterface->m_availableSpace.get().height();
			int maxHeight = screenHeight * 2 / 3;
			int minHeight = Min(screenHeight / 3, 160);
			newHeight = MinMax(minHeight, newHeight, maxHeight);
			requestHeight(newHeight);
			//g_debug("New height: %d, delta: %d, %d, %d, %dx%d", newHeight, deltaY, m_keymap.rect().left(), m_keymap.rect().top(), m_keymap.rect().width(), m_keymap.rect().height());
#else
			int deltaY = touch.m_lastPosition.y() - touchPosition.y();
			//g_debug("First position Y: %g, Last position Y: %g, delta: %d", touch.m_firstPosition.y(), touch.m_lastPosition.y(), deltaY);
			int currentHeight = m_requestedHeight;
			UKey size = cKey_Resize_Default;
			while (getKeyboardHeight(size) > currentHeight && UKeyIsKeyboardSizeKey(UKey(size - 1)))
				size = UKey(size - 1);
			while (getKeyboardHeight(size) < currentHeight && UKeyIsKeyboardSizeKey(UKey(size + 1)))
				size = UKey(size + 1);
			//g_debug("Size: %d, height: %d, deltaY: %d, getKeyboardHeight(+1) - currentHeight = %d", size - cKey_Resize_Default, currentHeight, deltaY, getKeyboardHeight(UKey(size + 1)) - currentHeight);
			//g_debug("Size: %d, height: %d, deltaY: %d, getKeyboardHeight(-1) - currentHeight = %d", size - cKey_Resize_Default, currentHeight, deltaY, getKeyboardHeight(UKey(size - 1)) - currentHeight);
			if (UKeyIsKeyboardSizeKey(UKey(size - 1)) && 3 * deltaY <= 2 * (getKeyboardHeight(UKey(size - 1)) - currentHeight))	// going down
				VirtualKeyboardPreferences::instance().selectKeyboardSize(size - cKey_Resize_Default - 1);
			else if (UKeyIsKeyboardSizeKey(UKey(size + 1)) && 3 * deltaY >= 2 * (getKeyboardHeight(UKey(size + 1)) - currentHeight))	// going up
				VirtualKeyboardPreferences::instance().selectKeyboardSize(size - cKey_Resize_Default + 1);
#endif
		}
		return;
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
					if (newTouch && (canRepeat(newKey) || m_keymap.getExtendedChars(keyCoordinate) || (newKey == cKey_Hide && m_touches.size() == 1)))
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
			{
				if (newKey == cKey_ResizeHandle)
				{
					m_resizeMode = true;
					PixmapCache::instance().suspendPurge(true);
					clearExtendedkeys();
				}
				else
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

void TabletKeyboard::handleKey(UKey key, QPointF where)
{
	//g_debug("TabletKeyboard::handleKey: '%s'", QString(key).toUtf8().data());
	TabletKeymap::EShiftMode	shiftMode = m_keymap.shiftMode();
	TabletKeymap::ESymbolMode	symbolMode = m_keymap.symbolMode();
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
			else if (m_keymap.symbolMode() == TabletKeymap::eSymbolMode_Lock)
				symbolMode = TabletKeymap::eSymbolMode_Off;
			else
				symbolMode = TabletKeymap::eSymbolMode_Lock, shiftMode = TabletKeymap::eShiftMode_Off;
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
				shiftMode = TabletKeymap::eShiftMode_CapsLock;
			else if (shiftMode == TabletKeymap::eShiftMode_CapsLock)
			{
				shiftMode = TabletKeymap::eShiftMode_Off;
				m_lastUnlockTime = now;
			}
			else if (shiftMode == TabletKeymap::eShiftMode_Off)
				shiftMode = TabletKeymap::eShiftMode_Once;
			else
				shiftMode = TabletKeymap::eShiftMode_Off;
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
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
            qtkey = key; // used to navigate the cursor
            break;
        case Qt::Key_Tab:
			switch (m_keymap.tabAction())
			{
			case TabletKeymap::eTabAction_Next:
				m_IMEDataInterface->performEditorAction(PalmIME::FieldAction_Next);
				break;
			case TabletKeymap::eTabAction_Previous:
				m_IMEDataInterface->performEditorAction(PalmIME::FieldAction_Previous);
				break;
			case TabletKeymap::eTabAction_Tab:
			default:
				qtkey = Qt::Key_Tab;
			}
			break;
		case cKey_Resize_Tiny:
		case cKey_Resize_Small:
		case cKey_Resize_Default:
		case cKey_Resize_Large:
			VirtualKeyboardPreferences::instance().selectKeyboardSize(key - cKey_Resize_Default);
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
			{
				UKey	altKey = m_keymap.map(m_keymap.pointToKeyboard(where.toPoint() + m_keymap.rect().topLeft(), !m_diamondOptimization));
				KeyLocationRecorder::instance().record(m_keymap.getKeyDisplayString(key, true), where.toPoint(), m_keymap.getKeyDisplayString(altKey, true));
			}
			if (UKeyIsTextShortcutKey(key))
			{
				m_IMEDataInterface->commitText(m_keymap.getKeyDisplayString(key).toUtf8().data());
				m_shortcutsHandler.resetEditor();
			}
			else if (m_shortcutsHandler.filterKey(qtkey))
			{
				if (UKeyIsFunctionKey(qtkey))
				{
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
			symbolMode = TabletKeymap::eSymbolMode_Off;
	}
	if (consumeMode)
	{
		if (m_keymap.shiftMode() == TabletKeymap::eShiftMode_Once)
			shiftMode = TabletKeymap::eShiftMode_Off;
	}
	if (m_keymap.shiftMode() != shiftMode)
		setShiftMode(shiftMode);
	if (m_keymap.symbolMode() != symbolMode)
		setSymbolMode(symbolMode);
}

void TabletKeyboard::sendKeyDownUp(Qt::Key key, Qt::KeyboardModifiers modifiers)
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

void TabletKeyboard::touchEvent(const QTouchEvent& te)
{
	if (m_IMEDataInterface)
	{
		const QList<QTouchEvent::TouchPoint> & touchPoints = te.touchPoints();
#if DEBUG_TOUCH
		std::string	str;
		for (QList<QTouchEvent::TouchPoint>::ConstIterator iter = touchPoints.constBegin(); iter != touchPoints.constEnd(); ++iter)
		{
			const QTouchEvent::TouchPoint & touchPoint = *iter;
			QPoint keyPos = m_keymap.pointToKeyboard(touchPoint.pos().toPoint(), m_diamondOptimization);
			Qt::Key key = m_keymap.map(keyPos);
            ::append_format(str, "   Id: %d, location: %gx%g %s, Key: %dx%d = '%s'.\n", touchPoint.id(),
                            touchPoint.pos().x(), touchPoint.pos().y(), touchPointState(touchPoint.state()), keyPos.x(), keyPos.y(), m_keymap.getKeyDisplayString(key, true).toUtf8().data());
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
					if (!m_resizeMode)
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
			if (m_resizeMode)
			{
				m_resizeMode = false;
				PixmapCache::instance().suspendPurge(false);
				triggerRepaint();
			}
			setShiftKeyDown(false);
			setSymbolKeyDown(false);
		}
	}
}

void TabletKeyboard::tapEvent(const QPoint& tapPt)
{
#if DEBUG_TOUCH
	g_debug("tapEvent: %d, %d", tapPt.x(), tapPt.y());
#endif
	m_candidateBar.tapEvent(tapPt);
}

void TabletKeyboard::screenEdgeFlickEvent()
{
	// Mark all touches as consumed
	for (std::map<int, Touch>::iterator iter = m_touches.begin(); iter != m_touches.end(); ++iter) {
		iter->second.m_consumed = true;
	}
}

void TabletKeyboard::repeatChar()
{
	if (TabletKeymap::isValidLocation(m_repeatKey))
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

bool TabletKeyboard::setExtendedKeys(QPoint keyCoord, bool cancelIfSame)
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

bool TabletKeyboard::pointToExtendedPopup(QPointF position, UKey & outKey)
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

void TabletKeyboard::getExtendedPopupSpec(int & outCellCount, int & outLineCount, int & outLineLength)
{
	outCellCount = 0;
	if (m_extendedKeys)
		while (m_extendedKeys[outCellCount] != cKey_None)
			++outCellCount;
	outLineCount = (outCellCount > cPopupSingleLineMax) ? 2 : 1;
	outLineLength = (outCellCount + outLineCount - 1) / outLineCount;
}

bool TabletKeyboard::canRepeat(UKey key) const
{
	return (key == Qt::Key_Space || key == Qt::Key_Backspace || key == Qt::Key_Left || key == Qt::Key_Right);
}

void TabletKeyboard::stopRepeat()
{
	m_timer.stop();
	m_repeatKey = cOutside;
	m_repeatStartTime = 0;
}

void TabletKeyboard::showKeymapRegions()
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
			QPoint	keycoord = m_keymap.pointToKeyboard(QPoint(x, y_offset + y), m_diamondOptimization);
			if (keycoord != cOutside)
			{
				painter.setPen(colorMap[QChar(m_keymap.map(keycoord)).unicode()]);	// create or reuse random color for this character
				painter.drawPoint(x, y);
			}
		}
	m_keyboardDirty = true;
}

void TabletKeyboard::paint(QPainter & painter)
{
	//painter.setClipping(false);
	PerfMonitor perf("TabletKeyboard::paint");
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
	CachedGlyphRenderer<GlyphSpec>	renderer(painter, m_glyphCache, doubleDrawRenderer, TabletKeymap::cKeymapColumns * (TabletKeymap::cKeymapRows + 1));
	for (int y = 0; y < TabletKeymap::cKeymapRows; ++y)
	{
		for (int x = 0; x < TabletKeymap::cKeymapColumns; ++x)
		{
			QPoint	keyCoord(x, y);
			UKey key = m_keymap.map(x, y);
			QRect r;
			int count = m_keymap.keyboardToKeyZone(keyCoord, r);
			if (count > 0 && key != cKey_None)
			{
				if (key == Qt::Key_Shift)
					drawKeyBackground(painter, r, keyCoord, key, false, count);
				drawKeyCap(&painter, renderer, r, keyCoord, key, false);
			}
		}
	}
	renderer.flush();
	perf.trace("Draw labels");
	if (m_resizeMode)
		painter.drawPixmap(keymapRect.left(), keymapRect.top() - m_drag_highlight.height() - m_keyboardTopPading - 1, keymapRect.width(), m_drag_highlight.height(), m_drag_highlight);
	UKey	extendedKey = cKey_None;
	QRect	r;
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
						drawKeyCap(&painter, renderer, r, touch.m_keyCoordinate, key, true);
						//g_debug("'%s' drawn pressed, consumed: %d", QString(key).toUtf8().data(), touch.m_consumed);
					}
				}
			}
		}
	}
	renderer.flush();
	m_candidateBar.paintTrace(painter, keyboardFrame.top() + m_keyboardTopPading, cBlueColor, 4);
	if (m_extendedKeys && m_extendedKeysFrame.isValid())
	{
		for (int y = 0; y < TabletKeymap::cKeymapRows; ++y)
		{
			for (int x = 0; x < TabletKeymap::cKeymapColumns; ++x)
			{
				if (m_keymap.getExtendedChars(QPoint(x, y)) && m_keymap.keyboardToKeyZone(QPoint(x, y), r) > 0)
				{
					r.setWidth(r.width() - 9 + m_9tileCorner.m_trimH); r.setHeight(r.height() - 9 + m_9tileCorner.m_trimV);
					renderer.render(r, GlyphSpec(sElipsis, cElipsisFontSize, false, cActiveColor, cActiveColor_back), sFont, Qt::AlignRight | Qt::AlignBottom);
				}
			}
		}
		renderer.flush();
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
			QString	text = m_keymap.getKeyDisplayString(key);
			int fontSize = (text.length() < 6) ? cPopupFontSize : cPopupFontSize - 8;
			if (sFont.pixelSize() != fontSize)
			{
				sFont.setPixelSize(fontSize);
				painter.setFont(sFont);
			}
			if (UKeyIsKeyboardSizeKey(key) && m_requestedHeight == getKeyboardHeight(key))
			{
				sFont.setBold(true);			painter.setFont(sFont);
				doubleDrawRenderer.renderNow(painter, cell, GlyphSpec(text, fontSize, true, cBlueColor, cBlueColor_back));	// don't ever cache, because of color exception!
				sFont.setBold(false);			painter.setFont(sFont);
			}
			else
				renderer.render(cell, GlyphSpec(text, fontSize, false, cActiveColor, cActiveColor_back), sFont);
		}
	}
	m_extendedKeyShown = extendedKey;
#if VKB_SHOW_GLYPH_CACHE
	painter.setPen(QColor(255, 0, 0)); painter.drawRect(QRect(QPoint(0, 0), m_glyphCache.pixmap().size())); painter.drawPixmap(0, 0, m_glyphCache.pixmap());
#endif
#if VKB_FORCE_FPS
	triggerRepaint();
#endif
	if (renderer.getCacheMissCount() > 0 && (!m_glyphCache.isFull() || m_keymap.getCachedGlyphsCount() < 3))
		queueIdlePrerendering();
}

bool TabletKeyboard::updateBackground()
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
#if RESIZE_HANDLES
			int xoffset = 3 - m_9tileCorner.m_trimH;
			int yoffset = 5 - m_9tileCorner.m_trimV;
			offscreenPainter.drawPixmap(keyboardFrame.topLeft() + QPoint(xoffset, yoffset), m_drag_handle);
			offscreenPainter.drawPixmap(keyboardFrame.topRight() + QPoint(-m_drag_handle.width() - xoffset, yoffset), m_drag_handle);
#endif
			m_nineTileSprites.reserve(true);
			for (int y = 0; y < TabletKeymap::cKeymapRows; ++y)
			{
				for (int x = 0; x < TabletKeymap::cKeymapColumns; ++x)
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
			for (int y = 0; y < TabletKeymap::cKeymapRows; ++y)
			{
				for (int x = 0; x < TabletKeymap::cKeymapColumns; ++x)
				{
					QPoint	keyCoord(x, y);
					UKey key = m_keymap.map(x, y);
					if (key != Qt::Key_Shift)
					{
						QRect r;
						int count = m_keymap.keyboardToKeyZone(keyCoord, r);
						if (count > 0 && key != cKey_None)
							drawKeyBackground(offscreenPainter, r, keyCoord, key, false, count);
					}
				}
			}
		}
		m_keyboardDirty = false;
		return true;
	}
	return false;
}

QPixmap & TabletKeyboard::getKeyBackground(const QPoint & keyCoord, UKey key)
{
	if (keyCoord.y() == 0)
		return m_short_gray_key;
	else if (key == Qt::Key_Shift)
	{
		switch (m_keymap.shiftMode())
		{
		case TabletKeymap::eShiftMode_CapsLock: return m_shift_lock_key;
		case TabletKeymap::eShiftMode_Once: return m_shift_on_key;
		default:
			return m_black_key;
		}
	}
	else
		return selectFromKeyType<QPixmap &>(m_keymap.map(keyCoord, TabletKeymap::eLayoutPage_plain), m_white_key, m_black_key, m_gray_key);
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

inline int font_size(const QString & text, const QColor & color, int baseSize, int percent)
{
	if (color != cActiveColor)
		return baseSize * percent / 100;
	return boostSize(text) ? baseSize + 2 : baseSize;
}

void TabletKeyboard::drawKeyCap(QPainter * painter, GlyphRenderer<GlyphSpec> & renderer, QRect location, const QPoint & keyCoord, UKey key, bool pressed)
{
	location.setBottom(location.bottom() - 4);
	if (pressed)
		location.translate(0, +2);
	QString		text, altText;
	bool		twoHorizontal = false;
	bool		twoVertical = false;
	QColor		mainCharColor = cActiveColor;
	QColor		mainCharColor_back = cActiveColor_back;
	QColor		altCharColor = cDisabledColor;
	QColor		altCharColor_back = cDisabledColor_back;
	bool		capitalize = m_keymap.isCapOrAutoCapActive();
//	bool		capitalize = key >= Qt::Key_A && key <= Qt::Key_Z;
	if (key == Qt::Key_Space)
		text = m_candidateBar.autoSelectCandidate();
	else if (UKeyIsUnicodeQtKey(key))
	{	// key is also a unicode character...
		UKey plain = m_keymap.map(keyCoord, TabletKeymap::eLayoutPage_plain);
		UKey alt = m_keymap.map(keyCoord, TabletKeymap::eLayoutPage_Alternate);
		if (plain != alt && alt != cKey_None)
		{
			twoHorizontal = KeyCap_TwoHorizontal(keyCoord, plain);
			twoVertical = KeyCap_TwoVertical(keyCoord, plain);
			if (twoHorizontal || twoVertical)
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
					altCharColor = cActiveColor;
					altCharColor_back = cActiveColor_back;
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
	else if ((UKeyIsEmoticonKey(key) && m_keymap.showEmoticonsAsGraphics()) || (text = m_keymap.getKeyDisplayString(key)).size() == 0)
	{
		QPixmap	*	pix = NULL;
		int cPixMargin = UKeyIsEmoticonKey(key) ? 8 : 2;
		switch ((int)key)
		{
		case Qt::Key_Shift:
			switch (m_keymap.shiftMode())
			{
			case TabletKeymap::eShiftMode_Once: 	pix = &m_shift_on.pixmap();		break;
			case TabletKeymap::eShiftMode_CapsLock:	pix = &m_shift_lock.pixmap();	break;
			case TabletKeymap::eShiftMode_Off:
			default:
			 pix = m_keymap.isAutoCapActive() ? &m_shift_on.pixmap() : &m_shift.pixmap();
			}
			break;
		case Qt::Key_Backspace:		pix = &m_backspace.pixmap();		break;
		case cKey_Hide:				pix = &m_hide.pixmap();				break;
		case cKey_Emoticon_Frown:	pix = m_emoticon_frown.height()  + 2 * cPixMargin > location.height() ? &m_emoticon_frown_small.pixmap() : &m_emoticon_frown.pixmap();	break;
		case cKey_Emoticon_Cry:		pix = m_emoticon_cry.height()  + 2 * cPixMargin > location.height() ? &m_emoticon_cry_small.pixmap() : &m_emoticon_cry.pixmap();		break;
		case cKey_Emoticon_Smile:	pix = m_emoticon_smile.height()  + 2 * cPixMargin > location.height() ? &m_emoticon_smile_small.pixmap() : &m_emoticon_smile.pixmap();	break;
		case cKey_Emoticon_Wink:	pix = m_emoticon_wink.height()  + 2 * cPixMargin > location.height() ? &m_emoticon_wink_small.pixmap() : &m_emoticon_wink.pixmap();	break;
		case cKey_Emoticon_Yuck:	pix = m_emoticon_yuck.height()  + 2 * cPixMargin > location.height() ? &m_emoticon_yuck_small.pixmap() : &m_emoticon_yuck.pixmap();	break;
		case cKey_Emoticon_Gasp:	pix = m_emoticon_gasp.height()  + 2 * cPixMargin > location.height() ? &m_emoticon_gasp_small.pixmap() : &m_emoticon_gasp.pixmap();	break;
		case cKey_Emoticon_Heart:	pix = m_emoticon_heart.height()  + 2 * cPixMargin > location.height() ? &m_emoticon_heart_small.pixmap() : &m_emoticon_heart.pixmap();	break;
		default: /* NOP */;
		}
		if (pix && painter)
		{
			location.adjust(cPixMargin, cPixMargin, - cPixMargin, - cPixMargin);
			if (pix->height() > location.height() || pix->width() > location.width())
			{
				//g_debug("TabletKeyboard::drawKeyCap shrinking \"%s\" by %d pixels", m_keymap.getKeyDisplayString(key, true).toUtf8().data(), location.height() - pix->height());
				painter->setRenderHints(cRenderHints, true);
				if (pix->height() * location.width() > location.height() * pix->width())
				{
					int targetWidth = location.height() * pix->width() / pix->height();
					painter->drawPixmap(location.left() + (location.width() - targetWidth) / 2, location.top(), targetWidth, location.height(), *pix);
				}
				else
				{
					int targetHeight = location.width() * pix->height() / pix->width();
					painter->drawPixmap(location.left(), location.top() + (location.height() - targetHeight) / 2, location.width(), targetHeight, *pix);
				}
			}
			else
				painter->drawPixmap((int) location.left() + (location.width() - pix->width()) / 2, (int) location.top() + (location.height() - pix->height()) / 2, *pix);
		}
	}

	if (text.size() > 0)
	{
		bool forceAlignHCenter = false;		// if too tight, center text for better looking results
		int height = location.height();
		int fontSize = 24;
		if (!twoVertical && !twoHorizontal && !UKeyIsFunctionKey(key))
			fontSize = 26;
		int centerOffset = 1;
		if (twoVertical && height / 3 < fontSize - 2)
			twoHorizontal = true, centerOffset = 2;
		if (height / 2 < fontSize)
			fontSize = (height + 1) / 2;
		if (text.size() > 1)
		{
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
			if (mainCharColor == cActiveColor)
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
			if (key == Qt::Key_Return)
			{ // Smaller, bottom right corner...
				location.setHeight(height * 80 / 100 + m_9tileCorner.m_trimV);
				if (forceAlignHCenter)
					renderer.render(location, GlyphSpec(text, qMin<int>(height, fontSize - 2), sFont.bold(), cFunctionColor, cFunctionColor_back), sFont, Qt::AlignBottom | Qt::AlignHCenter);
				else
				{
					location.setWidth(location.width() * 85 / 100 + m_9tileCorner.m_trimH);
					renderer.render(location, GlyphSpec(text, qMin<int>(height, fontSize - 2), sFont.bold(), cFunctionColor, cFunctionColor_back), sFont, Qt::AlignBottom | Qt::AlignRight);
				}
			}
			else
			{
				int size = qMin<int>(height, fontSize);
				if (keyCoord.y() == 0)
					renderer.render(location, GlyphSpec(text, size, sFont.bold(), cActiveColor, cActiveColor_back), sFont);
				else if (key == Qt::Key_Space)
				{
					if (painter)
						renderer.renderNow(location, GlyphSpec(text, size, sFont.bold(), selectFromKeyType<QColor>(key, cActiveColor, cFunctionColor, cActiveColor), selectFromKeyType<QColor>(key, cActiveColor_back, cFunctionColor_back, cActiveColor_back)), sFont);
				}
				else
					renderer.render(location, GlyphSpec(text, size, sFont.bold(), selectFromKeyType<QColor>(key, cActiveColor, cFunctionColor, cActiveColor), selectFromKeyType<QColor>(key, cActiveColor_back, cFunctionColor_back, cActiveColor_back)), sFont);
			}
		}
		sFont.setBold(false);
	}
}

bool TabletKeyboard::setShiftKeyDown(bool shiftKeyDown)
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

bool TabletKeyboard::setSymbolKeyDown(bool symbolKeyDown)
{
	if (m_keymap.setSymbolKeyDown(symbolKeyDown))
	{
		keyboardLayoutChanged();
		return true;
	}
	return false;
}

void TabletKeyboard::makeSound(UKey key)
{
	if (VirtualKeyboardPreferences::instance().getTapSounds() && key != cKey_None)
		m_IMEDataInterface->keyDownAudioFeedback(key);
}

void TabletKeyboard::queueIdlePrerendering()
{
	if (!m_idleInit)
	{
		m_idleInit = true;
		g_idle_add_full(G_PRIORITY_LOW, keyboard_idle, NULL, NULL);
	}
}

bool TabletKeyboard::idle()
{	// there is only ever one TabletKeyboard. Using statics to avoid exposing everywhere variables only used here
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
	int stateIndex = index / TabletKeymap::cKeymapRows;
	int y = index % TabletKeymap::cKeymapRows;

	if (stateIndex < 4)
	{	// pre-rendering all keyboards states for the current size, one row at a time...
		DoubleDrawRenderer				renderer;
		GlyphCachePopulator<GlyphSpec>	populator(m_glyphCache, renderer);

		bool						shiftDown = m_keymap.isShiftDown();
		bool						symbolDown = m_keymap.isSymbolDown();
		bool						autoCapActive = m_keymap.isAutoCapActive();
		TabletKeymap::EShiftMode	shiftMode = m_keymap.shiftMode();
		TabletKeymap::ESymbolMode	symbolMode = m_keymap.symbolMode();

		m_keymap.setShiftKeyDown(false);
		m_keymap.setSymbolKeyDown(false);
		m_keymap.setAutoCap(false);

		if (stateIndex == 0)
		{
			m_keymap.setShiftMode(TabletKeymap::eShiftMode_Off);
			m_keymap.setSymbolMode(TabletKeymap::eSymbolMode_Off);
		}
		else if (stateIndex == 1)
		{
			m_keymap.setShiftMode(TabletKeymap::eShiftMode_Off);
			m_keymap.setSymbolMode(TabletKeymap::eSymbolMode_Off);
			m_keymap.setAutoCap(true);
		}
		else if (stateIndex == 2)
		{
			m_keymap.setShiftMode(TabletKeymap::eShiftMode_Once);
			m_keymap.setSymbolMode(TabletKeymap::eSymbolMode_Lock);
		}
		else
		{
			m_keymap.setShiftMode(TabletKeymap::eShiftMode_Once);
			m_keymap.setSymbolMode(TabletKeymap::eSymbolMode_Lock);
			m_keymap.setAutoCap(true);
		}

		std::string msg = string_printf("TabletKeyboard pre-render (%dx%d): shift %d, symbol %d, cap %d, autoCap %d, index=%d, y=%d", m_keymap.rect().width(), m_keymap.rect().height(), m_keymap.isShiftActive(), m_keymap.isSymbolActive(), m_keymap.isCapActive(), m_keymap.isAutoCapActive(), stateIndex, y);
		PerfMonitor perf(msg.c_str());
		//g_debug("%s", msg.c_str());
		for (int x = 0; x < TabletKeymap::cKeymapColumns; ++x)
		{
			QPoint	keyCoord(x, y);
			UKey key = m_keymap.map(x, y);
			QRect r;
			int count = m_keymap.keyboardToKeyZone(keyCoord, r);
			r.moveTo(0, 0);
			if (count > 0 && key != Qt::Key_Space && key != cKey_None)
				drawKeyCap(NULL, populator, r, keyCoord, key, false);
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
		if (x < 0 || y < 0 || x >= TabletKeymap::cKeymapColumns || y >= TabletKeymap::cKeymapRows)
		{
			x = y = 0;
			extendedChars = m_keymap.getExtendedChars(QPoint(x, y));
			extendedIndex = 0;
		}
		uint64_t timeLimit = CURRENT_TIME + 10;	// process 10ms max
		do {
			if (extendedChars)
			{
				UKey key = extendedChars[extendedIndex];
				if (UKeyIsUnicodeQtKey(key))
				{
					//g_debug("pre-render %dx%d %s...", x, y, QString(QChar(key)).toUtf8().data());
					populator.render(QRect(QPoint(), m_popup_key.size()), GlyphSpec(QString(QChar(key).toLower()), cPopupFontSize, false, cActiveColor, cActiveColor_back), sFont);
					populator.render(QRect(QPoint(), m_popup_key.size()), GlyphSpec(QString(QChar(key).toUpper()), cPopupFontSize, false, cActiveColor, cActiveColor_back), sFont);
				}
				else
				{
					QString text = m_keymap.getKeyDisplayString(key);
					if (text.size())
						populator.render(QRect(QPoint(), m_popup_key.size()), GlyphSpec(text, cPopupFontSize, false, cActiveColor, cActiveColor_back), sFont);
				}
				if (!extendedChars[++extendedIndex])
					extendedChars = NULL;
			}
			if (!extendedChars)
			{
				if (++x >= TabletKeymap::cKeymapColumns)
				{
					x = 0;
					if (++y >= TabletKeymap::cKeymapRows)
						break;
				}
				extendedChars = m_keymap.getExtendedChars(QPoint(x, y));
				extendedIndex = 0;
			}
			if (CURRENT_TIME > timeLimit)
				return true;
		} while (true);

		// cache elipsis...
		populator.render(QRect(0, 0, 20, 20), GlyphSpec(sElipsis, cElipsisFontSize, false, cActiveColor, cActiveColor_back), sFont);

		sInitExtendedGlyphs = false;
	}

	m_keymap.incCachedGlyphs();
	sCount = IMEPixmap::count();
	m_idleInit = false;
	//g_debug("TabletKeyboard background init complete!");

#if 0
#if defined(TARGET_DEVICE)
	m_glyphCache.pixmap().toImage().save("/media/internal/glyphcache.png");
#else
	m_glyphCache.pixmap().toImage().save(QString(getenv("HOME")) + "/Desktop/glyphcache.png");
#endif
#endif

	return false;
}

}; // namespace Tablet_Keyboard
