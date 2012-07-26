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




#include "reorderableiconlayout.h"
#include "reorderablepage.h"
#include "iconlayoutsettings.h"
#include "dynamicssettings.h"
#include "iconreorderanimation.h"
#include "iconlayout.h"
#include "icon.h"
#include "iconheap.h"
#include "page.h"
#include "pixmapobject.h"
#include "dimensionslauncher.h"

#include <QPainter>
#include <QTransform>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QState>
#include <QStateMachine>
#include <QSignalTransition>

ReorderableIconLayout::ReorderableIconLayout(ReorderablePage * p_owner)
: IconLayout(p_owner)
, m_layoutSync(false)
, m_relayoutCount(0)
, m_disabledAutoPaint(true)
, m_usingReorderModeGraphics(false)
, m_maxWidthForRows(0.0)
, m_anchorRow(0)
, m_iconCellSize(0,0)
, m_horizIconSpacing(0.0)
, m_layoutSizeInPixels(0,0)
, m_listIdInUse(0)
, m_qp_reorderAnimationGroup(0)
, m_p_reorderFSM(0)
, m_p_fsmStateConsistent(0)
, m_p_fsmStateReorderPending(0)
, m_p_fsmStateTrackedFloating(0)
, m_p_fsmStateReorderPendingAndTrackedFloating(0)
{
	m_maxIconsPerRow = IconLayoutSettings::settings()->reorderablelayout_maxIconsPerRow;
	m_horizIconSpacingAdjust = IconLayoutSettings::settings()->reorderablelayout_iconHorizSpaceAdjustInPixels;
	m_interRowSpace = IconLayoutSettings::settings()->reorderablelayout_interRowSpaceInPixels;
	m_leftMarginForRows = IconLayoutSettings::settings()->reorderablelayout_rowLeftMarginInPixels;
	m_topMarginForRows = IconLayoutSettings::settings()->reorderablelayout_rowTopMarginInPixels;

	m_reorderEventSampleRate = DynamicsSettings::settings()->iconReorderSampleRate;
	m_maxVelocityForSampling = DynamicsSettings::settings()->maxVelocityForSampling;
	m_timeNormalizationUnit = DynamicsSettings::settings()->timeNormalizationUnit;
	m_magFactor = DynamicsSettings::settings()->distanceMagFactor;
	setupReorderFSM();
	startReorderFSM();
}

//virtual
ReorderableIconLayout::~ReorderableIconLayout()
{
	delete m_qp_reorderAnimationGroup;
}

//virtual
void ReorderableIconLayout::setLayoutRowSpacing(const qreal interRowSpace,
											const quint32 anchorRowNum)
{
	m_interRowSpace = qAbs(interRowSpace);

	if (m_iconRows.empty())
	{
		return;	//nothing to do
	}
	if (m_iconRows.size() == 1)
	{
		m_iconRows[0]->m_spaceToUpperAdjacentRow = m_topMarginForRows;
		m_iconRows[0]->m_spaceToLowerAdjacentRow = 0;
		return;
	}

	//TODO: respect anchor...the function is trivial right now
	quint32 anchorRow = (anchorRowNum >= (quint32)m_iconRows.size() ? 0 : anchorRowNum);
	m_anchorRow = anchorRow;
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		if ((it+1) == m_iconRows.end())
		{
			//bottom most
			(*it)->m_spaceToUpperAdjacentRow = m_interRowSpace;
			(*it)->m_spaceToLowerAdjacentRow = 0;
		}
		else if (it == m_iconRows.begin())
		{
			//top most
			(*it)->m_spaceToUpperAdjacentRow = m_topMarginForRows;
			(*it)->m_spaceToLowerAdjacentRow = m_interRowSpace;
		}
		else
		{
			(*it)->m_spaceToUpperAdjacentRow = m_interRowSpace;
			(*it)->m_spaceToLowerAdjacentRow = m_interRowSpace;
		}
	}
}

//virtual
void ReorderableIconLayout::resetLayoutRowSpacingToDefaultSettings()
{
	m_interRowSpace = IconLayoutSettings::settings()->reorderablelayout_interRowSpaceInPixels;
	setLayoutRowSpacing(m_interRowSpace);
}


//virtual
void ReorderableIconLayout::paint(QPainter * painter)
{
	//paint all the icons in the icon list
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paint(painter);
	}
}

//virtual
void ReorderableIconLayout::paint(QPainter * painter, const QRectF& sourceRect)
{
	//paint all rows, which will paint all the icons

	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paint(painter,sourceRect);
	}
}

//virtual
void ReorderableIconLayout::paint(const QPointF& translate,QPainter * painter)
{
	//paint all the icons in the icon list
	QTransform saveTran = painter->transform();
	painter->translate(translate);
	//paint all the icons in the icon list
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paint(painter);
	}
	painter->setTransform(saveTran);
}

//virtual
void ReorderableIconLayout::paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt)
{
	//paint all rows, which will paint all the icons

	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paint(painter,sourceRect,renderOpt);
	}
}

// paintOffscreen()s are supposed to ignore m_disabledPaint
//virtual
void ReorderableIconLayout::paintOffscreen(QPainter * painter)
{
	//paint all the icons in the icon list
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paintOffscreen(painter);
	}
}

//virtual
void ReorderableIconLayout::paintOffscreen(PixmapObject * p_pmo)
{
	if (!p_pmo)
	{
		return;
	}
	//TODO: check bounds
	((QPixmap *)p_pmo->data())->fill(Qt::transparent);
	QPainter painter(p_pmo->data());
	paintOffscreen(&painter);
}

//virtual
void ReorderableIconLayout::paintOffscreen(PixmapHugeObject * p_hugePmo)
{
	if (!p_hugePmo)
	{
		return;
	}
	//paint all the icons in the icon list
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paintOffscreen(p_hugePmo);
	}
}

//virtual
void ReorderableIconLayout::enableAutoPaint()
{
	m_disabledAutoPaint=false;
}

//virtual
void ReorderableIconLayout::disableAutoPaint()
{
	m_disabledAutoPaint=true;
}

//virtual
void ReorderableIconLayout::relayout(bool force)
{
	if (m_layoutSync && !force)
	{
		return;
	}
	if (m_iconRows.empty())
	{
		return;
	}

	//TODO: SLOW: fix this so it doesn't need to be run twice, fully...optim. can be made so that
	// the top left of the layout geom can be determined without a full initial layout with a fake top left

	//phase 0 has a m_geom with a top left of 0,0
	m_geom = QRectF(0,0,0,0);
	for (int layoutPhase = 0;layoutPhase < 2;++layoutPhase)
	{
		// if this is the first time this FUNCTION (not phase dependent)
		//	is being run, then use 0 as the anchor and position it manually
		if (m_relayoutCount == 0)
		{
			m_anchorRow = 0;
			//Row 0, if it is the initial anchor row, is positioned such that the topLeft point on the row geom is at the same location
			//	as the topLeft corner of the overall layout geom
			m_iconRows[0]->m_pos = m_geom.topLeft() - m_iconRows[0]->m_geom.topLeft()
									+ QPointF((qreal)m_leftMarginForRows,m_iconRows[0]->m_spaceToUpperAdjacentRow);
			m_iconRows[0]->relayout(m_geom.left()+m_leftMarginForRows,m_horizIconSpacing,true);
		}

		//		go through the rows, from anchor up and from anchor down, moving the rows and relaying out the icons as I go along

		//from anchor -1 and down			(i<size() due to wraparound to uint_max when --0)
		quint32 trailIdx;
		for (quint32 i=m_anchorRow-1;(i>=0) && (i< (quint32)m_iconRows.size()) && (m_anchorRow != 0);--i)
		{
			trailIdx = i+1;
			m_iconRows[i]->repositionAboveRow(*m_iconRows[trailIdx]);
			m_iconRows[i]->m_pos.setX(m_geom.left()-m_iconRows[i]->m_geom.left()+m_leftMarginForRows);
			m_iconRows[i]->relayout(m_geom.left()+m_leftMarginForRows,m_horizIconSpacing,true);
		}

		//from anchor +1 and up
		for (quint32 i=m_anchorRow+1;(i< (quint32)m_iconRows.size()) && (m_anchorRow != (quint32)m_iconRows.size()-1);++i)
		{
			trailIdx = i-1;
			m_iconRows[i]->repositionBelowRow(*m_iconRows[trailIdx]);
			m_iconRows[i]->m_pos.setX(m_geom.left()-m_iconRows[i]->m_geom.left()+m_leftMarginForRows);
			m_iconRows[i]->relayout(m_geom.left()+m_leftMarginForRows,m_horizIconSpacing,true);
		}

		//now recompute the m_geom and loop (or exit)

		m_layoutSizeInPixels =
			QSize(m_maxWidthForRows,
			 (quint32)DimensionsGlobal::roundUp(
			m_iconRows[m_iconRows.size()-1]->relativeGeometry().bottom()+m_iconRows[m_iconRows.size()-1]->m_spaceToLowerAdjacentRow
										- m_iconRows[0]->relativeGeometry().top()+m_iconRows[0]->m_spaceToUpperAdjacentRow));
		m_geom = DimensionsGlobal::realRectAroundRealPoint(m_layoutSizeInPixels);
	}


//	{
//		int i = 0;
//		////DEBUG://// check the row layouts
//		for (IconRowIter it = m_iconRows.begin();
//				it != m_iconRows.end();++it)
//		{
//			(*it)->relayout(force);
//			//qDebug() << "row layout relative geom" << i << " : " << (*it)->relativeGeometry()
//						<< " , row geom : " << (*it)->geometry()
//						<< " , position : " << (*it)->m_pos;
//			++i;
//		}
//	}

	//set the pixel size of the layout -- this is used when the layout is painted on a pixmap
	//TODO: PIXEL-ALIGN
	m_layoutSizeInPixels = m_geom.size().toSize();
	//qDebug() << "final calculated layout size : " << m_layoutSizeInPixels << " , geom: " << m_geom;
	++m_relayoutCount;
	m_layoutSync=true;
}

//virtual
void ReorderableIconLayout::resizeWidth(const quint32 w)
{
	if ((w == 0) || (m_maxWidthForRows == w))
	{
		return;
	}

//	//qDebug() << "%&%&%&%&%&%&%& " << __FUNCTION__ << " %&%&%&%&%&%&%&";

	//all reorders must stop...NOW!
	cancelAllReorder();
	// Code COMMENT: this will call back into ReorderablePage (if that is in fact the owner; if not, and it's a different
	// page type - not currently supported - then something similar will have to be called) ...for ReorderablePage, it'll
	// call slotTrackedIconCancelTrack(). Also, as a part of all of the icons individually commiting, eventually the
	// signalReorderFinished() will emit - in fact before slotTrackedIconCancelTrack is executed.

	m_maxWidthForRows = w;
	m_layoutSync = false;

	//TODO: TEMP: the idea of layout sync is to be able to delay full-out relayouts.
	// for now, run the relayout routines from here; later, more high level interface functions to IconLayout should exist
	// so that something else will determine if/when a full layout should take place

	relayoutExisting();

}

//virtual
void ReorderableIconLayout::setPosition(const QPointF& pos)
{
	IconLayout::setPosition(pos);
	m_layoutSync = false;
}

//virtual
void ReorderableIconLayout::setUniformCellSize(const QSize& size)
{
	m_iconCellSize = size;
	m_layoutSync = false;
}

//virtual
void ReorderableIconLayout::iconCellReleaseIcon(const QPoint& cellCoordinate)
{
	IconCell * pCell = iconCellAtGridCoordinate(cellCoordinate);
	if (!pCell)
	{
		return; //no cell here
	}
	if (!pCell->m_qp_icon)
	{
		return;	//no icon
	}
	pCell->resetIconPosition();
	pCell->m_qp_icon = 0;		//There'd better be another reference to it somewhere....
}

//virtual
IconCell * ReorderableIconLayout::iconCellAtLayoutCoordinate(const QPointF& coordinate)
{
	/*
	 *
	 * coordinate:
	 * 		expected to be in the cs of the layout, which is a translated ICS of the Page that owns this layout.
	 * 		i.e. if the layout has been shifted in some way by the page, it is expected that the page will unshift coordinate
	 * 		so that it's in the range of [m_geom.topLeft,m_geom.bottomRight]
	 *
	 */

	//TODO: SLOW

	for (IconRowIter it = m_iconRows.begin(); it != m_iconRows.end();++it)
	{
		if ((*it)->relativeGeometry().contains(coordinate))
		{
			for (IconRow::IconCellListIter iit = (*it)->m_iconList.begin(); iit != (*it)->m_iconList.end();++iit)
			{
				if ((*iit)->relativeGeometry().contains(coordinate))
				{
					return *iit;
				}
			}
			return 0;
		}
	}
	return 0;
}

//virtual
IconCell * ReorderableIconLayout::iconCellAtLayoutCoordinate(const QPointF& layoutCoordinate,QPoint& r_gridCoordinate)
{
	//TODO: SLOW

	qint32 r=0;
	qint32 c=0;
	for (IconRowIter it = m_iconRows.begin(); it != m_iconRows.end();++it)
	{
		if ((*it)->relativeGeometry().contains(layoutCoordinate))
		{
			for (IconRow::IconCellListIter iit = (*it)->m_iconList.begin(); iit != (*it)->m_iconList.end();++iit)
			{
				if ((*iit)->relativeGeometry().contains(layoutCoordinate))
				{
					r_gridCoordinate = QPoint(c,r);
					return *iit;
				}
				++c;
			}
			return 0;
		}
		++r;
	}
	return 0;
}

//virtual
QRectF ReorderableIconLayout::rowArea(quint32 rowIndex) const
{
	if (rowIndex >= (quint32)m_iconRows.size())
	{
		return QRectF();
	}
	IconRow const * pRow = m_iconRows.at(rowIndex);
	return pRow->relativeGeometry();
}

//virtual
QPoint ReorderableIconLayout::lastOccupiedGridPosition() const
{
	if (m_iconRows.isEmpty())
	{
		return QPoint(-1,-1);
	}
	IconRow * pLastRow = m_iconRows.at(m_iconRows.size()-1);
	return QPoint(pLastRow->m_iconList.size()-1,m_iconRows.size()-1);
}

//virtual
QPoint ReorderableIconLayout::nextAppendGridPosition() const
{
	if (m_iconRows.isEmpty())
	{
		return QPoint(0,0);
	}
	IconRow * pLastRow = m_iconRows.at(m_iconRows.size()-1);
	if ((quint32)(pLastRow->m_iconList.size()) >= m_maxIconsPerRow)
	{
		return QPoint(0,m_iconRows.size());
	}
	return QPoint(pLastRow->m_iconList.size(),m_iconRows.size()-1);
}

//virtual
qint32 ReorderableIconLayout::rowAtLayoutCoordinate(const QPointF& layoutCoordinate,bool clipMinMax)
{
	if (m_iconRows.isEmpty())
	{
		return -1;
	}
	if (m_iconRows.at(0)->relativeGeometry().y() > layoutCoordinate.y())
	{
		return ( clipMinMax ? 0 : -1);
	}

	qint32 rowIndex=0;
	for (IconRowConstIter it = m_iconRows.constBegin();
			it != m_iconRows.constEnd();++it,++rowIndex)
	{
		if ((*it)->relativeGeometry().contains(layoutCoordinate))
		{
			return rowIndex;
		}
	}
	return (clipMinMax ? m_iconRows.size()-1 : m_iconRows.size());
}

//virtual
qint32 ReorderableIconLayout::rowAtLayoutCoordinateFuzzy(const QPointF& layoutCoordinate,bool clipMinMax)
{
	if (m_iconRows.isEmpty())
	{
		return -1;
	}
	if (m_iconRows.at(0)->relativeGeometry().y() > layoutCoordinate.y())
	{
		return ( clipMinMax ? 0 : -1);
	}

	if (m_iconRows.at(0)->relativeGeometry().contains(layoutCoordinate))
	{
		return 0;
	}

	qint32 rowIndex=1;
	for (IconRowConstIter it = m_iconRows.constBegin()+1;
			it != m_iconRows.constEnd();++it,++rowIndex)
	{
		if ((*it)->relativeGeometry().contains(layoutCoordinate))
		{
			return rowIndex;
		}
		else if ( ((*(it-1))->relativeGeometry().bottom() < layoutCoordinate.y())
				&& ( (*it)->relativeGeometry().top() > layoutCoordinate.y())
				)
		{
			return rowIndex-1;
		}
	}
	return (m_iconRows.size()-1);
}

//virtual
IconCell * ReorderableIconLayout::iconCellAtGridCoordinate(const QPoint& gridCoordinate)
{

	//TODO: PAST-CRASH: crash seen here once, and safety "ifs" added.
	//		It was during a rotation, probably during a reorder that caused a cancelAllReorder.
	//	My guess is that the last icon was being reordered, which removed the row, and hence made
	// the gridCoordinate no longer valid for this layout
	if (gridCoordinate.y() < m_iconRows.size())
	{
		if (gridCoordinate.x() < m_iconRows[gridCoordinate.y()]->m_iconList.size())
		{
			IconCell * p = m_iconRows[gridCoordinate.y()]->m_iconList[gridCoordinate.x()];
			return p;
		}
	}
	return 0;
}

//virtual
bool ReorderableIconLayout::layoutCoordinateForGridCoordinate(const QPoint& gridCoordinate,QPointF& r_layoutCoordinate)
{

	//TODO: error check possibly...it's called from within the oplist functions though (which are called from move event handlers)
	// so need to be careful about performance
	//for now just always return true (it'll crash if it would have been false anyways)

	r_layoutCoordinate = m_iconRows.at(gridCoordinate.y())->m_iconList.at(gridCoordinate.x())->m_pos;
	return true;
}

//virtual
IconCell * ReorderableIconLayout::findIconByUid(const QUuid& iconUid,QPoint& r_gridCoordinate,bool includePendingIconsInCells)
{
	int x = 0;
	int y = 0;
	//TODO: SLOW
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it,++y,x=0)
	{
		for (IconRow::IconCellListIter cit = (*it)->m_iconList.begin();
				cit != (*it)->m_iconList.end();++cit,++x)
		{
			if ((*cit)->m_qp_icon)
			{
				if ((*cit)->m_qp_icon->uid() == iconUid)
				{
					r_gridCoordinate = QPoint(x,y);
					//qDebug() << __FUNCTION__ << " found icon on " << r_gridCoordinate;
					return *cit;
				}
				else
				{
					//qDebug() << __FUNCTION__ << " miss on " << QPoint(x,y) << " , uid there is: " << (*cit)->m_qp_icon->uid();
				}
			}
			else
			{
				//qDebug() << __FUNCTION__ << " miss on " << QPoint(x,y) << " no icon there";
			}
		}
	}
	if (includePendingIconsInCells)
	{
		x=0;
		y=0;
		for (IconRowIter it = m_iconRows.begin();
				it != m_iconRows.end();++it,++y,x=0)
		{
			for (IconRow::IconCellListIter cit = (*it)->m_iconList.begin();
					cit != (*it)->m_iconList.end();++cit,++x)
			{
				if ((*cit)->m_qp_pendingReorderIcon)
				{
					if ((*cit)->m_qp_pendingReorderIcon->uid() == iconUid)
					{
						r_gridCoordinate = QPoint(x,y);
						return *cit;
					}
				}
			}
		}
	}
	return 0;
}

//virtual
QList<IconCell *> ReorderableIconLayout::iconCellsInFlowOrder()
{
	QList<IconCell *> rlist;
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		for (IconRow::IconCellListIter cit = (*it)->m_iconList.begin();
				cit != (*it)->m_iconList.end();++cit)
		{
			rlist << *cit;
		}
	}
	return rlist;
}

// returns 0 (null) if tracking this icon isn't possible at the moment
//TODO: HACK: includePendingIconsInCells allows the use of an already pending icon. This is dangerous and isn't by design...
//virtual
IconBase * ReorderableIconLayout::startTrackingIcon(const QPoint& gridCoord,bool includePendingIconsInCells)
{

	//if something is currently pending, then disallow this
	if (!m_reorderPendingCellList.isEmpty())
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << gridCoord << " because there is an uncommited reorder pending";
		return 0;
	}


	IconCell * pCell = iconCellAtGridCoordinate(gridCoord);
	if (!pCell)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << gridCoord << " because there is no cell there";
		return 0;
	}
	IconBase * pIcon = 0;
	if (!pCell->m_qp_icon)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": icon @" << gridCoord << ": the cell there has no icon";
		if (includePendingIconsInCells)
		{
			if (pCell->m_qp_pendingReorderIcon)
			{
				qDebug() << " ... it DOES have a pending icon, and use of it was allowed";
				pIcon = pCell->m_qp_pendingReorderIcon;
			}
			else
			{
				qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << gridCoord << " because the cell there has no icon AND no pending icon (use of pending WAS allowed)";
				return 0;
			}
		}
		else
		{
			qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << gridCoord << " because the cell there has no icon (use of pending WAS NOT allowed)";
			qDebug() << " ... it " << (pCell->m_qp_pendingReorderIcon ? "DOES" : "DOES NOT") << " have a pending icon";
			return 0;
		}
	}
	else
	{
		pIcon = pCell->m_qp_icon;
	}

	////qDebug() << " the cell @" << gridCoord << "reports its position as: " << pCell->position() << " mapped to page: " << this->pageCoordinateFromLayoutCoordinate(pCell->position());
	if (m_trackedIconAnimations.constFind(pIcon->uid()) != m_trackedIconAnimations.constEnd())
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << gridCoord << " because this icon is busy returning from a reorder final animation";
		return 0;
	}
	Q_EMIT signalFSMTrackStarted_Trigger();
	m_trackedIconLastPosition.insert(pIcon->uid(),gridCoord);
	pIcon->setZValue(pIcon->zValue()+1.0);
	pCell->m_qp_icon = 0;		//this will prevent it from redrawing as a part of this layout
	pCell->m_qp_pendingReorderIcon = pIcon;		//..but it's still part of this cell until moved
	return pIcon;
}

//this variant starts tracking an icon by finding it in the layout by its actual pointer. it is used in the case where the icon
// comes back from the quicklaunch during one, continuous tracking operation (the following pattern: move icon to QL, don't let go
// of the icon on the QL but immediately move it back to the page). Returns true if the icon was found and can be tracked,
//	and puts its current grid coord in r_gridCoord.
//virtual
bool ReorderableIconLayout::startTrackingIcon(IconBase * p_icon,QPoint& r_gridCoord)
{
	if (p_icon == 0)
	{
		return false;
	}
	IconCell * pCell = findIconByUid(p_icon->master()->uid(),r_gridCoord,true);
	if (!pCell)
	{
		qDebug() << "ReorderableIconLayout::"<<__FUNCTION__ << ": icon not found!";
		return false;
	}

	IconBase * pCheck = startTrackingIcon(r_gridCoord,true);
	if (!pCheck)
	{
		qDebug() << "ReorderableIconLayout::"<<__FUNCTION__ << ": startTrackingIcon @ " << r_gridCoord << " failed";
		return false;
	}
	return true;
}

//virtual
IconBase * ReorderableIconLayout::startTrackingIconFromTransfer(const QPoint& gridCoord,IconBase * iconTracked)
{
	//if something is currently pending, then disallow this
	if (!m_reorderPendingCellList.isEmpty())
	{
		//qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << gridCoord << " because there is an uncommited reorder pending";
		return 0;
	}

	//the errors below represent real FUBARs!!!
	IconCell * pCell = iconCellAtGridCoordinate(gridCoord);
	if (!pCell)
	{
		//qDebug() << __PRETTY_FUNCTION__ << ": Cannot start transfer-tracking icon @" << gridCoord << " because there is no cell there";
		return 0;
	}
	if (pCell->m_qp_icon)
	{
		//qDebug() << __PRETTY_FUNCTION__ << ": Cannot start transfer-tracking icon @" << gridCoord << " because the cell there has an icon already";
		return 0;
	}
	Q_EMIT signalFSMTrackStarted_Trigger();
	m_trackedIconLastPosition.insert(iconTracked->uid(),gridCoord);
	iconTracked->setZValue(iconTracked->zValue()+1.0);
	pCell->m_qp_icon = 0;		//this will prevent it from redrawing as a part of this layout
	pCell->m_qp_pendingReorderIcon = iconTracked;		//..but it's still part of this cell until moved
	return iconTracked;
}

//virtual
bool ReorderableIconLayout::trackedIconMovedTo(IconBase * p_icon,const QPoint& gridCoord,QPoint& r_newGridCoord)
{
	if (!p_icon)
	{
		return false;
	}
	if (!m_reorderPendingCellList.isEmpty())
	{
		if (DynamicsSettings::settings()->alwaysWaitForIconReorderAnimToFinish)
		{
			////qDebug() << __PRETTY_FUNCTION__ << ": Cannot update tracking icon " << p_icon->uid() << " to " << gridCoord << " because there is an uncommited reorder pending";
			return false;
		}
		else
		{
			//I've been told I'm allowed to kill the animation and force a commit immediately. This will have the effect of zipping every icon immediately to its final location
			delete m_qp_reorderAnimationGroup;
			commitPending();
		}
	}

	//get the last position out of the map
	QPoint lastGridCoord;
	if (!lastTrackedPosition(p_icon->uid(),lastGridCoord))
	{
		//didn't find
		////qDebug() << __PRETTY_FUNCTION__ << ": Cannot update tracking icon " << p_icon->uid() << " to " << gridCoord << " because it apparently never started tracking";
	}
	QList<IconOperation> oplist = opListForMove(lastGridCoord,gridCoord);
	if (oplist.empty())
	{
		//nothing to do
		m_trackedIconLastPosition.insert(p_icon->uid(),gridCoord);
		r_newGridCoord = gridCoord;
		IconCell * pDestCell = this->iconCellAtGridCoordinate(gridCoord);
		if (!pDestCell)
		{
			//WHAT THE!!!????
			return false;
		}
		pDestCell->m_qp_pendingReorderIcon = p_icon;
		return true;
	}

	if (executeOpList(oplist))
	{
		//success - update the position
		m_trackedIconLastPosition.insert(p_icon->uid(),gridCoord);
		r_newGridCoord = gridCoord;
		IconCell * pDestCell = this->iconCellAtGridCoordinate(gridCoord);
		if (!pDestCell)
		{
			//WHAT THE!!!????
			return false;
		}
		pDestCell->m_qp_pendingReorderIcon = p_icon;
		return true;
	}
	return false;
}

//virtual
bool ReorderableIconLayout::lastTrackedPosition(const QUuid& iconUid,QPoint& r_lastGridCoord)
{
	QMap<QUuid,QPoint>::const_iterator f = m_trackedIconLastPosition.constFind(iconUid);
	if (f != m_trackedIconLastPosition.constEnd())
	{
		r_lastGridCoord = f.value();
		return true;
	}
	return false;
}

//virtual
void ReorderableIconLayout::stopTrackingIcon(IconBase * p_icon)
{
	if (p_icon == 0)
	{
		return;
	}

	QPoint lastGridCoord;
	if (!lastTrackedPosition(p_icon->uid(),lastGridCoord))
	{
		//didn't find
		////qDebug() << __PRETTY_FUNCTION__ << ": Cannot stop tracking icon " << p_icon->uid() << " @" << lastGridCoord << " because it apparently never started tracking";
		return;
	}

	//this should never be called on an uncommited state...
	if (!m_reorderPendingCellList.isEmpty())
	{
		if (m_qp_reorderAnimationGroup)
		{
			if ((m_qp_reorderAnimationGroup->state() == QAbstractAnimation::Running)
				&& (!DynamicsSettings::settings()->alwaysWaitForIconReorderAnimToFinish))
			{
				//I'm allowed to kill the animation prematurely
				delete m_qp_reorderAnimationGroup;
				commitPending();
			}
		}
		else
		{
			//no animation was started for this so i'm going to do a full commit immediately
			commitPending();
		}
	}

	//at this point, a commit did or definitely will happen (in the case of prefs telling me to let the animations finish)
	commitTracked(p_icon->uid(),lastGridCoord);
}

//virtual
void ReorderableIconLayout::stopTrackingAll()
{
	//this should never be called on an uncommited state...
	if (!m_reorderPendingCellList.isEmpty())
	{
		if (m_qp_reorderAnimationGroup)
		{
			if ((m_qp_reorderAnimationGroup->state() == QAbstractAnimation::Running)
				&& (!DynamicsSettings::settings()->alwaysWaitForIconReorderAnimToFinish))
			{
				//I'm allowed to kill the animation prematurely
				delete m_qp_reorderAnimationGroup;
				commitPending();
			}
		}
		else
		{
			//no animation was started for this so i'm going to do a full commit immediately
			commitPending();
		}
	}

	//at this point, a commit did or definitely will happen in a proper sequence (in the case of prefs telling me to let the animations finish)

	//must call commitTracked() via iterating over keys() because m_trackedIconLastPosition will be modified within which will inval the iterator if used here
	QList<QUuid> uidKeys = m_trackedIconLastPosition.keys();
	for (QList<QUuid>::const_iterator it = uidKeys.constBegin();
			it != uidKeys.constEnd();++it)
	{
		commitTracked(*it);
	}
}

//virtual
bool ReorderableIconLayout::trackedIconLeavingLayout(const QUuid& trackedIconUid)
{
	// THIS SHOULD NEVER BE CALLED WHILE THE TRACKED ICON IS NOT UNDER TOUCH CONTROL (e.g. animating back to a position)
	// OR OTHER ICONS ARE UNDER ACTIVE REORDERING (i.e. a reorder is pending)
	QPoint gridPos;
	if (!lastTrackedPosition(trackedIconUid,gridPos))
	{
		//qDebug() << __PRETTY_FUNCTION__ << ": Cannot stop tracking icon (transfer) " << trackedIconUid << " because it apparently never started tracking";
		return false;
	}

	//remove the cell - this will end in misery if the above warning isn't heeded
	removeIconCell(gridPos);

	Q_EMIT signalFSMTrackEnded_Trigger();
	return true;
}

//virtual
IconBase * ReorderableIconLayout::getTrackedIconByUid(const QUuid& trackedIconUid)
{

	//just a convenience function

	QMap<QUuid,QPoint>::const_iterator fp = m_trackedIconLastPosition.constFind(trackedIconUid);
	if (fp != m_trackedIconLastPosition.constEnd())
	{
		return IconHeap::iconHeap()->getIcon(trackedIconUid);
	}
	return 0;
}

//virtual
void ReorderableIconLayout::cancelAllReorder()
{
	commitPendingImmediately();
	//must call commitTracked() via iterating over keys() because m_trackedIconLastPosition will be modified within which will inval the iterator if used here
	QList<QUuid> uidKeys = m_trackedIconLastPosition.keys();
	for (QList<QUuid>::const_iterator it = uidKeys.constBegin();
			it != uidKeys.constEnd();++it)
	{
		commitTrackedImmediately(*it);
	}

	ReorderablePage * pReorderPage = qobject_cast<ReorderablePage *>(m_qp_ownerPage);
	if (pReorderPage)
	{
		//needs to be a reorder page
		for (QList<QUuid>::const_iterator it = uidKeys.constBegin();
				it != uidKeys.constEnd();++it)
		{
			pReorderPage->slotTrackedIconCancelTrack(*it);
		}
	}
}

//virtual
void ReorderableIconLayout::commitPendingImmediately()
{
	delete m_qp_reorderAnimationGroup;
	commitPending();
}

//virtual
void ReorderableIconLayout::commitTrackedImmediately(const QUuid& iconUid)
{
	QPoint lastGridCoord;
	if (!lastTrackedPosition(iconUid,lastGridCoord))
	{
		//didn't find it ...it could mean it was already processed, so i'll assume it's ok
		Q_EMIT signalFSMTrackEnded_Trigger();
		return;
	}

	return commitTrackedImmediately(iconUid,lastGridCoord);
}

//virtual
void ReorderableIconLayout::commitTrackedImmediately(const QUuid& iconUid,const QPoint& lastGridCoord)
{
	//the final destination cell should already have the floating icon referenced in the "pending" var
	IconCell * pDestCell = iconCellAtGridCoordinate(lastGridCoord);
	if (!pDestCell)
	{
		Q_EMIT signalFSMTrackEnded_Trigger();
		return;
	}
	if (!pDestCell->m_qp_pendingReorderIcon)
	{
		////qDebug() << __FUNCTION__ << ": ERROR: cell dest " << pDestCell << " @" << lastGridCoord << " has an empty 'pending' reference";
		Q_EMIT signalFSMTrackEnded_Trigger();
		return;
	}
	//if there is already an animation for this tracked icon (to place it to its final position), then delete the anim and commit to the cell immediately
	QMap<QUuid,QPointer<IconReorderAnimation> >::iterator f = m_trackedIconAnimations.find(iconUid);
	if (f != m_trackedIconAnimations.end())
	{
		delete f.value();
		m_trackedIconAnimations.erase(f);
	}

	//reload tracking info from the map; this is necessary because of the uncertainty of whether or not an animation that may or may not have existed,
	// actually placed the tracked icon into its final cell location (upon being deleted, right above here). If it did, then the map entry will be erased and nothing more needs to be done
	// if it did not, then explicitly do that placement here

	QMap<QUuid,QPoint>::iterator fp = m_trackedIconLastPosition.find(iconUid);
	if (fp != m_trackedIconLastPosition.end())
	{
		//not handled yet
		m_trackedIconLastPosition.erase(fp);
		//commit the icon to the destination cell
		IconCell * pDestCell = iconCellAtGridCoordinate(lastGridCoord);
		if (pDestCell)
		{
			pDestCell->commitPendingIcon();
		}
	}

	Q_EMIT signalFSMTrackEnded_Trigger();
	return;
}


#define IdOpMacro IconOperation(IconOperation::Id,0,0,0,0)
#define OneCellAhead(sourceCell,slotLeft) \
{	\
	if ((quint32)sourceCell.x() == (m_maxIconsPerRow-1)) \
	{ \
		slotLeft.setX(0); \
		slotLeft.setY(sourceCell.y()+1); \
	} \
	else \
	{ \
		slotLeft.setX(sourceCell.x()+1); \
		slotLeft.setY(sourceCell.y()); \
	} \
}
#define OneCellBehind(sourceCell,slotLeft) \
{	\
	if (sourceCell.x() == 0) \
	{ \
		slotLeft.setX(m_maxIconsPerRow-1); \
		slotLeft.setY(sourceCell.y()-1); \
	} \
	else \
	{ \
		slotLeft.setX(sourceCell.x()-1); \
		slotLeft.setY(sourceCell.y()); \
	} \
}

//virtual
QList<IconOperation> ReorderableIconLayout::opListForMove(const QPoint& sourceColumnRow,const QPoint& destColumnRow)
{
	//the qpoints have in them encoded x = column, y = row
	// turn them proper, by checking for out-of-bounds and neg values

	QPoint src = QPoint(qAbs(sourceColumnRow.x()),qAbs(sourceColumnRow.y()));
	QPoint dst = QPoint(qAbs(destColumnRow.x()),qAbs(destColumnRow.y()));

	// note that dst.y() == num icon rows *is* a valid position/index-of-row, since it represents an append to a
	// NEW row
	if ((src.y() >= m_iconRows.size()) || (dst.y() > m_iconRows.size()))
	{
		//invalid...return null list
		return QList<IconOperation>();
	}

	//note that x() == icon list.size() *is* a valid position, since it represents an append to the end of that row
	if ((src.x() > m_iconRows[src.y()]->m_iconList.size())
		||  (dst.x() > m_iconRows[dst.y()]->m_iconList.size()))
	{
		return QList<IconOperation>();
	}

	quint32 srcIdx = src.y() * m_maxIconsPerRow + src.x();
	quint32 dstIdx = dst.y() * m_maxIconsPerRow + dst.x();

	if (srcIdx == dstIdx)
	{
		//nothing to do
		return QList<IconOperation>();
	}
	//insert an Id op first
	QList<IconOperation> oplist;
	oplist << IdOpMacro;

	/*
	 * If the destination is AHEAD of the source, then all icons move BACKWARD one slot, with row i [head]
	 * moving to row i-1 [tail].
	 *
	 * If the destination is BEHIND the source, then all icons move FORWARD one slot, with row i [tail]
	 * moving to row i+1 [head]
	 *
	 */

	//TODO: DEBUG: broken out for debug; can be made a bit more efficient
	if (dstIdx > srcIdx)
	{
		//destination AHEAD

		QPoint sp,dp;
		OneCellAhead(src,sp);

		for (int row = sp.y();row <= dst.y();++row)
		{
			for (int col=(row == sp.y() ? sp.x() : 0);(quint32)col <= (quint32)(row == dst.y() ? dst.x() : m_maxIconsPerRow-1);++col)
			{
				QPoint t = QPoint(col,row);
				OneCellBehind(t,dp);
				oplist << IconOperation(IconOperation::Reorder,t.x(),t.y(),dp.x(),dp.y());
			}
		}
	}
	else
	{
		//destination BEHIND
		QPoint sp,dp;
		OneCellBehind(src,sp);		//sp isn't a "start" pt like in the AHEAD case here; it's really the last point
									//that needs to be auto moved (one behind the source pt)
		for (int row = dst.y();row <= sp.y();++row)
		{
			for (int col=(row == dst.y() ? dst.x() : 0);(quint32)col <= (quint32)(row == sp.y() ? sp.x() : m_maxIconsPerRow-1);++col)
			{
				QPoint t = QPoint(col,row);
				OneCellAhead(t,dp);
				oplist << IconOperation(IconOperation::Reorder,t.x(),t.y(),dp.x(),dp.y());
			}
		}
	}

	return oplist;
}

//returns the animations to perform the actions described in the opList
//virtual
QAnimationGroup * ReorderableIconLayout::animationsForOpList(QList<IconOperation>& opList)
{

	if (opList.empty())
	{
		return 0;
	}

	QParallelAnimationGroup * pAnimGroup = new QParallelAnimationGroup();
	for (IconOpListIter it = opList.begin();it != opList.end();++it)
	{
		//TODO: handle the other ops
		if (it->m_opname != IconOperation::Reorder)
		{
			continue;
		}
		IconCell * pCellSource = iconCellAtGridCoordinate(QPoint(it->m_originColumn,it->m_originRow));
		IconCell * pCellDestination = iconCellAtGridCoordinate(QPoint(it->m_destinationColumn,it->m_destinationRow));
		if ((!pCellSource) || (!pCellDestination))
		{
			//qDebug() << __FUNCTION__ << ": WARNING: cell src " << pCellSource << " @" << QPoint(it->m_originColumn,it->m_originRow)
			//		<< " or cell dest " << pCellDestination << " @" << QPoint(it->m_destinationColumn,it->m_destinationRow)
			//		<< " is null";
		}
		//the op list only contains cells that have "connected" icons at this point (i.e. they are not floating; they're
		// pinned to the layout)
		IconBase * pIcon = pCellSource->m_qp_icon;
		if (!pIcon)
		{
			////qDebug() << __FUNCTION__ << ": WARNING: icon at cell " << QPoint(it->m_originColumn,it->m_originRow) << " isn't connected!";
			//skip
			continue;
		}
		IconReorderAnimation * pReorderAnim = new IconReorderAnimation(pIcon,this,QPoint(it->m_originColumn,it->m_originRow),
																		QPoint(it->m_destinationColumn,it->m_destinationRow));
		if (m_qp_ownerPage)
		{
			pReorderAnim->setAutoclip(m_qp_ownerPage->areaScroller().toRect());
		}
		pAnimGroup->addAnimation(pReorderAnim);
	}
	return pAnimGroup;
}

//this operates on an already populated m_reorderPendingCellList. It's used to finalize a layout if an interruption happens
// on a pending reorder, including stopTrackingIcon called illegally
//virtual
bool ReorderableIconLayout::initializePendingCellListFromOpList(QList<IconOperation>& opList)
{
	//TODO: check for higher ids
	if (opList.empty())
	{
		return false;
	}

	Q_EMIT signalFSMReorderStarted_Trigger();

	QSet<IconCell *> destinationCells;	//to track duplicates, which violates reorder layout
	//	(there can only be 1 target cell for a reorder)

	for (IconOpListIter it = opList.begin();it != opList.end();++it)
	{
		//TODO: handle the other ops
		if (it->m_opname != IconOperation::Reorder)
		{
			continue;
		}
		IconCell * pCellSource = iconCellAtGridCoordinate(QPoint(it->m_originColumn,it->m_originRow));
		IconCell * pCellDestination = iconCellAtGridCoordinate(QPoint(it->m_destinationColumn,it->m_destinationRow));
		if ((!pCellSource) || (!pCellDestination))
		{
			////qDebug() << __FUNCTION__ << ": WARNING: cell src " << pCellSource << " @" << QPoint(it->m_originColumn,it->m_originRow)
			//						<< " or cell dest " << pCellDestination << " @" << QPoint(it->m_destinationColumn,it->m_destinationRow)
			//						<< " is null";
		}
		if (destinationCells.contains(pCellDestination))
		{
			////qDebug() << __FUNCTION__ << ": WARNING: cell dest " << pCellDestination << " @" << QPoint(it->m_destinationColumn,it->m_destinationRow) << " was already processed";
			continue;
		}
		destinationCells.insert(pCellDestination);
		pCellDestination->m_qp_pendingReorderIcon = pCellSource->m_qp_icon;
	}
	for (QSet<IconCell *>::iterator it = destinationCells.begin();
			it != destinationCells.end();++it)
	{
		m_reorderPendingCellList << QPointer<IconCell>(*it);
	}
	return true;
}

//virtual
bool ReorderableIconLayout::commitPending()
{
	//commit the layout; finalize it so that it is back to a static, consistent state and ready for the next reorder
	for (QList<QPointer<IconCell> >::iterator it = m_reorderPendingCellList.begin();
			it != m_reorderPendingCellList.end();++it)
	{
		if (*it)
		{
			(*it)->commitPendingIcon();
		}
	}
	//clear the list....
	m_reorderPendingCellList.clear();
	//this was entered via the animation finished slot..which means that the animation is going away (it was run as DeleteWhenStopped)
	// OR there was no animation for this reorder, in which case it doesn't matter:  set the animation pointer to 0
	// Note: if it WASN'T run as DeleteWhenStopped, then either go fix that, or this function needs to be split into 2 variants: one that deletes the
	// animation and one that just does what this one is doing. Otherwise, there can be a potential loss of sync and at worst, an animation from the previous
	// reorder cycle will run a commit when the next cycle has already begun
	m_qp_reorderAnimationGroup = 0;

	// ---------- I AM NOW FULLY COMMITED AND READY FOR THE NEXT REORDER! -------------
	Q_EMIT signalFSMReorderEnded_Trigger();
	return true;
}

//virtual
bool ReorderableIconLayout::commitTracked(const QUuid& iconUid)
{

	QPoint lastGridCoord;
	if (!lastTrackedPosition(iconUid,lastGridCoord))
	{
		//didn't find it ...it could mean it was already processed, so i'll assume it's ok
		return true;
	}

	return commitTracked(iconUid,lastGridCoord);
}

//virtual
bool ReorderableIconLayout::commitTracked(const QUuid& iconUid,const QPoint& lastGridCoord)
{
	IconReorderAnimation * pIconFinalAnimation = animationForTrackedIconFinal(iconUid,lastGridCoord);
	if (!pIconFinalAnimation)
	{
		//problem...
		Q_EMIT signalFSMTrackEnded_Trigger();			//failsafe, even though at this point other more serious problems will be happening
		return false;
	}
	// start an animation to bring the tracked icon to its final location
	//NOTE: the place in the map for this icon MUST be EMPTY - this is a PRE-REQ ASSUMPTION of this whole mechanism and can't be violated.
	// If violations are occuring here, then go and check startTrackingIcon() and any other retrofitted entry points to the reorder logic.
	// There cannot be more than 1 tracked icon animation per tracked icon

	m_trackedIconAnimations.insert(pIconFinalAnimation->animatedIcon()->uid(),pIconFinalAnimation);
	pIconFinalAnimation->start(IconReorderAnimation::DeleteWhenStopped);
	return true;
}

//virtual
IconReorderAnimation * ReorderableIconLayout::animationForTrackedIconFinal(const QUuid& trackedIconUid,const QPoint& lastGridCoord)
{
	IconCell * pDestCell = this->iconCellAtGridCoordinate(lastGridCoord);
	if (!pDestCell)
	{
		return 0;
	}
	if (!pDestCell->m_qp_pendingReorderIcon)
	{
		////qDebug() << __FUNCTION__ << ": ERROR: cell dest " << pDestCell << " @" << lastGridCoord << " has an empty 'pending' reference";
		return 0;
	}

	//this is the no-src variant of the animation...
	IconReorderAnimation * pIconFinalAnimation = new IconReorderAnimation(pDestCell->m_qp_pendingReorderIcon,this,lastGridCoord);
	connect(pIconFinalAnimation,SIGNAL(finished()),
			this,SLOT(slotTrackedIconReplacementAnimationFinished()));
	//change the speed based on distance? (the anim ctor set a default time)
	if (m_qp_ownerPage && (DynamicsSettings::settings()->iconReorderTrackedIconUseDistanceBasedAnimTime))
	{
		quint32 maxDist = DimensionsGlobal::approximateDistance(QPointF(0,0),QPointF(m_qp_ownerPage->geometry().width(),m_qp_ownerPage->geometry().height()));
		//WARNING: positions must be in the same coordinate system. Most common issue here will be if icon wasn't parented to the page
		quint32 dist = qBound((quint32)1,
									DimensionsGlobal::approximateDistance(pDestCell->m_qp_pendingReorderIcon->pos(),pDestCell->position()),
							  maxDist);
		qreal dTdD = (qreal)(DynamicsSettings::settings()->iconReorderIconMoveAnimTime - DynamicsSettings::settings()->iconReorderTrackedIconMinAnimTime) / (qreal)(maxDist);
		quint32 aTime = DynamicsSettings::settings()->iconReorderIconMoveAnimTime - dTdD*(maxDist-dist);
		////qDebug() << __FUNCTION__ << ": setting animation time to " << aTime << " ms";
		pIconFinalAnimation->setDuration(aTime);
	}
	return pIconFinalAnimation;
}

//virtual
IconReorderAnimation * ReorderableIconLayout::animationForTrackedIconFinal(const QUuid& trackedIconUid)
{
	QPoint lastGridCoord;
	if (!lastTrackedPosition(trackedIconUid,lastGridCoord))
	{
		return 0;
	}

	return animationForTrackedIconFinal(trackedIconUid,lastGridCoord);
}

//virtual
bool ReorderableIconLayout::executeOpList(QList<IconOperation>& opList)
{
	//DOES NOT CHECK FOR RUNNING ANIMATIONS, UNCOMMITTED LISTS, ETC.
	// DO THOSE OUTSIDE AND ONLY CALL ME WHEN SURE IT'S OK
	if (m_qp_reorderAnimationGroup)
	{
		delete m_qp_reorderAnimationGroup;	//it better be stopped at this point or else it will
											// do commit actions, which means I violated the warning above
	}
	m_qp_reorderAnimationGroup = animationsForOpList(opList);
	initializePendingCellListFromOpList(opList);
	if (m_qp_reorderAnimationGroup)
	{
		connect(m_qp_reorderAnimationGroup,SIGNAL(finished()),
				this,SLOT(slotReorderAnimationsFinished()));
		m_qp_reorderAnimationGroup->start(QAnimationGroup::DeleteWhenStopped);
	}
	return true;
}

//virtual
quint32 ReorderableIconLayout::addIcon(IconBase * p_icon)
{
	if (p_icon == NULL)
	{
		return 0;
	}

	if (m_maxIconsPerRow == 0)
	{
		//this is a problem! general layout config error which should be handled elsewhere,
		//but it could lead to infinite row adds here, so bail on it
		return 0;
	}

	//at this point, the layout will be messed with for sure, so mark the layout as modded
	m_layoutSync = false;

	if (m_iconRows.empty())
	{
		//no rows exist. Add one, insert the cell
		IconRow * pNewRow = new IconRow(this);
		pNewRow->appendCell(new IconCell(pNewRow,p_icon,m_iconCellSize));
		m_iconRows.append(pNewRow);
		return 1;	//remember, returns row index +1
	}

	//else, grab the last row, check to see if it can handle any more icons
	IconRow * pRow = m_iconRows.last();
	if ((quint32)(pRow->m_iconList.size()) >= m_maxIconsPerRow)
	{
		//too many icons in that last row. Create a new row and append it
		pRow = new IconRow(this);
		m_iconRows.append(pRow);
	}

	//now pRow is a good row to add the icon to...do it
	pRow->appendCell(new IconCell(pRow,p_icon,m_iconCellSize));
	return (m_iconRows.size());		//size() == index of last() + 1
}

//virtual
quint32 ReorderableIconLayout::addIcon(IconCell * p_cell)
{
	if (p_cell == NULL)
	{
		return 0;
	}

	if (m_maxIconsPerRow == 0)
	{
		//this is a problem! general layout config error which should be handled elsewhere,
		//but it could lead to infinite row adds here, so bail on it
		return 0;
	}

	//at this point, the layout will be messed with for sure, so mark the layout as modded
	m_layoutSync = false;

	if (m_iconRows.empty())
	{
		//qDebug() << "\t\t---------------------------------------------> " << __FUNCTION__ << " if (m_iconRows.empty()) == true";
		//no rows exist. Add one, insert the cell
		IconRow * pNewRow = new IconRow(this);
		p_cell->m_p_layoutRowObject = pNewRow;
		pNewRow->appendCell(p_cell);
		m_iconRows.append(pNewRow);
		return 1;	//remember, returns row index +1
	}

	//else, grab the last row, check to see if it can handle any more icons
	IconRow * pRow = m_iconRows.last();
	if ((quint32)(pRow->m_iconList.size()) >= m_maxIconsPerRow)
	{
		//qDebug() << "\t\t---------------------------------------------> " << __FUNCTION__ << " if ((quint32)(pRow->m_iconList.size()) >= m_maxIconsPerRow) == true";

		//too many icons in that last row. Create a new row and append it
		pRow = new IconRow(this);
		m_iconRows.append(pRow);
	}

	//qDebug() << "\t\t---------------------------------------------> " << __FUNCTION__ << " adding to last row";

	//now pRow is a good row to add the icon to...do it
	p_cell->m_p_layoutRowObject = pRow;
	pRow->appendCell(p_cell);
	return (m_iconRows.size());		//size() == index of last() + 1
}

//virtual
QPoint ReorderableIconLayout::addEmptyCell()
{
	if (m_maxIconsPerRow == 0)
	{
		//this is a problem! general layout config error which should be handled elsewhere,
		//but it could lead to infinite row adds here, so bail on it
		return QPoint(-1,-1);
	}

	//at this point, the layout will be messed with for sure, so mark the layout as modded
	m_layoutSync = false;

	if (m_iconRows.empty())
	{
		//no rows exist. Add one, insert the cell
		IconRow * pNewRow = new IconRow(this);
		pNewRow->appendCell(new IconCell(pNewRow,0,m_iconCellSize));
		m_iconRows.append(pNewRow);
		return QPoint(0,0);
	}

	//else, grab the last row, check to see if it can handle any more icons
	IconRow * pRow = m_iconRows.last();
	if ((quint32)(pRow->m_iconList.size()) >= m_maxIconsPerRow)
	{
		//too many icons in that last row. Create a new row and append it
		pRow = new IconRow(this);
		m_iconRows.append(pRow);
	}

	//now pRow is a good row to add the icon to...do it
	pRow->appendCell(new IconCell(pRow,0,m_iconCellSize));
	return QPoint(pRow->m_iconList.size()-1,m_iconRows.size()-1);
}

//versions of the above that insert into specific locations. If gridCoordinate is invalid, then it will default to appending
// return convention is the same as for addIcon
//virtual
quint32 ReorderableIconLayout::addIconAt(IconBase * p_icon,const QPoint& gridCoordinate)
{
	if (p_icon == NULL)
	{
		return 0;
	}
	if (m_maxIconsPerRow == 0)
	{
		//this is a problem! general layout config error which should be handled elsewhere,
		//but it could lead to infinite row adds here, so bail on it
		return 0;
	}

	QPoint iconPos;
	if ((gridCoordinate.y() < 0) || (gridCoordinate.y() >= m_iconRows.size()) || (gridCoordinate.x() < 0))
	{
		return addIcon(p_icon);
	}
	if (gridCoordinate.x() >= m_iconRows[gridCoordinate.y()]->m_iconList.size())
	{
		return addIcon(p_icon);
	}

	//at this point, the layout will be messed with for sure, so mark the layout as modded
	m_layoutSync = false;

	//add the icon at the right place
	IconRow * pRow = m_iconRows[gridCoordinate.y()];
	pRow->m_iconList.insert(gridCoordinate.x(),new IconCell(pRow,p_icon,m_iconCellSize));
	return (quint32)(gridCoordinate.y()+1);
}

//virtual
quint32 ReorderableIconLayout::addIconAt(IconCell * p_cell,const QPoint& gridCoordinate)
{
	if (p_cell == NULL)
	{
		return 0;
	}
	if (m_maxIconsPerRow == 0)
	{
		//this is a problem! general layout config error which should be handled elsewhere,
		//but it could lead to infinite row adds here, so bail on it
		return 0;
	}

	QPoint iconPos;
	if ((gridCoordinate.y() < 0) || (gridCoordinate.y() >= m_iconRows.size()) || (gridCoordinate.x() < 0))
	{
		return addIcon(p_cell);
	}
	if (gridCoordinate.x() >= m_iconRows[gridCoordinate.y()]->m_iconList.size())
	{
		return addIcon(p_cell);
	}

	//at this point, the layout will be messed with for sure, so mark the layout as modded
	m_layoutSync = false;

	//add the icon at the right place
	IconRow * pRow = m_iconRows[gridCoordinate.y()];
	pRow->m_iconList.insert(gridCoordinate.x(),p_cell);
	return (quint32)(gridCoordinate.y()+1);
}

//(this one also has the same return convention as addEmptyCell, and will default to appending to the end of the layout (same as addEmptyCell)
//	if gridCoordinate is invalid)
//virtual
QPoint ReorderableIconLayout::addEmptyCellAt(const QPoint& gridCoordinate)
{
	if (m_maxIconsPerRow == 0)
	{
		//this is a problem! general layout config error which should be handled elsewhere,
		//but it could lead to infinite row adds here, so bail on it
		return QPoint(-1,-1);
	}

	QPoint iconPos;
	if ((gridCoordinate.y() < 0) || (gridCoordinate.y() >= m_iconRows.size()) || (gridCoordinate.x() < 0))
	{
		return addEmptyCell();
	}
	if (gridCoordinate.x() >= m_iconRows[gridCoordinate.y()]->m_iconList.size())
	{
		return addEmptyCell();
	}

	//at this point, the layout will be messed with for sure, so mark the layout as modded
	m_layoutSync = false;

	IconRow * pRow = m_iconRows[gridCoordinate.y()];
	pRow->appendCell(new IconCell(pRow,0,m_iconCellSize));
	return QPoint(gridCoordinate.x(),gridCoordinate.y());
}

//virtual
bool ReorderableIconLayout::removeIconCell(const QPoint& gridCoordinate)
{
	//DO NOT CALL THIS DURING REORDER
	if ((gridCoordinate.x() < 0) || (gridCoordinate.y() < 0) || (gridCoordinate.y() >= m_iconRows.size()))
	{
		//invalid coord
		return false;
	}
	//get the row
	IconRow * pRow = m_iconRows[gridCoordinate.y()];
	if (!pRow)
	{
		///ugh, horrible! go check code - something is seriously wrong!
		return false;
	}
	if (gridCoordinate.x() >= pRow->m_iconList.size())
	{
		return false;	//invalid coord
	}

	//ok, remove the cell
	pRow->removeCell(gridCoordinate.x());

	if (pRow->m_iconList.empty())
	{
		m_iconRows.removeAt(gridCoordinate.y());
		delete pRow;
	}

	//signify that the layout is dirty and must be recomputed
	m_layoutSync = false;

	return true;
}

/////////////////////////////////////////////////////// INIT FUNCTIONS ////////////////////////////
///////////////////////////////////// (see .h file ) //////////////////////////////////////////////

//static
void ReorderableIconLayout::initLayoutFromSequentialIconList(ReorderableIconLayout& layout,
												const IconList iconList)
{
	if (!(layout.m_qp_ownerPage))
	{
		//not supporting unowned layouts for now
		return;
	}

	QSize savedCellSize = layout.m_iconCellSize;

	if (layout.m_iconCellSize == QSize(0,0))
	{
		if (IconLayoutSettings::settings()->reorderablelayout_useFixedIconCellSize)
		{
			layout.m_iconCellSize = IconLayoutSettings::settings()->reorderablelayout_fixedIconCellSize;
		}
		else
		{
			//find out how much the icons need
			QSize iconMaxSize = IconBase::maxIconSize(iconList);
			quint32 cellSize = qMax((quint32)iconMaxSize.width(),(quint32)iconMaxSize.height());
			//layout.m_iconCellSize = QSize(cellSize,cellSize);	//make them square. //TODO: make this better
			if ((iconMaxSize.width() == 0) || (iconMaxSize.height() == 0))
			{
				layout.m_iconCellSize = IconLayoutSettings::settings()->reorderablelayout_fixedIconCellSize;
			}
			else
			{
				layout.m_iconCellSize = iconMaxSize;
			}
		}
	}
	if ((quint32)layout.m_iconCellSize.width() > layout.m_maxWidthForRows)
	{
		//a single icon cell is larger than the width...not supported
//		////qDebug() << __FUNCTION__ << ": A single icon cell is larger than the width ( "
//				<< layout.m_iconCellSize << " > " << layout.m_maxWidthForRows
//				<< " )...not supported, exiting";
		layout.m_iconCellSize = savedCellSize;
		return;
	}

	layout.calculateAndSetHorizontalSpaceParameters();

	/// START THE LAYOUT
	layout.m_iconRows.clear();

	for (IconListConstIter it = iconList.constBegin();
			it != iconList.constEnd();++it)
	{
		if (*it == NULL)
		{
			continue;  //oops
		}
		//take the icon
		if ((*it)->take(layout.m_qp_ownerPage))
		{
			//disable its auto paint
			(*it)->slotDisableIconAutoRepaint();
			(*it)->setParentItem(layout.m_qp_ownerPage);
			layout.addIcon(*it);
		}
	}

	if (layout.m_iconRows.isEmpty())
	{
		//no rows were created
		return;
	}

	//position layout all the rows
	layout.setLayoutRowSpacing(layout.m_interRowSpace,0);

	//the very last row needs to have at least icon cell height spacing toward its bottom, because otherwise it might be occluded due to stuff
	// rendered over the bottom of the page, like the quicklaunch. This way, the contract for layout/page will guarantee at least an extra row of space
	// so the rest of the system can factor that in and not occlude the bottom

//	layout.m_iconRows[layout.m_iconRows.size()-1]->m_spaceToLowerAdjacentRow = qMax(layout.m_iconRows[layout.m_iconRows.size()-1]->m_spaceToLowerAdjacentRow,
//			(quint32)(layout.m_iconCellSize.height()));

	//relayout the icons
	layout.relayout();
}

///protected Q_SLOTS:

//virtual
void ReorderableIconLayout::slotReorderAnimationsFinished()
{
	commitPending();
	if (m_qp_ownerPage)
	{
		m_qp_ownerPage->update();
	}
}

//virtual
void ReorderableIconLayout::slotTrackedIconReplacementAnimationFinished()
{
	//clear out the animation from the map
	IconReorderAnimation * pFinishedAnim = qobject_cast<IconReorderAnimation *>(sender());
	if (!pFinishedAnim)
	{
		Q_EMIT signalFSMTrackEnded_Trigger();
		return;
	}

	IconBase * pAnimatedIcon = pFinishedAnim->animatedIcon();
	if (pAnimatedIcon)
	{
		m_trackedIconAnimations.remove(pAnimatedIcon->uid());
		QMap<QUuid,QPoint>::iterator f = m_trackedIconLastPosition.find(pAnimatedIcon->uid());
		if (f == m_trackedIconLastPosition.end())
		{
			//already handled and commited.
			Q_EMIT signalFSMTrackEnded_Trigger();
			return;
		}
		m_trackedIconLastPosition.erase(f);
		//commit the icon to the destination cell
		IconCell * pDestCell = this->iconCellAtGridCoordinate(pFinishedAnim->destinationCellCoordinate());
		if (pDestCell)
		{
			pDestCell->commitPendingIcon();
		}
		//swap the icon back to use reorder mode graphics (it lost its decorator when it began moving), OR
		// turn it back to normal graphics if reorder mode finished in the meantime
		if (m_usingReorderModeGraphics)
		{
			switchIconToReorderGraphics(pAnimatedIcon);
		}
		else
		{
			switchIconToNormalGraphics(pAnimatedIcon);
		}
	}
	Q_EMIT signalFSMTrackEnded_Trigger();
	if (m_qp_ownerPage)
	{
		m_qp_ownerPage->update();
	}
}

//////////////// -- other layout functions /////////////////

//virtual
void ReorderableIconLayout::destroyAllRows()
{
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		delete *it;
	}
	m_iconRows.clear();
	m_anchorRow = 0;
	m_relayoutCount = 0;		//this could theoretically be preserved along with the anchor row #,
								// to make relayouts on subtle resizes less jarring. But since currently most
								// resizes are just due to rotations of the screen, it doesn't matter (nothing subtle about it)
}

//virtual
qint32 ReorderableIconLayout::calculateAndSetHorizontalSpaceParameters()
{
	// (+1 on num icons per row for space taken, because icons are positioned center-to-center usually, which means there is 1/2 icon w on the first and last icon
	// that's "outside" the width)
	m_maxIconsPerRow = IconLayoutSettings::settings()->reorderablelayout_maxIconsPerRow;
	qint32 freeSpace =  m_maxWidthForRows - m_iconCellSize.width() * (m_maxIconsPerRow+1) + m_horizIconSpacingAdjust * m_maxIconsPerRow;
//	//qDebug() << "m_maxWidthForRows: " << m_maxWidthForRows << " , m_iconCellSize.width() * (m_maxIconsPerRow+1) = " << m_iconCellSize.width() * (m_maxIconsPerRow+1) << " , freeSpace = " << freeSpace;
	qint32 spacing = ( freeSpace <= 0
			? 0
			: DimensionsGlobal::maxCellWidth(m_maxIconsPerRow-1,(quint32)qMax(0,freeSpace))
					);
//	//qDebug() << "---> spacing: " << spacing;

	quint32 maxIcons = m_maxIconsPerRow;
	bool autoAdjustPerformed = false;
	while ((spacing == 0) && (maxIcons > 0))
	{
		autoAdjustPerformed = true;
		//can't fit this many icons of this cell size
		//reduce max icons per row and try again
		--maxIcons;
		freeSpace =  m_maxWidthForRows - m_iconCellSize.width() * (maxIcons+1) + m_horizIconSpacingAdjust * m_maxIconsPerRow;
		spacing = ( freeSpace <= 0
				? 0
						: DimensionsGlobal::maxCellWidth(maxIcons-1,(quint32)qMax(0,freeSpace))
		);
	}

	int returnV = 0;

	if (maxIcons == 0)
	{
		//ran out of space. max icons = 0 will indicate an error/invalid condition to the rest of the code
		m_maxIconsPerRow = 0;
		m_horizIconSpacing = 0;
		//TODO: set proper error return val
	}
	else
	{
		m_maxIconsPerRow = maxIcons;
		m_horizIconSpacing = (quint32)spacing;
		//TODO: set proper return val
	}

	if (autoAdjustPerformed)
	{
		//		////qDebug() << __FUNCTION__ << ": auto-adjustment performed: manual adj. parameter ignored and the following applied: "
		//								"max icons per row  = " << m_maxIconsPerRow
		//								<< " , inter-icon horiz. spacing = " << m_horizIconSpacing;
	}

	return returnV;
}

//virtual
void ReorderableIconLayout::relayoutExisting()
{
	//see comments in .h file

	//qDebug() << "%&%&%&%&%&%&%& " << __FUNCTION__ << " %&%&%&%&%&%&%&";

	if (m_iconRows.empty())
	{
		return;
	}

	//grab all the icon cells into a list
	QList<IconCell *> iconCellList = iconCellsInFlowOrder();

	//nuke all the rows
	destroyAllRows();

	//Do the layout

	//calculate layout parameters now that cell size is determined
	calculateAndSetHorizontalSpaceParameters();

	for (QList<IconCell *>::iterator it = iconCellList.begin();
			it != iconCellList.end();++it)
	{
		//clear the cell's info from the last layout
		(*it)->clear();
		addIcon(*it);
	}
	//position layout all the rows
	setLayoutRowSpacing(m_interRowSpace,0);

	//relayout routine as before; at this point, that function won't know the difference between an initial layout attempt
	// and this (repeated) one
	relayout(true);

}

//////////////////////////////////////// TRACKING / REORDER STATE MACHINE (FSM) ////////////////////////////////////////////////////

//helper
static QState* createState(QString name, QState *parent=0)
{
	QState *result = new QState(parent);
	result->setObjectName(name);
	return result;
}

//statics, see .h file
const char * ReorderableIconLayout::ReorderFSMPropertyName_isConsistent = "isConsistent";

//virtual
void ReorderableIconLayout::setupReorderFSM()
{

	if (m_p_reorderFSM)
	{
		//qDebug() << __FUNCTION__ << ": attempting to setup a new reorder FSM when one already exists! Bad! ignoring...";
		return;
	}
	m_p_reorderFSM = new QStateMachine(this);
	m_p_reorderFSM->setObjectName("reorderfsm");

	m_p_fsmStateConsistent           		= createState("reorderfsmstate_consistent");
	m_p_fsmStateReorderPending     = createState("reorderfsmstate_pending");
	m_p_fsmStateTrackedFloating = createState("reorderfsmstate_floating");
	m_p_fsmStateReorderPendingAndTrackedFloating = createState("reorderfsmstate_pending-and-floating");

	QSignalTransition * pTransition = 0;
	// ------------------- STATE: reorderfsmstate_consistent -----------------------------------
	//	reorderfsmstate_consistent PROPERTIES
	m_p_fsmStateConsistent->assignProperty(m_p_reorderFSM,ReorderFSMPropertyName_isConsistent, true);
	//  reorderfsmstate_consistent TRANSITIONS
	pTransition = m_p_fsmStateConsistent->addTransition(this,SIGNAL(signalFSMTrackStarted_Trigger()),m_p_fsmStateTrackedFloating);
	connect(pTransition,SIGNAL(triggered()),this,SIGNAL(signalReorderStarted()));
	pTransition = m_p_fsmStateConsistent->addTransition(this,SIGNAL(signalFSMReorderStarted_Trigger()),m_p_fsmStateReorderPending);
	connect(pTransition,SIGNAL(triggered()),this,SIGNAL(signalReorderStarted()));
	connect(m_p_fsmStateConsistent,SIGNAL(propertiesAssigned()),this,SIGNAL(signalReorderEnded()));
	connect(m_p_fsmStateConsistent,SIGNAL(entered()),this,SLOT(dbg_reorderFSMStateEntered()));

	// ------------------- STATE: reorderfsmstate_pending -----------------------------------
	//	reorderfsmstate_pending PROPERTIES
	m_p_fsmStateReorderPending->assignProperty(m_p_reorderFSM,ReorderFSMPropertyName_isConsistent, false);
	//  reorderfsmstate_pending TRANSITIONS
	m_p_fsmStateReorderPending->addTransition(this,SIGNAL(signalFSMTrackStarted_Trigger()),m_p_fsmStateReorderPendingAndTrackedFloating);
	m_p_fsmStateReorderPending->addTransition(this,SIGNAL(signalFSMReorderEnded_Trigger()),m_p_fsmStateConsistent);
	connect(m_p_fsmStateReorderPending,SIGNAL(entered()),this,SLOT(dbg_reorderFSMStateEntered()));

	// ------------------- STATE: reorderfsmstate_floating -----------------------------------
	//	reorderfsmstate_floating PROPERTIES
	m_p_fsmStateTrackedFloating->assignProperty(m_p_reorderFSM,ReorderFSMPropertyName_isConsistent, false);
	//  reorderfsmstate_floating TRANSITIONS
	m_p_fsmStateTrackedFloating->addTransition(this,SIGNAL(signalFSMReorderStarted_Trigger()),m_p_fsmStateReorderPendingAndTrackedFloating);
	m_p_fsmStateTrackedFloating->addTransition(this,SIGNAL(signalFSMLastTrackEndedTrigger()),m_p_fsmStateConsistent);
	connect(m_p_fsmStateTrackedFloating,SIGNAL(entered()),this,SLOT(dbg_reorderFSMStateEntered()));

	// ------------------- STATE: reorderfsmstate_pending-and-floating -----------------------------------
	//	reorderfsmstate_pending-and-floating PROPERTIES
	m_p_fsmStateReorderPendingAndTrackedFloating->assignProperty(m_p_reorderFSM,ReorderFSMPropertyName_isConsistent, false);
	//  reorderfsmstate_pending-and-floating TRANSITIONS
	m_p_fsmStateReorderPendingAndTrackedFloating->addTransition(this,SIGNAL(signalFSMReorderEnded_Trigger()),m_p_fsmStateTrackedFloating);
	m_p_fsmStateReorderPendingAndTrackedFloating->addTransition(this,SIGNAL(signalFSMLastTrackEndedTrigger()),m_p_fsmStateReorderPending);
	connect(m_p_fsmStateReorderPendingAndTrackedFloating,SIGNAL(entered()),this,SLOT(dbg_reorderFSMStateEntered()));

	//TODO: HACK: TEMP: see slotTrackForIconEnded in .h
	connect(this,SIGNAL(signalFSMTrackEnded_Trigger()),
			this,SLOT(slotTrackForIconEnded()));

	m_p_reorderFSM->addState(m_p_fsmStateConsistent);
	m_p_reorderFSM->addState(m_p_fsmStateReorderPending);
	m_p_reorderFSM->addState(m_p_fsmStateTrackedFloating);
	m_p_reorderFSM->addState(m_p_fsmStateReorderPendingAndTrackedFloating);

	m_p_reorderFSM->setInitialState(m_p_fsmStateConsistent);
}

//virtual
void ReorderableIconLayout::slotTrackForIconEnded()
{
	if (m_trackedIconLastPosition.isEmpty())
	{
		//last one finished
		Q_EMIT signalFSMLastTrackEndedTrigger();
	}
}

//virtual
void ReorderableIconLayout::startReorderFSM()
{
	if (!m_p_reorderFSM)
	{
		//qDebug() << __FUNCTION__ << ": Cannot start; FSM does not exist";
		return;
	}
	if (m_p_reorderFSM->isRunning())
	{
		m_p_reorderFSM->stop();
	}
	m_p_reorderFSM->setInitialState(m_p_fsmStateConsistent);
	m_p_reorderFSM->start();
}

//virtual
void ReorderableIconLayout::stopReorderFSM()
{
	if (!m_p_reorderFSM)
	{
		//qDebug() << __FUNCTION__ << ": Cannot stop; FSM does not exist";
		return;
	}
	m_p_reorderFSM->stop();
}

//virtual
bool ReorderableIconLayout::isReorderStateConsistent() const
{
	if (!m_p_reorderFSM->isRunning())
	{
		return false;		///can't be sure of state w/o the FSM
	}
	return (m_p_reorderFSM->property(ReorderFSMPropertyName_isConsistent).toBool());
}

//virtual
void ReorderableIconLayout::switchIconsToReorderGraphics()
{
	m_usingReorderModeGraphics = true;
	for (IconRowIter it = m_iconRows.begin();
				it != m_iconRows.end();++it)
	{
		for (IconRow::IconCellListIter cit = (*it)->m_iconList.begin();
				cit != (*it)->m_iconList.end();++cit)
		{
			if (*cit)
			{
				switchIconToReorderGraphics(*cit);
			}
		}
	}
}

//virtual
void ReorderableIconLayout::switchIconsToNormalGraphics()
{
	m_usingReorderModeGraphics = false;
	for (IconRowIter it = m_iconRows.begin();
				it != m_iconRows.end();++it)
	{
		for (IconRow::IconCellListIter cit = (*it)->m_iconList.begin();
				cit != (*it)->m_iconList.end();++cit)
		{
			if (*cit)
			{
				switchIconToNormalGraphics(*cit);
			}
		}
	}
}

void ReorderableIconLayout::switchIconToReorderGraphics(IconCell * p_iconCell)
{
	if (p_iconCell->m_qp_icon)
	{
		switchIconToReorderGraphics(p_iconCell->m_qp_icon);
	}
	else if (p_iconCell->m_qp_pendingReorderIcon)
	{
		switchIconToReorderGraphics(p_iconCell->m_qp_pendingReorderIcon);
	}
}

//virtual
void ReorderableIconLayout::switchIconToNormalGraphics(IconCell * p_iconCell)
{
	if (p_iconCell->m_qp_icon)
	{
		p_iconCell->m_qp_icon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
		p_iconCell->m_qp_icon->slotChangeIconFrameVisibility(false);
	}
	else if (p_iconCell->m_qp_pendingReorderIcon)
	{
		p_iconCell->m_qp_pendingReorderIcon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
		p_iconCell->m_qp_pendingReorderIcon->slotChangeIconFrameVisibility(false);
	}
}

//virtual
void ReorderableIconLayout::switchIconToReorderGraphics(IconBase * p_icon)
{
	//check to see if the icon represents a removable app, and only if that's true, show the delete decorator
	// (which is borderline ergneqrq since it doesn't allow easy viewing of the app info)
	LauncherObject * pLauncher = LauncherObject::primaryInstance();
	IconBase * pRefIcon = p_icon->master();
	if (pLauncher)
	{
		if (pLauncher->canShowRemoveDeleteDecoratorOnIcon(pRefIcon->uid()))
		{
			p_icon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::Delete);
		}
		else
		{
			p_icon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
		}
	}
	p_icon->slotChangeIconFrameVisibility(true);
}

//virtual
void ReorderableIconLayout::switchIconToNormalGraphics(IconBase * p_icon)
{
	p_icon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
	p_icon->slotChangeIconFrameVisibility(false);
}

///////// FRIENDS ///////////////////////////////////////////

QDataStream & operator<< (QDataStream& stream, const ReorderableIconLayout& s)
{
	//TODO: unimplemented / IMPLEMENT
	return stream;
}

QDebug operator<<(QDebug dbg, const ReorderableIconLayout &s)
{
	//TODO: unimplemented / IMPLEMENT
	return dbg.space();
}

QDataStream & operator>> (QDataStream& stream, ReorderableIconLayout& s)
{
	//TODO: IMPLEMENT
	return stream;
}

//////////////// DEBUG ////////////////////////////

//virtual
void ReorderableIconLayout::dbg_reorderFSMStateEntered()
{
//	//qDebug() << "STATE: " << sender()->objectName() << " entered";
}

//virtual
bool ReorderableIconLayout::areTherePendingReorderAnimations()
{
	return !m_reorderPendingCellList.empty();
}
