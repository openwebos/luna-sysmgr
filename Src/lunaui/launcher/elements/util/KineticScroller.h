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




#ifndef KINETIC_SCROLLER_H_
#define KINETIC_SCROLLER_H_

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QEasingCurve>
#include <QPointer>
#include <QTextStream>
#include <QFile>

#include <limits>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
QT_END_NAMESPACE

class KineticScroller : public QObject
{
	Q_OBJECT
	Q_PROPERTY(qreal scrollOffset READ scrollOffset WRITE setScrollOffset)
Q_SIGNALS:
	void scrolled(qreal oldScroll);

public:
	KineticScroller(qreal minValue = -std::numeric_limits<qreal>::infinity(),
	                qreal maxValue = std::numeric_limits<qreal>::infinity());
	~KineticScroller();

	void setMaxOverscroll(qreal overscroll);
	void setMinValue(qreal minValue);
	void setMaxValue(qreal maxValue);

	qreal scrollOffset() const;
	qreal getMinScrollValue() const;
	void scrollBy(qreal offset);

	void setScrollOffset(qreal offset);

	void handleFlick(qreal velocity);
	void handleMouseDown();
	void handleMouseUp();
	void stopImmediately();
	void correctOverscroll();

	bool animatingFlick() const;

private Q_SLOTS:
	void flickAnimationTick();
	void overscrollTrigger();
	void flickTransitionToOverscroll();
	void stopFlickAnimation();

private:
	enum FlickState {
		FlickNormal,
		FlickOverScroll,
		FlickOverscrollCorrection,
	};

	enum FlickDirection {
		FlickUp = 1,
		FlickDown = -1,
	};

	static FlickDirection direction(qreal x)
	{
		return x >= 0 ? FlickUp : FlickDown;
	}

	void zizz(qreal inV);
	bool inOverscroll() const;
	bool inOverscroll(qreal scrollOffset) const;
	void checkUserActionOnOverscroll();

	qreal amountInOverscroll() const;
	qreal instantaneousVelocity() const;
	qreal easingCubicCoEfficient(qreal index) const;

	QTimer m_flickAnimationTimer;
	QTimer m_flickFilter;
	QTimer m_overscroll;
	QTime m_clock;
	QEasingCurve m_easing;
	qreal m_minScroll;
	qreal m_maxScroll;
	qreal m_maxOverscroll;
	qreal m_scrollOffset;
	//static qreal m_scrollOffsetChecker;
	qreal m_previousVelocity;
	qreal m_accumulatedScroll;
	QPointer<QPropertyAnimation> m_flickAnimation;
	QPointer<QPropertyAnimation> m_flickOverscrollAnimation;
	QPointer<QPropertyAnimation> m_overscrollAnimation;
	FlickDirection m_flickEventDirection;
	FlickState m_flickState;
	qreal m_flickVelocity;
	qreal m_friction;
	qreal m_restoreForce;
	qreal m_flickStopOffset;
	qreal m_tRegularScrollingTime;
	qreal m_s0;
	qreal m_sp;
	qreal m_s1;
	qreal m_sReferenceTime;
	qreal m_sReferenceTime1;
	qreal m_v0;
	qreal m_overScrollCorrectionStart;
	bool m_BypassOverScrollInfo;
	bool m_OverScrollCorrectionInterrupted;
#if TRACE_SCROLLER
	QFile m_plotFile;
	QFile m_plotFile2;
	QTextStream *m_plot;
	QTextStream *m_plotMouse;
#endif
	bool m_flickAnimationTimerActive;
};

#endif

