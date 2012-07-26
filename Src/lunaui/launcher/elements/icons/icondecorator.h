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




#ifndef ICONDECORATOR_H_
#define ICONDECORATOR_H_

#include <QPointer>
#include <QRectF>
#include <QPointF>
#include <QPoint>

class PixmapObject;
class IconDecorator
{
public:
	IconDecorator();
	IconDecorator(PixmapObject * p_pix,const QRectF& ownerCoordRelativeBounds);
	~IconDecorator();

	//returns the old one
	PixmapObject * setNewPix(PixmapObject * p_newPix, bool show=false);
	bool hitTest(const QPointF& pt) const;
	bool hitTest(const QPoint& pt) const;
	PixmapObject * pix() const;
	void show();
	void hide();

	QRectF m_bounds;

	//TODO: clean up the meaning and usage of m_iconGeom...there are inconsistencies w/ its usage as a geom
	QRect	m_iconGeom;		//geom and pos are related to m_bounds...take care to not lose sync
	QPoint	m_iconPos;
	QPointer<PixmapObject> m_qp_pixmap;
	bool m_shown;
};

#endif /* ICONDECORATOR_H_ */
