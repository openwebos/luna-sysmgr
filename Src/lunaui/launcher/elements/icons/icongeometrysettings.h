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




#ifndef ICONGEOMETRYSETTINGS_H_
#define ICONGEOMETRYSETTINGS_H_

#include "Common.h"

#include <glib.h>
#include <QtGlobal>
#include <QSize>
#include <QSizeF>
#include <QColor>
#include <QPoint>

class IconGeometrySettings
{
public:

	bool useAbsoluteGeom;
	QSize absoluteGeomSizePx;

	QSize decoratorEdgeOffsetPx;					// 0 is outward edge of decorator flush with outward edge of icon frame/main icon
	quint32 labelVerticalSpacingPx;								// distance from bottom of frame/main icon to label boundary start (top edge)

	bool	useAbsoluteLabelBoxGeom;
	QSize	labelBoxAbsoluteGeomSizePx;
	QSizeF	labelBoxProportionToGeom;				// [0.1,1.0] bounded...  0.5,0.5 means label bound box will be 50% width, 50% height
																// of the icon picture area bounds
	bool	useAbsoluteFrameGeom;					//the "frame" is the box icon surrounding the main icon, for
														// designating reorder mode, and on whose edges decorators are located
	QSize	frameBoxAbsoluteGeomSizePx;
	QSizeF	frameBoxProportionToGeom;

	bool	useAbsoluteMainIconGeom;
	QSize	mainIconBoxAbsoluteGeomSizePx;
	QSizeF	mainIconBoxProportionToGeom;

	bool	useAbsoluteRemoveDeleteDecoratorGeom;
	QSize	removeDeleteDecoratorBoxAbsoluteGeomSizePx;
	QSizeF	removeDeleteDecoratorBoxProportionToGeom;

	QPoint	mainIconOffsetFromGeomOriginPx;
	QPoint	frameOffsetFromGeomOriginPx;
	QPoint	feedbackOffsetFromGeomOriginPx;

	bool	useAbsoluteRemoveDeleteDecoratorOffsetFromGeomOrigin;	  	//if true, then removeDeleteDecoratorOffsetFromGeomOriginPx is used.
																				//if false, it's icon frame geom.topLeft
	QPoint	removeDeleteDecoratorOffsetFromGeomOriginPx;

	bool	useAbsoluteInstallStatusDecoratorGeom;
	QSize	installStatusDecoratorBoxAbsoluteGeomSizePx;
	QSizeF	installStatusDecoratorBoxProportionToGeom;

	bool	useAbsoluteInstallStatusDecoratorOffsetFromGeomOrigin;	  	//if true, then installStatusDecoratorOffsetFromGeomOriginPx is used.
																					//if false, it's icon frame geom.topRight
	QPoint	installStatusDecoratorOffsetFromGeomOriginPx;

	quint32 labelFontSizePx;
	QColor	labelFontColor;
	bool 	labelFontEmbolden;

	bool	useAlignmentGeom;				// "alignment" geoms are a virtual total geom, for when you want the outside
											// world to see a different icon overall cell size (i.e. the whole icon; the total geom usually)
											// then what the icon logic is using to lay out its internal components.
											// this allows effects like overlapping and interleaving at the page level but doesn't
											// disturb the icon internals. Alignment geoms are always absolute specs. It doesn't make
											// much sense to make the proportional since what they are used for requires precise control on sizes
	QSize	alignmentGeomSizePx;

public:
	static IconGeometrySettings* settings() {

		if (G_UNLIKELY(s_instance == 0))
			new IconGeometrySettings;

		return s_instance;
	}

private:

	static IconGeometrySettings* s_instance;

private:

	IconGeometrySettings();
	~IconGeometrySettings();

	void readSettings(const char* filePath);
	void verify();
};


#endif /* ICONGEOMETRYSETTINGS_H_ */
