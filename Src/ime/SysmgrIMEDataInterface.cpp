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
#include <glib.h>
#include "HostBase.h"
#include "IMEController.h"
#include "Settings.h"
#include "VirtualKeyboardPreferences.h"

#include "SoundPlayerPool.h"
#include "SystemUiController.h"
#include "Localization.h"
#include "Preferences.h"
#include "BannerMessageHandler.h"
#include "BannerMessageEventFactory.h"

#include <QApplication>
#include <QDebug>
#include <QWidget>
#include <ime/IMEData.h>

SysmgrIMEModel::SysmgrIMEModel() : m_inputMethod(NULL),m_serviceHandle(NULL)
{
	const HostInfo& info = HostBase::instance()->getInfo();
	m_availableSpace.set(QRect(0, 0, info.displayWidth, info.displayHeight));
	m_screenSize.set(QSize(info.displayWidth, info.displayHeight));
}

void SysmgrIMEModel::setInputMethod(InputMethod * inputMethod)
{
	m_inputMethod = inputMethod;
}

QString SysmgrIMEModel::getLocalizedString(const std::string &str)
{
    return fromStdUtf8(LOCALIZED(str));
}

std::string SysmgrIMEModel::getLocale()
{
    return LocalePreferences::instance()->locale();
}

QVariant SysmgrIMEModel::getLunaSystemSetting(const QString &key)
{
    return Settings::LunaSettings()->getSetting(key);
}

void SysmgrIMEModel::createRemoveBannerMessage(const std::string& appId,
                                               const std::string& msgId)
{
    BannerMessageEvent* e = BannerMessageEventFactory::createRemoveMessageEvent(appId, msgId);
    BannerMessageHandler::instance()->handleBannerMessageEvent(e);

    delete e;
}

std::string SysmgrIMEModel::createAddBannerMessage(const std::string& appId,
                                                   const std::string& msg,
                                                   const std::string& launchParams,
                                                   const std::string& icon,
                                                   const std::string& soundClass,
                                                   const std::string& soundFile,
                                                   int duration,
                                                   bool doNotSuppress)
{
    BannerMessageEvent* e = BannerMessageEventFactory::createAddMessageEvent(appId, msg, launchParams, icon, soundClass, soundFile, duration, doNotSuppress);
    std::string lastMessageID = e->msgId;
    BannerMessageHandler::instance()->handleBannerMessageEvent(e);
    delete e;
    return lastMessageID;
}

VirtualKeyboardPreferences &SysmgrIMEModel::virtualKeyboardPreferences()
{
    return VirtualKeyboardPreferences::instance();
}

// If service handle exists, return it, otherwise create it then return it
// In an error situation, this could return NULL
LSHandle *SysmgrIMEModel::getLunaServiceHandle()
{
    if (!m_serviceHandle) {
        LSError lserror;
        LSErrorInit(&lserror);
        if (LSRegister(NULL, &m_serviceHandle, &lserror))
            LSGmainAttach(m_serviceHandle, HostBase::instance()->mainLoop(), &lserror);
        if (LSErrorIsSet(&lserror)) {
            qCritical() << lserror.message << lserror.file << lserror.line << lserror.func;
            LSErrorFree(&lserror);
        }
    }
    return m_serviceHandle;
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
		QChar qchar;
		switch(key) {
		case Qt::Key_Return:
		case Qt::Key_Enter:
			qchar = '\r';
			break;
		case Qt::Key_Tab:
			qchar = '\t';
			break;
		case Qt::Key_Backspace:
			qchar = '\b';
			break;
		default:
			qchar = QChar(key);
		}

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

void SysmgrIMEModel::applyInitSettings(VirtualKeyboard *ime)
{
    VirtualKeyboardPreferences::instance().applyInitSettings(ime);
}

void SysmgrIMEModel::activateCombo()
{
    VirtualKeyboardPreferences::instance().activateCombo();
}

void SysmgrIMEModel::selectKeyboardCombo(int index)
{
    VirtualKeyboardPreferences::instance().selectKeyboardCombo(index);
}

void SysmgrIMEModel::selectLayoutCombo(const char *layoutName)
{
    VirtualKeyboardPreferences::instance().selectLayoutCombo(layoutName);
}

void SysmgrIMEModel::selectNextKeyboardCombo()
{
    VirtualKeyboardPreferences::instance().selectNextKeyboardCombo();
}

void SysmgrIMEModel::createDefaultKeyboards()
{
    VirtualKeyboardPreferences::instance().createDefaultKeyboards();
}

void SysmgrIMEModel::clearDefaultDeyboards()
{
    VirtualKeyboardPreferences::instance().clearDefaultDeyboards();
}

void SysmgrIMEModel::toggleTapSounds()
{
    VirtualKeyboardPreferences::instance().setTapSounds(!VirtualKeyboardPreferences::instance().getTapSounds());
}

bool SysmgrIMEModel::getTapSounds() const
{
    return VirtualKeyboardPreferences::instance().getTapSounds();
}

int SysmgrIMEModel::getKeyboardComboCount() const
{
    return VirtualKeyboardPreferences::instance().getKeyboardComboCount();
}

bool SysmgrIMEModel::getSpaces2period() const
{
    return VirtualKeyboardPreferences::instance().getSpaces2period();
}

void SysmgrIMEModel::selectKeyboardSize(int size)
{
    VirtualKeyboardPreferences::instance().selectKeyboardSize(size);
}

const char *SysmgrIMEModel::getLanguageFromKeyboardCombo(int index)
{
    VirtualKeyboardPreferences &prefs = VirtualKeyboardPreferences::instance();
    return prefs.getkeyboardCombo(index).language.c_str();
}

const char *SysmgrIMEModel::getLayoutFromKeyboardCombo(int index)
{
    VirtualKeyboardPreferences &prefs = VirtualKeyboardPreferences::instance();
    return prefs.getkeyboardCombo(index).layout.c_str();
}
