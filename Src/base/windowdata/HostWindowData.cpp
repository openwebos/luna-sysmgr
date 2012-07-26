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

#include "HostWindowData.h"

#include "Settings.h"

#if defined(HAVE_OPENGL)
#include "HostWindowDataOpenGL.h"
#if defined(HAVE_TEXTURESHARING)
#include "HostWindowDataOpenGLTextureShared.h"
#endif
#endif

#include "HostWindowDataSoftware.h"

HostWindowData* HostWindowDataFactory::generate(int key, int metaDataKey, int width, int height, bool hasAlpha)
{
	HostWindowData* data = 0;

	if (Settings::LunaSettings()->forceSoftwareRendering) {
		data = new HostWindowDataSoftware(key, metaDataKey, width, height, hasAlpha);
	}
	else {	
#if defined(HAVE_OPENGL)
#if defined(HAVE_TEXTURESHARING)
		data = new HostWindowDataOpenGLTextureShared(key, metaDataKey, width, height, hasAlpha);
#else
		data = new HostWindowDataOpenGL(key, metaDataKey, width, height, hasAlpha);
#endif
#else
		data = new HostWindowDataSoftware(key, metaDataKey, width, height, hasAlpha);
#endif
	}
	
	if (!data->isValid()) {
		delete data;
		data = 0;
	}
		
	return data;	
}
