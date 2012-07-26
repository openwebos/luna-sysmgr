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
#include "Security.h"

#include "EASPolicyManager.h"
#include "SystemService.h"
#include "HostBase.h"

#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "Time.h"
#include "QtUtils.h"

#include <QCryptographicHash>
#include <QDebug>

static const std::string s_passcodeFile = "/var/luna/data/.passcode";
static const uint32_t s_deviceLockOutDuration = 15000; // in milliseconds
static const uint32_t s_defaultMaxRetries = 3;

static Security* s_instance = 0;


Security* Security::instance()
{
	if (G_UNLIKELY(s_instance == 0)) {
		new Security;
	}
	return s_instance;
}

Security::Security()
	: m_numRetries(s_defaultMaxRetries)
	, m_service(0)
{
	s_instance = this;

	readLockMode();

	registerService();

	connect(EASPolicyManager::instance(), SIGNAL(signalPolicyChanged(const EASPolicy* const)),
			this, SLOT(slotPolicyChanged(const EASPolicy* const)));
}

Security::~Security()
{
	s_instance = 0;
}

void Security::registerService()
{
	LSError err;
	LSErrorInit(&err);
	if (LSRegister(NULL, &m_service, &err)) {
		if (LSGmainAttach(m_service, HostBase::instance()->mainLoop(), &err)) {

			LSCall(m_service, "palm://com.palm.bus/signal/registerServerStatus",
				   "{\"serviceName\":\"com.palm.keymanager\"}", keyManagerConnected, NULL, NULL, &err);

			// attempt to unlock the key manager
			initKeyManager();
		}
	}
	if (LSErrorIsSet(&err)) {
		g_error("%s: failed to register service handle", __PRETTY_FUNCTION__);
		LSErrorPrint(&err, stderr);
		LSErrorFree(&err);
	}
}

bool Security::keyManagerConnected(LSHandle* handle, LSMessage* msg, void* ctxt)
{
	json_object* root = 0;
	const char* str = LSMessageGetPayload(msg);
	if (!str)
		return false;

	root = json_tokener_parse(str);
	if (!root || is_error(root))
		return true;

	bool connected = json_object_get_boolean(json_object_object_get(root, "connected"));
	if (connected) {
		// key manager daemon died, need to unlock it again
		Security::instance()->initKeyManager();
	}

	if (root)
		json_object_put(root);

	return true;
}

void Security::initKeyManager()
{
	LSError err;
	LSErrorInit(&err);

	std::string passcode = "";
	readDecryptedPasscode(passcode);

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "password", json_object_new_string(passcode.c_str()));
	if (!LSCall(m_service, "palm://com.palm.keymanager/initialize",
				json_object_to_json_string(payload), NULL, NULL, NULL, &err)) {
		g_warning("%s: failed to initialize keymanager", __PRETTY_FUNCTION__);
		LSErrorPrint(&err, stderr);
		LSErrorFree(&err);
	}

	json_object_put(payload);
	safelyEraseString(passcode);
}

void Security::updateKeyManager(const std::string& oldPasscode)
{
	LSError err;
	LSErrorInit(&err);

	std::string passcode = "";
	readDecryptedPasscode(passcode);

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "oldPassword", json_object_new_string(oldPasscode.c_str()));
	json_object_object_add(payload, "newPassword", json_object_new_string(passcode.c_str()));
	if (!LSCall(m_service, "palm://com.palm.keymanager/changePassword",
				json_object_to_json_string(payload), NULL, NULL, NULL, &err)) {
		g_warning("%s: failed to change keymanager password", __PRETTY_FUNCTION__);
		LSErrorPrint(&err, stderr);
		LSErrorFree(&err);
	}

	json_object_put(payload);
	safelyEraseString(passcode);
}

bool Security::passcodeSet() const
{
	return g_file_test(s_passcodeFile.c_str(), G_FILE_TEST_EXISTS);
}

int Security::setPasscode(const std::string& mode, const std::string& passcode, std::string& errorText)
{
	bool success = false;
	int errorCode = 0;
	const EASPolicy * const policy = EASPolicyManager::instance()->getPolicy();
	std::string oldPasscode = "";

	// try to protect our policy
	if (policy != 0 && policy->passwordRequired()) {
		errorCode = validatePasscode(policy, mode, passcode);
		if (errorCode < 0) { // failure occured
			switch (errorCode) {
				case FailureEmptyPasscode: errorText = "Passcode is empty"; break;
				case FailureMinLength: errorText = "Passcode not minimum length"; break;
				case FailureInvalidPassword: errorText = "Alphanumeric characters required"; break;
				case FailureAlphaNumeric: errorText = "Alphanumeric characters required"; break;
				case FailureInvalidPin: errorText = "Pin invalid"; break;
				case FailureWeakPasswordRepeat:
					if (mode == "pin")
						errorText = "No repeating numbers (3333)";
					else
						errorText = "No repeating characters (aaaa)";
					break;
				case FailureWeakPasswordSequence:
					if (mode == "pin")
						errorText = "No sequential numbers (1234)";
					else
						errorText = "No sequential characters (abcd)";
					break;

				default: errorText = "Passcode general failure"; break;
			}
			success = false;
			goto Done;
		}
	}

	// grab the old passcode to update the keymanager
	readDecryptedPasscode(oldPasscode);
	
	if (mode == "none") {
		success = true;
		unlink(s_passcodeFile.c_str()); // remove the passcode file
	}
	else if (mode == "pin" || mode == "password") {

		if (passcode.empty()) {
			errorText = "Passcode empty";
			errorCode = FailureEmptyPasscode;
			success = false;
			goto Done;
		}

		char* encryptedStr = 0;
		QByteArray hashed = QCryptographicHash::hash(QString(passcode.c_str()).toUtf8(), QCryptographicHash::Sha1);
		encryptedStr = new char[hashed.size() + 1];
		if (encryptedStr) {
            memset(encryptedStr, 0, hashed.size()+1);
            memcpy(encryptedStr, hashed.data(), hashed.size());

			json_object* saved = json_object_new_object();
			json_object_object_add(saved, mode.c_str(), json_object_new_string(encryptedStr));
            delete [] encryptedStr;

			// write to a tmp file first, then move atomically to real name
			std::string tmpFileName = s_passcodeFile + ".tmp";
			int fd = ::creat(tmpFileName.c_str(), S_IRUSR | S_IWUSR);

			if (fd != -1) {
				const char* buf = json_object_to_json_string(saved);
				ssize_t result = ::write(fd, buf, strlen(buf));
				Q_UNUSED(result);
				::fsync(fd);
				::close(fd);

				success = ::rename(tmpFileName.c_str(), s_passcodeFile.c_str()) == 0;
				if (!success) {
					errorCode = FailureSave;
					errorText = "Passcode save failed";	////-coverity-investigation:  where is it use?
				}
			}
			else {
				g_warning("%s: failed to open temp file '%s'", __PRETTY_FUNCTION__, strerror(errno));
                                ////-coverity-investigation: do we need to set error code and error text
			}

			if (saved && !is_error(saved))        
				json_object_put(saved);

			if (success)
				EASPolicyManager::instance()->passwordEnforced();
		}

	}

	// inform subscribers of the new mode/policy
	if (success) {
		m_lockMode = mode;
		updateKeyManager(oldPasscode);
		SystemService::instance()->postDeviceLockMode();
	}

Done:

	safelyEraseString(oldPasscode);

	return errorCode;
}

bool Security::matchPasscode(std::string passcode, int& retriesLeft, bool& lockedOut)
{
	bool success = false;
	std::string storedPasscode = "";
	char* encryptedStr = 0;
	static uint32_t lastFailure = Time::curTimeMs();
	QByteArray hashed;

	EASPolicyManager* pm = EASPolicyManager::instance();
	const EASPolicy * const policy = pm->getPolicy();

	lockedOut = false;

	// check whether we have locked out the user,
	// or remove the lock if one was set but time has expired
	if ((policy == 0 || pm->policyPending()) && m_numRetries == 0) {
		
		if (Time::curTimeMs() - lastFailure < s_deviceLockOutDuration) {
			lockedOut = true;
			goto Done;
		}
		m_numRetries = s_defaultMaxRetries; // reset allowable failures
	}

	// get encrypted strings for current and test passcodes
	storedPasscode = readPasscode();
	hashed = QCryptographicHash::hash(QString(passcode.c_str()).toUtf8(), QCryptographicHash::Sha1);
	encryptedStr = new char[hashed.size() + 1];

	if (encryptedStr != 0) {
        memset(encryptedStr, 0, hashed.size()+1);
        memcpy(encryptedStr, hashed.data(), hashed.size());
		passcode = encryptedStr;
		delete [] encryptedStr;
	}

	if (passcode != storedPasscode) {

		if (m_numRetries > 0)
			m_numRetries--;
	
		if (policy != 0 && !pm->policyPending() && policy->validMaxRetries()) {
			m_numRetries = pm->decrementRetries();
			if (m_numRetries == 0)
				eraseDevice(); // boom
		}

		lastFailure = Time::curTimeMs();
	}
	else {

		// reset allowable passcode failures
		if (policy != 0 && !pm->policyPending())
			m_numRetries = pm->resetRetries();
		else
			m_numRetries = s_defaultMaxRetries;

		success = true;
	}

Done:

	retriesLeft = m_numRetries;

	return success;
}

void Security::slotPolicyChanged(const EASPolicy * const policy)
{
	if (policy == 0) {
		m_numRetries = s_defaultMaxRetries;
	}
	else if (!policy->validMaxRetries() && !EASPolicyManager::instance()->policyPending()) {
		// unlimited failures!
		m_numRetries = 0;
	}
}

void Security::readLockMode()
{
	m_lockMode = "none";

	if (g_file_test(s_passcodeFile.c_str(), G_FILE_TEST_EXISTS)) {

		json_object* root = json_object_from_file((char*)s_passcodeFile.c_str());
		if (root && !is_error(root)) {

			json_object* obj = json_object_object_get(root, "pin");
			if(obj && !is_error(obj)) {
				m_lockMode = "pin";
			}
			else {
				obj = json_object_object_get(root, "password");
				if (obj && !is_error(obj)) {
					m_lockMode = "password";
				}
			}
			
			json_object_put(root);
		}

		// somehow, this is garbage so reset it
		if (m_lockMode == "none") {
			unlink(s_passcodeFile.c_str());
		}
	}
}

std::string Security::readPasscode() const
{
	std::string result = "";
	json_object* root = json_object_from_file((char*)s_passcodeFile.c_str());
	if (root && !is_error(root)) {

		json_object* key = json_object_object_get(root, m_lockMode.c_str());
		if (key && !is_error(key)) {

			const char* str = json_object_get_string(key);
			if (str)
				result = str;
		}
		json_object_put(root);
	}
	return result;
}

void Security::eraseDevice()
{
	LSError err;
	LSErrorInit(&err);
	g_warning ("%s: calling %s", __func__, "palm://com.palm.storage/erase/Wipe");
	bool result = LSCall(m_service, "palm://com.palm.storage/erase/Wipe", "{}", &Security::cbDeviceWipe, this, NULL, &err);
	if (!result) {
		g_warning ("%s: Failed at %s with message %s", __func__, err.func, err.message);
		LSErrorFree (&err);
	}
}

bool Security::cbDeviceWipe (LSHandle *sh, LSMessage *message, void *data)
{
	const char* str = LSMessageGetPayload(message);
	json_object *root = 0, *returnValue = 0;
	bool success = false;
	if (!str)
		goto done;

	root = json_tokener_parse(str);
	if (!root || is_error(root))
		goto done;

	returnValue = json_object_object_get (root, "returnValue");
	if (!returnValue || is_error (returnValue)) {
		g_warning ("%s: No return value", __func__);
		goto done;
	}

	success = json_object_get_boolean (returnValue);

done:
	if (success) {
		g_warning ("%s: wipe call succeeded, exiting now", __func__);
		exit(-2);
	}
	g_warning ("%s: Wipe failed", __func__);
	return true;
}


bool Security::passcodeSatisfiesPolicy(const EASPolicy * const policy) const
{
	// no policy to enforce or no PIN/password required
	if (!policy || !policy->passwordRequired())
		return true;

	// read encrypted passcode from disk
	std::string passcode;
	readDecryptedPasscode(passcode);
	if (passcode.empty())
		return false;

	// test passcode
	int failure = validatePasscode(policy, m_lockMode, passcode);

	safelyEraseString(passcode);

	return failure == 0;
}

void Security::readDecryptedPasscode(std::string& passcode) const
{
	passcode = readPasscode();
	if (passcode.empty())
		return;

	char* decryptedStr = 0;
//	decryptString(passcode.c_str(), s_passcodeBlowfishKey, &decryptedStr);
	if (decryptedStr != 0) {

		// clean up encrypted string
		/*passcode.replace(0, passcode.size(), passcode.size(), 0);
		passcode = (decryptedStr + s_passcodeSalt.size());
		memset(decryptedStr, 0, strlen(decryptedStr));
		delete [] decryptedStr;*/
	}
	else {
		safelyEraseString(passcode);
	}
}

void Security::safelyEraseString(std::string& str) const
{
	str.replace(0, str.size(), str.size(), 0);
	str.clear();
}

int Security::validatePasscode(const EASPolicy * const policy,	const std::string& mode, 
	const std::string& passcode) const
{
	if (policy == 0 || !policy->passwordRequired())
		return true;

	if (mode == "none" || passcode.empty())
		return FailureEmptyPasscode;

	// min length
	if (policy->validMinLength() && passcode.size() < policy->minLength())
		return FailureMinLength;

	QString pass = qFromUtf8Stl(passcode);

	if (policy->requiresAlphaNumeric()) {

		// an alphanumeric password MUST be set
		if (mode != "password")
			return FailureInvalidPassword;

		// contains alpha numeric characters
		bool containsAlpha = false, containsNumeric = false;
		for (int i=0; i<pass.length(); i++) {
			
			if (!containsAlpha)
				containsAlpha = pass.at(i).isLetter();
			if (!containsNumeric)
				containsNumeric = pass.at(i).isNumber();

			if (containsAlpha && containsNumeric)
				break;
		}

		if (!containsAlpha || !containsNumeric)
			return FailureAlphaNumeric;
	}
	else {
		// password's can contain any character types
		if (mode == "pin") {
			// pin's must contain only digits
			for (int i=0; i<pass.length(); i++) {
				if (!pass.at(i).isNumber())
					return FailureInvalidPin;
			}
		}
	}

	if (!policy->allowSimplePassword())
		return validateStrength(pass);
	
	return Success;
}

int Security::validateStrength(QString passcode) const
{
	const int MaxSequence = passcode.length() / 2;
	int direction = 0;
	int j = 0;

	for (int i=0; i<passcode.length()-1; i++) {

		// should we start tracking a sequence?
		QChar curChar = passcode.at(i);
		QChar nextChar = passcode.at(i+1);
		direction = nextChar.unicode() - curChar.unicode();

		if (direction > -2 && direction < 2) {

			int numSequence = 2;
			curChar = nextChar;
			for (j=i+2; j<passcode.length(); j++) {

				// does the sequence continue?
				nextChar = passcode.at(j);
				int diff = nextChar.unicode() - curChar.unicode();
				if (diff != direction)
					break;

				numSequence++;

				if (numSequence > MaxSequence) {
					if (diff == 0)
						return FailureWeakPasswordRepeat;
					else
						return FailureWeakPasswordSequence;
				}

				curChar = nextChar;
			}

			// skip characters we've already examined
			i = j-1;
		}
	}
	return Success;
}

