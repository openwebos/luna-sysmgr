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




#ifndef QT_UTILS_H_
#define QT_UTILS_H_

#include "Common.h"

// If you want to use this qHash wrapper for STL strings,
// make sure that you include this file before you include <QHash>
#include <string>

static inline unsigned int qHash(const std::string& str) __attribute__((unused));

#include <QString>
#include <QHash>

static inline __attribute__((unused)) QString qFromUtf8Stl(const std::string& str)
{
	return QString::fromUtf8(str.c_str(), str.length());
}

static inline QString fromStdUtf8(const std::string& str)
{
	return QString::fromUtf8(str.c_str(), str.size());
}

static inline unsigned int qHash(const std::string& str)
{
	return qHash(QByteArray::fromRawData(str.c_str(), str.length()));
}

#endif

