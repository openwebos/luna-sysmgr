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




#include "FpsHistory.h"
#include "Time.h"
#include <errno.h>
#include <string.h>
#include <cmath>

/**
 *  \param  NewSize Size of the buffer (0 to use current size)
**/
void FpsHistory::Reset(int NewSize)
{
    CircularBuffer<FpsHistoryElem>::Reset(NewSize);

    FrameCount = 0;
    LastSampleMs = Time::curTimeMs();
}

void FpsHistory::Dump()
{
    const char* Filename = "/tmp/luna-fps.log";
    FILE* File = fopen(Filename, "a");
    if (!File) {
        return;
    }
    
    if (SampleCount) {
        int StartIndex = (CurrentIndex + BufferSize - MIN(BufferSize , SampleCount) + 1) % BufferSize;

        // Dump from oldest to newest
        for (int i = 0; i < SampleCount && i < BufferSize; i++) {
            int Index = (StartIndex + i) % BufferSize;
            fprintf(File, "[%lld ms] Fps %d SD %d\n", Buffer[Index].Time, Buffer[Index].Data.Fps, Buffer[Index].Data.Standard_Deviation);
        }
    }
    
    fclose(File);
}
