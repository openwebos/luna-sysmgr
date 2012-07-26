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




#ifndef GFXSEPIAEFFECT_H_
#define GFXSEPIAEFFECT_H_

#include "gfxeffectbase.h"

class GfxSepiaEffect : public GfxEffectBase
{
	Q_OBJECT

public:
	GfxSepiaEffect(QObject * parent=0);
	virtual ~GfxSepiaEffect();

protected:

	virtual void draw ( QPainter * painter );

	virtual void sepiaTone(QPixmap * p_pix);

	QPixmap * m_p_pix;
	QPoint m_offset;
};

#endif /* GFXSEPIAEFFECT_H_ */
