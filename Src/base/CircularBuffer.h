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




#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_


template<typename T>
struct CircularBuffer
{
    CircularBuffer() :
        Buffer(NULL)
    {
    }
    
    CircularBuffer(int BufferSize) :
        Buffer(NULL)
    {
        Reset(BufferSize);
    }
    
    ~CircularBuffer()
    {
        if (Buffer) {
            delete [] Buffer;
        }
    }
    
    void Reset(int BufferSize = 0) 
    {
        if (BufferSize) {
            if (Buffer && BufferSize != this->BufferSize) {
                delete [] Buffer;
                Buffer = NULL;
            }
            this->BufferSize = BufferSize;
            if (!Buffer) {
                Buffer = new Sample[BufferSize];
            }
        }
        CurrentIndex = 0;
        SampleCount = 0;
    }
    
    void AddSample(const T& Item, long long int Time)
    {
	if(Buffer) {	
		if (++CurrentIndex >= BufferSize) {
        	    CurrentIndex = 0;
        	}
        	Buffer[CurrentIndex].Data = Item;
	        Buffer[CurrentIndex].Time = Time;
        	SampleCount++;
	}
    }
    
    const T& LastSample()
    {
        if (!Buffer)
		return NULL;

        return Buffer[CurrentIndex].Data;
    }
    
protected:
    struct Sample {
        T               Data;
        long long int   Time;
    };
    
    Sample* Buffer;
    int*    TimeStamp;
    int     BufferSize;
    int     SampleCount;
    int     CurrentIndex;
};

#endif /*CIRCULARBUFFER_H_*/
