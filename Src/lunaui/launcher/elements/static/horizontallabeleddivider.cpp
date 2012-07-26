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




#include "horizontallabeleddivider.h"
#include "staticelementsettings.h"
#include "dimensionsglobal.h"
#include "debugglobal.h"
#include "renderopts.h"
#include <QPainter>
#include <QPen>
#include <QDebug>

#include "Settings.h"	//for the font spec

QFont HorizontalLabeledDivider::s_dividerLabelFont = QFont();

QFont HorizontalLabeledDivider::staticLabelFontForHDiv()
{
	//TODO: specify a proper font
	static bool fontInitalized = false;
	if (!fontInitalized)
	{
		s_dividerLabelFont = QFont(QString::fromStdString(Settings::LunaSettings()->fontQuicklaunch));
		s_dividerLabelFont.setPixelSize(qBound(2,(int)(StaticElementSettings::settings()->horizLabeledDivider_labelFontSizePx),100));
		s_dividerLabelFont.setBold(StaticElementSettings::settings()->horizLabeledDivider_labelFontEmbolden);
	}
	return s_dividerLabelFont;
}

//static
HorizontalLabeledDivider * HorizontalLabeledDivider::NewHorizontalLabeledDivider(const QString& label,quint32 width,PixmapObject * p_dividerPixmapObject)
{
	//get the font size for the height
	//TODO: possible absoluteHeight override here (plumb it out to staticelementsettings)
	quint32 fontHeight = DimensionsGlobal::Even(
			qBound(2,(int)(StaticElementSettings::settings()->horizLabeledDivider_labelFontSizePx),100) + 2);

	//TODO: DEBUG: inline; broken out for debug examine
	QRectF geom = DimensionsGlobal::realRectAroundRealPoint(QSize(width,fontHeight));
	return new HorizontalLabeledDivider(geom,label,p_dividerPixmapObject);
}

HorizontalLabeledDivider::HorizontalLabeledDivider(const QRectF& geom,const QString& label,PixmapObject * p_dividerPixmapObject)
: HorizontalDivider(geom)
, m_qp_divPixmap(p_dividerPixmapObject)
, m_labelString(label)
{
	if (label.isEmpty())
	{
		//Not allowing empty labels...replace it with (?)
		m_labelString = QString("(?)");
	}
	m_labelFontColor = StaticElementSettings::settings()->horizLabeledDivider_labelFontColor;
	m_alignmentGeomPrecomputed = DimensionsGlobal::realRectAroundRealPoint(QSize((qint32)geom.width(),
										StaticElementSettings::settings()->horizLabeledDivider_alignmentHeightPx));
	//TODO: error check??
	m_labelPixmapSpacing = (qreal)StaticElementSettings::settings()->horizLabeledDivider_labelToPixmapSpacingPx;

	if (m_qp_divPixmap)
	{
		connect(m_qp_divPixmap,SIGNAL(signalObjectDestroyed()),
				this,SLOT(slotPixmapObjectInvalidated()));
	}
	redoLabelTextLayout();
	recalculateLabelPosition();
	recalculateDividerPixmapSizeAndPosition();
}

//virtual
HorizontalLabeledDivider::~HorizontalLabeledDivider()
{
}


//virtual
QRectF HorizontalLabeledDivider::geometry() const
{
	if (StaticElementSettings::settings()->horizLabeledDivider_useAlignmentGeom)
	{
		return m_alignmentGeomPrecomputed;
	}
	return ThingPaintable::geometry();
}

//virtual
QRectF HorizontalLabeledDivider::positionRelativeGeometry() const
{
	return geometry().translated(pos());

}

//virtual
bool HorizontalLabeledDivider::resize(const QSize& newSize)
{
	HorizontalDivider::resize(newSize);
	m_alignmentGeomPrecomputed = DimensionsGlobal::realRectAroundRealPoint(QSize((qint32)m_geom.width(),
			StaticElementSettings::settings()->horizLabeledDivider_alignmentHeightPx));

	redoLabelTextLayout();
	recalculateLabelPosition();
	recalculateDividerPixmapSizeAndPosition();
	update();
	return true;
}

//virtual
bool HorizontalLabeledDivider::resize(quint32 newWidth,quint32 newHeight)
{
	return resize(qMin((quint32)20,newWidth),qMin((quint32)2,newHeight));
}

//virtual
void HorizontalLabeledDivider::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	//TODO: PIXEL-ALIGN
	QPen savePen = painter->pen();
	painter->setPen(m_labelFontColor);
	m_textLayoutObject.draw(painter,m_labelGeom.translated(m_labelPosICS).topLeft());
	painter->setPen(savePen);

	if (m_qp_divPixmap)
	{
		m_qp_divPixmap->paint(painter,m_pixmapGeom.translated(m_pixmapPosICS).topLeft());
	}
}

//virtual
void HorizontalLabeledDivider::paint(QPainter *painter, const QRectF& sourceItemRect)
{

	//Can't really partial-paint the label, unless clipping is used...and I don't want to use it at this point!
	// (way too many bad experiences and unexpected failures)
	// so if the label OR the pixmap fail to be inside the source rect, then they won't be painted at all
	//TODO: PIXEL-ALIGN

	if (sourceItemRect.contains(m_labelGeom.translated(m_labelPosICS))
			&& (sourceItemRect.contains(m_pixmapGeom.translated(m_pixmapPosICS))))
	{
		paint(painter);
	}
}

//virtual
void HorizontalLabeledDivider::paint(QPainter *painter, const QRectF& sourceItemRect,qint32 renderOpts)
{
	if (sourceItemRect.contains(m_labelGeom.translated(m_labelPosICS))
			&& (sourceItemRect.contains(m_pixmapGeom.translated(m_pixmapPosICS))))
	{
		if (renderOpts & LabeledDivRenderStage::Label)
		{
			QPen savePen = painter->pen();
			painter->setPen(m_labelFontColor);
			m_textLayoutObject.draw(painter,m_labelGeom.translated(m_labelPosICS).topLeft());
			painter->setPen(savePen);
		}

		if (renderOpts & LabeledDivRenderStage::DivPix)
		{
			if (m_qp_divPixmap)
			{
				m_qp_divPixmap->paint(painter,m_pixmapGeom.translated(m_pixmapPosICS).topLeft());
			}
		}
	}
}

//virtual
void HorizontalLabeledDivider::paintOffscreen(QPainter *painter)
{
	//TODO: IMPLEMENT
}

//static
QRectF HorizontalLabeledDivider::AGEOM(const QRectF& geom)
{
	if (!(StaticElementSettings::settings()->horizLabeledDivider_useAlignmentGeom))
	{
		return geom;
	}
	return DimensionsGlobal::realRectAroundRealPoint(QSize((qint32)geom.width(),
								StaticElementSettings::settings()->horizLabeledDivider_alignmentHeightPx));
}

///protected:

//virtual
void HorizontalLabeledDivider::redoLabelTextLayout()
{
	m_textLayoutObject.clearLayout();
	m_textLayoutObject.setText(m_labelString);
	m_textLayoutObject.setFont(staticLabelFontForHDiv());
	m_labelFontColor = StaticElementSettings::settings()->horizLabeledDivider_labelFontColor;
	QFontMetrics fm(staticLabelFontForHDiv());
	int leading = fm.leading();
	int rise = fm.ascent();
	qreal height = 0;
	m_textLayoutObject.beginLayout();
	while (height < m_geom.height()) {
		QTextLine line = m_textLayoutObject.createLine();
		if (!line.isValid())
			break;
		line.setLineWidth(m_geom.width());
		line.setPosition(QPointF(0, height));
		height += line.height();
	}
	height = qMin((quint32)DimensionsGlobal::roundUp(height),(quint32)m_geom.height());
	height = DimensionsGlobal::Even(DimensionsGlobal::roundDown(height));	//force to an even #
	m_textLayoutObject.endLayout();
	m_labelGeom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(m_textLayoutObject.minimumWidth(),height)).toAlignedRect();
}

//virtual
void HorizontalLabeledDivider::recalculateLabelPosition()
{
	//left-align m_labelGeom against m_geom, and center it vertically in m_geom
	m_labelPosICS = QPointF(m_geom.left()-m_labelGeom.left(),0.0).toPoint();
}

//virtual
void HorizontalLabeledDivider::recalculateDividerPixmapSizeAndPosition()
{
	if (!m_qp_divPixmap)
	{
		m_pixmapGeom = QRect();
		m_pixmapPosICS = QPoint();
		return;
	}

	//TODO: PIXEL-ALIGN
	QSize pixmapGeomSize = QSizeF(m_geom.width()-m_labelGeom.width()-m_labelPixmapSpacing,
			qMin((qreal)m_qp_divPixmap->nativeHeight(),m_geom.height())).toSize();
	m_pixmapGeom = DimensionsGlobal::realRectAroundRealPoint(pixmapGeomSize).toRect();
	m_pixmapPosICS = QPointF(m_labelGeom.translated(m_labelPosICS).right()+m_labelPixmapSpacing-m_pixmapGeom.left(),0.0).toPoint();
	m_qp_divPixmap->resize(pixmapGeomSize);
}

//virtual
void HorizontalLabeledDivider::slotPixmapObjectInvalidated()
{
	//prevent all further paint()-s
	setFlag(ItemHasNoContents,true);
}

