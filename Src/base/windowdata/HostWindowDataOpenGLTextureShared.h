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




#ifndef HOSTWINDOWDATAOPENGLTEXTURESHARED_H
#define HOSTWINDOWDATAOPENGLTEXTURESHARED_H

#include "Common.h"

#include <HostWindowDataSoftware.h>

class PIpcBuffer;
class TextureSharedCompositingWindow;

class HostWindowDataOpenGLTextureShared : public HostWindowDataSoftware
{
public:

public:

	HostWindowDataOpenGLTextureShared(int key, int metaDataKey, int width, int height, bool hasAlpha);
	virtual ~HostWindowDataOpenGLTextureShared();

	virtual void initializePixmap(QPixmap& screenPixmap);
	virtual QPixmap* acquirePixmap(QPixmap& screenPixmap);
	virtual void allowUpdates(bool allow);

	virtual void flip();

	virtual void onUpdateRegion(QPixmap& screenPixmap, int x, int y, int w, int h);
	virtual void onUpdateWindowRequest();
	virtual void updateFromAppDirectRenderingLayer(int screenX, int screenY, int screenOrientation);
	virtual void onAboutToSendSyncMessage();
	
protected:

	void lock2();
	void unlock2();

	void releasePixmap();

	TextureSharedCompositingWindow* m_win;
	bool m_updatedAllowed;
	bool m_dirty;

	friend class TextureSharedCompositingWindow;
	
private:

	HostWindowDataOpenGLTextureShared(const HostWindowDataOpenGLTextureShared&);
	HostWindowDataOpenGLTextureShared& operator=(const HostWindowDataOpenGLTextureShared&);
};

#endif /* HOSTWINDOWDATAOPENGLTEXTURESHARED_H */
