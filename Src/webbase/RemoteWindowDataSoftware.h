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




#ifndef REMOTEWINDOWDATASOFTWARE_H
#define REMOTEWINDOWDATASOFTWARE_H

#include <PGSurface.h>

#include "Common.h"
#include "RemoteWindowData.h"
#include "Logging.h"

class PGContext;
class PIpcBuffer;

class RemoteWindowDataSoftware : public RemoteWindowData
{
public:

	RemoteWindowDataSoftware(int width, int height, bool hasAlpha, bool createIpcBuffer=true);
	virtual ~RemoteWindowDataSoftware();

	virtual int key() const;
	virtual int width() const { return m_width; }
	virtual int height() const { return m_height; }
	virtual bool hasAlpha() const { return m_hasAlpha; }
	virtual bool needsClear() const { return false; }
	virtual void setWindowMetaDataBuffer(PIpcBuffer* metaDataBuffer);
	virtual void flip();

	virtual PGContext* renderingContext();

	virtual void beginPaint();
	virtual void endPaint(bool preserveOnFlip, const PRect& rect, bool flipBuffers = true);
	virtual void sendWindowUpdate(int x, int y, int w, int h);

	virtual bool hasDirectRendering() const;
	virtual bool directRenderingAllowed(bool val);

	virtual void resize(int newWidth, int newHeight);
	virtual void clear();
	
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

	PGContext* m_context;
	PGSurface* m_surface;
	bool m_directRendering;
	bool m_displayOpened;

	friend class WindowedWebApp;

private:

	RemoteWindowDataSoftware(const RemoteWindowDataSoftware&);
	RemoteWindowDataSoftware& operator=(const RemoteWindowDataSoftware&);
};

#endif /* REMOTEWINDOWDATASOFTWARE_H */
