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




#ifndef TEXTBOX_H_
#define TEXTBOX_H_

#include "thingpaintable.h"
#include <QString>
#include <QPointer>
#include <QFont>
#include <QTextLayout>

class PixmapObject;

namespace TextBoxHorizontalAlignment
{
	enum Enum
	{
		INVALID,
		Left,
		Right,
		Center
	};
}

namespace TextBoxVerticalAlignment
{
	enum Enum
	{
		INVALID,
		Top,
		Bottom,
		Center
	};
}

class TextBox : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	Q_PROPERTY(QString innertext READ innerText WRITE setInnerText RESET resetInnerText NOTIFY signalInnerTextChanged)

public:

	static const char * InnerTextPropertyName;		//keep in sync with Q_PROPERTY

	TextBox(PixmapObject * p_backgroundPmo,const QRectF& geom);
	TextBox(PixmapObject * p_backgroundPmo,const QRectF& geom,const QString& inner_text);
	virtual ~TextBox();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter *painter);

	virtual QString innerText() const;
	virtual void setInnerText(const QString& v);
	virtual void resetInnerText();

	//TODO: temporary; remove when a better system (like styles) goes in place
	static QFont staticLabelFontForTextBox();

Q_SIGNALS:

    void signalInnerTextChanged(const QString&);

protected:

	//PREREQUISITE: m_geom must be valid, at least its size()
	//RESULT: m_innerTextMaxGeom will be valid
	virtual void recalculateMaxTextGeomForCurrentGeom();

	//	PREREQUISITE: m_innerTextMaxGeom must be valid, at least its size()
	//  RESULT: m_textLayoutObject will have the correct layout, m_innerTextGeom will be set correctly for the current inner text,
	virtual void	redoInnerTextLayout();		//run this after the geometry changes. This is absolutely necessary
												// ALSO run this after the inner text changes. don't ask, just do it. Not necessary now, but it most likely will be later

	//	PREREQUISITE: m_geom , m_innerTextMaxGeom and m_innerTextGeom are set
	//	RESULT:	m_innerTextPosICS and m_innerTextPosPntCS are set
	virtual void	recalculateInnerTextPosition();

protected:

	QString m_innerText;
	QPointer<PixmapObject> m_qp_background;

	//text layout for the label
	static QFont s_textFont;
	QPointF					m_innerTextPosICS;				// position in ICS, and corresponds to m_labelGeom
	QPoint					m_innerTextPosPntCS;			// same, but precomputed to offscreen painter coords relative to 0,0 at top left
	QRect					m_innerTextMaxGeom;			// precomputed by recalculateLabelBoundsForCurrentGeom()

	QRect					m_innerTextGeom;			//precomputed by redoLabelTextLayout(); this one is the CURRENT text box (always <= m_innerTextMaxGeom)
	QTextLayout				m_textLayoutObject;
	QPointer<PixmapObject>	m_qp_prerenderedInnerTextPixmap;

};

#endif /* TEXTBOX_H_ */
