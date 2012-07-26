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




#ifndef LABELEDBUTTON_H_
#define LABELEDBUTTON_H_

#include "thingpaintable.h"
#include <QRectF>
#include <QRect>
#include <QString>
#include <QColor>
#include <QPointF>
#include <QFont>
#include <QTextLayout>

class PixmapObject;
class LabeledButton : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:
	LabeledButton(const QRectF& buttonGeometry);
	virtual ~LabeledButton();

	virtual bool resize(const QSize& newSize);
	virtual bool resize(quint32 newWidth,quint32 newHeight);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0) = 0;
	virtual void paintOffscreen(QPainter * painter) = 0;

	virtual void setLabel(const QString& v);

protected:

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

	static QFont			s_standardButtonFont;
	QString					m_label;
	QFont					m_textFont;
	QColor 					m_selectedColor;
	QColor					m_unselectedColor;
	QTextLayout				m_textLayoutObject;
	QPointF					m_labelPosICS;				// position in ICS, and corresponds to m_labelGeom
	QPoint					m_labelPosPntCS;			// same, but precomputed to offscreen painter coords relative to 0,0 at top left
	QRect					m_labelMaxGeom;			// precomputed by recalculateLabelBoundsForCurrentGeom()
	QRect					m_labelGeom;			//precomputed by redoLabelTextLayout(); this one is the CURRENT label's box (always <= m_labelMaxGeom)
};

#endif /* LABELEDBUTTON_H_ */
