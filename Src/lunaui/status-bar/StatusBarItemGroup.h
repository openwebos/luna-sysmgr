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




#ifndef STATUSBARITEMGROUP_H
#define STATUSBARITEMGROUP_H

#include "StatusBar.h"
#include "StatusBarItem.h"
#include <QPixmap>
#include <QPointer>
#include <QPropertyAnimation>
#include <QGraphicsSceneMouseEvent>

class StatusBarItemGroup : public StatusBarItem
{
	Q_OBJECT
	Q_PROPERTY(qreal arrowAnimProgress READ arrowAnimProgress WRITE setArrowAnimProgress)
	Q_PROPERTY(qreal overlayOpacity READ overlayOpacity WRITE setOverlayOpacity)

public:

	StatusBarItemGroup(int height, bool hasArrow, bool showSeparator, Alignment);
	~StatusBarItemGroup();

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void setHeight(int h);

	int separatorWidth();

	void addItem(StatusBarItem* item);

	void show();
	void hide();

	void setMenuObject(QGraphicsObject* item);
	void activate();
	void deactivate();
	bool isActivated() { return m_active; }

	void setActionable(bool actionable);
	bool isActionable() const { return m_actionable; }

	void actionTriggered();

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

	qreal arrowAnimProgress() { return m_arrowAnimProg; }
	void setArrowAnimProgress(qreal prog);

	qreal overlayOpacity() { return m_overlayOpacity; }
	void setOverlayOpacity(qreal opacity);

Q_SIGNALS:

	void signalActionTriggered(bool active);
	void signalActivated(StatusBarItemGroup* group);

private Q_SLOTS:
	void slotChildBoundingRectChanged();
	void slotFadeAnimationFinished();
	void slotOverlayAnimationFinished();
	void slotOverlayAnimValueChanged(const QVariant& value);

private:
	void layout();
	void layoutCenter();
	void layoutRight();
	void layoutLeft();

	int      m_height;
	QVector<StatusBarItem*>  m_items; // items order from right (0) to left
	bool     m_hasArrow;
	qreal    m_arrowAnimProg;
	qreal    m_overlayOpacity;
	bool     m_active;
	bool     m_actionable;
	bool     m_mouseDown;
	QPixmap* m_activeBkgPix;
//	QPixmap* m_pressedBkgPix;
	QPixmap* m_arrowPix;
	QPixmap* m_separatorPix;

	QGraphicsObject* m_menuObj;

	QPointer<QPropertyAnimation> m_opacityAnimPtr;
	QPointer<QPropertyAnimation> m_arrowFadeAnimPtr;
	QPointer<QPropertyAnimation> m_overlayAnimPtr;
};



#endif /* STATUSBARITEMGROUP_H */
