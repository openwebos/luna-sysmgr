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




#include "Common.h"

#include "UiNavigationController.h"

static UiNavigationController* s_instance = 0;

UiNavigationController* UiNavigationController::instance()
{
    if (!s_instance)
		new UiNavigationController;
	return s_instance;
}

UiNavigationController::UiNavigationController()
{
    s_instance = this;
}

UiNavigationController::~UiNavigationController()
{
    s_instance = 0;
}

void UiNavigationController::registerNavigator(KeyNavigator* navigator)
{
    Q_ASSERT(!m_navigators.contains(navigator));
    m_navigators.append(navigator);
}

bool UiNavigationController::handleEvent(QEvent* event)
{
    if (!isUiNavigationEvent(event))
        return false;

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    bool propogate = true;
    bool handled = false;
    KeyNavigator* navigator = 0;
    Q_FOREACH(navigator, m_navigators)
    {
        handled = navigator->handleNavigationEvent(keyEvent, propogate);
        if (!propogate || handled)
            break;
    }
    return handled;
}

bool UiNavigationController::isUiNavigationEvent(QEvent* event) const
{
    if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)
        return false;

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (!keyEvent->modifiers() & Qt::ExternalKeyboardModifier)
        return false;

    switch (keyEvent->key())
    {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Return:
    case Qt::Key_Backspace:
        return true;
    default: 
        break;
    }
    return false;
}

