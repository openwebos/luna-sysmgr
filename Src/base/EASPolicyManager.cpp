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

#include "EASPolicyManager.h"

#include "HostBase.h"
#include "Security.h"
#include "SystemService.h"
#include "Preferences.h"

#include <glib.h>
#include <math.h>
#include <stdlib.h>


/*

Version 1
 
{
 	password: {
		enabled: <bool>, 		// is a password required?
		minLength: <int>,		// < 2 implies no minLength
		maxRetries: <int>,		// 0 implies unlimited retries / no lockout
		alphaNumeric: <bool>	// true = requires password, false = allow pin
	},
	inactivityInSeconds: <int>, // > 9998 means set to 0
	id: <string>,
	status: {
		enforced: <bool>,		// policy has been enforced completely
		retriesLeft: <int>		// only meaningful if maxRetries > 0
	}
}

Version 2

{
	version: <int>,

	status: {
		enforced: <bool>,
		retriesLeft: <int>
	},

	policies: [
		{
			password: {
				enabled: <bool>,
				minLength: <int>,
				maxRetries: <int>,
				alphaNumeric: <bool>
			},
			inactivityInSeconds: <int>,
			id: <string>
		},
		...
	]
}

Version 3 - mojodb file.
Device Policy fields defined in: https://wiki.palm.com/display/Nova/Device+Policy+Fields

*/

static const char* s_policyFile = "/var/luna/data/.policy";
static EASPolicyManager* s_instance = 0;
static const unsigned int s_version = 2;


EASPolicyManager* EASPolicyManager::instance()
{
	if (G_UNLIKELY(s_instance == 0))
		new EASPolicyManager;
	return s_instance;
}

EASPolicyManager::EASPolicyManager()
	: m_service(0)
	, m_isEnforced(false)
	, m_retriesLeft(0)
	, m_aggregate(0)
	, m_lastRev(0)
	, m_callToken(0)
{
	s_instance = this;
}

EASPolicyManager::~EASPolicyManager()
{
	s_instance = 0;
}


void EASPolicyManager::removeTemporaryPolicies()
{
    if (!m_service) {
		g_warning ("Service handle not available yet, cannot query device policy");
		return;
    }

    LSError lserror;
    LSErrorInit (&lserror);

    gchar* query = g_strdup ("{\"query\":{\"from\":\"com.palm.securitypolicy:1\", \"where\":[{\"prop\":\"isTemp\",\"op\":\"=\",\"val\":true}]}}");
    g_debug ("%s: Calling %s with %s", __func__, "palm://com.palm.db/del", query);
    bool result = LSCallOneReply (m_service, "palm://com.palm.db/del", query, &EASPolicyManager::cbTempPoliciesDeleted, this, NULL, &lserror);
    if (!result) {
		g_warning ("%s: Failed at %s with message %s", __func__, lserror.func, lserror.message);
		LSErrorFree (&lserror);
    }
    g_free (query);
}

bool EASPolicyManager::cbTempPoliciesDeleted (LSHandle *sh, LSMessage *message, void *data)
{
    const char* str = LSMessageGetPayload(message);
    json_object *root = 0, *returnValue = 0, *obj = 0;
    bool success = false;
    int count = 0;

    if (!str)
	    goto done;

    g_debug ("%s: response %s", __func__, str);

    root = json_tokener_parse(str);
    if (!root || is_error(root))
		goto done;
    
    returnValue = json_object_object_get (root, "returnValue");
    if (!returnValue || is_error(returnValue)) {
	    g_warning ("%s: check for returnValue failed", __func__);
	    goto done;
    }

    success = json_object_get_boolean (returnValue);
    if (!success) {
	    g_warning ("%s: delete of temporary policies failed", __func__);
	    goto done;
    }

    obj = json_object_object_get (root, "count");
    if (!obj || is_error(obj)) {
	    g_warning ("%s: count missing", __func__);
	    goto done;
    }

    count = json_object_get_int (obj);
    g_message ("%s: Deleted %d temporary policies", __func__, count);

done:
    if (root && !is_error(root))
	    json_object_put (root);
    // temporary policies cleared, query device policy now
    EASPolicyManager::instance()->queryDevicePolicy();
    return true;
}

void EASPolicyManager::queryDevicePolicy()
{
    if (!m_service) {
		g_warning ("Service handle not available yet, cannot query device policy");
		return;
    }

    LSError lserror;
    LSErrorInit (&lserror);

    gchar* query = g_strdup ("{\"query\":{\"from\":\"com.palm.securitypolicy.device:1\"}}");
    bool result = LSCallOneReply (m_service, "palm://com.palm.db/find", query, &EASPolicyManager::cbDevicePolicy, this, NULL, &lserror);
    if (!result) {
		g_warning ("%s: Failed at %s with message %s", __func__, lserror.func, lserror.message);
		LSErrorFree (&lserror);
    }
    g_free (query);
}

bool EASPolicyManager::cbDevicePolicy (LSHandle *sh, LSMessage *message, void *data)
{
    const char* str = LSMessageGetPayload(message);
    json_object *root = 0, *results = 0, *policy = 0;


    if (!str)
		goto done;

    root = json_tokener_parse(str);
    if (!root || is_error(root))
		goto done;
    
    results = json_object_object_get (root, "results");
    if (!results || json_object_array_length (results) == 0) {
		g_debug ("No security policies set, setting a default policy");
		EASPolicyManager::instance()->setDevicePolicy();
		goto done;
    }

    if (json_object_array_length (results) > 1) {
		g_warning ("Device policy > 1, BUG! Using the 1st");
    }

    policy  = json_object_array_get_idx (results, 0);
    if (!policy) {
		g_warning ("No device policy at index 0, BUG!");
		goto done;
    }

    EASPolicyManager::instance()->setDevicePolicy (policy);

done:
    EASPolicyManager::instance()->querySecurityPolicies();

	if (root && !is_error(root))
		json_object_put (root);

    return true;
}

void EASPolicyManager::setDevicePolicy (json_object* policy)
{
	if (m_aggregate)
		delete m_aggregate;

	// resetting the aggregate before parsing the security policies again.
	// no special handling required for changes or policy decisions
	m_lastRev = 0;
	m_aggregate = new EASPolicy(true);

	if (policy) {
		m_aggregate->fromNewJSON (policy);

		json_object* prop = json_object_object_get(policy, "status");
		if (prop && !is_error(prop)) {

			json_object* key = json_object_object_get(prop, "enforced");
			m_isEnforced = (key == 0 ? false : json_object_get_boolean(key));


			key = json_object_object_get(prop, "retriesLeft");
			if (key)
				m_retriesLeft = json_object_get_int(key);
			else if (m_aggregate)
				m_retriesLeft = m_aggregate->maxRetries();
		}
		else {
			m_isEnforced = false;
			if (m_aggregate)
				m_retriesLeft = m_aggregate->maxRetries();
		}

	}
	else {
		// if no device policy is available, save the default policy
		notifyPolicyChanged();
	}

}

void EASPolicyManager::querySecurityPolicies()
{
    if (!m_service) {
		g_warning ("Service handle not available yet, cannot query device policy");
		return;
    }

    LSError lserror;
    LSErrorInit (&lserror);

    gchar* query = g_strdup ("{\"query\":{\"from\":\"com.palm.securitypolicy:1\", \"incDel\" : true}}");
    bool result = LSCallOneReply (m_service, "palm://com.palm.db/find", query, &EASPolicyManager::cbSecurityPolicy, this, NULL, &lserror);
    if (!result) {
		g_warning ("%s: Failed at %s with message %s", __func__, lserror.func, lserror.message);
		LSErrorFree (&lserror);
    }
    g_free (query);
}


bool EASPolicyManager::cbSecurityPolicy (LSHandle *sh, LSMessage *message, void *data)
{
    const char* str = LSMessageGetPayload(message);
    json_object *root = 0, *results = 0, *policy = 0;

    if (!str)
		goto done;

    root = json_tokener_parse(str);
    if (!root || is_error(root))
		goto done;
    
    results = json_object_object_get (root, "results");
    if (!results) {
		g_debug ("No security policies set, no processing needed");
		goto done;
    }

    EASPolicyManager::instance()->updateDevicePolicy (results);

done:
	
    if (root && !is_error(root))
		json_object_put (root);

    return true;
}


void EASPolicyManager::updateDevicePolicy (json_object* policies)
{
    EASPolicy* oldPolicy = m_aggregate;
    int index;

    // resetting the aggregate before parsing the security policies again.
    // no special handling required for changes or policy decisions
    m_lastRev = 0;
    m_aggregate = new EASPolicy(true);
    if (oldPolicy) // inherit the id from the old policy
		m_aggregate->m_id = oldPolicy->m_id;

    bool updated = false;

    for (index = 0; index < json_object_array_length (policies); ++index) {
		json_object* policy = json_object_array_get_idx(policies, index);
		if (!policy) {
		    g_warning ("Index %d has no security policy", index);
	    	continue;
		}
		g_debug ("Importing policy %s", json_object_get_string(policy));

		EASPolicy newPolicy;
		newPolicy.fromNewJSON (policy);
		if (!newPolicy.m_isDevicePolicy && !newPolicy.m_isDeleted)
			m_aggregate->merge (&newPolicy);

		if (newPolicy.m_revision > m_lastRev) {
		    m_lastRev = newPolicy.m_revision;
		}
    }

	if (oldPolicy) {
		if (oldPolicy->maxRetries() != m_aggregate->maxRetries()) {
			g_debug("New policy has different number of retries: old (%d), new (%d)",
					oldPolicy->maxRetries(), m_aggregate->maxRetries());
			m_retriesLeft = m_aggregate->maxRetries();
		}
		else {
			g_debug("Not updating retriesLeft");
		}
	}
	else
		m_retriesLeft = m_aggregate->maxRetries();

    if (oldPolicy && *oldPolicy == *m_aggregate) {
	    watchSecurityPolicies(); // policy not changed, reset watch here
	    delete oldPolicy;
    }
    else {
	    notifyPolicyChanged();
	    if (oldPolicy)
		    delete oldPolicy;
	    // watch will be set after saving the new policy
    }
}


void EASPolicyManager::watchSecurityPolicies()
{
    LSError lserror;
    LSErrorInit (&lserror);

    if (m_callToken) {
	    g_debug ("Cancelling call token %lu", m_callToken);
	    if (LSCallCancel (m_service, m_callToken, &lserror)) {
		    g_warning ("Unable to cancel call with token %lu error message %s", m_callToken, lserror.message);
	    }
	    m_callToken = 0;
    }


    gchar* query = g_strdup_printf ("{\"query\":{\"from\":\"com.palm.securitypolicy:1\",\"where\":[{\"prop\":\"_rev\",\"op\":\">\",\"val\":%d}], \"incDel\": true }}", m_lastRev);
    g_message ("Setting watch with paramters %s", query);
    bool result = LSCall (m_service, "palm://com.palm.db/watch", query, &EASPolicyManager::cbWatchResponse, this,&m_callToken, &lserror);
    if (!result) {
		g_warning ("%s: Failed at %s with message %s", __func__, lserror.func, lserror.message);
		LSErrorFree (&lserror);
    }
    g_free (query);
}


bool EASPolicyManager::cbWatchResponse (LSHandle *sh, LSMessage *message, void *data)
{
    const char* str = LSMessageGetPayload(message);
    json_object *root = 0, *label = 0;
    bool returnValue = false;
    bool fired = false;

    if (!str)
		goto error;
    g_debug ("%s: response %s", __func__, str);

    root = json_tokener_parse(str);
    if (!root || is_error(root))
		goto error;
    
    label = json_object_object_get (root, "returnValue");
    if (!label || is_error(label))
		goto error;

    returnValue = json_object_get_boolean (label);
    if (!returnValue) {
		g_warning ("%s: returnValue is false", __func__);
		goto error;
    }

    label = json_object_object_get (root, "fired");
    if (!label || is_error(label)) {
		goto error;
    }

    fired = json_object_get_boolean (label);
    if (!fired) {
		g_warning ("%s: fired is false", __func__);
		goto error;
    }

    EASPolicyManager::instance()->m_callToken = 0; // resetting call token after watch has fired

    g_debug ("%s: Watch on security policy fired, query security policy", __func__);
    EASPolicyManager::instance()->querySecurityPolicies();

error:

	if (root && !is_error(root))
		json_object_put (root);
    
    return true;
}


bool EASPolicyManager::load()
{
	m_service = SystemService::instance()->serviceHandle();

    if (!m_service) {
		g_warning ("system service handle is missing");
		return false;
    }

    if (!m_aggregate)
		m_aggregate = new EASPolicy (true);

    if (!m_aggregate)
		return false;

#if !defined(TARGET_DESKTOP) 
    removeTemporaryPolicies();
    // clean up temporary policies before restoring current policies

#else
    setDevicePolicy();
#endif

    return true;
}


bool EASPolicyManager::importOldPolicySettings()
{
	json_object *root = 0, *prop = 0, *key = 0;
	root = json_object_from_file((char*)s_policyFile);
	if (!root || is_error(root))
		return false;


	prop = json_object_object_get(root, "version");
	if (!prop) {

		g_message("converting version 1 schema to version %d", s_version);

		// migrate version 1 schema
		EASPolicy* p = new EASPolicy;
		if(p->fromJSON(root))
		    m_aggregate->merge (p);
	}
	else if (json_object_get_int(prop) == 2) {

		// parse version 2 schema
		g_message("parsing version 2 schema");
		
		// parse all policies and create aggregate
		prop = json_object_object_get(root, "policies");
		if (prop && !is_error(prop) && json_object_is_type(prop, json_type_array)) {

			for (int i = 0; i < json_object_array_length(prop); i++) {

				EASPolicy* p = new EASPolicy;
				key = json_object_array_get_idx(prop, i);
				if (p->fromJSON(key))
				    m_aggregate->merge(p);
                                delete p;
			}
		}
	}
	else {
		g_critical("unrecognized EAS policy schema version %d", json_object_get_int(prop));
		return false;
	}
	
	// status
	prop = json_object_object_get(root, "status");
	if (prop && !is_error(prop)) {
		
		key = json_object_object_get(prop, "enforced");
		m_isEnforced = (key == 0 ? false : json_object_get_boolean(key));

		
		key = json_object_object_get(prop, "retriesLeft");
		if (key)
			m_retriesLeft = json_object_get_int(key);
		else if (m_aggregate)
			m_retriesLeft = m_aggregate->maxRetries();
	}
	else {
		m_isEnforced = false;
		if (m_aggregate)
			m_retriesLeft = m_aggregate->maxRetries();
	}
	
	// check the installed policies against the set of email accounts

    json_object_put(root);
	return true;
}

void EASPolicyManager::save()
{
	if (!m_service) {
		g_warning ("Service handle not available yet, cannot query device policy");
		return;
	}

	LSError lserror;
	LSErrorInit (&lserror);

	if (m_callToken) {
		g_debug ("Cancelling call token %lu", m_callToken);
		if (LSCallCancel (m_service, m_callToken, &lserror)) {
			g_warning ("Unable to cancel call with token %lu error message %s", m_callToken, lserror.message);
		}
		m_callToken = 0;
	}

	json_object* policyJson = m_aggregate->toNewJSON();
	json_object_object_add(policyJson, "status", getPolicyStatus());
	gchar* policyStr = g_strdup_printf ("{ \"objects\":[%s]}", json_object_to_json_string (policyJson));
	g_message ("%s: Writing device policy %s to mojodb", __func__, policyStr);

	if (m_aggregate->m_id.empty()) {

	    if (!LSCallOneReply (m_service, "palm://com.palm.db/put", policyStr, 
							 &EASPolicyManager::cbDevicePolicySaved, this, NULL, &lserror)) 
		{
			g_warning ("%s: Failed at %s with message %s", __func__, lserror.func, lserror.message);
			LSErrorFree (&lserror);
	    }
	}
	else {

	    if (!LSCallOneReply (m_service, "palm://com.palm.db/merge", policyStr, 
				    &EASPolicyManager::cbDevicePolicySaved, this, NULL, &lserror)) {
			g_warning ("%s: Failed at %s with message %s", __func__, lserror.func, lserror.message);
			LSErrorFree (&lserror);
	    }
	}

	json_object_put (policyJson);
	g_free (policyStr);
}

bool EASPolicyManager::cbDevicePolicySaved (LSHandle *sh, LSMessage *message, void *data)
{
    const char* str = LSMessageGetPayload(message);
    json_object *root = 0, *label = 0, *policy = 0, *results = 0;
    bool returnValue = false;
    int rev = 0;
    std::string id;

    if (!str)
		goto error;

    g_debug ("%s: response %s", __func__, str);

    root = json_tokener_parse(str);
    if (!root || is_error(root))
		goto error;

    label = json_object_object_get (root, "returnValue");
    if (!label)  {
		g_warning ("No returnValue available");
		goto error;
    }

    returnValue = json_object_get_boolean (label);
    if (!returnValue) {
		g_warning ("returnValue is false, call failed");
		goto error;
    }

    results = json_object_object_get (root, "results");
    if (!results) {
		g_warning ("No results in device policy, call to store device policy failed");
		goto error;
    }

    if (json_object_array_length (results) != 1) {
		g_warning ("Device policy != 1, BUG! Using the 1st");
    }

    policy = json_object_array_get_idx(results, 0);
    if (!policy) {
		g_warning ("No policy in list, cannot update id");
		goto error;
    }

    label = json_object_object_get (policy, "id");
    if (!label)  {
		g_warning ("No id available");
		goto error;
    }

    id = json_object_get_string (label);
    if (id.empty()) {
		g_warning ("Invalid id");
		goto error;
    }

    label = json_object_object_get (policy, "rev");
    if (!label)  {
		g_warning ("No rev available");
		goto error;
    }
 
    rev = json_object_get_int (label);
    if (rev <= 0) {
		g_warning ("Invalid rev");
		goto error;
    }

    if (EASPolicyManager::instance()->m_aggregate->m_id != id) {
	    EASPolicyManager::instance()->m_aggregate->m_id = id;
	    g_debug ("%s: updated id to %s", __func__, id.c_str());
    }

    // do not update the watch rev after saving the device policy
    // otherwise the watch will miss upates that might have
    // occured between the last query of the security policies
    // and the saving of this device policy.
    // Note, this will cause the watch to always get triggered
    // once after saving a device policy, due to the updated 
    // revision of the device policy

error:
    EASPolicyManager::instance()->watchSecurityPolicies();

    if (root && !is_error(root))
		json_object_put (root);
    return true;
}


void EASPolicyManager::notifyPolicyChanged()
{
	// immediately enforce policy?
	m_isEnforced = Security::instance()->passcodeSatisfiesPolicy(m_aggregate);

	Q_EMIT signalPolicyChanged(m_aggregate);

	g_message("policy enforcement succeeded? %d", m_isEnforced);

	SystemService::instance()->postDeviceLockMode();

#if !defined(TARGET_DESKTOP)
	save();
#endif
}


std::string EASPolicyManager::getPolicyState() const
{
	if (m_aggregate == 0)
		return "none";
	return m_isEnforced ? "active" : "pending";
}

json_object* EASPolicyManager::getPolicyStatus() const
{
	if (m_aggregate == 0)
		return 0;

	json_object* status = json_object_new_object();
	json_object_object_add(status, "enforced", json_object_new_boolean(m_isEnforced));
	json_object_object_add(status, "retriesLeft", json_object_new_int(m_retriesLeft));

	return status;
}

void EASPolicyManager::passwordEnforced()
{
	if (!policyPending())
		return;

	m_isEnforced = true;
	m_retriesLeft = m_aggregate->maxRetries();

	Q_EMIT signalPolicyChanged(m_aggregate);

	save();
}

uint32_t EASPolicyManager::retriesLeft() const
{
	if (m_aggregate == 0 || !m_aggregate->validMaxRetries() || !m_isEnforced)
		return 0;
	return m_retriesLeft;
}

int EASPolicyManager::decrementRetries()
{
	if (m_aggregate == 0 || !m_aggregate->validMaxRetries() || !m_isEnforced)
		return 0;

	if (m_retriesLeft > 0) {
		m_retriesLeft--;
	}

	save();
	return m_retriesLeft;
}

int EASPolicyManager::resetRetries()
{
	if (m_aggregate == 0 || !m_aggregate->validMaxRetries() || !m_isEnforced )
		return 0;
	
	m_retriesLeft = m_aggregate->maxRetries();

	save();
	return m_retriesLeft;
}

/*********************************************************************************************/

json_object* EASPolicy::toNewJSON() const
{
	json_object* policy = json_object_new_object();

	json_object_object_add(policy, "devicePasswordEnabled", json_object_new_boolean(m_passwordRequired));
	json_object_object_add(policy, "minDevicePasswordLength", json_object_new_int(m_minLength));
	json_object_object_add(policy, "maxDevicePasswordFailedAttempts", json_object_new_int(m_maxRetries));
	json_object_object_add(policy, "alphanumericDevicePasswordRequired", json_object_new_boolean(m_isAlphaNumeric));
	json_object_object_add(policy, "allowSimpleDevicePassword", json_object_new_boolean(m_allowSimplePassword));

	json_object_object_add(policy, "maxInactivityTimeDeviceLock", json_object_new_int(m_inactivityInSeconds));

	if (!m_id.empty()) {
	    json_object_object_add(policy, "_id", json_object_new_string(m_id.c_str()));
	}
	if (m_isDevicePolicy)
	    json_object_object_add(policy, "_kind", json_object_new_string("com.palm.securitypolicy.device:1"));

	return policy;
}

json_object* EASPolicy::toJSON() const
{
	json_object* policy = json_object_new_object();

	json_object* password = json_object_new_object();
	json_object_object_add(password, "enabled", json_object_new_boolean(m_passwordRequired));
	json_object_object_add(password, "minLength", json_object_new_int(m_minLength));
	json_object_object_add(password, "maxRetries", json_object_new_int(m_maxRetries));
	json_object_object_add(password, "alphaNumeric", json_object_new_boolean(m_isAlphaNumeric));
	if (m_passwordRequired && !m_isAlphaNumeric) {
		json_object_object_add(password, "allowSimplePassword", json_object_new_boolean(m_allowSimplePassword));
	}

	json_object_object_add (policy, "password", password);

	json_object_object_add(policy, "inactivityInSeconds", json_object_new_int(m_inactivityInSeconds));

	json_object_object_add(policy, "id", json_object_new_string(m_id.c_str()));

	return policy;
}

bool EASPolicy::fromJSON(json_object* policy)
{
    json_object *prop = 0, *key = 0;

	prop = json_object_object_get(policy, "password");
	if (prop && !is_error(prop)) {
		
		key = json_object_object_get(prop, "enabled");
		m_passwordRequired = json_object_get_boolean(key);
		

		key = json_object_object_get(prop, "minLength");
		if (key)
			m_minLength = json_object_get_int(key);
		else
			m_minLength = 1;

		key = json_object_object_get(prop, "maxRetries");
		if (key)
			m_maxRetries = json_object_get_int(key);
		else
			m_maxRetries = 1;

		key = json_object_object_get(prop, "alphaNumeric");
		m_isAlphaNumeric = json_object_get_boolean(key);
	}
	else {
		m_passwordRequired = false;
		m_minLength = 1;
		m_maxRetries = 1;
		m_isAlphaNumeric = false;
	}

	key = json_object_object_get(policy, "inactivityInSeconds");
	if (key) {
		m_inactivityInSeconds = json_object_get_int(key);
	}
	else {
		m_inactivityInSeconds = 0;
	}

	key = json_object_object_get(policy, "id");
	if (key) {
		char* str = json_object_get_string(key);
		m_id = (str != NULL ? str : "");
	}

	return !m_id.empty();
}

bool EASPolicy::fromNewJSON(json_object* policy)
{
    json_object *key = 0;

	key = json_object_object_get(policy, "devicePasswordEnabled");
	if (key)
	    m_passwordRequired = json_object_get_boolean(key);
	else
	    m_passwordRequired = false;

	key = json_object_object_get(policy, "alphanumericDevicePasswordRequired");
	if (key)
	    m_isAlphaNumeric = json_object_get_boolean(key);
	else 
	    m_isAlphaNumeric = false;

	key = json_object_object_get(policy, "minDevicePasswordLength");
	if (key)
	    m_minLength = json_object_get_int(key);
	else
	    m_minLength = 1;

	key = json_object_object_get(policy, "maxDevicePasswordFailedAttempts");
	if (key)
	    m_maxRetries = json_object_get_int(key);
	else
	    m_maxRetries = 1;

	key = json_object_object_get(policy, "maxInactivityTimeDeviceLock");
	if (key)
		m_inactivityInSeconds = json_object_get_int(key);
	else 
		m_inactivityInSeconds = 0;

	key = json_object_object_get(policy, "allowSimpleDevicePassword");
	if (key)
		m_allowSimplePassword = json_object_get_boolean(key);
	else
		m_allowSimplePassword = true;

	key = json_object_object_get(policy, "_id");
	if (key) {
		char* str = json_object_get_string(key);
		m_id = (str != NULL ? str : "");
	}

	if (!m_id.empty())
		key = json_object_object_get (policy, "_rev");

	if (key)
	    m_revision = json_object_get_int (key);
	else
	    m_revision = 0;
	
	key = json_object_object_get(policy, "_kind");
	if (key) {
		std::string kind = json_object_get_string(key);
		if (kind == "com.palm.securitypolicy.device:1")
		    m_isDevicePolicy = true;
		else 
		    m_isDevicePolicy = false;
	}
	else {
	    m_isDevicePolicy = false;
	}

	key = json_object_object_get(policy, "_del");
	if (key) {
		m_isDeleted = json_object_get_boolean(key);
	}
	else {
		m_isDeleted = false;
	}

	return !m_id.empty();
}

uint32_t EASPolicy::maxInactivityInSeconds() const
{
	if (m_inactivityInSeconds < 60) {

		// round to the lowest 30 second interval
		return m_inactivityInSeconds - (m_inactivityInSeconds % 30);
	}
	else if (m_inactivityInSeconds < 9999) {

		// round to the lowest 60 second interval
		return m_inactivityInSeconds - (m_inactivityInSeconds % 60);
	}
	return 0; // default to the strictest timeout
}

uint32_t EASPolicy::clampInactivityInSeconds(uint32_t inactivityInSeconds) const
{
	// clamp to a valid value
	uint32_t maxTimeout = maxInactivityInSeconds();
	if (validInactivityInSeconds() && inactivityInSeconds >= maxTimeout)
		return maxTimeout;
	else
		return Preferences::roundLockTimeout(inactivityInSeconds);
}

void EASPolicy::merge(const EASPolicy* newPolicy)
{
	// update password
	if (!this->m_passwordRequired && newPolicy->m_passwordRequired) {

		// first time we are trying to enforce a password
		this->m_passwordRequired = newPolicy->m_passwordRequired;
		this->m_maxRetries = newPolicy->m_maxRetries;
		this->m_minLength = newPolicy->m_minLength;
		this->m_isAlphaNumeric = newPolicy->m_isAlphaNumeric;
		this->m_allowSimplePassword = newPolicy->m_allowSimplePassword;

		// inactivity should be ignored if no password is required
		this->m_inactivityInSeconds = newPolicy->maxInactivityInSeconds();
	}
	else if (this->m_passwordRequired && newPolicy->m_passwordRequired) {

		// update inactivity timeout
		if (newPolicy->m_inactivityInSeconds < this->m_inactivityInSeconds) {
			this->m_inactivityInSeconds = newPolicy->maxInactivityInSeconds();
		}

		// possibly update the terms of password enforcement
		if (newPolicy->validMaxRetries()) {
			if (!this->validMaxRetries() || newPolicy->m_maxRetries < this->m_maxRetries) {
				this->m_maxRetries = newPolicy->m_maxRetries;
			}
		}

		// alphanumeric is stricter than allowing a PIN
		if (!this->m_isAlphaNumeric && newPolicy->m_isAlphaNumeric) {
			this->m_isAlphaNumeric = newPolicy->m_isAlphaNumeric;
		}

		// not allowing simple device password is stricter
		if (this->m_allowSimplePassword && !newPolicy->m_allowSimplePassword) {
			this->m_allowSimplePassword = newPolicy->m_allowSimplePassword;
		}

		// chose the highest min length
		if (newPolicy->validMinLength() && newPolicy->m_minLength > this->m_minLength) {
			this->m_minLength = newPolicy->m_minLength;
		}
	}

}

