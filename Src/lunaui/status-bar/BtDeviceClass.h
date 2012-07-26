/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef BTDEVICECLASS_H
#define BTDEVICECLASS_H


// Bluetooth COD value definifions
#define COD_SERVICE_MASK                    	0xffe000
#define COD_MAJOR_MASK    						0x001f00
#define COD_MINOR_MASK    						0x0000fc

// Major Service classes
#define COD_LIMITED_DISCOVERABLE_MODE    		0x00002000
#define COD_NETWORKING    						0x00020000
#define COD_RENDERING    						0x00040000
#define COD_CAPTURING    						0x00080000
#define COD_OBJECT_TRANSFER    					0x00100000
#define COD_AUDIO    							0x00200000
#define COD_TELEPHONY    						0x00400000
#define COD_INFORMATION    						0x00800000
#define COD_SERVICE_ANY    						0x00ffE000

// Major device classes
#define COD_MAJOR_MISC    						0x00000000
#define COD_MAJOR_COMPUTER    					0x00000100
#define COD_MAJOR_PHONE    						0x00000200
#define COD_MAJOR_LAN           				0x00000300
#define COD_MAJOR_AUDIO    						0x00000400
#define COD_MAJOR_PERIPHERAL    				0x00000500
#define COD_MAJOR_IMAGING    				    0x00000600
#define COD_MAJOR_WEARABLE    				    0x00000700
#define COD_MAJOR_TOY    				        0x00000800
#define COD_MAJOR_UNCLASSIFIED    				0x00001F00
#define COD_MAJOR_ANY    						0x00001F00
#define COD_MINOR_ANY    						0x000000FC

// Minor Device Class - Computer Minor class
#define COD_MINOR_COMP_UNCLASSIFIED    			0x00000000
#define COD_MINOR_COMP_DESKTOP    				0x00000004
#define COD_MINOR_COMP_SERVER    				0x00000008
#define COD_MINOR_COMP_LAPTOP    				0x0000000C
#define COD_MINOR_COMP_HANDHELD    				0x00000010
#define COD_MINOR_COMP_PALM    					0x00000014

// Minor Device Class - Phone Minor class
#define COD_MINOR_PHONE_UNCLASSIFIED    		0x00000000
#define COD_MINOR_PHONE_CELLULAR    			0x00000004
#define COD_MINOR_PHONE_CORDLESS    			0x00000008
#define COD_MINOR_PHONE_SMART    				0x0000000C
#define COD_MINOR_PHONE_MODEM    				0x00000010
#define COD_MINOR_PHONE_ISDN    				0x00000014

// Minor Device Class - Audio Major class
#define COD_MINOR_AUDIO_UNCLASSIFIED    		0x00000000
#define COD_MINOR_AUDIO_HEADSET    				0x00000004
#define COD_MINOR_AUDIO_HANDFREE    			0x00000008
#define COD_MINOR_AUDIO_MICROPHONE    			0x00000010
#define COD_MINOR_AUDIO_LOUDSPEAKER    			0x00000014
#define COD_MINOR_AUDIO_HEADPHONE    			0x00000018
#define COD_MINOR_AUDIO_PORTABLE_AUDIO    		0x0000001C
#define COD_MINOR_AUDIO_CAR_AUDIO    			0x00000020
#define COD_MINOR_AUDIO_SET_TOP_BOX    			0x00000024
#define COD_MINOR_AUDIO_HIFI_AUDIO    			0x00000028
#define COD_MINOR_AUDIO_VCR    					0x0000002C
#define COD_MINOR_AUDIO_VIDEO_CAMERA    		0x00000030
#define COD_MINOR_AUDIO_CAMCORDER    			0x00000034
#define COD_MINOR_AUDIO_VIDEO_MONITOR    		0x00000038
#define COD_MINOR_AUDIO_VIDEO_DISPLAY    		0x0000003C
#define COD_MINOR_AUDIO_VIDEO_CONFERENCE    	0x00000040
#define COD_MINOR_AUDIO_GAME_TOY    			0x00000048

// Minor Device Class - Peripheral Major class
#define COD_MINOR_PERIPHERAL_UNCLASSIFIED    	0x00000000
#define COD_MINOR_PERIPHERAL_KEYBOARD    		0x00000040
#define COD_MINOR_PERIPHERAL_MOUSE       		0x00000080
#define COD_MINOR_PERIPHERAL_COMBO    			0x000000B0

// Minor sub Device Class - Peripheral Major class
#define COD_MINOR_PERIPHERAL_JOYSTICK    		0x00000004
#define COD_MINOR_PERIPHERAL_GAMEPAD    		0x00000008
#define COD_MINOR_PERIPHERAL_REMOTE_CONTROL    	0x0000000C
#define COD_MINOR_PERIPHERAL_SENSING    		0x00000010
#define COD_MINOR_PERIPHERAL_DIGITIZER    		0x00000014
#define COD_MINOR_PERIPHERAL_CARD_READER    	0x00000018

// Group: Minor Device Class - Imaging Major class (Select multiple is possible)
#define COD_MINOR_IMAGING_UNCLASSIFIED    		0x00000000
#define COD_MINOR_IMAGING_DISPLAY    			0x00000010
#define COD_MINOR_IMAGING_CAMERA    			0x00000020
#define COD_MINOR_IMAGING_SCANNER    			0x00000040
#define COD_MINOR_IMAGING_PRINTER    			0x00000080

//HFG Device Mask
#define HFG_MASK    (COD_AUDIO + COD_MAJOR_AUDIO + COD_MINOR_AUDIO_HEADSET + COD_MINOR_AUDIO_HANDFREE)

//A2DP Device Mask
#define A2DP_MASK   (COD_RENDERING + COD_MAJOR_AUDIO)

namespace BtDeviceClass {

	bool isCODInMask(unsigned int cod, unsigned int mask);

	bool isAudioDevice(unsigned int cod);

	bool isHFGSupported(unsigned int cod);

	bool isPhone(unsigned int cod);

	bool isA2DPSupported(unsigned int cod);

	bool isComputerOrPhone(unsigned int cod);

}


#endif /* BTDEVICECLASS_H */
