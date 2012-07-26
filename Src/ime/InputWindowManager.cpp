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




#include "InputWindowManager.h"

#include "HostBase.h"
#include "SystemUiController.h"
#include "IMEController.h"
#include "Utils.h"
#include "SystemService.h"
#include "AnimationSettings.h"


InputWindowManager::InputWindowManager(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_activeIME(0)
	, m_imeView(0)
	, m_imeShown(false)
{
	connect(SystemUiController::instance(), SIGNAL(signalNegativeSpaceChanged(QRect)),
											SLOT(slotNegativeSpaceChanged(QRect)));

	connect(IMEController::instance(), SIGNAL(signalHideIME()),
									   SLOT(slotHideIME()));
	connect(IMEController::instance(), SIGNAL(signalShowIME()),
									   SLOT(slotShowIME()));
    connect(IMEController::instance(), SIGNAL(signalRestartInput(const PalmIME::EditorState&)),
                                       SLOT(slotRestartInput(const PalmIME::EditorState&)));
    connect(IMEController::instance(), SIGNAL(signalAutoCapChanged(bool)),
                                       SLOT(slotAutoCapChanged(bool)));

    // hide/show the IMEView when entering/exiting MSM
    connect(SystemService::instance(), SIGNAL(signalEnterBrickMode(bool)), 
                                        SLOT(slotEnterBrickMode(bool)));
    connect(SystemService::instance(), SIGNAL(signalExitBrickMode()), 
                                        SLOT(slotExitBrickMode()));
}

void InputWindowManager::init()
{
	QRectF r = boundingRect();
	m_imeView = new IMEView(this);
	m_imeView->setBoundingRect(QRectF(0, 0, r.width(), r.height()));
	m_imeView->setPos(r.topLeft());

	m_activeIME = m_imeMgr.createPreferredIME(SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight());
	Q_ASSERT(m_activeIME);

	connect(&(m_activeIME->m_keyboardHeight), SIGNAL(valueChanged(const qint32&)), 
			SLOT(slotKeyboardHeightChanged(const qint32&)));

	m_imeView->attach(m_activeIME);

    m_fadeAnim.setTargetObject(m_imeView);
    m_fadeAnim.setPropertyName("opacity");
    m_fadeAnim.setDuration(AS(brickDuration));
    m_fadeAnim.setEasingCurve(AS_CURVE(brickCurve));
}

void InputWindowManager::resize(int width, int height)
{
	WindowManagerBase::resize(width, height);

	m_imeView->setBoundingRect(QRectF(0, 0, width, height));
	m_imeView->setPos(boundingRect().topLeft());

	if (m_activeIME) {
		m_activeIME->m_screenSize.set(QSize(width, height));
		m_activeIME->m_availableSpace.set(QRect(0, 0, width, height));
	}
}

bool InputWindowManager::doReticle(QPointF pos)
{
	if (m_imeView)
		return !m_imeView->acceptPoint(m_imeView->mapFromScene(pos));
	return true;
}

void InputWindowManager::slotNegativeSpaceChanged(QRect r)
{
	if (!m_activeIME)
		return;

	qreal y = m_activeIME->m_keyboardHeight.get() - r.height();
	m_imeView->setVisible(r.height() != 0);
	setPosTopLeft(m_imeView, 0, y);
}

void InputWindowManager::slotKeyboardHeightChanged(const qint32& newHeight)
{
	if (!m_activeIME)
		return;

	if (m_imeShown) {

		int targetTop = SystemUiController::instance()->currentUiHeight() - newHeight;
		SystemUiController::instance()->changeNegativeSpace(targetTop, true, true);
	}
	else {

		int targetTop = SystemUiController::instance()->currentUiHeight();
		SystemUiController::instance()->changeNegativeSpace(targetTop, true, true);
	}
}

void InputWindowManager::slotHideIME()
{
	if (!m_imeShown)
		return;
	
	g_debug("System request to hide the IME");
	
	m_imeShown = false;

	if (m_activeIME)
		m_activeIME->m_visible.set(false);

	int targetTop = SystemUiController::instance()->currentUiHeight();
	SystemUiController::instance()->changeNegativeSpace(targetTop, true);
}

void InputWindowManager::slotShowIME()
{
	g_debug("System request to show the IME");

	m_imeShown = true;

	if (m_activeIME)
		m_activeIME->m_visible.set(true);

	int targetTop = SystemUiController::instance()->currentUiHeight() - m_activeIME->m_keyboardHeight.get();
	SystemUiController::instance()->changeNegativeSpace(targetTop, true);
}

void InputWindowManager::slotRestartInput(const PalmIME::EditorState & state)
{
	// NOTE: this should this be a callback?
	if (m_activeIME)
		m_activeIME->m_editorState.set(state);
}

void InputWindowManager::slotAutoCapChanged(bool enabled)
{
	g_debug("%s: Got auto cap change %d", __PRETTY_FUNCTION__, enabled);
    if (m_activeIME)
        m_activeIME->m_autoCap.set(enabled);
}

void InputWindowManager::slotEnterBrickMode(bool)
{
    if (!m_imeView)
        return;

    m_fadeAnim.stop();
    m_fadeAnim.setStartValue(m_imeView->opacity());
    m_fadeAnim.setEndValue(0.0);
    m_fadeAnim.start();
}

void InputWindowManager::slotExitBrickMode()
{
    if (!m_imeView)
        return;

    m_fadeAnim.stop();
    m_fadeAnim.setStartValue(m_imeView->opacity());
    m_fadeAnim.setEndValue(1.0);
    m_fadeAnim.start();
}

