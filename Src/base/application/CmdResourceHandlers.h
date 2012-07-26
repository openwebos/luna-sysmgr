/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef __CmdResourceHandlers_h__
#define __CmdResourceHandlers_h__


#include "Common.h"

#include <regex.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

/**
 * Maps a URL regular expression to the application id that can handle a matching
 * URL.
 */

class RedirectHandlerNode;
class ResourceHandlerNode;

class RedirectHandler
{
	public:
		RedirectHandler(const std::string& urlRe, const std::string& appId, bool schemeform );
		RedirectHandler(const std::string& urlRe, const std::string& appId, bool schemeform, const std::string& handler_tag);
		virtual ~RedirectHandler();
		RedirectHandler();
		
		const std::string& urlRe() const { return m_urlRe; }
		const std::string& appId() const { return m_appId; }
		bool matches(const std::string& url) const;
		bool reValid() const;
		
		bool addVerb(const std::string& verb,const std::string& jsonizedParams);
		void removeVerb(const std::string& verb);
		
		bool operator==(const RedirectHandler& c) const 
		{
			return ((m_urlRe == c.m_urlRe) && (m_appId == c.m_appId));
		}
		bool operator!=(const RedirectHandler& c) const 
		{
			return ((m_urlRe != c.m_urlRe) || (m_appId != c.m_appId));
		}
		bool equals(const std::string& url,const std::string& appId)
		{
			return ((m_urlRe == url) && (m_appId == appId));
		}
		
		RedirectHandler(const RedirectHandler& c);
		RedirectHandler& operator=(const RedirectHandler& rhs);
		
		friend class RedirectHandlerNode;
		
		bool valid() const	{ return m_valid;}
		void markInvalid() { m_valid = false; }
		bool isSchemeForm() const { return m_schemeForm;}
		
		const std::string& tag() const { return m_tag; }
		void setTag(const std::string& newtag) { m_tag = newtag;}
		uint32_t index() { return m_index; }
		uint32_t setIndex(uint32_t newindex) { uint32_t t = m_index; m_index = newindex; return t;}

		std::string		toJsonString();
		struct json_object * toJson();			//WARNING: memory allocated; caller must clean
		
		const std::map<std::string,std::string>& verbs() { return m_verbs;}
		
	private:
		
		std::string m_urlRe; ///< The URL regular expression
		std::string m_appId;
		regex_t m_urlReg; ///< The compiled URL regular expression
		bool	m_valid;
		bool	m_schemeForm;
		std::string m_tag;
		uint32_t m_index;
		std::map<std::string,std::string>	m_verbs;				// < Verb , json-ized string of parameters >

};

/**
 * Associate a resource file extension and MIME type to the application that can
 * open that resource type.
 */
class ResourceHandler
{
	public:
		ResourceHandler( const std::string& ext, 
				const std::string& contentType, 
				const std::string& appId, 
				bool stream=false );
		ResourceHandler( const std::string& ext, 
				const std::string& contentType, 
				const std::string& appId,
				bool stream,
				const std::string& handler_tag);
		
		~ResourceHandler() { }
		ResourceHandler() : m_stream(false), m_valid(false), m_index(0) {}
		
		ResourceHandler(const ResourceHandler& c);
		ResourceHandler& operator=(const ResourceHandler& c);

		bool operator==(const ResourceHandler& c) const
		{
			return ((m_fileExt == c.m_fileExt) && (m_contentType == c.m_contentType) && (m_appId == c.m_appId) && (m_stream == c.m_stream));
		}
		
		bool operator!=(const ResourceHandler& c) const
		{
			return ((m_fileExt != c.m_fileExt) || (m_contentType != c.m_contentType) || (m_appId != c.m_appId) || (m_stream != c.m_stream));
		}

		bool match(const std::string& extension,const std::string& appId,const std::string& mimeType,bool stream) const
		{
			return ((m_fileExt == extension) && (m_contentType == mimeType) && (m_appId == appId) && (m_stream == stream));
		}
		bool match(const std::string& extension,const std::string& appId,const std::string& mimeType) const
		{
			return ((m_fileExt == extension) && (m_contentType == mimeType) && (m_appId == appId));		
		}
		bool match(const std::string& appId,const std::string& mimeType) const
		{
			return ((m_contentType == mimeType) && (m_appId == appId));		
		}
		
		const std::string& appId() const { return m_appId; }
		const std::string& fileExt() const { return m_fileExt; }
		const std::string& contentType() const { return m_contentType; }
		const std::string& tag() const { return m_tag; }
		void setTag(const std::string& newtag) { m_tag = newtag;}
		uint32_t index() { return m_index; }
		uint32_t setIndex(uint32_t newindex) { uint32_t t = m_index; m_index = newindex; return t;}
		bool stream() const { return m_stream; }

		friend class ResourceHandlerNode;
		
		bool valid()	{ return m_valid;}
		void markInvalid() { m_valid = false; }
		bool addVerb(const std::string& verb,const std::string& jsonizedParams);
		void removeVerb(const std::string& verb);
				
		std::string		toJsonString();
		struct json_object * toJson();			//WARNING: memory allocated; caller must clean
		
		const std::map<std::string,std::string>& verbs() { return m_verbs;}
		
	private:
		std::string m_fileExt;
		std::string m_contentType;
		std::string m_appId;
		bool m_stream;
		bool m_valid;
		std::string m_tag;
		uint32_t	m_index;
		std::map<std::string,std::string>	m_verbs;				// < Verb , json-ized string of parameters >
};

#endif
