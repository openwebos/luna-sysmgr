/* @@@LICENSE
*
*      Copyright (c) 2011-2013 LG Electronics, Inc.
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




#include "blacklist.h"
#include "stringtranslator.h"
#include "operationalsettings.h"

namespace DimensionsSystemInterface
{

Blacklist::Blacklist()
: StaticMatchList(StaticMatchListType::Blacklist)
{
}

Blacklist::Blacklist(const QStringList& ids)
: StaticMatchList(StaticMatchListType::Blacklist,ids)
{
}

//virtual
Blacklist::~Blacklist()
{
}

//virtual
bool Blacklist::allow(const QString& id)
{
	return (!match(id));
}
//virtual
bool Blacklist::deny(const QString& id)
{
	return (match(id));
}

//virtual
bool Blacklist::loadDefault()
{
	return load(OperationalSettings::settings()->appBlacklistFilepath);
}

} //end namespace


