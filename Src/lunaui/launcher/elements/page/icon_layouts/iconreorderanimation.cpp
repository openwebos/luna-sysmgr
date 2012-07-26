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




#include "iconreorderanimation.h"
#include "reorderableiconlayout.h"
#include "icon.h"
#include "dynamicssettings.h"

IconReorderAnimation::IconReorderAnimation(IconBase * p_icon,ReorderableIconLayout * p_layout,
						 const QPoint& srcCellCoordinate,const QPoint& destCellCoordinate,QObject * parent)
: QPropertyAnimation(p_icon,"animatePosition",parent)
, m_qp_icon(p_icon)
, m_qp_layout(p_layout)
, m_noSrc(false)
, m_srcCellCoord(srcCellCoordinate)
, m_destCellCoord(destCellCoordinate)
, m_commited(false)
{
	if (!m_qp_layout)
	{
		return;
	}

	IconCell * pDstCell = m_qp_layout->iconCellAtGridCoordinate(m_destCellCoord);
	if (pDstCell)
	{
		setEndValue(m_qp_layout->pageCoordinateFromLayoutCoordinate(pDstCell->position()));
//needs to be the EXTRAPOLATED PAGE POSITION
		//default anim values...the caller can always override them post-construction
		setDuration(DynamicsSettings::settings()->iconReorderIconMoveAnimTime);
		setEasingCurve(DynamicsSettings::settings()->iconReorderIconMoveAnimCurve);
	}
}

IconReorderAnimation::IconReorderAnimation(IconBase * p_icon,ReorderableIconLayout * p_layout,const QPoint& destCellCoordinate,QObject * parent)
: QPropertyAnimation(p_icon,"animatePosition",parent)
, m_qp_icon(p_icon)
, m_qp_layout(p_layout)
, m_noSrc(true)
, m_destCellCoord(destCellCoordinate)
, m_commited(false)
{
	if (!m_qp_layout)
	{
		return;
	}

	IconCell * pDstCell = m_qp_layout->iconCellAtGridCoordinate(m_destCellCoord);
	if (pDstCell)
	{
		setEndValue(m_qp_layout->pageCoordinateFromLayoutCoordinate(pDstCell->position()));
		//default anim values...the caller can always override them post-construction
		setDuration(DynamicsSettings::settings()->iconReorderIconMoveAnimTime);
		setEasingCurve(DynamicsSettings::settings()->iconReorderIconMoveAnimCurve);
	}
}

//virtual
IconReorderAnimation::~IconReorderAnimation()
{
	commit();
}

IconBase * IconReorderAnimation::animatedIcon() const
{
	return m_qp_icon;
}

QPoint IconReorderAnimation::destinationCellCoordinate() const
{
	return m_destCellCoord;
}

//virtual
void IconReorderAnimation::setAutoclip(const QRect& cRect)
{
	if (m_qp_icon)
	{
		m_qp_icon->setAutopaintClipRect(cRect);
	}
}

//virtual
void IconReorderAnimation::clearAutoclip()
{
	if (m_qp_icon)
	{
		m_qp_icon->clearAutopaintClipRect();
	}
}

//virtual
void IconReorderAnimation::init()
{
	//tell the layout that this item is animating
	// TODO: COMMENT: this probably isn't the perfect place for this since it mixes abstraction levels a bit...
	if ((m_qp_layout) && (!m_noSrc))
	{
		m_qp_layout->iconCellReleaseIcon(m_srcCellCoord);
	}
	//this is for the icon itself
	if (m_qp_icon)
	{
		m_qp_icon->slotEnableIconAutoRepaint();
	}
}

//virtual
void IconReorderAnimation::commit()
{
	//DO NOTHING FOR NOW.
	// (place any final things to be done to the icon here; for the cell finalization, the iconlayout's
	// "finish()" animation slot does that
	m_commited = true;
	if (m_qp_icon)
	{
		m_qp_icon->slotDisableIconAutoRepaint();
	}

	Q_EMIT commitFinished();
}

//virtual
void IconReorderAnimation::updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
	if ((oldState == QAbstractAnimation::Stopped) && (newState != QAbstractAnimation::Stopped))
	{
		//started
		init();
	}
	else if ((oldState != QAbstractAnimation::Stopped) && (newState == QAbstractAnimation::Stopped))
	{
		//stopped.
		commit();
	}
	QPropertyAnimation::updateState(newState,oldState);
}
