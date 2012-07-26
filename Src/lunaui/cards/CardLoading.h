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




#ifndef CARDLOADING_H
#define CARDLOADING_H

#include "Common.h"

#include "AnimationEquations.h"

#include <QPainter>
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QTimer>

class Window;
class CardRoundedCornerShaderStage;

class CardLoading : public QObject
{
	Q_OBJECT
	Q_PROPERTY(qreal pulseOpacity READ pulseOpacity WRITE setPulseOpacity)
	Q_PROPERTY(qreal fadeOpacity READ fadeOpacity WRITE setFadeOpacity)

public:

	CardLoading(Window* win);
	~CardLoading();

	// tell the loading animation to start transitioning to a finished state
	void finish();

	// are we trying to finish?
	bool finishing() const;

	void paint(QPainter* painter, bool maximized);

        void setScale(float scale);

Q_SIGNALS:

	void signalLoadingFinished();

private Q_SLOTS:

	void slotCrossFadeFinished();
	void slotPulseTimeout();

private:

	qreal pulseOpacity() const { return m_pulseOpacity; }
	void setPulseOpacity(qreal opacity);

	qreal fadeOpacity() const { return m_fadeOpacity; }
	void setFadeOpacity(qreal opacity);

        void initializeRoundedCornerStage();

	Window* m_win;
	
	qreal m_fadeOpacity;
	qreal m_pulseOpacity;
	qreal m_pulseOpacityAnimInc;
	bool m_pulseIn;

	QTimer m_pulseTimer;
	QTimer m_pulsePauseTimer;
	QPropertyAnimation m_fadeAnimation;

	QPixmap m_background;
	QPixmap m_icon;
	static unsigned int s_glowRefCount;
	static QPixmap* s_glow;
	bool m_showAnimation;

        static unsigned int s_backgroundRefCount;
        static QPixmap* s_background;
        CardRoundedCornerShaderStage* m_shader;
};

#endif /* CARDLOADING_H */
