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




#include "Common.h"

#include "VolumeControlAlertWindow.h"
#include "Settings.h"
#include "WindowServer.h"
#include "NativeAlertManager.h"
#include "QtUtils.h"
#include "SoundPlayerPool.h"
#include "SystemUiController.h"

#include <QPainter>

#define		VOLUME_GFX_PHONE_FILENAME		"notification-volume-indicator.png"
#define		VOLUME_GFX_MEDIA_FILENAME		"notification-music-indicator.png"
#define 	VOLUME_GFX_RING_FILENAME		"notification-ringtone-indicator.png"
#define		VOLUME_GFX_RINGMUTE_FILENAME	"bell_off.png"

std::map<int, int> VolumeControlAlertWindow::s_bitmapOffsetMapping;

VolumeControlAlertWindow::VolumeControlAlertWindow(int width, int height,
												   bool hasAlpha, VolumeControlType drawWhat,
												   int initialVolume)
	: AlertWindow(Window::Type_BannerAlert, width, height, hasAlpha)
	, m_drawWhat(drawWhat)
	, m_volumeLevel(initialVolume)
{
	setTransient(true);

	//if the map doesn't exist, create it
	if (s_bitmapOffsetMapping.empty()) {
		s_bitmapOffsetMapping[0] = 0;
		s_bitmapOffsetMapping[1] = 48;
		s_bitmapOffsetMapping[2] = 96;
		s_bitmapOffsetMapping[3] = 144;
		s_bitmapOffsetMapping[4] = 192;
		s_bitmapOffsetMapping[5] = 240;
		s_bitmapOffsetMapping[6] = 288;
		s_bitmapOffsetMapping[7] = 336;
		s_bitmapOffsetMapping[8] = 384;
		s_bitmapOffsetMapping[9] = 432;
		s_bitmapOffsetMapping[10] = 432;
	}

	std::string resourcePath = Settings::LunaSettings()->lunaSystemResourcesPath + std::string("/");
	m_phoneVolumeGfxElements = QPixmap(qFromUtf8Stl(resourcePath + VOLUME_GFX_PHONE_FILENAME));

	m_ringtoneVolumeGfxElements = QPixmap(qFromUtf8Stl(resourcePath + VOLUME_GFX_RING_FILENAME));

	m_mediaVolumeGfxElements = QPixmap(qFromUtf8Stl(resourcePath + VOLUME_GFX_MEDIA_FILENAME));

	m_ringerMuteIcon = QPixmap(qFromUtf8Stl(resourcePath + VOLUME_GFX_RINGMUTE_FILENAME));

	m_initialWidth = m_phoneVolumeGfxElements.width();
	if(m_ringtoneVolumeGfxElements.width() > m_initialWidth)
		m_initialWidth = m_ringtoneVolumeGfxElements.width();
	if(m_mediaVolumeGfxElements.width() > m_initialWidth)
		m_initialWidth = m_mediaVolumeGfxElements.width();
	if(m_ringerMuteIcon.width() > m_initialWidth)
		m_initialWidth = m_ringerMuteIcon.width();

	setSoundParams(std::string(), "none");

	// content rect coordinates are treated in the web app system (0,0, top,left)
	setContentRect(QRect(0, 0, m_initialWidth, height));

	updateScreenPixmap();
}

VolumeControlAlertWindow::~VolumeControlAlertWindow()
{
}

void VolumeControlAlertWindow::setDrawType(VolumeControlType val)
{
	m_drawWhat = val;
	updateScreenPixmap();
	WindowServer::instance()->windowUpdated(this);
	update();
}

void VolumeControlAlertWindow::setVolumeLevel(int val)
{
	m_volumeLevel = val;
	updateScreenPixmap();
	WindowServer::instance()->windowUpdated(this);
	update();

}

void VolumeControlAlertWindow::resizeEventSync(int w, int h)
{
	resize(w, h);
	updateScreenPixmap();
	WindowServer::instance()->windowUpdated(this);
	update();
}

void VolumeControlAlertWindow::close()
{
	//CAREFUL! this results in an effective "delete this"...so don't do ANYTHING in this object after this line
	NativeAlertManager::instance()->closeWindow(this);
}

#define clamp(x,low,high) 		((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))

//static
void VolumeControlAlertWindow::translateVolumeToGfxOffsets(int volumeLevel, int& sourceX, int& sourceY,
														   int& w, int& h)
{
	// Don't ask *sigh*...it's what the audiod people say should be used (and might change in the future)
	int l = clamp((volumeLevel+6),0,110) / 11;

	sourceX = 0;
	sourceY = s_bitmapOffsetMapping[l]; //since I set up the map, i know all values of l are in there
	w = 160;
	h = 48; //both w and h are this for now since the sizes of the source PNGs are known.
}

void VolumeControlAlertWindow::updateScreenPixmap()
{
	m_screenPixmap.fill(QColor(0x0, 0x0, 0x0, 0x0));
	QPainter painter(&m_screenPixmap);
	painter.translate(-boundingRect().x(), -boundingRect().y());

	QPixmap* pSrc = NULL;
	//figure out what to draw
	int sx, sy, sw, sh;

	switch (m_drawWhat) {
	case VolumeControl_MuteBell:
		pSrc = &m_ringerMuteIcon;
		sx = 0;
		sy = 0;
		sw = pSrc->width();
		sh = pSrc->height();
		break;
	case VolumeControl_Phone:
		pSrc = &m_phoneVolumeGfxElements;
		translateVolumeToGfxOffsets(m_volumeLevel, sx, sy, sw, sh);
		break;
	case VolumeControl_Media:
		pSrc = &m_mediaVolumeGfxElements;
		translateVolumeToGfxOffsets(m_volumeLevel, sx, sy, sw, sh);
		break;
	case VolumeControl_Ringtone:
		pSrc = &m_ringtoneVolumeGfxElements;
		translateVolumeToGfxOffsets(m_volumeLevel, sx, sy, sw, sh);
		break;
	}

//	// clear the rect
//	painter.fillRect(boundingRect(), QColor(0x0, 0x0, 0x0, 0x0));

	int destWidth = boundingRect().toRect().width();
	int destOriginX = ((destWidth - sw) >> 1) - (destWidth >> 1);
	int destOriginY = -(boundingRect().toRect().height() >> 1);

	painter.drawPixmap(destOriginX, destOriginY, sw, sh, *pSrc, sx, sy, sw, sh);

	painter.end();
}
