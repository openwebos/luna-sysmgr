/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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

#include "MetaKeyManager.h"

#include <QKeyEvent>
#include <QDebug>

bool MetaKeyManager::handleEvent(QEvent* event)
{
    if (QEvent::KeyPress != event->type() && QEvent::KeyRelease != event->type())
        return false;

    QKeyEvent *keyEvent = static_cast<QKeyEvent*> (event);

    // Preprocess the event to handle clipboard operations.
    // Filter c, x, v keys from ever getting into webkit so cut/copy/paste work
    // for non-frameworked text edit fields.
    static Qt::KeyboardModifiers commandModifier = (Qt::ControlModifier|Qt::ExternalKeyboardModifier);
    if (keyEvent->modifiers() & Qt::MetaModifier 
        || (keyEvent->modifiers() & commandModifier) == commandModifier)
    { 
        if (keyEvent->key() == Qt::Key_C) {
            if (keyEvent->type() == QEvent::KeyPress) {
                Q_EMIT signalCopy();
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_X) {
            if (keyEvent->type() == QEvent::KeyPress) {
                Q_EMIT signalCut();
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_V) {
            if (keyEvent->type() == QEvent::KeyPress) {
                Q_EMIT signalPaste();
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_A) {
            // NOTE: SelectAll should be detected and triggered by webkit,
            // This is a workaround until we move to QtWebKit
            if (keyEvent->type() == QEvent::KeyPress) {
                Q_EMIT signalSelectAll();
            }
            return true;
        }

    }
    return false;
}

