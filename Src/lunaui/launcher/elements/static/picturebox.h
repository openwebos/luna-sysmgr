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




#ifndef PICTUREBOX_H_
#define PICTUREBOX_H_

#include "thingpaintable.h"
#include <QString>
#include <QPointer>
#include <QFont>
#include <QTextLayout>

class PixmapObject;

namespace PictureBoxHorizontalAlignment
{
	enum Enum
	{
		INVALID,
		Left,
		Right,
		Center
	};
}

namespace PictureBoxVerticalAlignment
{
	enum Enum
	{
		INVALID,
		Top,
		Bottom,
		Center
	};
}

class PictureBox : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:

	PictureBox(PixmapObject * p_mainPmo,PixmapObject * p_backgroundPmo = 0,const QRectF& geom = QRectF());
	virtual ~PictureBox();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);

protected:

	QString m_innerText;
	QPointer<PixmapObject> m_qp_background;
	QPointer<PixmapObject> m_qp_innerPic;

};

#endif /* PICTUREBOX_H_ */
