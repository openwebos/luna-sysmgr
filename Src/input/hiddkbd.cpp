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




#include "Common.h"

#include <QKbdDriverPlugin>

#include "hiddkbd_qws.h"


class HiddKbdPlugin: public QKbdDriverPlugin {
public:
	HiddKbdPlugin();

	QStringList keys() const;
	QWSKeyboardHandler* create(const QString &driver, const QString &device);
};

HiddKbdPlugin::HiddKbdPlugin() :
	QKbdDriverPlugin() {
    qWarning("%s called\n", __PRETTY_FUNCTION__);
}

QStringList HiddKbdPlugin::keys() const {
	return (QStringList() << "HiddKbd");
}

QWSKeyboardHandler* HiddKbdPlugin::create(const QString &driver, const QString &device) {
	if (driver.compare(QLatin1String("HiddKbd"), Qt::CaseInsensitive))
		return 0;
	return new QWSHiddKbdHandler(driver, device);
}

Q_EXPORT_PLUGIN2(hiddkbd, HiddKbdPlugin)

