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




#ifndef ICONREORDERANIMATION_H_
#define ICONREORDERANIMATION_H_

#include <QPropertyAnimation>
#include <QPoint>
#include <QPointer>

class ReorderableIconLayout;
class IconBase;

class IconReorderAnimation : public QPropertyAnimation
{
	Q_OBJECT


public:
	IconReorderAnimation(IconBase * p_icon,ReorderableIconLayout * p_layout,
						 const QPoint& srcCellCoordinate,const QPoint& destCellCoordinate,QObject * parent = 0);
	IconReorderAnimation(IconBase * p_icon,ReorderableIconLayout * p_layout,const QPoint& destCellCoordinate,QObject * parent = 0);
	virtual ~IconReorderAnimation();

	virtual IconBase * animatedIcon() const;
	virtual QPoint	destinationCellCoordinate() const;

	virtual void setAutoclip(const QRect& cRect);
	virtual void clearAutoclip();

Q_SIGNALS:

	void commitFinished();

protected:

	virtual void init();
	virtual void commit();
	virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

protected:

	QPointer<ReorderableIconLayout> m_qp_layout;		//the layout on which this animation will operate
	QPointer<IconBase>	m_qp_icon;
	bool	m_noSrc;
	QPoint	m_srcCellCoord;
	QPoint	m_destCellCoord;
	bool	m_commited;
};

#endif /* ICONREORDERANIMATION_H_ */
