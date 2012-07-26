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




#ifndef PAGETAB_H_
#define PAGETAB_H_

#include <QUuid>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QFont>
#include <QFontMetrics>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QTextLayout>
#include <QStaticText>
#include <QPointer>
#include <QMap>
#include <QList>

#include "thingpaintable.h"

class PageTabBar;
class QGraphicsSceneMouseEvent;
class QGesture;
class QGestureEvent;
class QTouchEvent;
class QPanGesture;
class QSwipeGesture;
class QPinchGesture;
class QTapAndHoldGesture;
class QTapGesture;
class FlickGesture;
class Page;

namespace PageTabDisplayMode
{
	enum Enum
	{
		INVALID,
		Normal,
		Selected,
		Highlighted
	};
}

class PixmapObject;
class PageTab : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:

	static const char * PropertyNameIndex;

	friend class PageTabBar;

	PageTab(const QRectF& pageTabGeometry,const QString& label,PageTabBar * p_belongsTo,Page * p_refersToPage=0);
	virtual ~PageTab();

	Page * relatedPage() const;

	virtual bool testForIntersect(const QPointF& tabBarPositionICS,bool highlight=false);
	virtual void syntheticTap();

	virtual quint32 tabIndex() const;

public Q_SLOTS:

	virtual void slotHighlight();
	virtual void slotUnHighlight();

Q_SIGNALS:

	void signalActivatedTap();
	void signalActivatedTapAndHold();

	void signalSlotHighlighted();

protected Q_SLOTS:

	virtual void slotSetDisplayMode(PageTabDisplayMode::Enum mode);

	virtual void slotRelatedPageActive();
	virtual void slotRelatedPageDeactivated();

	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	virtual void slotAnimationFinished() {}

	virtual void slotInteractionsBlocked();
	virtual void slotInteractionsAllowed();

protected:

	virtual void setTabIndex(quint32 i);

	virtual void initObjects();
	virtual void initSignalSlotConnections();

	virtual PixmapObject * setBackground(PixmapObject * p_new,PageTabDisplayMode::Enum mode, bool makeCurrent=false);
	virtual QList<PixmapObject *> setBackgrounds(PixmapObject * p_newNormal,
												PixmapObject * p_newSelected,
												PixmapObject * p_newHighlighted);
	virtual PixmapObject * setLeftVerticalDivider(PixmapObject * p_new);
	virtual PixmapObject * setRightVerticalDivider(PixmapObject * p_new);
	virtual void setLeftVerticalDividerVisible(bool visible);
	virtual void setRightVerticalDividerVisible(bool visible);

	virtual PixmapObject * setBackgroundShadow(PixmapObject * p_new);

	virtual bool resize(const QSize& s);
	virtual bool resize(quint32 newWidth,quint32 newHeight);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);

	virtual bool touchEvent(QTouchEvent * event) { return true; }
	virtual bool touchStartEvent(QTouchEvent *event) { return true; }
	virtual bool touchUpdateEvent(QTouchEvent *event) { return true; }
	virtual bool touchEndEvent(QTouchEvent *event) { return true; }

	virtual bool sceneEvent(QEvent * event);
	virtual bool gestureEvent(QGestureEvent *gestureEvent) { return true; }

	virtual bool flickGesture(FlickGesture *flickEvent,QGestureEvent * baseGestureEvent);
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent,QGestureEvent * baseGestureEvent);
	virtual bool tapGesture(QTapGesture *tapEvent,QGestureEvent * baseGestureEvent);

	// PREREQUISITE: m_geom is valid
	// RESULT: m_labelMaxGeom is set
	virtual void recalculateLabelBoundsForCurrentGeom();

	//	PREREQUISITE: m_labelMaxGeom must be valid, at least its size()
	//  RESULT: m_textLayoutObject will have the correct layout, m_labelGeom will be set correctly for the current label,
	virtual void	redoLabelTextLayout();		//run this after the geometry changes. This is absolutely necessary
												// ALSO run this after the label changes. don't ask, just do it. Not necessary now, but it most likely will be later

	//	PREREQUISITE: m_geom , m_labelMaxGeom and m_labelGeom are set
	//	RESULT:	m_labelPosICS and m_labelPosPntCS are set
	virtual void	recalculateLabelPosition();

private:

	QPointer<PageTabBar>  m_qp_currentTabBarOwner;
	QPointer<Page>			m_qp_relatedToPage;

	QRectF					m_backgroundGeom;
	QRectF					m_backgroundShadowGeom;

	QMap<qint32,QPointer<PixmapObject> > m_modeBackgrounds;
	QPointer<PixmapObject> m_qp_backgroundShadow;

	PageTabDisplayMode::Enum m_currentMode;
	PageTabDisplayMode::Enum m_savedMode;

	bool					m_interactionsBlocked;
	QPointer<PixmapObject>  m_currentBackgroundPmo;	//keep 'em in sync; it's to avoid a map lookup in paint()
	bool					m_showLeftDivider;
	bool					m_showRightDivider;
	QPointer<PixmapObject> m_leftDividerPmo;
	QPointer<PixmapObject> m_rightDividerPmo;

	QPointF					m_leftDividerPosPntCS;
	QPointF					m_rightDividerPosPntCS;

	QString					m_tabLabel;
	QFont					m_textFont;
	QColor 					m_selectedColor;
	QColor					m_unselectedColor;
	QTextLayout				m_textLayoutObject;
	QPointF					m_labelPosICS;				// position in ICS, and corresponds to m_labelGeom
	QPoint					m_labelPosPntCS;			// same, but precomputed to offscreen painter coords relative to 0,0 at top left
	QRect					m_labelMaxGeom;			// precomputed by recalculateLabelBoundsForCurrentGeom()
	QRect					m_labelGeom;			//precomputed by redoLabelTextLayout(); this one is the CURRENT label's box (always <= m_labelMaxGeom)


};



#endif /* PAGETAB_H_ */
