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




#ifndef PIXBUTTON_H_
#define PIXBUTTON_H_

#include "thingpaintable.h"
#include <QRectF>
#include <QRect>
#include <QString>
#include <QColor>
#include <QPointF>
#include <QFont>
#include <QTextLayout>

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
class PixmapObject;


class PixButton;
/*
 *
 * PixButtonExtraHitArea - enlarges the hit area beyond the bounds of the button
 *
 * supports only rectangular areas
 *
 */
class PixButtonExtraHitArea : public QGraphicsObject
{
	Q_OBJECT

public:

	friend class PixButton;

	//area will be min bounded by the button's geom; this class can only be used to enlarge a hit area
	//	it makes no sense in the context of trying to make it smaller, since the button's own hit detection
	//	will trigger up to its own bounds
	PixButtonExtraHitArea(PixButton& ownerButton,const QSize& area);
	virtual ~PixButtonExtraHitArea();
	virtual void commonCtor();

	virtual QRectF boundingRect() const;
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);

Q_SIGNALS:
	void signalHit(const QPointF pt = QPointF());

public Q_SLOTS:

	void slotActivate();
	void slotDeactivate();
	//use slotResize for when the hit area should be deliberately resized to get a larger hit area; don't use it as a response to an auto-resize
	//	e.g. when the UI resizes and the cascading chain of resize() calls comes all the way down to the PixButtons.
	// for that, slotOwnerGeometryChanged will take care of the details
	void slotResize(quint32 w,quint32 h);
	void slotOwnerGeometryChanged(const QRectF& newGeom);
	void slotOwnerDestroyed(QObject * p);

protected:

	QSizeF m_savedOwnerSize;
	QRectF m_boundingRect;		//doesn't need a geom since it'll never paint...so brect == geom
	QPointer<PixButton> m_qp_ownerButton;

};

class PixButton : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:
	PixButton(const QRectF& buttonGeometry);
	virtual ~PixButton();

	virtual bool resize(const QSize& s);
	virtual bool resize(quint32 newWidth,quint32 newHeight);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter * painter);
	//TODO: need other paintOffscreen flavors, e.g. PixmapHugeObject

	virtual bool valid();

	virtual void setLabel(const QString& v);
	//override to make label bigger/smaller than the default
	virtual void setLabel(const QString& v,quint32 fontSizePx);
	virtual void setLabelVerticalAdjust(qint32 adjustPx);

Q_SIGNALS:

	void signalFirstContact();
	void signalContact();
	void signalRelease();
	void signalLastRelease();
	void signalCancel();
	void signalActivated();		//high level signal indicating a full state change; the 'dumbest' and most ambiguous of all the signals
								//useful for when the signal watcher doesn't care about what kind of button it is, it just wants to know when
								// it was clicked, tapped, whatever

protected:

	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	virtual bool touchStartEvent(QTouchEvent *event);
	virtual bool touchUpdateEvent(QTouchEvent *event);
	virtual bool touchEndEvent(QTouchEvent *event);

	virtual bool sceneEvent(QEvent * event);
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent);
	virtual bool tapGesture(QTapGesture *tapEvent);
	virtual bool customGesture(QGesture *customGesture);

	//unused
	virtual bool panGesture(QPanGesture *panEvent) { return true; }
	virtual bool swipeGesture(QSwipeGesture *swipeEvent) { return true; }
	virtual bool flickGesture(FlickGesture *flickEvent) { return true; }
	virtual bool pinchGesture(QPinchGesture *pinchEvent) { return true; }

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

	static QFont staticLabelFontForButtons();

protected:

	// touch related
	bool	m_trackingTouch;
	qint32  m_trackingTouchId;
	// --end touch related

	static QFont			s_standardButtonFont;
	qint32 					m_touchCount;
	QString					m_label;
	QFont					m_textFont;
	QColor 					m_selectedColor;
	QColor					m_unselectedColor;
	QTextLayout				m_textLayoutObject;
	qint32					m_labelVerticalAdjust;		//will be computed into the positions below; this just stores it when setLabelVerticalAdjust() is called
	QPointF					m_labelPosICS;				// position in ICS, and corresponds to m_labelGeom
	QPoint					m_labelPosPntCS;			// same, but precomputed to offscreen painter coords relative to 0,0 at top left
	QRect					m_labelMaxGeom;			// precomputed by recalculateLabelBoundsForCurrentGeom()
	QRect					m_labelGeom;			//precomputed by redoLabelTextLayout(); this one is the CURRENT label's box (always <= m_labelMaxGeom)
};

#endif /* PIXBUTTON_H_ */
