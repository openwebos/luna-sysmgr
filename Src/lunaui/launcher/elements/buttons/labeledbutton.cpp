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
#include "labeledbutton.h"
#include "layoutsettings.h"

#include <QPainter>
#include <QString>
#include <QDebug>
#include <QTextOption>

#include "Settings.h"

QFont LabeledButton::s_standardButtonFont = QFont();

QFont LabeledButton::staticLabelFontForButtons()
{
	//TODO: specify a proper font
	static bool fontInitialized = false;
	if (!fontInitialized)
	{
		s_standardButtonFont = QFont(QString::fromStdString(Settings::LunaSettings()->fontQuicklaunch));
		quint32 fontSize = qBound((quint32)2,LayoutSettings::settings()->tabBarTabFontSizePx,(quint32)100);
		s_standardButtonFont.setPixelSize(fontSize);
		s_standardButtonFont.setBold(LayoutSettings::settings()->tabBarTabFontEmbolden);
	}
	return s_standardButtonFont;
}

LabeledButton::LabeledButton(const QRectF& buttonGeometry)
: ThingPaintable(buttonGeometry)
{
}

//virtual
LabeledButton::~LabeledButton()
{

}

//virtual
bool LabeledButton::resize(const QSize& newSize)
{
	ThingPaintable::resize(newSize);
	recalculateLabelBoundsForCurrentGeom();
	redoLabelTextLayout();
	recalculateLabelPosition();
	return true;
}

//virtual
bool LabeledButton::resize(quint32 newWidth,quint32 newHeight)
{
	return resize(QSize(newWidth,newHeight));
}

//virtual
void LabeledButton::setLabel(const QString& v)
{
	m_textFont = LabeledButton::staticLabelFontForButtons();
	m_selectedColor = LayoutSettings::settings()->tabBarSelectedTabFontColor;
	m_unselectedColor = LayoutSettings::settings()->tabBarUnSelectedTabFontColor;
	recalculateLabelBoundsForCurrentGeom();
	redoLabelTextLayout();
	recalculateLabelPosition();
	update();
}

//virtual
void LabeledButton::recalculateLabelBoundsForCurrentGeom()
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
void	LabeledButton::redoLabelTextLayout()
{
	//TODO: somewhat wasteful. If there is no label, should just exit early and leave a layout that will be left unrendered by paint()
	m_textLayoutObject.clearLayout();
	m_textLayoutObject.setText(m_label);
//	int fontSize = qBound(4,(int)((qreal)(m_labelMaxGeom.height())*0.5),24) -2;
//	fontSize = fontSize - (fontSize % 2);
//	m_textFont.setPixelSize(fontSize);
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
void	LabeledButton::recalculateLabelPosition()
{
	//TODO: MYSTERY! don't know why it needs the 2.0 bump in Y. investigate the geom creation in redoLabelTextLayout()
	m_labelPosICS = QPointF(0,2.0);
	m_labelPosPntCS = (m_labelGeom.topLeft() + m_labelPosICS).toPoint();
}
