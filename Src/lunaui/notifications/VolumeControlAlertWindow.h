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




#ifndef VOLUMEALERTWINDOW_H
#define VOLUMEALERTWINDOW_H

#include "Common.h"

#include "AlertWindow.h"
#include <map>
#include <QPixmap>

class NativeAlertManager;

class VolumeControlAlertWindow : public AlertWindow
{
public:
	
	enum VolumeControlType {
		VolumeControl_MuteBell,
		VolumeControl_Phone,
		VolumeControl_Media,
		VolumeControl_Ringtone
	};
	
	VolumeControlAlertWindow(int width, int height, 
							 bool hasAlpha, VolumeControlType drawWhat, 
							 int initialVolume=0);
	virtual ~VolumeControlAlertWindow();

	void setDrawType(VolumeControlType val);
	void setVolumeLevel(int val);
	
	virtual void resizeEventSync(int w, int h);
	virtual void close();
	
	friend class NativeAlertManager;
	
private:
	
	void updateScreenPixmap();

	VolumeControlType 		m_drawWhat;
	int				 		m_volumeLevel;
	
	QPixmap 				m_phoneVolumeGfxElements;
	QPixmap				 	m_ringtoneVolumeGfxElements;
	QPixmap				 	m_mediaVolumeGfxElements;
	QPixmap				 	m_ringerMuteIcon;
	
	static void translateVolumeToGfxOffsets(int volumeLevel, int& sourceX, int& sourceY, int& w, int& h);
	// static to avoid having to constantly recreate the exact same mapping
	static std::map<int, int> s_bitmapOffsetMapping;
};

#endif /* VOLUMEALERTWINDOW_H */
