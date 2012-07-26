/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef STATICMATCHLIST_H_
#define STATICMATCHLIST_H_

#include "filterlist.h"
#include <QSet>
#include <QString>

class QStringList;
class QString;
namespace DimensionsSystemInterface
{

namespace StaticMatchListType
{
	enum Enum
	{
		INVALID,
		Blacklist
	};
}

class StaticMatchList : public FilterList
{
public:
	StaticMatchList(StaticMatchListType::Enum type);
	StaticMatchList(StaticMatchListType::Enum type, const QStringList& ids);
	virtual ~StaticMatchList();

	virtual void add(const QString& id);
	virtual void remove(const QString& id);
	virtual void clear();

	virtual bool save(const QString& filepath);
	//if successfull, load() deletes  the set and populates it from the file
	virtual bool load(const QString& filepath);

	//continued plumbing from FilterList
	virtual bool allow(const QString& id) = 0;
	virtual bool deny(const QString& id) = 0;

	static QString SaveTagKey_Type;
	static QString SaveTagKey_Id;

protected:

	virtual bool match(const QString& id);
	StaticMatchListType::Enum m_type;
	QSet<QString> m_ids;
};

}

#endif /* STATICMATCHLIST_H_ */
