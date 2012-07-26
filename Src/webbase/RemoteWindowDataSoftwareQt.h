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


#ifndef REMOTEWINDOWDATASOFTWARE_H
#define REMOTEWINDOWDATASOFTWARE_H

#include "Common.h"
#include "RemoteWindowData.h"
#include "Logging.h"

class QPainter;
class QImage;
class PIpcBuffer;

class RemoteWindowDataSoftwareQt : public RemoteWindowData
{
public:

	RemoteWindowDataSoftwareQt(int width, int height, bool hasAlpha, bool createIpcBuffer=true);
	virtual ~RemoteWindowDataSoftwareQt();

	virtual int key() const;
	virtual int width() const { return m_width; }
	virtual int height() const { return m_height; }
	virtual bool hasAlpha() const { return m_hasAlpha; }
	virtual bool needsClear() const { return false; }
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
    virtual bool supportsPartialUpdates() const { return false; }
protected:

	virtual void lock();
	virtual void unlock();
	virtual void* data();
	virtual int	 calcPitch(int width);
	
	PIpcBuffer* m_ipcBuffer;
	int m_width;
	int m_height;
	int m_pitch;
	bool m_hasAlpha;

    QPainter* m_context;
    QImage* m_surface;
	bool m_directRendering;
	bool m_displayOpened;

	friend class WindowedWebApp;

private:

	RemoteWindowDataSoftwareQt(const RemoteWindowDataSoftwareQt&);
	RemoteWindowDataSoftwareQt& operator=(const RemoteWindowDataSoftwareQt&);
};

#endif /* REMOTEWINDOWDATASOFTWARE_H */
