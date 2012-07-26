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



#ifndef __GraphicsSettings_h__
#define __GraphicsSettings_h__

#include "Common.h"

#include <QString>
#include <vector>
#include <set>
#include <glib.h>
#include <QtGlobal>
#include <QSize>

class GraphicsSettings
{
public:

	void  reload();

	quint32 totalCacheSizeLimitInBytes;
	bool	atlasPagesExemptFromSizeLimit;
	quint32 allowedScaleUpPercentage;
	quint32 allowedScaleDownPercentage;
	quint32 allowedAsymmetricScalePercentage;			//for when the pixmap is not square to begin with
	quint32 forceSquarifyUsesMaxBoundSquare;			//for when the pixmap is not square, true here causes a square of s = max(w,h)
	QString dbg_dumpFunctionsWriteableDirectory;	//for debug functions that dump stuff to disk, this is the location it goes to
	QString graphicsAssetBaseDirectory;				//the root dir where all the Dimensions UI graphics assets are located
	QString dbg_graphicsAssetBaseDirectory;			// same, but dbg

	QSize	maxPixSize;							//maximum dimensions of a single pixmap; set according to gfx hardware and sw limitations

	QSize	maxVCamPixSize;						//maximum dimensions of a virtual camera picture
	bool	useFixedVCamPixSize;
	QSize	fixedVCamPixSize;

	QSize	goggleSize;
	bool	vCamHorizontalFlip;
	bool 	vCamVerticalFlip;
	bool 	dbgSaveVcamOutput;

	QSize	jupocInnerSpacing;				//min spacing between items in a JUPOC

	static inline GraphicsSettings*  DiUiGraphicsSettings() {
		if (G_LIKELY(s_settings))
			return s_settings;

		s_settings = new GraphicsSettings();
		return s_settings;
	}

	static inline GraphicsSettings * settings() {
		return DiUiGraphicsSettings();
	}

private:
	void load(const char* settingsFile);
	void postLoad();
	GraphicsSettings();
	~GraphicsSettings();

	static GraphicsSettings* s_settings;
	QString m_settingsFile;
};

#endif // GraphicsSettings

