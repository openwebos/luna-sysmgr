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




#ifndef HOSTQTDESKTOP_H
#define HOSTQTDESKTOP_H

#include "Common.h"

#include <QWidget>

#include "HostBase.h"

QT_BEGIN_NAMESPACE
class QWidget;
class QEvent;
QT_END_NAMESPACE

class HostQtDesktopKeyFilter;
class HostQtDesktopMouseFilter;

class HostQtDesktop : public HostBase
{
public:

	HostQtDesktop();
	virtual ~HostQtDesktop();

	virtual void init(int w, int h);

	virtual void show();
	virtual unsigned short translateKeyWithMeta( unsigned short key, bool withShift, bool withAlt );

	virtual const char* hardwareName() const;
	virtual void setCentralWidget(QWidget* view);

	bool hasAltKey(Qt::KeyboardModifiers modifiers);

private:
	QWidget* m_widget;
	HostQtDesktopKeyFilter* m_keyFilter;
	HostQtDesktopMouseFilter* m_mouseFilter;
};



#endif /* HOSTQTDESKTOP_H */
