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




#include "SysmgrIMEDataInterface.h"
#include "HostBase.h"
#include "IMEController.h"

#include "SoundPlayerPool.h"
#include "SystemUiController.h"

#include <QApplication>
#include <QWidget>

SysmgrIMEModel::SysmgrIMEModel() : m_inputMethod(NULL)
{
	const HostInfo& info = HostBase::instance()->getInfo();
	m_availableSpace.set(QRect(0, 0, info.displayWidth, info.displayHeight));
	m_screenSize.set(QSize(info.displayWidth, info.displayHeight));
}

void SysmgrIMEModel::setInputMethod(InputMethod * inputMethod)
{
	m_inputMethod = inputMethod;
}

void SysmgrIMEModel::touchEvent(const QTouchEvent& te)
{
	if (m_inputMethod)
		m_inputMethod->touchEvent(te);
}

void SysmgrIMEModel::paint(QPainter& painter)
{
	if (m_inputMethod)
		m_inputMethod->paint(painter);
}

void SysmgrIMEModel::tapEvent(const QPoint& tapPt)
{
	if (m_inputMethod)
		m_inputMethod->tapEvent(tapPt);
}

void SysmgrIMEModel::screenEdgeFlickEvent()
{
	if (m_inputMethod)
		m_inputMethod->screenEdgeFlickEvent();
}

void SysmgrIMEModel::setComposingText(const std::string& text)
{
	IMEController::instance()->setComposingText(text);
}

void SysmgrIMEModel::commitComposingText()
{
	IMEController::instance()->commitComposingText();
}

void SysmgrIMEModel::commitText(const std::string& text)
{
	IMEController::instance()->commitText(text);
}

void SysmgrIMEModel::performEditorAction(PalmIME::FieldAction action)
{
	IMEController::instance()->performEditorAction(action);
}

void SysmgrIMEModel::sendKeyEvent(QEvent::Type type, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    QWidget* focusedWidget = QApplication::focusWidget();
    if (focusedWidget && (type == QEvent::KeyPress || type == QEvent::KeyRelease)) {
		QChar qchar(key);

		// only lower case A to Z. Other keys are unicode characters with proper casing already...
		if (key >= Qt::Key_A && key <= Qt::Key_Z && !(modifiers & Qt::ShiftModifier))
			qchar = qchar.toLower();
		QApplication::postEvent(focusedWidget, new QKeyEvent(type, key, modifiers, qchar));
    }
}

void SysmgrIMEModel::requestHide()
{
	IMEController::instance()->hideIME();
}

bool SysmgrIMEModel::isUIAnimationActive()
{
	return SystemUiController::instance()->isUiRotating();
}

void SysmgrIMEModel::keyDownAudioFeedback(Qt::Key key)
{
	if (key == Qt::Key_Space)
		SoundPlayerPool::instance()->playFeedback("space");
	else if (key == Qt::Key_Backspace)
		SoundPlayerPool::instance()->playFeedback("backspace");
	else if (key == Qt::Key_Return)
		SoundPlayerPool::instance()->playFeedback("return");
	else if (key)
		SoundPlayerPool::instance()->playFeedback("key");
}

