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




#ifndef HOSTWINDOWDATAOPENGL_H
#define HOSTWINDOWDATAOPENGL_H

#include "Common.h"

#include "HostWindowDataSoftware.h"

class HostWindowDataOpenGL : public HostWindowDataSoftware
{
public:

	HostWindowDataOpenGL(int key, int metaDataKey, int width, int height, bool hasAlpha);
	virtual ~HostWindowDataOpenGL();

	virtual void initializePixmap(QPixmap& screenPixmap);
	virtual void onUpdateRegion(QPixmap& screenPixmap, int x, int y, int w, int h);
	virtual QPixmap* acquirePixmap(QPixmap& screenPixmap);
	virtual void updateFromAppDirectRenderingLayer(int screenX, int screenY,
												   int screenOrientation);
	virtual void flip();

private:

	unsigned int m_textureId;

	HostWindowDataOpenGL(const HostWindowDataOpenGL&);
	HostWindowDataOpenGL& operator=(const HostWindowDataOpenGL&);
};

#endif /* HOSTWINDOWDATAOPENGL_H */
