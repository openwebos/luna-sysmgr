/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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

#include <glib.h>

#include "RoundedCorners.h"
#include "Settings.h"
#include "QtUtils.h"

static QPixmap loadResource(const QString& path)
{
	Settings* settings = Settings::LunaSettings();
	QString prefix = qFromUtf8Stl(settings->lunaSystemResourcesPath) + "/";
	return QPixmap(prefix + path);
}

QPixmap& RoundedCorners::topLeft()
{
	static QPixmap s_topLeftCornerSurf;
	if (G_UNLIKELY(s_topLeftCornerSurf.isNull()))
		s_topLeftCornerSurf = loadResource("wm-corner-top-left.png");

	return s_topLeftCornerSurf;
}

QPixmap& RoundedCorners::topRight()
{
	static QPixmap s_topRightCornerSurf;
	if (G_UNLIKELY(s_topRightCornerSurf.isNull()))
		s_topRightCornerSurf = loadResource("wm-corner-top-right.png");

	return s_topRightCornerSurf;
}

QPixmap& RoundedCorners::bottomLeft()
{
	static QPixmap s_bottomLeftCornerSurf;
	if (G_UNLIKELY(s_bottomLeftCornerSurf.isNull()))
		s_bottomLeftCornerSurf = loadResource("wm-corner-bottom-left.png");

	return s_bottomLeftCornerSurf;
}

QPixmap& RoundedCorners::bottomRight()
{
	static QPixmap s_bottomRightCornerSurf;
	if (G_UNLIKELY(s_bottomRightCornerSurf.isNull()))
		s_bottomRightCornerSurf = loadResource("wm-corner-bottom-right.png");

	return s_bottomRightCornerSurf;
}
