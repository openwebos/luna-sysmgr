/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef KEYLOCATIONRECORDER_H
#define KEYLOCATIONRECORDER_H

#include <stdio.h>
#include <string>

#include <qpoint.h>
#include <qrect.h>
#include <qchar.h>
#include <qstring.h>

class KeyLocationRecorder
{
public:
    KeyLocationRecorder();

	static KeyLocationRecorder & instance();

	bool	isRecording() const		{ return m_file != NULL; }

	void	startStop(const char * layoutName, const QRect & keymapRect);
	void	record(const QString & text, const QPoint & where, const QString & altText = QString());
	void	keyboardSizeChanged(const char * layoutName, const QRect & keymapRect);

	FILE *		m_file;
	std::string	m_lastMessageID;
};

#endif // KEYLOCATIONRECORDER_H
