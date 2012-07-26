/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef SAFEFILEOPS_H_
#define SAFEFILEOPS_H_

#include <QSettings>
#include <QString>

namespace DimensionsSystemInterface
{


class SafeFileOperator : public QObject
{
	Q_OBJECT

public:
	enum OpType
	{
		Read,
		Write
	};

	SafeFileOperator(OpType op,const QString& fileName,QSettings::Format format, QObject *parent = 0);
	virtual ~SafeFileOperator();

	QSettings& safeSettings();

protected:

	OpType m_opMode;
	QString m_actualName;
	QString m_originalPath;
	QString	m_tempName;
	QString m_tempBasename;

	int m_tmpFh;
	bool m_unsafe;
	QSettings * m_p_settings;
};

}

#endif /* SAFEFILEOPS_H_ */
