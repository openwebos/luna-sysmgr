/* @@@LICENSE
*
*      Copyright (c) 2010-2013 LG Electronics, Inc.
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




#ifndef GFXEFFECTBASE_H_
#define GFXEFFECTBASE_H_

#include <QGraphicsEffect>

class GfxEffectBase : public QGraphicsEffect
{
	Q_OBJECT
public:
	GfxEffectBase(QObject * parent=0);
	virtual ~GfxEffectBase();

protected:

	virtual void draw ( QPainter * painter );

};

#endif /* GFXEFFECTBASE_H_ */
