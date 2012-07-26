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
#include "VirtualKeyboard.h"

// This is to generate physical device layouts on desktop. Keep this undefined unless that's what you're working on!
//#define GENERATE_PRE_LAYOUTS

#ifdef GENERATE_PRE_LAYOUTS
#include "PreKeymap.h"
#endif

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
	QStringList	list;
	VirtualKeyboardFactory * factory = VirtualKeyboardFactory::getFirstFactory();
	while (factory)
	{
		list.push_back(factory->name());
		factory = factory->getNextFactory();
	}
	return list;
}

IMEDataInterface* IMEManager::createIME(const QString& key)
{
	VirtualKeyboardFactory * factory = VirtualKeyboardFactory::getFirstFactory();
	while (factory && factory->name() != key)
		factory = factory->getNextFactory();

	if (factory) {
		SysmgrIMEModel * imeDataInterface = new SysmgrIMEModel();
		InputMethod * keyboard = factory->getVirtualKeyboard(imeDataInterface);
		imeDataInterface->setInputMethod(keyboard);

		return imeDataInterface;
	}
	return NULL;
}

IMEDataInterface * IMEManager::createPreferredIME(int maxWidth, int maxHeight)
{
	VirtualKeyboardFactory::EVirtualKeyboardSupport	bestSupport = VirtualKeyboardFactory::eVirtualKeyboardSupport_NotSupported;
	VirtualKeyboardFactory * bestFactory = NULL;

	VirtualKeyboardFactory * factory = VirtualKeyboardFactory::getFirstFactory();
	while (factory)
	{
		VirtualKeyboardFactory::EVirtualKeyboardSupport	support = factory->getSupport(maxWidth, maxHeight);
		if (support > bestSupport)
		{
			bestFactory = factory;
			bestSupport = support;
		}
		factory = factory->getNextFactory();
	}
	if (bestSupport > VirtualKeyboardFactory::eVirtualKeyboardSupport_NotSupported && bestFactory) {
		g_message("IMEManager::createPreferredIME: selecting '%s'", bestFactory->name().toUtf8().data());
		SysmgrIMEModel * imeDataInterface = new SysmgrIMEModel();
		InputMethod * keyboard = bestFactory->getVirtualKeyboard(imeDataInterface);
		imeDataInterface->setInputMethod(keyboard);

		return imeDataInterface;
	}
	return NULL;
}
