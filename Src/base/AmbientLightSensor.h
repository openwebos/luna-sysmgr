/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef AMBIENTLIGHTSENSOR_H
#define AMBIENTLIGHTSENSOR_H

#include "Common.h"

#include "lunaservice.h"
#include <list>

#define ALS_INIT_SAMPLE_SIZE   10
#define ALS_SAMPLE_SIZE 	   10
#define ALS_REGION_COUNT       5

#define ALS_REGION_UNDEFINED  0
#define ALS_REGION_DARK       1
#define ALS_REGION_DIM        2
#define ALS_REGION_INDOOR     3
#define ALS_REGION_OUTDOOR    4


class AmbientLightSensor
{

public:
	AmbientLightSensor();

	virtual ~AmbientLightSensor();
    static AmbientLightSensor* instance ();

    bool update (int intensity);
    int getCurrentRegion ();

    bool start ();
    bool stop ();

    static bool controlStatus(LSHandle *sh, LSMessage *message, void *ctx);
    static bool cancelSubscription(LSHandle *sh, LSMessage *message, void *ctx);
    static bool hiddServiceNotification(LSHandle *sh, const char *serviceName, bool connected, void *ctx);


private:
    LSHandle*              m_service;
    bool                   m_alsEnabled;
    bool                   m_alsIsOn;
    int32_t                m_alsValue[ALS_SAMPLE_SIZE];
    int32_t                m_alsBorder[ALS_REGION_COUNT];
    int32_t                m_alsMargin[ALS_REGION_COUNT];
    int32_t                m_alsPointer;
    int32_t                m_alsRegion;
    int32_t                m_alsSum;
    uint32_t               m_alsLastOff;
    bool                   m_alsDisplayOn;
    int32_t                m_alsSubscriptions;
    int32_t                m_alsDisabled;
    bool                   m_alsHiddOnline;
    bool                   m_alsFastRate;
    int32_t                m_alsSampleCount;
    int32_t                m_alsCountInRegion;
    int32_t                m_alsSamplesNeeded;
    uint32_t               m_alsLastSampleTs;
    std::list<int32_t>	   m_alsSampleList;

    static AmbientLightSensor * m_instance;

    bool on();
    bool off ();

    bool updateAls (int intensity);
};

#endif /* AMBIENTLIGHTSENSOR_H */

