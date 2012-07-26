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




#ifndef GROUPANCHORITEM_H_
#define GROUPANCHORITEM_H_

#include <QObject>
#include <QGraphicsItemGroup>
#include <QRectF>
#include <QPointer>

class LauncherObject;
class QAbstractAnimation;
class QGraphicsSceneMouseEvent;

class GroupAnchorItem : public QObject , public QGraphicsItemGroup
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)
	Q_PROPERTY(QPointF pos READ pos WRITE setPos)

public:
	GroupAnchorItem();
	GroupAnchorItem(LauncherObject * p_belongsTo);
	virtual ~GroupAnchorItem();

	void setOwner(LauncherObject * p_belongsTo);

	QPointer<QAbstractAnimation> setAnimation(QAbstractAnimation * p_anim);	//does NOT stop or mod the animation

protected:

	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);


private Q_SLOTS:

	void slotAnimationFinished() {}

private:

	QPointer<LauncherObject> m_qp_currentUIOwner;
	QPointer<QAbstractAnimation> m_qp_currentAnimation;		//UI owns this! this is just a convenient place to store it
};

#endif /* GROUPANCHORITEM_H_ */
