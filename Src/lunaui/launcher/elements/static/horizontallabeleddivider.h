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




#ifndef HORIZONTALLABELEDDIVIDER_H_
#define HORIZONTALLABELEDDIVIDER_H_


#include "horizontaldivider.h"

#include <QFont>
#include <QTextLayout>

class HorizontalLabeledDivider : public HorizontalDivider
{
	Q_OBJECT

public:

	static HorizontalLabeledDivider * NewHorizontalLabeledDivider(const QString& label,quint32 width,PixmapObject * p_dividerPixmapObject);
	virtual ~HorizontalLabeledDivider();

	// override the geometry logic from the base class...it must be done in order to make use of "alignment" geometry, if it is used
	//	see the icongeometrysettings.h/cpp files
	// essentially, it's a way to trick the users of this class into thinking the geometry is actually different (intended only to be < than the actual geom)
	virtual QRectF geometry() const;
	virtual QRectF positionRelativeGeometry() const;

	virtual bool resize(const QSize& newSize);
	virtual bool resize(quint32 newWidth,quint32 newHeight);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paint(QPainter *painter, const QRectF& sourceRect);
	virtual void paint(QPainter *painter, const QRectF& sourceRect,qint32 renderOpts);
	virtual void paintOffscreen(QPainter *painter);

	//TODO: temporary; remove when a better system (like styles) goes in place
	static QFont staticLabelFontForHDiv();

	static QRectF AGEOM(const QRectF& geom);	//alignment geom, if used

protected Q_SLOTS:

	virtual void slotPixmapObjectInvalidated();


protected:

	HorizontalLabeledDivider();
	HorizontalLabeledDivider(const QRectF& geom,const QString& label,PixmapObject * p_dividerPixmapObject);

	//	PREREQUISITE: m_geom must be set correctly
	//  RESULT: m_textLayoutObject will have the correct layout, m_labelGeom will be set correctly for the current label,
	virtual void	redoLabelTextLayout();		//run this after the geometry changes. This is absolutely necessary
	// ALSO run this after the label changes. don't ask, just do it. Not necessary now, but it most likely will be later

	//	PREREQUISITE: m_geom and m_labelGeom are set
	//	RESULT:	m_labelPosICS is set
	virtual void	recalculateLabelPosition();

	// PREREQUISITE: m_geom, m_labelGeom, m_labelPosICS, m_labelPixmapSpacing must be set correctly
	// RESULT:	m_pixmapGeom and m_pixmapPosICS will be set
	virtual void	recalculateDividerPixmapSizeAndPosition();

	QRectF					m_alignmentGeomPrecomputed;		//populated only if StaticElementSettings::horizLabeledDivider_useAlignmentGeom is set true

	QPointer<PixmapObject> 	m_qp_divPixmap;				// the graphic part of the divider
	QPoint				m_pixmapPosICS;				//the position of the div pixmap (see base class) in ICS
	QRect					m_pixmapGeom;				// using QRect so that it's easier on paint() (it doesn't have to pixalign constantly)

	qreal					m_labelPixmapSpacing;		//between the label and pixmap

	//text layout for the label
	QString 				m_labelString;
	static QFont 			s_dividerLabelFont;
	QColor					m_labelFontColor;

	QPoint					m_labelPosICS;				// position in ICS, and corresponds to m_labelGeom
	QRect					m_labelGeom;

	QTextLayout				m_textLayoutObject;

};

#endif /* HORIZONTALLABELEDDIVIDER_H_ */
