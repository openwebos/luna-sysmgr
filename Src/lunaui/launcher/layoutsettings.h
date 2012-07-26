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



#ifndef __LayoutSettings_h__
#define __LayoutSettings_h__

#include "Common.h"

#include <string>
#include <vector>
#include <set>
#include <glib.h>
#include <QtGlobal>
#include <QSizeF>
#include <QString>
#include <QPoint>
#include <QColor>

class LayoutSettings
{
public:

	QSizeF launcherSizePctScreenRelative;
	bool	autoPageSize;			//default - true; if true, then the page will size fullwidth of launcher, and
									// the height will be so it fits between the bottom of the page tab bar
									// and the bottom of the launcher (geom.bottom)
									// if false, then it will obey pageSizePctLauncherRelative
	QSizeF pageSizePctLauncherRelative;
	QSize	pageVerticalBorderActivationAreaSizePx;			//for top/bottom and sides; this is the "frame" around the page where something (e.g. and icon) hovered there for a period of time will cause an action
													// (e.g. a page horiz. pan). Expressed as pixels from edge (10 = 10px from the corresponding edge of the page)
	QSize	pageHorizontalBorderActivationAreaSizePx;

	quint32 pageTopBorderActivationTimeoutMs;			// related to the activation size; it's the amount of time to wait on the hovered area without any motion before activating.
	quint32 pageBottomBorderActivationTimeoutMs;		// (in millisecs)
	quint32 pageLeftBorderActivationTimeoutMs;
	quint32 pageRightBorderActivationTimeoutMs;

	bool	tabBarUseAbsoluteSize;
	QSizeF tabBarSizePctLauncherRelative;
	quint32	tabBarHeightAbsolute;
	quint32 tabBarTabFontSizePx;
	bool	tabBarTabFontEmbolden;
	QColor tabBarSelectedTabFontColor;
	QColor tabBarUnSelectedTabFontColor;
	qint32  tabTextVerticalPosAdjust;

	quint32 doneButtonFontSizePx;
	bool	doneButtonFontEmbolden;
	QColor 	doneButtonFontColor;
	qint32  doneButtonTextVerticalPosAdjust;

	QPoint 	doneButtonPositionAdjust;

	bool   quickLaunchBarUseAbsoluteSize;
	QSizeF quickLaunchBarSizePctScreenRelative;
	quint32  quickLaunchBarHeightAbsolute;

	//how far from the edge (left or right; that conf setting is TODO) should the button be.
	// the measurement is pixels from QL bar top right/left corner to nearest top corner of the icon
	// (e.g. QL top right, which is the default, to the icon's top right edge. This makes it icon size
	// independent). It's in abs coordinates so x: -val is to the left of the corner, +val to the right
	//	y: -val is above.., +val is below
	QPoint quickLaunchBarLauncherAccessButtonOffsetPx;

	//the offset from topLeft corner of QL. If the access button is on the left edge, then it will be
	// from the point @ (button.right,QL.top)
	//TODO: probably should allow this to be set auto (as an option), since it depends on the height of the QL and the sizes
	// of the icons that will be in the QL.
	QPoint quickLaunchItemAreaOffsetPx;

	//you still have to be careful how you set this. It will not prevent things from overlapping. Consider the sizes of your icons
	// and the potential sizes of the QL (in every screen configuration)
	quint32 quickLaunchMaxItems;

	qint32 centerUiVerticalOffset;

	quint32 searchPillWidth;
	quint32 searchPillInnerIconRightOffset;
	quint32 searchPillTopOffsetFromStatusBar;
	QPoint	searchPillInnerTextAdjust;

	QPoint iconVisibleLocationOffsetPx;			//how far away from the actual touch location should the icon be rendered.
												// +values are down and to the right, -values are up and left)

	//	this controls whether or not coordinates and sizes will be set to the nearest (sometimes next highest, sometimes next lowest)
	//	even integer; it's intended to work around rare, weird behavior in some cases,
	//  when rendering starting at an odd coordinate - this may not be needed in the future
	// Note that this may cause things painted using this adjustment to not be aligned as expected (+-1 pixel off), and may
	// run over the boundaries
	bool	openglHatesOddNumbers;

	static inline LayoutSettings*  DiUiLayoutSettings() {
		if (G_LIKELY(s_settings))
			return s_settings;

		s_settings = new LayoutSettings();
		return s_settings;
	}

	static inline LayoutSettings* settings() {
		return DiUiLayoutSettings();
	}

private:

	void load(const char* settingsFile);
	void postLoad();

	LayoutSettings();
	~LayoutSettings();

	static LayoutSettings* s_settings;
};

#endif // LayoutSettings

