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




#ifndef SERVICEDESCRIPTION_H
#define SERVICEDESCRIPTION_H

#include <string>

struct json_object;

class ServiceDescription
{
public:

	ServiceDescription();
	~ServiceDescription();

	static ServiceDescription* fromFile(const std::string& filePath);

	const std::string& id()			const { return m_id; }
	const std::string& jsonString() const { return m_jsonString; }

	// NOTE: it is the callers responsibility to json_object_put the return value
	json_object* toJSON() const;

	bool operator==(const ServiceDescription& cmp) const;
	bool operator!=(const ServiceDescription& cmp) const;

private:
	std::string m_id;
	std::string m_jsonString;
};


#endif /* SERVICEDESCRIPTION_H */
