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




#ifndef STATICELEMENTSETTINGS_H_
#define STATICELEMENTSETTINGS_H_

#include "Common.h"

#include <glib.h>
#include <QtGlobal>
#include <QSize>
#include <QSizeF>
#include <QColor>
#include <QPoint>

class StaticElementSettings
{
public:

	// "alignment" geoms are a virtual total geom, for when you want the outside
	// world to see a different icon overall cell size (i.e. the whole icon; the total geom usually)
	// then what the icon logic is using to lay out its internal components.
	// this allows effects like overlapping and interleaving at the page level but doesn't
	// disturb the icon internals. Alignment geoms are always absolute specs. It doesn't make
	// much sense to make the proportional since what they are used for requires precise control on sizes
	bool	horizLabeledDivider_useAlignmentGeom;
	quint32	horizLabeledDivider_alignmentHeightPx;
	quint32 horizLabeledDivider_labelToPixmapSpacingPx;
	quint32 horizLabeledDivider_labelFontSizePx;
	QColor	horizLabeledDivider_labelFontColor;
	bool 	horizLabeledDivider_labelFontEmbolden;

public:
	static StaticElementSettings* settings() {

		if (G_UNLIKELY(s_instance == 0))
			new StaticElementSettings;

		return s_instance;
	}

private:

	static StaticElementSettings* s_instance;

private:

	StaticElementSettings();
	~StaticElementSettings();

	void readSettings(const char* filePath);
	void verify();
};


#endif /* STATICELEMENTSETTINGS_H_ */
