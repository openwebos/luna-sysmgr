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




#ifndef PIXBUTTONSIMPLE_H_
#define PIXBUTTONSIMPLE_H_

#include "pixbutton.h"
#include <QPointer>

class PixButtonSimple : public PixButton
{
	Q_OBJECT
public:
	PixButtonSimple(PixmapObject * p_pix);
	PixButtonSimple(PixmapObject * p_pix,const QSize& hitAreaSize);
	virtual ~PixButtonSimple();
	virtual void commonCtor();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(PixmapObject * p_pix);
	//TODO: need other paintOffscreen flavors, e.g. PixmapHugeObject

	virtual bool valid();

protected:

	QPointer<PixmapObject>  m_qp_pix;
	bool m_valid;
	QPointer<PixButtonExtraHitArea> m_qp_hitArea;
};

#endif /* PIXBUTTONSIMPLE_H_ */
