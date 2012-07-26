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




#include "textbox.h"
#include "dimensionsglobal.h"
#include "debugglobal.h"
#include "pixmapobject.h"
#include "iconlayoutsettings.h"

#include <QPainter>
#include <QFont>
#include <QString>
#include <QDebug>

#include "Settings.h"

const char * TextBox::InnerTextPropertyName = "innertext";
QFont TextBox::s_textFont = QFont();

QFont TextBox::staticLabelFontForTextBox()
{
	//TODO: specify a proper font
	static bool fontInitalized = false;
	if (!fontInitalized)
	{
		//TODO: TEMP: don't hiijack
		s_textFont = QFont(QString::fromStdString(Settings::LunaSettings()->fontQuicklaunch));
	}
	return s_textFont;
}

TextBox::TextBox(PixmapObject * p_backgroundPmo,const QRectF& geom)
: ThingPaintable(geom)
, m_qp_background(p_backgroundPmo)
, m_qp_prerenderedInnerTextPixmap(0)
{
}

TextBox::TextBox(PixmapObject * p_backgroundPmo,const QRectF& geom,const QString& inner_text)
: ThingPaintable(geom)
, m_qp_background(p_backgroundPmo)
, m_qp_prerenderedInnerTextPixmap(0)
{
	setInnerText(inner_text);
}

//virtual
TextBox::~TextBox()
{

}

//virtual
void TextBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	//DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,4);
	QPen savePen = painter->pen();
	painter->setPen(IconLayoutSettings::settings()->reorderablelayout_emptyPageTextFontColor);
	m_textLayoutObject.draw(painter,m_innerTextPosICS+m_innerTextGeom.topLeft());
	painter->setPen(savePen);
}

//virtual
void TextBox::paintOffscreen(QPainter *painter)
{

}

//virtual
QString TextBox::innerText() const
{
	return m_innerText;
}

//virtual
void TextBox::setInnerText(const QString& v)
{
	m_innerText = v;
	recalculateMaxTextGeomForCurrentGeom();
	redoInnerTextLayout();
	recalculateInnerTextPosition();
	update();
}

//virtual
void TextBox::resetInnerText()
{
	//TODO: LOCALIZE:
	m_innerText = QString();
	m_textLayoutObject.clearLayout();
	update();
}

///protected:

//virtual
void TextBox::recalculateMaxTextGeomForCurrentGeom()
{
	qint32 width = DimensionsGlobal::roundDown(m_geom.width());
	qint32 height = DimensionsGlobal::roundDown(m_geom.height());
	width = qMax(2,width - 2 - (width % 2));
	height = qMax(2,height - 2 - (height % 2));
	m_innerTextMaxGeom = DimensionsGlobal::realRectAroundRealPoint(QSize(width,height)).toRect();

}

//virtual
void	TextBox::redoInnerTextLayout()
{
	//TODO: somewhat wasteful. If there is no label, should just exit early and leave a layout that will be left unrendered by paint()
	m_textLayoutObject.clearLayout();
	m_textLayoutObject.setText(m_innerText);
	QFont f = staticLabelFontForTextBox();
	f.setBold(IconLayoutSettings::settings()->reorderablelayout_emptyPageTextFontEmbolden);
	f.setPixelSize(qMax((quint32)4,IconLayoutSettings::settings()->reorderablelayout_emptyPageTextFontSizePx));
	m_textLayoutObject.setFont(f);
	QTextOption textOpts;
	textOpts.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	textOpts.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	m_textLayoutObject.setTextOption(textOpts);
	QFontMetrics fm(staticLabelFontForTextBox());
	int leading = fm.leading();
	int rise = fm.ascent();
	qreal height = 0;
	m_textLayoutObject.beginLayout();
	while (height < m_innerTextMaxGeom.height()) {
		QTextLine line = m_textLayoutObject.createLine();
		if (!line.isValid())
			break;
		line.setLineWidth(m_innerTextMaxGeom.width());
		if (m_textLayoutObject.lineCount() > 1)
		{
		//	height += leading;
		}
		line.setPosition(QPointF(0, height));
		height += line.height();
	}
	height = qMin((quint32)DimensionsGlobal::roundUp(height),(quint32)m_innerTextMaxGeom.height());
	height = DimensionsGlobal::roundDown(height) - (DimensionsGlobal::roundDown(height) % 2);	//force to an even #
	m_textLayoutObject.endLayout();
	//TODO: PIXEL-ALIGN
	m_innerTextGeom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(m_textLayoutObject.boundingRect().width(),height)).toAlignedRect();
}

//virtual
void	TextBox::recalculateInnerTextPosition()
{
	m_innerTextPosICS = QPointF(m_geom.center().x(),m_geom.top() - (qreal)m_innerTextGeom.top());
	QPointF offset = m_innerTextPosICS + m_innerTextGeom.topLeft() - m_geom.topLeft();
	offset += QPointF(DimensionsGlobal::centerRectInRectPixelAlign(m_innerTextMaxGeom,m_innerTextGeom));
	m_innerTextPosPntCS = DimensionsGlobal::realPointAsPixelPosition(offset);
}

