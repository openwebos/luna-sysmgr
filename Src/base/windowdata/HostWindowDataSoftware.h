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




#ifndef HOSTWINDOWDATASOFTWARE_H
#define HOSTWINDOWDATASOFTWARE_H

#include "Common.h"

#include "HostWindowData.h"

#include <QPixmap>
#include <PIpcBuffer.h>

class HostWindowDataSoftware : public HostWindowData
{
public:

	HostWindowDataSoftware(int key, int metaDataKey, int width, int height, bool hasAlpha);
	virtual ~HostWindowDataSoftware();

	virtual bool isValid() const { return m_ipcBuffer != 0; }
	virtual int key() const { return m_ipcBuffer->key(); }
	virtual int width() const { return m_width; }
	virtual int height() const { return m_height; }
	virtual bool hasAlpha() const { return m_hasAlpha; }
	virtual void flip();
	virtual PIpcBuffer* metaDataBuffer() const { return m_metaDataBuffer; }
	virtual void initializePixmap(QPixmap& screenPixmap) {}
	virtual QPixmap* acquirePixmap(QPixmap& screenPixmap);
	virtual void allowUpdates(bool allow) {}
	virtual void onUpdateRegion(QPixmap& screenPixmap, int x, int y, int w, int h);
	virtual void onUpdateWindowRequest();
	virtual void updateFromAppDirectRenderingLayer(int screenX, int screenY, int screenOrientation);
	virtual void onAboutToSendSyncMessage() {}

protected:

	PIpcBuffer* m_ipcBuffer;
	PIpcBuffer* m_metaDataBuffer;
	int m_width;
	int m_height;
	bool m_hasAlpha;
	bool m_dirty;

private:

	HostWindowDataSoftware(const HostWindowDataSoftware&);
	HostWindowDataSoftware& operator=(const HostWindowDataSoftware&);
};

#endif /* HOSTWINDOWDATASOFTWARE_H */
