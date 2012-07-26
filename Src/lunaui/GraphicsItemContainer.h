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




#ifndef GRAPHICSITEMCONTAINER_H
#define GRAPHICSITEMCONTAINER_H

#include "Common.h"

#include <QBrush>
#include <QGraphicsObject>
#include <QGraphicsSceneMouseEvent>

class Pixmap9TileObject;

class GraphicsItemContainer : public QGraphicsObject
{
public:

	typedef enum ContainerBackgroundType {
		NoBackground = 0,
		SolidRectBackground,
		PopupBackground,
		TransientAlertBackground
	} t_backgroundType;

	GraphicsItemContainer(int width, int height, t_backgroundType backgroundType = NoBackground);
	~GraphicsItemContainer();

	void setBackgroundType(t_backgroundType backgroundType);
	bool hasBackground() const;
	void setBlockGesturesAndMouse(bool block);
	void setBrush(const QBrush& brush);
	void raiseChild(QGraphicsItem* item);
	void resize(int width, int height);
	int width() const;
	int height() const;
	
	virtual QRectF boundingRect() const;

	int getMarginOffset() { return m_bkgImgMarging; }


protected:

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);
	virtual void paintPopup(QPainter* painter);
	virtual bool sceneEvent(QEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);

protected:

	int m_contentWidth;
	int m_contentHeight;
	int m_bkgImgMarging;
	t_backgroundType m_bkgType;
	bool m_blockGesturesAndMouse;
	QBrush m_brush;
	Pixmap9TileObject* m_background9Tile;
};
	

#endif /* GRAPHICSITEMCONTAINER_H */
