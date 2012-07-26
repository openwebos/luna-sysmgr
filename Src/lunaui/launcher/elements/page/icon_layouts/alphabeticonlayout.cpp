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




#include "debugglobal.h"
#include "alphabeticonlayout.h"
#include "alphabetpage.h"
#include "iconlayoutsettings.h"
#include "dynamicssettings.h"
#include "gfxsettings.h"
#include "layoutitem.h"
#include "iconlayout.h"
#include "icon.h"
#include "iconheap.h"
#include "page.h"
#include "pixmapobject.h"
#include "horizontallabeleddivider.h"
#include "pixmaploader.h"

#include <QPainter>
#include <QTransform>
#include <QState>
#include <QStateMachine>
#include <QPropertyAnimation>
#include <QSignalTransition>

#define DIUI_HDIV_FILEPATH QString("list-divider.png")

IconRowAlpha::IconRowAlpha(AlphabetIconLayout * p_owner)
: IconRow(p_owner)
, m_layoutSync(false)
{
}

IconRowAlpha::IconRowAlpha(AlphabetIconLayout * p_owner,const QString& alphaDesignator)
: IconRow(p_owner)
, m_alphaDesignator(alphaDesignator)
, m_layoutSync(false)
{
}

IconRowAlpha::IconRowAlpha(AlphabetIconLayout * p_owner,const QChar& alphaDesignator)
: IconRow(p_owner)
, m_alphaDesignator(QString(alphaDesignator))
, m_layoutSync(false)
{
}

//virtual
IconRowAlpha::~IconRowAlpha()
{
}

//TODO: appendCell and addCell for IconRowAlpha are wasteful! base IconRow does unite() on geoms, but this
//		will have to do it again because it has to reposition cells according to m_horizIconSpacing


//virtual
void IconRowAlpha::appendCell(IconCell * p_cell)
{
	//no such thing for the alpha row...it's the alphabetic add
	if (!p_cell->m_qp_icon)
		return;
	IconRow::appendCell(p_cell);
	qSort(m_iconList.begin(),m_iconList.end(),iconCellLessThan);
	m_layoutSync = false;

}

//virtual
void IconRowAlpha::addCell(IconCell * p_cell,const quint32 index)
{
	//ignores the index...calls append() directly since "append" actually does an alphabetic add
	appendCell(p_cell);
}
//virtual
void IconRowAlpha::appendCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 alignToCellIndex)
{
	//see append()...
	IconRow::appendCellAndAlignVerticallyToCell(p_cell,alignToCellIndex);
	qSort(m_iconList.begin(),m_iconList.end(),iconCellLessThan);
	m_layoutSync = false;
}
//virtual
void IconRowAlpha::addCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 index,const quint32 alignToCellIndex)
{
	//see addCell()
	appendCellAndAlignVerticallyToCell(p_cell,alignToCellIndex);
}

//virtual
void IconRowAlpha::paintOffscreen(PixmapHugeObject * p_hugePmo)
{
	//paint all the icons
	for (IconCellListIter it = m_iconList.begin();
			it != m_iconList.end();++it)
	{
		(*it)->paintOffscreen(p_hugePmo);
		m_lastPaintedPixmapTargetRect |= (*it)->m_lastPaintedPixmapTargetRect;
	}
}

//static
bool IconRowAlpha::iconCellLessThan(const IconCell * p_a,const IconCell * p_b)
{
	const IconBase * p_aIcon = p_a->m_qp_icon;
	const IconBase * p_bIcon = p_b->m_qp_icon;

	if (!(p_aIcon) || !(p_bIcon))
	{
		return false;
	}
	return (p_aIcon->iconLabel().toLower() < p_bIcon->iconLabel().toLower());
}

/// THIS ONE SHOULD DO A FULL RELAYOUT,
//virtual
void IconRowAlpha::relayout(bool force)
{
	if ((!force && m_layoutSync) || (m_iconList.empty()))
	{
		return;
	}

	//in addition, the owner must be AlphabetIconLayout
	AlphabetIconLayout * pOwnerLayout = qobject_cast<AlphabetIconLayout *>(m_p_layoutObject);
	if (pOwnerLayout == 0)
	{
		return;
	}

	//else, run through all the icons and reposition them...
	//first make sure the row is sorted
	qSort(m_iconList.begin(),m_iconList.end(),iconCellLessThan);

	//redistribute icons horizontally, but make sure that the left edge is flush with the layout
	redistributeIconsHorizontally(pOwnerLayout->m_geom.left()+pOwnerLayout->m_leftMarginForRows,pOwnerLayout->m_horizIconSpacing);
	//set all the icons' centers to the same y as this row
	alignIconsVerticallyCentered(m_pos.y());

	// for relayout(), pass in true to recomputeGeometry() so that it will move this row's position according to the icons
	//	that were just relaid out
	recomputeGeometry(true);
	m_layoutSync = true;
}

QDataStream & operator<< (QDataStream& stream, const IconRowAlpha& s)
{
	stream << "\"IconRowAlpha\": { \"alphaDesignator\": " << s.m_alphaDesignator << " , "
			<< (const IconRow&)(s) << " }";
	return stream;
}

QDebug operator<<(QDebug dbg, const IconRowAlpha &s)
{
	dbg.nospace() << "\"IconRowAlpha\": { \"alphaDesignator\": " << s.m_alphaDesignator << " , "
			<< (const IconRow&)(s) << " }";
	return dbg.space();

}

QDataStream & operator>> (QDataStream& stream, IconRowAlpha& s)
{
	//TODO: IMPLEMENT
	return stream;
}

/////////////////////////////////////// AlphabetIconLayout //////////////////////////////////////////////////

AlphabetIconLayout::AlphabetIconLayout(AlphabetPage * p_owner)
: IconLayout(p_owner)
, m_layoutSync(false)
, m_relayoutCount(0)
, m_usingReorderModeGraphics(false)
, m_disabledPaint(true)
, m_maxWidthForRows(0)
, m_anchorRow(0)
, m_iconCellSize(0,0)
, m_horizIconSpacing(0)
, m_layoutSizeInPixels(0,0)
, m_minRowHeight(0)
, m_maxRowHeight(0)
, m_p_reorderFSM(0)
{
	m_maxIconsPerRow = IconLayoutSettings::settings()->alphabetlayout_maxIconsPerRow;
	m_horizIconSpacingAdjust = IconLayoutSettings::settings()->alphabetlayout_iconHorizSpaceAdjustInPixels;
	m_leftMarginForRows = IconLayoutSettings::settings()->alphabetlayout_rowLeftMarginInPixels;
	m_topMarginForRows = IconLayoutSettings::settings()->alphabetlayout_rowTopMarginInPixels;
	m_intraAlphaRowSpace = IconLayoutSettings::settings()->alphabetlayout_intraAlphaRowSpaceInPixels;
	m_interAlphaRowSpace = IconLayoutSettings::settings()->alphabetlayout_interAlphaRowSpaceInPixels;

	m_reorderEventSampleRate = DynamicsSettings::settings()->iconReorderSampleRate;

	setupReorderFSM();
	startReorderFSM();

}

//virtual
AlphabetIconLayout::~AlphabetIconLayout()
{
}

//static
void AlphabetIconLayout::initDefaultEmptyLayoutFullEnglishAlpha(AlphabetIconLayout& layout)
{
	layout.m_iconRows.clear();
	for (int i=65;i<=90;++i)
	{
		//rely on ascii codes
		layout.m_iconRows.append(new IconRowAlpha(&layout,QString(QChar((char)i))));
	}
	layout.resetLayoutRowSpacingToDefaultSettings();
}

//static
void AlphabetIconLayout::initLayoutFromSequentialIconListFullEnglishAlpha(AlphabetIconLayout& layout,
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
		if (IconLayoutSettings::settings()->alphabetlayout_useFixedIconCellSize)
		{
			layout.m_iconCellSize = IconLayoutSettings::settings()->alphabetlayout_fixedIconCellSize;
		}
		else
		{
			//find out how much the icons need
			QSize iconMaxSize = IconBase::maxIconSize(iconList);
			quint32 cellSize = qMax((quint32)iconMaxSize.width(),(quint32)iconMaxSize.height());
			//layout.m_iconCellSize = QSize(cellSize,cellSize);	//make them square. //TODO: make this better
			layout.m_iconCellSize = iconMaxSize;
		}
	}

	if ((quint32)layout.m_iconCellSize.width() > layout.m_maxWidthForRows)
	{
		//a single icon cell is larger than the width...not supported
		//qDebug() << __FUNCTION__ << ": A single icon cell is larger than the width ( "
//								<< layout.m_iconCellSize << " > " << layout.m_maxWidthForRows
//									<< " )...not supported, exiting";
		layout.m_iconCellSize = savedCellSize;
		return;
	}

	//calculate layout parameters now that cell size is determined
	layout.calculateAndSetHorizontalSpaceParameters();

	/// START THE LAYOUT
	layout.m_iconRows.clear();

	//sort the icon list by label, alphabetically
	IconList sortedIconList = iconList;
	qSort(sortedIconList.begin(),sortedIconList.end(),IconBase::iconLessThanByLabel);
	for (IconListIter it = sortedIconList.begin();
			it != sortedIconList.end();++it)
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
			layout.insertAlphabetically(*it);
		}
	}
	if (layout.m_iconRows.isEmpty())
	{
		//no rows were created
		return;
	}

	//position layout all the rows
	//TODO: stop using Uniform, and fix setLayoutRowSpacing
	layout.setLayoutRowSpacing(layout.m_intraAlphaRowSpace,layout.m_interAlphaRowSpace,0);

	//the very last row needs to have at least icon cell height spacing toward its bottom, because otherwise it might be occluded due to stuff
	// rendered over the bottom of the page, like the quicklaunch. This way, the contract for layout/page will guarantee at least an extra row of space
	// so the rest of the system can factor that in and not occlude the bottom

	layout.m_iconRows[layout.m_iconRows.size()-1]->m_spaceToLowerAdjacentRow = qMax(layout.m_iconRows[layout.m_iconRows.size()-1]->m_spaceToLowerAdjacentRow,
																			(quint32)(layout.m_iconCellSize.height()));

	//relayout the icons
	layout.relayout();
}

void AlphabetIconLayout::setLayoutUniformRowSpacing(const qreal rowSpace,const quint32 anchorRowNum)
{
	setLayoutRowSpacing(rowSpace,rowSpace,anchorRowNum);
}

//TODO: still BROKEN:
//virtual
//void AlphabetIconLayout::setLayoutRowSpacing(const qreal intraAlphaRowSpace,
//											const qreal interAlphaRowSpace,
//											const quint32 anchorRowNum)
//{
//	m_intraAlphaRowSpace = qAbs(intraAlphaRowSpace);
//	m_interAlphaRowSpace = qAbs(interAlphaRowSpace);
//
//	if (m_iconRows.size() <= 1)
//	{
//		return;	//nothing to do
//	}
//
////	qDebug() << "33 m_iconRows size = " << m_iconRows.size();
////	qDebug() << "m_iconRows: " << m_iconRows;
//
//	quint32 anchorRow = (anchorRowNum >= (quint32)m_iconRows.size() ? 0 : anchorRowNum);
//	m_anchorRow = anchorRow;
//	quint32 rowspace = 0;
//	for (int i=0;i<m_iconRows.size();i+=2)
//	{
//
//		if ((i > 0) && (i < m_iconRows.size()-1))
//		{
//			//common case, both top and bottom adjacents
////			//qDebug() << "["<<i<<"]:["<< m_iconRows[i]->m_alphaDesignator.toLower()<<"] "
////					<< ", vs ["<<i-1<<"]:["<< m_iconRows[i-1]->m_alphaDesignator.toLower()<<"]";
////			qDebug() << " CLAUSE 1a: i = " << i << "i-1: " << i-1 << " , tolower i: " <<  m_iconRows[i]->m_alphaDesignator.toLower();
//			qDebug() << m_iconRows[i]->m_alphaDesignator.toLower() << m_iconRows[i];
//			printsome(this);
//			QString sme = m_iconRows[i-1]->m_alphaDesignator;
//			qDebug() << sme << " addr: " << m_iconRows[i-1];
//			rowspace = ( (m_iconRows[i]->m_alphaDesignator.toLower() == m_iconRows[i-1]->m_alphaDesignator.toLower()) ?
//					m_intraAlphaRowSpace : m_interAlphaRowSpace);
//			m_iconRows[i]->m_spaceToUpperAdjacentRow = rowspace;
//			m_iconRows[i-1]->m_spaceToLowerAdjacentRow = rowspace;
//
////			//qDebug() << "["<<i<<"]:["<< m_iconRows[i]->m_alphaDesignator.toLower()<<"] "
////								<< ", vs ["<<i+1<<"]:["<< m_iconRows[i+1]->m_alphaDesignator.toLower()<<"]";
//
//			rowspace = ( (m_iconRows[i]->m_alphaDesignator.toLower() == m_iconRows[i+1]->m_alphaDesignator.toLower()) ?
//					m_intraAlphaRowSpace : m_interAlphaRowSpace);
////			qDebug() << " CLAUSE 1b: i = " << i << "i+1: " << i+1;
//			m_iconRows[i]->m_spaceToLowerAdjacentRow = rowspace;
//			m_iconRows[i+1]->m_spaceToUpperAdjacentRow = rowspace;
//		}
//		else if (i == 0)
//		{
//			//topmost
////			qDebug() << " CLAUSE 2: i = " << i << "i+1: " << i+1;
//			rowspace = ( (m_iconRows[0]->m_alphaDesignator.toLower() == m_iconRows[1]->m_alphaDesignator.toLower()) ?
//					m_intraAlphaRowSpace : m_interAlphaRowSpace);
//			m_iconRows[i]->m_spaceToLowerAdjacentRow = rowspace;
//			m_iconRows[i+1]->m_spaceToUpperAdjacentRow = rowspace;
//		}
//		else
//		{
//			//bottom most
////			qDebug() << " CLAUSE 3: i = " << i << "i-1: " << i-1;
//			rowspace = ( (m_iconRows[i]->m_alphaDesignator.toLower() == m_iconRows[i-1]->m_alphaDesignator.toLower()) ?
//					m_intraAlphaRowSpace : m_interAlphaRowSpace);
//			m_iconRows[i-1]->m_spaceToLowerAdjacentRow = rowspace;
//			m_iconRows[i]->m_spaceToUpperAdjacentRow = rowspace;
//		}
//	}
//}

//virtual
void AlphabetIconLayout::setLayoutRowSpacing(const qreal intraAlphaRowSpace,
											const qreal interAlphaRowSpace,
											const quint32 anchorRowNum)
{
	m_intraAlphaRowSpace = qAbs(intraAlphaRowSpace);
	m_interAlphaRowSpace = qAbs(interAlphaRowSpace);

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

	m_anchorRow = (anchorRowNum >= (quint32)m_iconRows.size() ? 0 : anchorRowNum);
	quint32 rowspaceTop = 0;
	quint32 rowspaceBottom = 0;
	quint32 rowCount=0;
	//at this point there are at least 2 rows so there is at least row[0] and row[1]
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it,++rowCount)
	{
		if ((it+1) == m_iconRows.end())
		{
			//bottom most
			rowspaceTop = ( ((*it)->m_alphaDesignator.toLower() == (*(it-1))->m_alphaDesignator.toLower()) ?
					m_intraAlphaRowSpace : m_interAlphaRowSpace);
			(*it)->m_spaceToUpperAdjacentRow = rowspaceTop;
			(*it)->m_spaceToLowerAdjacentRow = 0;
		}
		else if (it == m_iconRows.begin())
		{
			//top most
			rowspaceBottom = ( ((*it)->m_alphaDesignator.toLower() == (*(it+1))->m_alphaDesignator.toLower()) ?
					m_intraAlphaRowSpace : m_interAlphaRowSpace);
			(*it)->m_spaceToLowerAdjacentRow = rowspaceBottom;
			(*it)->m_spaceToUpperAdjacentRow = m_topMarginForRows;
		}
		else
		{
			//mid
			rowspaceTop = ( ((*it)->m_alphaDesignator.toLower() == (*(it-1))->m_alphaDesignator.toLower()) ?
					m_intraAlphaRowSpace : m_interAlphaRowSpace);
			(*it)->m_spaceToUpperAdjacentRow = rowspaceTop;
			rowspaceBottom = ( ((*it)->m_alphaDesignator.toLower() == (*(it+1))->m_alphaDesignator.toLower()) ?
					m_intraAlphaRowSpace : m_interAlphaRowSpace);
			(*it)->m_spaceToLowerAdjacentRow = rowspaceBottom;
		}
	}
}

//virtual
void AlphabetIconLayout::resetLayoutRowSpacingToDefaultSettings()
{
	m_intraAlphaRowSpace = IconLayoutSettings::settings()->alphabetlayout_intraAlphaRowSpaceInPixels;
	m_interAlphaRowSpace = IconLayoutSettings::settings()->alphabetlayout_interAlphaRowSpaceInPixels;
	setLayoutRowSpacing(m_intraAlphaRowSpace,m_interAlphaRowSpace);
}

//each returns a list of operations needed to move the OTHER icons around (icons already in the layout)
//virtual
QList<IconOperation> AlphabetIconLayout::addIconAt(quint32 row,quint32 column)
{
	return QList<IconOperation>();
}

//virtual
QList<IconOperation> AlphabetIconLayout::removeIconFrom(quint32 row,quint32 column)
{
	return QList<IconOperation>();
}

//virtual
QList<quint32> AlphabetIconLayout::rowListForAlpha(const QString& alphaDesignator)
{
	//TODO: IMPROVE - based on ordering, this can be more efficient
	QList<quint32> rlist;
	quint32 rowNum = 0;
	for (IconRowConstIter it = m_iconRows.constBegin();
			it != m_iconRows.constEnd();++it)
	{
		if ((*it)->m_alphaDesignator == alphaDesignator)
		{
			rlist.push_back(rowNum);
		}
		++rowNum;
	}
	return rlist;
}

//virtual
void AlphabetIconLayout::paint(QPainter * painter)
{
	//paint all rows, which will paint all the icons
	QTransform saveTran = painter->transform();
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paint(painter);
	}
//	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,14);
	painter->setTransform(saveTran);
}

//virtual
void AlphabetIconLayout::paint(QPainter * painter, const QRectF& sourceRect)
{
	//paint all rows, which will paint all the icons

	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		(*it)->paint(painter,sourceRect);
	}
	//paint all the dividers
	int r=0;
	for (RowDividerMapIter dit = m_rowDividers.begin();
			dit != m_rowDividers.end();++dit)
	{
		++r;
		QRectF sourceGeomIntersectArea = (*dit)->untranslateFromPosition((*dit)->positionRelativeGeometry().intersect(sourceRect));
//		//qDebug() << "hdiv " << r
//						<< " sourceRect: " << sourceRect
//						<< " hdiv geom: " << (*dit)->geometry()
//						<< " hdiv pos: " << (*dit)->pos()
//						<< " hdiv rgeom: " << (*dit)->positionRelativeGeometry()
//						<< " intersect: " << (*dit)->positionRelativeGeometry().intersect(sourceRect)
//						<< " sourceGeomIntersectArea: " << sourceGeomIntersectArea;

		if (sourceGeomIntersectArea.isEmpty())
		{
			continue;
		}
		QTransform saveTran = painter->transform();
//		//qDebug() << "hdiv " << r << " translating to " << (*dit)->pos();
		painter->translate((*dit)->pos()-sourceRect.topLeft());
//		DimensionsDebugGlobal::dbgPaintBoundingRect(painter,(*dit)->geometry(),6);
		(*dit)->paint(painter,sourceGeomIntersectArea);
		painter->setTransform(saveTran);
	}

}

//virtual
void AlphabetIconLayout::paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt)
{
	//paint all rows, which will paint all the icons

	if (renderOpt & ~(2 << IconRenderStage::NBITS))
	{
		for (IconRowIter it = m_iconRows.begin();
				it != m_iconRows.end();++it)
		{
			(*it)->paint(painter,sourceRect,renderOpt);
		}
	}
	//The render opts in this case have been joined together, with the low bits up to IconRenderStage::LAST being the icon render opts, and the bits above being the
	//	horiz labeled divider opts (this way, the same opts can apply to the other page type that doesn't have labeled dividers)
	renderOpt = renderOpt >> (IconRenderStage::NBITS + 1);
	//paint all the dividers
	if (renderOpt)
	{
		for (RowDividerMapIter dit = m_rowDividers.begin();
				dit != m_rowDividers.end();++dit)
		{
			QRectF sourceGeomIntersectArea = (*dit)->untranslateFromPosition((*dit)->positionRelativeGeometry().intersect(sourceRect));
			if (sourceGeomIntersectArea.isEmpty())
			{
				continue;
			}
			QTransform saveTran = painter->transform();
			painter->translate((*dit)->pos()-sourceRect.topLeft());
			//		DimensionsDebugGlobal::dbgPaintBoundingRect(painter,(*dit)->geometry(),6);
			(*dit)->paint(painter,sourceGeomIntersectArea,renderOpt);
			painter->setTransform(saveTran);
		}
	}
}

//virtual
void AlphabetIconLayout::paint(const QPointF& translate,QPainter * painter)
{
	if (m_disabledPaint)
		return;
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


// paintOffscreen()s are supposed to ignore m_disabledPaint
//virtual
void AlphabetIconLayout::paintOffscreen(QPainter * painter)
{
	//paint all the icons in the icon list
	for (IconRowIter it = m_iconRows.begin();
			it != m_iconRows.end();++it)
	{
		//TODO: remove class specifier when class heirarch is completed
		(*it)->IconRow::paintOffscreen(painter);
	}
}

//virtual
void AlphabetIconLayout::paintOffscreen(PixmapObject * p_pmo)
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
void AlphabetIconLayout::paintOffscreen(PixmapHugeObject * p_hugePmo)
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
void AlphabetIconLayout::enableAutoPaint()
{
	m_disabledPaint=false;
}

//virtual
void AlphabetIconLayout::disableAutoPaint()
{
	m_disabledPaint=true;
}

//virtual
void AlphabetIconLayout::relayout(bool force)
{
	if (m_layoutSync && !force)
	{
		return;
	}
	if (m_iconRows.empty())
	{
		return;
	}

	qint32 HdivXadjust = IconLayoutSettings::settings()->alphabetlayout_rowDividerLeftOffsetPx;
	qint32 HdivYadjust = IconLayoutSettings::settings()->alphabetlayout_rowDividerTopOffsetPx;

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
			//	as the topLeft corner of the overall layout geom + whatever spacing was set in row layout spacing + m_interAlphaRowSpace vertically to account for the divider that
			// will be placed there
			m_iconRows[0]->m_pos = m_geom.topLeft() - m_iconRows[0]->m_geom.topLeft()
									+ QPointF((qreal)m_leftMarginForRows,m_iconRows[0]->m_spaceToUpperAdjacentRow +(qreal)m_interAlphaRowSpace);
			m_iconRows[0]->relayout(true);
		}

		//		go through the rows, from anchor up and from anchor down, moving the rows and relaying out the icons as I go along

		//from anchor -1 and up			(i<size() due to wraparound to uint_max when --0)
		quint32 trailIdx;
		for (quint32 i=m_anchorRow-1;(i>=0) && (i< (quint32)m_iconRows.size()) && (m_anchorRow != 0);--i)
		{
			trailIdx = i+1;
			m_iconRows[i]->repositionAboveRow(*m_iconRows[trailIdx]);
			m_iconRows[i]->m_pos.setX(m_geom.left()-m_iconRows[i]->m_geom.left()+m_leftMarginForRows);
			m_iconRows[i]->relayout(true);
		}

		//from anchor +1 and down
		for (quint32 i=m_anchorRow+1;(i< (quint32)m_iconRows.size()) && (m_anchorRow != (quint32)m_iconRows.size()-1);++i)
		{
			trailIdx = i-1;
			m_iconRows[i]->repositionBelowRow(*m_iconRows[trailIdx]);
			m_iconRows[i]->m_pos.setX(m_geom.left()-m_iconRows[i]->m_geom.left()+m_leftMarginForRows);
			m_iconRows[i]->relayout(true);
		}

		//now recompute the m_geom and loop (or exit)

		m_layoutSizeInPixels =
			QSize(m_maxWidthForRows,
			(quint32)DimensionsGlobal::roundUp(
			m_iconRows[m_iconRows.size()-1]->relativeGeometry().bottom()+m_iconRows[m_iconRows.size()-1]->m_spaceToLowerAdjacentRow
										- m_iconRows[0]->relativeGeometry().top()+m_iconRows[0]->m_spaceToUpperAdjacentRow +(qreal)m_interAlphaRowSpace));
		m_geom = DimensionsGlobal::realRectAroundRealPoint(m_layoutSizeInPixels);
		//qDebug() << "end of phase " << layoutPhase << " , geom: " << m_geom;
	}

	//go through all the rows and place row dividers in the right places
	QString prevAlpha = m_iconRows[0]->m_alphaDesignator;
	//check the map for an existing divider of this alpha
	HorizontalDivider * pHdiv = m_rowDividers.value(prevAlpha);
	if (pHdiv == 0)
	{
		//insert a new one
		pHdiv = createNewDivider(prevAlpha);
		m_rowDividers.insert(prevAlpha,pHdiv);
	}
	else
	{
		//found existing but make sure it's the right size by calling a resize on it
		pHdiv->resize(horizontalDividerSize());
	}

	//place the divider m_interAlphaRowSpace/2 above the row
	// and at the same level as the first icon's left edge
	//TODO: there is a problem with the current way icon geoms (and thus row geoms) are computed. The label and the frame cause the
	//		geom to be much larger than what the icon by itself takes up. Since layouts and positioning work relative to geoms and
	// 		not visible pixels, this causes alignments, such as for the hdivs, to be wrong...since left-edge of the geom might be
	//		waaay further left than what the eye sees as the "icon" when rendered.
	//	Fix this by providing geom "hints" in the icon cell and row as to where the actual icon alignment should be
	//		(e.g. an "alignment geom")
	// Workaround for right now to get things going is to artificially bump it inwards

//	pHdiv->setPos(
//		DimensionsGlobal::realPointAsPixelPosition(QPointF(m_iconRows[0]->relativeGeometry().left()+HdivXadjust-pHdiv->geometry().left(),
//			m_iconRows[0]->relativeGeometry().top() +HdivYadjust - ((qreal)m_interAlphaRowSpace)/2.0 + pHdiv->geometry().bottom())));

//	pHdiv->setPos(
//			DimensionsGlobal::realPointAsPixelPosition(QPointF(m_geom.left()+HdivXadjust-pHdiv->geometry().left(),
//					m_iconRows[0]->relativeGeometry().top() +HdivYadjust - ((qreal)m_interAlphaRowSpace)/2.0 + pHdiv->geometry().bottom())));

	pHdiv->setPos(
				DimensionsGlobal::realPointAsPixelPosition(QPointF(m_geom.left()+HdivXadjust-pHdiv->geometry().left(),
						m_iconRows[0]->relativeGeometry().top())));

//	qDebug() << "hdiv 1 position set as " << pHdiv->pos() << " , iconrow rgeom: " << m_iconRows[0]->relativeGeometry()
//			 << " , hdiv geom: " << pHdiv->geometry() << " ===> hdiv rgeom: " << pHdiv->geometry().translated(pHdiv->pos());

	for (int ri=1;ri < m_iconRows.size();++ri)
	{
		if (m_iconRows[ri]->m_alphaDesignator == prevAlpha)
		{
			continue;
		}
		//divider needed
		prevAlpha = m_iconRows[ri]->m_alphaDesignator;
		pHdiv = m_rowDividers.value(prevAlpha);
		if (pHdiv == 0)
		{
			//insert a new one
			pHdiv = createNewDivider(prevAlpha);
			m_rowDividers.insert(prevAlpha,pHdiv);
		}
		else
		{
			//found existing but make sure it's the right size by calling a resize on it
			pHdiv->resize(horizontalDividerSize());
		}
		//place the divider m_interAlphaRowSpace/2 above the row
		pHdiv->setPos(DimensionsGlobal::realPointAsPixelPosition(QPointF(m_geom.left()+HdivXadjust-pHdiv->geometry().left(),
				m_iconRows[ri]->relativeGeometry().top() + HdivYadjust - ((qreal)m_interAlphaRowSpace)/2.0 - pHdiv->geometry().bottom())));
	}

//	{
//	int i = 0;
//	////DEBUG://// check the row layouts
//	for (IconRowIter it = m_iconRows.begin();
//			it != m_iconRows.end();++it)
//	{
//		(*it)->relayout(force);
//		qDebug() << "row layout relative geom" << i << " : " << (*it)->relativeGeometry()
//					<< " , row geom : " << (*it)->geometry()
//					<< " , position : " << (*it)->m_pos;
//		++i;
//	}
//	}
	//set the pixel size of the layout -- this is used when the layout is painted on a pixmap
	//TODO: PIXEL-ALIGN
	m_layoutSizeInPixels = m_geom.size().toSize();
//	qDebug() << "final calculated layout size : " << m_layoutSizeInPixels << " , geom: " << m_geom;
	++m_relayoutCount;
	m_layoutSync=true;
}


//virtual
void AlphabetIconLayout::destroyAllRows()
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
QList<IconCell *> AlphabetIconLayout::iconCellsInFlowOrder()
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

//virtual
qint32 AlphabetIconLayout::calculateAndSetHorizontalSpaceParameters()
{
	// (+1 on num icons per row for space taken, because icons are positioned center-to-center usually, which means there is 1/2 icon w on the first and last icon
	// that's "outside" the width)
	m_maxIconsPerRow = IconLayoutSettings::settings()->alphabetlayout_maxIconsPerRow;

	qint32 spacing = ( m_maxWidthForRows < m_iconCellSize.width() * (m_maxIconsPerRow+1)
			? 0
					: DimensionsGlobal::maxCellWidth(m_maxIconsPerRow-1,m_maxWidthForRows - m_iconCellSize.width() * (m_maxIconsPerRow+1)));
	if ((m_horizIconSpacingAdjust < 0) && (-spacing >= m_horizIconSpacingAdjust))
	{
		//cannot adjust spacing to negative value.
		//qDebug() << __FUNCTION__ << ": Warning: invalid space adjust (" << m_horizIconSpacingAdjust
		//								<< "); it would result in <= 0 inter-icon horiz. spacing (setting adj. to 0)";
		m_horizIconSpacingAdjust = 0;
	}
	else
	{
		spacing += m_horizIconSpacingAdjust;
	}
	quint32 maxIcons = m_maxIconsPerRow;
	bool autoAdjustPerformed = false;
	while ((spacing == 0) && (maxIcons > 0))
	{
		autoAdjustPerformed = true;
		//can't fit this many icons of this cell size
		//reduce max icons per row and try again
		--maxIcons;
		spacing = ( m_maxWidthForRows < m_iconCellSize.width() * (maxIcons+1)
				? 0
						: DimensionsGlobal::maxCellWidth(maxIcons-1,m_maxWidthForRows - m_iconCellSize.width() * (maxIcons+1)));
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
		//		//qDebug() << __FUNCTION__ << ": auto-adjustment performed: manual adj. parameter ignored and the following applied: "
		//								"max icons per row  = " << m_maxIconsPerRow
		//								<< " , inter-icon horiz. spacing = " << m_horizIconSpacing;
	}

	return returnV;
}

//virtual
void AlphabetIconLayout::relayoutExisting()
{
	//see comments in .h file

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
		//clear the cell's info from last layout
		(*it)->clear();
		insertAlphabetically(*it);
	}
	//position layout all the rows
	//TODO: stop using Uniform, and fix setLayoutRowSpacing
	setLayoutRowSpacing(m_intraAlphaRowSpace,m_interAlphaRowSpace,0);

	//the very last row needs to have at least icon cell height spacing toward its bottom, because otherwise it might be occluded due to stuff
	// rendered over the bottom of the page, like the quicklaunch. This way, the contract for layout/page will guarantee at least an extra row of space
	// so the rest of the system can factor that in and not occlude the bottom

	m_iconRows[m_iconRows.size()-1]->m_spaceToLowerAdjacentRow = qMax(m_iconRows[m_iconRows.size()-1]->m_spaceToLowerAdjacentRow,
			(quint32)(m_iconCellSize.height()));

	//relayout routine as before; at this point, that function won't know the difference between an initial layout attempt
	// and this (repeated) one
	relayout(true);

}

//virtual
void AlphabetIconLayout::resizeWidth(const quint32 w)
{
	if ((w == 0) || (m_maxWidthForRows == w))
	{
		return;
	}

	//all reorders must stop...NOW!
	cancelAllReorder();
	// Code COMMENT: this will call back into AlphabetPage (if that is in fact the owner; if not, and it's a different
	// page type - not currently supported - then something similar will have to be called) ...for AlphabetPage, it'll
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
void AlphabetIconLayout::setPosition(const QPointF& pos)
{
	IconLayout::setPosition(pos);
	m_layoutSync = false;
}

//virtual
void AlphabetIconLayout::setUniformCellSize(const QSize& size)
{
	m_iconCellSize = size;
	m_layoutSync = false;
}

//virtual
IconCell * AlphabetIconLayout::iconCellAtLayoutCoordinate(const QPointF& coordinate)
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
IconCell * AlphabetIconLayout::iconCellAtLayoutCoordinate(const QPointF& layoutCoordinate,QPoint& r_gridCoordinate)
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
IconCell * AlphabetIconLayout::findIconByUid(const QUuid& iconUid,QPoint& r_gridCoordinate)
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
					return *cit;
				}
			}
		}
	}
	return 0;
}

//virtual
qreal	AlphabetIconLayout::verticalDistanceToNearestUpperRow(const QPointF& layoutCoordinate)
{
	//TODO: SLOW

	//find in which icon row the layout coordinate is contained
	IconRow * pRow = 0;
	int r=0;
	IconRowIter prevIt;
	for (IconRowIter it = m_iconRows.begin(); it != m_iconRows.end();++it,++r)
	{
		if ((*it)->relativeGeometry().contains(layoutCoordinate))
		{
			pRow = *it;
			break;
		}
		prevIt = it;
	}
	if (!pRow)
	{
		return 0.0;
	}
	//first, the distance from the given point in the row to the top of the row
	QPointF interRowCoordinate = layoutCoordinate - pRow->relativeGeometry().topLeft();
	qreal vDist = interRowCoordinate.y() - pRow->relativeGeometry().top();
	if (r == 0)
	{
		//no row on top of the first one so the distance just computed is it
		return vDist;
	}
	//else, at least 1 row into the layout (downward), so prevIt is valid and points to the previous row
	return (vDist + pRow->relativeGeometry().top() - (*prevIt)->relativeGeometry().top());
}

//virtual
qreal	AlphabetIconLayout::verticalDistanceToNearestLowerRow(const QPointF& layoutCoordinate)
{
	//find in which icon row the layout coordinate is contained
	IconRow * pRow = 0;
	int r=0;
	IconRowIter nextIt;
	for (IconRowIter it = m_iconRows.begin(); it != m_iconRows.end();++it,++r)
	{
		nextIt = it+1;
		if ((*it)->relativeGeometry().contains(layoutCoordinate))
		{
			pRow = *it;
			break;
		}
	}
	if (!pRow)
	{
		return 0.0;
	}
	//first, the distance from the given point in the row to the bottom of the row
	QPointF interRowCoordinate = layoutCoordinate - pRow->relativeGeometry().topLeft();
	qreal vDist = interRowCoordinate.y() - pRow->relativeGeometry().bottom();
	if (r == m_iconRows.size()-1)
	{
		//no row on bottom of the last one so the distance just computed is it
		return vDist;
	}
	//else, at least 1 row before the end of the layout (upward), so nextIt is valid and points to the next row
	return (vDist + (*nextIt)->relativeGeometry().bottom() - pRow->relativeGeometry().bottom());
}

//virtual
quint32 AlphabetIconLayout::maximumRowHeight() const
{
	//this value is precomputed when the layout is done...so just return it
	return m_maxRowHeight;
}

//virtual
quint32 AlphabetIconLayout::minimumRowHeight() const
{
	//this value is precomputed when the layout is done...so just return it
	return m_minRowHeight;
}

//static
bool AlphabetIconLayout::iconRowLessThan(const IconRowAlpha * p_a,const IconRowAlpha * p_b)
{
	return (p_a->m_alphaDesignator < p_b->m_alphaDesignator);
}

//TODO: BROKEN!
//virtual
quint32 AlphabetIconLayout::insertAlphabetically(IconBase * p_icon)
{
	if (p_icon == NULL)
	{
		return 0;
	}
	//if the icon has no label, reject
	if (p_icon->iconLabel().isEmpty())
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

	//check the title and insert into the right alpha in the list
	// all alpha designators that are single char are UPPERCASE, so first take the char and up it
	QString iconAlpha = QString(p_icon->iconLabel().at(0).toUpper());

	//locate the right row, or create one if necessary
	quint32 row = 1;		//1 due to return req. see above comments
	for (IconRowIter it = m_iconRows.begin(); it != m_iconRows.end();++it)
	{
		//assumes icon rows are sorted in alpha order
		if ((*it)->m_alphaDesignator == iconAlpha)
		{
			//found the row...
			if ((*it)->numIcons() < m_maxIconsPerRow)
			{
				//insert icon within row
				//(remember, icon ptr ownership is not being transferred)
				(*it)->appendCell(new IconCell((*it),p_icon,m_iconCellSize));
				return 0;	//done!
			}
			//next row, this one is too full
		}
		else if ((*it)->m_alphaDesignator > iconAlpha)
		{
			//ran out of rows that existed with the right alpha ,they were all too full
			//   OR
			// didn't find any rows with the right alpha
			// since the row list is in alpha order, at this point no more with a proper alpha can be found
			//...create a new one, insert after it
			IconRowAlpha * p_newAlphaRow = new IconRowAlpha(this,iconAlpha);
			p_newAlphaRow->appendCell(new IconCell(p_newAlphaRow,p_icon,m_iconCellSize));
			m_iconRows.insert(it+1,p_newAlphaRow);
			return row+1; //done!
		}
		++row;
	}
	//ran out of rows and didn't insert. Append a new one
	IconRowAlpha * p_newAlphaRow = new IconRowAlpha(this,iconAlpha);
	p_newAlphaRow->appendCell(new IconCell(p_newAlphaRow,p_icon,m_iconCellSize));
	m_iconRows.append(p_newAlphaRow);
	return row;
}

//virtual
quint32 AlphabetIconLayout::insertAlphabetically(IconCell * p_cell)
{
	if (p_cell == NULL)
	{
		return 0;
	}

	//get the icon from the cell...
	IconBase * p_icon = p_cell->m_qp_icon;
	if (p_icon == NULL)
	{
		//no icon!
		return 0;
	}

	//if the icon has no label, reject
	if (p_icon->iconLabel().isEmpty())
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

	//check the title and insert into the right alpha in the list
	// all alpha designators that are single char are UPPERCASE, so first take the char and up it
	QString iconAlpha = QString(p_icon->iconLabel().at(0).toUpper());

	//locate the right row, or create one if necessary
	quint32 row = 1;		//1 due to return req. see above comments
	for (IconRowIter it = m_iconRows.begin(); it != m_iconRows.end();++it)
	{
		//assumes icon rows are sorted in alpha order
		if ((*it)->m_alphaDesignator == iconAlpha)
		{
			//found the row...
			if ((*it)->numIcons() < m_maxIconsPerRow)
			{
				//insert icon within row
				//(remember, icon ptr ownership is not being transferred)
				p_cell->m_p_layoutRowObject = (*it);
				(*it)->appendCell(p_cell);
				return 0;	//done!
			}
			//next row, this one is too full
		}
		else if ((*it)->m_alphaDesignator > iconAlpha)
		{
			//ran out of rows that existed with the right alpha ,they were all too full
			//   OR
			// didn't find any rows with the right alpha
			// since the row list is in alpha order, at this point no more with a proper alpha can be found
			//...create a new one, insert right in front of it   <--- BROKEN here
			IconRowAlpha * p_newAlphaRow = new IconRowAlpha(this,iconAlpha);
			p_cell->m_p_layoutRowObject = p_newAlphaRow;
			p_newAlphaRow->appendCell(p_cell);
			m_iconRows.insert(it,p_newAlphaRow);
			return row; //done!
		}
		++row;
	}
	//ran out of rows and didn't insert. Append a new one
	IconRowAlpha * p_newAlphaRow = new IconRowAlpha(this,iconAlpha);
	p_cell->m_p_layoutRowObject = p_newAlphaRow;
	p_newAlphaRow->appendCell(p_cell);
	m_iconRows.append(p_newAlphaRow);
	return row;
}

//virtual
HorizontalLabeledDivider * AlphabetIconLayout::createNewDivider(const QString& alphaLabel)
{
	//TODO: IMPROVE: !!!  Demo shortcuts
	PixmapObject * pHdivPmo =
			(PixmapObject *)PixmapObjectLoader::instance()->quickLoadThreeHorizTiled(
					GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + DIUI_HDIV_FILEPATH,
					10,10);
//	QRectF maxGeomForHDivs = DimensionsGlobal::realRectAroundRealPoint(
//			horizontalDividerSize());

	QSize divSize = horizontalDividerSize();
	return HorizontalLabeledDivider::NewHorizontalLabeledDivider(alphaLabel,divSize.width(),pHdivPmo);
}

//virtual
QSize AlphabetIconLayout::horizontalDividerSize()
{
	//TODO: IMPROVE: Demo shortcuts
//	qDebug() << "div size " << QSize(m_qp_ownerPage->geometry().width()-20,20);
	//TODO: that height = 20 doesn't actually do anything now
	return QSize(m_qp_ownerPage->geometry().width()-IconLayoutSettings::settings()->alphabetlayout_rowDividerLeftOffsetPx,20);
}

QDataStream & operator<< (QDataStream& stream, const AlphabetIconLayout& s)
{
	stream << "{ \"AlphabetIconLayout\": { \"numRows\":"
			<< s.m_iconRows.size()
			<< " , " << "\"iconCellSize\":\"" << s.m_iconCellSize << "\""
			<< " , " << "\"maxWidth\":" << s.m_maxWidthForRows
			<< " , " << "\"maxIconsPerRow\":" << s.m_maxIconsPerRow
			<< " , " << "\"geom\":\"" << s.constGeometry() << "\""
			<< " , " << "\"rows\": [ ";
	for (AlphabetIconLayout::IconRowConstIter it = s.m_iconRows.constBegin(); it != s.m_iconRows.constEnd();++it)
	{
		if (it != s.m_iconRows.constBegin())
		{
			stream << " , ";
		}
		stream << " { " << *(*it) << " } ";
	}
	stream << " ] } }";
	return stream;
}

QDebug operator<<(QDebug dbg, const AlphabetIconLayout &s)
{
	dbg.nospace() << "{ \"AlphabetIconLayout\": { \"numRows\":"
			<< s.m_iconRows.size()
			<< " , " << "\"iconCellSize\":\"" << s.m_iconCellSize << "\""
			<< " , " << "\"maxWidth\":" << s.m_maxWidthForRows
			<< " , " << "\"maxIconsPerRow\":" << s.m_maxIconsPerRow
			<< " , " << "\"geom\":\"" << s.constGeometry() << "\""
			<< " , " << "\"rows\": [ ";
	for (AlphabetIconLayout::IconRowConstIter it = s.m_iconRows.constBegin(); it != s.m_iconRows.constEnd();++it)
	{
		if (it != s.m_iconRows.constBegin())
		{
			dbg.nospace() << " , ";
		}
		dbg.nospace() << " { " << *(*it) << " } ";
	}
	dbg.nospace() << " ] } }";
	return dbg.space();
}

QDataStream & operator>> (QDataStream& stream, AlphabetIconLayout& s)
{
	//TODO: IMPLEMENT
	return stream;
}

///////////////////////////////////////   Icon Tracking ///////////////////////////
//TODO: probably should move this code within this file to better organize it for readability/searchability

// returns 0 (null) if tracking this icon isn't possible at the moment
//virtual
bool AlphabetIconLayout::startTrackingIcon(const QPointF& layoutCoordinate,QPair<QUuid,QUuid>& r_iconUids)
{
	IconCell * pCell = iconCellAtLayoutCoordinate(layoutCoordinate);

	if (!pCell)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << layoutCoordinate << " because there is no cell there";
		return false;
	}
	if (!pCell->m_qp_icon)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << layoutCoordinate << " because the cell there has no icon";
		return false;
	}
	IconBase * pIcon = pCell->m_qp_icon;
	qDebug() << " the cell @" << layoutCoordinate << "reports its position as: " << pCell->position() << " mapped to page: " << pageCoordinateFromLayoutCoordinate(pCell->position());

	//copy the icon via the icon heap
	IconBase * pIconCopy = IconHeap::iconHeap()->copyIcon(pIcon->uid());
	if (!pIconCopy)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot start tracking icon @" << layoutCoordinate << " because the icon heap could not copy it";
		return false;
	}

	r_iconUids = QPair<QUuid,QUuid>(pIcon->uid(),pIconCopy->uid());
	Q_EMIT signalFSMTrackStarted_Trigger();
	m_trackedIcons.insert(pIconCopy->uid(),QPointer<IconBase>(pIconCopy));

	return true;
}

//virtual
bool AlphabetIconLayout::trackedIconLeavingLayout(const QUuid& trackedIconUid)
{
	IconBase * pTrackedIcon = getTrackedIcon(trackedIconUid);

	if (!pTrackedIcon)
	{
		//didn't find
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot stop tracking icon (transfer) " << trackedIconUid << " because it apparently never started tracking";
		return true;
	}

	//do not animate anything; just clean it from all the structures
	QMap<QUuid,QPointer<IconBase> >::iterator fp = m_trackedIcons.find(trackedIconUid);
	if (fp != m_trackedIcons.end())
	{
		m_trackedIcons.erase(fp);
	}
	Q_EMIT signalFSMTrackEnded_Trigger();
	return true;
}

//virtual
void AlphabetIconLayout::stopTrackingIcon(const QUuid& trackedIconUid)
{
	IconBase * pTrackedIcon = getTrackedIcon(trackedIconUid);

	if (!pTrackedIcon)
	{
		//didn't find
		qDebug() << __PRETTY_FUNCTION__ << ": Cannot stop tracking icon " << trackedIconUid << " because it apparently never started tracking";
		return;
	}

	commitTracked(trackedIconUid);
}

//virtual
IconBase * AlphabetIconLayout::getTrackedIcon(const QUuid& trackedIconUid)
{
	QMap<QUuid,QPointer<IconBase> >::const_iterator f = m_trackedIcons.constFind(trackedIconUid);
	if (f != m_trackedIcons.constEnd())
	{
		return f.value();
	}
	return 0;
}

//virtual
void AlphabetIconLayout::stopTrackingAll()
{

	//must call commitTracked() via iterating over keys() because m_trackedIcons will be modified within which will inval the iterator if used here
	QList<QUuid> uidKeys = m_trackedIcons.keys();
	for (QList<QUuid>::const_iterator it = uidKeys.constBegin();
			it != uidKeys.constEnd();++it)
	{
		commitTracked(*it);
	}
}

bool AlphabetIconLayout::commitTracked(const QUuid& iconUid)
{
	QAbstractAnimation * pIconFinalAnimation = animationForTrackedIconFinal(iconUid);
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

	m_trackedIconAnimations.insert(iconUid,pIconFinalAnimation);
	pIconFinalAnimation->start(QAbstractAnimation::DeleteWhenStopped);
	return true;
}

//virtual
QAbstractAnimation * AlphabetIconLayout::animationForTrackedIconFinal(const QUuid& trackedIconUid)
{
	//get the icon...
	IconBase * pIcon = getTrackedIcon(trackedIconUid);
	if (!pIcon)
	{
		return 0;
	}
	QAbstractAnimation * pAnim = 0;
	if (DynamicsSettings::settings()->alphaIconMoveTrackedIconAnimType == IconAnimationType::Fade)
	{
		pAnim = fadeAnimationForTrackedIcon(pIcon);
	}
	//TODO: other types?

	// ---- no more types, tag it with the uid of the icon as a property and return
	if (pAnim)
	{
		pAnim->setProperty(TrackedAnimationPropertyName_iconUid,trackedIconUid.toString());
	}
	return pAnim;
}

QPropertyAnimation * AlphabetIconLayout::fadeAnimationForTrackedIcon(IconBase * p_icon)
{
	QPropertyAnimation * pAnim = new QPropertyAnimation(p_icon,"opacity");
	pAnim->setEndValue(0.0);
	pAnim->setDuration(DynamicsSettings::settings()->alphaIconMoveTrackedIconAnimTime);
	return pAnim;
}

//virtual
void AlphabetIconLayout::cancelAllReorder()
{
	//must call commitTracked() via iterating over keys() because m_trackedIconLastPosition will be modified within which will inval the iterator if used here
	QList<QUuid> uidKeys = m_trackedIcons.keys();
	for (QList<QUuid>::const_iterator it = uidKeys.constBegin();
			it != uidKeys.constEnd();++it)
	{
		commitTrackedImmediately(*it);
	}

	AlphabetPage * pAlphabetPage = qobject_cast<AlphabetPage *>(m_qp_ownerPage);
	if (pAlphabetPage)
	{
		for (QList<QUuid>::const_iterator it = uidKeys.constBegin();
				it != uidKeys.constEnd();++it)
		{
			pAlphabetPage->slotTrackedIconCancelTrack(*it);
		}
	}
}

//virtual
void AlphabetIconLayout::commitTrackedImmediately(const QUuid& iconUid)
{

	//if there is already an animation for this tracked icon (to place it to its final position), then delete the anim and commit to the cell immediately
	QMap<QUuid,QPointer<QAbstractAnimation> >::iterator f = m_trackedIconAnimations.find(iconUid);
	if (f != m_trackedIconAnimations.end())
	{
		//The delete will trigger slotTrackedIconAnimationFinished() which will actually delete the icon in the icon heap
		delete f.value();
		m_trackedIconAnimations.erase(f);
	}

	//reload tracking info from the map; this is necessary because of the uncertainty of whether or not an animation that may or may not have existed,
	// actually placed the tracked icon into its final cell location (upon being deleted, right above here). If it did, then the map entry will be erased and nothing more needs to be done
	// if it did not, then explicitly do that placement here

	QMap<QUuid,QPointer<IconBase> >::iterator fp = m_trackedIcons.find(iconUid);
	if (fp != m_trackedIcons.end())
	{
		//entry exists, so there was probably no anim for this icon; thus, it's not handled yet
		m_trackedIcons.erase(fp);
		destroyTrackedIcon(iconUid);
	}

	Q_EMIT signalFSMTrackEnded_Trigger();
	return;
}

//TODO: currently the same between Reorderable and Alphabet layouts...could be unified to the base class
//virtual
bool AlphabetIconLayout::removeIconCell(const QPoint& gridCoordinate)
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

	//signify that the layout is dirty and must be recomputed
	m_layoutSync = false;

	return true;
}

///protected Q_SLOTS:

//virtual
void AlphabetIconLayout::slotTrackedIconAnimationFinished()
{
	//clear out the animation from the map
	QAbstractAnimation * pFinishedAnim = qobject_cast<QAbstractAnimation *>(sender());
	if (!pFinishedAnim)
	{
		Q_EMIT signalFSMTrackEnded_Trigger();
		return;
	}

	//grab the icon's uid from the animation property
	QUuid trackedIconUid = QUuid(pFinishedAnim->property(TrackedAnimationPropertyName_iconUid).toString());
	//because Qt 4.7 lacks QUuid-as-QVariant typing (and I don't feel like qRegister'ing constantly to make custom types of Q-builtins)

	//get rid of the icon (it's a copy, remember?)
	IconHeap::iconHeap()->deleteIconCopy(trackedIconUid);

	Q_EMIT signalFSMTrackEnded_Trigger();
	if (m_qp_ownerPage)
	{
		m_qp_ownerPage->update();
	}
}

//virtual
void AlphabetIconLayout::destroyTrackedIcon(const QUuid& iconUid)
{
	//tell the icon heap to get rid of the icon
	IconHeap::iconHeap()->deleteIconCopy(iconUid);
}

//virtual
void AlphabetIconLayout::slotTrackForIconEnded()
{
	if (m_trackedIcons.isEmpty())
	{
		//last one finished
		Q_EMIT signalFSMLastTrackEndedTrigger();
	}
}

//virtual
void AlphabetIconLayout::dbg_reorderFSMStateEntered()
{
//	qDebug() << "STATE: " << sender()->objectName() << " entered";
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

const char * AlphabetIconLayout::TrackedAnimationPropertyName_iconUid = "iconUid";
const char * AlphabetIconLayout::ReorderFSMPropertyName_isConsistent = "isConsistent";

//virtual
void AlphabetIconLayout::setupReorderFSM()
{

	if (m_p_reorderFSM)
	{
		qDebug() << __FUNCTION__ << ": attempting to setup a new reorder FSM when one already exists! Bad! ignoring...";
		return;
	}
	m_p_reorderFSM = new QStateMachine(this);
	m_p_reorderFSM->setObjectName("reorderfsm");

	m_p_fsmStateConsistent           		= createState("reorderfsmstate_consistent");
	m_p_fsmStateTrackedFloating = createState("reorderfsmstate_floating");

	QSignalTransition * pTransition = 0;
	// ------------------- STATE: reorderfsmstate_consistent -----------------------------------

	//	reorderfsmstate_consistent PROPERTIES
	m_p_fsmStateConsistent->assignProperty(m_p_reorderFSM,ReorderFSMPropertyName_isConsistent, true);
	//  reorderfsmstate_consistent TRANSITIONS
	pTransition = m_p_fsmStateConsistent->addTransition(this,SIGNAL(signalFSMTrackStarted_Trigger()),m_p_fsmStateTrackedFloating);
	connect(pTransition,SIGNAL(triggered()),this,SIGNAL(signalReorderStarted()));
	connect(m_p_fsmStateConsistent,SIGNAL(entered()),this,SLOT(dbg_reorderFSMStateEntered()));

	// ------------------- STATE: reorderfsmstate_floating -----------------------------------

	//	reorderfsmstate_floating PROPERTIES
	m_p_fsmStateTrackedFloating->assignProperty(m_p_reorderFSM,ReorderFSMPropertyName_isConsistent, false);
	//  reorderfsmstate_floating TRANSITIONS
	pTransition = m_p_fsmStateTrackedFloating->addTransition(this,SIGNAL(signalFSMLastTrackEndedTrigger()),m_p_fsmStateConsistent);
	connect(pTransition,SIGNAL(triggered()),this,SIGNAL(signalReorderEnded()));
	connect(m_p_fsmStateTrackedFloating,SIGNAL(entered()),this,SLOT(dbg_reorderFSMStateEntered()));

	//TODO: HACK: TEMP: see slotTrackForIconEnded in .h
	connect(this,SIGNAL(signalFSMTrackEnded_Trigger()),
			this,SLOT(slotTrackForIconEnded()));

	m_p_reorderFSM->addState(m_p_fsmStateConsistent);
	m_p_reorderFSM->addState(m_p_fsmStateTrackedFloating);

	m_p_reorderFSM->setInitialState(m_p_fsmStateConsistent);
}

//virtual
void AlphabetIconLayout::startReorderFSM()
{
	if (!m_p_reorderFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot start; FSM does not exist";
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
void AlphabetIconLayout::stopReorderFSM()
{
	if (!m_p_reorderFSM)
	{
		qDebug() << __FUNCTION__ << ": Cannot stop; FSM does not exist";
		return;
	}
	m_p_reorderFSM->stop();
}

//virtual
bool AlphabetIconLayout::isReorderStateConsistent() const
{
	if (!m_p_reorderFSM->isRunning())
	{
		return false;		///can't be sure of state w/o the FSM
	}
	return (m_p_reorderFSM->property(ReorderFSMPropertyName_isConsistent).toBool());
}

//virtual
void AlphabetIconLayout::switchIconsToReorderGraphics()
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
void AlphabetIconLayout::switchIconsToNormalGraphics()
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

void AlphabetIconLayout::switchIconToReorderGraphics(IconCell * p_iconCell)
{
	if (p_iconCell->m_qp_icon)
	{
		p_iconCell->m_qp_icon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::Delete);
		p_iconCell->m_qp_icon->slotChangeIconFrameVisibility(true);
	}
	else if (p_iconCell->m_qp_pendingReorderIcon)
	{
		p_iconCell->m_qp_pendingReorderIcon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::Delete);
		p_iconCell->m_qp_pendingReorderIcon->slotChangeIconFrameVisibility(true);
	}
}

//virtual
void AlphabetIconLayout::switchIconToNormalGraphics(IconCell * p_iconCell)
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
void AlphabetIconLayout::switchIconToReorderGraphics(IconBase * p_icon)
{
	p_icon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::Delete);
	p_icon->slotChangeIconFrameVisibility(true);
}

//virtual
void AlphabetIconLayout::switchIconToNormalGraphics(IconBase * p_icon)
{
	p_icon->slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::None);
	p_icon->slotChangeIconFrameVisibility(false);
}
