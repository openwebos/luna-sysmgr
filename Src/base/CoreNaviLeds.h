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




#ifndef CORENAVILEDS_H
#define CORENAVILEDS_H

#include "Common.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>

#include <nyx/nyx_client.h>


class CoreNaviLeds {
    public:
        int Left();
        int Right();
        int Center();
        int All();
        void stopAll();

        void stopAll(int led);
        //void ledBlink(int led);
        void ledFade(int led, int brightness, int cFadeIn, int cFadeOut, int sFadeIn, int sFadeOut);
        void ledPulsate(int led, int brightness, int startDelay, int FadeIn, int FadeOut, int FadeOutDelay, int RepeatDelay, int repeat);
	    void ledDoublePulse(int led, int brightness, int pulseRamp, int pulseDur, int pulseDelay, int RepeatDelay, int repeat);

        void ledFullFade(int brightness, int firstIn,int firstOut, int secondIn, int secondOut, int thirdOut, bool left);
        void ledSet (int led, int brightness);
        void ledRampTo(int led, int brightness, int time);
        void ledWaterdrop(int brightness, int cFadeIn, int cFadeOut, int sFadeIn, int sFadeOut,bool reverse = false);

        // for the light bar
        void ledLightbarSwipe (int brightness, int fadeTime, bool goLeft);
        void ledLightbarFullSwipe (int brightness, int fadeTime, bool goLeft);

        void ledSwipe (int brightness, int fadeTime, bool goLeft);
        void ledFullSwipe (int brightness, int fadeTime, bool goLeft);

        void ledSeesaw(int brightness, int fadeTime, bool reverse = false);
	void ledFadeOff (int brightness, int fadeTime, bool goLeft);

        static CoreNaviLeds* m_instance;
        static CoreNaviLeds* instance ();

        CoreNaviLeds();
        ~CoreNaviLeds();

    private:
        void configureParameters(int n, ...);
        void initializeEffect(nyx_led_controller_effect_type_t effect_type, int led);
        void finalizeAndExecute();

        nyx_led_controller_core_configuration_handle_t m_config;
        nyx_led_controller_effect_t m_effect;

        bool m_lightbarEnabled;
        nyx_device_handle_t m_device;

};

#endif//CORENAVILEDS_H
