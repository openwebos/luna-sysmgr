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




#ifndef SECURITY_H
#define SECURITY_H

#include "Common.h"

#include <string>

#include <lunaservice.h>

#include "cjson/json.h"

#include <QObject>
#include <QString>

class EASPolicy;

class Security : public QObject
{
	Q_OBJECT

public:

	static Security* instance();
	~Security();

	bool passcodeSet() const;

	std::string getLockMode() const { return m_lockMode; };

	int setPasscode(const std::string& mode, const std::string& passcode, std::string& errorText);
	bool matchPasscode(std::string passcode, int& retriesLeft, bool& lockedOut);

	bool passcodeSatisfiesPolicy(const EASPolicy * const policy) const;

	enum FailureCode {
		Success = 0,
		FailureEmptyPasscode = -1,
		FailureMinLength = -2,
		FailureInvalidPassword = -3,
		FailureAlphaNumeric = -4,
		FailureInvalidPin = -5,
		FailureWeakPassword = -6,
		FailureSave = -7,
		FailureWeakPasswordRepeat = -8,
		FailureWeakPasswordSequence = -9
	};

	static bool cbDeviceWipe (LSHandle *sh, LSMessage *message, void *data);
private:

	Security();

	void registerService();

	int validatePasscode(const EASPolicy * const policy, const std::string& mode, 
						  const std::string& passcode) const;
	int validateStrength(QString passcode) const;

	void readLockMode();
	std::string readPasscode() const;
	void readDecryptedPasscode(std::string& passcode) const;
	void safelyEraseString(std::string& str) const;

	void eraseDevice();

	static bool keyManagerConnected(LSHandle* handle, LSMessage* msg, void* ctxt);
	void initKeyManager();
	void updateKeyManager(const std::string& oldPasscode);

private Q_SLOTS:

	void slotPolicyChanged (const EASPolicy * const policy);

private:

	int m_numRetries;
	std::string m_lockMode;
	LSHandle* m_service;
};

#endif
