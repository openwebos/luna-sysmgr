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




#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "Common.h"

#include "QtUtils.h"
#include <QHash>

class Localization
{
public:

	static Localization* instance();

	std::string getLocalizedString(const std::string& str) const;

	// call if locale changed. Warning! Cached translations will of course not be updated automatically...
	void loadLocalizedStrings();

private:

	Localization();
	~Localization();

private:
	typedef QHash<std::string, std::string> LocalizationMap;
	LocalizationMap m_localizationMap;
};

std::string LOCALIZED(const std::string& str);

#endif /* LOCALIZATION_H */
