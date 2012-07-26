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




#include "groupanchoritem.h"
#include "dimensionslauncher.h"
#include <QAbstractAnimation>
#include <QGraphicsSceneMouseEvent>

GroupAnchorItem::GroupAnchorItem()
: m_qp_currentUIOwner(0)
{
	setHandlesChildEvents(false);
}

GroupAnchorItem::GroupAnchorItem(LauncherObject * p_belongsTo)
: QGraphicsItemGroup(p_belongsTo)
, m_qp_currentUIOwner(p_belongsTo)
{
	setFlag(QGraphicsItem::ItemHasNoContents,true);
	setHandlesChildEvents(false);
}

//virtual
GroupAnchorItem::~GroupAnchorItem()
{
}

void GroupAnchorItem::setOwner(LauncherObject * p_belongsTo)
{
	m_qp_currentUIOwner = p_belongsTo;
	setParentItem(p_belongsTo);
}

QPointer<QAbstractAnimation> GroupAnchorItem::setAnimation(QAbstractAnimation * p_anim)
{
	QPointer<QAbstractAnimation> rqp(m_qp_currentAnimation);
	m_qp_currentAnimation = p_anim;
	return rqp;
}

//virtual
void GroupAnchorItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{ event->ignore(); }
//virtual
void GroupAnchorItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{ event->ignore(); }
//virtual
void GroupAnchorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{ event->ignore(); }
//virtual
void GroupAnchorItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{ event->ignore(); }
