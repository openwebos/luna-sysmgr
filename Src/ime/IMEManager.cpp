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



#include "IMEManager.h"

#include "Logging.h"
#include "SysmgrIMEDataInterface.h"

#include <QDebug>
#include <QDir>
#include <QPluginLoader>
#include <VirtualKeyboard.h>

// This is to generate physical device layouts on desktop. Keep this undefined unless that's what you're working on!
//#define GENERATE_PRE_LAYOUTS

#ifdef GENERATE_PRE_LAYOUTS
#include "PreKeymap.h"
#endif

static QList<VirtualKeyboardFactory *> getVKBFactories()
{
    QList<VirtualKeyboardFactory *> list;
    QString path("/usr/lib/luna");
    qDebug() << "\033[1;33;45m" << Q_FUNC_INFO << "Searching for VKB plugins in" << path << "\033[0m";

    QDir plugindir = QDir(path);
    QStringList files = plugindir.entryList(QDir::Files);
    qDebug() << "\033[1;33;45m" << Q_FUNC_INFO << "Found" << files.count() << "files" << "\033[0m";

    QPluginLoader loader;
    QStringList::const_iterator it = files.constBegin();

    while (it != files.constEnd()) {
        qDebug() << "\033[1;33;40m" << Q_FUNC_INFO << "Checking" << (*it) << "\033[0m";
        loader.setFileName(plugindir.absoluteFilePath(*it));

        VirtualKeyboardFactory *factory =
            qobject_cast<VirtualKeyboardFactory *>(loader.instance());

        if (factory) {
            qDebug() << "\033[1;32;40m" << Q_FUNC_INFO << "Loaded plugin"
                     << (*it) << "successfully" << "\033[0m";
            list.append(factory);
        } else {
            qWarning() << "\033[1;31;40m" << Q_FUNC_INFO << "Failed to load"
                       << (*it) << "\n" << loader.errorString() << "\033[0m";

        }

        ++it;
    }

    return list;
}

IMEManager::IMEManager()
{
#ifdef GENERATE_PRE_LAYOUTS
    Pre_Keyboard::PreKeymap	keymap;
    keymap.setRect(0, 0, 500, 200);

    const Pre_Keyboard::PreKeymap::LayoutFamily * qwerty = Pre_Keyboard::PreKeymap::LayoutFamily::findLayoutFamily("qwerty", false);
    const Pre_Keyboard::PreKeymap::LayoutFamily * qwertz = Pre_Keyboard::PreKeymap::LayoutFamily::findLayoutFamily("qwertz", false);
    const Pre_Keyboard::PreKeymap::LayoutFamily * azerty = Pre_Keyboard::PreKeymap::LayoutFamily::findLayoutFamily("azerty", false);
    if (VERIFY(qwerty))
    {
        keymap.setLayoutFamily(qwerty);
        keymap.generateKeyboardLayout("pre_qwerty.xml");
    }
    if (VERIFY(qwertz))
    {
        keymap.setLayoutFamily(qwertz);
        keymap.generateKeyboardLayout("pre_qwertz.xml");
    }
    if (VERIFY(azerty))
    {
        keymap.setLayoutFamily(azerty);
        keymap.generateKeyboardLayout("pre_azerty.xml");
    }

    exit(0);
#endif
}

QStringList IMEManager::availableIMEs() const
{
    QStringList	names;
    QList<VirtualKeyboardFactory *> factories = getVKBFactories();
    QList<VirtualKeyboardFactory *>::const_iterator it = factories.constBegin();

    while (it != factories.constEnd()) {
        names.append((*it)->name());
        ++it;
    }

    return names;
}

IMEDataInterface *IMEManager::createIME(const QString &key)
{
    QList<VirtualKeyboardFactory *> factories = getVKBFactories();
    QList<VirtualKeyboardFactory *>::const_iterator it = factories.constBegin();
    VirtualKeyboardFactory *factory = 0;
    InputMethod *keyboard = 0;

    while (it != factories.constEnd()) {
        if ((*it)->name() == key) {
            factory = *it;
            break;
        }

        ++it;
    }

    if (factory) {
        SysmgrIMEModel *imeDataInterface = new SysmgrIMEModel();
        keyboard = factory->newVirtualKeyboard(imeDataInterface);

        if (keyboard) {
            qDebug() << "\033[1;32;40m" << Q_FUNC_INFO
                     << QString("Selecting \"%1\"").arg(factory->name())
                     << "\033[0m";
            imeDataInterface->setInputMethod(keyboard);
            return imeDataInterface;
        }

        delete imeDataInterface;
    }

    qCritical() << "\033[1;33;41m" << Q_FUNC_INFO << "Unable to create keyboard!"
                << "factory:" << factory << "keyboard:" << keyboard << "\033[0m";
    return 0;
}

IMEDataInterface *IMEManager::createPreferredIME(int maxWidth,
                                                 int maxHeight,
                                                 int dpi,
                                                 const std::string locale)
{
    QList<VirtualKeyboardFactory *> factories = getVKBFactories();
    QList<VirtualKeyboardFactory *>::const_iterator it = factories.constBegin();
    VirtualKeyboardFactory::EVirtualKeyboardSupport bestSupport =
        VirtualKeyboardFactory::eVirtualKeyboardSupport_NotSupported;
    VirtualKeyboardFactory *bestFactory = 0;
    InputMethod *keyboard = 0;

    while (it != factories.constEnd()) {
        VirtualKeyboardFactory::EVirtualKeyboardSupport support =
            (*it)->getSupport(maxWidth, maxHeight, dpi, locale);

        if (support > bestSupport) {
            bestFactory = *it;
            bestSupport = support;
        }

        ++it;
    }

    if (bestSupport > VirtualKeyboardFactory::eVirtualKeyboardSupport_NotSupported &&
        bestFactory) {
        SysmgrIMEModel *imeDataInterface = new SysmgrIMEModel();
        keyboard = bestFactory->newVirtualKeyboard(imeDataInterface);

        if (keyboard) {
            qDebug() << "\033[1;32;40m" << Q_FUNC_INFO
                     << QString("Selecting \"%1\"").arg(bestFactory->name())
                     << "\033[0m";
            imeDataInterface->setInputMethod(keyboard);
            return imeDataInterface;
        }

        delete imeDataInterface;
    }

    qCritical() << "\033[1;33;41m" << Q_FUNC_INFO << "Unable to create keyboard!"
                << "bestFactory:" << bestFactory << "keyboard:" << keyboard << "\033[0m";
    return 0;
}
