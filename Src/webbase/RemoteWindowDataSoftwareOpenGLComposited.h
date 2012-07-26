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




#ifndef REMOTEWINDOWDATASOFTWAREOPENGLCOMPOSITED_H_
#define REMOTEWINDOWDATASOFTWAREOPENGLCOMPOSITED_H_

#include <PGLESContext.h>
#include <WebGLES2Context.h>

#include "RemoteWindowDataSoftware.h"
#include "NAppWindow.h"

class PGLES2Context;
class PGLESPixmap;

class RemoteWindowDataSoftwareOpenGLComposited: public RemoteWindowDataSoftware {
public:
	RemoteWindowDataSoftwareOpenGLComposited (int width, int height, bool hasAlpha, bool createIpcBuffer = true);
	virtual ~RemoteWindowDataSoftwareOpenGLComposited();
	virtual Palm::WebGLES2Context* getGLES2Context();
	virtual bool directRenderingAllowed(bool val);
	virtual bool isDirectRendering();
	virtual void translate(int x, int y);
    virtual void rotate(int degrees);
	virtual void flip();

	virtual void sendCompositedTextureUpdate(int x, int y, int w, int h);

protected:
	PGLES2Context* m_glContext;
};

namespace Palm {
	class WebGLES2Context;
	class WebView;
}

class PGLES2Context: public Palm::WebGLES2Context {
public:

	PGLES2Context(RemoteWindowDataSoftwareOpenGLComposited* data);
	virtual ~PGLES2Context();
	virtual bool initialize(Palm::WebView* view, WebGLES2Context* parent);
	virtual void enableDirectRendering(bool val);
	virtual bool isDirectRendering();
	virtual bool makeCurrent();
	virtual bool destroy();
	virtual bool swapBuffers();
    virtual void translate(int x, int y);
    virtual void getTranslation(int& x, int& y);
    virtual void getScreenWidthAndHeight(int& width, int& height);
    virtual void rotate(int degrees);
    virtual int getRotation();
	virtual void recreateTexture();
	virtual void reshape(int width, int height);
	virtual void resizeWindow(int width, int height);
	virtual PGLESPixmap* getOffscreenContentParentTextureId();
	virtual PGLESContext2D* getPGLESContext();

protected:

	virtual bool renderOffscreen();
	virtual bool renderOnScreen();
	virtual void createWindow(int width, int height);
	virtual bool setDisplay();
	virtual void setToCurrent();
	PGLESContext2D* context();
    
    Palm::WebView* m_view;
    Palm::WebGLES2Context* m_parent;
    PGLESPixmap* m_glPixmap;
    NAppWindow* m_nappWindow;
    RemoteWindowDataSoftwareOpenGLComposited* m_data;
    int m_xOffset;
    int m_yOffset;
    int m_rotationDegrees;
};
#endif /* REMOTEWINDOWDATASOFTWAREOPENGLCOMPOSITED_H_ */
