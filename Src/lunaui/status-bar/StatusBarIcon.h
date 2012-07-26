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




#ifndef STATUSBARICON_H
#define STATUSBARICON_H

#include <QPixmap>
#include <QObject>
#include <QPainter>
#include <QGraphicsObject>
#include <QPointer>
#include <QEasingCurve>
#include "VariantAnimation.h"

class StatusBarIcon;

#define ICON_SPACING    5


class StatusBarIconContainer {
public:
	virtual void updateBoundingRect(bool forceRepaint=false)=0;
	virtual void iconRemoved(StatusBarIcon* icon) {}
};

class StatusBarIcon : public QObject
{
	Q_OBJECT

public:
	StatusBarIcon(StatusBarIconContainer* parent);
	~StatusBarIcon();

	QRect boundingRect() const;

	void show();
	void hide();

	bool isVisible() const { return m_visible; }

	qreal visiblePortion() { return m_animWidth; }

	void paint(QPainter* painter, QPoint centerRight, int width=-1);

	void setImage(QPixmap* img);

protected Q_SLOTS:
	void slotAnimFinished();

protected:

	virtual void setVisible(bool visible) { m_visible = visible; }

	void animValueChanged(const QVariant& value);
	void updateBoundingRect(bool forceRepaint = false);

	enum AnimationState{
		NO_ANIMATION,
		SLIDE_ANIMATION
	};

	QPixmap* m_imgPtr;

	QRect m_bounds;
	bool  m_visible;

	StatusBarIconContainer* m_parent;

	AnimationState m_animState;

	typedef VariantAnimation<StatusBarIcon> tIconAnim;
	QEasingCurve m_curve;
	QPointer<tIconAnim> m_animPtr;
	qreal m_animWidth;
	qreal m_animOpacity;
};

#endif /* STATUSBARICON_H */
