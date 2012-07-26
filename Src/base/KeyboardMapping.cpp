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

#include "KeyboardMapping.h"

#include <glib.h>
#include <map>

#include "Event.h"

static bool s_initialized = false;
static std::map<int, int> s_normalKeyMap;
static std::map<int, int> s_shiftKeyMap;
static std::map<int, int> s_optKeyMap;
static std::map<int, int> s_qtKeyMap;
static std::map<int, int> s_qtOptKeyMap;

static const KeyMapType* s_deviceKeyMap = 0;

static void initializeIfNecessary()
{
	if (G_LIKELY(s_initialized == true))
		return;

	s_initialized = true;

	//s_deviceKeyMap = webosGetDeviceKeymap();

	if (s_deviceKeyMap) {

		int index = 0;
		while (s_deviceKeyMap[index].devicekey != (int) LAST_KEY) {

			const KeyMapType& k = s_deviceKeyMap[index];

			if (s_normalKeyMap.find(k.normal) == s_normalKeyMap.end() &&
				s_shiftKeyMap.find(k.shift) == s_shiftKeyMap.end() &&
				s_optKeyMap.find(k.opt) == s_optKeyMap.end() &&
				s_qtKeyMap.find(k.qtKey) == s_qtKeyMap.end() &&
				s_qtOptKeyMap.find(k.qtKeyOpt) == s_qtOptKeyMap.end()) {

				s_normalKeyMap[k.normal] = index;
				s_shiftKeyMap[k.shift]  = index;
				s_optKeyMap[k.opt]    = index;
				s_qtKeyMap[k.qtKey] = index;
				s_qtOptKeyMap[k.qtKeyOpt] = index;
			}

			index++;
		}
	}
}

KeyMapType getDetailsForQtKey(int qtKey, Qt::KeyboardModifiers modifiers)
{
	initializeIfNecessary();

	KeyMapType result;
	result.devicekey = 0;
	result.normal	= Key_Null;
	result.shift	= Key_Null;
	result.opt		= Key_Null;
	result.qtKey	= Qt::Key_unknown;
	result.qtKeyOpt	= Qt::Key_unknown;
	result.virtualkeycode = Vk_NULL;

	if (modifiers & Qt::AltModifier) {
		std::map<int, int>::const_iterator it = s_qtOptKeyMap.find(qtKey);
		if (it != s_qtOptKeyMap.end()) {
			int index = it->second;
			result = s_deviceKeyMap[index];
		}
	}
	else {
		std::map<int, int>::const_iterator it = s_qtKeyMap.find(qtKey);
		if (it != s_qtKeyMap.end()) {
			int index = it->second;
			result = s_deviceKeyMap[index];
		}
	}

	return result;
}

KeyMapType getDetailsForKey(int key, unsigned int modifiers)
{
	initializeIfNecessary();

	KeyMapType result;
	result.devicekey = 0;
	result.normal	= Key_Null;
	result.shift	= Key_Null;
	result.opt		= Key_Null;
	result.qtKey	= Qt::Key_unknown;
	result.qtKeyOpt	= Qt::Key_unknown;
	result.virtualkeycode = Vk_NULL;

	if (modifiers & Event::Alt) {
		std::map<int, int>::const_iterator it = s_optKeyMap.find(key);
		if (it != s_optKeyMap.end()) {
			int index = it->second;
			result = s_deviceKeyMap[index];
		}
	}
	else if (modifiers & Event::Shift) {
		std::map<int, int>::const_iterator it = s_shiftKeyMap.find(key);
		if (it != s_shiftKeyMap.end()) {
			int index = it->second;
			result = s_deviceKeyMap[index];
		}
	}
	else {
		std::map<int, int>::const_iterator it = s_normalKeyMap.find(key);
		if (it != s_normalKeyMap.end()) {
			int index = it->second;
			result = s_deviceKeyMap[index];
		}
	}

	return result;
}

KeyMapType getDetailsForKey(int key)
{
	initializeIfNecessary();
	
	KeyMapType result;
	result.devicekey = 0;
	result.normal	= Key_Null;
	result.shift	= Key_Null;
	result.opt		= Key_Null;
	result.qtKey	= Qt::Key_unknown;
	result.qtKeyOpt	= Qt::Key_unknown;
	result.virtualkeycode = Vk_NULL;
	
	std::map<int, int>::const_iterator it;

	it = s_normalKeyMap.find(key);
	if (it != s_normalKeyMap.end()) {
		int index = it->second;
		result = s_deviceKeyMap[index];
	}
	else {

		it = s_shiftKeyMap.find(key);
		if (it != s_shiftKeyMap.end()) {
			int index = it->second;
			result = s_deviceKeyMap[index];
		}
		else {

			it = s_optKeyMap.find(key);
			if (it != s_optKeyMap.end()) {
				int index = it->second;
				result = s_deviceKeyMap[index];
			}
		}
	}

	return result;
}
