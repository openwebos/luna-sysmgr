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




#include "Common.h"

#include "RemoteWindowData.h"

#if defined(HAVE_OPENGL)
#include "RemoteWindowDataOpenGLQt.h"
#if defined(HAVE_TEXTURESHARING)
#include "RemoteWindowDataOpenGLTextureShared.h"
#elif defined(OPENGLCOMPOSITED)
#include "RemoteWindowDataSoftwareOpenGLComposited.h"
#endif
#endif

#if defined(HAVE_TEXTURESHARING)
#include "RemoteWindowDataSoftwareTextureShared.h"
#else
#include "RemoteWindowDataSoftwareQt.h"
#endif

RemoteWindowData* RemoteWindowDataFactory::generate(int width, int height, bool hasAlpha)
{
	RemoteWindowData* data = 0;
#if defined(HAVE_TEXTURESHARING)
	data = new RemoteWindowDataSoftwareTextureShared(width, height, hasAlpha);
#elif defined(OPENGLCOMPOSITED)
	data = new RemoteWindowDataSoftwareOpenGLComposited(width, height, hasAlpha);
    // for now, we don't support remote OpenGL on Desktop
#elif defined(HAVE_OPENGL) && defined(DIRECT_RENDERING)
    data = new RemoteWindowDataOpenGLQt(width, height, hasAlpha);
#else
	data = new RemoteWindowDataSoftwareQt(width, height, hasAlpha);
#endif	
	if (!data->isValid()) {
		delete data;
		data = 0;
	}
		
	return data;    
}

void RemoteWindowData::setSupportsDirectRendering(bool val)
{
    m_supportsDirectRendering = val;
}

bool RemoteWindowData::supportsDirectRendering() const
{
	return m_supportsDirectRendering;
}
