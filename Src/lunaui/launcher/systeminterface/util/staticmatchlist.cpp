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




#include "staticmatchlist.h"
#include <QStringList>
#include <QString>

#include "safefileops.h"


#include <QDebug>

namespace DimensionsSystemInterface
{

QString StaticMatchList::SaveTagKey_Type = QString("type");
QString StaticMatchList::SaveTagKey_Id = QString("id");
///public:

StaticMatchList::StaticMatchList(StaticMatchListType::Enum type)
: m_type(type)
{

}

StaticMatchList::StaticMatchList(StaticMatchListType::Enum type, const QStringList& ids)
: m_type(type)
{
	m_ids = ids.toSet();
}

//virtual
StaticMatchList::~StaticMatchList()
{

}

//virtual
void StaticMatchList::add(const QString& id)
{
	m_ids.insert(id);
}

//virtual
void StaticMatchList::remove(const QString& id)
{
	m_ids.remove(id);
}

//virtual
void StaticMatchList::clear()
{
	m_ids.clear();
}

//virtual
bool StaticMatchList::save(const QString& filepath)
{
	SafeFileOperator safesave(SafeFileOperator::Write,filepath,QSettings::IniFormat);
	QSettings& saved = safesave.safeSettings();
	if (saved.status() != QSettings::NoError)
	{
		//problem with the file op
		return false;
	}

	saved.beginGroup("header");
	saved.setValue(StaticMatchList::SaveTagKey_Type,m_type);
	saved.endGroup();

	saved.beginWriteArray("list");
	int idx=0;
	for (QSet<QString>::iterator it = m_ids.begin();
			it != m_ids.end();++it,++idx)
	{
		saved.setArrayIndex(idx);
		saved.setValue(StaticMatchList::SaveTagKey_Id,*it);
	}
	saved.endArray();

	return true;
}

//virtual
bool StaticMatchList::load(const QString& filepath)
{
	qDebug() << "Trying to load: " << filepath;
	SafeFileOperator safesave(SafeFileOperator::Read,filepath,QSettings::IniFormat);
	QSettings& saved = safesave.safeSettings();
	if (saved.status() != QSettings::NoError)
	{
		//problem with the file op
		return false;
	}

	int numIds = saved.beginReadArray("list");
	if (numIds == 0)
	{
		return false;
	}

	m_ids.clear();
	for (int i = 0;i < numIds;++i)
	{
		saved.setArrayIndex(i);
		m_ids.insert(saved.value(StaticMatchList::SaveTagKey_Id).toString());
	}
	return true;
}

///protected:

//virtual
bool StaticMatchList::match(const QString& id)
{
	return m_ids.contains(id);
}

} //end namespace
