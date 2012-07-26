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




#include "Common.h"

#include "GraphicsItemContainer.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QEvent>
#include <QGesture>
#include <QGestureEvent>
#include "FlickGesture.h"
#include "Settings.h"

// includes for the 9tile
#include "pixmaploader.h"
#include "pixmap9tileobject.h"


static const int kPopupImgMargin = 20;

GraphicsItemContainer::GraphicsItemContainer(int width, int height, t_backgroundType backgroundType)
	: m_contentWidth(width)
	, m_contentHeight(height)
	, m_blockGesturesAndMouse(false)
	, m_background9Tile(0)
	, m_bkgImgMarging(0)
	, m_bkgType(NoBackground)
{
	setBackgroundType(backgroundType);
}

GraphicsItemContainer::~GraphicsItemContainer()
{    
}

void GraphicsItemContainer::setBlockGesturesAndMouse(bool block)
{
	if(m_blockGesturesAndMouse == block)
		return;

	m_blockGesturesAndMouse = block;

	if(m_blockGesturesAndMouse) {
		grabGesture(Qt::TapGesture);
		grabGesture(Qt::TapAndHoldGesture);
		grabGesture(Qt::PinchGesture);
		grabGesture((Qt::GestureType) SysMgrGestureFlick);
		grabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	} else {
		ungrabGesture(Qt::TapGesture);
		ungrabGesture(Qt::TapAndHoldGesture);
		ungrabGesture(Qt::PinchGesture);
		ungrabGesture((Qt::GestureType) SysMgrGestureFlick);
		ungrabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	}
}

void GraphicsItemContainer::setBrush(const QBrush& brush)
{
	m_brush = brush;
	setBackgroundType((m_brush.style() != Qt::NoBrush) ? SolidRectBackground : NoBackground);
}

void GraphicsItemContainer::setBackgroundType(t_backgroundType background)
{
	if(background == m_bkgType)
		return;

	if(m_background9Tile) {
		delete m_background9Tile;
		m_background9Tile = 0;
	}

	m_bkgType = background;
	setFlag(QGraphicsItem::ItemHasNoContents, m_bkgType == NoBackground);

	if(m_bkgType == PopupBackground) {

		m_bkgImgMarging = kPopupImgMargin;

		std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/popup-bg.png";
		m_background9Tile = PixmapObjectLoader::instance()->quickLoadNineTiled (QString (filePath.c_str()),
				(quint32)(m_bkgImgMarging),
				(quint32)(m_bkgImgMarging),
				(quint32)(m_bkgImgMarging),
				(quint32)(m_bkgImgMarging));
	} else if(m_bkgType == TransientAlertBackground) {

			m_bkgImgMarging = kPopupImgMargin;

			std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/transient-alart-bg.png";
			m_background9Tile = PixmapObjectLoader::instance()->quickLoadNineTiled (QString (filePath.c_str()),
					(quint32)(m_bkgImgMarging),
					(quint32)(m_bkgImgMarging),
					(quint32)(m_bkgImgMarging),
					(quint32)(m_bkgImgMarging));
	}
}

bool GraphicsItemContainer::hasBackground() const
{
    return m_bkgType != NoBackground;
}

QRectF GraphicsItemContainer::boundingRect() const
{
	if(m_bkgType == PopupBackground)
		return QRectF(-m_contentWidth/2 - m_bkgImgMarging, -m_contentHeight/2 - m_bkgImgMarging,
				      m_contentWidth + 2*m_bkgImgMarging, m_contentHeight + 2*m_bkgImgMarging);
	else
		return QRectF(-m_contentWidth/2, -m_contentHeight/2, m_contentWidth, m_contentHeight);
}

int GraphicsItemContainer::width() const
{
	if(m_bkgType == PopupBackground)
		return m_contentWidth + 2*m_bkgImgMarging;
	else
		return m_contentWidth;
}

int GraphicsItemContainer::height() const
{
	if(m_bkgType == PopupBackground)
		return m_contentHeight + 2*m_bkgImgMarging;
	else
		return m_contentHeight;
}

void GraphicsItemContainer::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (m_bkgType == NoBackground)
		return;
	
	if((m_bkgType == PopupBackground) || (m_bkgType == TransientAlertBackground)) {
		paintPopup(painter);
		return;
	}

	// Override the opacity of this item so it is ALWAYS opaque. This is used during
	// the Dock mode in/out animation.
	
	QPainter::CompositionMode previous = painter->compositionMode();
	qreal prevOp = painter->opacity();
	
	painter->setCompositionMode(QPainter::CompositionMode_Source);
	painter->setOpacity(1.0);
	
	painter->fillRect(-m_contentWidth/2, -m_contentHeight/2, m_contentWidth, m_contentHeight, m_brush);
	
	painter->setCompositionMode(previous);
	painter->setOpacity(prevOp);
}

void GraphicsItemContainer::paintPopup(QPainter* painter)
{
	if (m_background9Tile) {
		m_background9Tile->resize(m_contentWidth + 2*m_bkgImgMarging, m_contentHeight + 2*m_bkgImgMarging);
		m_background9Tile->paint (painter, QPointF (-m_contentWidth/2 - m_bkgImgMarging, -m_contentHeight/2 - m_bkgImgMarging));
	}

}

void GraphicsItemContainer::raiseChild(QGraphicsItem* item)
{
	scene()->removeItem(item);
	item->setParentItem(this);
}

void GraphicsItemContainer::resize(int width, int height)
{
	m_contentWidth = width;
	m_contentHeight = height;
	prepareGeometryChange();
}


bool GraphicsItemContainer::sceneEvent(QEvent* event)
{
	switch (event->type()) {
	case QEvent::GestureOverride: {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QList<QGesture*> activeGestures = ge->activeGestures();
		Q_FOREACH(QGesture* g, activeGestures) {
			if (m_blockGesturesAndMouse && g->hasHotSpot()) {
				QPointF pt = ge->mapToGraphicsScene(g->hotSpot());
				ge->accept(g);
			}
		}
		break;
	}

	case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
	case QEvent::TouchEnd:
		if(isVisible())
			return true;

	default:
		break;
	}

	return QGraphicsObject::sceneEvent(event);
}

void GraphicsItemContainer::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if(m_blockGesturesAndMouse) {
		event->accept();
	} else {
		event->ignore();
	}
}

