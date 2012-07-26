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




#include "IMEController.h"

#include "InputClient.h"
#include "HostBase.h"
#include "WindowServer.h"

#include <glib.h>

static IMEController* s_instance = 0;

IMEController* IMEController::instance()
{
	if (G_UNLIKELY(s_instance == 0)) {
		return new IMEController();
	}
	return s_instance;
}

IMEController::IMEController()
    : m_client(0)
    , m_imeOpened(false)
    , m_imeAllowed(true)
    , m_pendingVisibility(PendingVisibilityNone)
{
    s_instance = this;

    connect(HostBase::instance(), SIGNAL(signalBluetoothKeyboardActive(bool)), 
        SLOT(slotBluetoothKeyboardChanged(bool)));

    connect(WindowServer::instance(), SIGNAL(signalTouchesReleasedFromScreen()),
        SLOT(slotTouchesReleasedFromScreen()));
}

void IMEController::setClient(InputClient* client)
{
    if (!client)
        return;

    // new clients can overwrite existing clients
    if (m_client != client) {
        m_client = client;
        notifyInputFocusChange(m_client, (m_client ? m_client->inputFocus() : false));
    }
}

void IMEController::removeClient(InputClient* client)
{
    if (!client)
        return;

    // only the active client can remove itself
    if (m_client == client) {

        hideIMEInternal();
        m_client = 0;
    }
}

void IMEController::setComposingText(const std::string& text)
{
	if (m_client && m_imeAllowed)
		m_client->setComposingText(text);
}

void IMEController::commitComposingText()
{
	if (m_client)
		m_client->commitComposingText();
}

void IMEController::commitText(const std::string& text)
{
	if (m_client && m_imeAllowed)
		m_client->commitText(text);
}

void IMEController::performEditorAction(PalmIME::FieldAction action)
{
	if (m_client && m_imeAllowed)
		m_client->performEditorAction(action);
}

void IMEController::hideIME()
{
    if (m_client && !HostBase::instance()->bluetoothKeyboardActive()) {
        m_client->removeInputFocus();
    }
    else {
        hideIMEInternal();
    }
}

void IMEController::hideIMEInternal()
{
    if (WindowServer::instance()->touchOnScreen()) {
        m_pendingVisibility = PendingVisibilityHide;
    }
    else if (m_imeOpened) {
        m_imeOpened = false;
        Q_EMIT signalHideIME();
    }
}

void IMEController::showIMEInternal()
{
    if (WindowServer::instance()->touchOnScreen()) {
        m_pendingVisibility = PendingVisibilityShow;
    }
    else {
        if (!m_imeOpened) {
            m_imeOpened = true;
            Q_EMIT signalShowIME();
        }

        if (m_client) {
            Q_EMIT signalRestartInput(m_client->inputState());
        }
    }
}

void IMEController::notifyInputFocusChange(InputClient* client, bool focused)
{
    if (!m_imeAllowed)
        return;

    if (!client || client != m_client)
        return;

    if (focused) {
        showIMEInternal();
    }
    else {
        hideIMEInternal();
    }
}

void IMEController::notifyAutoCapChanged(InputClient* client, bool enable)
{
    if (!m_imeAllowed)
        return;

    if (!client || client != m_client)
        return;

    Q_EMIT signalAutoCapChanged(enable);
}

void IMEController::slotBluetoothKeyboardChanged(bool active)
{
#if !defined(TARGET_EMULATOR)
    setIMEActive(!active);
#endif
}

void IMEController::slotTouchesReleasedFromScreen()
{
    if (m_pendingVisibility == PendingVisibilityHide) {

        m_pendingVisibility = PendingVisibilityNone;
        hideIMEInternal();
    }
    else if (m_pendingVisibility == PendingVisibilityShow) {

        m_pendingVisibility = PendingVisibilityNone;
        showIMEInternal();
    }
}

void IMEController::setIMEActive(bool active)
{
    m_imeAllowed = active;

    if (!m_imeAllowed) {
        hideIME();
    }
    else {
        notifyInputFocusChange(m_client, (m_client ? m_client->inputFocus() : false));
    }
}

