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




#include "dimensionsglobal.h"
#include "pixbutton.h"
#include "pixmapobject.h"
#include "layoutsettings.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QString>
#include <QDebug>
#include <QTextOption>
#include <QEvent>
#include <QGesture>
#include <QGestureEvent>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#include "Settings.h"

QFont PixButton::s_standardButtonFont = QFont();

QFont PixButton::staticLabelFontForButtons()
{
	//TODO: specify a proper font
	static bool fontInitialized = false;
	if (!fontInitialized)
	{
		s_standardButtonFont = QFont(QString::fromStdString(Settings::LunaSettings()->fontQuicklaunch));
		quint32 fontSize = qBound((quint32)2,LayoutSettings::settings()->doneButtonFontSizePx,(quint32)100);
		s_standardButtonFont.setPixelSize(fontSize);
		s_standardButtonFont.setBold(LayoutSettings::settings()->doneButtonFontEmbolden);
	}
	return s_standardButtonFont;
}

PixButton::PixButton(const QRectF& buttonGeometry)
: ThingPaintable(buttonGeometry)
, m_trackingTouch(false)
, m_touchCount(0)
, m_labelVerticalAdjust(0)
{

	setAcceptTouchEvents(true);
	grabGesture(Qt::TapGesture);
}

//virtual
PixButton::~PixButton()
{

}

//virtual
bool PixButton::resize(const QSize& s)
{
	//TODO: IMPLEMENT
	return true;
}

//virtual
bool PixButton::resize(quint32 newWidth,quint32 newHeight)
{
	return resize(QSize(newWidth,newHeight));
}

//virtual
void PixButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
}

//virtual
void PixButton::paintOffscreen(QPainter * painter)
{
}

//virtual
bool PixButton::valid()
{
	return false;
}

//virtual
void PixButton::setLabel(const QString& v)
{
	m_label = v;
	m_textFont = PixButton::staticLabelFontForButtons();
	m_selectedColor = LayoutSettings::settings()->tabBarSelectedTabFontColor;
	m_unselectedColor = LayoutSettings::settings()->doneButtonFontColor;
	recalculateLabelBoundsForCurrentGeom();
	redoLabelTextLayout();
	recalculateLabelPosition();
	update();
}

//virtual
void PixButton::setLabel(const QString& v,quint32 fontSizePx)
{
	m_label = v;
	m_textFont = PixButton::staticLabelFontForButtons();
	quint32 fontSize = qBound((quint32)2,fontSizePx,(quint32)100);
	s_standardButtonFont.setPixelSize(fontSize);
	m_selectedColor = LayoutSettings::settings()->tabBarSelectedTabFontColor;
	m_unselectedColor = LayoutSettings::settings()->doneButtonFontColor;
	recalculateLabelBoundsForCurrentGeom();
	redoLabelTextLayout();
	recalculateLabelPosition();
	update();
}

//virtual
void PixButton::setLabelVerticalAdjust(qint32 adjustPx)
{
	m_labelVerticalAdjust = adjustPx;
	m_textFont = PixButton::staticLabelFontForButtons();
	m_selectedColor = LayoutSettings::settings()->tabBarSelectedTabFontColor;
	m_unselectedColor = LayoutSettings::settings()->doneButtonFontColor;
	recalculateLabelBoundsForCurrentGeom();
	redoLabelTextLayout();
	recalculateLabelPosition();
	update();
}

//protected Q_SLOTS:

//virtual
void PixButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();	//prevent leakage to others
}

//virtual
void PixButton::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{

}

//virtual
void PixButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{

}

//protected:

//virtual
bool PixButton::sceneEvent(QEvent * event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture * g = 0;
		g = ge->gesture(Qt::TapGesture);
		if (g) {
			QTapGesture* tap = static_cast<QTapGesture*>(g);
			if (tap->state() == Qt::GestureFinished) {
				tapGesture(tap);
			}
			return true;
		}
		g = ge->gesture(Qt::TapAndHoldGesture);
		if (g) {
			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
			if (hold->state() == Qt::GestureFinished) {
				tapAndHoldGesture(hold);
			}
			return true;
		}
	}
	else if (event->type() == QEvent::TouchBegin)
	{
		return touchStartEvent(static_cast<QTouchEvent *>(event));
	}
	else if (event->type() == QEvent::TouchUpdate)
	{
		return touchUpdateEvent(static_cast<QTouchEvent *>(event));
	}
	else if (event->type() == QEvent::TouchEnd)
	{
		return touchEndEvent(static_cast<QTouchEvent *>(event));
	}
	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool PixButton::touchStartEvent(QTouchEvent *event)
{
//	++m_touchCount;
//	if (m_touchCount == 1)
//	{
//		Q_EMIT signalFirstContact();
//	}
//	else
//	{
//		Q_EMIT signalContact();
//	}
//	return true;

	event->accept();
	if (m_trackingTouch)
	{
		//already tracking a touch point.
		//consume and ignore
		return true;
	}
	m_trackingTouch = true;
	Q_EMIT signalFirstContact();
	m_trackingTouchId = event->touchPoints().first().id();
	return true;
}

//virtual
bool PixButton::touchUpdateEvent(QTouchEvent *event)
{
	if (!m_trackingTouch)
	{
		return true;
	}

	//dig out the tracked point
	QList<QTouchEvent::TouchPoint> tlist = event->touchPoints();
	for (QList<QTouchEvent::TouchPoint>::const_iterator it = tlist.constBegin();
			it != tlist.constEnd();++it)
	{
		if (it->id() == m_trackingTouchId)
		{
			//check the position. if it's left the button bounds, cancel it all
			if (!boundingRect().contains(it->pos()))
			{
				m_trackingTouch = false;
				Q_EMIT signalCancel();
				return true;
			}
		}
	}

	return true;
}
//virtual
bool PixButton::touchEndEvent(QTouchEvent *event)
{
//	--m_touchCount;
//	if (m_touchCount <= 0)
//	{
//		m_touchCount = 0;
//		Q_EMIT signalLastRelease();
//	}
//	else
//	{
//		Q_EMIT signalRelease();
//	}
//	return true;

	if (!m_trackingTouch)
	{
		return true;
	}

	//dig out the tracked point
	QList<QTouchEvent::TouchPoint> tlist = event->touchPoints();
	for (QList<QTouchEvent::TouchPoint>::const_iterator it = tlist.constBegin();
			it != tlist.constEnd();++it)
	{
		if (it->id() == m_trackingTouchId)
		{
			//check the position. if it's left the button bounds, don't send release
			if (boundingRect().contains(it->pos()))
			{
				//send release
				Q_EMIT signalLastRelease();
			}
			else
			{
				//send cancel
				Q_EMIT signalCancel();
			}
			m_trackingTouch = false;
			return true;
		}
	}
	return true;
}

//virtual
bool PixButton::tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent)
{
	if (!m_trackingTouch)
	{
		return true;
	}
	Q_EMIT signalContact();
	Q_EMIT signalActivated();
	return true;
}
//virtual
bool PixButton::tapGesture(QTapGesture *tapEvent)
{
	if (!m_trackingTouch)
	{
		return true;
	}
	Q_EMIT signalContact();
	Q_EMIT signalActivated();
	return true;
}
//virtual
bool PixButton::customGesture(QGesture *customGesture)
{
	return true;
}


//virtual
void PixButton::recalculateLabelBoundsForCurrentGeom()
{
	//the label geom is the next smallest even integer in height and width from m_geom.
	// if that int is 0 in either case, then it's == m_geom's w or h
	quint32 gw = DimensionsGlobal::roundDown(m_geom.width());
	quint32 gh = DimensionsGlobal::roundDown(m_geom.height());
	QSize s = QSize(
				( gw < 2 ? gw : ( gw - (gw % 2))),
				( gh < 2 ? gh : ( gh - (gh % 2))));

	m_labelMaxGeom = DimensionsGlobal::realRectAroundRealPoint(s).toRect();
}

//virtual
void	PixButton::redoLabelTextLayout()
{
	//TODO: somewhat wasteful. If there is no label, should just exit early and leave a layout that will be left unrendered by paint()
	m_textLayoutObject.clearLayout();
	m_textLayoutObject.setText(m_label);
	m_textLayoutObject.setFont(m_textFont);
	QTextOption textOpts;
	textOpts.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	textOpts.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	m_textLayoutObject.setTextOption(textOpts);

	QFontMetrics textFontMetrics(m_textFont);
	int leading = textFontMetrics.leading();
	int rise = textFontMetrics.ascent();
	qreal height = 0;

	m_textLayoutObject.beginLayout();
	while (height < m_labelMaxGeom.height()) {
		QTextLine line = m_textLayoutObject.createLine();
		if (!line.isValid())
			break;
		line.setLineWidth(m_labelMaxGeom.width());
		if (m_textLayoutObject.lineCount() > 1)
		{
			height += leading;
		}
		line.setPosition(QPointF(0, height));
		height += line.height();
	}
	height = qMin((quint32)DimensionsGlobal::roundUp(height),(quint32)m_labelMaxGeom.height());
	height = DimensionsGlobal::roundDown(height) - (DimensionsGlobal::roundDown(height) % 2);	//force to an even #
	m_textLayoutObject.endLayout();
	//TODO: PIXEL-ALIGN
	m_labelGeom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(m_textLayoutObject.boundingRect().width(),height)).toAlignedRect();
}

//virtual
void	PixButton::recalculateLabelPosition()
{
	//TODO: MYSTERY! don't know why it needs the 2.0 bump in Y. investigate the geom creation in redoLabelTextLayout()
	m_labelPosICS = QPointF(0,2.0+(qreal)(m_labelVerticalAdjust));
	m_labelPosPntCS = (m_labelGeom.topLeft() + m_labelPosICS).toPoint();
}

///////////////////////////////// PixButtonExtraHitArea ///////////////////////////////////////////////////////////////

PixButtonExtraHitArea::PixButtonExtraHitArea(PixButton& ownerButton,const QSize& area)
: m_savedOwnerSize(ownerButton.geometry().size())
, m_qp_ownerButton(&ownerButton)
{
	setFlag(ItemHasNoContents,true);
	QSize effArea = QSize(qMax(area.width(),ownerButton.geometry().size().toSize().width()),
							qMax(area.height(),ownerButton.geometry().size().toSize().height()));
	m_boundingRect = DimensionsGlobal::realRectAroundRealPoint(effArea);
	setParentItem(&ownerButton);
}

//virtual
PixButtonExtraHitArea::~PixButtonExtraHitArea()
{
}

//virtual
void PixButtonExtraHitArea::commonCtor()
{
	if (m_qp_ownerButton)
	{
		connect(m_qp_ownerButton,SIGNAL(destroyed(QObject *)),
				this,SLOT(slotOwnerDestroyed(QObject *)));
		connect(m_qp_ownerButton,SIGNAL(signalThingGeometryChanged(const QRectF&)),
				this,SLOT(slotOwnerGeometryChanged(const QRectF&)));
	}
}

//virtual
QRectF PixButtonExtraHitArea::boundingRect() const
{
	return m_boundingRect;
}

//virtual
void PixButtonExtraHitArea::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
}

//public Q_SLOTS:

void PixButtonExtraHitArea::slotActivate()
{
	//not really "visible" since there isn't any paint but it will make it visible to the event system to get mouse events!
	setVisible(true);
}

void PixButtonExtraHitArea::slotDeactivate()
{
	setVisible(false);
}

void PixButtonExtraHitArea::slotResize(quint32 w,quint32 h)
{
	QSize effArea = QSize(qMax(w,(quint32)m_savedOwnerSize.toSize().width()),
			qMax(h,(quint32)m_savedOwnerSize.toSize().height()));
	m_boundingRect = DimensionsGlobal::realRectAroundRealPoint(effArea);
}

void PixButtonExtraHitArea::slotOwnerGeometryChanged(const QRectF& newGeom)
{
	//try and maintain the proportional size of the hit area to the old geom
	qreal xp = m_boundingRect.width() / m_savedOwnerSize.width();
	qreal yp = m_boundingRect.height() / m_savedOwnerSize.height();
	m_boundingRect = DimensionsGlobal::realRectAroundRealPoint(QSizeF(newGeom.width()*xp,newGeom.height()*yp));
	m_savedOwnerSize = newGeom.size();
}

void PixButtonExtraHitArea::slotOwnerDestroyed(QObject * p)
{
	if (p == m_qp_ownerButton)
	{
		deleteLater();		//commit suicide...later  =)
	}
}
