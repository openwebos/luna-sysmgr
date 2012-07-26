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




#ifndef FPSHISTORY_H
#define FPSHISTORY_H

#include <stdio.h>
#include "CircularBuffer.h"
#include <vector>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

struct FpsHistoryElem
{
    int Fps;
    int Standard_Deviation;
};

struct FpsHistory : public CircularBuffer<FpsHistoryElem>
{
    FpsHistory()
    {
        Reset(1);
        SetSamplePeriod(200);
    }

    void SetSamplePeriod(int PeriodMs)
    {
        SamplePeriodMs = PeriodMs;
    }
    
    void    Reset(int NewSize);
    void    Dump();

    const FpsHistoryElem & Flip();

protected:
    int             SamplePeriodMs;
    int             FrameCount;
    int			LastSampleMs;

};

#endif
