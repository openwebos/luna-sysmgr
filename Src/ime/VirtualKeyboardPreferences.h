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




#ifndef VIRTUAL_KEYBOARD_PREFERENCES_H
#define VIRTUAL_KEYBOARD_PREFERENCES_H

#include <string>
#include <vector>

#include "VirtualKeyboard.h"
#include "qobject.h"

class VirtualKeyboardPreferences : public QObject
{
	Q_OBJECT

	struct SKeyboardCombo
	{
		void	clear()										{ layout.clear(); language.clear(); }
		void	set(const std::string & layout_)			{ layout = layout_; language.clear(); }
		void	set(const std::string & layout_, const std::string & language_)
															{ layout = layout_; language = language_; }

		bool	empty() const								{ return layout.empty() && language.empty(); }
		bool	operator==(const SKeyboardCombo & rhs)		{ return layout == rhs.layout && language == rhs.language; }
		bool	operator!=(const SKeyboardCombo & rhs)		{ return layout != rhs.layout || language != rhs.language; }

		std::string	layout;
		std::string language;
	};

public Q_SLOTS:

	void bootFinished();

public:
	VirtualKeyboardPreferences();

	// An instance always exists, hence the reference as opposed to a pointer...
	static VirtualKeyboardPreferences & instance();

	// This is the only way to get access to the virtual keyboard outside of its implementation.
	// Will return NULL when it's not yet created, when in WebAppMgr, or when there is none (physical keyboard...)
	VirtualKeyboard *			virtualKeyboard() const					{ return mVirtualKeyboard; }

	void						applyInitSettings(VirtualKeyboard * keyboard);

    void                        applyFirstUseSettings();
    void                        localeChanged();

	void						selectNextKeyboardCombo();
	void						selectKeyboardCombo(int index);
	void						selectLayoutCombo(const char * layoutName);
	void						selectKeyboardSize(int size);	// 0 is default, 1 is large, -1 small, -2 extra small...
	int							getKeyboardComboCount() const			{ return mCombos.size(); }
	const SKeyboardCombo &		getkeyboardCombo(int index) const		{ return mCombos[index]; }

	SKeyboardCombo				getDefault();
	void						createDefaultKeyboards();
	void						clearDefaultDeyboards();
	void						setTapSounds(bool on);

	void						virtualKeyboardPreferencesChanged(const char * prefs);
	void						virtualKeyboardSettingsChanged(const char * settings);

	bool						getTapSounds() const					{ return mTapSounds; }
	bool						getSpaces2period() const				{ return mSpaces2period; }
	void						activateCombo();

private:
	void						saveSettings();
	void						savePreferences(const std::vector<SKeyboardCombo> & combos);		// normaly only ever done by the prefs app. For testing only.

	bool						mTapSounds;							// should key presses make a sound?
	bool						mSpaces2period;						// should two-spaces be converted to a period, à la iPhone & Blackberry?
	int							mKeyboardSize;

	bool						mSettingsReceived;
	bool						mPrefsReceived;

	std::vector<SKeyboardCombo>	mCombos;
	SKeyboardCombo				mActiveCombo;

	VirtualKeyboard *			mVirtualKeyboard;
};

#define PALM_VIRTUAL_KEYBOARD_PREFS		"x_palm_virtualkeyboard_prefs"
#define PALM_VIRTUAL_KEYBOARD_SETTINGS	"x_palm_virtualkeyboard_settings"
#define PALM_VIRTUAL_KEYBOARD_LAYOUTS	"x_palm_virtualkeyboard_layouts"

#endif // VIRTUAL_KEYBOARD_PREFERENCES_H
