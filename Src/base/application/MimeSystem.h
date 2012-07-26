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




#ifndef MIMESYSTEM_H_
#define MIMESYSTEM_H_

#include "Common.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

#include "Mutex.h"
#include "CmdResourceHandlers.h"

class MimeSystem
{
public:
	
	class VerbInfo {
	public:
		VerbInfo(const std::string& verb,const std::string& params,const std::string handlerAppId, uint32_t handlerIndex) :
			m_verb(verb) , m_params(params) , m_handlerAppId(handlerAppId) , m_handlerIndex(handlerIndex) , m_valid(true) {}
		VerbInfo() : m_verb("") , m_params("") , m_handlerAppId("") ,  m_handlerIndex(0) , m_valid(false) {}
		VerbInfo(const VerbInfo& c) {
			m_valid = c.m_valid;
			m_handlerAppId = c.m_handlerAppId;
			m_handlerIndex = c.m_handlerIndex;
			m_verb = c.m_verb;
			m_params = c.m_params;
		}

		VerbInfo& operator=(const VerbInfo& c) {
			if (&c == this)
				return *this;

			m_valid = c.m_valid;
			m_handlerAppId = c.m_handlerAppId;
			m_handlerIndex = c.m_handlerIndex;
			m_verb = c.m_verb;
			m_params = c.m_params;
			return *this;
		}

		std::string m_verb;
		std::string m_params;
		std::string m_handlerAppId;
		uint32_t m_handlerIndex;
		bool m_valid;
	
	};
	
public:
	
	static MimeSystem * instance();
	static MimeSystem * instance(const std::string& baseConfigFile);
	static MimeSystem * instance(const std::string& baseConfigFile,const std::string& customizedConfigFile);
	
	std::string			getActiveAppIdForResource(std::string mimeType);
	int					getAllAppIdForResource(std::string mimeType,std::string& r_active,std::vector<std::string>& r_handlerAppIds);
	
	ResourceHandler		getActiveHandlerForResource(std::string mimeType);
	int					getAllHandlersForResource(std::string mimeType,ResourceHandler& r_active,std::vector<ResourceHandler>& r_handlers);
		
	std::string			getActiveAppIdForRedirect(const std::string& url,bool doNotUseRegexpMatch,bool disallowSchemeForms);
	int					getAllAppIdForRedirect(const std::string& url,bool doNotUseRegexpMatch,std::string& r_active,std::vector<std::string>& r_handlerAppIds);
	
	RedirectHandler		getActiveHandlerForRedirect(const std::string& url,bool doNotUseRegexpMatch,bool disallowSchemeForms);
	int					getAllHandlersForRedirect(const std::string& url,bool doNotUseRegexpMatch,RedirectHandler& r_active,std::vector<RedirectHandler>& r_handlers);
		
	std::string			getAppIdByVerbForResource(std::string mimeType,const std::string& verb,std::string& r_params,uint32_t& r_index);
	ResourceHandler		getHandlerByVerbForResource(std::string mimeType,const std::string& verb);
	int					getAllHandlersByVerbForResource(std::string mimeType,const std::string& verb,std::vector<ResourceHandler>& r_handlers);
	int 				getAllAppIdByVerbForResource(std::string mimeType,const std::string& verb,std::vector<VerbInfo>& r_handlers);
	
	std::string			getAppIdByVerbForRedirect(const std::string& url,bool disallowSchemeForms,const std::string& verb,std::string& r_params,uint32_t& r_index);
	RedirectHandler		getHandlerByVerbForRedirect(const std::string& url,bool disallowSchemeForms,const std::string& verb);
	int 				getAllHandlersByVerbForRedirect(const std::string& url,const std::string& verb,std::vector<RedirectHandler>& r_handlers);
	int 				getAllAppIdByVerbForRedirect(const std::string& url,const std::string& verb,std::vector<VerbInfo>& r_handlers);
	
	RedirectHandler		getRedirectHandlerDirect(const uint32_t index);
	ResourceHandler		getResourceHandlerDirect(const uint32_t index);
	
	int 				removeAllForAppId(const std::string& appId);
	int					removeAllForMimeType(std::string mimeType);
	int					removeAllForUrl(const std::string& url);
	
	int					addResourceHandler(std::string& extension,std::string mimeType,bool shouldDownload,const std::string appId,const std::map<std::string,std::string> * pVerbs,bool sysDefault);
	int					addResourceHandler(std::string extension,bool shouldDownload,const std::string appId,const std::map<std::string,std::string> * pVerbs,bool sysDefault);
	int					addRedirectHandler(const std::string& url,const std::string appId,const std::map<std::string,std::string> * pVerbs,bool isSchemeForm,bool sysDefault);
	
	int					addVerbsToResourceHandler(std::string mimeType,const std::string& appId,const std::map<std::string,std::string>& verbs);
	int					addVerbsToRedirectHandler(const std::string& url,const std::string& appId,const std::map<std::string,std::string>& verbs);
	int					addVerbsDirect(uint32_t index,const std::map<std::string,std::string>& verbs);
	
	int 				swapResourceHandler(std::string mimeType, uint32_t index);
	int					swapRedirectHandler(const std::string& url, uint32_t index);
	
	static bool 		getExtensionFromUrl(const std::string& url,std::string& r_extn);
	bool				getMimeTypeByExtension(std::string extension,std::string& r_mimeType);
	
	static uint32_t		assignIndex();
	static uint32_t		getLastAssignedIndex();
	static std::string	makePseudoExtensionFromMime(const std::string& mimeType);
	
	std::string			allTablesAsJsonString();
	std::string			resourceTableAsJsonString();
	json_object *		resourceTableAsJson();	//WARNING: memory allocated; caller must clean
	json_object *		resourceTableAsJsonArray();	//WARNING: memory allocated; caller must clean
	
	std::string			redirectTableAsJsonString();
	json_object *		redirectTableAsJson();	//WARNING: memory allocated; caller must clean
	json_object *		redirectTableAsJsonArray();	//WARNING: memory allocated; caller must clean
	
	std::string			extensionMapAsJsonString();
	json_object *		extensionMapAsJson();	//WARNING: memory allocated; caller must clean
	json_object *		extensionMapAsJsonArray();	//WARNING: memory allocated; caller must clean
		
	bool				saveMimeTable(const std::string& file,std::string& r_err);
	bool				saveMimeTableToActiveFile(std::string& r_err);
	bool				restoreMimeTable(const std::string& file,std::string& r_err);
	bool				restoreMimeTable(json_object * source,std::string& r_err);			//a version of restore that takes a read-in version of the file as a json obj.
	bool				clearMimeTable();
	static void			deleteSavedMimeTable();				
	
	//some utils
	static int extractVerbsFromHandlerEntryJson(struct json_object * jsonHandlerEntry,std::map<std::string,std::string>& r_verbs);
	static int extractVerbsFromHandlerNodeEntryJson(struct json_object * jsonHandlerNodeEntry,std::map<std::string,uint32_t>& r_verbs);
		
private:
	
	MimeSystem();
	MimeSystem(const MimeSystem& c) {}
	MimeSystem& operator=(const MimeSystem& c) { return *this;}
	virtual ~MimeSystem();
	void destroy();
	
	int populateFromJson(struct json_object * jobj);		//useful only for the initial command-resource-handlers.json file reading
	
/// -------------- helper classes --------------------------------------------------------------------------------------
	
	class VerbCacheEntry {
	public:
		
		VerbCacheEntry() : activeIndex(0) , useCount(0) {}
		VerbCacheEntry(uint32_t index) : activeIndex(index) , useCount(1) {}
		VerbCacheEntry(const VerbCacheEntry& c) : activeIndex(c.activeIndex) , useCount(c.useCount) {}
		VerbCacheEntry& operator=(const VerbCacheEntry& c) {
			if (this == &c)
				return *this;
			activeIndex = c.activeIndex;
			useCount = c.useCount;
			return *this;
		}
		uint32_t activeIndex;
		uint32_t useCount;
	};
	
	class RedirectHandlerNode {
	public:
		RedirectHandlerNode(const std::string& urlRe, const std::string& appId , bool schemeForm) : m_redirectHandler(urlRe,appId,schemeForm) {
			m_handlersByIndex[m_redirectHandler.index()] = &m_redirectHandler;
		}
		RedirectHandler	m_redirectHandler;
		std::vector<RedirectHandler *> m_alternates;
	
		std::map<std::string,VerbCacheEntry> m_verbCache;
		std::map<uint32_t,RedirectHandler *> m_handlersByIndex;
		
		int removePrimary();
		int removeAppId(const std::string& appId);
		int swapHandler(uint32_t index);
		
		int removeVerb(const std::string& verb,RedirectHandler& handler);
		int removeAllVerbsOfHandler(RedirectHandler& handler);
		bool isCurrentVerbHandler(const std::string& verb,RedirectHandler& handler);
		bool pickRandomVerbHandler(const std::string& verb,uint32_t& r_chosenIndex);
		bool reassignRandomVerbHandler(const std::string& verb);
		
		bool exists(const std::string& urlRe, const std::string& appId) {
			if (m_redirectHandler.equals(urlRe,appId))
				return true;
			for (std::vector<RedirectHandler *>::iterator it=m_alternates.begin();
					it != m_alternates.end();++it)
			{
				if ((*it)->equals(urlRe,appId))
					return true;
			}
			return false;
		}
		
		virtual ~RedirectHandlerNode();
		
		std::string toJsonString();
		struct json_object * toJson();			//WARNING: memory allocated; caller must clean
		static MimeSystem::RedirectHandlerNode * fromJsonString(const std::string& jsonString);
		static MimeSystem::RedirectHandlerNode * fromJson(struct json_object * jobj);
		
		int fixupVerbCacheTable(struct json_object * jsonHandlerNodeEntry);
	};

	class ResourceHandlerNode {
	public:
		ResourceHandlerNode(const std::string& ext, 
						const std::string& contentType, 
						const std::string& appId, 
						bool stream=false ) : m_resourceHandler(ext,contentType,appId,stream) {
			m_handlersByIndex[m_resourceHandler.index()] = &m_resourceHandler;
		}
		
		ResourceHandler		m_resourceHandler;
		std::vector<ResourceHandler *> m_alternates;
		std::map<std::string,VerbCacheEntry> m_verbCache;
		std::map<uint32_t,ResourceHandler *> m_handlersByIndex;
		
		int removePrimary();
		int removeAppId(const std::string& appId);
		int swapHandler(uint32_t index);
		
		int removeVerb(const std::string& verb,ResourceHandler& handler);
		int removeAllVerbsOfHandler(ResourceHandler& handler);
		bool isCurrentVerbHandler(const std::string& verb,ResourceHandler& handler);
		bool pickRandomVerbHandler(const std::string& verb,uint32_t& r_chosenIndex);
		bool reassignRandomVerbHandler(const std::string& verb);
		
		bool exists(const std::string& extension,const std::string& appId,const std::string& contentType,bool stream) {
			if (m_resourceHandler.match(extension,appId,contentType,stream))
				return true;
			for (std::vector<ResourceHandler *>::iterator it=m_alternates.begin();
			it != m_alternates.end();++it)
			{
				if ((*it)->match(extension,appId,contentType,stream))
					return true;
			}
			return false;
		}
		
		bool exists(const std::string& appId,const std::string& contentType) {
			if (m_resourceHandler.match(appId,contentType))
				return true;
			for (std::vector<ResourceHandler *>::iterator it=m_alternates.begin();
			it != m_alternates.end();++it)
			{
				if ((*it)->match(appId,contentType))
					return true;
			}
			return false;
		}
		
		virtual ~ResourceHandlerNode();
		
		std::string toJsonString();
		struct json_object * toJson();			//WARNING: memory allocated; caller must clean
		static MimeSystem::ResourceHandlerNode * fromJsonString(const std::string& jsonString);
		static MimeSystem::ResourceHandlerNode * fromJson(struct json_object * jobj);
		
		int fixupVerbCacheTable(struct json_object * jsonHandlerNodeEntry);
	};
	
	static void reclaimIndex(uint32_t idx);
	
	static int addVerbs(const std::map<std::string,std::string>& verbs,ResourceHandlerNode& resourceHandlerNode,ResourceHandler& newHandler);
	static int addVerbs(const std::map<std::string,std::string>& verbs,RedirectHandlerNode& redirectHandlerNode,RedirectHandler& newHandler);

public:
	
	//dbg
	bool				dbg_printMimeTables();
	bool				dbg_getResourceTableStrings(std::vector<std::pair<std::string,std::vector<std::string> > >& r_resourceTableStrings);
	bool				dbg_getRedirectTableStrings(std::vector<std::pair<std::string,std::vector<std::string> > >& r_redirectTableStrings);
	void				dbg_printVerbCacheTableForResource(const std::string& mime);
	void 				dbg_printVerbCacheTableForRedirect(const std::string& url);
	void				dbg_printVerbCacheTableForScheme(const std::string& url);
	static void			dbg_printVerbCacheTable(const std::map<std::string,VerbCacheEntry> * p_verbCacheTable);
	static void			dbg_printResourceHandlerNode(const ResourceHandlerNode * p_resourceHandlerNode,int level=0);
	static void			dbg_printRedirectHandlerNode(const RedirectHandlerNode * p_redirectHandlerNode,int level=0);
	
private:

	ResourceHandlerNode *	getResourceHandlerNode(const std::string& mimeType);
	RedirectHandlerNode *	getRedirectHandlerNode(const std::string& url);
	RedirectHandlerNode *	getSchemeHandlerNode(const std::string& url);
	
/// ------------------------------------------- vars -------------------------------------------------------------------
	
	static MimeSystem * s_p_inst;
	
	static Mutex 	s_mutex;
	Mutex 			m_mutex;
	
	std::map<std::string,MimeSystem::ResourceHandlerNode *> m_resourceHandlerMap;
	std::map<std::string,MimeSystem::RedirectHandlerNode *> m_redirectHandlerMap;
	
	std::map<std::string,std::string>						m_extensionToMimeMap;
	static uint32_t 	s_genIndex;
	static uint32_t		s_lastAssignedIndex;
	static std::vector<uint32_t> s_indexRecycler;
	
	typedef std::map<std::string,MimeSystem::ResourceHandlerNode *> ResourceMapType;
	typedef std::map<std::string,MimeSystem::ResourceHandlerNode *>::iterator ResourceMapIterType;
	typedef std::map<std::string,MimeSystem::RedirectHandlerNode *> RedirectMapType;
	typedef std::map<std::string,MimeSystem::RedirectHandlerNode *>::iterator RedirectMapIterType;
	typedef std::map<std::string,MimeSystem::VerbCacheEntry> VerbCacheMapType;
	typedef std::map<std::string,MimeSystem::VerbCacheEntry>::iterator VerbCacheMapIterType;
};

#endif /*MIMESYSTEM_H_*/
