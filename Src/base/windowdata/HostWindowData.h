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




#ifndef HOSTWINDOWDATA_H
#define HOSTWINDOWDATA_H

#include "Common.h"

#include <QPixmap>
#include "PIpcChannel.h"

class PIpcBuffer;

class HostWindowData
{
public:

	HostWindowData() {}
	virtual ~HostWindowData() {}

	virtual bool isValid() const { return true; }
	virtual int key() const = 0;
	virtual int width() const = 0;
	virtual int height() const = 0;
	virtual bool hasAlpha() const = 0;
	virtual void flip() = 0;
	virtual PIpcBuffer* metaDataBuffer() const = 0;
	virtual void initializePixmap(QPixmap& screenPixmap) = 0;
	virtual QPixmap* acquirePixmap(QPixmap& screenPixmap) = 0;
	virtual void allowUpdates(bool allow) = 0;
	virtual void onUpdateRegion(QPixmap& screenPixmap, int x, int y, int w, int h) = 0;
	virtual void onUpdateWindowRequest() = 0;
	virtual void updateFromAppDirectRenderingLayer(int screenX, int screenY, int screenOrientation) = 0;
	virtual void onAboutToSendSyncMessage() = 0;
};

class HostWindowDataFactory
{
public:

	static HostWindowData* generate(int key, int metaDataKey, int width, int height, bool hasAlpha);
};

#endif /* HOSTWINDOWDATA_H */
