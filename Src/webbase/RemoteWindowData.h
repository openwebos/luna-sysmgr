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




#ifndef REMOTEWINDOWDATA_H
#define REMOTEWINDOWDATA_H

#include "Common.h"

#include <QRect>

#include "CardWebApp.h"

class PIpcChannel;
class PGContext;
class PIpcBuffer;
class QPainter;
namespace Palm {
	class WebGLES2Context;
}

class RemoteWindowData
{
public:

	RemoteWindowData() : m_channel(0)
					   , m_metaDataBuffer(0)
					   , m_supportsDirectRendering(false)
	{}

	virtual ~RemoteWindowData() {}

	virtual void setWindowMetaDataBuffer(PIpcBuffer* metaDataBuffer) = 0;

	virtual bool isValid() const { return true; }
	virtual int key() const = 0;
	virtual int width() const = 0;
	virtual int height() const = 0;
	virtual bool hasAlpha() const = 0;
	virtual bool needsClear() const = 0;
	virtual bool supportsPartialUpdates() const { return true; }

	virtual void flip() = 0;

	virtual PGContext* renderingContext() = 0;
    virtual QPainter* qtRenderingContext() { return 0; }
	virtual Palm::WebGLES2Context* getGLES2Context() { return 0; }
	virtual void translate(int x, int y) { }
	virtual void rotate(int degrees) { }
	virtual void beginPaint() = 0;
	virtual void endPaint(bool preserveOnFlip, const QRect& rect, bool flipBuffers = true) = 0;
	virtual void sendWindowUpdate(int x, int y, int w, int h) = 0;

	virtual bool hasDirectRendering() const { return false; }
	virtual bool directRenderingAllowed(bool val) = 0;

	void setChannel(PIpcChannel* channel) { m_channel = channel; }

	virtual void resize(int newWidth, int newHeight) = 0;
	virtual void clear() = 0;

	void setSupportsDirectRendering(bool val);
	bool supportsDirectRendering() const;

protected:

	PIpcChannel* m_channel;
	PIpcBuffer* m_metaDataBuffer;
	bool m_supportsDirectRendering;
};

class RemoteWindowDataFactory
{
public:

	static RemoteWindowData* generate(int width, int height, bool hasAlpha);
};

#endif /* REMOTEWINDOWDATA_H */
