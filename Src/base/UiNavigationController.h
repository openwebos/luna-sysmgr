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




#ifndef UINAVIGATIONCONTROLLER_H
#define UINAVIGATIONCONTROLLER_H

#include "Common.h"

#include <QObject>
#include <QKeyEvent>

class KeyNavigator
{
public:
    virtual bool handleNavigationEvent(QKeyEvent* event, bool& propogate) = 0;
};

class UiNavigationController : public QObject
{
	Q_OBJECT
public:

	static UiNavigationController* instance();
	~UiNavigationController();

    void registerNavigator(KeyNavigator* navigator);

    bool handleEvent(QEvent *event);

private:
    bool isUiNavigationEvent(QEvent *event) const;

	UiNavigationController();

    QList<KeyNavigator*> m_navigators;
};

#endif /* UINAVIGATIONCONTROLLER_H */
