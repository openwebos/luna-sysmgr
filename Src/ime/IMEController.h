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




#ifndef IMECONTROLLER_H
#define IMECONTROLLER_H

#include <QObject>

#include <palmimedefines.h>

class InputClient;

class IMEController : public QObject
{
	Q_OBJECT

public:
	static IMEController* instance();

    void setClient(InputClient* newClient);
    void removeClient(InputClient* client);

    const InputClient* const client() const { return m_client; }

	bool isIMEOpened() const { return m_imeOpened; }
    bool isIMEActive() const { return m_imeAllowed; }

	void setComposingText(const std::string& text);
	void commitComposingText();

	void commitText(const std::string& text);

	void performEditorAction(PalmIME::FieldAction action);

	void hideIME();
	
    void notifyInputFocusChange(InputClient* client, bool focused);

    void notifyAutoCapChanged(InputClient* client, bool enabled);

    // explicit setting for whether the IME will show itself when
    // an input field gains focus in the active client
    void setIMEActive(bool active);

Q_SIGNALS:
	void signalHideIME();
	void signalShowIME();
	void signalRestartInput(const PalmIME::EditorState& state);
    void signalAutoCapChanged(bool enabled);

private Q_SLOTS:
    void slotBluetoothKeyboardChanged(bool active);
    void slotTouchesReleasedFromScreen();

private:
    void hideIMEInternal();
    void showIMEInternal();

private:
    InputClient* m_client;
	bool m_imeOpened;
    bool m_imeAllowed;

    enum PendingVisibility {
        PendingVisibilityNone = 0,
        PendingVisibilityHide,
        PendingVisibilityShow
    };
    PendingVisibility m_pendingVisibility;

private:
	IMEController();
	Q_DISABLE_COPY(IMEController)
};

#endif

