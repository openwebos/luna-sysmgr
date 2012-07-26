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



#include "Common.h"

#include "QuicklaunchLayout.h"


#define MAX_ITEM_DISTANCE   200
#define MIN_ITEM_DISTANCE    5

namespace QuicklaunchLayout {
	int QuicklaunchWidth;
	int QuicklaunchHeight;

	int gIconYPos;

	int gIconHeight;
	int gIconWidth;

	static int s_maxNumColumns;

	/** initialize the constants for this platform */
	void initConstants(int qlMinWidth, int qlMinHeight)
	{
		QuicklaunchWidth = qlMinWidth;
		QuicklaunchHeight = qlMinHeight;

		gIconYPos = 20;

    	gIconHeight = 64;
		gIconWidth = 64;

		s_maxNumColumns = (QuicklaunchWidth - MIN_ITEM_DISTANCE * 4) / (gIconWidth + MIN_ITEM_DISTANCE * 2);
	}

	void setNewQuicklaunchWidth(int width)
	{
		QuicklaunchWidth = width;
	}

	int maxNumColumns()
	{
		return s_maxNumColumns;
	}

	QRect iconBounds(const QPoint& position)
	{
		return QRect(position, QSize(gIconWidth, gIconHeight));
	}


	QRect iconBounds(int i, int numIcons)
	{
		int slotWidth = QuicklaunchWidth / numIcons;
		int xOffset = 0;

		if(slotWidth > MAX_ITEM_DISTANCE) {
			slotWidth = MAX_ITEM_DISTANCE;
			xOffset = (QuicklaunchWidth - (slotWidth * numIcons)) / 2;
		}

		int itemX = xOffset + i * slotWidth + (slotWidth / 2);

		return QRect(
			itemX - gIconWidth/2,
			gIconYPos,
		    gIconWidth, gIconHeight
		);
	}

	QRect iconSlotBounds(int i, int numIcons)
	{
		int slotWidth = QuicklaunchWidth / numIcons;
		int xOffset = 0;

		if(slotWidth > MAX_ITEM_DISTANCE) {
			slotWidth = MAX_ITEM_DISTANCE;
			xOffset = (QuicklaunchWidth - (slotWidth * numIcons)) / 2;
		}

		int itemX = xOffset + i * slotWidth + (slotWidth / 2);

		QRect slot = QRect(itemX - slotWidth/2, gIconYPos, slotWidth, gIconHeight);

		if(i == 0) {
			slot.setLeft(0);
		}

		if(i == (numIcons - 1)) {
			slot.setRight(QuicklaunchWidth);
		}

		return slot;
	}
}
