
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




#ifndef OVERLAYWINDOWMANAGER_P_H_
#define OVERLAYWINDOWMANAGER_P_H_

#include "Common.h"

#include <QGesture>
#include <QGestureEvent>
#include <QGraphicsObject>
#include <QPainter>
#include <QPointer>

#include "Settings.h"
#include "HostBase.h"
#include "Localization.h"
#include "SystemUiController.h"

#include "pixmapobject.h"
#include "dimensionsglobal.h"
#include "layoutsettings.h"

class SearchPill : public QGraphicsObject
{
	Q_OBJECT

public:
	SearchPill(PixmapObject * p_mainPixmap,PixmapObject * p_iconPixmap,const QRectF& geom, QGraphicsItem* parent)
		: QGraphicsObject(parent)
	, m_qp_pillPmo(p_mainPixmap)
	, m_qp_iconPmo(p_iconPixmap)
	, m_noResize(true)
	{
		// Cache the SearchPill in the QPixmapCache
		// to improve speed during animations and panning
                setCacheMode(QGraphicsItem::DeviceCoordinateCache);
                m_geom = geom;
		m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
		if (m_qp_pillPmo)
		{
			m_qp_pillPmo->resize(m_geom.size().toSize());
		}
		positionIconWithin();

		QFont font(qFromUtf8Stl(Settings::LunaSettings()->fontQuicklaunch), 18);
		font.setPixelSize(18);
		font.setStyle(QFont::StyleOblique);
		QString title = qFromUtf8Stl(LOCALIZED("Just type..."));

		QRect textBounds(0, 0, m_boundingRect.width(), m_boundingRect.height());
		textBounds.adjust(20,0,-45,0);

		QFontMetrics metrics(font);
		title = metrics.elidedText(title, Qt::ElideRight, textBounds.width());

		int width = metrics.width(title);
		QPainter painter;


		m_textPix = QPixmap(width + 20, textBounds.height());
		m_textPix.fill(Qt::transparent);
		painter.begin(&m_textPix);
		painter.setPen(Qt::white);
		painter.setFont(font);
		painter.setOpacity(0.8);
		painter.drawText(textBounds, title, QTextOption(Qt::AlignLeft|Qt::AlignVCenter));
		painter.end();

		m_textPos = m_geom.topLeft() + QPointF(LayoutSettings::settings()->searchPillInnerTextAdjust);
		grabGesture(Qt::TapGesture);
	}

	~SearchPill()
	{
		ungrabGesture(Qt::TapGesture);
	}

	virtual bool sceneEvent(QEvent* event)
	{
		if (event->type() == QEvent::GestureOverride) {

			if (canInteract()) {
				event->accept();
			}
		}
		else if (event->type() == QEvent::Gesture) {

			QGestureEvent* ge = static_cast<QGestureEvent*>(event);
			QGesture* g = ge->gesture(Qt::TapGesture);
			if (g && g->state() == Qt::GestureFinished && canInteract()) {
				Q_EMIT signalTapped();
			}
		}
		return QGraphicsObject::sceneEvent(event);
	}

	virtual QRectF geometry() const
	{
		return m_geom;
	}

	virtual QRectF boundingRect() const
	{
		return m_boundingRect;
	}

	void setNoResize(bool v=true)
	{
		m_noResize = v;
	}

	void positionIconWithin()
	{
		if (!m_qp_iconPmo)
		{
			return;
		}

		QRectF iconGeom = DimensionsGlobal::realRectAroundRealPoint(m_qp_iconPmo->size());
		qreal rightOffset = (qreal)qMin(LayoutSettings::settings()->searchPillInnerIconRightOffset,(quint32)(m_geom.right()-iconGeom.width()));
		m_iconPos = QPointF(m_geom.right()-iconGeom.width()-rightOffset,
				m_geom.top()+(m_geom.height()-iconGeom.height())/2.0);
	}

	void resize(int width, int height)
	{
		if (m_noResize)
		{
			return;
		}
		//make the width and height as nearest even int
		prepareGeometryChange();
		QSize newSize = QSize((width < 2 ? 2 : width - (width %2)),(height < 2 ? 2 : height - (height %2)));
		m_geom = DimensionsGlobal::realRectAroundRealPoint(newSize);
		m_boundingRect = m_geom.adjusted(-0.5,-0.5,0.5,0.5);
		if (m_qp_pillPmo)
		{
			m_qp_pillPmo->resize(newSize);
		}
		//reposition icon
		positionIconWithin();
		//and text
		m_textPos = m_geom.topLeft() + QPointF(LayoutSettings::settings()->searchPillInnerTextAdjust);
	}

#define SEARCH_PILL_TIPS_WIDTH 50

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
	{


		if (m_qp_pillPmo)
		{
			m_qp_pillPmo->paint(painter,m_geom.topLeft());
		}
		if (m_qp_iconPmo)
		{
			m_qp_iconPmo->paint(painter,m_iconPos);
		}
		painter->drawPixmap(m_textPos, m_textPix);
	}

Q_SIGNALS:
	void signalTapped();

private:

	// TODO: not the best determinate of interaction but will do for now
	bool canInteract() { return opacity() == 1.0; }

	bool m_noResize;
	QPointer<PixmapObject> m_qp_pillPmo;
	QPointer<PixmapObject> m_qp_iconPmo;
	QPixmap m_textPix;
	QRectF m_geom;
	QPointF m_iconPos;		//in painting coords (top left of pixmap target box)
	QPointF m_textPos;		// ditto
	QRectF m_boundingRect;
};

#endif
