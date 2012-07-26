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




#ifndef REMOTEWINDOWDATASOFTWARETEXTURESHARED_H
#define REMOTEWINDOWDATASOFTWARETEXTURESHARED_H

#include "Common.h"

#include "RemoteWindowDataSoftwareOpenGLComposited.h"

class PGSurface;
class PGContext;
class PIpcBuffer;
class NSharedWindowBuffer;

class RemoteWindowDataSoftwareTextureShared : public RemoteWindowDataSoftwareOpenGLComposited
{
public:

	RemoteWindowDataSoftwareTextureShared (int width, int height, bool hasAlpha);
	virtual ~RemoteWindowDataSoftwareTextureShared();

	virtual Palm::WebGLES2Context* getGLES2Context();
	virtual void resize(int newWidth, int newHeight);
	virtual void flip();
	virtual NSharedWindowBuffer* getSharedWindowBuffer() { return m_sharedWinBuffer; }

protected:

	virtual int key() const;
	virtual void lock();
	virtual void unlock();
	virtual void* data();
	virtual int	 calcPitch(int width);
    
	NSharedWindowBuffer* m_sharedWinBuffer;
	bool m_mustPost;
	bool m_lockedBuffer;
};

class PGLES2ContextTextureShared: public PGLES2Context {
public:
	PGLES2ContextTextureShared(RemoteWindowDataSoftwareTextureShared* data);
	virtual ~PGLES2ContextTextureShared();
	virtual bool swapBuffers();

protected:
	virtual void createWindow(int width, int height);
	virtual bool renderOffscreen();
	virtual bool renderOnScreen();
};

#endif /* REMOTEWINDOWDATASOFTWARETEXTURESHARED_H */
