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

#include "BootupAnimation.h"

#include <time.h>
#include <errno.h>

#include <QPainter>

#include <PGFallbackFonts.h>
#include <PGContext.h>
#include <PGSurface.h>
#include <PGFont.h>
#include <sys/wait.h>

//#include "GraphicsUtil.h"
#include "HostBase.h"
#include "Localization.h"
#include "Logging.h"
#include "Settings.h"

const qreal kLowOpacity  = -0.5;
const qreal kHighOpacity = 1.0;

const int kFirstGlowAnimDuration = 4000; // ms
const int kGlowAnimDuration      = 2080; // ms
const int kFadeAnimDuration      = 700;  // ms

static const int s_activityProgressTotal = 20;
static const int s_activitySpinnerTotal = 10;
static const int s_frameTimeSlow = 80; // 80ms, 12.5FPS
static const int s_frameTimeFast = 33; // 33ms, 30FPS
static int s_frameTime = 0;
static const int s_fontHeight1 = 20;
static const int s_fontHeight2 = 16;
static const char* s_line1 = "Updating the system";
static const char* s_line2 = "Do not remove battery";

static void sleepMs(unsigned int ms)
{
	struct timespec req = {0, 0};

	req.tv_sec = 0;
	req.tv_nsec = ms * 1000000L;

	while (true) {
		if (nanosleep(&req, &req) == 0)
			break;
		if (errno != EINTR)
			break;
	}
}

BootupAnimation::BootupAnimation(int readPipeFd)
	: m_renderThread(0)
	, m_state(StateIdle)
	, m_ctxt(0)
	, m_font(0)
	, m_fallbackFonts(0)
	, m_logoSurf(0)
	, m_logoBrightSurf(0)
	, m_activityStaticSurf(0)
	, m_activityProgressSurf(0)
	, m_activitySpinnerSurf(0)
	, m_widthLine1(0)
	, m_widthLine2(0)
	, m_readPipeFd(readPipeFd)
	, m_readNotifier(readPipeFd, QSocketNotifier::Read, this)
	, m_rotation(0)
{
	Settings::LunaSettings()->forceSoftwareRendering = true;

	m_readNotifier.setEnabled(true);
	connect(&m_readNotifier, SIGNAL(activated(int)), SLOT(pipeDataAvailable(int)));
}

BootupAnimation::~BootupAnimation()
{
	if (m_renderThread) {
		g_thread_join(m_renderThread);
		m_renderThread = 0;
	}
}

void BootupAnimation::init()
{
	s_frameTime = s_frameTimeSlow;

	HostBase* host = HostBase::instance();
	const HostInfo& info = host->getInfo();

	m_ctxt = PGContext::create();
	m_ctxt->setDisplay(PPrimary, 0, PFORMAT_8888, 3);
	
	// Fill with black
	m_ctxt->push();
	m_ctxt->setStrokeColor(PColor32(0x00, 0x00, 0x00, 0x00));
	m_ctxt->setFillColor(PColor32(0x00, 0x00, 0x00, 0xFF));
	m_ctxt->drawRect(0, 0, (int) info.displayWidth, (int) info.displayHeight);
	m_ctxt->pop();

	std::string imageRootPath = Settings::LunaSettings()->lunaSystemResourcesPath + "/";
	std::string imagePath;

	imagePath = imageRootPath + "hp-logo.png";
	m_logoSurf = PGSurface::createFromPNGFile(imagePath.c_str());
	if (!m_logoSurf)
		g_warning("Failed to load image: %s", imagePath.c_str());

	imagePath = imageRootPath + "hp-logo-bright.png";
	m_logoBrightSurf = PGSurface::createFromPNGFile(imagePath.c_str());
	if (!m_logoBrightSurf)
		g_warning("Failed to load image: %s", imagePath.c_str());

	imagePath = imageRootPath + "activity-static.png";
	m_activityStaticSurf = PGSurface::createFromPNGFile(imagePath.c_str());
	if (!m_activityStaticSurf)
		g_warning("Failed to load image: %s", imagePath.c_str());
	
	imagePath = imageRootPath + "activity-spinner.png";
	m_activitySpinnerSurf = PGSurface::createFromPNGFile(imagePath.c_str());
	if (!m_activitySpinnerSurf)
		g_warning("Failed to load image: %s", imagePath.c_str());

	imagePath = imageRootPath + "activity-progress.png";
	m_activityProgressSurf = PGSurface::createFromPNGFile(imagePath.c_str());
	if (!m_activityProgressSurf)
		g_warning("Failed to load image: %s", imagePath.c_str());

	const char* fontName = Settings::LunaSettings()->fontBootupAnimation.c_str();
	m_font = PGFont::createFromFile(fontName, s_fontHeight1);
	if (!m_font) {
		g_critical("%s: Failed to load font: %s", __PRETTY_FUNCTION__, fontName);
	}

	m_fallbackFonts = new PGFallbackFonts();

	generateUtf16AndGlyphOffsets();

	m_logoScale = 1.0f;
	m_logoAlpha = 0xFF;

	m_activityProgress = 0;
	m_activitySpinner = 0;

	switch (Settings::LunaSettings()->homeButtonOrientationAngle) {
	case 90:
		m_rotation = -90;
		break;
	case -90:
	case 270:
		m_rotation = 90;
		break;
	case 180:
		m_rotation = 180;
		break;
	default:
		m_rotation = 0;
	}
}

void BootupAnimation::deinit()
{
	s_frameTime = s_frameTimeSlow;

	if (m_ctxt)
		m_ctxt->releaseRef();

	if (m_logoSurf)
		m_logoSurf->releaseRef();

	if (m_logoBrightSurf)
		m_logoBrightSurf->releaseRef();

	if (m_activityStaticSurf)
		m_activityStaticSurf->releaseRef();

	if (m_activitySpinnerSurf)
		m_activitySpinnerSurf->releaseRef();

	if (m_activityProgressSurf)
		m_activityProgressSurf->releaseRef();

	if (m_font)
		m_font->releaseRef();

	delete m_fallbackFonts;

	m_ctxt = 0;
	m_logoSurf = 0;
	m_logoBrightSurf = 0;
	m_activityStaticSurf = 0;
	m_activityProgressSurf = 0;
	m_activitySpinnerSurf = 0;
	m_font = 0;
	m_fallbackFonts = 0;

	m_textLine1.clear();
	m_textLine2.clear();
	m_glyphOffsetsLine1.clear();
	m_glyphOffsetsLine2.clear();
	m_widthLine1 = 0;
	m_widthLine2 = 0;
}

gpointer BootupAnimation::renderThread(gpointer arg)
{
	BootupAnimation* ptr = (BootupAnimation*) arg;
	ptr->init();
	ptr->renderThread();
	ptr->deinit();
	return NULL;
}

void BootupAnimation::renderThread()
{
	while (m_state != StateIdle) {

		switch (m_state) {
		case (StateLogo):
			renderInStateLogo();
			break;
		case (StateActivity):
			renderInStateActivity();
			break;
		default:
			break;
		}

		sleepMs(s_frameTime);
	}
}

void BootupAnimation::renderInStateLogo()
{
	if (!m_logoSurf || !m_logoBrightSurf)
		return;

	const int kLowAlpha  = 0x00 - 0x80;
	const int kHighAlpha = 0xFF;

	static int sCurrAlpha = kLowAlpha;
	static int sDelta = 0x08;

	HostBase* host = HostBase::instance();
	const HostInfo& info = host->getInfo();

	int w = (int) (m_logoSurf->width());
	int h = (int) (m_logoSurf->height());

	int dx = (info.displayWidth  - w) / 2;
	int dy = (info.displayHeight - h) / 2;
	int dr = dx + w;
	int db = dy + h;

	m_ctxt->push();

	m_ctxt->setStrokeColor(PColor32(0x00, 0x00, 0x00, 0x00));
	m_ctxt->setFillColor(PColor32(0x00, 0x00, 0x00, 0xFF));
	m_ctxt->drawRect(0, 0, (int) info.displayWidth, (int) info.displayHeight);

	if (m_rotation != 0) {
		m_ctxt->translate((int)info.displayWidth/2, (int) info.displayHeight/2);
		m_ctxt->rotate(m_rotation);
		m_ctxt->translate((int) -info.displayWidth/2, (int) -info.displayHeight/2);
	}

	if (m_logoSurf) {
		m_ctxt->bitblt(m_logoSurf, dx, dy, dr, db);
	}

	if (m_logoBrightSurf) {
		m_ctxt->setFillOpacity(MAX(MIN(sCurrAlpha, 0xFF), 0x00));
		m_ctxt->bitblt(m_logoBrightSurf, dx, dy, dr, db);
	}

	m_ctxt->pop();

	m_ctxt->flip();

	sCurrAlpha += sDelta;
	if (sCurrAlpha <= kLowAlpha) {
		sDelta = 0x0F;
		sCurrAlpha = kLowAlpha;
	}
	else if (sCurrAlpha >= kHighAlpha) {
		sDelta = -0x0F;
		sCurrAlpha = kHighAlpha;
	}
}

void BootupAnimation::renderInStateActivity()
{
	if (!m_activityStaticSurf || !m_activitySpinnerSurf || !m_activityProgressSurf)
		return;

	const HostInfo& info = HostBase::instance()->getInfo();

	int wStatic = (int) (m_activityStaticSurf->width());
	int hStatic = (int) (m_activityStaticSurf->height());
	int dxStatic = (info.displayWidth - wStatic) / 2;
	int dyStatic = (info.displayHeight - hStatic) / 2;

	int wSpinner = (int) (m_activitySpinnerSurf->width());
	int hSpinner = (int) (m_activitySpinnerSurf->height()) / s_activitySpinnerTotal;
	int dxSpinner = (info.displayWidth - wSpinner) / 2;
	int dySpinner = (info.displayHeight - hSpinner) / 2;

	m_ctxt->push();

	// Fill with black
	m_ctxt->setStrokeColor(PColor32(0x00, 0x00, 0x00, 0x00));
	m_ctxt->setFillColor(PColor32(0x00, 0x00, 0x00, 0xFF));
	m_ctxt->drawRect(0, 0, (int) info.displayWidth, (int) info.displayHeight);

	if (m_rotation != 0) {
		m_ctxt->translate((int)info.displayWidth/2, (int) info.displayHeight/2);
		m_ctxt->rotate(m_rotation);
		m_ctxt->translate((int) -info.displayWidth/2, (int) -info.displayHeight/2);
	}
	
	int progress = m_activityProgress;

	if (progress == 0) {
		m_ctxt->bitblt(m_activityStaticSurf, dxStatic, dyStatic,
					   dxStatic + wStatic, dyStatic + hStatic);
	}
	else {
		progress = progress - 1;
		m_ctxt->bitblt(m_activityProgressSurf,
					   0, progress * hStatic,
					   wStatic, hStatic + progress * hStatic,
					   dxStatic, dyStatic,
					   dxStatic + wStatic,
					   dyStatic + hStatic);
	}

	m_ctxt->bitblt(m_activitySpinnerSurf,
				   0, m_activitySpinner * hSpinner,
				   wSpinner, hSpinner + m_activitySpinner * hSpinner,
				   dxSpinner, dySpinner,
				   dxSpinner + wSpinner,
				   dySpinner + hSpinner);


	m_font->setPointSize(s_fontHeight1);
	m_ctxt->setFont(m_font);

	int xTextLine1 = (info.displayWidth - m_widthLine1) / 2;
	int xTextLine2 = (info.displayWidth - m_widthLine2) / 2;
	int yTextLine1 = (info.displayHeight - s_fontHeight1 * 2);
	int yTextLine2 = (info.displayHeight - s_fontHeight1);
	int xTextOffset = 0;
	int yTextOffset = 0;

	m_ctxt->setStrokeColor(PColor32(0xFF, 0xFF, 0xFF, 0xC0));
	m_ctxt->setFillColor(PColor32(0xFF, 0xFF, 0xFF, 0xC0));

	xTextOffset = xTextLine1;
	yTextOffset = yTextLine1;
	for (unsigned int i = 0; i < m_textLine1.size(); i++) {
		m_ctxt->drawCharacter(m_textLine1[i], xTextOffset, yTextOffset);
		xTextOffset += m_glyphOffsetsLine1[i];
	}

	m_font->setPointSize(s_fontHeight2);
	m_ctxt->setFont(m_font);

	m_ctxt->setStrokeColor(PColor32(0xFF, 0xFF, 0xFF, 0x80));
	m_ctxt->setFillColor(PColor32(0xFF, 0xFF, 0xFF, 0x80));

	xTextOffset = xTextLine2;
	yTextOffset = yTextLine2;
	for (unsigned int i = 0; i < m_textLine2.size(); i++) {
		m_ctxt->drawCharacter(m_textLine2[i], xTextOffset, yTextOffset);
		xTextOffset += m_glyphOffsetsLine2[i];
	}

	m_ctxt->pop();

	m_activitySpinner = (m_activitySpinner + 1) % s_activitySpinnerTotal;

	if (m_activityProgress == s_activityProgressTotal) {

		// Fill with black
		m_ctxt->push();
		m_ctxt->setStrokeColor(PColor32(0x00, 0x00, 0x00, 0x00));
		m_ctxt->setFillColor(PColor32(0x00, 0x00, 0x00, 0xFF));
		m_ctxt->drawRect(0, 0, (int) info.displayWidth, (int) info.displayHeight);
		m_ctxt->pop();

		m_logoScale = 1.0f;
		m_logoAlpha = 0xFF;
		m_state = StateLogo;
	}

	m_ctxt->flip();
}

// static function invoked by the main process to stop the animation
void BootupAnimation::stopBootupAnimation()
{
	if(bootAnimPipeFd < 0)
		return;
	
	char message[BOOT_ANIM_MESSAGE_LENGHT];
	message[0] = BOOT_ANIM_MESSAGE_END_ANIMATION;
	
	::write(bootAnimPipeFd, message, BOOT_ANIM_MESSAGE_LENGHT);
	
	// now wait until the boot animation process exits
	::waitpid(bootAnimPid, 0, 0);
}

// static function invoked by the main process to start the progress part of the animation
void BootupAnimation::startActivityAnimation()
{
	if(bootAnimPipeFd < 0)
		return;
	
	char message[BOOT_ANIM_MESSAGE_LENGHT];
	message[0] = BOOT_ANIM_MESSAGE_START_PROGRESS;
	
	::write(bootAnimPipeFd, message, BOOT_ANIM_MESSAGE_LENGHT);
}

// static function invoked by the main process to stop the progress part of the animation
void BootupAnimation::stopActivityAnimation()
{
	if(bootAnimPipeFd < 0)
		return;
	
	char message[BOOT_ANIM_MESSAGE_LENGHT];
	message[0] = BOOT_ANIM_MESSAGE_END_PROGRESS;
	
	::write(bootAnimPipeFd, message, BOOT_ANIM_MESSAGE_LENGHT);
}

// static function invoked by the main process to update the progress meter during the animation
void BootupAnimation::setActivityAnimationProgress(int val, int total)
{
	if(bootAnimPipeFd < 0)
		return;
	
	char message[BOOT_ANIM_MESSAGE_LENGHT];
	message[0] = BOOT_ANIM_MESSAGE_PROGRESS_UPDATE;
	
	int* p = (int*)&(message[1]);
	*p = val;
	
	p = (int*)&(message[1 + sizeof(int)]);
	*p = total;
	
	::write(bootAnimPipeFd, message, BOOT_ANIM_MESSAGE_LENGHT);
}

void BootupAnimation::start()
{

	init();
	luna_assert(m_state == StateIdle);
	luna_assert(m_renderThread == 0);

	m_state = StateLogo;
	m_renderThread = g_thread_create(BootupAnimation::renderThread,
										this, true, NULL);
}

void BootupAnimation::stop()
{
	luna_assert(m_state == StateLogo);
	luna_assert(m_renderThread);

	s_frameTime = s_frameTimeFast;
	m_logoAlpha = 0xFF;
	m_logoScale = 1.0f;
	m_state = StateIdle;
	g_thread_join(m_renderThread);
	m_renderThread = 0;
	//At this point, the render thread has terminated and we have
	//done a deinit of all resources.  exit
	exit(0);
}

void BootupAnimation::startActivity()
{
	luna_assert(m_state == StateLogo);
	luna_assert(m_renderThread);

	setActivityProgress(0, 1);
	m_state = StateActivity;
}

void BootupAnimation::stopActivity()
{
	luna_assert(m_renderThread);

	setActivityProgress(1, 1);    
}

void BootupAnimation::setActivityProgress(int val, int total)
{
	val = CLAMP(val, 0, total);
	m_activityProgress = (int) ::roundf((val * s_activityProgressTotal) / (total * 1.0f));
	m_activityProgress = CLAMP(m_activityProgress, 0, s_activityProgressTotal);
}

void BootupAnimation::pipeDataAvailable(int pipe)
{
	char buffer[BOOT_ANIM_MESSAGE_LENGHT];
	int index = 0;
	char data;
	
	while(index < BOOT_ANIM_MESSAGE_LENGHT && (::read(pipe, &(buffer[index]), 1) == 1)) {
		index++;
	}
	if(buffer[0] == BOOT_ANIM_MESSAGE_END_ANIMATION) {
		stop();
	} else if(buffer[0] == BOOT_ANIM_MESSAGE_START_PROGRESS) {
		startActivity();
	} else if(buffer[0] == BOOT_ANIM_MESSAGE_END_PROGRESS) {
		stopActivity();
	} else if(buffer[0] == BOOT_ANIM_MESSAGE_PROGRESS_UPDATE) {
		int val, total;
		
		int* p = (int*)&(buffer[1]);
		val = *p;
		
		p = (int*)&(buffer[1 + sizeof(int)]);
		total = *p;

		setActivityProgress(val, total);
	} 
}

// ====================================================================

BootupAnimationTransition::BootupAnimationTransition()
	: m_screenPixmap(0)
{
	init();
}

BootupAnimationTransition::~BootupAnimationTransition()
{

}

void BootupAnimationTransition::init()
{
	HostBase* host = HostBase::instance();
	const HostInfo& info = host->getInfo();
	
	m_bounds = QRect(-info.displayWidth/2, -info.displayHeight/2, info.displayWidth, info.displayHeight);

	std::string imageRootPath = Settings::LunaSettings()->lunaSystemResourcesPath + "/";
	std::string imagePath;

	imagePath = imageRootPath + "hp-logo.png";
	QPixmap logoPixmap(imagePath.c_str());
	if (logoPixmap.isNull())
		g_warning("Failed to load image: %s", imagePath.c_str());
	
	m_screenPixmap = new QPixmap(info.displayWidth, info.displayHeight);

	m_rotation = Settings::LunaSettings()->homeButtonOrientationAngle;
	
	// prepare the full screen pixmap for the animation
	QPainter painter(m_screenPixmap);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.fillRect(QRect(0, 0, info.displayWidth, info.displayHeight), QColor(0x00, 0x00, 0x00, 0xFF));
	if (m_rotation != 0) {
		painter.translate(info.displayWidth/2, info.displayHeight/2);
		painter.rotate(m_rotation);
		painter.translate(-info.displayWidth/2, -info.displayHeight/2);
	}

	painter.drawPixmap(info.displayWidth/2 - logoPixmap.width()/ 2, info.displayHeight/2 - logoPixmap.height()/ 2, logoPixmap);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
	painter.end();

	m_opacityAnimationPtr = new QPropertyAnimation();
	m_opacityAnimationPtr->setPropertyName("opacity");
	m_opacityAnimationPtr->setEasingCurve(QEasingCurve::Linear);
	m_opacityAnimationPtr->setTargetObject(this);
	m_opacityAnimationPtr->setDuration(kFadeAnimDuration); 
	m_opacityAnimationPtr->setStartValue(1.0);
	m_opacityAnimationPtr->setEndValue(0.0);
	
	m_scaleAnimationPtr   = new QPropertyAnimation();
	m_scaleAnimationPtr->setPropertyName("scale");
	m_scaleAnimationPtr->setEasingCurve(QEasingCurve::Linear);
	m_scaleAnimationPtr->setTargetObject(this);
	m_scaleAnimationPtr->setDuration(kFadeAnimDuration); 
	m_scaleAnimationPtr->setStartValue(1.0);
	m_scaleAnimationPtr->setEndValue(2.0);

	m_fadeAnimationGroupPtr = new QParallelAnimationGroup();
	m_fadeAnimationGroupPtr->addAnimation(m_opacityAnimationPtr);	
	m_fadeAnimationGroupPtr->addAnimation(m_scaleAnimationPtr);
	connect(m_fadeAnimationGroupPtr, SIGNAL(finished()), SLOT(fadeAnimationFinished()));
}

void BootupAnimationTransition::deinit()
{
	
	if(m_screenPixmap) {
		delete m_screenPixmap;
		m_screenPixmap = 0;
	}
	
	if(!m_fadeAnimationGroupPtr.isNull()) {
		m_fadeAnimationGroupPtr->stop();
		delete m_fadeAnimationGroupPtr;
	}
	
	if(!m_opacityAnimationPtr.isNull()) {
		m_opacityAnimationPtr->stop();
		delete m_opacityAnimationPtr;
	}
	
	if(!m_scaleAnimationPtr.isNull()) {
		m_scaleAnimationPtr->stop();
		delete m_scaleAnimationPtr;
	}
}

void BootupAnimationTransition::start()
{
	luna_assert(!m_fadeAnimationGroupPtr.isNull());
	
	m_fadeAnimationGroupPtr->start();
}


QRectF BootupAnimationTransition::boundingRect() const
{
	return m_bounds;
}

void BootupAnimationTransition::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if (!m_screenPixmap)
		return;
	
	painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

	painter->drawPixmap(-m_screenPixmap->width()/ 2, -m_screenPixmap->height()/ 2, *m_screenPixmap);
	
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
}

void BootupAnimationTransition::fadeAnimationFinished()
{
	deinit();
	
	// self-destruct
	delete this;
}

void BootupAnimation::generateUtf16AndGlyphOffsets()
{
	m_textLine1 = convertToUtf16(LOCALIZED(s_line1));
	m_textLine2 = convertToUtf16(LOCALIZED(s_line2));

	m_glyphOffsetsLine1 = getGlyphOffsets(m_textLine1, s_fontHeight1);
	m_glyphOffsetsLine2 = getGlyphOffsets(m_textLine2, s_fontHeight2);

	m_widthLine1 = 0;
	for (unsigned int i = 0; i < m_glyphOffsetsLine1.size(); i++)
		m_widthLine1 += m_glyphOffsetsLine1[i];

	m_widthLine2 = 0;
	for (unsigned int i = 0; i < m_glyphOffsetsLine2.size(); i++)
		m_widthLine2 += m_glyphOffsetsLine2[i];

}

std::vector<gunichar2> BootupAnimation::convertToUtf16(const std::string& s) const
{
	glong charsWritten = 0;
	gunichar2* utf16Str = g_utf8_to_utf16(s.c_str(), -1, NULL, &charsWritten, NULL);
	std::vector<gunichar2> res;

	if (!utf16Str || charsWritten <= 0) {

		g_warning("%s: Failed to convert banner message to utf16: %s",
				  __PRETTY_FUNCTION__, s.c_str());

		res.resize(s.size());
		for (unsigned int i = 0; i < s.size(); i++) {
			res[i] = s[i];
		}

	}
	else {
		res.resize(charsWritten);
		for (int i = 0; i < charsWritten; i++) {
			res[i] = utf16Str[i];
		}
	}

	g_free(utf16Str);

	return res;
}

std::vector<int> BootupAnimation::getGlyphOffsets(const std::vector<gunichar2>& s, int height) const
{
	std::vector<int> res;
	res.resize(s.size());

	m_font->setPointSize(height, true);
	for (unsigned int i = 0; i < s.size(); i++)
		res[i] = m_font->width(s[i], static_cast<PGContext*>(m_fallbackFonts->sharedFontMeasuringContext()));


	return res;
}

void BootupAnimation::paintEvent(QPaintEvent *event)
{

}
