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




#ifndef GFXEFFECTGLSLBASE_H_
#define GFXEFFECTGLSLBASE_H_

#include "gfxeffectbase.h"
#include <QMap>
#include <QString>

class GfxEffectGLSLBase : public GfxEffectBase
{
	Q_OBJECT
public:

	GfxEffectGLSLBase(QObject * parent=0);
	virtual ~GfxEffectGLSLBase();

	//this also initializes the map
	static const QMap<QString,QString>& globalFragmentShaders();

protected:

	virtual void draw ( QPainter * painter );

	static QMap<QString,QString> s_fragmentShaderPrograms;
	QMap<QString,QString> m_localFragmentShaderPrograms;
};

#endif /* GFXEFFECTGLSLBASE_H_ */
