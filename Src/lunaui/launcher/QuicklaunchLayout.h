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



#ifndef QUICKLAUNCHLAYOUT_H
#define QUICKLAUNCHLAYOUT_H

#include "Common.h"

/*
 * In the quicklaunch the layout has several components:
 *
 * Internal padding indicates how much space should be left from the edge of
 * the card to the icons.  Right now there are no complicated calculations about
 * what the actual padding will be (in other words automatically increase that & decrease
 * number of items in a column if the parameters don't match up).
 *
 * An item is the launcher item - it consists of an icon & label.
 *
 * The distance between the left of one item & the left of next one in the row is:
 * gItemWidth + gItemXPad
 *
 * The position of the icon within the item icon space is given by:
 * (gItemWidth - gIconWidth) / 2, (gItemIconHeight - gIconHeight) / 2
 *
 * The co-ordinates here are assumed to be 0,0 = top-left & increasing bottom-right.
 */

#include <QRect>
#include <QPoint>

namespace QuicklaunchLayout {
	struct Position {
		int x;
		int y;
	};

	struct GridPosition {
		int row;
		int column;
	};

	/** initialze the constants for this platform */
	void initConstants(int qlMaxWidth, int iconYOffset);

	void setNewQuicklaunchWidth(int width);

	int maxNumColumns();
	int maxNumColumnsAllowed();

	QRect iconBounds(int i, int numIcons);
	QRect iconBounds(const QPoint& position);

	QRect iconSlotBounds(int i, int numIcons);

	// The internal quicklaunch dimenions - they do not necessarily reflect the size on the
	// screen at any point in time.
	// TODO: These are global constants calculated at runtime - hide behind function calls.
	extern int QuicklaunchWidth;
	extern int QuicklaunchHeight;

	extern int gIconYPos; /// internal Y position of icons in the QL

	extern int gIconHeight; /// the height of the icon - cannot exceed gItemIconHeight
	extern int gIconWidth; /// the width of the icon - cannot exceed gItemIconWidth.
}

#endif // QUICKLAUNCHLAYOUT_H_
