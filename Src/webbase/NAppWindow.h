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




#ifndef NAPPWINDOW_H_
#define NAPPWINDOW_H_


#include <stdio.h>

//work-around so that we don't have to include libnapp, libhelpers, and remote-adapter-ipc in the desktop build
#if defined(HAS_NAPP)
#include <NWindow.h>
struct NAppWindow : public NWindow {
#else
struct NAppWindow {
#endif
    NAppWindow()
    {
    }

	~NAppWindow()
	{
	}
	
	virtual void Set(int Width, int Height)
	{
#if defined(HAS_NAPP)
		NWindow::Set(Width, Height);
#endif
	}

    virtual bool AttachBuffers(int NumBuffers, unsigned int Handles[])
    {
#if defined(HAS_NAPP)
		return NWindow::AttachBuffers(NumBuffers, Handles);
#else
        return true;
#endif
    }
    
    virtual void Post(int Buffer)
    {
    }

    virtual void Wait(int* Buffer)
    {
    }
};

#endif /* NAPPWINDOW_H_ */
