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




#ifndef REMOTEWINDOWDATAOPENGLTEXTURESHARED_H
#define REMOTEWINDOWDATAOPENGLTEXTURESHARED_H

#include "Common.h"

#include "RemoteWindowData.h"

class PGContext;
class TextureSharedRenderingWindow;

class RemoteWindowDataOpenGLTextureShared : public RemoteWindowData
{
public:

	RemoteWindowDataOpenGLTextureShared(int width, int height, bool hasAlpha);
	virtual ~RemoteWindowDataOpenGLTextureShared();

	virtual int key() const { return m_key; }
	virtual int width() const { return m_width; }
	virtual int height() const { return m_height; }
	virtual bool hasAlpha() const { return m_hasAlpha; }
	virtual bool needsClear() const;
	virtual void flip();

	virtual PGContext* renderingContext();
	virtual void beginPaint();
	virtual void endPaint(bool preserveOnFlip, const PRect& rect, bool flipBuffers = true);
	virtual void sendWindowUpdate(int x, int y, int w, int h);

	virtual bool hasDirectRendering() const { return true; }
	virtual bool directRenderingAllowed(bool val);
	
private:

	int m_key;
	int m_width;
	int m_height;
	bool m_hasAlpha;

	PGContext* m_context;
	TextureSharedRenderingWindow* m_win;
	bool m_directRendering;

private:

	friend class TextureSharedRenderingWindow;

	RemoteWindowDataOpenGLTextureShared(const RemoteWindowDataOpenGLTextureShared&);
	RemoteWindowDataOpenGLTextureShared& operator=(const RemoteWindowDataOpenGLTextureShared&);
};

#endif /* REMOTEWINDOWDATAOPENGLTEXTURESHARED_H */
