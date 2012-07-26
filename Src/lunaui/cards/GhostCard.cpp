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




#include "GhostCard.h"

GhostCard::GhostCard(const QPixmap& pixmap, const QPainterPath& path, const CardWindow::Position& pos)
    : m_pixmap(pixmap)
    , m_painterPath(path)
    , m_boundingRect(path.boundingRect())
    , m_position(pos)
{
}

GhostCard::~GhostCard()
{
}

void GhostCard::setPosition(const CardWindow::Position& pos)
{
    m_position = pos;
    setTransform(m_position.toTransform());
}

void GhostCard::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (m_pixmap.isNull())
        return;

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter->setBrushOrigin(m_boundingRect.x(), m_boundingRect.y());
    painter->fillPath(m_painterPath, m_pixmap);
    painter->setBrushOrigin(0, 0);

    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
}

