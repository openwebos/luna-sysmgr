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

#include <qmousedriverplugin_qws.h>

#include "hiddtp_qws.h"

class HiddTpPlugin: public QMouseDriverPlugin {
public:
	HiddTpPlugin();

	QStringList keys() const;
	QWSMouseHandler* create(const QString &driver, const QString &device);
};

HiddTpPlugin::HiddTpPlugin() : QMouseDriverPlugin() {
    qWarning("%s called\n", __PRETTY_FUNCTION__);
}

QStringList HiddTpPlugin::keys() const {
	return (QStringList() << "HiddTp");
}

QWSMouseHandler* HiddTpPlugin::create(const QString &driver, const QString &device) {
	if (driver.compare(QLatin1String("HiddTp"), Qt::CaseInsensitive))
		return 0;
	return new QWSHiddTpHandler(driver, device);
}

Q_EXPORT_PLUGIN2(hiddtp, HiddTpPlugin)


