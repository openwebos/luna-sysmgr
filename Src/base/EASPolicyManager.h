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




#ifndef EASPOLICYMANAGER_H
#define EASPOLICYMANAGER_H

#include "Common.h"

#include <string>
#include "cjson/json.h"
#include "PtrArray.h"

#include <lunaservice.h>

#include <QObject>

class EASPolicyManager;
class EASPolicy
{
public:
	EASPolicy(bool isDevicePolicy = false)
		: m_passwordRequired(false)
		, m_maxRetries(0)
		, m_minLength(1)
		, m_isAlphaNumeric(false)
		, m_allowSimplePassword(true)
		, m_inactivityInSeconds(9998) // highest allowable inactivity timeout
		, m_id("") 
		, m_revision (0)
		, m_isDevicePolicy(isDevicePolicy)
		, m_isDeleted(false)
	{
	}

	bool passwordRequired() const { return m_passwordRequired; }
	bool requiresAlphaNumeric() const { return m_isAlphaNumeric; }
	bool allowSimplePassword() const { return m_allowSimplePassword; }
	uint32_t maxRetries() const { return m_maxRetries; }
	uint32_t minLength() const { return m_minLength; }

	// returns a platform supported value for m_inactivityInSeconds
	uint32_t maxInactivityInSeconds() const;
	uint32_t clampInactivityInSeconds(uint32_t inactivityInSeconds) const;

	const std::string& id() const { return m_id; }
	
	json_object* toJSON() const;
	json_object* toNewJSON() const;
	bool fromJSON(json_object* policy);
	bool fromNewJSON(json_object* policy);

	bool validMaxRetries() const { return m_passwordRequired && m_maxRetries > 1; }
	bool validMinLength() const { return m_passwordRequired && m_minLength > 1; }
	bool validInactivityInSeconds() const { return m_passwordRequired; }

	void merge(const EASPolicy* newPolicy);
	
private:
	bool operator== (const EASPolicy& r) const {
		return (m_passwordRequired == r.m_passwordRequired &&
				m_maxRetries == r.m_maxRetries &&
				m_minLength == r.m_minLength &&
				m_isAlphaNumeric == r.m_isAlphaNumeric &&
				m_allowSimplePassword == r.m_allowSimplePassword &&
				m_inactivityInSeconds == r.m_inactivityInSeconds);
	}

	// policy properties
	bool m_passwordRequired;
	uint32_t m_maxRetries;
	uint32_t m_minLength;
	bool m_isAlphaNumeric;
	bool m_allowSimplePassword;

	uint32_t m_inactivityInSeconds;
	
	std::string m_id;
	int m_revision;
	bool m_isDevicePolicy;
	bool m_isDeleted;

	friend class EASPolicyManager;
};


class EASPolicyManager : public QObject
{
	Q_OBJECT

public:

	static EASPolicyManager* instance();
	~EASPolicyManager();

	bool load();

	bool policyPending() const { return (m_aggregate != 0 && !m_isEnforced); }
	const EASPolicy * const getPolicy() const { return m_aggregate; }

	std::string getPolicyState() const;
	json_object* getPolicyStatus() const;

	void passwordEnforced();

	uint32_t retriesLeft() const;
	int decrementRetries();
	int resetRetries();

	
	void removeTemporaryPolicies();
	void queryDevicePolicy();
	void querySecurityPolicies();
	void setDevicePolicy (json_object* policy = NULL);
	void updateDevicePolicy (json_object* policies);
	void watchSecurityPolicies();
	bool importOldPolicySettings();

	static bool cbSecurityPolicy (LSHandle *sh, LSMessage *message, void *data);
	static bool cbWatchResponse (LSHandle *sh, LSMessage *message, void *data);
	static bool cbDevicePolicy (LSHandle *sh, LSMessage *message, void *data);
	static bool cbDevicePolicySaved (LSHandle *sh, LSMessage *message, void *data);
	static bool cbTempPoliciesDeleted (LSHandle *sh, LSMessage *message, void *data);

Q_SIGNALS:

	void signalPolicyChanged(const EASPolicy* const);

private:

	EASPolicyManager();

	void save();

	void notifyPolicyChanged();

	LSHandle* m_service;

	bool m_isEnforced;
	uint32_t m_retriesLeft;

	EASPolicy* m_aggregate;
	int m_lastRev;
	LSMessageToken m_callToken;
};

#endif
