/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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



#ifndef CANDIDATEBARREMOTE_H
#define CANDIDATEBARREMOTE_H

#include "CandidateBar.h"
#include <lunaservice.h>

class CandidateBarRemote : public CandidateBar
{

public:
	CandidateBarRemote(Mapper_IF& mapper, IMEDataInterface * dataInterface);

	virtual void	setEnabled(bool enable);

	virtual bool	keyboardTap(QPointF where, Qt::Key car);	// returns true if the touch was "used" and no key should be sent
	virtual bool	backspace(bool shiftDown);					// returns true if backspace was "used"

	virtual bool	loadKeyboardLayoutFile(const char * fullPath, quint16 primaryID, quint16 secondaryID);
	virtual void	setLanguage(const std::string & languageName);
	virtual void	updateKeyboardLayout(const char * layoutName, quint32 page, const QRect & keyboardFrame, bool shiftActive, bool capsLockActive, bool autoCapActive);
	virtual void	updateSuggestions(bool trace);
	virtual void	processTrace();

	virtual void	clearCandidates();

	virtual void	drawXT9Regions(QPixmap * pixmap, int topPadding);

	bool			handleSmartkeyReply(const char * reply);

private:
	bool					m_shift;
	bool					m_capsLock;
	bool					m_autoCap;

	LSHandle *				m_serviceHandle;
};

#endif // CANDIDATEBARREMOTE_H
