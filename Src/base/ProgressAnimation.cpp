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

#include "ProgressAnimation.h"

#include "HostBase.h"
#include "Settings.h"
#include "AnimationSettings.h"
#include "QtUtils.h"
#include "SystemUiController.h"
#include "Settings.h"
#include "Localization.h"

#include <QPainter>

static const int kFontSize = 18;

const qreal kLabelRatio = 0.58;

ProgressAnimation::ProgressAnimation(Type type)
	: m_brightness(0.0)
	, m_pulsing(true)
{
	// make it a square so we don't need to resize in case of rotation
	int maxDimension = SystemUiController::instance()->currentUiWidth();
	if (maxDimension < SystemUiController::instance()->currentUiHeight())
		maxDimension = SystemUiController::instance()->currentUiHeight();

	m_boundingRect = QRectF(-maxDimension/2, -maxDimension/2, maxDimension, maxDimension);

    std::string basePath = Settings::LunaSettings()->lunaSystemResourcesPath;
    std::string normalFile, brightFile, logoFile;
    QString title, description;

    switch (type) {
    case TypeHp:
        normalFile = "/hp-logo.png";
        brightFile = "/hp-logo-bright.png";
        break;
    case TypeMsm:
        normalFile = "/normal-bg.png";
        brightFile = "/glow-bg.png";
        logoFile = "/normal-usb.png";
        break;
    case TypeFsck:
        normalFile = "/normal-bg.png";
        brightFile = "/glow-bg.png";
        logoFile = "/fsck-usb.png";
        title = QString::fromUtf8(LOCALIZED("OWWW! That hurts!").c_str());
        description = QString::fromUtf8(LOCALIZED("Next time, please unmount the drive from the desktop.").c_str());
        break;
    default:
        Q_ASSERT_X(false, __PRETTY_FUNCTION__, "invalid progress type");
        break;
    }

    // MANDATORY
	m_normalPix.load(qFromUtf8Stl(basePath + normalFile));
    Q_ASSERT_X(!m_normalPix.isNull(), __PRETTY_FUNCTION__, "missing normal image");
	m_brightPix.load(qFromUtf8Stl(basePath + brightFile));
    Q_ASSERT_X(!m_brightPix.isNull(), __PRETTY_FUNCTION__, "missing bright image");

    // OPTIONAL
    m_logoPix.load(qFromUtf8Stl(basePath + logoFile));

    constructLogo(title, description);

	// setup pulsing animation
	m_pulseAnimation.setTargetObject(this);
	m_pulseAnimation.setPropertyName("brightness");
	m_pulseAnimation.setDuration(AS(progressPulseDuration));
	m_pulseAnimation.setEasingCurve(AS_CURVE(progressPulseCurve));
	m_pulseAnimation.setStartValue(0.0);
	m_pulseAnimation.setEndValue(1.0);
	connect(&m_pulseAnimation, SIGNAL(finished()), SLOT(slotPulseCompleted()));

	// setup zoom-fade finish animation
	QPropertyAnimation* scale = new QPropertyAnimation(this, "scale");
	scale->setDuration(AS(progressFinishDuration));
	scale->setEasingCurve(AS_CURVE(progressFinishCurve));
	scale->setEndValue(2.0);

	QPropertyAnimation* opacity = new QPropertyAnimation(this, "opacity");
	opacity->setDuration(AS(progressFinishDuration));
	opacity->setEasingCurve(AS_CURVE(progressFinishCurve));
	opacity->setEndValue(0.0);

	m_zoomFadeAnimation.addAnimation(scale);
	m_zoomFadeAnimation.addAnimation(opacity);
	connect(&m_zoomFadeAnimation, SIGNAL(finished()), SIGNAL(signalProgressAnimationCompleted()));
}

ProgressAnimation::~ProgressAnimation()
{
	m_pulsing = false;
	m_pulseAnimation.stop();
	m_zoomFadeAnimation.stop();
}

void ProgressAnimation::constructLogo(const QString& title, const QString& description)
{
    if (m_logoPix.isNull())
        return;

    QFont fontBold(QString::fromStdString(Settings::LunaSettings()->fontProgressAnimationBold));
    QFont font(QString::fromStdString(Settings::LunaSettings()->fontProgressAnimation));
    fontBold.setPixelSize(kFontSize);
    fontBold.setBold(true);
    font.setPixelSize(kFontSize);

    int xoffset = m_logoPix.width() / 3;
    int yoffset = m_logoPix.height() * kLabelRatio;
    int titleFlags = Qt::AlignHCenter|Qt::TextSingleLine|Qt::TextDontClip;
    int descriptionFlags = Qt::AlignHCenter|Qt::TextWordWrap|Qt::TextDontClip;
    QRect titleBounds, descriptionBounds;
    if (!title.isEmpty()) {
        QFontMetrics fm(fontBold);
        titleBounds = fm.boundingRect(QRect(xoffset, yoffset, xoffset, m_logoPix.height() - yoffset), titleFlags, title);
    }
    if (!description.isEmpty()) {
        QFontMetrics fm(font);
        yoffset += (titleBounds.height() * 2);
        descriptionBounds = fm.boundingRect(QRect(xoffset, yoffset, xoffset, m_logoPix.height() - yoffset), descriptionFlags, description);
    }

    QPainter painter(&m_logoPix);
    painter.setPen(Qt::white);
    
    if (!title.isEmpty()) {
        painter.setFont(fontBold);
        painter.drawText(titleBounds, titleFlags, title);
    }
    if (!description.isEmpty()) {
        painter.setFont(font);
        painter.drawText(descriptionBounds, descriptionFlags, description);
    }

    painter.end();
}

void ProgressAnimation::setBrightness(qreal brightness)
{
	m_brightness = qBound((qreal)0.0, brightness, (qreal)1.0);
	update();
}

QRectF ProgressAnimation::boundingRect() const
{
	return m_boundingRect;
}

void ProgressAnimation::start()
{
	m_pulsing = true;
	if (m_pulseAnimation.state() != QAbstractAnimation::Running)
		m_pulseAnimation.start();
}

void ProgressAnimation::stop()
{
	m_pulsing = false;
	m_pulseAnimation.stop();

	m_zoomFadeAnimation.start();
}

void ProgressAnimation::slotPulseCompleted()
{
	if (m_pulsing) {
		// switch directions
		if (m_pulseAnimation.direction() == QAbstractAnimation::Forward)
			m_pulseAnimation.setDirection(QAbstractAnimation::Backward);
		else
			m_pulseAnimation.setDirection(QAbstractAnimation::Forward);
		m_pulseAnimation.start();
	}
}

void ProgressAnimation::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	painter->fillRect(m_boundingRect, Qt::black);

	QPainter::CompositionMode oldMode = painter->compositionMode();
	painter->setCompositionMode(QPainter::CompositionMode_Plus);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter->drawPixmap(-m_normalPix.width()/2, -m_normalPix.height()/2, m_normalPix);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	painter->setCompositionMode(oldMode);

	if (m_pulsing) {
		qreal oldOpacity = painter->opacity();
		painter->setOpacity(m_brightness);
		painter->drawPixmap(-m_brightPix.width()/2, -m_brightPix.height()/2, m_brightPix);
		painter->setOpacity(oldOpacity);
	}

    if (!m_logoPix.isNull()) {
	    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawPixmap(-m_logoPix.width()/2, -m_logoPix.height()/2, m_logoPix);
    	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    }
}

