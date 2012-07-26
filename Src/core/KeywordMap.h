/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef KEYWORDMAP_H
#define KEYWORDMAP_H

#include "Common.h"

/*
 * class for doing keyword matching. Used by ApplicationDescription and ApplicationManager[Service]; 
 * simple container for now for the std::map class but designed as a container to allow swapping in better keyword
 * matching implementations later 
 * 
 */

#include <string>
#include <list>

#include <glib.h>

struct json_object;

class KeywordMap 
{
public:
	
	KeywordMap();
	~KeywordMap();
	
	void addKeywords(json_object* strArray);

	// NOTE: assumes keyword has already been lowercase'd via g_utf8_strdown
	bool hasMatch(const gchar* keyword, bool onlyExact) const;
	
	std::list<std::string> allKeywords() const;

private:
	
	KeywordMap(const KeywordMap& c);
	KeywordMap& operator=(const KeywordMap& c);

	std::list<gchar*> m_keywords;
};

#endif /*  KEYWORDMAP_H  */
