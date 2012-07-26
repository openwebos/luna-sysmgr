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




#include "ShortcutsHandler.h"
#include "PalmIMEHelpers.h"
#include "qchar.h"
#include "VirtualKeyboardPreferences.h"

const quint64   cMaxDelay = 1000;
Qt::Key         cUnknownKey = Qt::Key_A;

ShortcutsHandler::ShortcutsHandler(IMEDataInterface * dataInterface)
    : m_IMEDataInterface(dataInterface)
    , m_enableDoubleSpacePeriod(false)
    , m_lastSpaceTime(0)
    , m_lastKey(cUnknownKey)
{
}

void ShortcutsHandler::resetEditor(const PalmIME::EditorState & state)
{
    m_enableDoubleSpacePeriod = (state.type == PalmIME::FieldType_Text) && VirtualKeyboardPreferences::instance().getSpaces2period();
    resetEditor();
}

void ShortcutsHandler::resetEditor()
{
    m_lastSpaceTime = 0;
    m_lastKey = cUnknownKey;
}

bool ShortcutsHandler::filterKey(Qt::Key & key)
{
    if (m_enableDoubleSpacePeriod)
    {
        //g_debug("ShortcutsHandler::filterKey: '%s' - 0x%X", QString(key).toUtf8().data(), key);
        if (key == Qt::Key_Space && m_lastSpaceTime)
        {
            if (m_lastSpaceTime + cMaxDelay > currentTime())
            {
                m_IMEDataInterface->sendKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
                m_IMEDataInterface->sendKeyEvent(QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier);
                m_IMEDataInterface->sendKeyEvent(QEvent::KeyPress, Qt::Key_Period, Qt::NoModifier);
                m_IMEDataInterface->sendKeyEvent(QEvent::KeyRelease, Qt::Key_Period, Qt::NoModifier);
                key = Qt::Key_Right;
            }
            m_lastSpaceTime = 0;
            m_lastKey = cNoKey;
        }
        else
        {
            if (key == Qt::Key_Space && m_lastKey != cNoKey && ::strchr(".,;:!?", m_lastKey) == 0)
                m_lastSpaceTime = currentTime();
            else
            {
                m_lastSpaceTime = 0;
                if (key == Qt::Key_Backspace)
                    m_lastKey = cUnknownKey;    // after backspace, allow double-space to period
                else if (key == Qt::Key_Space || !isUnicodeQtKey(key))
                    m_lastKey = cNoKey;
                else
                    m_lastKey = key;
            }
        }
    }
    return true;
}
