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




#ifndef CARDDROPSHADOWEFFECT_H_
#define CARDDROPSHADOWEFFECT_H_

#include "Common.h"

#include <QGraphicsEffect>
#include <QRect>
#include <QVector>
#include <QSize>

#include <qdrawutil.h>

class QGraphicsItem;

class CardDropShadowEffect: public QGraphicsEffect
{
public:

	CardDropShadowEffect(QGraphicsItem *item, QObject *parent = 0);

	void cacheDrawingData();

protected:

	virtual void draw(QPainter *painter);

private:

	Q_DISABLE_COPY(CardDropShadowEffect)

	QVector<QDrawPixmaps::Data> m_drawingData;
	QGraphicsItem* m_item;
	QSize m_itemDims;
};

#endif /* CARDDROPSHADOWEFFECT_H_ */
