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




#ifndef ICONLAYOUTSETTINGS_H_
#define ICONLAYOUTSETTINGS_H_

#include "Common.h"

#include <glib.h>
#include <QtGlobal>
#include <QSize>
#include <QColor>
#include <QPoint>

class IconLayoutSettings
{
public:

	quint32 alphabetlayout_interAlphaRowSpaceInPixels;
	quint32 alphabetlayout_intraAlphaRowSpaceInPixels;
	quint32 alphabetlayout_maxIconsPerRow;
	qint32 	alphabetlayout_iconHorizSpaceAdjustInPixels;		// 0 is no adjustment; the base value is automatically calc'd based on icons per row, width, etc.
																// negative integers move the icons closer, pos move them farther apart
	quint32 alphabetlayout_rowLeftMarginInPixels;
	quint32 alphabetlayout_rowTopMarginInPixels;

	bool	alphabetlayout_useFixedIconCellSize;
	QSize	alphabetlayout_fixedIconCellSize;

	qint32	alphabetlayout_rowDividerLeftOffsetPx;
	quint32  alphabetlayout_rowDividerTopOffsetPx;

	quint32 reorderablelayout_interRowSpaceInPixels;
	quint32 reorderablelayout_maxIconsPerRow;
	qint32 reorderablelayout_iconHorizSpaceAdjustInPixels;		// 0 is no adjustment; the base value is automatically calc'd based on icons per row, width, etc.
																// negative integers move the icons closer, pos move them farther apart

	bool	reorderablelayout_useFixedIconCellSize;
	QSize	reorderablelayout_fixedIconCellSize;
	quint32 reorderablelayout_rowLeftMarginInPixels;
	quint32 reorderablelayout_rowTopMarginInPixels;

	QString reorderablelayout_emptyPageText;
	QSize 	reorderablelayout_emptyPageTextBoxSize;
	quint32 reorderablelayout_emptyPageTextFontSizePx;
	bool	reorderablelayout_emptyPageTextFontEmbolden;
	QColor	reorderablelayout_emptyPageTextFontColor;
	QPoint	reorderablelayout_emptyPageTextOffsetFromCenter;
	QPoint	reorderablelayout_emptyPageIconOffsetFromCenter;

public:
	static IconLayoutSettings* settings() {

		if (G_UNLIKELY(s_instance == 0))
			new IconLayoutSettings;

		return s_instance;
	}

private:

	static IconLayoutSettings* s_instance;

private:

	IconLayoutSettings();
	~IconLayoutSettings();

	void readSettings(const char* filePath);

	void verify();
};


#endif /* ICONLAYOUTSETTINGS_H_ */
