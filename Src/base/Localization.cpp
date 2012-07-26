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




#include "Common.h"

#include <pbnjson.hpp>

#include "Localization.h"

#include "Preferences.h"
#include "Settings.h"

#include <QVector>
#include <QDebug>
#include <QFile>

static const char* s_localeFile = "/strings.json";

Localization* Localization::instance()
{
	// Not thread-safe. Make sure to initialize this in the Main before spawning
	// other threads
	static Localization* s_instance = 0;
	if (G_UNLIKELY(s_instance == 0))
		s_instance = new Localization;

	return s_instance;
}

Localization::Localization()
{
	loadLocalizedStrings();
}

Localization::~Localization()
{
}

void Localization::loadLocalizedStrings()
{
	m_localizationMap.clear();

	QVector<std::string> translations;

	std::string locale = Preferences::instance()->locale();
	translations << Settings::LunaSettings()->lunaCustomizationLocalePath + "/" + locale + s_localeFile;
	translations << Settings::LunaSettings()->lunaSystemLocalePath + "/" + locale + s_localeFile;

	pbnjson::JSchema localeSchema = pbnjson::JSchemaFile("/etc/palm/schemas/localization.schema");

	pbnjson::JDomParser parser;
	Q_FOREACH(std::string translation, translations) {
		if (!QFile::exists(qFromUtf8Stl(translation))) {
			//qDebug() << "Failed to find localization" << translation.c_str();
			continue;
		}

		if (!parser.parseFile(translation, localeSchema, JFileOptMMap)) {
			//qWarning() << "Failed to load localization from" << translation.c_str();
			continue;
		}

		pbnjson::JValue localized = parser.getDom();
		pbnjson::JValue::ObjectIterator iter;
		for (iter = localized.begin(); iter != localized.end(); iter++) {
			pbnjson::JValue::KeyValue pair = (*iter);
			std::string key = pair.first.asString();
			std::string value = pair.second.asString();

			if (!m_localizationMap.contains(key))
				m_localizationMap.insert(key, value);
		}
	}
}

std::string Localization::getLocalizedString(const std::string& str) const
{
	LocalizationMap::const_iterator it = m_localizationMap.find(str);
	if (G_UNLIKELY(it == m_localizationMap.end()))
		return str;

	return it.value();
}

std::string LOCALIZED(const std::string& str)
{
	return Localization::instance()->getLocalizedString(str);
}

