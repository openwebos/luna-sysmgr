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




#ifndef SHORTCUTSHANDLER_H
#define SHORTCUTSHANDLER_H

#include "IMEDataInterface.h"
#include <qnamespace.h>

#include <ime/palmimedefines.h>

/*
    Implementation of virtual keyboard shortcuts by filtering virtual keystrokes.
     - double-space -> period (under some conditions).
*/

class ShortcutsHandler
{
    static const Qt::Key cNoKey = Qt::Key(0);
public:
    ShortcutsHandler(IMEDataInterface * dataInterface);
    void    resetEditor(const PalmIME::EditorState & state);
    void    resetEditor();
    bool    filterKey(Qt::Key & key);

private:
    IMEDataInterface *  m_IMEDataInterface;
    bool                m_enableDoubleSpacePeriod;
    quint64             m_lastSpaceTime;
    Qt::Key             m_lastKey;
};

#endif // SHORTCUTSHANDLER_H
