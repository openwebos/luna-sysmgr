/* @@@LICENSE
*
*      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include <string.h>
#include "KeywordMap.h"

#include "cjson/json.h"

/// ------------------------------------ public: -----------------------------------------------------------------------

KeywordMap::KeywordMap()
{
	
}

KeywordMap::~KeywordMap()
{
	std::list<gchar*>::iterator it = m_keywords.begin();
	std::list<gchar*>::const_iterator end = m_keywords.end();
	for (; it != end; ++it) {
		g_free(static_cast<gchar*>(*it));
	}
	m_keywords.clear();
}

void KeywordMap::addKeywords(json_object* strArray)
{
	if (strArray == 0 || !json_object_is_type(strArray, json_type_array))
		return;

	int numItems = json_object_array_length(strArray);
	for (int i=0; i < numItems; i++) {

		json_object* key = json_object_array_get_idx(strArray, i);
		if (json_object_is_type(key, json_type_string)) {
			gchar* newKeyword = g_utf8_strdown(json_object_get_string(key), -1);
			if (newKeyword)
				m_keywords.push_back(newKeyword);
		}
	}
}

bool KeywordMap::hasMatch(const gchar* keyword, bool onlyExact) const
{
	if (!keyword || m_keywords.empty())
		return false;

	size_t keyLen = strlen(keyword);
	std::list<gchar*>::const_iterator it = m_keywords.begin();
	std::list<gchar*>::const_iterator end = m_keywords.end();
	for (; it != end; ++it) {

		const gchar* cur = static_cast<const gchar*>(*it);
		if (g_str_has_prefix(cur, keyword)) {
			if (onlyExact && keyLen != strlen(cur))
				continue;
			return true;
		}
	}
	return false;
}

std::list<std::string> KeywordMap::allKeywords() const
{
	std::list<std::string> rlist;
	for (std::list<gchar*>::const_iterator it = m_keywords.begin();
			it != m_keywords.end();++it)
	{
		if (*it)
		{
			rlist.push_back(std::string(*it));
		}
	}
	return rlist;
}

