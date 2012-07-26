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




#include "iconcmdevents.h"
#include "icon.h"

QEvent::Type IconCmdRequestEvent::s_eventType = (QEvent::Type)(QEvent::MaxUser+1);

IconCmdRequestEvent::IconCmdRequestEvent(IconBase * p_iconSource,int cmdRequest)
: QEvent(s_eventType) , m_qp_icon(p_iconSource) , m_cmdRequest(cmdRequest)
{
}
