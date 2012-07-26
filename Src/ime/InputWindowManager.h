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




#ifndef INPUTWINDOWMANAGER_H
#define INPUTWINDOWMANAGER_H

#include "WindowManagerBase.h"
#include "IMEManager.h"
#include "IMEView.h"

#include <QPropertyAnimation>

class InputWindowManager : public WindowManagerBase
{
	Q_OBJECT

public:
	InputWindowManager(int maxWidth, int maxHeight);

	virtual void init();

	virtual void resize(int width, int height);
	virtual bool doReticle(QPointF pos);

private Q_SLOTS:
	void slotNegativeSpaceChanged(QRect r);

	void slotKeyboardHeightChanged(const qint32& newHeight);

	void slotHideIME();
	void slotShowIME();
	void slotRestartInput(const PalmIME::EditorState& state);
    void slotAutoCapChanged(bool enabled);

    void slotEnterBrickMode(bool);
    void slotExitBrickMode();

private:
	IMEManager m_imeMgr;
	IMEDataInterface* m_activeIME;
	IMEView* m_imeView;
	bool m_imeShown;
    QPropertyAnimation m_fadeAnim;
};

#endif

