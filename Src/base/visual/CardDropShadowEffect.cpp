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




#include "CardDropShadowEffect.h"

#include <QGraphicsItem>
#include "Common.h"

#include <QPainter>
#include <QPixmap>
#include <QMargins>

#include "Settings.h"
#include "glib.h"

static const int kShadowWidth = 20;
static const int kShadowOffsetY = 5;

static QPixmap* s_shadowPixmap = 0;

CardDropShadowEffect::CardDropShadowEffect(QGraphicsItem *item, QObject *parent) 
	: QGraphicsEffect(parent)
	, m_item(item)
{
	if (G_UNLIKELY(!s_shadowPixmap)) {
		std::string tileImagePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/card-shadow-tile.png";
		s_shadowPixmap = new QPixmap(tileImagePath.c_str());
	}

	cacheDrawingData();
}

void CardDropShadowEffect::draw(QPainter *painter)
{
	Q_ASSERT(s_shadowPixmap != 0);

	qDrawPixmaps(painter, m_drawingData.constData(), m_drawingData.size(), *s_shadowPixmap);
	
	// paint the item, of course
    drawSource(painter);
}

void CardDropShadowEffect::cacheDrawingData()
{
	Q_ASSERT(m_item != 0);
	Q_ASSERT(s_shadowPixmap != 0);

	QRect bounds = m_item->boundingRect().toRect();
	if (m_itemDims == bounds.size())
		return;
	m_itemDims = bounds.size();
	
	bounds.translate(-kShadowWidth, -kShadowWidth);
	bounds.setHeight(bounds.height() + 2 * kShadowWidth);
	bounds.setWidth(bounds.width() + 2 * kShadowWidth);
	bounds.moveTop(bounds.y() + kShadowOffsetY);

	QMargins margins(s_shadowPixmap->width()/2, s_shadowPixmap->height()/2,
					 s_shadowPixmap->width()/2, s_shadowPixmap->height()/2);

	m_drawingData = qCalculateFrameBorderData(bounds, margins, s_shadowPixmap->rect(), margins);
}

