/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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

#include <pthread.h>
#include <sched.h>
#include <string.h>

#include "HapticsController.h"

class HapticsControllerCastle : public HapticsController {
	public:
	HapticsControllerCastle();
	~HapticsControllerCastle();	
	virtual int cancel(int id);
	virtual int vibrate(int period,int duration);
	virtual int vibrate(const char *name);
	virtual int vibrateWithAudioFeedback(const char *name, const char *audiosample);
	virtual void cancelAll();

};

static nyx_device_handle_t d = NULL;

HapticsControllerCastle::HapticsControllerCastle()
{
    nyx_error_t error = NYX_ERROR_NONE;

    error = nyx_device_open(NYX_DEVICE_HAPTICS, "Main", &d);

    if (error != NYX_ERROR_NONE)
        g_critical("HapticsControllerCastle failed to open device successfully!");
}

HapticsControllerCastle::~HapticsControllerCastle()
{
    nyx_error_t error = NYX_ERROR_NONE;

    error = nyx_device_close(d);

    if (error != NYX_ERROR_NONE)
        g_critical("HapticsControllerCastle failed to close device successfully!");
  
}


int HapticsControllerCastle::cancel(int id)
{
    nyx_error_t error = NYX_ERROR_NONE;

    error = nyx_haptics_cancel(d, id);

    if (error != NYX_ERROR_NONE)
        g_critical("HapticsControllerCastle failed to close device successfully!");

    return 1;
}

int HapticsControllerCastle::vibrate(const char *name)
{
    nyx_error_t error = NYX_ERROR_NONE;
    nyx_haptics_configuration_t configuration;

	int index = 0;
	int id;
	if(strcmp(name,"ringtone") == 0) {
		configuration.type = NYX_HAPTICS_EFFECT_RINGTONE;
	} else if(strcmp(name,"alert") == 0) { //long
		configuration.type = NYX_HAPTICS_EFFECT_ALERT;
	} else if(strcmp(name, "notification") == 0) { //short
        configuration.type = NYX_HAPTICS_EFFECT_NOTIFICATION;
	} else if(strcmp(name, "tapdown") == 0) {
        configuration.type = NYX_HAPTICS_EFFECT_TAPDOWN;
	} else if(strcmp(name, "tapup") == 0) {
        configuration.type = NYX_HAPTICS_EFFECT_TAPUP;
	} else {
		return -1;
	}

    error = nyx_haptics_vibrate(d, configuration);

    if (error != NYX_ERROR_NONE)
        g_critical("Failed on nyx_haptics_vibrate!");
    else
    {
        error = nyx_haptics_get_effect_id(d, &id);
        if (error != NYX_ERROR_NONE)
            g_critical("Failed to obtain haptics effect id!");
        else
            return id;

    }
    return -1;
}

int HapticsControllerCastle::vibrateWithAudioFeedback(const char *name, const char *audiosample)
{
    return -1;
}

int HapticsControllerCastle::vibrate(int period,int duration)
{
	int id;
    //printf("vibrate(%d,%d)\n",period,duration);
	if(duration == 0) 
		duration = 2147483647L;

    nyx_error_t error = NYX_ERROR_NONE;
    nyx_haptics_configuration_t configuration;
    configuration.type = NYX_HAPTICS_EFFECT_UNDEFINED;
    configuration.period = period;
    configuration.duration = duration;

    error = nyx_haptics_vibrate(d, configuration);

    if (error != NYX_ERROR_NONE)
        g_critical("Failed on nyx_haptics_vibrate!");
    else
    {
        error = nyx_haptics_get_effect_id(d, &id);
        if (error != NYX_ERROR_NONE)
            g_critical("Failed to obtain haptics effect id!");
        else
            return id;

    }
    return -1;
}


void HapticsControllerCastle::cancelAll() 
{

    nyx_error_t error = NYX_ERROR_NONE;
    error = nyx_haptics_cancel_all(d);

    if (error != NYX_ERROR_NONE)
        g_critical("Failed on nyx_haptics_cancel_all!");
}
