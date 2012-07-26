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




#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

#include "StatusBar.h"

#include <QGraphicsObject>

class StatusBarItem : public QGraphicsObject
{
	Q_OBJECT

public:
	enum Alignment {
		AlignCenter = 0,  // The item's position (pos()) is the position of the CENTER of the item
		AlignRight,       // The item's position (pos()) is the position of the RIGHT EDGE of the item
		AlignLeft         // The item's position (pos()) is the position of the LEFT EDGE of the item
	};

	StatusBarItem(Alignment align = AlignCenter, bool overridesArrowOpacity = false)
	   :  m_alignment(align) {}

	~StatusBarItem() {}

	Alignment alignment() const { return m_alignment; }

Q_SIGNALS:
	void signalBoundingRectChanged();

protected:

	Alignment m_alignment;
	QRectF m_bounds;
};



#endif /* STATUSBARITEM_H */
