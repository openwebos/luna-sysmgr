/**
 *  Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#ifndef REMOTEWINDOWDATAOPENGL_H
#define REMOTEWINDOWDATAOPENGL_H

#include "Common.h"

#include "RemoteWindowData.h"

class QRect;
class QPainter;
class QImage;
class QGLWidget;
class PGSurface;
class PGContext;
class PIpcBuffer;

class RemoteWindowDataOpenGLQt : public RemoteWindowData
{
public:

	RemoteWindowDataOpenGLQt(int width, int height, bool hasAlpha, bool createIpcBuffer=true);
	virtual ~RemoteWindowDataOpenGLQt();

	virtual int key() const;
	virtual int width() const { return m_width; }
	virtual int height() const { return m_height; }
	virtual bool hasAlpha() const { return m_hasAlpha; }
	virtual bool needsClear() const { return false; }
	virtual bool supportsPartialUpdates() const { return false; }
	virtual void setWindowMetaDataBuffer(PIpcBuffer* metaDataBuffer);
	virtual void flip();

	virtual PGContext* renderingContext() { return 0; }
	virtual QPainter* qtRenderingContext();
	virtual void beginPaint();
	virtual void endPaint(bool preserveOnFlip, const QRect& rect, bool flipBuffers = true);
	virtual void sendWindowUpdate(int x, int y, int w, int h);
	
	virtual bool hasDirectRendering() const;
	virtual bool directRenderingAllowed(bool val);

	virtual void resize(int newWidth, int newHeight);
	virtual void clear();
    QGLWidget* getWidget();
private:

	PIpcBuffer* m_ipcBuffer;
	int m_width;
	int m_height;
	bool m_hasAlpha;
    int m_pitch;

	QPainter* m_context;
    QImage* m_surface;
	bool m_directRendering;
	bool m_displayOpened;

	friend class WindowedWebApp;
private:

	RemoteWindowDataOpenGLQt(const RemoteWindowDataOpenGLQt&);
	RemoteWindowDataOpenGLQt& operator=(const RemoteWindowDataOpenGLQt&);
    void lock();
    void unlock();
    int calcPitch(int width);
};
	

#endif /* REMOTEWINDOWDATAOPENGL_H */
