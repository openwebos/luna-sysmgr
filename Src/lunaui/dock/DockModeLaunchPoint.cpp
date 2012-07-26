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




#include "Common.h"

#include <glib.h>
#include <string>

#include "HostBase.h"
#include "Settings.h"
#include "DockModeWindowManager.h"
#include "DockModePositionManager.h"
#include "Window.h"
#include "WindowServer.h"
#include "Logging.h"
#include "DockModeWindow.h"
#include "LaunchPoint.h"
#include "ApplicationDescription.h"
#include "DockModeLaunchPoint.h"
#include "CardDropShadowEffect.h"
#include "QtUtils.h"

#include <QTextLayout>
#include <QGraphicsDropShadowEffect>

#if defined(HAVE_OPENGL)
#include <QGLContext>
#include <QGLFramebufferObject>
#endif

#include "cjson/json.h"

static int kRoundedCornerRadius = 25;
static int kLabelTextOffset = 5;


#ifndef GL_BGRA
#define GL_BGRA	0x80E1
#endif


// static class members
QPainter DockModeLaunchPoint::imagePainter;
QPixmap* DockModeLaunchPoint::s_scrimPixmap = 0;
QPixmap* DockModeLaunchPoint::s_defaultSplash = 0;

// Add icon images
QPixmap *addCardPortrait=0, *addCardPortraitHighlight=0, *addCardLandscape=0, *addCardLandscapeHighlight=0;
QPixmap *cardMaskPortrait=0, *cardMaskLandscape=0;


DockModeLaunchPoint::DockModeLaunchPoint(int width, int height, const LaunchPoint* lp, bool isPermanent)
	: m_launchPoint(lp) 
	, m_beingDeleted(false)
    , m_orientation(OrientationEvent::Orientation_Up)
	, m_splashImage(0)
	, m_state(NotLaunched)
	, m_openWindow(0)
	, m_appScreenShot(0)
    , m_screenShotOrientation(OrientationEvent::Orientation_Up)
	, m_width(width)
	, m_height(height)
	, m_hasScreenshot(false)
	, m_positionAnimationPtr(0)
	, m_buttonHighlight(false)
	, m_buttonPressed(false)
	, m_isPermanent(isPermanent)
{

	m_imageBounds = QRect(-m_width/2, -(m_height)/2, m_width, m_height);
	m_bounds = QRect(-m_width/2, -(m_height)/2, m_width, m_height);

	if(!cardMaskPortrait) {
		std::string path = Settings::LunaSettings()->lunaSystemResourcesPath + "/dock-mode-card-mask-portrait.png";
		cardMaskPortrait = new QPixmap(path.c_str());
	}

	if(!cardMaskLandscape) {
		std::string path = Settings::LunaSettings()->lunaSystemResourcesPath + "/dock-mode-card-mask-landscape.png";
		cardMaskLandscape = new QPixmap(path.c_str());
	}

	m_splashImage = new QPixmap(m_width, m_height);

	const char* fontName = Settings::LunaSettings()->fontDockMode.c_str();
	m_font = new QFont(fontName, 14); 
	m_font->setBold(true);

	m_textLayout.setFont(*m_font);
	m_textLayout.clearLayout();
	m_textLayout.setText(fromStdUtf8(m_launchPoint->title()));
	m_textLayout.setTextOption(QTextOption(Qt::AlignCenter));
	QFontMetrics fontMetrics(*m_font);

	// create the text layout for the label
	int leading = fontMetrics.leading();
	m_textHeight = 0;
	m_textLayout.beginLayout();
	int i = 0;
	while (i < 2) {
		QTextLine line = m_textLayout.createLine();
		if (!line.isValid())
			break;
		line.setLineWidth(m_width);
		m_textHeight += leading;
		line.setPosition(QPointF(0, m_textHeight));
		m_textHeight += line.height();
	}
	m_textLayout.endLayout();
}

QRectF DockModeLaunchPoint::boundingRect() const 
{ 
	return m_bounds; 
}

DockModeLaunchPoint::~DockModeLaunchPoint()
{
	if (m_font)
		delete m_font;	

	if (m_splashImage)
		delete m_splashImage;	

	if(m_appScreenShot)
		delete m_appScreenShot;
}


void DockModeLaunchPoint::setOpenWindow(Window* win)
{ 
	m_openWindow = win; 
}

bool DockModeLaunchPoint::windowJustFinishedLoading()
{
	return (m_openWindow != 0);
}

Window* DockModeLaunchPoint::openWindow() 
{ 
	return  m_openWindow; 
}

void DockModeLaunchPoint::setState(State newState)
{ 
	m_state = newState; 
	
	if(m_state == Closed){
		if(!m_hasScreenshot) {
			m_state = NotLaunched;
		}
	}
	
}


void DockModeLaunchPoint::createAppScreenshot(bool closing)
{	
	if(!m_openWindow || m_beingDeleted)
		return;

	if(!m_appScreenShot) {
		const HostInfo& info = HostBase::instance()->getInfo();
		m_appScreenShot = new QPixmap(info.displayWidth, info.displayHeight);

		if(!m_appScreenShot)
			return;

	}
	
	m_appScreenShot->fill(QColor(0xFF, 0xFF, 0xFF, 0x00));

#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
	QGLContext* gc = (QGLContext*) QGLContext::currentContext();
	luna_assert(gc);
	GLuint textureId = gc->bindTexture(*m_appScreenShot, GL_TEXTURE_2D, GL_BGRA, QGLContext::PremultipliedAlphaBindOption);

	QGLFramebufferObject fbo(m_appScreenShot->width(), m_appScreenShot->height());
	fbo.bind();

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		g_critical("1: FBO generation failed with error: 0x%04x for texture: %d",
			   status, textureId);
		g_critical("glFramebufferTexture2D result: 0x%08x", glGetError());
		return;
	}
	imagePainter.begin(&fbo);	
#else
	imagePainter.begin(m_appScreenShot);	
#endif
	
	imagePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	imagePainter.setCompositionMode(QPainter::CompositionMode_Source);

	const QPixmap* pix = m_openWindow->acquireScreenPixmap();
	if (pix) {
		imagePainter.drawPixmap(0, 0, m_appScreenShot->width(), m_appScreenShot->height(), *pix);
	}
	else {
		g_critical("%s: Failed to acquireScreenPixmap", __PRETTY_FUNCTION__);
	}

	if(closing) {
		imagePainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		imagePainter.drawPixmap(0, 0, m_appScreenShot->width(), m_appScreenShot->height(), *(scrimImage()));
	}
	imagePainter.end();	

#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
	fbo.release();
	gc->deleteTexture(textureId);
#endif
	
	m_screenShotOrientation = m_orientation;
	
	m_hasScreenshot = true;
	
	// always update the scaled down version
	createSplashImage(m_appScreenShot, m_screenShotOrientation);
}

void DockModeLaunchPoint::deleteScreenShot()
{
	if(m_appScreenShot) {
		delete m_appScreenShot;
		m_appScreenShot = 0;
	}
}

void DockModeLaunchPoint::createSplashImage(QPixmap* img, OrientationEvent::Orientation imgOrientation)
{
	if(!img || !m_splashImage)
		return;
	
	if(!m_hasScreenshot && m_state == Closed)
		m_state = NotLaunched;
	
	copyAndRotateImage(img, imgOrientation, m_splashImage, m_orientation, MINIMIZED_SCALE_FACTOR);
	
	// now draw the card mask on top 
	imagePainter.begin(m_splashImage);	
    if(m_orientation == OrientationEvent::Orientation_Right) {
		if(G_LIKELY(cardMaskLandscape))
			imagePainter.drawPixmap(QRectF(0, 0, m_width, m_height), 
			   	                    *cardMaskLandscape,
				                    QRectF(0, 0, cardMaskLandscape->width(), cardMaskLandscape->height()));
    } else if(m_orientation == OrientationEvent::Orientation_Left) {
		imagePainter.rotate(180);
		if(G_LIKELY(cardMaskLandscape))
			imagePainter.drawPixmap(QRectF(-m_width, -m_height, m_width, m_height), 
			   	                    *cardMaskLandscape,
				                    QRectF(0, 0, cardMaskLandscape->width(), cardMaskLandscape->height()));
		imagePainter.rotate(-180);
		
    } else if(m_orientation == OrientationEvent::Orientation_Down) {
		imagePainter.rotate(180);
		if(G_LIKELY(cardMaskPortrait))
			imagePainter.drawPixmap(QRectF(-m_width, -m_height, m_width, m_height), 
			   	                    *cardMaskPortrait,
				                    QRectF(0, 0, cardMaskPortrait->width(), cardMaskPortrait->height()));
		imagePainter.rotate(-180);
	} else {
		if(G_LIKELY(cardMaskPortrait))
			imagePainter.drawPixmap(QRectF(0, 0, m_width, m_height), 
			   	                    *cardMaskPortrait,
				                    QRectF(0, 0, cardMaskPortrait->width(), cardMaskPortrait->height()));
	}

	imagePainter.end();
}

void DockModeLaunchPoint::copyAndRotateImage(QPixmap* source, OrientationEvent::Orientation srcOrient,
                                             QPixmap* destination, OrientationEvent::Orientation dstOrient, qreal scaleFactor)
{
	destination->fill(QColor(0xFF, 0xFF, 0xFF, 0x00));
	imagePainter.begin(destination);
	
	imagePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	
	QPainterPath paintPath;
	qreal destRotation;
	
	switch (dstOrient) {
    case OrientationEvent::Orientation_Left:
		destRotation = 90;
	break;
    case OrientationEvent::Orientation_Down:
		destRotation = 180;
	break;		
    case OrientationEvent::Orientation_Right:
		destRotation = 270;
	break;
    case OrientationEvent::Orientation_Up:
	default:
		destRotation = 0;
	break;
	}

	imagePainter.translate((qreal)destination->width()/2.0, (qreal)destination->height()/2.0);
	imagePainter.setCompositionMode(QPainter::CompositionMode_SourceOver);	
	imagePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);	
	imagePainter.setRenderHint(QPainter::Antialiasing, true);	
	
	if( (dstOrient == srcOrient) ||
        ((dstOrient == OrientationEvent::Orientation_Up) && (srcOrient == OrientationEvent::Orientation_Down))    ||
        ((dstOrient == OrientationEvent::Orientation_Down) && (srcOrient == OrientationEvent::Orientation_Up))    ||
        ((dstOrient == OrientationEvent::Orientation_Left) && (srcOrient == OrientationEvent::Orientation_Right)) ||
        ((dstOrient == OrientationEvent::Orientation_Right) && (srcOrient == OrientationEvent::Orientation_Left))  )
	{
	   // No need to crop
		switch (srcOrient) {
        case OrientationEvent::Orientation_Left:
			imagePainter.rotate(90 - destRotation);
		break;
        case OrientationEvent::Orientation_Down:
			imagePainter.rotate(180 - destRotation);
		break;		
        case OrientationEvent::Orientation_Right:
			imagePainter.rotate(270 - destRotation);
		break;
        case OrientationEvent::Orientation_Up:
		default:
			imagePainter.rotate(-destRotation);
		break;
		}
		
		paintPath.addRoundedRect(QRect(-source->width()/2, -source->height()/2, source->width(), source->height()), 
				                 kRoundedCornerRadius, kRoundedCornerRadius);

		imagePainter.scale(1.0/scaleFactor, 1.0/scaleFactor);
		imagePainter.setBrushOrigin(-(source->width())/2, -(source->height())/2);
		imagePainter.fillPath(paintPath, *source);
		imagePainter.scale(scaleFactor, scaleFactor);
	} else {
		// Need to rotate and crop the image
		qreal x, y, w, h;
        if((m_screenShotOrientation == OrientationEvent::Orientation_Up) || (m_screenShotOrientation == OrientationEvent::Orientation_Down)) {
			x = 0; w = source->width();
			h = (int)((double)source->width() * ((double)source->width() / (double)source->height()));
			y = 0;
		} else{
			x = 0; w = source->width();
			h = (int)((double)source->width() * ((double)source->width() / (double)source->height()));
			y = (source->height() - h) / 2;
		}

		switch (srcOrient) {
        case OrientationEvent::Orientation_Left:
			imagePainter.rotate(destRotation - 90);
		break;
        case OrientationEvent::Orientation_Down:
			imagePainter.rotate(-destRotation);
		break;		
        case OrientationEvent::Orientation_Right:
			imagePainter.rotate(destRotation + 90);
		break;
        case OrientationEvent::Orientation_Up:
		default:
			imagePainter.rotate(destRotation);
		break;
		}
		
		qreal cropScale = w/h;
		qreal paintScale = scaleFactor / cropScale;
		
		paintPath.addRoundedRect(QRect(-(source->height() / (cropScale)) /2, 
				                       -(source->width()  / (cropScale)) /2, 
				                       source->height()   / (cropScale), 
				                       source->width()    / (cropScale)     ), 
				                 kRoundedCornerRadius / (cropScale) , kRoundedCornerRadius / (cropScale));

		imagePainter.scale(1.0/(paintScale), 1.0/(paintScale));
		imagePainter.setBrushOrigin(-(source->height() * h/w)/2, -(source->width() * h/w)/2);
		imagePainter.fillPath(paintPath, *source);
		imagePainter.scale((paintScale), (paintScale));
	}
	imagePainter.end();
	
}


void DockModeLaunchPoint::resize (int width, int height)
{
	m_width = width;
	m_height = height;
	m_bounds = QRect(-width/2, -height/2, width, height);

}


void DockModeLaunchPoint::setOrientation(OrientationEvent::Orientation orient)
{
	if(m_beingDeleted)
		return;
		
	
	m_orientation = orient;

	switch (m_orientation) {
    case OrientationEvent::Orientation_Left:
		m_imageBounds = QRect(-m_height/2, -(m_width)/2, m_height, m_width);			
		m_rotation = 90;
		
	break;
    case OrientationEvent::Orientation_Down:
		m_imageBounds = QRect(-m_width/2, -(m_height)/2, m_width, m_height);	
		m_rotation = 180;
		
	break;		
    case OrientationEvent::Orientation_Right:
		m_imageBounds = QRect(-m_height/2, -(m_width)/2, m_height, m_width);			
		m_rotation = 270;
		
	break;
    case OrientationEvent::Orientation_Up:
	default:
		m_imageBounds = QRect(-m_width/2, -(m_height)/2, m_width, m_height);	
		m_rotation = 0;
		
	break;
	}
	
	if(!m_appScreenShot) {
		// create a default loading screen based on the app specified splash image
		m_appScreenShot = new QPixmap(*(defaultSplashImage())); 
		
        m_screenShotOrientation = OrientationEvent::Orientation_Up;
		m_hasScreenshot = false;
		
		if(!m_appScreenShot)
			return;
	}
	
	if ((m_state != NotLaunched))
		createSplashImage(m_appScreenShot, m_screenShotOrientation);
	
	if (m_openWindow)
		m_openWindow->setRotation (-m_rotation);
}

void DockModeLaunchPoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPen oldPen = painter->pen();
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

	// The Splash image is always generated for the correct screen orientation, so we don't need to
	// rotate when paiting it. We only need t rotate to draw the icon and the Label.
}

void DockModeLaunchPoint::positionAnimationValueChanged(const QVariant& value)
{
	QPointF position = value.toPointF();
	if(m_openWindow && (m_openWindow->scale() != 1.0))
		m_openWindow->setPos(position);
	
}


QPixmap* DockModeLaunchPoint::defaultSplashImage() 
{
	if(!s_defaultSplash) {
		std::string path = Settings::LunaSettings()->lunaSystemResourcesPath + "/dock-app-default-splash-image.png";
		s_defaultSplash = new QPixmap(path.c_str());
	}
	
	return s_defaultSplash;
}

QPixmap* DockModeLaunchPoint::scrimImage() 
{
	if(!s_scrimPixmap) {
		std::string scrimPath = Settings::LunaSettings()->lunaSystemResourcesPath + "/scrim.png";
		s_scrimPixmap = new QPixmap(scrimPath.c_str());
	}
	
	return s_scrimPixmap;
}

void DockModeLaunchPoint::deleteStaticImages() 
{
	if(s_defaultSplash) {
		delete s_defaultSplash;
		s_defaultSplash = 0;
	}
	
	if(s_scrimPixmap) {
		delete s_scrimPixmap;
		s_scrimPixmap = 0;
	}
}

