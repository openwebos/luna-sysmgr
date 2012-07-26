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



#include "CandidateBarRemote.h"
#include "JSONUtils.h"
#include <glib.h>
#include "HostBase.h"
#include "Logging.h"

CandidateBarRemote::CandidateBarRemote(Mapper_IF& mapper, IMEDataInterface * dataInterface) :
	CandidateBar(mapper, dataInterface), m_shift(false), m_capsLock(false), m_autoCap(false), m_serviceHandle(0)
{
}

void CandidateBarRemote::setEnabled(bool enable)
{
	if (m_enabled != enable)
	{
		if (enable)
			clearCandidates();
		else
			commit();
		m_enabled = enable;
		Q_EMIT resized();
	}
}

bool CandidateBarRemote::loadKeyboardLayoutFile(const char * fullPath, quint16 primaryID, quint16 secondaryID)
{
	return true;
}

void CandidateBarRemote::setLanguage(const std::string & languageName)
{
}

void CandidateBarRemote::updateKeyboardLayout(const char * layoutName, quint32 page, const QRect & keyboardFrame, bool shiftActive, bool capsLockActive, bool autoCapActive)
{
	m_shift = shiftActive;
	m_capsLock = capsLockActive;
	m_autoCap = autoCapActive;
}

void CandidateBarRemote::clearCandidates()
{
	CandidateBar::clearCandidates();
	m_IMEDataInterface->setComposingText("");
}

bool CandidateBarRemote::keyboardTap(QPointF where, Qt::Key car)
{
	if (m_enabled)
	{
		m_traceEntry = false;
		m_taps.push_back(TapRecord(where.x(), where.y(), car, m_shift || m_capsLock || (m_autoCap && m_taps.size() == 0)));

		updateSuggestions(false);
		return true;
	}

	return false;
}

void CandidateBarRemote::processTrace()
{
	updateSuggestions(true);
}

bool CandidateBarRemote::backspace(bool shiftDown)
{
	if (!m_enabled)
		return false;

	if (m_traceEntry)
	{
		m_traceEntry = false;
		m_taps.clear();
	}
	else
	{
		if (m_taps.size() == 0)
			return false;	// nothing to clear
	}

	m_tracePoints.clear();
	if (m_traceEntry || shiftDown)
	{
		m_taps.clear();
	}
	else if (m_taps.size() > 0)
	{
		m_taps.pop_back();
	}

	updateSuggestions(false);

	return true;
}

static bool smartkeyReplyHandler(LSHandle *sh, LSMessage *reply, void *ctx)
{
	const char* str = LSMessageGetPayload(reply);
	if (!str)
		return true;
	g_debug("Smartkey reply: %s", str);

	return reinterpret_cast<CandidateBarRemote *>(ctx)->handleSmartkeyReply(str);
}

bool CandidateBarRemote::handleSmartkeyReply(const char * reply)
{
	JsonMessageParser	parser(reply, SCHEMA_ANY);
	if (parser.parse(__FUNCTION__) && parser.get("traceEntry", m_traceEntry))
	{
		pbnjson::JValue spelledCorrectly(parser.get("spelledCorrectly"));
		pbnjson::JValue	guesses(parser.get("guesses"));
		if (guesses.isArray() && spelledCorrectly.isBoolean())
		{
			m_candidates.clear();
			m_bestCandidate.clear();
			for (ssize_t index = 0; index < guesses.arraySize(); ++index)
			{
				JsonValue value(guesses[index]);
				std::string	str;
				bool		correct, autoAccept;
				if (value.get("str", str) && value.get("sp", correct))
				{
					m_candidates.push_back(QString::fromUtf8(str.c_str()));
					if (m_bestCandidate.length() == 0 && value.get("auto-accept", autoAccept) && autoAccept)
						m_bestCandidate = m_candidates.back();
				}
			}
			if (m_bestCandidate.length() == 0)
			{
				if (spelledCorrectly.asBool() && m_candidates.size() > 0)
					m_bestCandidate = m_candidates[0];
				else if (m_candidates.size() >= 2)
					m_bestCandidate = m_candidates[1];
			}
			m_IMEDataInterface->setComposingText(m_bestCandidate.toUtf8().data());

			Q_EMIT needsRedraw();
		}
	}
	else
		clearCandidates();
	return true;
}

void CandidateBarRemote::updateSuggestions(bool trace)
{
	if (!m_enabled)
		return;

	clearCandidates();

	pbnjson::JValue msg = pbnjson::Object();
	if (trace && m_tracePoints.size() > 0)
	{
		m_traceEntry = true;
		if (m_capsLock)
			msg.put("shift", "lock");
		else if (m_shift || m_autoCap)
			msg.put("shift", "once");
		else
			msg.put("shift", "off");
		pbnjson::JValue array = pbnjson::Array();
		for (size_t n = 0; n < m_tracePoints.size(); ++n)
		{
			const TracePoint & point = m_tracePoints[n];
			array << (int32_t) (((point.m_x & 0xFFFF) << 16) | (point.m_y & 0xFFFF));	// put both coordindates in a single 32 bit number to reduce json churn by 2
		}
		msg.put("trace", array);
		const TracePoint &	first = m_tracePoints.front();
		std::string			firstChars = m_mapper.pointToKeys(QPoint(first.m_x, first.m_y));
		const TracePoint &	last = m_tracePoints.back();
		std::string			lastChars = m_mapper.pointToKeys(QPoint(last.m_x, last.m_y));
		msg.put("first", firstChars);
		msg.put("last", lastChars);
		g_debug("Trace from '%s' to '%s'", firstChars.c_str(), lastChars.c_str());
	}
	else if (!trace && m_taps.size() > 0)
	{
		m_traceEntry = false;
		pbnjson::JValue array = pbnjson::Array();
		for (size_t n = 0; n < m_taps.size(); ++n)
		{
			const TapRecord & record = m_taps[n];
			array << (int32_t) record.m_p.m_x;
			array << (int32_t) record.m_p.m_y;
			array << (int32_t) record.m_char;
			array << record.m_shift;
		}
		msg.put("taps", array);
	}
	else
		return;	// we're done: nothing to send!

	std::string msgStr = jsonToString(msg);
	//g_debug("Sending: %s", msgStr.c_str());

	LSError lserror;
	LSErrorInit(&lserror);

	if (m_serviceHandle || (VERIFY(LSRegister(NULL, &m_serviceHandle, &lserror)) &&
		VERIFY(LSGmainAttach(m_serviceHandle, HostBase::instance()->mainLoop(), &lserror))))
	{
		VERIFY(LSCall(m_serviceHandle, "palm://com.palm.smartKey/processTaps",
							msgStr.c_str(), smartkeyReplyHandler, this, NULL, &lserror));
	}
	if (LSErrorIsSet(&lserror))
	{
		logFailure(lserror.message, lserror.file, lserror.line, lserror.func);
		LSErrorFree(&lserror);
	}
}

void CandidateBarRemote::drawXT9Regions(QPixmap * pixmap, int topPadding)
{
	// can't be done simply...
}
