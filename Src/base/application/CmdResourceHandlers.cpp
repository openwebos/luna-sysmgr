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

#include <stdlib.h>
#include <string.h>
#include <string>
#include "CmdResourceHandlers.h"
#include "MimeSystem.h"
#include <cjson/json.h>
#include <cjson/json_util.h>

/**
 * Constructor.
 */
RedirectHandler::RedirectHandler(const std::string& urlRe, const std::string& appId , bool schemeform ) :
	m_urlRe(urlRe), m_appId(appId) , m_valid(true) , m_schemeForm(schemeform) , m_tag("")
{
	m_index = MimeSystem::assignIndex();
	if (!urlRe.empty() && 0 == regcomp(&m_urlReg, urlRe.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
	}
	else {
		::memset(&m_urlReg, 0, sizeof(m_urlReg));
	}
}

RedirectHandler::RedirectHandler(const std::string& urlRe, const std::string& appId , bool schemeform, const std::string& handler_tag) :
	m_urlRe(urlRe), m_appId(appId) , m_valid(true), m_schemeForm(schemeform) , m_tag(handler_tag)
{
	m_index = MimeSystem::assignIndex();
	if (!urlRe.empty() && 0 == regcomp(&m_urlReg, urlRe.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
	}
	else {
		::memset(&m_urlReg, 0, sizeof(m_urlReg));
	}
}

RedirectHandler::RedirectHandler(const RedirectHandler& c) 
{
	m_urlRe = c.m_urlRe;
	m_appId = c.m_appId;
	m_valid = c.m_valid;
	m_tag = c.m_tag;
	m_index = c.m_index;
	m_schemeForm = c.m_schemeForm;
	m_verbs = c.m_verbs;
	
	if (!m_urlRe.empty() && 0 == regcomp(&m_urlReg, m_urlRe.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
	}
	else {
		::memset(&m_urlReg, 0, sizeof(m_urlReg));
	}
	
}
RedirectHandler& RedirectHandler::operator=(const RedirectHandler& c) 
{
	if (this == &c)
		return *this;
	
	if (reValid()) {
		regfree(&m_urlReg);
	}
	
	m_urlRe = c.m_urlRe;
	m_appId = c.m_appId;
	m_valid = c.m_valid;
	m_tag = c.m_tag;
	m_index = c.m_index;
	m_schemeForm = c.m_schemeForm;
	m_verbs = c.m_verbs;
	
	if (!m_urlRe.empty() && 0 == regcomp(&m_urlReg, m_urlRe.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
	}
	else {
		::memset(&m_urlReg, 0, sizeof(m_urlReg));
	}
	
	return *this;
}

RedirectHandler::RedirectHandler() : m_valid(false), m_schemeForm(false), m_index(0)
{
	::memset(&m_urlReg, 0, sizeof(m_urlReg));
}

/**
 * Destructor.
 */
RedirectHandler::~RedirectHandler()
{
	if (reValid()) {
		regfree(&m_urlReg);
	}
}

/**
 * Determine if a given URL string matches the URL regular expression 
 * for this redirect handler.
 */
bool RedirectHandler::matches(const std::string& url) const
{
	return !url.empty() && reValid() && regexec(&m_urlReg, url.c_str(), 0, NULL, 0) == 0;
}

/**
 * Is the URL regular expression for this redirect handler valid?
 */
bool RedirectHandler::reValid() const
{
	return m_urlReg.buffer != NULL;
}

bool RedirectHandler::addVerb(const std::string& verb,const std::string& jsonizedParams)
{
	struct json_object * jobj = json_tokener_parse(jsonizedParams.c_str());
	if ((!jobj) || (is_error(jobj)))
		return false;
	
	m_verbs[verb] = jsonizedParams;
	
	return true;
}

void RedirectHandler::removeVerb(const std::string& verb)
{
	m_verbs.erase(verb);
}

std::string	RedirectHandler::toJsonString()
{
	struct json_object * jobj = toJson();
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}

struct json_object * RedirectHandler::toJson()			//WARNING: memory allocated; caller must clean
{
	struct json_object * jobj = json_object_new_object();
	json_object_object_add(jobj,(char *)"url",json_object_new_string(m_urlRe.c_str()));
	json_object_object_add(jobj,(char *)"appId",json_object_new_string(m_appId.c_str()));
	json_object_object_add(jobj,(char *)"index",json_object_new_int(m_index));
	if (m_tag.size())
		json_object_object_add(jobj,(char *)"tag",json_object_new_string(m_tag.c_str()));
	json_object_object_add(jobj,(char *)"schemeForm",json_object_new_boolean(m_schemeForm));
	if (m_verbs.size()) {
		json_object * jparam = json_object_new_object();
		for (std::map<std::string,std::string>::iterator it = m_verbs.begin();
		it != m_verbs.end();++it)
		{
			json_object_object_add(jparam,(char *)(it->first.c_str()),json_object_new_string(it->second.c_str()));
		}
		json_object_object_add(jobj,(char*)"verbs",jparam);
	}

	return jobj;
}

///--------------------------------------------------------------------------------------------------------------------


ResourceHandler::ResourceHandler( const std::string& ext, const std::string& contentType, 
								const std::string& appId, bool stream) 
: m_fileExt(ext)
, m_contentType(contentType)
, m_appId(appId)
, m_stream(stream)
, m_valid(true)
, m_tag("")
{
	m_index = MimeSystem::assignIndex();
}

ResourceHandler::ResourceHandler( const std::string& ext, 
				const std::string& contentType, 
				const std::string& appId,
				bool stream,
				const std::string& handler_tag)
: m_fileExt(ext)
, m_contentType(contentType)
, m_appId(appId)
, m_stream(stream)
, m_valid(true)
, m_tag(handler_tag)
{
	m_index = MimeSystem::assignIndex();
}

ResourceHandler::ResourceHandler(const ResourceHandler& c) 
{
	m_fileExt = c.m_fileExt;
	m_contentType = c.m_contentType;
	m_appId = c.m_appId;
	m_stream = c.m_stream;
	m_valid = c.m_valid;
	m_tag = c.m_tag;
	m_index = c.m_index;
	m_verbs = c.m_verbs;
	
}

ResourceHandler& ResourceHandler::operator=(const ResourceHandler& c) 
{
	if (this == &c)
		return *this;
	m_fileExt = c.m_fileExt;
	m_contentType = c.m_contentType;
	m_appId = c.m_appId;
	m_stream = c.m_stream;
	m_valid = c.m_valid;
	m_tag = c.m_tag;
	m_index = c.m_index;
	m_verbs = c.m_verbs;
	
	return *this;
}

bool ResourceHandler::addVerb(const std::string& verb,const std::string& jsonizedParams)
{
	struct json_object * jobj = json_tokener_parse(jsonizedParams.c_str());
	if ((!jobj) || (is_error(jobj)))
		return false;
	
	m_verbs[verb] = jsonizedParams;
	
	return true;
}

void ResourceHandler::removeVerb(const std::string& verb)
{
	m_verbs.erase(verb);
}

std::string	ResourceHandler::toJsonString()
{
	struct json_object * jobj = toJson();
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}

struct json_object * ResourceHandler::toJson()			//WARNING: memory allocated; caller must clean
{
	struct json_object * jobj = json_object_new_object();
	json_object_object_add(jobj,(char *)"mime",json_object_new_string(m_contentType.c_str()));
	json_object_object_add(jobj,(char *)"extension",json_object_new_string(m_fileExt.c_str()));
	json_object_object_add(jobj,(char *)"appId",json_object_new_string(m_appId.c_str()));
	json_object_object_add(jobj,(char *)"streamable",json_object_new_boolean(m_stream));
	json_object_object_add(jobj,(char *)"index",json_object_new_int(m_index));
	if (m_tag.size())
		json_object_object_add(jobj,(char *)"tag",json_object_new_string(m_tag.c_str()));
	if (m_verbs.size()) {
		json_object * jparam = json_object_new_object();
		for (std::map<std::string,std::string>::iterator it = m_verbs.begin();
		it != m_verbs.end();++it)
		{
			json_object_object_add(jparam,(char *)(it->first.c_str()),json_object_new_string(it->second.c_str()));
		}
		json_object_object_add(jobj,(char*)"verbs",jparam);
	}
	
	return jobj;
}
