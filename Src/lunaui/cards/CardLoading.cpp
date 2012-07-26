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

#include "CardLoading.h"

#include "ApplicationDescription.h"
#include "AnimationSettings.h"
#include "CardWindow.h"
#include "Logging.h"
#include "Settings.h"
#include "SystemService.h"
#include "CardRoundedCornerShaderStage.h"

#include <string>

unsigned int CardLoading::s_glowRefCount = 0;
QPixmap* CardLoading::s_glow = 0;
unsigned int CardLoading::s_backgroundRefCount = 0;
QPixmap* CardLoading::s_background = 0;

CardLoading::CardLoading(Window* win)
	: m_win(win)
	, m_fadeOpacity(1.0)
	, m_pulseOpacity(0.0)
	, m_pulseOpacityAnimInc(0.0)
	, m_pulseIn(true)
    , m_shader(0)
	, m_showAnimation(SystemService::instance()->showCardLoadingAnimation())
{
    luna_assert(m_win != 0);

	if (G_LIKELY(s_glow == 0)) {
		QString path(Settings::LunaSettings()->lunaSystemResourcesPath.c_str());
		path.append("/loading-glow.png");
		s_glow = new QPixmap(path);
		if(s_glow)
			s_glowRefCount++;
		if (!s_glow || s_glow->isNull()) {
			g_critical("%s: Failed to load image '%s'", __PRETTY_FUNCTION__, qPrintable(path));
		}
	} else {
		s_glowRefCount++;
	}

        if (G_LIKELY(s_background == 0)) {
            QString path(Settings::LunaSettings()->lunaSystemResourcesPath.c_str());
            path.append("/loading-bg.png");
            s_background = new QPixmap(path);
            if (s_background) {
                    s_backgroundRefCount++;
            }
            if (!s_background || s_background->isNull()) {
                    g_critical("%s: Failed to load image '%s'", __PRETTY_FUNCTION__, qPrintable(path));
            }
        } else {
                s_backgroundRefCount++;
        }

    // Next, allow app specific settings to override if appropriate:
	ApplicationDescription* appDesc = m_win->appDescription();
    if (appDesc) {
        int size = Settings::LunaSettings()->splashIconSize;
        m_icon.load(appDesc->splashIconName().c_str());
        if (!m_icon.isNull()) {
            // scale splash icon to fit the devices screen dimensions
            if (m_icon.width() != size || m_icon.height() != size) {
                m_icon = m_icon.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
        else {
            // just use the launcher icon
            m_icon = appDesc->getDefaultLaunchPoint()->icon();
            if (m_icon.width() != size || m_icon.height() != size) {
                int newWidth = qMin((int)(m_icon.width()*1.5), size);
                int newHeight = qMin((int)(m_icon.height()*1.5), size);
                m_icon = m_icon.scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }

        // Initialize to the scene specific splash background if applicable:
        if (Settings::LunaSettings()->enableSplashBackgrounds) {
            luna_assert(Window::Type_Card == m_win->type());
            CardWindow *cardWin = static_cast<CardWindow*>(m_win);
            std::string sceneBackgroundName  = cardWin->splashBackgroundName();
            // If the scene did not specify a specific scene background, then use
            // the one in the appinfo if available:
            if (sceneBackgroundName.empty()) {
                sceneBackgroundName = appDesc->splashBackgroundName();
            } else {
                // The scene specific splash background does not have the app path.
                // Prepend it:
                sceneBackgroundName = appDesc->folderPath() + '/' + sceneBackgroundName;
            }

            // Load the app / scene specific background if applicable:
            m_background.load(sceneBackgroundName.c_str());
        }
    }
    else {
        g_warning("Failed to find app descriptor for %s (%s)", m_win->appId().c_str(), m_win->name().c_str());
    }

	// set up the pulsing animation
	qreal frameInterval = 1000/AS(slowFPS);
	m_pulseTimer.setInterval(frameInterval);
	m_pulseTimer.setSingleShot(false);

	m_pulseOpacityAnimInc = 1.0f / (AS(cardLoadingPulseDuration) / frameInterval * 0.5);

	m_pulsePauseTimer.setInterval(AS(cardLoadingTimeBeforeShowingPulsing));
	m_pulsePauseTimer.setSingleShot(true);

	connect(&m_pulsePauseTimer, SIGNAL(timeout()),
			&m_pulseTimer, SLOT(start()));
	connect(&m_pulseTimer, SIGNAL(timeout()),
			SLOT(slotPulseTimeout()));

	// setup fade out animation
	m_fadeAnimation.setTargetObject(this);
	m_fadeAnimation.setPropertyName("fadeOpacity");
	m_fadeAnimation.setDuration(AS(cardLoadingCrossFadeDuration));
	m_fadeAnimation.setEasingCurve(AS_CURVE(cardLoadingCrossFadeCurve));
	m_fadeAnimation.setEndValue(0.0);

	connect(&m_fadeAnimation, SIGNAL(finished()), this, SLOT(slotCrossFadeFinished()));

	// kick off the pulsing
	if (m_showAnimation)
		m_pulsePauseTimer.start();
}

CardLoading::~CardLoading()
{
	m_pulseTimer.stop();
	m_pulsePauseTimer.stop();
	m_fadeAnimation.stop();

	s_glowRefCount--;
	if(!s_glowRefCount) {
		delete s_glow;
		s_glow = 0;
	}
        s_backgroundRefCount--;
        if(!s_backgroundRefCount) {
                delete s_background;
                s_background = 0;
        }

        #if defined(USE_ROUNDEDCORNER_SHADER)
            delete m_shader;
        #endif
}

void CardLoading::finish()
{
	if (!m_showAnimation) {
		slotCrossFadeFinished();
		return;
	}
		
	m_pulseTimer.stop();
	m_pulsePauseTimer.stop();
	m_fadeAnimation.start();
}

void CardLoading::setPulseOpacity(qreal opacity)
{
	m_pulseOpacity = opacity;
	m_win->update();
}

void CardLoading::setFadeOpacity(qreal opacity)
{
	m_fadeOpacity = opacity;
	m_win->update();
}

bool CardLoading::finishing() const
{
	return m_fadeAnimation.state() == QAbstractAnimation::Running;
}

void CardLoading::slotCrossFadeFinished()
{
	Q_EMIT signalLoadingFinished();
}

void CardLoading::setScale(float scale)
{
#if defined(USE_ROUNDEDCORNER_SHADER)
        if(m_shader) {
                m_shader->setScale(scale);
        }
#endif
}

void CardLoading::initializeRoundedCornerStage()
{
#if defined(USE_ROUNDEDCORNER_SHADER)
    if (!m_shader){
        m_shader = new CardRoundedCornerShaderStage();
    }

    if (s_background) {
        m_shader->setParameters(s_background->height(),
                                s_background->width(),
                                s_background->height(),
                                s_background->width(), 
                                77,
                                static_cast<CardWindow*>(m_win)->dimming());
    }
#endif
}

void CardLoading::paint(QPainter* painter, bool maximized)
{
	// always blend over
	QPainter::CompositionMode oldMode = painter->compositionMode();
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter->setOpacity(fadeOpacity());

	if (Window::Type_Card == m_win->type()) {
		// clip the background to the card
		QRectF r = m_win->boundingRect();
		QPainterPath path;
		if (maximized) {
			path.addRect(r);
		}
		else {
			path.addPath(static_cast<CardWindow*>(m_win)->paintPath());
		}

                if (!m_background.isNull()) {
                    painter->setBrushOrigin(r.x(), r.y());
                    painter->fillPath(path, m_background);
                    painter->setBrushOrigin(0, 0);
                }
                else if (s_background != 0 && !s_background->isNull()) {

                    initializeRoundedCornerStage();
#if defined(USE_ROUNDEDCORNER_SHADER)
                    m_shader->setOnPainter(painter);
                    painter->drawPixmap(r, *s_background, s_background->rect());
                    m_shader->removeFromPainter(painter);
#else
                    painter->drawPixmap(r, *s_background, s_background->rect());
#endif
                }
                else {
                    QLinearGradient grad(0, r.y(), 0, r.bottom());
                    grad.setColorAt(0, QColor(72,72,72));
                    grad.setColorAt(1, QColor(30,30,30));
                    painter->fillPath(path, grad);
		}
	}

	if (!m_icon.isNull()) {

	    // Draw the glow pulse first
	    if (s_glow) {
			painter->setOpacity(fadeOpacity() * pulseOpacity());
			painter->drawPixmap(-s_glow->width()/2, -s_glow->height()/2, *s_glow);
			painter->setOpacity(fadeOpacity());
		}

	    // Draw the icon in front of the glow
		painter->drawPixmap(-m_icon.width()/2, -m_icon.height()/2, m_icon);
	}

	// restore modified state
	painter->setOpacity(1.0);
	painter->setCompositionMode(oldMode);
}

void CardLoading::slotPulseTimeout()
{
	if (m_pulseIn) {

		m_pulseOpacity += m_pulseOpacityAnimInc;
		if (m_pulseOpacity > 1.0f) {
			m_pulseOpacity = 1.0f;
			m_pulseIn = false;
		}

		setPulseOpacity(m_pulseOpacity);
	}
	else {

		m_pulseOpacity -= m_pulseOpacityAnimInc;
		if (m_pulseOpacity < 0.0f) {
			m_pulseOpacity = 0.0f;
			m_pulseIn = true;

			m_pulseTimer.stop();
			m_pulsePauseTimer.setInterval(AS(cardLoadingPulsePauseDuration));
			m_pulsePauseTimer.start();
		}

		setPulseOpacity(m_pulseOpacity);
	}	    
}
