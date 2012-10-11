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

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <cmath>

#include <string>
#include <tr1/unordered_set>

#include <QApplication>
#include <QGesture>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QtDebug>
#include <QGestureRecognizer>

#if defined(HAVE_OPENGL)
#include <QGLWidget>
#endif

#include <SysMgrDefs.h>


#include "WindowServer.h"

#include "BootupAnimation.h"
#include "Logging.h"
#include "Debug.h"
#include "HapticsController.h"
#include "HostBase.h"
#include "MutexLocker.h"
#include "Window.h"
#include "MetaKeyManager.h"
#include "CoreNaviManager.h"
#include "ReticleItem.h"
#include "Settings.h"
#include "AnimationSettings.h"
#include "SystemUiController.h"
#include "WebAppMgrProxy.h"
#include "SystemService.h"
#include "EventReporter.h"
#include "CustomEvents.h"
#include "WindowServerLuna.h"
#include "WindowServerMinimal.h"
#include "SoundPlayerPool.h"
#include "Time.h"
#include "TouchToShareGlow.h"
#include "WSOverlayScreenShotAnimation.h"
#include "Localization.h"
#include "SharedGlobalProperties.h"

#include "Preferences.h"            // Neeed for IME

#include "NativeAlertManager.h"
#include "SingleClickGestureRecognizer.h"
#include "QtUtils.h"


#if defined(HAVE_OPENGL)
#include <QGLContext>
#include <QGLFramebufferObject>
#endif

#if defined(TARGET_DEVICE) && !defined(HAS_QPA)
// Obsolete, since we always effectively assume there will be a QPA
#include <QWSServer>
#include <input/hiddtp_qws.h>
#endif

#include <vector>
#include "FpsHistory.h"

#include "TouchPlot.h"

#include "WindowManagerBase.h"

static const char* kWindowSrvChnl = "WindowServer";
static std::tr1::unordered_set<unsigned long> s_registeredWindows;

static const int kMaxPaintFPS = 120;
static const int kMinPaintInterval = 1000 / kMaxPaintFPS;
static const unsigned int kResizePendingTickIntervalInMS    = 100;
static const int kDeferredNewOrientationIntervalMs = 200;

// ------------------------------------------------------------------

class FpsCounter : public QGraphicsItem
{
public:

	FpsCounter() {
		m_fpsMarkers = new float[FPSNumStates - 1];
		m_fpsMarkers[FPSTerrible] = 1000 / 30.0f; // < 30 fps
		m_fpsMarkers[FPSPoor] = 1000 / 40.0f; // 30fps <= fps < 40 fps
		m_fpsMarkers[FPSOk] = 1000 / 50.0f; // < 40fps <= fps < 50 fps

		m_fpsBrushes = new QBrush[FPSNumStates];
		m_fpsBrushes[FPSTerrible] = QBrush(Qt::red);
		m_fpsBrushes[FPSPoor] = QBrush(QColor(255, 140, 0));
		m_fpsBrushes[FPSOk] = QBrush(Qt::yellow);
		m_fpsBrushes[FPSGood] = QBrush(Qt::green);

		m_performance = FPSOk;

		m_elapsed.start();
		setFPS(0,0,0);
	}

	virtual QRectF boundingRect () const  {
		const int width = 150;
		const int height = 20;
		return QRectF(-width/2, -height/2, width, height);
	}

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
		QRectF rect(boundingRect());
		painter->fillRect(rect, m_fpsBrushes[m_performance]);
		painter->setPen(QPen(Qt::black));
		painter->setFont(scene()->views().first()->viewport()->font());
		painter->drawText(rect, Qt::AlignLeft, m_fpsString);
		painter->drawText(rect, Qt::AlignRight, m_stdDevString);
	}

	virtual void setFPS(int fps, int std_dev, uint32_t timeMs)
	{
		FpsHistoryElem elem;

		elem.Fps = fps;
		elem.Standard_Deviation = std_dev;

		m_fpsString = QString::fromLatin1("%03 FPS").arg(fps);
		m_stdDevString = QString::fromLatin1("%03 ms").arg(std_dev);

		//Add the passed in fps/std_dev value to the circular buffer
		m_fpsHistory.AddSample(elem, timeMs);
	
	}

	virtual void dumpFPS()
	{
		//Dump the FPS and std_dev from the circular buffer
		m_fpsHistory.Dump();
	}

	virtual void resetFpsBuffer(int newBufSize)
	{
		//Reset the circular buffer to the requesetd size
		m_fpsHistory.Reset(newBufSize);
	}

private:
	QString m_fpsString;
	QString m_stdDevString;

	enum FPSState {
		FPSTerrible = 0,
		FPSPoor,
		FPSOk,
		FPSGood,
		FPSNumStates
	};
	float *m_fpsMarkers;
	QBrush *m_fpsBrushes;

	FPSState m_performance;
	FPSState m_nextPerformance;

	QTime m_elapsed;

	FpsHistory m_fpsHistory;
};

static FpsCounter* s_fpsCounter = 0;
static TouchPlot *s_TouchPlot = 0;

// ------------------------------------------------------------------

class WindowServerOverlay : public QGraphicsItem
{
public:
	
	WindowServerOverlay() {
		const HostInfo& info = HostBase::instance()->getInfo();
		m_width = info.displayWidth;
		m_height = 20;
	}
	
	virtual QRectF boundingRect () const  {
		return QRectF(-m_width/2, -m_height/2, m_width, m_height);
	}

	void setString(const QString& str) {
		m_string = str;
		update();
	}

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
		QRectF rect(boundingRect());
		painter->fillRect(rect, Qt::white);
		QRectF fpsRect(rect.x() + rect.width() / 2, rect.y(), rect.width() / 2, rect.height());
		painter->setPen(QPen(Qt::black));
		painter->setFont(QFont("Prelude", 10));
		painter->drawText(rect, Qt::AlignLeft, m_string);
	}
	
private:

	int m_width;
	int m_height;
	QString m_string;
};


static WindowServerOverlay* s_overlay = 0;

// ------------------------------------------------------------------

Runtime::Runtime(QObject* parent)
	: QObject(parent), m_orientation(Orientation_Portrait), m_twelveHourClock (true)
{
	connect (Preferences::instance(), SIGNAL(signalTimeFormatChanged(const char*)),
			this, SLOT(slotTimeFormatChanged (const char*)));
}

Runtime* Runtime::instance()
{
	static Runtime *instance = 0;
	if (!instance)
		instance = new Runtime;
	return instance;
}

void Runtime::setOrientation (OrientationEvent::Orientation orient)
{
	switch (orient) {
        case OrientationEvent::Orientation_Up:  m_orientation = Orientation_Portrait; break;
        case OrientationEvent::Orientation_Down: m_orientation = Orientation_PortraitInverted; break;
        case OrientationEvent::Orientation_Left: m_orientation = Orientation_Landscape; break;
        case OrientationEvent::Orientation_Right: m_orientation = Orientation_LandscapeInverted; break;
		default: return;
	};

	Q_EMIT orientationChanged();
}

void Runtime::slotTimeFormatChanged (const char* format)
{
	bool oldSetting = m_twelveHourClock;
	m_twelveHourClock = (Preferences::instance()->timeFormat() == "HH12");
	if (oldSetting != m_twelveHourClock)
		Q_EMIT clockFormatChanged();
}

QString Runtime::getLocalizedString(QString text)
 {
	 return QString::fromUtf8(LOCALIZED(text.toStdString()).c_str());
 }

QString Runtime::getLocalizedDay()
 {
    QDate date = QDate::currentDate();
    QLocale locale;
    return locale.dayName(date.dayOfWeek(),QLocale::ShortFormat);//date.toString("dddd"));
 }

QString Runtime::getLocalizedMonth()
 {
    QDate date = QDate::currentDate();
    QLocale locale;
    return locale.monthName(date.month(),QLocale::ShortFormat).toUpper();
 }

QString Runtime::getLocalizedAMPM()
 {
    QLocale locale;
    QTime time = QTime::currentTime();
    if(time.hour()<12)
        return locale.amText();
    else
        return locale.pmText();
 }


// ------------------------------------------------------------------

static WindowServer* s_instance = NULL;
static PIpcBuffer* s_globalPropsBuffer = NULL;

WindowServer* WindowServer::instance()
{
    if (!s_instance) {
		switch (Settings::LunaSettings()->uiType) {
		case (Settings::UI_MINIMAL):
			new WindowServerMinimal;
			break;
		case (Settings::UI_LUNA):
		default:
			new WindowServerLuna;
			break;
		}
	}

	return s_instance;
}

#ifdef DEBUG_RECORD_PAINT
static bool vsyncDisabled;
#endif

WindowServer::WindowServer()
	: m_displayMgr(0)
	, m_inputMgr(0)
	, m_metaKeyMgr(0)
	, m_coreNaviMgr(0)
	, m_inputWindowMgr(0)
	, m_screenWidth(0)
	, m_screenHeight(0)
	, m_uiElementsGroup(0)
	, m_reticle(0)
	, m_bootingUp(true)
	, m_runningProgress(false)
	, m_progressAnim(0)
	, m_uiRotationMode(RotationMode_FreeRotation)
    , m_orientation(OrientationEvent::Orientation_Up)
    , m_currentUiOrientation(OrientationEvent::Orientation_Up)
    , m_pendingOrientation(OrientationEvent::Orientation_Up)
	, m_pendingRotationType(Rotation_RotateAndCrossFade)
	, m_screenShotImagesValid(false)
	, m_beforePixItem(0)
	, m_afterPixItem(0)
	, m_rotationImageBeforePtr(0)
	, m_rotationImageAfterPtr(0)
	, m_rotationAnim(0)
    , m_cachedFocusedItem(0)
	, m_inRotationAnimation(Rotation_NoAnimation)
	, m_fingerDownOnScreen(false)
#ifdef DEBUG_RECORD_PAINT
	, m_paintTrace(NULL)
	, m_paintTraceFile(QLatin1String("/media/internal/lsm_paint_trace.csv"))
#endif
{
	s_instance = this;

	std::string imagePath = Settings::LunaSettings()->lunaSystemResourcesPath;
	imagePath += "/hp-logo.png";
	m_bootupScreen = QPixmap(qFromUtf8Stl(imagePath));

	// needs to be initialized once per process
	HostBase* host = HostBase::instance();
	EventReporter::init(host->mainLoop());

	const HostInfo& info = host->getInfo();

	m_screenWidth = info.displayWidth;
	m_screenHeight = info.displayHeight;

	if(info.displayWidth > info.displayHeight) {
		m_deviceIsPortraitType = false;
	} else {
		m_deviceIsPortraitType = true;
	}

	setAttribute(Qt::WA_AcceptTouchEvents);

	//suppress painting until the bootup animation finishes

	QWidget* viewportWidget = 0;

#if defined(HAVE_OPENGL)
	if (!Settings::LunaSettings()->forceSoftwareRendering) {
		QGLFormat glFormat;
		glFormat.setSamples(0);
		glFormat.setSampleBuffers(false);
		glFormat.setRedBufferSize(info.displayRedLength);
		glFormat.setGreenBufferSize(info.displayGreenLength);
		glFormat.setBlueBufferSize(info.displayBlueLength);
		if (info.displayAlphaLength > 0) {
			glFormat.setAlpha(true);
			glFormat.setAlphaBufferSize(info.displayAlphaLength);
		}
		QGLFormat::setDefaultFormat(glFormat);
		QGLWidget* glWidget = new QGLWidget(glFormat);
		glWidget->makeCurrent();
		viewportWidget = glWidget;
	}
#endif

	if (!viewportWidget)
		viewportWidget = new QWidget;

	QGestureRecognizer::registerRecognizer(new SingleClickGestureRecognizer);

	viewportWidget->setAttribute(Qt::WA_AcceptTouchEvents);
	viewportWidget->setAttribute(Qt::WA_OpaquePaintEvent, true);
	viewportWidget->setAttribute(Qt::WA_NoSystemBackground, true);
	viewportWidget->setAutoFillBackground(false);

	QFont widgetFont("Prelude", 12);
	widgetFont.setPixelSize(12);
	viewportWidget->setFont(widgetFont);

	QGraphicsScene* scene = new QGraphicsScene(0, 0, m_screenWidth, m_screenHeight);
	setScene(scene);

	scene->setStickyFocus(true);

	setBackgroundBrush(Qt::black);
	setFixedSize(m_screenWidth, m_screenHeight);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setFrameStyle(QFrame::NoFrame);

	setRenderHints(QPainter::Antialiasing);
	setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
	setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
	scene->setItemIndexMethod(QGraphicsScene::NoIndex);

	viewportWidget->grabGesture(Qt::TapGesture);
	viewportWidget->grabGesture(Qt::TapAndHoldGesture);
	viewportWidget->grabGesture(Qt::PinchGesture);
	viewportWidget->grabGesture((Qt::GestureType) SysMgrGestureFlick);
	viewportWidget->grabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	viewportWidget->grabGesture((Qt::GestureType) SysMgrGestureScreenEdgeFlick);

	m_uiRootItem.setBoundingRect(QRectF(-SystemUiController::instance()->currentUiWidth()/2, -SystemUiController::instance()->currentUiHeight()/2,
						         SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight()));

	m_displayMgr = new DisplayManager();
    m_displayMgr->pushDNAST ("startup-sequence");
	m_inputMgr = new InputManager();
	m_metaKeyMgr = new MetaKeyManager();

	m_coreNaviMgr = CoreNaviManager::instance();

    m_orientation = OrientationEvent::Orientation_Up;

    // The Qt QPA system and/or the XCB QPA has trouble with reparenting nested OpenGL widgets. It will destroy the platform
    // context before reparenting without recreating it. We can avoid this by setting the viewport only after we move this widget
    // into the layout through setCentralWidget. Just working around a XCB/QPA bug.
	HostBase::instance()->setCentralWidget(this);
    setViewport(viewportWidget);

	m_uiElementsGroup = new QGraphicsItemGroup;
	m_uiElementsGroup->setZValue(1000);
	scene->addItem(m_uiElementsGroup);

    if (Settings::LunaSettings()->showReticle) {
    	m_reticle = new ReticleItem;
		m_uiElementsGroup->addToGroup(m_reticle);
    }

	m_uiElementsGroup->addToGroup(new TouchToShareGlow);
	m_uiElementsGroup->addToGroup(new WSOverlayScreenShotAnimation);
	
	m_resizePendingTimer.setInterval(kResizePendingTickIntervalInMS);
	m_resizePendingTimer.setSingleShot(false);
	connect(&m_resizePendingTimer, SIGNAL(timeout()), SLOT(slotResizePendingTimerTicked()));

	m_deferredNewOrientationTimer.setInterval(kDeferredNewOrientationIntervalMs);
	m_deferredNewOrientationTimer.setSingleShot(true);
	connect(&m_deferredNewOrientationTimer, SIGNAL(timeout()),
			SLOT(slotDeferredNewOrientation()));

    connect(Preferences::instance(),SIGNAL(signalRotationLockChanged(OrientationEvent::Orientation)), SLOT(slotRotationLockChanged(OrientationEvent::Orientation)));


    m_currentUiOrientation = OrientationEvent::Orientation_Up;
    setOrientation(OrientationEvent::Orientation_Up);

	s_globalPropsBuffer = PIpcBuffer::create(sizeof(SharedGlobalProperties));
	::memset(s_globalPropsBuffer->data(), 0, sizeof(SharedGlobalProperties));
	WebAppMgrProxy::instance()->setGlobalProperties(s_globalPropsBuffer->key());

#ifdef DEBUG_RECORD_PAINT
#if 0
	initBaseLineTimespec();
#endif

	if (m_paintTraceFile.exists())
		m_paintTraceFile.remove();

	if (m_paintTraceFile.open(QIODevice::WriteOnly)) {
		m_paintTrace = new QTextStream(&m_paintTraceFile);
		vsyncDisabled = DisplayManager::isVsyncOff();
		*m_paintTrace << "vsync " << (vsyncDisabled ? "disabled" : "enabled") << "\n";
	}
#endif
}

#ifdef DEBUG_RECORD_PAINT
static qreal now()
{
	struct timespec tp;
#if 0
	clock_gettime(CLOCK_REALTIME, &tp);
	return (tp.tv_sec - baseLineTimespec().tv_sec) * 1000 + (tp.tv_nsec - baseLineTimespec().tv_nsec) / 1000000.0;
#else
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec * 1000 + tp.tv_nsec / 1000000.0;
#endif
}

static void mark(struct timespec& ts)
{
	clock_gettime(CLOCK_MONOTONIC, &ts);
}

static qreal elapsedMS(const struct timespec& ts)
{
	struct timespec ts_now;
	mark(ts_now);
	return (ts_now.tv_sec - ts.tv_sec) * 1000 + (ts_now.tv_nsec - ts.tv_nsec) / 1000000.0;
}

void WindowServer::tracePaint(qreal durationMs)
{
	if (m_paintTrace) {
		static int frames = 0;
		if (vsyncDisabled != DisplayManager::isVsyncOff()) {
			vsyncDisabled = DisplayManager::isVsyncOff();
			*m_paintTrace << "vsync " << (vsyncDisabled ? "disabled" : "enabled") << "\n";
		}

		qreal timestamp = now();
		*m_paintTrace << timestamp << "," << durationMs << "\n";
		if (frames++ == 60) {
			m_paintTrace->flush();
			frames = 0;
		}
	}
}
#endif

WindowServer::~WindowServer()
{
	delete m_displayMgr;
}

bool WindowServer::processSystemShortcut(QEvent* event)
{
	if (SystemUiController::instance()->bootFinished() &&
		(event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {

		static bool symDown = false;
		static bool altDown = false;
		static bool powerKeyDown = false;
		static bool homeKeyDown = false;
		static bool eatHomeUpKey = false;
		static bool eatPowerUpKey = false;
		static unsigned int powerKeyDownTimeStamp = 0;
		static unsigned int homeKeyDownTimeStamp = 0;

		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		switch (keyEvent->key()) {
		case Qt::Key_Alt:
			altDown = keyEvent->type() == QEvent::KeyPress;
			break;

		case Qt::Key_Control:
			symDown = keyEvent->type() == QEvent::KeyPress;
			break;

		case Qt::Key_Power: {
			powerKeyDown = keyEvent->type() == QEvent::KeyPress;
			if (powerKeyDown) {
				eatPowerUpKey = false;
				powerKeyDownTimeStamp = Time::curTimeMs();
			}
			else {
				if (eatPowerUpKey) {
					QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
					keyEvent->setModifiers(keyEvent->modifiers() | Qt::GroupSwitchModifier);
					eatPowerUpKey = false;
				}
				else {
					unsigned int curTime = Time::curTimeMs();
					if (homeKeyDown && (curTime - powerKeyDownTimeStamp) <= 3000) {
						takeAndSaveScreenShot();
						QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
						keyEvent->setModifiers(keyEvent->modifiers() | Qt::GroupSwitchModifier);
						eatHomeUpKey = true;
					}
				}
				powerKeyDownTimeStamp = 0;
			}

			return false;
		}
		case Qt::Key_CoreNavi_Home:
			homeKeyDown = keyEvent->type() == QEvent::KeyPress;
			if (homeKeyDown) {
				eatHomeUpKey = false;
				homeKeyDownTimeStamp = Time::curTimeMs();
			}
			else {
				if (eatHomeUpKey) {
					QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
					keyEvent->setModifiers(keyEvent->modifiers() | Qt::GroupSwitchModifier);
					eatHomeUpKey = false;
				}
				else {
					unsigned int curTime = Time::curTimeMs();
					if (powerKeyDown && (curTime - homeKeyDownTimeStamp) <= 3000) {
						takeAndSaveScreenShot();
						QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
						keyEvent->setModifiers(keyEvent->modifiers() | Qt::GroupSwitchModifier);
						eatPowerUpKey = true;
					}
				}
				homeKeyDownTimeStamp = 0;
			}

			return false;
		default:
			break;
		}
/*
		KeyMapType key = getDetailsForQtKey(keyEvent->key(), keyEvent->modifiers());
		switch (key.qtKey) {

		case Qt::Key_P: {
			// take a screenshot
			if (altDown && symDown && keyEvent->type() == QEvent::KeyPress) {

				// consume key but don't take a consecutive screen shot
				if (keyEvent->isAutoRepeat())
					return true;

				takeAndSaveScreenShot();
				return true;
			}
			break;
		}
		case Qt::Key_U: 
			// force the device into MSM
			if (altDown && symDown && keyEvent->type() == QEvent::KeyPress) {
				SystemService::instance()->enterMSM();
				return true;
			}
			break;
		case Qt::Key_W: {
#if defined(TARGET_DEVICE) && !defined(HAS_QPA)			
			if (altDown && symDown && keyEvent->type() == QEvent::KeyPress) {
				static bool doTouchpanelRecording = false;
				QWSHiddTpHandler* handler = static_cast<QWSHiddTpHandler*>(QWSServer::mouseHandler());
				if (!doTouchpanelRecording) {	
					g_message("touch input recording started");
					doTouchpanelRecording = true;
					handler->startRecording();
					enableOverlay("TouchInput recording. Press Sym+Opt+w to stop...");
				}
				else {
					doTouchpanelRecording = false;
					handler->stopRecording();
					disableOverlay();
					g_message("touch input recording stopped");
				}
			}
#endif			
			break;
		}
		default:
			break;
		}*/
	}
	return false;
}

bool WindowServer::eventFilter(QObject *obj, QEvent *event)
{
	// Key events are filtered here. Mouse events are filtered by viewportEvent
	if (this == obj && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease ||
			event->type() > QEvent::User)) {
		return sysmgrEventFilters(event);
	}
	return false;
}

bool WindowServer::viewportEvent(QEvent* event)
{
//	QTime paintEventDuration;
#ifdef DEBUG_RECORD_PAINT
	struct timespec paintStart;
#endif

	if(event->type() == QEvent::TouchBegin) {
		m_fingerDownOnScreen = true;
	}

	if(event->type() == QEvent::TouchEnd) {
		m_fingerDownOnScreen = false;
        Q_EMIT signalTouchesReleasedFromScreen();
	}

	switch (event->type()) {
	case QEvent::MouseButtonRelease:
		if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
			// hook for automated performance testing
//			g_message("SYSMGR PERF: PEN UP appid: %s, processid: %s, type: %s, time: %d",
//				"unknown", "unknown", "unknown", Time::curTimeMs());
		}
		if (sysmgrEventFilters(event)) {
			return true;
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseMove:
		if (sysmgrEventFilters(event)) {
			return true;
		}
		break;
	case QEvent::Gesture: {
		if (sysmgrEventFilters(event)) {
			return true;
		}
		gestureEvent(static_cast<QGestureEvent*>(event));
		break;
	}
	case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
	case QEvent::TouchEnd: {
		if(G_UNLIKELY(s_TouchPlot)) {

			QTouchEvent* te = static_cast<QTouchEvent*>(event);

			s_TouchPlot->updateTouches(te);

			scene()->update();

		}

#if defined (DEBUG_TOUCH_EVENTS)
		g_debug ("Received touch event %p type %d isAccepted %d", event, event->type(), event->isAccepted());
		QTouchEvent* te = static_cast<QTouchEvent*>(event);
		QList<QTouchEvent::TouchPoint>::const_iterator it;
		for (it = te->touchPoints().begin(); it != te->touchPoints().end(); ++it)
		    g_debug ("\tTouchpoint pos (%f, %f) screen pos (%f, %f)\n",
			    (*it).pos().x(), (*it).pos().y(),
			    (*it).screenPos().x(), (*it).screenPos().y());
#endif
		if (sysmgrEventFilters(event)) {
			return true;
		}
		break;
	}
	case QEvent::Paint: {

#ifdef DEBUG_RECORD_PAINT
		if (m_paintTrace)
			mark(paintStart);
#endif

		if (G_UNLIKELY(m_bootingUp)) {
#if !defined(TARGET_DEVICE)
			paintBootupScreen();
#endif
			return true;
		}

#if false && defined(HAVE_OPENGL)
		if (m_timeSinceLastPaint.isNull()) {
			connect(&m_unaliasPaintEvent, SIGNAL(timeout()), SLOT(repaint()));
			m_unaliasPaintEvent.setSingleShot(true);
		} else if (m_timeSinceLastPaint.elapsed() < kMinPaintInterval * 2) {
			if (!m_unaliasPaintEvent.isActive()) {
				m_unaliasPaintEvent.start(kMinPaintInterval - m_timeSinceLastPaint.elapsed() * 2);
			}
			return true;
		}
		m_timeSinceLastPaint.start();
#endif

		if (G_UNLIKELY(s_fpsCounter)) {

			static uint32_t startTime = Time::curTimeMs();
			static std::vector<int> frameInterval;
			static int frameCount = 0;
			const uint32_t interval = 100;

			uint32_t currTime = Time::curTimeMs();

			if ((currTime - startTime) > interval) {
				if (frameCount > 0) {
					int fps = (frameCount * 1000.0) / (currTime - startTime);
				    	int std_deviation = 0;

				    	if ( fps > 0 && frameInterval.size() > 1 ) {
						int ideal_ms = 1000 / fps;
						int sum = 0;

						for (size_t i = 1; i < frameInterval.size(); i++) {
					    		int part = (int)(frameInterval[i] - frameInterval[i-1]) - ideal_ms;
					    		sum += part*part;
						}

						std_deviation = (int)(::sqrt( sum / (frameInterval.size() - 1)));
				    	}
					frameInterval.clear();
					s_fpsCounter->setFPS(fps, std_deviation, currTime);
				}

				startTime = currTime;
				frameCount = 0;
			}
			else {
				frameInterval.push_back(currTime);
				frameCount++;
			}

//			paintEventDuration.start();
		}
	}
	default:
		break;
	}

	bool result =  QGraphicsView::viewportEvent(event);

	switch (event->type()) {
		case QEvent::Paint: {
#ifdef DEBUG_RECORD_PAINT
			if (m_paintTrace)
				tracePaint(elapsedMS(paintStart));
#endif

			HostBase::instance()->flip();

			break;
		}
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		case QEvent::TouchEnd: {
			event->setAccepted (true);
#if defined(DEBUG_TOUCH_EVENTS)
			g_debug ("Accepted touch event %p type %d isAccepted %d", event, event->type(), event->isAccepted());
			QTouchEvent* te = static_cast<QTouchEvent*>(event);
			QList<QTouchEvent::TouchPoint>::const_iterator it;
			for (it = te->touchPoints().begin(); it != te->touchPoints().end(); ++it)
				g_debug ("Touchpoint pos (%f, %f)\n", (*it).pos().x(), (*it).pos().y());
#endif
			return true;
		}
		default:
			break;
	}

	return result;
}

bool WindowServer::handleEvent(QEvent* event)
{
	static bool aAltDown = false;
	static bool sShiftDown = false;
	static bool capsLock = false;

	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
#if 0	// to help debug key events
			if (keyEvent->key() >= Qt::Key_Escape)
				g_warning("WindowServer::handleEvent: KeyPress %0x, text: '%s' mod: %x", keyEvent->key(), keyEvent->text().toUtf8().data(), (int) keyEvent->modifiers());
			else
				g_warning("WindowServer::handleEvent: KeyPress %0x: '%s', text: '%s' mod: %x", keyEvent->key(), QString(keyEvent->key()).toUtf8().data(), keyEvent->text().toUtf8().data(), (int) keyEvent->modifiers());
#endif
			if (keyEvent->key() == Qt::Key_Shift ) {
				if (capsLock) {
					g_debug ("Dropping shift key if CapsLock is on, required to offset webkits shifting logic");
					return true;
				}
				if (sShiftDown)
					return true;
				sShiftDown = true;
			}
			else if (keyEvent->key() == Qt::Key_Alt) {
				if (aAltDown)
					return true;
				aAltDown = true;
			}
			else if (keyEvent->key() == Qt::Key_CapsLock) {
				capsLock = !capsLock;
				return true;
			}
            else if (keyEvent->key() == Qt::Key_Power && keyEvent->isAutoRepeat()) {
                // safeguard against repeat power key events (BT keyboards)
		    return true;
            }
		}
		break;

	case QEvent::KeyRelease:
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent->key() == Qt::Key_Shift)
				sShiftDown = false;
			else if (keyEvent->key() == Qt::Key_Alt)
				aAltDown = false;
		}
		break;

    case (QEvent::Type) orientationEventType:
    {
        OrientationEvent* orientationEvent = static_cast<OrientationEvent *>(event);
        // g_message ("Accelerometer event: x: %1.4f, y: %1.4f, z: %1.4f, orientation: %d\n", accelEvent->x(), accelEvent->y(), accelEvent->z(), accelEvent->orientation());
        OrientationEvent::Orientation newOrientation = OrientationEvent::Orientation_Invalid;

        if (newOrientation != OrientationEvent::Orientation_FaceUp &&
            newOrientation != OrientationEvent::Orientation_FaceDown)
        {
            // TODO: do we care about face up/down ?
            newOrientation = orientationEvent->orientation();
        }

        if (newOrientation != OrientationEvent::Orientation_Invalid &&
            m_deferredNewOrientation != newOrientation)
        {
            m_deferredNewOrientation = newOrientation;
            m_deferredNewOrientationTimer.start();
        }
        break;
    }

	default:
		break;
	}
	return processSystemShortcut(event);
}

void WindowServer::slotDeferredNewOrientation()
{
    if (m_deferredNewOrientation == OrientationEvent::Orientation_Invalid)
		return;

	if (m_deferredNewOrientation != m_orientation) {
		WebAppMgrProxy::instance()->setOrientation(m_deferredNewOrientation);
		setOrientation(m_deferredNewOrientation);
	}
}

void WindowServer::resizeWindowManagers(int width, int height)
{

}

QRectF WindowServer::mapRectToRoot(const QGraphicsItem* item, const QRectF& rect) const
{
    return (item ? item->mapRectToScene(rect) : rect);
}

void WindowServer::prepareAddWindow(Window* win)
{

}

void WindowServer::addWindowTimedOut(Window* win)
{

}

void WindowServer::addWindow(Window* win)
{

}

void WindowServer::removeWindow(Window* win)
{

}

void WindowServer::focusWindow(Window* win)
{

}

void WindowServer::unfocusWindow(Window* win)
{

}

#ifndef GL_BGRA
#define GL_BGRA	0x80E1
#endif

bool WindowServer::takeScreenShot(const char* filePath)
{
	QImage screenShot(getScreenShotImage());
	screenShot.setDotsPerMeterY(screenShot.dotsPerMeterX());
	bool ret = screenShot.save(filePath);
	
	if (ret)
		g_warning("Screenshot: Wrote %s", filePath);
	else
		g_warning("Screenshot: FAILED to write %s", filePath);

	return ret;
}

QPixmap* WindowServer::takeScreenShot()
{
	QPixmap* pix = new QPixmap(m_screenWidth, m_screenHeight);
	QPainter painter(pix);
	render(&painter);
	painter.end();
	return pix;
}
void WindowServer::shutdown()
{
	WebAppMgrProxy::instance()->postShutdownEvent();

	QApplication::instance()->quit();
}

void WindowServer::startProgressAnimation(ProgressAnimation::Type type)
{
	if (m_progressAnim)
		return;

	m_progressAnim = new ProgressAnimation(type);
	m_progressAnim->setParentItem(&m_uiRootItem);
	m_progressAnim->setPos(0,0);

	connect(m_progressAnim, SIGNAL(signalProgressAnimationCompleted()),
			SLOT(slotProgressAnimationCompleted()));

	m_progressAnim->start();

	m_runningProgress = true;

	m_displayMgr->pushDNAST("progress-sequence");
}

void WindowServer::stopProgressAnimation()
{
	if (!m_progressAnim)
		return;

	m_progressAnim->stop();
}

void WindowServer::slotProgressAnimationCompleted()
{
	m_progressAnim->setVisible(false);
	m_progressAnim->deleteLater();
	m_progressAnim = 0;

	m_runningProgress = false;

	m_displayMgr->popDNAST("progress-sequence");
}

void WindowServer::startDrag(int x, int y, void* imgRef, const std::string& lpid)
{
}

void WindowServer::endDrag(int x, int y, const std::string& lpid, bool handled)
{
}

static void activateSuspend()
{
	int fd;
	fd = open("/tmp/suspend_active", O_RDWR | O_CREAT,
			  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < 0) {
		g_critical("Could not activate suspend: Could not create /tmp/suspend_active");
	}
	else {
		const int len = 64;
		char buf[len];
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		int numChars = snprintf(buf, len, "%ld\n", ts.tv_sec);
        if (numChars > 0) {
            ssize_t result = write(fd, buf, numChars);
            Q_UNUSED(result);
        }

		close(fd);
	}
}

OrientationEvent::Orientation WindowServer::getInitialDeviceOrientation()
{
	int angle = Settings::LunaSettings()->homeButtonOrientationAngle;
	switch (angle) {
		case 90:
		{
            return OrientationEvent::Orientation_Left;
		}
		break;

		case -180:
		case 180:
		{
            return OrientationEvent::Orientation_Down;
		}
		break;

		case -90:
		case 270:
		{
            return OrientationEvent::Orientation_Right;
		}
		break;

		default:
            return OrientationEvent::Orientation_Up;
	}
}

void WindowServer::bootupFinished()
{
    if(Preferences::instance()->rotationLock() == OrientationEvent::Orientation_Invalid) {
        OrientationEvent::Orientation initialOrientation = getInitialDeviceOrientation();
		setUiOrientation(initialOrientation, Rotation_NoAnimation);
		WebAppMgrProxy::instance()->setOrientation(initialOrientation);
		setOrientation(initialOrientation);
	} else {
        OrientationEvent::Orientation rotationLock= Preferences::instance()->rotationLock();
		if(rotationLock != m_currentUiOrientation) {
			m_pendingOrientation = rotationLock;
			setUiOrientation(rotationLock, Rotation_NoAnimation);
			WebAppMgrProxy::instance()->setOrientation(rotationLock);
			setOrientation(rotationLock);
		}
	}
    m_deferredNewOrientation = m_orientation;

#if defined(TARGET_DEVICE)
	struct timespec timeThen, timeNow;
	clock_gettime(CLOCK_MONOTONIC, &timeThen);

	sync();

	clock_gettime(CLOCK_MONOTONIC, &timeNow);
	g_warning("Sync at end of sysmgr bootup took: %ld seconds",
			  timeNow.tv_sec - timeThen.tv_sec);
#endif

	m_bootingUp = false;
	m_bootupScreen = QPixmap();
#if defined(TARGET_DEVICE)
//	BootupAnimation::stopBootupAnimation();
#endif

/*	BootupAnimationTransition* anim = new BootupAnimationTransition();
	scene()->addItem(anim);
	// put it on top
	anim->setZValue(100);
	anim->setPos(m_screenWidth/2,m_screenHeight/2);
	anim->setVisible(true);
	anim->start();
*/
	update();
	viewport()->update(); // need to update the viewport since the boot screen was rendered to it

	activateSuspend();

    m_displayMgr->popDNAST ("startup-sequence");

	markBootFinish();

	SystemUiController::instance()->setBootFinished();

	SoundPlayerPool::instance()->play("/usr/palm/sounds/boot.mp3", "notifications", false, -1);
        // Emit LunaSysMgr-ready to upstart
       system("/sbin/initctl emit --no-wait LunaSysMgr-ready");

}


static struct timespec s_bootStartTime;
static struct timespec s_bootFinishTime;

void WindowServer::markBootStart()
{
    clock_gettime(CLOCK_MONOTONIC, &s_bootStartTime);
}

void WindowServer::markBootFinish()
{
    clock_gettime(CLOCK_MONOTONIC, &s_bootFinishTime);

	g_warning("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ BOOT TOOK: %g seconds",
			  (s_bootFinishTime.tv_sec - s_bootStartTime.tv_sec) * 1.0 +
			  (s_bootFinishTime.tv_nsec - s_bootStartTime.tv_nsec) / 1000000000.0);
}

void WindowServer::windowUpdated(Window* win)
{
}

void WindowServer::setPaintingDisabled(bool val)
{
	if (val == !updatesEnabled())
		return;

	if (val) {

		g_message("Painting disabled");
		setUpdatesEnabled(false);
		viewport()->setUpdatesEnabled(false);
	}
	else {

		g_message("Painting enabled");
		setUpdatesEnabled(true);
		viewport()->setUpdatesEnabled(true);

		for (int i = 0; i < 2; i++)
			viewport()->repaint();

		update();
		viewport()->update();
	}
}

void WindowServer::registerWindow(Window* win)
{
	s_registeredWindows.insert((unsigned long) win);
}

void WindowServer::unregisterWindow(Window* win)
{
	s_registeredWindows.erase((unsigned long) win);
}

bool WindowServer::windowIsRegistered(Window* win)
{
	return (s_registeredWindows.find((unsigned long) win) != s_registeredWindows.end());
}

void WindowServer::cancelVibrations()
{
	HapticsController::instance()->cancelAll();
}

void WindowServer::setWindowProperties(Window* win, const WindowProperties& props)
{
    if (!win)
		return;

	switch (win->type()) {
	case (Window::Type_Card):
	case (Window::Type_ChildCard):
	case (Window::Type_Emergency):
	case (Window::Type_Dashboard):
		break;
	default:
		return;
	}

	win->setWindowProperties(props);

    SystemUiController::instance()->updateProperties(win, props);
}

void WindowServer::applyLaunchFeedback(int centerX, int centerY)
{
}

void WindowServer::showReticle(const QPoint& pos)
{
	if (m_reticle)
		m_reticle->startAt(pos);
}

void WindowServer::gestureEvent(QGestureEvent* event)
{
    if (QGesture* t = event->gesture(Qt::TapGesture)) {
		QTapGesture* tap = static_cast<QTapGesture*>(t);
		if (tap->state() == Qt::GestureFinished) {
			if (!m_inputWindowMgr || m_inputWindowMgr->doReticle(mapToScene(mapFromGlobal(t->hotSpot().toPoint()))))
				showReticle(mapFromGlobal(t->hotSpot().toPoint()));
		}
    }
}

void WindowServer::paintBootupScreen()
{
	QPainter p(viewport());
	p.fillRect(0, 0, m_screenWidth, m_screenHeight, QColor(0x00, 0x00, 0x00, 0xFF));

	if (!m_bootupScreen.isNull()) {
		int w = m_bootupScreen.width();
		int h = m_bootupScreen.height();
		int x = (m_screenWidth - w) / 2;
		int y = (m_screenHeight - h) / 2;
		p.drawPixmap(x, y, w, h, m_bootupScreen);
	}

	p.end();
}

void WindowServer::enableFpsCounter(bool enable)
{
	if (enable) {
		if (s_fpsCounter)
			return;

		s_fpsCounter = new FpsCounter();
		QRectF rect = s_fpsCounter->boundingRect();

		WindowServer* ws = WindowServer::instance();
		s_fpsCounter->setPos(rect.width()/2, ws->m_screenHeight - s_fpsCounter->boundingRect().height()/2);
		ws->m_uiElementsGroup->addToGroup(s_fpsCounter);
	}
	else {
		delete s_fpsCounter;
		s_fpsCounter = NULL;
	}
}

void WindowServer::dumpFpsHistory()
{
	if (s_fpsCounter){
		s_fpsCounter->dumpFPS();
	}
}

void WindowServer::resetFpsBuffer(int newBufSize)
{

	if (s_fpsCounter){
		s_fpsCounter->resetFpsBuffer(newBufSize);
	}

}

void WindowServer::enableTouchPlotOption(TouchPlot::TouchPlotOption_t type, bool enable)
{
	if(enable && !s_TouchPlot)
	{
		s_TouchPlot = new TouchPlot();

		WindowServer* ws = WindowServer::instance();

		s_TouchPlot->setPos(0,0);
		s_TouchPlot->setSize(ws->m_screenWidth, ws->m_screenHeight);

		ws->m_uiElementsGroup->addToGroup(s_TouchPlot);
	}

	s_TouchPlot->enableTouchPlotOption(type, enable);

	if(!s_TouchPlot->hasOptionsEnabled())
	{
		delete s_TouchPlot;
		s_TouchPlot = NULL;
	}
}

//virtual
WindowManagerBase* WindowServer::getWindowManagerByClassName(const QString& undecoratedClassName) const
{
	//search the multimap for key = undecoratedClassName. If there is more than 1 result (can't happen at the current
	// Dfish version of sysmgr (dec.2010) since only 1 window manager of a type can exist), then pick the first non-null
	// result.

	WindowManagerMapConstIter f = m_windowManagerMap.constFind(undecoratedClassName);
	for (;(f != m_windowManagerMap.constEnd()) && (f.key() ==  undecoratedClassName);f++)
	{
		WindowManagerBase * p = f.value();
		if (p)
		{
			//non null (so it hasn't been nuked in the meantime w/o being cleared from the map)
			return f.value();
		}
	}
	return NULL;
}

void WindowServer::enableOverlay(const QString& str)
{
    if (s_overlay) {
		s_overlay->setString(str);
		return;
	}

	s_overlay = new WindowServerOverlay();
	QRectF rect = s_overlay->boundingRect();

	s_overlay->setPos(rect.width()/2, m_screenHeight - rect.height()/2);
	m_uiElementsGroup->addToGroup(s_overlay);

	s_overlay->setString(str);
}

void WindowServer::disableOverlay()
{
	delete s_overlay;
    s_overlay = 0;
}

struct ScreenShotParameter
{
	QImage image;
	std::string filePath;
    OrientationEvent::Orientation orientation;
};

static gpointer PrvSaveScreenShot(gpointer data)
{
	if (!data)
		return 0;

	::prctl(PR_SET_NAME, "ScreenShotSaver", 0, 0, 0);
	
	// Low priority thread
	::setpriority(PRIO_PROCESS, ::getpid(), 5);
	
	ScreenShotParameter* param = (ScreenShotParameter*) data;

	int angle = 0;
	
	switch (param->orientation) {
    case OrientationEvent::Orientation_Down:
		angle = 180;
		break;				
    case OrientationEvent::Orientation_Right:
		angle = 90;
		break;
    case OrientationEvent::Orientation_Left:
		angle = -90;
		break;
    case OrientationEvent::Orientation_Up:
	default:
		angle = 0;
		break;
	}

	if (angle != 0) {
		QTransform trans;
		trans.rotate(angle);
		param->image = param->image.transformed(trans);
	}

	param->image.setDotsPerMeterY(param->image.dotsPerMeterX());
	param->image.save(param->filePath.c_str());
	delete param;

	return 0;
}

void WindowServer::takeAndSaveScreenShot()
{
	Q_EMIT signalAboutToTakeScreenShot();

	viewport()->repaint();

	std::string fullPath;
	while (1) {
		std::string appName = SystemUiController::instance()->activeApplicationName();
		size_t i = appName.rfind('.');
		if (i > 0)
			appName = appName.substr(i+1);

		time_t result = time(NULL);
		struct tm* t = localtime(&result);
		char buffer[64];
		sprintf(buffer, "_%04d-%02d-%02d_%02d%02d%02d",
				t->tm_year + 1900, t->tm_mday, t->tm_mon+1,
				t->tm_hour, t->tm_min, t->tm_sec);

		fullPath = Settings::LunaSettings()->lunaScreenCapturesPath + "/" + appName + std::string(buffer) + ".png";
		if (!g_file_test(fullPath.c_str(), G_FILE_TEST_EXISTS)) {
			break;
		}
	}

	SoundPlayerPool::instance()->playFeedback(Settings::LunaSettings()->lunaSystemSoundScreenCapture);
	QImage image(getScreenShotImageFromFb());

	Q_EMIT signalTookScreenShot();
	
	ScreenShotParameter* param = new ScreenShotParameter;
	param->image = image;
	param->filePath = fullPath;
	param->orientation = getUiOrientation();
	
	g_thread_create(PrvSaveScreenShot, param, FALSE, NULL);
}

QImage WindowServer::getScreenShotImage()
{
#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
	QGLContext* gc = (QGLContext*) QGLContext::currentContext();
	luna_assert(gc);

	QGLFramebufferObject fbo(m_screenWidth, m_screenHeight);
	fbo.bind();

	QPainter painter(&fbo);
	render(&painter);
	painter.end();

	QImage screenShot = fbo.toImage();
	fbo.release();

#else
	
	QPixmap pix(m_screenWidth, m_screenHeight);
	QPainter painter(&pix);
	render(&painter);
	painter.end();

	QImage screenShot = pix.toImage();

#endif

	QImage appScreenShot = HostBase::instance()->takeAppDirectRenderingScreenShot();

	if (!appScreenShot.isNull()) {

		QPainter painter(&appScreenShot);
		painter.drawImage(0, 0, screenShot);
		painter.end();
		screenShot = appScreenShot;
	}

	return screenShot;
}

QImage WindowServer::getScreenShotImageFromFb()
{
	QImage screenShot = HostBase::instance()->takeScreenShot();
	QImage appScreenShot = HostBase::instance()->takeAppDirectRenderingScreenShot();

	if (!appScreenShot.isNull()) {

		QPainter painter(&appScreenShot);
		painter.drawImage(0, 0, screenShot);
		painter.end();
		screenShot = appScreenShot;
	}

	return screenShot;	
}

int WindowServer::angleForOrientation(OrientationEvent::Orientation orient)
{
	switch(orient) {
        case OrientationEvent::Orientation_Up: {
			return 0;
		}
		break;
        case OrientationEvent::Orientation_Left: {
			return 90;
		}
		break;
        case OrientationEvent::Orientation_Down: {
			return 180;
		}
		break;
        case OrientationEvent::Orientation_Right: {
			return 270;
		}
		break;
		default: {
			return -1;
		}
	}
}

bool WindowServer::isOrientationAllowed(OrientationEvent::Orientation newOrient)
{
	if(m_uiRotationMode == RotationMode_FreeRotation) {
		return true;

	}

	if(m_uiRotationMode == RotationMode_LimitedLandscape){
		if(m_deviceIsPortraitType) {
            if((newOrient == OrientationEvent::Orientation_Left) || (newOrient == OrientationEvent::Orientation_Right)) {
				return true;
			}
		} else {
            if((newOrient == OrientationEvent::Orientation_Up) || (newOrient == OrientationEvent::Orientation_Down)) {
				return true;
			}
		}
	}

	if(m_uiRotationMode == RotationMode_LimitedPortrait){
		if(!m_deviceIsPortraitType) {
            if((newOrient == OrientationEvent::Orientation_Left) || (newOrient == OrientationEvent::Orientation_Right)) {
				return true;
			}
		} else {
            if((newOrient == OrientationEvent::Orientation_Up) || (newOrient == OrientationEvent::Orientation_Down)) {
				return true;
			}
		}
	}

    if((m_uiRotationMode == RotationMode_FixedPortrait) && (newOrient == OrientationEvent::Orientation_Up)) {
		return true;
	}

    if((m_uiRotationMode == RotationMode_FixedPortraitInverted) && (newOrient == OrientationEvent::Orientation_Down)) {
		return true;
	}

    if((m_uiRotationMode == RotationMode_FixedLandscape) && (newOrient == OrientationEvent::Orientation_Left)) {
		return true;
	}
    if((m_uiRotationMode == RotationMode_FixedLandscapeInverted) && (newOrient == OrientationEvent::Orientation_Right)) {
		return true;
	}
	return false;
}

void WindowServer::setOrientation(OrientationEvent::Orientation newOrient)
{
    bool post = m_orientation != newOrient;

	m_orientation = newOrient;

	setUiOrientation(newOrient);
	Runtime::instance()->setOrientation (newOrient);

	if (post)
		SystemService::instance()->postSystemStatus();
}

void WindowServer::setUiOrientation(OrientationEvent::Orientation newOrient, UiRotationAnimationType animation)
{
	if(Settings::LunaSettings()->displayUiRotates) {
		if(newOrient == m_currentUiOrientation) {
			if(m_resizePendingTimer.isActive())
				m_resizePendingTimer.stop();
			m_pendingOrientation = m_currentUiOrientation;
			m_pendingRotationType = Rotation_RotateAndCrossFade;
			return;
		}

        if((newOrient != OrientationEvent::Orientation_Up) &&
           (newOrient != OrientationEvent::Orientation_Down) &&
           (newOrient != OrientationEvent::Orientation_Left) &&
           (newOrient != OrientationEvent::Orientation_Right)) {


			if(m_pendingOrientation != m_currentUiOrientation && Preferences::instance()->rotationLock() != m_pendingOrientation) {
				// device was waiting for Rotation Lock to perform rotation, but we are no longer in an orientation
				// where we should rotate, so cancel that
				if(m_resizePendingTimer.isActive())
					m_resizePendingTimer.stop();
				m_pendingOrientation = m_currentUiOrientation;
				m_pendingRotationType = Rotation_RotateAndCrossFade;
			}
			return;
		}

        if((Preferences::instance()->rotationLock() != OrientationEvent::Orientation_Invalid) &&
		   (Preferences::instance()->rotationLock() != newOrient) &&
			!m_bootingUp && isOrientationAllowed(m_currentUiOrientation)) {
			// rotation locked, so save the new orientation and rotate when the user unlocks rotation
			m_pendingOrientation = newOrient;
			m_pendingRotationType = Rotation_RotateAndCrossFade;
			if(m_resizePendingTimer.isActive())
				m_resizePendingTimer.stop();
			return;
		} else if(!isOrientationAllowed(newOrient) && !SystemUiController::instance()->isScreenLocked()) {
			// rotation restricted by a maximized card, so save the new orientation and rotate when the card is minimized
			// note that this should not be done if the lock screen is up
			m_pendingOrientation = newOrient;
			m_pendingRotationType = Rotation_RotateAndCrossFade;
			if(m_resizePendingTimer.isActive())
				m_resizePendingTimer.stop();
			return;
		} else if((m_fingerDownOnScreen && isOrientationAllowed(m_currentUiOrientation)) || !okToResizeUi()) {
			// cannot perform the rotation right now, so start a periodic timer to trigger the rotation when possible
			m_pendingOrientation = newOrient;
			m_pendingRotationType = animation;
			if(!m_resizePendingTimer.isActive())
				m_resizePendingTimer.start();
			return;
		} else {
			if(m_resizePendingTimer.isActive())
				m_resizePendingTimer.stop();
		}

		rotateUi(newOrient, animation);

		m_pendingOrientation = newOrient;

		scene()->update();

		SystemService::instance()->postSystemStatus();
    }
}

void WindowServer::setUiRotationMode(RotationMode uiRotationMode, bool cardMaximizing, bool skipAnimation)
{
	m_uiRotationMode = uiRotationMode;

	if(m_uiRotationMode == RotationMode_NoRotation)
		return;

	// rotation type for when maximizing cards with fixed orientation
	UiRotationAnimationType maximizeAnimType = !cardMaximizing ? Rotation_RotateAndCrossFade : (!skipAnimation ? Rotation_CrossFadeOnly : Rotation_NoAnimation);

	// adjust orientation if necessary

	if(m_uiRotationMode == RotationMode_FreeRotation) {
        OrientationEvent::Orientation fixedOrient = Preferences::instance()->rotationLock();
        if((fixedOrient != OrientationEvent::Orientation_Invalid) && (fixedOrient != m_currentUiOrientation)) {
		        setUiOrientation(fixedOrient);
		    	m_pendingOrientation = m_orientation;
				WebAppMgrProxy::instance()->setOrientation(fixedOrient);
				setOrientation(fixedOrient);
		} else {
			if(m_orientation != m_currentUiOrientation) {
                if((m_orientation == OrientationEvent::Orientation_Up) ||
                   (m_orientation == OrientationEvent::Orientation_Left) ||
                   (m_orientation == OrientationEvent::Orientation_Right) ||
                   (m_orientation == OrientationEvent::Orientation_Down)) {
					setUiOrientation(m_orientation);
				}
			}
		}
	} else if(m_uiRotationMode == RotationMode_LimitedLandscape) {
		if(SystemUiController::instance()->isUiInPortraitMode()) {
			if(m_deviceIsPortraitType) {
                if(m_orientation == OrientationEvent::Orientation_Right || m_orientation == OrientationEvent::Orientation_Down)
                    setUiOrientation(OrientationEvent::Orientation_Right, maximizeAnimType);
				else
                    setUiOrientation(OrientationEvent::Orientation_Left, maximizeAnimType);
			} else {
                if(m_orientation == OrientationEvent::Orientation_Down || m_orientation == OrientationEvent::Orientation_Left)
                    setUiOrientation(OrientationEvent::Orientation_Down, maximizeAnimType);
				else
                    setUiOrientation(OrientationEvent::Orientation_Up, maximizeAnimType);
			}
		}
	} else if(m_uiRotationMode == RotationMode_LimitedPortrait) {
		if(!SystemUiController::instance()->isUiInPortraitMode()) {
			if(m_deviceIsPortraitType) {
                if(m_orientation == OrientationEvent::Orientation_Down || m_orientation == OrientationEvent::Orientation_Left)
                    setUiOrientation(OrientationEvent::Orientation_Down, maximizeAnimType);
				else
                    setUiOrientation(OrientationEvent::Orientation_Up, maximizeAnimType);
			} else {
                if(m_orientation == OrientationEvent::Orientation_Right || m_orientation == OrientationEvent::Orientation_Down)
                    setUiOrientation(OrientationEvent::Orientation_Right, maximizeAnimType);
				else
                    setUiOrientation(OrientationEvent::Orientation_Left, maximizeAnimType);
			}
		}
    } else if((m_uiRotationMode == RotationMode_FixedPortrait) && (m_currentUiOrientation != OrientationEvent::Orientation_Up)) {
        setUiOrientation(OrientationEvent::Orientation_Up, maximizeAnimType);
    } else if((m_uiRotationMode == RotationMode_FixedLandscape) && (m_currentUiOrientation != OrientationEvent::Orientation_Left)) {
        setUiOrientation(OrientationEvent::Orientation_Left, maximizeAnimType);
    } else if((m_uiRotationMode == RotationMode_FixedPortraitInverted) && (m_currentUiOrientation != OrientationEvent::Orientation_Down)) {
        setUiOrientation(OrientationEvent::Orientation_Down, maximizeAnimType);
    } else if((m_uiRotationMode == RotationMode_FixedLandscapeInverted) && (m_currentUiOrientation != OrientationEvent::Orientation_Right)) {
        setUiOrientation(OrientationEvent::Orientation_Right, maximizeAnimType);
	}
}

void WindowServer::rotateUi(OrientationEvent::Orientation newOrientation, UiRotationAnimationType animation, bool forceResize)
{
	int currAngle, newAngle;
	int rotationAngle;

	if(!Settings::LunaSettings()->displayUiRotates)
		return;

	currAngle = angleForOrientation(m_currentUiOrientation);
	newAngle  = angleForOrientation(newOrientation);

	if(newAngle < 0)
		return;

	g_message("ROTATION: [%s]: ROTATION START m_currentUiOrientation = %d, newOrientation = %d, animation = %d, forceResize = %d", __PRETTY_FUNCTION__,
			  m_currentUiOrientation, newOrientation, animation, forceResize);

	rotationAngle = newAngle - currAngle;

	if(rotationAngle > 180)
		rotationAngle -= 360;
	else if(rotationAngle < -180)
		rotationAngle += 360;

	SystemUiController::instance()->rotationStarting();

	// check to see if we already have screen shots and if it is ok to reuse them this time
	if(m_screenShotImagesValid && (abs(m_animationTargetRotationAngle) != abs(rotationAngle))) {
		// screen shots from previous rotation cannot be reused
		if(m_beforePixItem) {
			m_beforePixItem->setVisible(false);
			scene()->removeItem(m_beforePixItem);
			m_beforePixItem->deleteLater();
			m_beforePixItem = 0;
		}
		if(m_afterPixItem) {
			m_afterPixItem->setVisible(false);
			scene()->removeItem(m_afterPixItem);
			m_afterPixItem->deleteLater();
			m_afterPixItem = 0;
		}
		m_screenShotImagesValid = false;
		m_uiRootItem.setVisible(true);
	}

	if(animation != Rotation_NoAnimation && !m_screenShotImagesValid) {
		SystemUiController::instance()->enableDirectRendering(false);
		//m_rotationImageBeforePtr = takeScreenShot();
		QImage fbImage = getScreenShotImageFromFb();
		m_rotationImageBeforePtr = new QPixmap(m_screenWidth, m_screenHeight);
		QPainter painter(m_rotationImageBeforePtr);
		painter.drawImage(0,0,fbImage);
		painter.end();
	}

	HostBase::instance()->setOrientation(newOrientation);
	m_currentUiOrientation = newOrientation;

	if((rotationAngle == 90) || (rotationAngle == -90)) {
		// need to resize the UI (this calls resizeWindowManagers)
		SystemUiController::instance()->resizeAndRotateUi(SystemUiController::instance()->currentUiHeight(), SystemUiController::instance()->currentUiWidth(), rotationAngle);
	} else if (forceResize) {
		SystemUiController::instance()->resizeAndRotateUi(SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight(), rotationAngle);
	}

	// rotate the UI
    m_uiRootItem.setRotation(m_uiRootItem.rotation() + rotationAngle);

	// need to update the wallpaper
	updateWallpaperForRotation(m_currentUiOrientation);

	Q_EMIT signalUiRotated();

	g_message("ROTATION: [%s]: RESIZE COMPLETE", __PRETTY_FUNCTION__);

	if(animation != Rotation_NoAnimation) {
		const HostInfo& hostInfo = HostBase::instance()->getInfo();
		int displayWidth = hostInfo.displayWidth;
		int displayHeight = hostInfo.displayHeight;

		if(!m_screenShotImagesValid) {
			m_rotationImageAfterPtr = takeScreenShot();
		}

        cacheFocusedItem();
		m_uiRootItem.setVisible(false);
        restoreCachedFocusItem();

		if(!m_screenShotImagesValid) {
			m_beforePixItem = new QGraphicsPixmapObject();
			m_beforePixItem->setPixmap(m_rotationImageBeforePtr);
			m_afterPixItem = new QGraphicsPixmapObject();
			m_afterPixItem->setPixmap(m_rotationImageAfterPtr);
			m_screenShotImagesValid = true;

			scene()->addItem(m_afterPixItem);
			m_afterPixItem->setPos(displayWidth/2, displayHeight/2);
			scene()->addItem(m_beforePixItem);
			m_beforePixItem->setPos(displayWidth/2, displayHeight/2);
		} else {
			// flip the screen shot images
			QGraphicsPixmapObject *tempBefore = m_beforePixItem;
			m_beforePixItem = m_afterPixItem;
			m_afterPixItem = tempBefore;
			m_afterPixItem->stackBefore(m_beforePixItem);
			m_afterPixItem->setInternalRotation(m_afterPixItem->internalRotation() + rotationAngle);
		}

		if (!m_rotationAnim) {
			m_rotationAnim = new VariantAnimation<WindowServer>(this, &WindowServer::rotationValueChanged);
			m_rotationAnim->setEasingCurve(QEasingCurve::InOutCubic);
			m_rotationAnim->setDuration(AS(rotationAnimationDuration));
			connect(m_rotationAnim, SIGNAL(finished()), SLOT(slotRotationAnimFinished()));
		} else {
			m_rotationAnim->stop();
		}

		m_rotationAnim->setStartValue(0);
		m_rotationAnim->setEndValue(rotationAngle);
		m_animationTargetRotationAngle = rotationAngle;

		if(animation == Rotation_RotateAndCrossFade) {
			m_afterPixItem->setRotation(-rotationAngle);
			m_afterPixItem->setOpacity(0.0);
		} else {
			m_afterPixItem->setRotation(0);
			m_afterPixItem->setOpacity(1.0);
		}

		m_beforePixItem->setRotation(0);
		m_beforePixItem->setOpacity(1.0);

		m_inRotationAnimation = animation;
		m_rotationAnim->start();
		g_message("ROTATION: [%s]: ANIMATION STARTED", __PRETTY_FUNCTION__);

	} else {
		if(m_beforePixItem) {
			delete m_beforePixItem;
			m_beforePixItem = 0;
		}
		if(m_afterPixItem) {
			delete m_afterPixItem;
			m_afterPixItem = 0;
		}

		SystemUiController::instance()->rotationComplete();
		g_message("ROTATION: [%s]: NO ANIMATION", __PRETTY_FUNCTION__);
		rotatePendingWindows();
		g_message("ROTATION: [%s]: ROTATION DONE", __PRETTY_FUNCTION__);
	}

}

void WindowServer::rotationValueChanged(const QVariant& value)
{
	int progress = value.toInt();
	if(m_inRotationAnimation == Rotation_NoAnimation)
		return;

	if(m_inRotationAnimation == Rotation_RotateAndCrossFade) {
		m_afterPixItem->setRotation(progress - m_animationTargetRotationAngle);
		m_beforePixItem->setRotation(progress);
		m_afterPixItem->setOpacity(((qreal)progress) / ((qreal)m_animationTargetRotationAngle));
	}

	m_beforePixItem->setOpacity(1.0 - ((qreal)progress) / ((qreal)m_animationTargetRotationAngle));
	//repaint();
	//update();
}

void WindowServer::slotResizePendingTimerTicked()
{
    setUiOrientation(m_pendingOrientation, m_pendingRotationType);
}           
        
void WindowServer::slotRotationLockChanged(OrientationEvent::Orientation rotationLock)
{   
	if(!m_bootingUp) {
        if((rotationLock == OrientationEvent::Orientation_Invalid) && (m_pendingOrientation != m_currentUiOrientation)) {
			setUiOrientation(m_pendingOrientation);
        } else if((rotationLock != OrientationEvent::Orientation_Invalid) && (rotationLock != m_currentUiOrientation)) {
			m_pendingOrientation = rotationLock;
			setUiOrientation(rotationLock);
			WebAppMgrProxy::instance()->setOrientation(rotationLock);
			setOrientation(rotationLock);
		}
	}
}

void WindowServer::slotRotationAnimFinished()
{
	if(m_inRotationAnimation == Rotation_NoAnimation)
		return;

	g_message("ROTATION: [%s]: ANIMATION FINISHED", __PRETTY_FUNCTION__);

	m_inRotationAnimation = Rotation_NoAnimation;

	if(m_rotationAnim) {
		delete m_rotationAnim;
		m_rotationAnim = 0;
	}
	//repaint();
//	update();

	if((m_pendingOrientation != m_currentUiOrientation) &&
       (Preferences::instance()->rotationLock() == OrientationEvent::Orientation_Invalid) && okToResizeUi(true)) {

		g_message("ROTATION: [%s]: New Rotation Request, Rotating to new Orientation", __PRETTY_FUNCTION__);
		// a new rotation request came in, so drop pending window flip requests and
		// process the new rotation request

		cancelPendingWindowRotations();

		if(m_resizePendingTimer.isActive())
			m_resizePendingTimer.stop();

		m_beforePixItem->setInternalRotation(m_beforePixItem->internalRotation() + m_beforePixItem->rotation());
		m_afterPixItem->setInternalRotation(m_afterPixItem->internalRotation() + m_afterPixItem->rotation());

		rotateUi(m_pendingOrientation, Rotation_RotateAndCrossFade, true);

		SystemService::instance()->postSystemStatus();
	} else {
		m_uiRootItem.setVisible(true);

		if(m_beforePixItem) {
			scene()->removeItem(m_beforePixItem);
			delete m_beforePixItem;
			m_beforePixItem = 0;
		}
		if(m_afterPixItem) {
			scene()->removeItem(m_afterPixItem);
			delete m_afterPixItem;
			m_afterPixItem = 0;
		}
		m_screenShotImagesValid = false;

		SystemUiController::instance()->rotationComplete();
		SystemUiController::instance()->enableDirectRendering(true);

		rotatePendingWindows();
	}
	g_message("ROTATION: [%s]: ROTATION DONE ", __PRETTY_FUNCTION__);
        resetCachedContent(); // Reset wallpaper cache
}

bool WindowServer::enqueueWindowForFlip(Window* window, QRect& windowScreenBoundaries, bool sync)
{
	if(!window->isIpcWindow())
		return false;

	t_cardFlipRequest req;
	req.windowPtr = QPointer<HostWindow>(static_cast<HostWindow*>(window));
	req.windowScreenBounds = windowScreenBoundaries;
	req.sync = sync;

	m_pendingFlipRequests.enqueue(req);

	return true;
}

bool WindowServer::removeWindowFromFlipQueue(Window* window)
{
	if(!window->isIpcWindow())
		return false;

	bool ret = false;
	HostWindow* win = static_cast<HostWindow*>(window);

	QQueue<t_cardFlipRequest>::Iterator it = m_pendingFlipRequests.begin();
	while (it != m_pendingFlipRequests.end()) {
		if(it->windowPtr.data() == win) {
			ret = true;
			it = m_pendingFlipRequests.erase(it);
		} else {
			it++;
		}
	}

	return ret;
}

void WindowServer::rotatePendingWindows()
{
	g_message("ROTATION: [%s]: Rotating Windows in pending Queue", __PRETTY_FUNCTION__);
	while(!m_pendingFlipRequests.empty()) {
		t_cardFlipRequest req = m_pendingFlipRequests.dequeue();
		if(!req.windowPtr.isNull()) {
			if(req.sync) {
				req.windowPtr->flipEventSync(true);
			} else {
				req.windowPtr->flipEventAsync(req.windowScreenBounds, true);
			}
		}
	}
}

void WindowServer::cancelPendingWindowRotations()
{
	g_message("ROTATION: [%s]: Canceling Pending Rotation Requests", __PRETTY_FUNCTION__);
	while(!m_pendingFlipRequests.empty()) {
		t_cardFlipRequest req = m_pendingFlipRequests.dequeue();
		if(!req.windowPtr.isNull()) {
			req.windowPtr->queuedFlipCanceled(req.windowScreenBounds);
		}
	}
}

void WindowServer::cacheFocusedItem()
{
    if (scene())
        m_cachedFocusedItem = scene()->focusItem();
    else
        m_cachedFocusedItem = 0;
}

void WindowServer::restoreCachedFocusItem()
{
    if (m_cachedFocusedItem && scene()) {
        // queue up focus for an item which had focus but had it
        // taken away due to visibility changes
        if (!m_cachedFocusedItem->isVisible() &&
            !m_cachedFocusedItem->hasFocus() &&
            scene()->focusItem() != m_cachedFocusedItem)
        {
            m_cachedFocusedItem->setFocus();
        }
    }

    m_cachedFocusedItem = 0;
}

// ======================================================================================

QGraphicsPixmapObject::QGraphicsPixmapObject()
	: m_pixmap(0)
	, m_compMode(QPainter::CompositionMode_SourceOver)
	, m_topClipHeight(0)
	, m_rotation(0)
{

}

QGraphicsPixmapObject::~QGraphicsPixmapObject()
{
	if (m_pixmap)
		delete m_pixmap;
}

QRectF QGraphicsPixmapObject::boundingRect() const
{
	if(!m_pixmap || m_pixmap->isNull()) {
		return QRectF(0,0,0,0);
	} else {
		return QRectF(-m_pixmap->width()/2, -m_pixmap->height()/2, m_pixmap->width(), m_pixmap->height());
	}
}

void QGraphicsPixmapObject::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(m_pixmap) {
		painter->setCompositionMode(m_compMode);
		painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
		if(m_rotation)
			painter->rotate(m_rotation);
		painter->drawPixmap(boundingRect().toRect(), *m_pixmap);
		if(m_rotation)
			painter->rotate(-m_rotation);
		painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	}
}

void QGraphicsPixmapObject::setPixmap(QPixmap* img)
{
	m_pixmap = img;
}


SharedGlobalProperties* WindowServer::globalProperties()
{
    return (SharedGlobalProperties*) s_globalPropsBuffer->data();
}
