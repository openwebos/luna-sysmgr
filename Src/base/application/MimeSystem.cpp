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
#include <stdio.h>
#include <unistd.h>

#include <cjson/json.h>
#include <cjson/json_util.h>
#include <glib.h>
#include "MimeSystem.h"
#include "MutexLocker.h"
#include <algorithm>
#include "Utils.h"
#include "Settings.h"

#include <QUrl>

MimeSystem * MimeSystem::s_p_inst = NULL;
uint32_t 	MimeSystem::s_genIndex = 1;
uint32_t MimeSystem::s_lastAssignedIndex = 0;
std::vector<uint32_t> MimeSystem::s_indexRecycler;
Mutex 		MimeSystem::s_mutex;

// ---------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------- public ----------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

#define RC_HANDLERNODE_REMOVEPRIMARY_REMOVENODE		1
#define RC_HANDLERNODE_REMOVEPRIMARY_OK				2

int MimeSystem::RedirectHandlerNode::removePrimary()
{
	//mark it invalid
	m_redirectHandler.markInvalid();
	//remove all verbs of this primary handler
	removeAllVerbsOfHandler(m_redirectHandler);
	//check to see if the alternates vector has at least 1 entry
	if (m_alternates.size() == 0) {
		//no...the removal of the primary handler means the removal of the whole node
		return RC_HANDLERNODE_REMOVEPRIMARY_REMOVENODE;
	}
	//else, remove from the front of the alternates, and copy it into the primary
	RedirectHandler * tmp = *(m_alternates.begin());
	m_alternates.erase(m_alternates.begin());
	MimeSystem::reclaimIndex(m_redirectHandler.index());
	m_handlersByIndex.erase(m_redirectHandler.index());
	m_redirectHandler = *tmp;
	m_handlersByIndex[m_redirectHandler.index()] = &m_redirectHandler;
	delete tmp;
	return RC_HANDLERNODE_REMOVEPRIMARY_OK;
}

#define RC_HANDLERNODE_REMOVEAPPID_REMOVENODE	1
#define RC_HANDLERNODE_REMOVEAPPID_OK			2
int MimeSystem::RedirectHandlerNode::removeAppId(const std::string& appId)
{
	std::vector<RedirectHandler *>::iterator it = m_alternates.begin();
	while (it != m_alternates.end()) {
		RedirectHandler * tmp = *it;
		if (tmp->appId() == appId) {
			it = m_alternates.erase(it);
			//mark it invalid
			tmp->markInvalid();
			//remove all of its verbs
			removeAllVerbsOfHandler(*tmp);
			//remove from the handlersByIndex map
			m_handlersByIndex.erase(tmp->index());
			MimeSystem::reclaimIndex(tmp->index());
			delete tmp;
		}
		else
			++it;
	}
	
	//check the primary
	if (m_redirectHandler.appId() == appId)
		if (removePrimary() == RC_HANDLERNODE_REMOVEPRIMARY_REMOVENODE)
			return RC_HANDLERNODE_REMOVEAPPID_REMOVENODE;
	return RC_HANDLERNODE_REMOVEAPPID_OK;
}

#define RC_HANDLERNODE_REMOVEVERB_NOSUCHVERB		0
#define RC_HANDLERNODE_REMOVEVERB_OK				1
#define RC_HANDLERNODE_REMOVEVERB_REMOVEDVCENTRY	2

int MimeSystem::RedirectHandlerNode::removeVerb(const std::string& verb,RedirectHandler& handler)
{
	//get the verbcacheentry from the map
	std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.find(verb);
	if (it == m_verbCache.end())
		return RC_HANDLERNODE_REMOVEVERB_NOSUCHVERB;
	
	int rc = RC_HANDLERNODE_REMOVEVERB_OK;
	if (it->second.useCount > 0)		//error safety check: in case useCount became 0 through external manipulation (this would be bad)
		--(it->second.useCount);
	
	if (it->second.useCount == 0) {
		//remove the entry completely from the map
		m_verbCache.erase(it);
		rc = RC_HANDLERNODE_REMOVEVERB_REMOVEDVCENTRY;		//overwrite rcode to note that the last user of this verb was removed
	}
	else {
		//if the handler passed in is the current active verb handler, then reassign.
		// This should always succeed, because there is guaranteed to be at least on more user (in this conditional branch, useCount >=1)
		if (it->second.activeIndex == handler.index())
			reassignRandomVerbHandler(verb);
	}
	
	return rc;
}

int MimeSystem::RedirectHandlerNode::removeAllVerbsOfHandler(RedirectHandler& handler)
{
	int rc=0;
	for (std::map<std::string,std::string>::const_iterator it =  handler.verbs().begin();
			it != handler.verbs().end();++it) 
	{		
		if (removeVerb(it->first,handler))
			++rc;
	}
	
	return rc;
}

bool MimeSystem::RedirectHandlerNode::isCurrentVerbHandler(const std::string& verb,RedirectHandler& handler)
{
	std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.find(verb);
	if (it == m_verbCache.end())
		return false;		//no such verb
	
	return (it->second.activeIndex == handler.index());
}

bool MimeSystem::RedirectHandlerNode::pickRandomVerbHandler(const std::string& verb,uint32_t& r_chosenIndex)
{
	if (m_redirectHandler.valid()) {
		if (m_redirectHandler.verbs().find(verb) != m_redirectHandler.verbs().end()) {
			r_chosenIndex = m_redirectHandler.index();
			return true;
		}
	}
	
	//else search the alternates
	for (std::vector<RedirectHandler *>::iterator it = m_alternates.begin();
			it != m_alternates.end();++it)
	{
		if ((*it)->valid() == false)
			continue;
		if ((*it)->verbs().find(verb) != (*it)->verbs().end()) {
			r_chosenIndex = (*it)->index();
			return true;
		}
	}
	
	return false;
}

bool MimeSystem::RedirectHandlerNode::reassignRandomVerbHandler(const std::string& verb)
{
	uint32_t newHandlerIndex;
	if (!pickRandomVerbHandler(verb,newHandlerIndex))
		return false;
	
	std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.find(verb);
	if (it == m_verbCache.end())
		return false;
	
	it->second.activeIndex = newHandlerIndex;
	return true;
}

int MimeSystem::RedirectHandlerNode::swapHandler(uint32_t index)
{
	std::vector<RedirectHandler *>::iterator it;
	RedirectHandler * p_foundHandler = NULL;
	for (it = m_alternates.begin();it != m_alternates.end();++it) 
	{
		if ((*it)->index() == index)
		{
			p_foundHandler = *it;
			break;
		}
	}
	if (!p_foundHandler)
		return 0;	//not found
	
	RedirectHandler * tmp = new RedirectHandler(m_redirectHandler);
	m_redirectHandler = *p_foundHandler;
	m_alternates.erase(it);
	delete (p_foundHandler);
	m_alternates.push_back(tmp);
	
	//reassign pointers to handlersByIndex map
	m_handlersByIndex[m_redirectHandler.index()] = &m_redirectHandler;
	m_handlersByIndex[tmp->index()] = tmp;
		
	return 1;
}

MimeSystem::RedirectHandlerNode::~RedirectHandlerNode()
{
	//clear out all entries
	for (std::vector<RedirectHandler *>::iterator it = m_alternates.begin();
			it != m_alternates.end();++it) {
		MimeSystem::reclaimIndex((*it)->index());
		delete ((*it));
	}
}

std::string MimeSystem::RedirectHandlerNode::toJsonString()
{
	struct json_object * jobj = toJson();
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}

struct json_object * MimeSystem::RedirectHandlerNode::toJson() 		//WARNING: memory allocated; caller must clean
{
	struct json_object * jobj = json_object_new_object();
	json_object_object_add(jobj,(char *)"primary",m_redirectHandler.toJson());
	if (m_alternates.size()) {
		struct json_object * jarray = json_object_new_array();
		for (std::vector<RedirectHandler *>::iterator it = m_alternates.begin();it != m_alternates.end();++it)
			json_object_array_add(jarray,(*it)->toJson());
		json_object_object_add(jobj,(char *)"alternates",jarray);
	}
	if (m_verbCache.size()) {
		struct json_object * jarray = json_object_new_array();
		for (std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.begin();it != m_verbCache.end();++it) {
			struct json_object * innerVerbObject = json_object_new_object();
			json_object_object_add(innerVerbObject,(char *)"verb",json_object_new_string(it->first.c_str()));
			json_object_object_add(innerVerbObject,(char *)"index",json_object_new_int(it->second.activeIndex));
			json_object_array_add(jarray,innerVerbObject);
		}
		json_object_object_add(jobj,(char *)"verbs",jarray);
	}

	return jobj;
}

//static 
MimeSystem::RedirectHandlerNode * MimeSystem::RedirectHandlerNode::fromJsonString(const std::string& jsonString)  //WARNING: memory allocated (RedirectHandlerNode object)
{
	struct json_object * root = json_tokener_parse( jsonString.c_str() );
	if (!root || is_error(root)) {
		return NULL;
	}
	RedirectHandlerNode * ptr = fromJson(root);
	json_object_put(root);
	return ptr;
}

//static 
MimeSystem::RedirectHandlerNode * MimeSystem::RedirectHandlerNode::fromJson(struct json_object * jobj)		//WARNING: memory allocated (RedirectHandlerNode object)
{
	
	/*
	 * {"primary":{"url":"^et:","appId":"com.palm.app.phone","index":22,"tag":"system-default","schemeForm":true, "verbs":{"play": "{}"}},
	 * "alternates":[
	 * 			{"url":"^et:","appId":"com.palm.app.et","index":24,"schemeForm":true , "verbs":{"play": "{}","run": "{\"param\":\"value\"}"}} }
	 * 				],
	 * "verbs":[
	 * 		{"verb":"play" , "index":67}
	 * 		   ]
	 * }
	 * 
	 */
	
	if ((jobj == NULL) || (is_error(jobj)))
		return NULL;

	struct json_object * primary_jobj = JsonGetObject(jobj,"primary");
	if (primary_jobj == NULL)
		return NULL;
	std::string url,appId,tag;
	uint32_t index;
	bool schemeForm;
	struct json_object * tmp = 0;
	if (extractFromJson(primary_jobj,"url",url) == false)
		return NULL;
	if (extractFromJson(primary_jobj,"appId",appId) == false)
		return NULL;
	if ((tmp = JsonGetObject(primary_jobj,"index")) == NULL)
		return NULL;
	index = json_object_get_int(tmp);
	if ((tmp = JsonGetObject(primary_jobj,"schemeForm")) == NULL)
		return NULL;
	schemeForm = json_object_get_boolean(tmp);
	if (extractFromJson(primary_jobj,"tag",tag) == false) {
		tag = "";
	}

	//create!
	RedirectHandlerNode * p_rhn;
	p_rhn = new RedirectHandlerNode(url,appId,schemeForm);
	p_rhn->m_redirectHandler.setTag(tag);
	
	//extract verbs
	std::map<std::string,std::string> verbs;

	if (MimeSystem::extractVerbsFromHandlerEntryJson(primary_jobj,verbs) > 0) 
	{
		//go through the verbs and add them
		for (std::map<std::string,std::string>::iterator it = verbs.begin();
		it != verbs.end();++it) 
		{
			MimeSystem::addVerbs(verbs,*p_rhn,p_rhn->m_redirectHandler);
		}
	}
		
	struct json_object * alternate_jobj = JsonGetObject(jobj,"alternates");
	if (alternate_jobj == NULL) {
		p_rhn->fixupVerbCacheTable(jobj);
		return p_rhn;		//there are no alternate handlers for this
	}
	
	if (json_object_is_type(alternate_jobj, json_type_array) == false) {
		p_rhn->fixupVerbCacheTable(jobj);
		return p_rhn;
	}

	for (int i=0; i<json_object_array_length(alternate_jobj); i++) {
		json_object* e = json_object_array_get_idx(alternate_jobj, i);
		if (extractFromJson(e,"url",url) == false)
			return NULL;
		if (extractFromJson(e,"appId",appId) == false)
			return NULL;
		if ((tmp = JsonGetObject(e,"index")) == NULL)
			return NULL;
		index = json_object_get_int(tmp);
		if ((tmp = JsonGetObject(e,"schemeForm")) == NULL)
			return NULL;
		schemeForm = json_object_get_boolean(tmp);
		if (extractFromJson(e,"tag",tag) == false) {
			tag = "";
		}
		
		RedirectHandler * p_newHandler = new RedirectHandler(url,appId,schemeForm,tag);
		p_rhn->m_handlersByIndex[p_newHandler->index()] = p_newHandler;
		p_rhn->m_alternates.push_back(p_newHandler);
		
		//extract the handler's verbs and add them
		verbs.clear();
		if (MimeSystem::extractVerbsFromHandlerEntryJson(primary_jobj,verbs) > 0) 
		{
			//go through the verbs and add them
			for (std::map<std::string,std::string>::iterator it = verbs.begin();
			it != verbs.end();++it) 
			{
				MimeSystem::addVerbs(verbs,*p_rhn,*p_newHandler);
			}
		}
				
	}

	return p_rhn;
}
	
int MimeSystem::RedirectHandlerNode::fixupVerbCacheTable(struct json_object * jsonHandlerNodeEntry)
{
	std::map<std::string,uint32_t> verbs;
	if (MimeSystem::extractVerbsFromHandlerNodeEntryJson(jsonHandlerNodeEntry,verbs) == 0)
		return 0;

	int rc=0;
	for (std::map<std::string,uint32_t>::iterator it = verbs.begin();
	it != verbs.end();++it) 
	{
		//locate the VerbCacheEntry correspoding to the key in the verb iterator
		std::map<std::string,VerbCacheEntry>::iterator search_it = m_verbCache.find(it->first);
		if (search_it == m_verbCache.end()) {
			//oops, didn't find it. This is a pretty serious error (means that the addition of verbs failed and/or the VCE table in the json entry is corrupt)
			//try to recover by ignoring it
			continue;
		}
		search_it->second.activeIndex = it->second;
		++rc;
	}

	return rc;
}

int MimeSystem::ResourceHandlerNode::removePrimary()
{
	//mark it invalid
	m_resourceHandler.markInvalid();
	//remove all of its verbs
	removeAllVerbsOfHandler(m_resourceHandler);
				
	//check to see if the alternates vector has at least 1 entry
	if (m_alternates.size() == 0) {
		//no...the removal of the primary handler means the removal of the whole node
		return RC_HANDLERNODE_REMOVEPRIMARY_REMOVENODE;
	}
	//else, remove from the front of the alternates, and copy it into the primary
	ResourceHandler * tmp = *(m_alternates.begin());
	m_alternates.erase(m_alternates.begin());
	MimeSystem::reclaimIndex(m_resourceHandler.index());
	m_handlersByIndex.erase(m_resourceHandler.index());
	m_resourceHandler = *tmp;
	m_handlersByIndex[m_resourceHandler.index()] = &m_resourceHandler;
	delete tmp;
	return RC_HANDLERNODE_REMOVEPRIMARY_OK;
}

int MimeSystem::ResourceHandlerNode::removeAppId(const std::string& appId)
{
	std::vector<ResourceHandler *>::iterator it = m_alternates.begin();
	while (it != m_alternates.end()) {
		ResourceHandler * tmp = *it;
		if (tmp->appId() == appId) {
			it = m_alternates.erase(it);
			//mark it invalid
			tmp->markInvalid();
			//remove all of its verbs
			removeAllVerbsOfHandler(*tmp);
			
			//remove from handlersByIndex map
			m_handlersByIndex.erase(tmp->index());
			MimeSystem::reclaimIndex(tmp->index());
			delete tmp;
		}
		else
			++it;
	}
	
	//check the primary
	if (m_resourceHandler.appId() == appId)
		if (removePrimary() == RC_HANDLERNODE_REMOVEPRIMARY_REMOVENODE)
			return RC_HANDLERNODE_REMOVEAPPID_REMOVENODE;
	return RC_HANDLERNODE_REMOVEAPPID_OK;
}

int MimeSystem::ResourceHandlerNode::swapHandler(uint32_t index)
{
	std::vector<ResourceHandler *>::iterator it;
	ResourceHandler * p_foundHandler = NULL;
	for (it = m_alternates.begin();it != m_alternates.end();++it) 
	{
		if ((*it)->index() == index)
		{
			p_foundHandler = *it;
			break;
		}
	}
	if (!p_foundHandler)
		return 0;	//not found
	
	ResourceHandler * tmp = new ResourceHandler(m_resourceHandler);
	m_resourceHandler = *p_foundHandler;
	m_alternates.erase(it);
	delete (p_foundHandler);
	m_alternates.push_back(tmp);
	
	//reassign pointers to handlersByIndex map
	m_handlersByIndex[m_resourceHandler.index()] = &m_resourceHandler;
	m_handlersByIndex[tmp->index()] = tmp;
	
	return 1;
}

int MimeSystem::ResourceHandlerNode::removeVerb(const std::string& verb,ResourceHandler& handler)
{
	//get the verbcacheentry from the map
	std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.find(verb);
	if (it == m_verbCache.end())
		return RC_HANDLERNODE_REMOVEVERB_NOSUCHVERB;

	int rc = RC_HANDLERNODE_REMOVEVERB_OK;
	if (it->second.useCount > 0)		//error safety check: in case useCount became 0 through external manipulation (this would be bad)
		--(it->second.useCount);

	if (it->second.useCount == 0) {
		//remove the entry completely from the map
		m_verbCache.erase(it);
		rc = RC_HANDLERNODE_REMOVEVERB_REMOVEDVCENTRY;		//overwrite rcode to note that the last user of this verb was removed
	}
	else {
		//if the handler passed in is the current active verb handler, then reassign.
		// This should always succeed, because there is guaranteed to be at least on more user (in this conditional branch, useCount >=1)
		if (it->second.activeIndex == handler.index())
			reassignRandomVerbHandler(verb);
	}
	return rc;
}

int MimeSystem::ResourceHandlerNode::removeAllVerbsOfHandler(ResourceHandler& handler)
{
	int rc=0;
	for (std::map<std::string,std::string>::const_iterator it =  handler.verbs().begin();
			it != handler.verbs().end();++it) 
	{
		if (removeVerb(it->first,handler))
			++rc;
	}
	return rc;
}

bool MimeSystem::ResourceHandlerNode::isCurrentVerbHandler(const std::string& verb,ResourceHandler& handler)
{
	std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.find(verb);
	if (it == m_verbCache.end())
		return false;		//no such verb
	
	return (it->second.activeIndex == handler.index());
}

bool MimeSystem::ResourceHandlerNode::pickRandomVerbHandler(const std::string& verb,uint32_t& r_chosenIndex)
{
	if (m_resourceHandler.valid()) {
		if (m_resourceHandler.verbs().find(verb) != m_resourceHandler.verbs().end()) {
			r_chosenIndex = m_resourceHandler.index();
			return true;
		}
	}
	
	//else search the alternates
	for (std::vector<ResourceHandler *>::iterator it = m_alternates.begin();
			it != m_alternates.end();++it)
	{
		if ((*it)->valid() == false)
			continue;
		if ((*it)->verbs().find(verb) != (*it)->verbs().end()) {
			r_chosenIndex = (*it)->index();
			return true;
		}
	}
	
	return false;
}

bool MimeSystem::ResourceHandlerNode::reassignRandomVerbHandler(const std::string& verb)
{
	uint32_t newHandlerIndex;
	if (!pickRandomVerbHandler(verb,newHandlerIndex))
		return false;
	
	std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.find(verb);
	if (it == m_verbCache.end())
		return false;
	
	it->second.activeIndex = newHandlerIndex;
	return true;
}

MimeSystem::ResourceHandlerNode::~ResourceHandlerNode()
{
	//clear out all entries
	for (std::vector<ResourceHandler *>::iterator it = m_alternates.begin();
			it != m_alternates.end();++it) {
		MimeSystem::reclaimIndex((*it)->index());
		delete ((*it));
	}
}

std::string MimeSystem::ResourceHandlerNode::toJsonString()
{
	struct json_object * jobj = toJson();
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}

struct json_object * MimeSystem::ResourceHandlerNode::toJson() 		//WARNING: memory allocated; caller must clean
{
	struct json_object * jobj = json_object_new_object();
	json_object_object_add(jobj,(char *)"primary",m_resourceHandler.toJson());
	if (m_alternates.size()) {
		struct json_object * jarray = json_object_new_array();
		for (std::vector<ResourceHandler *>::iterator it = m_alternates.begin();it != m_alternates.end();++it)
			json_object_array_add(jarray,(*it)->toJson());
		json_object_object_add(jobj,(char *)"alternates",jarray);
	}
	if (m_verbCache.size()) {
		struct json_object * jarray = json_object_new_array();
		for (std::map<std::string,VerbCacheEntry>::iterator it = m_verbCache.begin();it != m_verbCache.end();++it) {
			json_object * innerVerbObject = json_object_new_object();
			json_object_object_add(innerVerbObject,(char *)"verb",json_object_new_string(it->first.c_str()));
			json_object_object_add(innerVerbObject,(char *)"index",json_object_new_int(it->second.activeIndex));
			json_object_array_add(jarray,innerVerbObject);
		}
		json_object_object_add(jobj,(char *)"verbs",jarray);
	}

	return jobj;
}

//static 
MimeSystem::ResourceHandlerNode * MimeSystem::ResourceHandlerNode::fromJsonString(const std::string& jsonString)	//WARNING: memory allocated (ResourceHandlerNode object)
{
	struct json_object * root = json_tokener_parse( jsonString.c_str() );
	if (!root || is_error(root)) {
		return NULL;
	}
	ResourceHandlerNode * ptr = fromJson(root);
	json_object_put(root);
	return ptr;
}

//static 
MimeSystem::ResourceHandlerNode * MimeSystem::ResourceHandlerNode::fromJson(struct json_object * jobj) //WARNING: memory allocated (ResourceHandlerNode object)
{

	/*{"primary":{"mime":"audio\/3gpp","extension":"3gpp", "appId":"com.palm.app.streamingmusicplayer","streamable":true,"index":67,"tag":"system-default",  "verbs":{"play": "{}"}},
	 * "alternates":[
	 * 		{"mime":"audio\/3gpp","appId":"com.palm.app.badplayer","streamable":true,"index":68 ,  "verbs":{"play": "{}","run": "{\"param\":\"value\"}"}} 
	 * 				],
	 * "verbs":[
	 * 		{"verb":"play" , "index":67}
	 * 		   ]
	 * }
	 */
	
	if ((jobj == NULL) || (is_error(jobj)))
		return NULL;
	
	struct json_object * primary_jobj = JsonGetObject(jobj,"primary");
	if (primary_jobj == NULL)
		return NULL;
	std::string mime,extension,appId,tag;
	uint32_t index;
	bool streamable;
	struct json_object * tmp = 0;
	if (extractFromJson(primary_jobj,"mime",mime) == false)
		return NULL;
	else
		std::transform(mime.begin(), mime.end(), mime.begin(), tolower);
	if (extractFromJson(primary_jobj,"appId",appId) == false)
		return NULL;
	if ((tmp = JsonGetObject(primary_jobj,"index")) == NULL)
		return NULL;
	index = json_object_get_int(tmp);
	if ((tmp = JsonGetObject(primary_jobj,"streamable")) == NULL)
		return NULL;
	streamable = json_object_get_boolean(tmp);
	if (extractFromJson(primary_jobj,"tag",tag) == false) {
		tag = "";
	}
	if (extractFromJson(primary_jobj,"extension",extension) == false) {
		extension = "";
	}
	else
		std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
		
	//create!
	ResourceHandlerNode * p_rhn;
	if (extension.size())
		p_rhn = new ResourceHandlerNode(extension,mime,appId,streamable);
	else
		p_rhn = new ResourceHandlerNode(MimeSystem::makePseudoExtensionFromMime(mime),mime,appId,streamable);  //make pseudo-extension
	
	p_rhn->m_resourceHandler.setTag(tag);
	
	//extract verbs
	std::map<std::string,std::string> verbs;
		
	if (MimeSystem::extractVerbsFromHandlerEntryJson(primary_jobj,verbs) > 0) 
	{
		//go through the verbs and add them
		for (std::map<std::string,std::string>::iterator it = verbs.begin();
				it != verbs.end();++it) 
		{
			MimeSystem::addVerbs(verbs,*p_rhn,p_rhn->m_resourceHandler);
		}
	}
		
	struct json_object * alternate_jobj = JsonGetObject(jobj,"alternates");
	if (alternate_jobj == NULL) {
		p_rhn->fixupVerbCacheTable(jobj);
		return p_rhn;		//there are no alternate handlers for this
	}
	if (json_object_is_type(alternate_jobj, json_type_array) == false) {
		p_rhn->fixupVerbCacheTable(jobj);
		return p_rhn;
	}
	
	for (int i=0; i<json_object_array_length(alternate_jobj); i++) {
		json_object* e = json_object_array_get_idx(alternate_jobj, i);
		if (extractFromJson(e,"mime",mime) == false)
			continue;
		else
			std::transform(mime.begin(), mime.end(), mime.begin(), tolower);
		if (extractFromJson(e,"appId",appId) == false)
			continue;
		if ((tmp = JsonGetObject(e,"index")) == NULL)
			continue;
		index = json_object_get_int(tmp);
		if ((tmp = JsonGetObject(e,"streamable")) == NULL)
			continue;
		streamable = json_object_get_boolean(tmp);
		extractFromJson(e,"tag",tag);
		extractFromJson(e,"extension",extension);
		if (extractFromJson(e,"tag",tag) == false) {
			tag = "";
		}
		if (extractFromJson(e,"extension",extension) == false) {
			extension = "";
		}
		else
			std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
		//HERE
		ResourceHandler * p_newHandler;
		if (extension.size()) {
			p_newHandler = new ResourceHandler(extension,mime,appId,streamable,tag);
			p_rhn->m_handlersByIndex[p_newHandler->index()] = p_newHandler;
			p_rhn->m_alternates.push_back(p_newHandler);
		}
		else {
			p_newHandler = new ResourceHandler(MimeSystem::makePseudoExtensionFromMime(mime),mime,appId,streamable,tag);
			p_rhn->m_handlersByIndex[p_newHandler->index()] = p_newHandler;
			p_rhn->m_alternates.push_back(p_newHandler);	
			
		}
		
		//extract the handler's verbs and add them
		verbs.clear();
		if (MimeSystem::extractVerbsFromHandlerEntryJson(primary_jobj,verbs) > 0) 
		{
			//go through the verbs and add them
			for (std::map<std::string,std::string>::iterator it = verbs.begin();
			it != verbs.end();++it) 
			{
				MimeSystem::addVerbs(verbs,*p_rhn,*p_newHandler);
			}
		}
	}
	
	//since all the verbs were added now, fixup the verb cache table according to the verbs entry in the main json object
	p_rhn->fixupVerbCacheTable(jobj);
	return p_rhn;
}
	
int MimeSystem::ResourceHandlerNode::fixupVerbCacheTable(struct json_object * jsonHandlerNodeEntry)
{
	std::map<std::string,uint32_t> verbs;
	if (MimeSystem::extractVerbsFromHandlerNodeEntryJson(jsonHandlerNodeEntry,verbs) == 0)
		return 0;

	int rc=0;
	for (std::map<std::string,uint32_t>::iterator it = verbs.begin();
			it != verbs.end();++it) 
	{
		//locate the VerbCacheEntry correspoding to the key in the verb iterator
		std::map<std::string,VerbCacheEntry>::iterator search_it = m_verbCache.find(it->first);
		if (search_it == m_verbCache.end()) {
			//oops, didn't find it. This is a pretty serious error (means that the addition of verbs failed and/or the VCE table in the json entry is corrupt)
			//try to recover by ignoring it
			continue;
		}
		search_it->second.activeIndex = it->second;
		++rc;
	}
	
	return rc;
}

//static 
MimeSystem * MimeSystem::instance()
{
	MutexLocker lock(&s_mutex);
	if (s_p_inst == NULL)
		s_p_inst = new MimeSystem();
	return s_p_inst;
}

//static 
MimeSystem * MimeSystem::instance(const std::string& baseConfigFile)
{

	MimeSystem * inst = instance();	
	MutexLocker lock(&(inst->m_mutex));
	
	struct json_object * file_root_jobj = json_object_from_file(const_cast<char *>(baseConfigFile.c_str()));
	if ((file_root_jobj == NULL) || (is_error(file_root_jobj)))
		return inst;
	
	inst->populateFromJson(file_root_jobj);
	json_object_put(file_root_jobj);
	return inst;
}

//static 
MimeSystem * MimeSystem::instance(const std::string& baseConfigFile,const std::string& customizedConfigFile)
{
	MimeSystem * inst = instance();
	MutexLocker lock(&(inst->m_mutex));
	
	//Since whatever comes first will not get overridden by what comes after, start with the customized file and then load the base file
	
	struct json_object * file_root_jobj = json_object_from_file(const_cast<char *>(customizedConfigFile.c_str()));
	if ((file_root_jobj != NULL) && (!is_error(file_root_jobj))) {
		inst->populateFromJson(file_root_jobj);
		json_object_put(file_root_jobj);
	}
	
	file_root_jobj = json_object_from_file(const_cast<char *>(baseConfigFile.c_str()));
	if ((file_root_jobj != NULL) && (!is_error(file_root_jobj))) {
		inst->populateFromJson(file_root_jobj);
		json_object_put(file_root_jobj);
	}

	return inst;
}

/*
 * the object must follow form of what is in the command-resource-handlers.json file
 * 
 */
int MimeSystem::populateFromJson(struct json_object * root)
{
	MutexLocker lock(&m_mutex);
	if ((root == NULL) || (is_error(root)))
		return 0;

	//figure out the schema version...
	json_object * schemaVer = json_object_object_get(root,"schemaVersion");
	if ((schemaVer) && !(is_error(schemaVer))) {
		if (json_object_is_type(schemaVer,json_type_int)) {
			if (json_object_get_int(schemaVer) == 2) {
				//this is a new version, compatible with the save format...so read it through "restore"
				std::string err;
				if (restoreMimeTable(root,err) == 0) {
					g_warning("MimeSystem::populateFromJson(): failed restoring from schema version=2 file: %s",err.c_str());
					return 0;
				}
				else 
					return 1;
			}
		}
	}
	json_object* commands = json_object_object_get(root, "commands");	
	if (commands && !is_error(commands))
	{
		if (json_object_is_type(commands, json_type_array))
		{
			for (int i=0; i<json_object_array_length(commands); i++) {
				json_object* e = json_object_array_get_idx(commands, i);
				const char* re = json_object_get_string(json_object_object_get(e, "url"));
				const char* appId = json_object_get_string(json_object_object_get(e, "appId"));
				if (re && appId) {
					RedirectHandler handler(re, appId,true);
					if (handler.reValid()) {
						addRedirectHandler(re,appId,NULL,true,true);
					}
					else {
						g_warning("Unable to parse cmd urlRe '%s'", re);
					}
				}
			}
		}
	}

	json_object* redirects = json_object_object_get(root, "redirects");
	if (redirects && !is_error(redirects))
	{
		if (json_object_is_type(redirects, json_type_array))
		{
			for (int i=0; i<json_object_array_length(redirects); i++) {
				json_object* e = json_object_array_get_idx(redirects, i);
				const char* re = json_object_get_string(json_object_object_get(e, "url"));
				const char* appId = json_object_get_string(json_object_object_get(e, "appId"));
				if (re && appId) {
					RedirectHandler handler(re, appId,false);
					if (handler.reValid()) {
						addRedirectHandler(re,appId,NULL,false,true);
					}
					else {
						g_warning("Unable to parse redirect urlRe '%s'", re);
					}
				}
			}
		}
	}

	json_object* resources = json_object_object_get(root, "resources");
	if (resources && !is_error(resources))
	{
		if (json_object_is_type(resources, json_type_array))
		{
			for (int i=0; i<json_object_array_length(resources); i++) {
				json_object* e = json_object_array_get_idx(resources, i);
				const char* extn = json_object_get_string(json_object_object_get(e, "extn"));
				const char* mime = json_object_get_string(json_object_object_get(e, "mime"));
				const char* appId = json_object_get_string(json_object_object_get(e, "appId"));
				bool stream = json_object_get_boolean(json_object_object_get(e, "streamable"));
				if (extn && appId && mime) {
					std::string extension(extn);
					addResourceHandler(extension,mime,!stream,appId,NULL,true);
				}
				else {
					g_warning("Error parsing resource idx %d", i);
				}
			}
		}
	}
	return 1;
}

std::string	MimeSystem::getActiveAppIdForResource(std::string mimeType)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
	
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it != m_resourceHandlerMap.end()) {
		return it->second->m_resourceHandler.appId();
	}
	return "";
}

int	MimeSystem::getAllAppIdForResource(std::string mimeType,std::string& r_active,std::vector<std::string>& r_alternatives)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
	
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end()) {
		return 0;
	}
	ResourceHandlerNode * p_rhn = it->second;
	r_active = p_rhn->m_resourceHandler.appId();
	for (std::vector<ResourceHandler *>::const_iterator rit = p_rhn->m_alternates.begin();
			rit != p_rhn->m_alternates.end();++rit) 
	{
		r_alternatives.push_back((*rit)->appId());
	}

	return (p_rhn->m_alternates.size() +1);
}

ResourceHandler	MimeSystem::getActiveHandlerForResource(std::string mimeType)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
	
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it != m_resourceHandlerMap.end()) {
		return it->second->m_resourceHandler;
	}
	return ResourceHandler();	//return invalid object (see ResourceHandler::valid() )
}

int	MimeSystem::getAllHandlersForResource(std::string mimeType,ResourceHandler& r_active,std::vector<ResourceHandler>& r_alternatives)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
	
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end()) {
		return 0;
	}
	ResourceHandlerNode * p_rhn = it->second;
	r_active = p_rhn->m_resourceHandler;
	for (std::vector<ResourceHandler *>::const_iterator rit = p_rhn->m_alternates.begin();
	rit != p_rhn->m_alternates.end();++rit) 
	{
		r_alternatives.push_back(*(*rit));
	}
	
	return (p_rhn->m_alternates.size() +1);
}
	
std::string	MimeSystem::getActiveAppIdForRedirect(const std::string& url,bool doNotUseRegexpMatch,bool disallowSchemeForms)
{
	MutexLocker lock(&m_mutex);
	RedirectMapIterType it;
	
	if (doNotUseRegexpMatch) {
		//strict retrieval by string equivalence on the regexp
		it = m_redirectHandlerMap.find(url);
		if (it != m_redirectHandlerMap.end()) {
			//found
			return it->second->m_redirectHandler.appId();
		}
	}
	else {
		for (it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
			if ((disallowSchemeForms) && (it->second->m_redirectHandler.isSchemeForm()))
				continue;
			//try and match against it
			if (it->second->m_redirectHandler.matches(url))
				return it->second->m_redirectHandler.appId();
		}
	}
	return "";
}

int	MimeSystem::getAllAppIdForRedirect(const std::string& url,bool doNotUseRegexpMatch,std::string& r_active,std::vector<std::string>& r_alternatives)
{
	MutexLocker lock(&m_mutex);
	int rc=0;
	RedirectMapIterType it;
	if (doNotUseRegexpMatch) {
		//strict retrieval by string equivalence on the regexp
		it = m_redirectHandlerMap.find(url);
		if (it != m_redirectHandlerMap.end()) {
			//found
			RedirectHandlerNode * p_rhn = it->second;
			//Active is a litte bit ambiguous here since there may be multiple nodes that match the url (regexps can overlap, and also scheme and "redirect" forms can refer to the same url patterns)
			//But we want an "active" to keep the API somewhat consistent...so just set the "active" as the primary handler of the first node that's found
			r_active = p_rhn->m_redirectHandler.appId();
			++rc;
			//now the rest of the alternatives from the node
			for (std::vector<RedirectHandler *>::const_iterator rit = p_rhn->m_alternates.begin();
			rit != p_rhn->m_alternates.end();++rit) 
			{
				r_alternatives.push_back((*rit)->appId());
				++rc;
			}
		}
		return rc;
	}
	
	//else, do a regexp match
	for (it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		//try and match against it
		if (it->second->m_redirectHandler.matches(url) == false)
			continue;
		
		//found a node that matches the url
		RedirectHandlerNode * p_rhn = it->second;
		//Active is a litte bit ambiguous here since there may be multiple nodes that match the url (regexps can overlap, and also scheme and "redirect" forms can refer to the same url patterns)
		//But we want an "active" to keep the API somewhat consistent...so just set the "active" as the primary handler of the first node that's found
		if (rc == 0) {
			r_active = p_rhn->m_redirectHandler.appId();
		}
		else {
			//it goes in as an alternate
			r_alternatives.push_back(p_rhn->m_redirectHandler.appId());
		}
		++rc;
		
		//now the rest of the alternatives from the node
		for (std::vector<RedirectHandler *>::const_iterator rit = p_rhn->m_alternates.begin();
				rit != p_rhn->m_alternates.end();++rit) 
		{
			r_alternatives.push_back((*rit)->appId());
			++rc;
		}
	}
	return rc;
}

RedirectHandler	MimeSystem::getActiveHandlerForRedirect(const std::string& url,bool doNotUseRegexpMatch, bool disallowSchemeForms)
{
	MutexLocker lock(&m_mutex);
	RedirectMapIterType it;
	
	if (doNotUseRegexpMatch) {
		//strict retrieval by string equivalence on the regexp
		it = m_redirectHandlerMap.find(url);
		if (it != m_redirectHandlerMap.end()) {
			//found
			return it->second->m_redirectHandler;
		}
	}
	else {
		for (it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
			if ((disallowSchemeForms) && (it->second->m_redirectHandler.isSchemeForm()))
				continue;
			//try and match against it
			if (it->second->m_redirectHandler.matches(url))
				return it->second->m_redirectHandler;
		}
	}
	return RedirectHandler();
}

int	MimeSystem::getAllHandlersForRedirect(const std::string& url,bool doNotUseRegexpMatch,RedirectHandler& r_active,std::vector<RedirectHandler>& r_alternatives)
{
	MutexLocker lock(&m_mutex);
	int rc=0;
	RedirectMapIterType it;
	if (doNotUseRegexpMatch) {
		//strict retrieval by string equivalence on the regexp
		it = m_redirectHandlerMap.find(url);
		if (it != m_redirectHandlerMap.end()) {
			//found
			RedirectHandlerNode * p_rhn = it->second;
			//Active is a litte bit ambiguous here since there may be multiple nodes that match the url (regexps can overlap, and also scheme and "redirect" forms can refer to the same url patterns)
			//But we want an "active" to keep the API somewhat consistent...so just set the "active" as the primary handler of the first node that's found
			++rc;
			r_active = RedirectHandler(p_rhn->m_redirectHandler);
			//now the rest of the alternatives from the node
			for (std::vector<RedirectHandler *>::const_iterator rit = p_rhn->m_alternates.begin();
			rit != p_rhn->m_alternates.end();++rit) 
			{
				RedirectHandler * tp = *rit;
				g_warning("tp is [%s]",tp->appId().c_str());
				r_alternatives.push_back(*(*rit));
				++rc;
			}
		}
		return rc;
	}

	//else, do a regexp match

	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		//try and match against it
		if (it->second->m_redirectHandler.matches(url) == false)
			continue;

		//found a node that matches the url
		RedirectHandlerNode * p_rhn = it->second;
		//Active is a litte bit ambiguous here since there may be multiple nodes that match the url (regexps can overlap, and also scheme and "redirect" forms can refer to the same url patterns)
		//But we want an "active" to keep the API somewhat consistent...so just set the "active" as the primary handler of the first node that's found
		
		if (rc == 0) {
			r_active = p_rhn->m_redirectHandler;
		}
		else {
			//it goes in as an alternate
			r_alternatives.push_back(p_rhn->m_redirectHandler);
		}
		++rc;
		//now the rest of the alternatives from the node
		for (std::vector<RedirectHandler *>::const_iterator rit = p_rhn->m_alternates.begin();
		rit != p_rhn->m_alternates.end();++rit) 
		{
			r_alternatives.push_back(*(*rit));
			++rc;
		}
	}
	return rc;
}

std::string	MimeSystem::getAppIdByVerbForResource(std::string mimeType,const std::string& verb,std::string& r_params,uint32_t& r_index)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
		
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end())
		return "";
	
	//found...
	ResourceHandlerNode * p_rhn = it->second;
	//look up the verb in the verb cache map
	VerbCacheMapIterType vc_it = p_rhn->m_verbCache.find(verb);
	if (vc_it == p_rhn->m_verbCache.end())
		return "";
	
	r_index = vc_it->second.activeIndex;
	
	//retrieve the handler by index
	std::map<uint32_t,ResourceHandler *>::iterator handler_it = p_rhn->m_handlersByIndex.find(vc_it->second.activeIndex);
	if (handler_it == p_rhn->m_handlersByIndex.end())
		return "";
	
	ResourceHandler * p_rh = handler_it->second;
	//get the parameters for the verb
	std::map<std::string,std::string>::const_iterator verb_it = p_rh->verbs().find(verb);
	if (verb_it != p_rh->verbs().end())
		r_params = verb_it->second;
	
	return (p_rh->appId());
}

ResourceHandler	MimeSystem::getHandlerByVerbForResource(std::string mimeType,const std::string& verb)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
		
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end())
		return ResourceHandler();

	//found...
	ResourceHandlerNode * p_rhn = it->second;
	//look up the verb in the verb cache map
	VerbCacheMapIterType vc_it = p_rhn->m_verbCache.find(verb);
	if (vc_it == p_rhn->m_verbCache.end())
		return ResourceHandler();

	//retrieve the handler by index
	std::map<uint32_t,ResourceHandler *>::iterator handler_it = p_rhn->m_handlersByIndex.find(vc_it->second.activeIndex);
	if (handler_it == p_rhn->m_handlersByIndex.end())
		return ResourceHandler();

	ResourceHandler * p_rh = handler_it->second;						//TODO: broken out for debugging; condense.
	return *p_rh;
}

int	MimeSystem::getAllHandlersByVerbForResource(std::string mimeType,const std::string& verb,std::vector<ResourceHandler>& r_handlers)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
		
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end())
		return 0;

	//found...
	ResourceHandlerNode * p_rhn = it->second;
	//go through all the verbs in all the handlers, adding the matches info into the return vector
	//VerbInfo(const std::string& verb,const std::string& params,const std::string handlerAppId, uint32_t handlerIndex)
	std::map<std::string,std::string>::const_iterator verb_it;

	int rc=0;
	for (verb_it = p_rhn->m_resourceHandler.verbs().begin();
	verb_it != p_rhn->m_resourceHandler.verbs().end();++verb_it)
	{
		if (verb_it->first == verb) {
			r_handlers.push_back(p_rhn->m_resourceHandler);
			++rc;
			break;
		}
	}

	for (std::vector<ResourceHandler *>::const_iterator handler_it = p_rhn->m_alternates.begin();
	handler_it != p_rhn->m_alternates.end();++handler_it)
	{
		for (verb_it = (*handler_it)->verbs().begin();
		verb_it != (*handler_it)->verbs().end();++verb_it)
		{
			if (verb_it->first == verb) {
				r_handlers.push_back(*(*handler_it));
				++rc;
				break;
			}
		}
	}
	return rc;
}

int MimeSystem::getAllAppIdByVerbForResource(std::string mimeType,const std::string& verb,std::vector<VerbInfo>& r_handlers)
{
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
	
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end())
		return 0;

	//found...
	ResourceHandlerNode * p_rhn = it->second;
	//go through all the verbs in all the handlers, adding the matches info into the return vector
	//VerbInfo(const std::string& verb,const std::string& params,const std::string handlerAppId, uint32_t handlerIndex)
	std::map<std::string,std::string>::const_iterator verb_it;
	
	int rc=0;
	for (verb_it = p_rhn->m_resourceHandler.verbs().begin();
		verb_it != p_rhn->m_resourceHandler.verbs().end();++verb_it)
	{
		if (verb_it->first == verb) {
			r_handlers.push_back(VerbInfo(verb_it->first,verb_it->second,p_rhn->m_resourceHandler.appId(),p_rhn->m_resourceHandler.index()));
			++rc;
		}
	}
	
	for (std::vector<ResourceHandler *>::const_iterator handler_it = p_rhn->m_alternates.begin();
		handler_it != p_rhn->m_alternates.end();++handler_it)
	{
		for (verb_it = (*handler_it)->verbs().begin();
		verb_it != (*handler_it)->verbs().end();++verb_it)
		{
			if (verb_it->first == verb) {
				r_handlers.push_back(VerbInfo(verb_it->first,verb_it->second,(*handler_it)->appId(),(*handler_it)->index()));
				++rc;
			}
		}
	}
	return rc;
}

std::string	MimeSystem::getAppIdByVerbForRedirect(const std::string& url,bool disallowSchemeForms,const std::string& verb,std::string& r_params,uint32_t& r_index)
{
	MutexLocker lock(&m_mutex);
	RedirectHandlerNode * p_rhn = NULL;
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		if ((disallowSchemeForms) && (it->second->m_redirectHandler.isSchemeForm()))
			continue;
		//try and match against it
		if (it->second->m_redirectHandler.matches(url)) {
			p_rhn = it->second;
			break;
		}
	}
	
	if (p_rhn == NULL)
		return "";
	
	//look up the verb in the verb cache map
	VerbCacheMapIterType vc_it = p_rhn->m_verbCache.find(verb);
	if (vc_it == p_rhn->m_verbCache.end())
		return "";

	r_index = vc_it->second.activeIndex;
	
	//retrieve the handler by index
	std::map<uint32_t,RedirectHandler *>::iterator handler_it = p_rhn->m_handlersByIndex.find(vc_it->second.activeIndex);
	if (handler_it == p_rhn->m_handlersByIndex.end())
		return "";

	RedirectHandler * p_rh = handler_it->second;
	//get the parameters for the verb
	std::map<std::string,std::string>::const_iterator verb_it = p_rh->verbs().find(verb);
	if (verb_it != p_rh->verbs().end())
		r_params = verb_it->second;

	return (p_rh->appId());
	
}

RedirectHandler	MimeSystem::getHandlerByVerbForRedirect(const std::string& url,bool disallowSchemeForms,const std::string& verb)
{
	MutexLocker lock(&m_mutex);
	RedirectHandlerNode * p_rhn = NULL;
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		if ((disallowSchemeForms) && (it->second->m_redirectHandler.isSchemeForm()))
			continue;
		//try and match against it
		if (it->second->m_redirectHandler.matches(url)) {
			p_rhn = it->second;
			break;
		}
	}

	if (p_rhn == NULL)
		return RedirectHandler();

	//look up the verb in the verb cache map
	VerbCacheMapIterType vc_it = p_rhn->m_verbCache.find(verb);
	if (vc_it == p_rhn->m_verbCache.end())
		return RedirectHandler();

	//retrieve the handler by index
	std::map<uint32_t,RedirectHandler *>::iterator handler_it = p_rhn->m_handlersByIndex.find(vc_it->second.activeIndex);
	if (handler_it == p_rhn->m_handlersByIndex.end())
		return RedirectHandler();

	RedirectHandler * p_rh = handler_it->second;

	return *p_rh;
	
}

int MimeSystem::getAllHandlersByVerbForRedirect(const std::string& url,const std::string& verb,std::vector<RedirectHandler>& r_handlers)
{
	MutexLocker lock(&m_mutex);
	RedirectHandlerNode * p_rhn = NULL;
	int rc = 0;
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		if (it->second->m_redirectHandler.matches(url) == false)
			continue;

		p_rhn = it->second;

		//found...

		//go through all the verbs in all the handlers, adding the matches' info into the return vector
		//VerbInfo(const std::string& verb,const std::string& params,const std::string handlerAppId, uint32_t handlerIndex)
		std::map<std::string,std::string>::const_iterator verb_it;

		for (verb_it = p_rhn->m_redirectHandler.verbs().begin();
		verb_it != p_rhn->m_redirectHandler.verbs().end();++verb_it)
		{
			if (verb_it->first == verb) {
				r_handlers.push_back(p_rhn->m_redirectHandler);
				++rc;
			}
		}

		for (std::vector<RedirectHandler *>::const_iterator handler_it = p_rhn->m_alternates.begin();
		handler_it != p_rhn->m_alternates.end();++handler_it)
		{
			for (verb_it = (*handler_it)->verbs().begin();
			verb_it != (*handler_it)->verbs().end();++verb_it)
			{
				if (verb_it->first == verb) {
					r_handlers.push_back(*(*handler_it));
					++rc;
				}
			}
		}
	}
	return rc;
}

int MimeSystem::getAllAppIdByVerbForRedirect(const std::string& url,const std::string& verb,std::vector<VerbInfo>& r_handlers)
{
	MutexLocker lock(&m_mutex);
	RedirectHandlerNode * p_rhn = NULL;
	int rc = 0;
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		if (it->second->m_redirectHandler.matches(url) == false)
			continue;

		p_rhn = it->second;

		//found...

		//go through all the verbs in all the handlers, adding the matches' info into the return vector
		//VerbInfo(const std::string& verb,const std::string& params,const std::string handlerAppId, uint32_t handlerIndex)
		std::map<std::string,std::string>::const_iterator verb_it;

		for (verb_it = p_rhn->m_redirectHandler.verbs().begin();
		verb_it != p_rhn->m_redirectHandler.verbs().end();++verb_it)
		{
			if (verb_it->first == verb) {
				r_handlers.push_back(VerbInfo(verb_it->first,verb_it->second,p_rhn->m_redirectHandler.appId(),p_rhn->m_redirectHandler.index()));
				++rc;
			}
		}

		for (std::vector<RedirectHandler *>::const_iterator handler_it = p_rhn->m_alternates.begin();
		handler_it != p_rhn->m_alternates.end();++handler_it)
		{
			for (verb_it = (*handler_it)->verbs().begin();
			verb_it != (*handler_it)->verbs().end();++verb_it)
			{
				if (verb_it->first == verb) {
					r_handlers.push_back(VerbInfo(verb_it->first,verb_it->second,(*handler_it)->appId(),(*handler_it)->index()));
					++rc;
				}
			}
		}
	}
	return rc;
}

RedirectHandler	MimeSystem::getRedirectHandlerDirect(const uint32_t index)
{
	
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		std::map<uint32_t,RedirectHandler *>::iterator rit = it->second->m_handlersByIndex.find(index);
		if (rit != it->second->m_handlersByIndex.end())
			return (*(rit->second));
	}
	
	return RedirectHandler();
}

ResourceHandler	MimeSystem::getResourceHandlerDirect(const uint32_t index)
{
	for (ResourceMapIterType it = m_resourceHandlerMap.begin();it != m_resourceHandlerMap.end();++it) {
		std::map<uint32_t,ResourceHandler *>::iterator rit = it->second->m_handlersByIndex.find(index);
		if (rit != it->second->m_handlersByIndex.end())
			return (*(rit->second));
	}
	
	return ResourceHandler();
}
	
int MimeSystem::removeAllForAppId(const std::string& appId)
{
	MutexLocker lock(&m_mutex);
	std::vector<std::string> keys;
	//go through all the nodes
	
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) 
	{
		int rc = it->second->removeAppId(appId);
		if (rc == RC_HANDLERNODE_REMOVEAPPID_REMOVENODE) {
			//need to remove the whole node
			keys.push_back(it->first);
		}
	}
	
	//erase all the keys for nodes which are completely obliterated
	for (std::vector<std::string>::iterator it = keys.begin();it != keys.end();++it) {
		RedirectMapIterType found_it = m_redirectHandlerMap.find(*it);
		MimeSystem::reclaimIndex(found_it->second->m_redirectHandler.index());
		delete (found_it->second);
		m_redirectHandlerMap.erase(*it);
	}
	keys.clear();
	// and do the same for the Resources...
	
	for (ResourceMapIterType it = m_resourceHandlerMap.begin();it != m_resourceHandlerMap.end();++it) 
	{
		int rc = it->second->removeAppId(appId);
		if (rc == RC_HANDLERNODE_REMOVEAPPID_REMOVENODE) {
			//need to remove the whole node
			keys.push_back(it->first);
		}
	}

	//erase all the keys for nodes which are completely obliterated
	for (std::vector<std::string>::iterator it = keys.begin();it != keys.end();++it) {
		ResourceMapIterType found_it = m_resourceHandlerMap.find(*it);
		MimeSystem::reclaimIndex(found_it->second->m_resourceHandler.index());
		delete (found_it->second);
		m_resourceHandlerMap.erase(*it);
	}

	return 1;
}

int	MimeSystem::removeAllForMimeType(std::string mimeType)
{	
	MutexLocker lock(&m_mutex);
	
	std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
		
	//find the ResourceHandlerNode, and delete it
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end())
		return 0;
	delete (it->second);
	m_resourceHandlerMap.erase(it);
	return 1;
}

int	MimeSystem::removeAllForUrl(const std::string& url)
{
	MutexLocker lock(&m_mutex);
	//find the RedirectHandlerNode, and delete it
	RedirectMapIterType it = m_redirectHandlerMap.find(url);
	if (it == m_redirectHandlerMap.end())
		return 0;
	delete (it->second);
	m_redirectHandlerMap.erase(it);
	return 1;
}

/*
 * returns >0 on success, 0 on error
 */
int	MimeSystem::addResourceHandler(std::string& extension,std::string mimeType,bool shouldDownload,const std::string appId,const std::map<std::string,std::string> * pVerbs,bool sysDefault)
{
	MutexLocker lock(&m_mutex);
	
	//if mimeType is blank, bail
	if (mimeType.size() == 0)
		return 0;				///error
	else
		std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(), tolower);
	
	//if the extension is blank, then create pseudo extension
	if (extension.empty())
		extension = MimeSystem::makePseudoExtensionFromMime(mimeType);
	else
		std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
	
	//check to see if the mime<->extension mapping already exists
	std::map<std::string,std::string>::iterator mit = m_extensionToMimeMap.find(extension);
	if (mit == m_extensionToMimeMap.end()) {
		//add it...
		m_extensionToMimeMap[extension] = mimeType;
	}
	else {
	
	//( if it already exists, just ignore the redefinition)
	//...but replace the extension passed in with the found one, so that the caller knows his extension proposal was rejected and a different one used
		for (mit = m_extensionToMimeMap.begin();mit != m_extensionToMimeMap.end();++mit)
		{
			if (mit->second == mimeType) {
				extension = mit->first;
				break;
			}
		}
	}
	//see if there is a primary entry already
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end()) {
		//no...this will be the primary(active) one
		ResourceHandlerNode * p_rhn = new ResourceHandlerNode(extension,mimeType,appId,!shouldDownload);
		if (sysDefault)
			p_rhn->m_resourceHandler.setTag("system-default");	//also tag as a system default
		m_resourceHandlerMap[mimeType] = p_rhn;
		return 1;
	}
	
	//exists...this will have to be added as an alternate entry, but only if it doesn't already exist in the alternates
	ResourceHandlerNode * p_rhn = it->second;
	if (p_rhn->exists(appId,mimeType))
		return 3;		//it existing here is not an error. Just quietly exit
	
	//add it
	ResourceHandler * p_newHandler = new ResourceHandler(extension,mimeType,appId,!shouldDownload);
	p_rhn->m_handlersByIndex[p_newHandler->index()] = p_newHandler;
	p_rhn->m_alternates.push_back(p_newHandler);
	return 2;
}

int	MimeSystem::addResourceHandler(std::string extension,bool shouldDownload,const std::string appId,const std::map<std::string,std::string> * pVerbs,bool sysDefault)
{
	MutexLocker lock(&m_mutex);
	//find the mime type for this extension
	std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
	std::map<std::string,std::string>::iterator mit = m_extensionToMimeMap.find(extension);
	if (mit == m_extensionToMimeMap.end()) {
		//doesn't exist... bail
		return 0;
	}
	std::string mimeType = mit->second;
	
	//see if there is a primary entry already
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end()) {
		//no...this will be the primary(active) one
		ResourceHandlerNode * p_rhn = new ResourceHandlerNode(extension,mimeType,appId,!shouldDownload);
		if (sysDefault)
			p_rhn->m_resourceHandler.setTag("system-default");	//also tag as a system default
		m_resourceHandlerMap[mimeType] = p_rhn;
		return 1;
	}

	//exists...this will have to be added as an alternate entry, but only if it doesn't already exist in the alternates
	ResourceHandlerNode * p_rhn = it->second;
	if (p_rhn->exists(appId,mimeType))
		return 3;			//it existing here is not an error. Just quietly exit

	//add it
	ResourceHandler * p_newHandler = new ResourceHandler(extension,mimeType,appId,!shouldDownload);
	p_rhn->m_handlersByIndex[p_newHandler->index()] = p_newHandler;

	p_rhn->m_alternates.push_back(p_newHandler);
		
	return 2;
}

int	MimeSystem::addRedirectHandler(const std::string& url,const std::string appId,const std::map<std::string,std::string> * pVerbs,bool isSchemeForm,bool sysDefault)
{
	MutexLocker lock(&m_mutex);
	//see if there is a primary entry already
	RedirectMapIterType it = m_redirectHandlerMap.find(url);
	if (it == m_redirectHandlerMap.end()) {
		//no...this will be the primary(active) one
		RedirectHandlerNode * p_rhn = new RedirectHandlerNode(url,appId,isSchemeForm);
		if (sysDefault)
			p_rhn->m_redirectHandler.setTag("system-default");	//also tag as a system default
		m_redirectHandlerMap[url] = p_rhn;
		return 1;
	}

	//exists...this will have to be added as an alternate entry, but only if it doesn't already exist in the alternates
	RedirectHandlerNode * p_rhn = it->second;
	if (p_rhn->exists(url,appId))
		return 3;				//it existing here is not an error. Just quietly exit

	//add it
	RedirectHandler * p_newHandler = new RedirectHandler(url,appId,isSchemeForm);
	p_rhn->m_handlersByIndex[p_newHandler->index()] = p_newHandler;
	
	p_rhn->m_alternates.push_back(p_newHandler);
	return 2;
}

int	MimeSystem::addVerbsToResourceHandler(std::string mimeType,const std::string& appId,const std::map<std::string,std::string>& verbs)
{

	std::transform(mimeType.begin(),mimeType.end(),mimeType.begin(),tolower);
	ResourceMapIterType resource_it = m_resourceHandlerMap.find(mimeType);
	if (resource_it != m_resourceHandlerMap.end())
	{
		ResourceHandlerNode * p_rhn = resource_it->second;
		//go through the handlers to find the app id
		if (p_rhn->m_resourceHandler.appId() == appId)
			return MimeSystem::addVerbs(verbs,*p_rhn,p_rhn->m_resourceHandler);		//found it...add verbs
		for (std::vector<ResourceHandler *>::iterator v_it = p_rhn->m_alternates.begin();
				v_it != p_rhn->m_alternates.end();++v_it)
		{
			if ((*v_it)->appId() == appId)
				return MimeSystem::addVerbs(verbs,*p_rhn,*(*v_it));		//found it...add verbs
		}
	}
	return 0;
}

int	MimeSystem::addVerbsToRedirectHandler(const std::string& url,const std::string& appId,const std::map<std::string,std::string>& verbs)
{
	RedirectMapIterType redirect_it = m_redirectHandlerMap.find(url);
	if (redirect_it != m_redirectHandlerMap.end())
	{
		RedirectHandlerNode * p_rhn = redirect_it->second;
		//go through the handlers to find the app id
		if (p_rhn->m_redirectHandler.appId() == appId)
			return MimeSystem::addVerbs(verbs,*p_rhn,p_rhn->m_redirectHandler);		//found it...add verbs
		for (std::vector<RedirectHandler *>::iterator v_it = p_rhn->m_alternates.begin();
		v_it != p_rhn->m_alternates.end();++v_it)
		{
			if ((*v_it)->appId() == appId)
				return MimeSystem::addVerbs(verbs,*p_rhn,*(*v_it));		//found it...add verbs
		}
	}
	return 0;
}

int	MimeSystem::addVerbsDirect(uint32_t index,const std::map<std::string,std::string>& verbs)
{
	//scan all the maps to find one that has the index in question
	for (ResourceMapIterType resource_it = m_resourceHandlerMap.begin();
			resource_it != m_resourceHandlerMap.end();++resource_it)
	{
		ResourceHandlerNode * p_rhn = resource_it->second;
		std::map<uint32_t,ResourceHandler *>::iterator find_it = p_rhn->m_handlersByIndex.find(index);
		if (find_it != p_rhn->m_handlersByIndex.end())
			return MimeSystem::addVerbs(verbs,*p_rhn,*(find_it->second));		//found it...add verbs
	}
	for (RedirectMapIterType redirect_it = m_redirectHandlerMap.begin();
	redirect_it != m_redirectHandlerMap.end();++redirect_it)
	{
		RedirectHandlerNode * p_rhn = redirect_it->second;
		std::map<uint32_t,RedirectHandler *>::iterator find_it = p_rhn->m_handlersByIndex.find(index);
		if (find_it != p_rhn->m_handlersByIndex.end())
			return MimeSystem::addVerbs(verbs,*p_rhn,*(find_it->second));		//found it...add verbs
	}
	return 0;
}
	
int MimeSystem::swapResourceHandler(std::string mimeType, uint32_t index)
{
	MutexLocker lock(&m_mutex);
	std::transform(mimeType.begin(),mimeType.end(),mimeType.begin(),tolower);
		
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it == m_resourceHandlerMap.end())
		return 0;
	
	return (it->second->swapHandler(index));
}

int	MimeSystem::swapRedirectHandler(const std::string& url, uint32_t index)
{
	MutexLocker lock(&m_mutex);
	RedirectMapIterType it = m_redirectHandlerMap.find(url);
	if (it == m_redirectHandlerMap.end())
		return 0;

	return (it->second->swapHandler(index));
}

//static
bool MimeSystem::getExtensionFromUrl(const std::string& url,std::string& r_extn)
{
	std::string resource;
	QUrl qurl(url.c_str());
	if (qurl.isValid()) {
		resource = getResourceNameFromUrl(qurl);
	}
	else {
		resource = url;
	}

	std::string::size_type pos = resource.rfind('.');
	if ( (pos >= 0) && (pos != std::string::npos) ) {
		r_extn = resource.substr(pos+1);
		std::transform(r_extn.begin(), r_extn.end(), r_extn.begin(), tolower);
		return true;
	}

	return false;
}

bool MimeSystem::getMimeTypeByExtension(std::string extension,std::string& r_mimeType)
{
	MutexLocker lock(&m_mutex);
	std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
	std::map<std::string,std::string>::iterator it = m_extensionToMimeMap.find(extension);
	if (it != m_extensionToMimeMap.end()) 
	{
		r_mimeType = it->second;
		return true;
	}
	return false;
}

//static
uint32_t MimeSystem::assignIndex()
{
	MutexLocker lock(&s_mutex);
	if (s_indexRecycler.size() == 0) {
		s_lastAssignedIndex = s_genIndex++;
		return (s_lastAssignedIndex);
	}
	s_lastAssignedIndex = *(s_indexRecycler.begin());
	s_indexRecycler.erase(s_indexRecycler.begin());
	return s_lastAssignedIndex;
}

//static 
uint32_t MimeSystem::getLastAssignedIndex()
{
	MutexLocker lock(&s_mutex);
	return s_lastAssignedIndex;
}

//static 
std::string	MimeSystem::makePseudoExtensionFromMime(const std::string& mimeType)
{
	std::string s = mimeType;
	std::replace(s.begin(), s.end(), '/', '.');
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s;
}

std::string	MimeSystem::allTablesAsJsonString()
{
	MutexLocker locker(&m_mutex);
	json_object * jobj = json_object_new_object();
	json_object_object_add(jobj,(char *)"resources",resourceTableAsJsonArray());
	json_object_object_add(jobj,(char *)"redirects",redirectTableAsJsonArray());
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}

std::string	MimeSystem::resourceTableAsJsonString()
{
	MutexLocker locker(&m_mutex);
	json_object * jobj = resourceTableAsJson();
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}

json_object * MimeSystem::resourceTableAsJson()	//WARNING: memory allocated; caller must clean
{
	MutexLocker locker(&m_mutex);
	json_object * jobj = json_object_new_object();
	json_object * jarray = json_object_new_array();
	for (ResourceMapIterType it = m_resourceHandlerMap.begin();
	it != m_resourceHandlerMap.end();++it) 
	{
		json_object * inner_jobj = json_object_new_object();
		json_object_object_add(inner_jobj,(char *)"mimeType",json_object_new_string(it->first.c_str()));
		json_object_object_add(inner_jobj,(char *)"handlers",it->second->toJson());
		json_object_array_add(jarray,inner_jobj);
	}
	json_object_object_add(jobj,(char *)"resources",jarray);
	return jobj;
}

json_object * MimeSystem::resourceTableAsJsonArray()	//WARNING: memory allocated; caller must clean
{
	MutexLocker locker(&m_mutex);
	json_object * jobj = json_object_new_array();
	for (ResourceMapIterType it = m_resourceHandlerMap.begin();
	it != m_resourceHandlerMap.end();++it) 
	{
		json_object * inner_jobj = json_object_new_object();
		json_object_object_add(inner_jobj,(char *)"mimeType",json_object_new_string(it->first.c_str()));
		json_object_object_add(inner_jobj,(char *)"handlers",it->second->toJson());
		json_object_array_add(jobj,inner_jobj);
	}
	
	return jobj;
}


std::string	MimeSystem::redirectTableAsJsonString()
{
	MutexLocker locker(&m_mutex);
	json_object * jobj = redirectTableAsJson();
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}
	
json_object * MimeSystem::redirectTableAsJson() //WARNING: memory allocated; caller must clean
{
	MutexLocker locker(&m_mutex);
	json_object * jobj = json_object_new_object();
	json_object * jarray = json_object_new_array();
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();
	it != m_redirectHandlerMap.end();++it) 
	{
		json_object * inner_jobj = json_object_new_object();
		json_object_object_add(inner_jobj,(char *)"url",json_object_new_string(it->first.c_str()));
		json_object_object_add(inner_jobj,(char *)"handlers",it->second->toJson());
		json_object_array_add(jarray,inner_jobj);
	}
	json_object_object_add(jobj,(char *)"redirects",jarray);
	return jobj;
}

json_object * MimeSystem::redirectTableAsJsonArray() //WARNING: memory allocated; caller must clean
{
	MutexLocker locker(&m_mutex);
	json_object * jobj = json_object_new_array();
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();
	it != m_redirectHandlerMap.end();++it) 
	{
		json_object * inner_jobj = json_object_new_object();
		json_object_object_add(inner_jobj,(char *)"url",json_object_new_string(it->first.c_str()));
		json_object_object_add(inner_jobj,(char *)"handlers",it->second->toJson());
		json_object_array_add(jobj,inner_jobj);
	}
	return jobj;
}

std::string	MimeSystem::extensionMapAsJsonString()
{
	MutexLocker locker(&m_mutex);
	struct json_object * jobj = extensionMapAsJson();
	std::string s = json_object_to_json_string(jobj);
	json_object_put(jobj);
	return s;
}

json_object * MimeSystem::extensionMapAsJson()	//WARNING: memory allocated; caller must clean
{
	MutexLocker locker(&m_mutex);
	struct json_object * jobj = json_object_new_object();
	json_object * jarr = json_object_new_array();
	
	for (std::map<std::string,std::string>::iterator it = m_extensionToMimeMap.begin();
		it != m_extensionToMimeMap.end();++it) {
		struct json_object * jobj_inner = json_object_new_object();
		json_object_object_add(jobj_inner,(char *)(it->first.c_str()),json_object_new_string(it->second.c_str()));
		json_object_array_add(jarr,jobj_inner);
	}
	json_object_object_add(jobj,(char *)"extensionMap",jarr);
	return jobj;
}

json_object * MimeSystem::extensionMapAsJsonArray() //WARNING: memory allocated; caller must clean
{
	MutexLocker locker(&m_mutex);
	json_object * jarr = json_object_new_array();
	
	for (std::map<std::string,std::string>::iterator it = m_extensionToMimeMap.begin();
		it != m_extensionToMimeMap.end();++it) {
		struct json_object * jobj_inner = json_object_new_object();
		json_object_object_add(jobj_inner,(char *)(it->first.c_str()),json_object_new_string(it->second.c_str()));
		json_object_array_add(jarr,jobj_inner);
	}
	return jarr;
}

bool MimeSystem::saveMimeTable(const std::string& file,std::string& r_err)
{
	MutexLocker locker(&m_mutex);
	std::string mimeTablesJsonStr;
	r_err.clear();
	//try and open the file
	FILE * fp = fopen(file.c_str(),"w");
	if (!fp) {
		r_err = "Unable to open file "+file;
		return false;
	}
	
	struct json_object * outer_jobj = json_object_new_object();
	
	//get the extension map, and the mime tables and wrap into a json object
	json_object_object_add(outer_jobj,(char *)"schemaVersion",json_object_new_int(2));
	json_object_object_add(outer_jobj,(char *)"extensionMap",extensionMapAsJsonArray());
	json_object_object_add(outer_jobj,(char *)"resources",resourceTableAsJsonArray());
	json_object_object_add(outer_jobj,(char *)"redirects",redirectTableAsJsonArray());
	
	std::string s = json_object_to_json_string(outer_jobj);
	json_object_put(outer_jobj);
	
	//write it
	size_t wr = fwrite(s.data(),1,s.size(),fp);
	if (wr != s.size()) {
		r_err = "Couldn't write maps to file "+file;
		goto Done_saveMimeTable;
	}
	
	fprintf(fp,"\n\n");
	
Done_saveMimeTable:

	fclose(fp);
	if (r_err.size())
		return false;
	return true;
}

bool MimeSystem::saveMimeTableToActiveFile(std::string& r_err)
{
	MutexLocker locker(&m_mutex);
	std::string mimeTablesJsonStr;
	r_err.clear();
	//try and open the file
	FILE * fp = fopen(Settings::LunaSettings()->lunaCmdHandlerSavedPath.c_str(),"w");
	if (!fp) {
		r_err = std::string("Unable to open file ")+Settings::LunaSettings()->lunaCmdHandlerSavedPath;
		return false;
	}
	
	struct json_object * outer_jobj = json_object_new_object();
	
	//get the extension map, and the mime tables and wrap into a json object
	json_object_object_add(outer_jobj,(char *)"schemaVersion",json_object_new_int(2));
	json_object_object_add(outer_jobj,(char *)"extensionMap",extensionMapAsJsonArray());
	json_object_object_add(outer_jobj,(char *)"resources",resourceTableAsJsonArray());
	json_object_object_add(outer_jobj,(char *)"redirects",redirectTableAsJsonArray());
	
	std::string s = json_object_to_json_string(outer_jobj);
	json_object_put(outer_jobj);
	
	//write it
	size_t wr = fwrite(s.data(),1,s.size(),fp);
	if (wr != s.size()) {
		r_err = std::string("Couldn't write maps to file ")+Settings::LunaSettings()->lunaCmdHandlerSavedPath;
		goto Done_saveMimeTableToActiveFile;
	}
	
	fprintf(fp,"\n\n");
	
Done_saveMimeTableToActiveFile:

	fclose(fp);
	if (r_err.size())
		return false;
	return true;
}

//TODO: reimplement as static factory fn?
bool MimeSystem::restoreMimeTable(const std::string& file,std::string& r_err)
{
	//read in the file as json
	char* tables = readFile(file.c_str());
	if (!tables) {
		r_err = "No saved tables found in "+file;
		return false;
	}

	json_object* root = json_tokener_parse(tables);
	delete[] tables;
	
	bool rc = restoreMimeTable(root,r_err); 
	
	if (root)
		json_object_put(root);
	
	return rc;
}

bool MimeSystem::restoreMimeTable(json_object * root,std::string& r_err)
{
	std::string val_s;
	json_object * topLevel_jobj;
	
	if (!root || is_error(root))
	{
		root = 0;
		r_err = "Invalid json";
		goto Done_restoreMimeTable;
	}

	//restore the extension map
	m_extensionToMimeMap.clear();

	if ((topLevel_jobj = JsonGetObject(root,"extensionMap")) != NULL) 
	{
		//found the extn map...
		if (json_object_is_type(topLevel_jobj, json_type_array))
		{
			for (int i=0; i<json_object_array_length(topLevel_jobj); i++) {
				json_object* e = json_object_array_get_idx(topLevel_jobj, i);
				json_object_object_foreach(e,key,val) {		//a bit awkward since there's only going to be 1 k-v pair per object but using the long (expanded out) version of the macro is messy
					val_s = json_object_get_string(val);
					m_extensionToMimeMap[key] = val_s;
				}
			}
		}
	}
	// if the extension map is not found, this isn't a fatal error. It could be that a dynamic mapping (e.g. libmagic) system is in place.

	//restore the redirect and resource maps
	if ((topLevel_jobj = JsonGetObject(root,"redirects")) != NULL) 
	{
		//found the redirects map...
		if (json_object_is_type(topLevel_jobj, json_type_array)) 
		{
			for (int i=0; i<json_object_array_length(topLevel_jobj); i++) {
				json_object* e = json_object_array_get_idx(topLevel_jobj, i);
				//get the handlers object from within
				json_object* h = json_object_object_get(e,"handlers");
				if (!h || (is_error(h)))
					continue;		//skip...error.
				
				RedirectHandlerNode * p_rhn = RedirectHandlerNode::fromJson(h);
				if (p_rhn != NULL) {
					//add...
					m_redirectHandlerMap[p_rhn->m_redirectHandler.urlRe()] = p_rhn;
				}
			}
		}
	}

	if ((topLevel_jobj = JsonGetObject(root,"resources")) != NULL) 
	{
		//found the redirects map...
		if (json_object_is_type(topLevel_jobj, json_type_array)) 
		{
			for (int i=0; i<json_object_array_length(topLevel_jobj); i++) {
				json_object* e = json_object_array_get_idx(topLevel_jobj, i);
				//get the handlers object from within
				json_object* h = json_object_object_get(e,"handlers");
				if (!h || (is_error(h)))
					continue;		//skip...error.
				ResourceHandlerNode * p_rhn = ResourceHandlerNode::fromJson(h);
				if (p_rhn != NULL) {
					//add...
					m_resourceHandlerMap[p_rhn->m_resourceHandler.contentType()] = p_rhn;
				}
			}
		}
	}

	Done_restoreMimeTable:

	if (r_err.size())
		return false;
	return true;
}

bool MimeSystem::clearMimeTable()
{
	//for now, just a wrapper on destroy()
	destroy();
	return true;
}

//static
void MimeSystem::deleteSavedMimeTable() 
{
	MutexLocker locker(&s_mutex);
	unlink(Settings::LunaSettings()->lunaCmdHandlerSavedPath.c_str());
	
}

bool MimeSystem::dbg_printMimeTables()
{
	std::vector<std::pair<std::string,std::vector<std::string> > > r_resourceTableString;
	std::vector<std::pair<std::string,std::vector<std::string> > > r_redirectTableString;
	MimeSystem::instance()->dbg_getResourceTableStrings(r_resourceTableString);
	MimeSystem::instance()->dbg_getRedirectTableStrings(r_redirectTableString);

	for (std::vector<std::pair<std::string,std::vector<std::string> > >::iterator outer_it = r_resourceTableString.begin();
	outer_it != r_resourceTableString.end();++outer_it) 
	{
		for (std::vector<std::string>::iterator inner_it = outer_it->second.begin();inner_it != outer_it->second.end();++inner_it)
		{
			if (inner_it == outer_it->second.begin()) {
				printf("mimeType: [%s] , primary: %s\n",outer_it->first.c_str(),(*inner_it).c_str());
			}
			else
				printf("\t\t%s\n",(*inner_it).c_str());
		}
	}
	printf("\n\n");
	for (std::vector<std::pair<std::string,std::vector<std::string> > >::iterator outer_it = r_redirectTableString.begin();
	outer_it != r_redirectTableString.end();++outer_it) 
	{
		for (std::vector<std::string>::iterator inner_it = outer_it->second.begin();inner_it != outer_it->second.end();++inner_it)
		{
			if (inner_it == outer_it->second.begin()) {
				printf("url: [%s] , primary: %s\n",outer_it->first.c_str(),(*inner_it).c_str());
			}
			else
				printf("\t\t%s\n",(*inner_it).c_str());
		}
	}
	return true;
}

bool MimeSystem::dbg_getResourceTableStrings(std::vector<std::pair<std::string,std::vector<std::string> > >& r_resourceTableStrings)
{
	MutexLocker locker(&m_mutex);
	for (ResourceMapIterType it = m_resourceHandlerMap.begin();
	it != m_resourceHandlerMap.end();++it) 
	{
		ResourceHandlerNode * p_rhn = it->second;
		std::vector<std::string> strings;
		strings.push_back(p_rhn->m_resourceHandler.toJsonString());
		for (std::vector<ResourceHandler *>::iterator in_it = p_rhn->m_alternates.begin();
				in_it !=  p_rhn->m_alternates.end();++in_it) 
		{
			strings.push_back((*in_it)->toJsonString());
		}
		r_resourceTableStrings.push_back(std::pair<std::string,std::vector<std::string> >(it->first,strings));
	}
	return true;
}

bool MimeSystem::dbg_getRedirectTableStrings(std::vector<std::pair<std::string,std::vector<std::string> > >& r_redirectTableStrings)
{
	MutexLocker locker(&m_mutex);
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();
	it != m_redirectHandlerMap.end();++it) 
	{
		RedirectHandlerNode * p_rhn = it->second;
		std::vector<std::string> strings;
		strings.push_back(p_rhn->m_redirectHandler.toJsonString());
		for (std::vector<RedirectHandler *>::iterator in_it = p_rhn->m_alternates.begin();
				in_it !=  p_rhn->m_alternates.end();++in_it) 
		{
			strings.push_back((*in_it)->toJsonString());
		}
		r_redirectTableStrings.push_back(std::pair<std::string,std::vector<std::string> >(it->first,strings));
	}
	return true;
}

void MimeSystem::dbg_printVerbCacheTableForResource(const std::string& mime)
{
	MutexLocker locker(&m_mutex);
	ResourceHandlerNode * p_rhn = getResourceHandlerNode(mime);
	if (p_rhn == NULL) {
		printf("didn't find any nodes for mime type %s\n",mime.c_str());
		return;
	}
	dbg_printVerbCacheTable(&(p_rhn->m_verbCache));
	
}

void MimeSystem::dbg_printVerbCacheTableForRedirect(const std::string& url)
{
	MutexLocker locker(&m_mutex);
	RedirectHandlerNode * p_rhn = this->getRedirectHandlerNode(url);
	if (p_rhn == NULL) {
		printf("didn't find any nodes for redirect(url) %s\n",url.c_str());
		return;
	}
	dbg_printVerbCacheTable(&(p_rhn->m_verbCache));
}

void MimeSystem::dbg_printVerbCacheTableForScheme(const std::string& url)
{
	MutexLocker locker(&m_mutex);
	RedirectHandlerNode * p_rhn = this->getSchemeHandlerNode(url);
	if (p_rhn == NULL) {
		printf("didn't find any nodes for redirect(scheme) %s\n",url.c_str());
		return;
	}
	dbg_printVerbCacheTable(&(p_rhn->m_verbCache));
}

//static		(CALL UNDER PROPER LOCK FOR p_verbCacheTable) 
void MimeSystem::dbg_printVerbCacheTable(const std::map<std::string,MimeSystem::VerbCacheEntry> * p_verbCacheTable)
{
	if (p_verbCacheTable == NULL)
		return;
	
	for (std::map<std::string,VerbCacheEntry>::const_iterator it = p_verbCacheTable->begin();
			it != p_verbCacheTable->end();++it)
	{
		std::string verbStr = it->first;
		printf("VerbCacheEntry: [%s],\t\t\tuseCount = %u,\t\tactiveIndex = %u\n",verbStr.c_str(),it->second.useCount,it->second.activeIndex);
	}
}

//static 		(CALL UNDER PROPER LOCK FOR p_resourceHandlerNode) 
void MimeSystem::dbg_printResourceHandlerNode(const ResourceHandlerNode * p_resourceHandlerNode,int level) 
{
	
}
	
//static 		(CALL UNDER PROPER LOCK FOR p_redirectHandlerNode) 
void MimeSystem::dbg_printRedirectHandlerNode(const RedirectHandlerNode * p_redirectHandlerNode,int level)
{
	
}
	
// ---------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------- private ---------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

MimeSystem::MimeSystem() 
{
	
}

//virtual 
MimeSystem::~MimeSystem()
{
	destroy();
}

void MimeSystem::destroy()
{
	MutexLocker locker(&m_mutex);
	m_extensionToMimeMap.clear();
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();
		it != m_redirectHandlerMap.end();++it) 
		delete it->second;
	m_redirectHandlerMap.clear();
	
	for (ResourceMapIterType it = m_resourceHandlerMap.begin();
		it != m_resourceHandlerMap.end();++it) 
		delete it->second;
	m_resourceHandlerMap.clear();
	
	MutexLocker lock_index(&s_mutex);
	s_indexRecycler.clear();
	s_genIndex = 1;
	s_lastAssignedIndex = 0;
	
}

void MimeSystem::reclaimIndex(uint32_t idx)
{
	MutexLocker lock(&s_mutex);
	s_indexRecycler.push_back(idx);
}

//static 
int MimeSystem::addVerbs(const std::map<std::string,std::string>& verbs,ResourceHandlerNode& resourceHandlerNode,ResourceHandler& newHandler)
{
	int rc=0;
	for (std::map<std::string,std::string>::const_iterator it = verbs.begin();
			it != verbs.end();++it) 
	{
		//add the verb to the handler...if not successful, then skip the rest
		if (!newHandler.addVerb(it->first,it->second))
			continue;
		
		//if the verb is not already in the verbCache, add it 
		std::map<std::string,VerbCacheEntry>::iterator vce_it= resourceHandlerNode.m_verbCache.find(it->first);
		if (vce_it == resourceHandlerNode.m_verbCache.end())
		{
			resourceHandlerNode.m_verbCache[it->first] = VerbCacheEntry(newHandler.index());
		}
		else {
			++(vce_it->second.useCount);
		}
		++rc;
	}
	
	return rc;
}

//static 
int MimeSystem::addVerbs(const std::map<std::string,std::string>& verbs,RedirectHandlerNode& redirectHandlerNode,RedirectHandler& newHandler)
{
	int rc=0;
	for (std::map<std::string,std::string>::const_iterator it = verbs.begin();
	it != verbs.end();++it) 
	{
		//add the verb to the handler...if not successful, then skip the rest
		if (!newHandler.addVerb(it->first,it->second))
			continue;

		//if the verb is not already in the verbCache, add it 
		std::map<std::string,VerbCacheEntry>::iterator vce_it= redirectHandlerNode.m_verbCache.find(it->first);
		if (vce_it == redirectHandlerNode.m_verbCache.end())
		{
			redirectHandlerNode.m_verbCache[it->first] = VerbCacheEntry(newHandler.index());
		}
		else {
			++(vce_it->second.useCount);
		}
		++rc;
	}

	return rc;
}

//static 
int MimeSystem::extractVerbsFromHandlerEntryJson(struct json_object * jsonHandlerEntry,std::map<std::string,std::string>& r_verbs)
{
	
	/*{"primary":{"mime":"audio\/3gpp","extension":"3gpp", "appId":"com.palm.app.streamingmusicplayer","streamable":true,"index":67,"tag":"system-default",  "verbs":{"play": "{}"}},
	 * "alternates":[
	 * 		{"mime":"audio\/3gpp","appId":"com.palm.app.badplayer","streamable":true,"index":68 ,  "verbs":{"play": "{}","run": "{\"param\":\"value\"}"}} 
	 * 				],
	 * "verbs":[
	 * 		{"verb":"play" , "index":67}
	 * 		   ]
	 * }
	 */
	
	if ((jsonHandlerEntry == NULL) || (is_error(jsonHandlerEntry)))
		return 0;
	
	json_object * verbObj = json_object_object_get(jsonHandlerEntry,"verbs");
	if ((!verbObj) || (is_error(verbObj)))
		return 0;
	if (json_object_is_type(verbObj,json_type_object) == false)
		return 0;
	
	int rc=0;

	json_object_object_foreach(verbObj, key, val) 
	{
		std::string strKey = key;					//TODO: broken out for debugging; condense.
		std::string strVal;
		if (json_object_is_type(val,json_type_string) == false) {
			strVal = json_object_to_json_string(val);
		}
		else {
			strVal = json_object_get_string(val);
		}
		r_verbs[strKey] = strVal; 
		++rc;
	}

	return rc;
}

//static 
int MimeSystem::extractVerbsFromHandlerNodeEntryJson(struct json_object * jsonHandlerNodeEntry,std::map<std::string,uint32_t>& r_verbs)
{
	
	/*{"primary":{"mime":"audio\/3gpp","extension":"3gpp", "appId":"com.palm.app.streamingmusicplayer","streamable":true,"index":67,"tag":"system-default",  "verbs":{"play": "{}"}},
	 * "alternates":[
	 * 		{"mime":"audio\/3gpp","appId":"com.palm.app.badplayer","streamable":true,"index":68 ,  "verbs":{"play": "{}","run": "{\"param\":\"value\"}"}} 
	 * 				],
	 * "verbs":[
	 * 		{"verb":"play" , "index":67}
	 * 		   ]
	 * }
	 */

	if ((jsonHandlerNodeEntry == NULL) || (is_error(jsonHandlerNodeEntry)))
		return 0;

	json_object * arrayLabel = json_object_object_get(jsonHandlerNodeEntry,"verbs");
	array_list * srcJsonArray = json_object_get_array(arrayLabel);
	if ((!srcJsonArray) || (is_error(srcJsonArray))) {
		return 0;
	}
	int rc=0;
	for (int i = 0; i < array_list_length(srcJsonArray); i++) {
		json_object* obj = (json_object*) array_list_get_idx(srcJsonArray, i);
		if (!(json_object_is_type(obj,json_type_string)))
			continue;
		std::string verb;
		uint32_t idx;
		if (extractFromJson(obj,"verb",verb) == false)
			continue;
		json_object * label = json_object_object_get(obj,"index");
		if ((!label) || (is_error(label)))
			continue;
		idx = json_object_get_int(label);
		r_verbs[verb] = idx;
		++rc;
	}
	return rc;
}

MimeSystem::ResourceHandlerNode * MimeSystem::getResourceHandlerNode(const std::string& mimeType)
{
	MutexLocker lock(&m_mutex);
	ResourceMapIterType it = m_resourceHandlerMap.find(mimeType);
	if (it != m_resourceHandlerMap.end()) {
		return it->second;
	}
	return NULL;	//return invalid object (see ResourceHandler::valid() )
		
}

MimeSystem::RedirectHandlerNode * MimeSystem::getRedirectHandlerNode(const std::string& url)
{
	MutexLocker lock(&m_mutex);
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		if (it->second->m_redirectHandler.isSchemeForm())
			continue;
		//try and match against it
		if (it->second->m_redirectHandler.matches(url))
			return it->second;
	}
	return NULL;
		
}

MimeSystem::RedirectHandlerNode * MimeSystem::getSchemeHandlerNode(const std::string& url)
{
	MutexLocker lock(&m_mutex);
	for (RedirectMapIterType it = m_redirectHandlerMap.begin();it != m_redirectHandlerMap.end();++it) {
		if (it->second->m_redirectHandler.isSchemeForm() == false)
			continue;
		//try and match against it
		if (it->second->m_redirectHandler.matches(url))
			return it->second;
	}
	return NULL;
}

