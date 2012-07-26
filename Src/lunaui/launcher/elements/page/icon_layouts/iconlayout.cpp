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




#include "iconlayout.h"
#include "icon.h"
#include "dimensionsglobal.h"
#include "debugglobal.h"
#include "page.h"
#include "pixmaphugeobject.h"
#include <QPainter>
#include <QTransform>


quint64 IconOperation::s_idcounter = 1;			//0 == invalid

////////////////////////// IconOperation //////////////////////////////////////

QString IconOperation::Names[] =
{ QString("INVALID") , QString("Id") , QString("Add") , QString("Remove") , QString("Reorder") };

IconOperation::IconOperation()
: m_valid(false)
, m_opname(IconOperation::INVALID)
, m_originRow(0)
, m_originColumn(0)
, m_destinationRow(0)
, m_destinationColumn(0)
, m_sequenceNum(0)
{
}

IconOperation::IconOperation(const Name op,const quint32 origin_column,const quint32 origin_row,
		const quint32 destination_column,const quint32 destination_row)
: m_valid(true)
, m_opname(op)
, m_originRow(origin_row)
, m_originColumn(origin_column)
, m_destinationRow(destination_row)
, m_destinationColumn(destination_column)
, m_sequenceNum(0)
{
	if (op == IconOperation::Id)
	{
		m_sequenceNum = s_idcounter++;
	}
}

IconOperation::IconOperation(const Name op,const quint32 origin_column,const quint32 origin_row,
		const quint32 destination_column,const quint32 destination_row,
		const quint32 seq)
: m_valid(true)
, m_opname(op)
, m_originRow(origin_row)
, m_originColumn(origin_column)
, m_destinationRow(destination_row)
, m_destinationColumn(destination_column)
, m_sequenceNum(seq)
{
}

QDataStream & operator<< (QDataStream& stream, const IconOperation& s)
{
	stream << "\"IconOperation\":[ "
			<< s.m_valid
			<< " , " << IconOperation::Names[s.m_opname]
			                                 << " , " << s.m_originColumn
			                                 << " , " << s.m_originRow
			                                 << " , " << s.m_destinationColumn
			                                 << " , " << s.m_destinationRow
			                                 << " , " << s.m_sequenceNum
			                                 << " ]";
	return stream;
}

QDebug operator<<(QDebug dbg, const IconOperation &s)
{
	dbg.nospace() << "\"IconOperation\":[ "
			<< s.m_valid
			<< " , " << IconOperation::Names[s.m_opname]
			                                 << " , " << s.m_originColumn
			                                 << " , " << s.m_originRow
			                                 << " , " << s.m_destinationColumn
			                                 << " , " << s.m_destinationRow
			                                 << " , " << s.m_sequenceNum
			                                 << " ]";
	return dbg.space();
}

QDataStream & operator>> (QDataStream& stream, IconOperation& s)
{
	//TODO: IMPLEMENT
	return stream;
}



///////////////////////////////////// IconCell ///////////////////////////////////////////////

IconCell::IconCell(IconRow * p_owner)
: m_qp_icon(0)
, m_qp_pendingReorderIcon(0)
, m_p_layoutRowObject(p_owner)
{
}

IconCell::IconCell(IconRow * p_owner,IconBase * p_icon)
: m_qp_icon(p_icon)
, m_qp_pendingReorderIcon(0)
, m_p_layoutRowObject(p_owner)
{
}

//convenience...size rect around 0,0
IconCell::IconCell(IconRow * p_owner,IconBase * p_icon,const QSizeF& size)
: m_qp_icon(p_icon)
, m_qp_pendingReorderIcon(0)
, m_p_layoutRowObject(p_owner)
{
	m_geom = DimensionsGlobal::realRectAroundRealPoint(size);
	m_pos = QPointF(0,0);
}

IconCell::IconCell(IconRow * p_owner,IconBase * p_icon,const QSize& size)
: m_qp_icon(p_icon)
, m_qp_pendingReorderIcon(0)
, m_p_layoutRowObject(p_owner)
{
	m_geom = DimensionsGlobal::realRectAroundRealPoint(size);
	m_pos = QPointF(0,0);
}

//virtual
void IconCell::clear()
{
	m_p_layoutRowObject = 0;
	m_pos = QPointF();
	m_lastPaintedPixmapTargetRect = QRect();
}

//virtual
QRectF IconCell::geometry()
{
	return m_geom;
}

//virtual
QRectF IconCell::relativeGeometry()
{
	return m_geom.translated(m_pos);
}

//virtual
QRectF IconCell::untranslateGeometry(const QRectF& geom)
{
	//no checks on what someone passed in as geom.
	return geom.translated(-m_pos);
}

//virtual
QPointF IconCell::position()
{
	return m_pos;
}

//virtual
IconBase * IconCell::setNewIcon(IconBase * p_newIcon,bool repositionIconToCell)
{
	if (!p_newIcon)
	{
		return 0;	//nothing to do
	}
	IconBase * p = m_qp_icon;
	m_qp_icon = p_newIcon;

	//making it a little more safe since this could conceivably be called when the cell is "orphaned"/un-owned,
	// during relayout
	if ((repositionIconToCell) && (m_p_layoutRowObject))
	{
		if (m_p_layoutRowObject->m_p_layoutObject)
		{
			p_newIcon->setPos(m_p_layoutRowObject->m_p_layoutObject->pageCoordinateFromLayoutCoordinate(m_pos));
		}
	}
	//TODO: TEMP: RE-ENABLE asap! temporarily disabled for REORDER work
//	if ((p_newIcon->geometry().size().toSize() != p->geometry().size().toSize())
//			&& (m_p_layoutRowObject))
//	{
//		m_p_layoutRowObject->cellGeomChangedDueToNewIcon(this);
//	}
	return p;
}

//virtual
void IconCell::resetIconPosition()
{
	if (m_qp_icon)
	{
		m_qp_icon->setPos(m_p_layoutRowObject->m_p_layoutObject->pageCoordinateFromLayoutCoordinate(m_pos));
	}
}

//virtual
void IconCell::commitPendingIcon()
{
	if (m_qp_pendingReorderIcon)
	{
		m_qp_pendingReorderIcon->slotDisableIconAutoRepaint();
		m_qp_pendingReorderIcon->setZValue(0.0);
		(void)setNewIcon(m_qp_pendingReorderIcon,true);
	}
	m_qp_pendingReorderIcon = 0;	//clear so that there isn't any confusion next reorder cycle
}

//remember, reposition() functions have nothing to do with geom; they don't change m_geom
QPointF IconCell::reposition(const QPointF& newCenterPointICS)
{
	QPointF v = m_pos;
	m_pos = newCenterPointICS;
	if (m_qp_icon)
	{
		m_qp_icon->setPos(m_p_layoutRowObject->m_p_layoutObject->pageCoordinateFromLayoutCoordinate(m_pos));
	}
	return v;
}

qreal IconCell::repositionX(const qreal x)
{
	qreal v = m_pos.x();
	m_pos.setX(x);
	if (m_qp_icon)
	{
		m_qp_icon->setPos(m_p_layoutRowObject->m_p_layoutObject->pageCoordinateFromLayoutCoordinate(m_pos));
	}
	return v;
}

qreal IconCell::repositionY(const qreal y)
{
	qreal v = m_pos.y();
	m_pos.setY(y);
	if (m_qp_icon)
	{
		m_qp_icon->setPos(m_p_layoutRowObject->m_p_layoutObject->pageCoordinateFromLayoutCoordinate(m_pos));
	}
	return v;
}

void IconCell::moveBy(const QPointF& d)
{
	m_pos += d;
	if (m_qp_icon)
	{
		m_qp_icon->setPos(m_p_layoutRowObject->m_p_layoutObject->pageCoordinateFromLayoutCoordinate(m_pos));
	}
}

void IconCell::moveBy(const qreal dx,const qreal dy)
{
	m_pos += QPointF(dx,dy);
	if (m_qp_icon)
	{
		m_qp_icon->setPos(m_p_layoutRowObject->m_p_layoutObject->pageCoordinateFromLayoutCoordinate(m_pos));
	}
}

//virtual
void IconCell::paint(QPainter * painter)
{
	//TODO: DEBUG: moved outside of 'if' for debug...move back in for efficiency
	QTransform saveTran = painter->transform();
	painter->translate(m_pos);
	//paint the icon pixmap at the icon's current position
	if (m_qp_icon)
	{
//		//qDebug() << "iconCell translating to " << m_pos;
		m_qp_icon->paint(painter);
//		DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,2);
	}
//	else
//	{
//		DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,2);
//	}
	painter->setTransform(saveTran);
}

//virtual
void IconCell::paint(QPainter * painter, const QRectF& sourceRect)
{
	/*
	 * notes: sourceRect is in ICS of the layout. The compatible iconcell space is relativeGeometry()
	 *
	 */

	//paint the icon pixmap at the icon's current position,
	// and only if its within the sourceRect

	//untranslate() is done because the icon itself should be detached from cell position as much as possible
	// even though they are closely related and usually correlated
	//	This way though, the icon's paint (IconBase::paint() for example) can just deal with ICS coordinates
	// (its own ICS...from -w/2,-h/2 blahblah... where w = width of the icon object itself; all totally contained
	//	inside the icon class (like IconBase)

	QRectF sourceGeomIntersectArea = untranslateGeometry(relativeGeometry().intersect(sourceRect));

	if (sourceGeomIntersectArea.isEmpty())
	{
		//			//qDebug() << "iconCell at geom (relgeom) " << m_geom << "( " << relativeGeometry() << " )"
		//					<< " has an empty intersect with sourceRect " << sourceRect;
		return;
	}
	QTransform saveTran = painter->transform();
	painter->translate(m_pos-sourceRect.topLeft());
	if (m_qp_icon)
	{
		//TODO: DEBUG: broken-out for debug. rewrite more eff/cleanly
		//TODO: PIXEL-ALIGN
		m_qp_icon->paint(painter,sourceGeomIntersectArea);
	}
//	else
//	{
//		DimensionsDebugGlobal::dbgPaintBoundingRect(painter,this->geometry(),7);
//	}
	painter->setTransform(saveTran);
}

//virtual
void IconCell::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate)
{

}

//virtual
void IconCell::paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt)
{
	/*
	 * notes: sourceRect is in ICS of the layout. The compatible iconcell space is relativeGeometry()
	 *
	 */

	//paint the icon pixmap at the icon's current position,
	// and only if its within the sourceRect

	//untranslate() is done because the icon itself should be detached from cell position as much as possible
	// even though they are closely related and usually correlated
	//	This way though, the icon's paint (IconBase::paint() for example) can just deal with ICS coordinates
	// (its own ICS...from -w/2,-h/2 blahblah... where w = width of the icon object itself; all totally contained
	//	inside the icon class (like IconBase)

	QRectF sourceGeomIntersectArea = untranslateGeometry(relativeGeometry().intersect(sourceRect));

	if (sourceGeomIntersectArea.isEmpty())
	{
		return;
	}
	QTransform saveTran = painter->transform();
	painter->translate(m_pos-sourceRect.topLeft());
	if (m_qp_icon)
	{
		//TODO: DEBUG: broken-out for debug. rewrite more eff/cleanly
		//TODO: PIXEL-ALIGN
		m_qp_icon->paint(painter,sourceGeomIntersectArea,renderOpt);
	}
	painter->setTransform(saveTran);
}

//virtual
void IconCell::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt)
{

}

//virtual
void IconCell::paintOffscreen(QPainter * painter)
{
	if (m_qp_icon)
	{
		QTransform saveTran = painter->transform();
		QPointF startPainterPoint = m_pos + m_geom.topLeft();
		painter->translate(startPainterPoint);
		m_qp_icon->paintOffscreen(painter);
		//TODO: PIXEL-ALIGN
		m_lastPaintedPixmapTargetRect = painter->transform().mapRect(QRectF(startPainterPoint,m_geom.size())).toRect();
		painter->setTransform(saveTran);
	}
}

//virtual
void IconCell::paintOffscreen(PixmapObject * p_pmo)
{
	//warning: won't check for p_pmo actually being a subclass of PixmapObject (e.g. ..HugeObject)
	// be responsible in the caller
	QPainter painter(p_pmo->data());
	paintOffscreen(&painter);
}

//virtual
void IconCell::paintOffscreen(PixmapHugeObject * p_hugePmo)
{
	if (m_qp_icon)
	{
		//TODO: PIXEL-ALIGN
		//get the paint coordinate (fragment) data for this icon cell
		//...first the rect in absolute coordinate space (this cs stretches across the whole huge pmo)
		QPoint translationPoint = (m_pos + m_geom.topLeft()).toPoint();
		QRect cellRect = QRect(translationPoint,m_geom.size().toSize());
		//TODO: PIXEL-ALIGN
		m_lastPaintedPixmapTargetRect = cellRect;	//can't use the painter transform trick here since there are
													// individual painters for all the pixmaps in the huge...
													// ASSUMPTION: no scaling or translation on the painters' transforms (i.e. they're 1:1)
		QVector<PixmapHugeObject::FragmentedPaintCoordinate> coords = p_hugePmo->paintCoordinates(cellRect);
		//paint it!
		for (int i=0;i<coords.size();++i)
		{
			QPainter painter(p_hugePmo->pixAt(coords.at(i).pixmapIndex));
			//bring the source rect back into my coordinate space
			QRect localSourceRect = coords.at(i).sourceRect.translated(-translationPoint);
			m_qp_icon->paintOffscreen(&painter,localSourceRect,coords.at(i).targetRect.topLeft());
		}
	}
}

QDataStream & operator<< (QDataStream& stream, const IconCell& s)
{
	stream << "\"IconCell\":{ \"geom\": \"" << s.m_geom << "\" , \"pos\":\"" << s.m_pos << "\" }";
	return stream;
}

QDebug operator<<(QDebug dbg, const IconCell &s)
{
	dbg.nospace() << "\"IconCell\":{ \"geom\": \"" << s.m_geom << "\" , \"pos\":\"" << s.m_pos << "\" }";
	return dbg.space();
}

QDataStream & operator>> (QDataStream& stream, IconCell& s)
{
	//TODO: IMPLEMENT
	return stream;
}

//virtual
IconCell::~IconCell()
{
}

IconRow::IconRow(IconLayout * p_owner)
: m_p_layoutObject(p_owner)
, m_spaceToUpperAdjacentRow(0)
, m_spaceToLowerAdjacentRow(0)
{
}

//virtual
IconRow::~IconRow()
{
}

IconRow::IconRow(IconLayout * p_owner,const QRectF& geom)
: m_p_layoutObject(p_owner)
, m_spaceToUpperAdjacentRow(0)
, m_spaceToLowerAdjacentRow(0)
{
	m_geom = DimensionsGlobal::realRectAroundRealPoint(geom.size());
}

quint32 IconRow::numIcons() const
{
	return (quint32)m_iconList.size();
}

//virtual
QRectF IconRow::geometry()
{
	return m_geom;
}

//virtual
QRectF IconRow::relativeGeometry() const
{
	return m_geom.translated(m_pos);
}

//virtual
void IconRow::recomputeGeometry(bool adjustPosition)
{
	m_geom = QRectF();
	if (!m_iconList.empty())
	{
		for (int i=0;i<m_iconList.size();++i)
		{
			m_geom = m_geom.unite(m_iconList[i]->geometry().translated(m_iconList[i]->position()));
		}
		if (adjustPosition)
		{
			m_pos = m_geom.center();
		}
		//recenter it
		m_geom = DimensionsGlobal::realRectAroundRealPoint(m_geom.size());
	}
}

//virtual
void IconRow::repositionAboveRow(IconRow& refRow,const quint32 space)
{
	if (&refRow == this)
	{
		return;
	}
	m_pos = QPointF(refRow.m_pos.x(),
			refRow.relativeGeometry().top() - (qreal)space - m_geom.bottom());
	//bottom trick uses fact that m_geom is 0,0 origin in local row coords
}

//virtual
void IconRow::repositionBelowRow(IconRow& refRow,const quint32 space)
{
	if (&refRow == this)
	{
		return;
	}
	if (&refRow == this)
	{
		return;
	}
	m_pos = QPointF(refRow.m_pos.x(),
			refRow.relativeGeometry().bottom() + (qreal)space - m_geom.top());
	//top trick uses fact that m_geom is 0,0 origin in local row coords (so top will be neg.)
}

//virtual
void IconRow::repositionAboveRow(IconRow& refRow)
{
	repositionAboveRow(refRow,m_spaceToLowerAdjacentRow);
}

//virtual
void IconRow::repositionBelowRow(IconRow& refRow)
{
	repositionBelowRow(refRow,m_spaceToUpperAdjacentRow);
}

void IconRow::moveBy(const QPointF& d)
{
	for (int i=0;i<m_iconList.size();++i)
	{
		m_iconList[i]->moveBy(d);
	}
	recomputeGeometry();
}

void IconRow::moveBy(const qreal dx,const qreal dy)
{
	for (int i=0;i<m_iconList.size();++i)
	{
		m_iconList[i]->moveBy(dx,dy);
	}
	recomputeGeometry();
}

void IconRow::appendCell(IconCell * p_cell)
{
	m_iconList.append(p_cell);
	recomputeGeometry();
}

void IconRow::addCell(IconCell * p_cell,const quint32 index)
{
	if (index >= (quint32)m_iconList.size())
	{
		m_iconList.append(p_cell);
	}
	else
	{
		m_iconList.insert(index,p_cell);
	}
	recomputeGeometry();
}

//the alignToCellIndex is the index in the ORIGINAL list (before appending)
void IconRow::appendCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 alignToCellIndex)
{
	if (alignToCellIndex >= (quint32)m_iconList.size())
	{
		p_cell->m_pos.setY(m_iconList[0]->position().y());
	}
	else
	{
		p_cell->m_pos.setY(m_iconList[alignToCellIndex]->position().y());
	}
	m_iconList.append(p_cell);
	recomputeGeometry();
}

//the alignToCellIndex is the index in the ORIGINAL list (before adding)
void IconRow::addCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 index,const quint32 alignToCellIndex)
{
	if (alignToCellIndex >= (quint32)m_iconList.size())
	{
		p_cell->m_pos.setY(m_iconList[0]->position().y());
	}
	else
	{
		p_cell->m_pos.setY(m_iconList[alignToCellIndex]->position().y());
	}
	if (index >= (quint32)m_iconList.size())
	{
		m_iconList.append(p_cell);
	}
	else
	{
		m_iconList.insert(index,p_cell);
	}
	recomputeGeometry();
}

void IconRow::removeCell(const quint32 index)
{
	if (index >= (quint32)m_iconList.size())
		return;
	IconCell * p_cell = m_iconList.takeAt(index);
	delete p_cell;
	recomputeGeometry();
}

//virtual
void IconRow::paint(QPainter * painter)
{
	//paint all the icons
	for (IconCellListIter it = m_iconList.begin();
			it != m_iconList.end();++it)
	{
		(*it)->paint(painter);
	}
}

//virtual
void IconRow::paint(QPainter * painter, const QRectF& sourceRect)
{
	//paint all the icons that are in the sourceRect...
	// for now just punt forward to the icon itself and let it figure it out
	// TODO: SLOW: could potentially just avoid the whole row if its geom is already outside the source rect
	//		(since, assuming its geom is correct, it encompasses all icon geoms)
//	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_pos-sourceRect.topLeft(),this->m_geom,6);
	for (IconCellListIter it = m_iconList.begin();
			it != m_iconList.end();++it)
	{
		(*it)->paint(painter,sourceRect);
	}

}

//virtual
void IconRow::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate)
{

}

//virtual
void IconRow::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt)
{

}

//virtual
void IconRow::paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt)
{
//	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_pos-sourceRect.topLeft(),this->m_geom,6);
	for (IconCellListIter it = m_iconList.begin();
			it != m_iconList.end();++it)
	{
		(*it)->paint(painter,sourceRect,renderOpt);
	}
}

//virtual
void IconRow::paintOffscreen(QPainter * painter)
{
	//paint all the icons
	m_lastPaintedPixmapTargetRect = QRect();
	for (IconCellListIter it = m_iconList.begin();
			it != m_iconList.end();++it)
	{
		(*it)->paintOffscreen(painter);
		m_lastPaintedPixmapTargetRect |= (*it)->m_lastPaintedPixmapTargetRect;
	}
}

//virtual
void IconRow::paintOffscreen(PixmapObject * p_pmo)
{
	//warning: won't check for p_pmo actually being a subclass of PixmapObject (e.g. ..HugeObject)
	// be responsible in the caller
	QPainter painter(p_pmo->data());
	paintOffscreen(&painter);
}

//virtual
void IconRow::paintOffscreen(PixmapHugeObject * p_hugePmo)
{
	//do nothing. This one has to be specific to the subclass of iconrow, for speed
}

//virtual
void IconRow::relayout(bool force)
{
	//do nothing (meant to be overriden)
}

//virtual
void IconRow::relayout(const qreal leftEdgeX,const quint32 spacing,bool force)
{
	if (m_iconList.empty())
	{
		return;
	}

	//redistribute icons horizontally, but make sure that the left edge is flush with the layout
	redistributeIconsHorizontally(leftEdgeX,spacing);
	//set all the icons' centers to the same y as this row
	alignIconsVerticallyCentered(m_pos.y());

	// for relayout(), pass in true to recomputeGeometry() so that it will move this row's position according to the icons
	//	that were just relaid out
	recomputeGeometry(true);
}

//virtual
void IconRow::redistributeIconsHorizontally(const qreal leftEdgeX,const quint32 spacing)
{
	if (m_iconList.empty())
	{
		return;
	}

	qreal xAnchor = (qreal)leftEdgeX;
	IconCellListIter it = m_iconList.begin();
	xAnchor += (*it)->m_geom.right();
	(*it)->repositionX(xAnchor);
	++it;
	for (;it != m_iconList.end();++it)
	{
		//move to the next cell's coords..
		xAnchor += (*(it-1))->m_geom.width()+(qreal)spacing;
		(*it)->repositionX(xAnchor);
	}
}

//virtual
void IconRow::alignIconsVerticallyCentered(const qreal yPosition)
{
	if (m_iconList.empty())
	{
		return;
	}

	//set them all to the same y
	for (IconCellListIter it = m_iconList.begin();it != m_iconList.end();++it)
	{
		(*it)->repositionY(yPosition);
	}
}

//virtual
void IconRow::cellGeomChangedDueToNewIcon(IconCell * reportingCell)
{
	//TODO: IMPLEMENT / unimplemented
}

QDataStream & operator<< (QDataStream& stream, const IconRow& s)
{
	stream << " \"IconRow\": { "
				<< "\"geom\":\"" << s.m_geom << "\""
				<< " , \"pos\":\"" << s.m_pos << "\""
				 << " , \"icons\":[ ";
	for (IconRow::IconCellListConstIter it = s.m_iconList.constBegin();
			it != s.m_iconList.constEnd();++it)
	{
		if (it != s.m_iconList.constBegin())
		{
			stream << " , ";
		}
		stream << "{ " << *(*it) << " } ";
	}
	stream << " ] ";
	return stream;
}

QDebug operator<<(QDebug dbg, const IconRow &s)
{
	dbg.nospace() << " \"IconRow\": { "
			<< "\"geom\":\"" << s.m_geom << "\""
			<< " , \"pos\":\"" << s.m_pos << "\""
			 << " , \"icons\":[ ";
	for (IconRow::IconCellListConstIter it = s.m_iconList.constBegin();
			it != s.m_iconList.constEnd();++it)
	{
		if (it != s.m_iconList.constBegin())
		{
			dbg.nospace() << " , ";
		}
		dbg.nospace() << "{ " << *(*it) << " } ";
	}
	dbg.nospace() << " ] }";
	return dbg.space();
}

QDataStream & operator>> (QDataStream& stream, IconRow& s)
{
	//TODO: IMPLEMENT
	return stream;
}

/////////////////////////////////////// IconLayout //////////////////////////////////////////////////

IconLayout::IconLayout(Page * p_owner)
: m_qp_ownerPage(p_owner)
{
	//no sense in setting up transforms right now, since the layout doesn't have any geometry
}

//virtual
IconLayout::~IconLayout()
{
}

//virtual
void IconLayout::paint(QPainter * painter)
{
	//do nothing
}

//virtual
void IconLayout::paint(const QPointF& translate,QPainter * painter)
{
	//do nothing
}

//virtual
void IconLayout::paint(QPainter * painter, const QRectF& sourceRect)
{
	//do nothing
}

//virtual
void IconLayout::paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt)
{
	//do nothing
}

//virtual
void IconLayout::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate)
{

}

//virtual
void IconLayout::paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt)
{

}

//virtual
void IconLayout::paintOffscreen(QPainter * painter)
{
	//do nothing
}

//virtual
void IconLayout::paintOffscreen(PixmapObject * p_pmo)
{
	//do nothing
}

//virtual
void IconLayout::paintOffscreen(PixmapHugeObject * p_pmo)
{
	//do nothing
}

//virtual
void IconLayout::enableAutoPaint()
{
	//do nothing
}

//virtual
void IconLayout::disableAutoPaint()
{
	//do nothing
}

//virtual
QRectF IconLayout::geometry()
{
	return m_geom;
}

//virtual
QRectF IconLayout::relativeGeometry()
{
	return m_geom.translated(m_pos);
}

//virtual
void IconLayout::relayout(bool force)
{
	//do nothing
}

//virtual
void IconLayout::resizeWidth(const quint32 w)
{
	//do nothing
}

//virtual
void IconLayout::setPosition(const QPointF& pos)
{
	m_pos = pos;
}

//virtual
QPointF IconLayout::position()
{
	return m_pos;
}

//virtual
void IconLayout::setUniformCellSize(const QSize& size)
{
	//do nothing
}

//virtual
void IconLayout::iconCellReleaseIcon(const QPoint& cellCoordinate)
{
	//do nothing
}

//virtual
IconCell * IconLayout::iconCellAtLayoutCoordinate(const QPointF& coordinate)
{
	//do nothing
	return 0;
}

//virtual
IconCell * IconLayout::iconCellAtLayoutCoordinate(const QPointF& layoutCoordinate,QPoint& r_gridCoordinate)
{
	//do nothing
	return 0;
}

//virtual
QRectF IconLayout::rowArea(quint32 rowIndex) const
{
	return QRectF();
}

//virtual
qint32 IconLayout::rowAtLayoutCoordinate(const QPointF& layoutCoordinate,bool clipMinMax)
{
	return -1;
}

//virtual
QPointF IconLayout::sceneCoordinateFromLayoutCoordinate(const QPointF& layoutCoordinate)
{
	if (m_qp_ownerPage)
	{
		return m_qp_ownerPage->mapToScene(m_qp_ownerPage->pageCoordinateFromLayoutCoordinate(layoutCoordinate));
	}
	return layoutCoordinate;
}

//virtual
QPointF IconLayout::pageCoordinateFromLayoutCoordinate(const QPointF& layoutCoordinate)
{
	if (m_qp_ownerPage)
	{
		return m_qp_ownerPage->pageCoordinateFromLayoutCoordinate(layoutCoordinate);
	}
	return layoutCoordinate;
}

//virtual
QPointF IconLayout::rawPageCoordinateFromLayoutCoordinate(const QPointF& layoutCoordinate) const
{
	return m_layoutToPageTran.map(layoutCoordinate);
}
//virtual
QPointF IconLayout::rawLayoutCoordinateFromPageCoordinate(const QPointF& pageCoordinate) const
{
	return m_pageToLayoutTran.map(pageCoordinate);
}
//virtual
QRectF  IconLayout::rawPageRectFromLayoutRect(const QRectF& layoutRect) const
{
	return m_layoutToPageTran.mapRect(layoutRect);
}
//virtual
QRectF	IconLayout::rawLayoutRectFromPageRect(const QRectF& pageRect) const
{
	return m_pageToLayoutTran.mapRect(pageRect);
}

//virtual
void IconLayout::recomputePageLayoutTransforms()
{
	if (m_qp_ownerPage)
	{
		m_layoutToPageTran.translate(-m_pos.x(),-m_pos.y());
		qreal uw = (DimensionsGlobal::isZeroF(m_qp_ownerPage->geometry().width())
					? 1.0
					: m_geom.width() / m_qp_ownerPage->geometry().width()
					);
		qreal uh = (DimensionsGlobal::isZeroF(m_qp_ownerPage->geometry().height())
					? 1.0
					: m_geom.height() / m_qp_ownerPage->geometry().height()
					);
		m_layoutToPageTran.scale(uw,uh);

		m_pageToLayoutTran.translate(m_pos.x(),m_pos.y());
		uw = (DimensionsGlobal::isZeroF(m_geom.width())
					? 1.0
					: m_qp_ownerPage->geometry().width() / m_geom.width()
					);
		uh = (DimensionsGlobal::isZeroF(m_geom.height())
					? 1.0
					: m_qp_ownerPage->geometry().height() / m_geom.height()
					);
		m_pageToLayoutTran.scale(uw,uh);
	}
	else
	{
		m_layoutToPageTran = QTransform();
		m_pageToLayoutTran = QTransform();
	}
}

//virtual
qreal	IconLayout::verticalDistanceToNearestUpperRow(const QPointF& layoutCoordinate)
{
	return 0.0;
}

//virtual
qreal	IconLayout::verticalDistanceToNearestLowerRow(const QPointF& layoutCoordinate)
{
	return 0.0;
}

//virtual
quint32 IconLayout::maximumRowHeight() const
{
	return 0;
}

//virtual
quint32 IconLayout::minimumRowHeight() const
{
	return 0;
}

//virtual
QList<IconCell *> IconLayout::iconCellsInFlowOrder()
{
	//do nothing; row types are specific to subclass'd layouts
	// (i.e. see AlphabeticIconLayout, ReorderableIconLayout,... versions of this fn)
	return QList<IconCell *>();
}

//protected:

QDataStream & operator<< (QDataStream& stream, const IconLayout& s)
{
	stream << "\"IconLayout\": {}";
	return stream;
}

QDebug operator<<(QDebug dbg, const IconLayout &s)
{
	dbg.nospace() << "\"IconLayout\": {}";
	return dbg.space();
}

QDataStream & operator>> (QDataStream& stream, IconLayout& s)
{
	//TODO: IMPLEMENT
	return stream;
}
