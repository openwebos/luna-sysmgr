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




#ifndef ICONCMDEVENTS_H_
#define ICONCMDEVENTS_H_

#include <QEvent>
#include <QPointer>

class IconBase;

class IconCmdRequestEvent : public QEvent
{
public:
	IconCmdRequestEvent(IconBase * p_iconSource,int cmdRequest);

	QPointer<IconBase> m_qp_icon;
	int m_cmdRequest;
	static QEvent::Type s_eventType;
};


#endif /* ICONCMDEVENTS_H_ */
