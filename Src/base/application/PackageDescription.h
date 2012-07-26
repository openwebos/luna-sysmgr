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




#ifndef PACKAGEDESCRIPTION_H
#define PACKAGEDESCRIPTION_H

#include <string>
#include <stdint.h>
#include <vector>
#include <cjson/json.h>

struct json_object;

class ApplicationDescription;

class PackageDescription
{
public:

	PackageDescription();
	~PackageDescription();

	static PackageDescription* fromFile(const std::string& filePath, const std::string& folderPath);
	static PackageDescription* fromJson(json_object* root, const std::string& folderPath);
	static PackageDescription* fromApplicationDescription(ApplicationDescription* appDesc);

	const std::string& id()         				const { return m_id; }
	const std::string& version()    				const { return m_version; }
	const std::string& folderPath() 				const { return m_folderPath; }
	uint64_t packageSize() 							const {return m_packageSize;}
	void setPackageSize(uint64_t s) 				{ m_packageSize = s;}
	uint32_t blockSize() 							const { return m_fsBlockSize; }
	void setBlockSize(uint32_t s) 					{ m_fsBlockSize = s;}
	bool isOldStyle() 								const { return m_isOldStyle; }
	const std::vector<std::string>& appIds() 		const { return m_appIds; }
	const std::vector<std::string>& serviceIds() 	const { return m_serviceIds; }
	const std::vector<std::string>& accountIds() 	const { return m_accountIds; }
	const std::string& jsonString() 				const { return m_jsonString; }

	// NOTE: it is the callers responsibility to json_object_put the return value
	json_object* toJSON() const;

	bool operator==(const PackageDescription& cmp) const;
	bool operator!=(const PackageDescription& cmp) const;

private:

	std::string            		m_id;
	std::string            		m_version;
	std::string            		m_folderPath;
	uint64_t					m_packageSize;
	uint32_t					m_fsBlockSize;		/// the blocksize with which m_appSize was calculated; this is useful to have here in case the bsize changes so a recalc can be trigerred...
	bool						m_isOldStyle;
	std::vector<std::string> 	m_appIds;
	std::vector<std::string> 	m_serviceIds;
	std::vector<std::string> 	m_accountIds;
	std::string 				m_jsonString;
};


#endif /* PACKAGEDESCRIPTION_H */
