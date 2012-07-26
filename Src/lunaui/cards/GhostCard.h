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




#ifndef GHOSTCARD_H
#define GHOSTCARD_H

#include "Common.h"

#include "CardWindow.h"

class GhostCard : public QGraphicsObject
{
	Q_OBJECT
	Q_PROPERTY(CardWindow::Position position READ position WRITE setPosition)

public:

	GhostCard(const QPixmap& pixmap, const QPainterPath& path = QPainterPath(), 
                    const CardWindow::Position& pos = CardWindow::Position());
	virtual ~GhostCard();
	
	virtual QRectF boundingRect() const { return m_boundingRect; }

	CardWindow::Position position() const { return m_position; }
	void setPosition(const CardWindow::Position& pos);

protected:

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    QPixmap m_pixmap;
	QPainterPath m_painterPath;
    QRectF m_boundingRect;
	CardWindow::Position m_position;
};

#endif /* CLONEDCARD_H */
