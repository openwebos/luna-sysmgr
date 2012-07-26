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




#include "KineticScroller.h"
#include <sys/time.h>
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QDebug>
#include <Qt>
#include <cmath>
#include "Time.h"
#include "Settings.h"
#include "DisplayManager.h"

static const int kQbstractAnimationTimeout = QAbstractAnimation::animationTimerInterval();
static const int kOverScrollCorrectionTimeOut = 350;
static const int kOverscrollTimeout = 200; // ms
static const int kFlickFilterTimeout = 100; // ms
static const qreal sFlickScalar = 2225;
static const qreal kMaxSpeed = 100;
static const qreal kDefaultFriction = 8e-4;

static inline const qreal qInf()
{
	return std::numeric_limits<qreal>::infinity();
}

static bool sameDirection(qreal x, qreal y)
{
	return ((x >= 0 ? 1 : -1) * y) >= 0;
}

#if TRACE_SCROLLER
static void printVsyncState(QTextStream *stream)
{
	if (stream)
		*stream << "vsync " << (DisplayManager::isVsyncOff() ? "disabled" : "enabled") << "\n";
}
#endif

KineticScroller::KineticScroller(qreal minValue, qreal maxValue)
	: m_minScroll(minValue)
	, m_maxScroll(maxValue)
	, m_maxOverscroll(100)
	, m_scrollOffset(0)
	, m_previousVelocity(0)
	, m_accumulatedScroll(0)
	, m_tRegularScrollingTime(0)
	, m_s0(0)
	, m_sp(0)
	, m_s1(0)
	, m_v0(0)
	, m_sReferenceTime(0)
	, m_sReferenceTime1(0)
	, m_overScrollCorrectionStart(0)
	, m_BypassOverScrollInfo(false)
	, m_OverScrollCorrectionInterrupted(false)

#if TRACE_SCROLLER
	, m_plotFile("/media/internal/kinetic_scroll_plot.csv")
	, m_plotFile2("/media/internal/mouse_move_plot.csv")
	, m_plot(NULL)
	, m_plotMouse(NULL)
#endif
	, m_flickAnimationTimerActive(false)
{
	m_overscroll.setInterval(kOverscrollTimeout);
	m_overscroll.setSingleShot(true);
	connect(&m_overscroll, SIGNAL(timeout()), SLOT(overscrollTrigger()));

	m_flickFilter.setInterval(kFlickFilterTimeout);
	m_flickFilter.setSingleShot(true);
	connect(&m_flickFilter, SIGNAL(timeout()), SLOT(stopFlickAnimation()));

	connect(&m_flickAnimationTimer, SIGNAL(timeout()), SLOT(flickAnimationTick()));

	m_easing = QEasingCurve(QEasingCurve::OutCubic);

#if TRACE_SCROLLER
	if (m_plotFile.open(QIODevice::WriteOnly)) {
		m_plot = new QTextStream(&m_plotFile);
	}

	if (m_plotFile2.open(QIODevice::WriteOnly)) {
		m_plotMouse = new QTextStream(&m_plotFile2);
	}

	// requires Qt code
	initBaseLineTimespec();
#endif
}

KineticScroller::~KineticScroller()
{
}

#if TRACE_SCROLLER
static qreal now()
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return (tp.tv_sec - baseLineTimespec().tv_sec) * 1000 + (tp.tv_nsec - baseLineTimespec().tv_nsec) / 1000000.0;
}
#endif

void KineticScroller::flickAnimationTick()
{
	qreal timeToCompletion,easeResult,endDistance,diffDistance,tCurTime = 0.0, velocityAtCurTime;
	static qreal velocityAtCurTimeOld, frictionOpposingMotion, referenceVelocity, timeToZeroVelocity;

	switch (m_flickState) {
		case FlickNormal:
			// get current time.
			tCurTime = Time::curTimeMs();

			// if we are in overscroll, set the flag to be overscrolling and continue for sometime, before the overscrollcorrection kicks in.
			if(true == inOverscroll()) {
				if(false == m_BypassOverScrollInfo) {

					// Update m_sReferenceTime also, so that we stay as close as possible to the currentOffset
					m_sReferenceTime = tCurTime;

					// we have been travelling from refTime to tCurTime with a starting velocity v and accleration m_friction. Compute the final velocity at this point.
					// This is the initial velocity when we go in overscroll.
					velocityAtCurTimeOld = m_v0 + (kDefaultFriction * (tCurTime - m_sReferenceTime));

					// Store this for future computations.
					referenceVelocity = velocityAtCurTimeOld;

					// Set the initial value of friction opposing motion.
					frictionOpposingMotion = kDefaultFriction * -direction(m_v0);

					// with initial velocity as velocityAtCurTimeOld and deceleration as frictionOpposingMotion, compute how long we'll need to get to zero final velocity.
					timeToZeroVelocity = referenceVelocity/-frictionOpposingMotion;

					// set the state that we are going into overscroll.
					m_flickState = FlickOverScroll;
					return;
				}
			}
			else {
				// we are not overscrolling. If the time to scroll has elapsed, then just stop.
				if((tCurTime - m_sReferenceTime) > m_tRegularScrollingTime) {
					// If any of these flags were set, reset them as we have scrolled for the duration of the scroll and then come to a stop.
					if(true == m_OverScrollCorrectionInterrupted) {
						m_OverScrollCorrectionInterrupted = false;
					}
					if(true == m_BypassOverScrollInfo) {
						m_BypassOverScrollInfo = false;
					}
					stopFlickAnimation();
					return;
				}
			}
			break;

		case FlickOverScroll:

			// get current time.
			tCurTime = Time::curTimeMs();

			// get the current position for subsequent use.
			m_s0 = scrollOffset();

			// save sp
			m_sp = m_s0;

			// distance we have travelled since the last tick, slow down by applying additional friction.
			m_s1 = (referenceVelocity * timeToZeroVelocity) + (0.5 * timeToZeroVelocity * timeToZeroVelocity * frictionOpposingMotion) + m_s0;

			// get the end velocity after travelling timeToZeroVelocity time units.
			velocityAtCurTime = referenceVelocity + ( frictionOpposingMotion * (tCurTime - m_sReferenceTime1));

			// if velocityAtCurTime and velocityAtCurTimeOld are of different signs, we are done overscrolling.
			if(((velocityAtCurTimeOld < 0 && velocityAtCurTime >= 0) || (velocityAtCurTimeOld > 0 && velocityAtCurTime <= 0))) {
				m_flickState = FlickOverscrollCorrection;
				flickAnimationTick();
				return;
			}

			// increase the friction for the next iteration.
			frictionOpposingMotion += (frictionOpposingMotion * amountInOverscroll()/m_maxOverscroll);

			// save the reference velocity for the next iteration
			referenceVelocity = velocityAtCurTime;

			// compute the new time to based on the increased friction and current velocity
			timeToZeroVelocity = referenceVelocity/-frictionOpposingMotion;
			break;

		case FlickOverscrollCorrection:
			stopFlickAnimation();
			velocityAtCurTime = 0;
			velocityAtCurTimeOld = 0;
			frictionOpposingMotion = 0;
			m_overScrollCorrectionStart = Time::curTimeMs();
			m_flickOverscrollAnimation = new QPropertyAnimation(this, "scrollOffset");
			m_flickOverscrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
			m_flickOverscrollAnimation->setStartValue(m_scrollOffset);
			m_flickOverscrollAnimation->setEndValue(m_scrollOffset < m_minScroll ? m_minScroll : m_maxScroll);
			m_flickOverscrollAnimation->setDuration(kOverScrollCorrectionTimeOut);
			m_flickOverscrollAnimation->start(QAbstractAnimation::DeleteWhenStopped);
			return;
		}

	// Calculate the ratio for time to completion
	timeToCompletion = (tCurTime - m_sReferenceTime)/m_tRegularScrollingTime;

	// use an easing function to convert linear motion into a pleasing curve [Scroller.js uses cubicOut [defined as Math.pow(n-1,3)+1] to compute the value.
	easeResult = easingCubicCoEfficient(timeToCompletion);

	// compute the ending distance
	endDistance = (m_s1 - m_s0) * easeResult + m_s0;

	// Try to be as frameNeutral as possible.
	diffDistance = endDistance - m_sp;

	// cache this position.
	m_sp = endDistance;

	// set the new offset
	setScrollOffset(scrollOffset() + diffDistance);

	// Check if an overscrollcorrection was interrupted.
	if(true == m_OverScrollCorrectionInterrupted) {
		// if we are set to ignore overScroll, check if we need to flip the flag.
		if(true == m_BypassOverScrollInfo) {
			// we flicked down, so if the current offset is >= minScroll we need to enable this back again
			if(FlickDown == direction(m_v0)) {
				if(scrollOffset() < m_minScroll) {
					m_BypassOverScrollInfo = false;
					m_OverScrollCorrectionInterrupted = false;
				}
			}
			else {
				if(scrollOffset() > m_minScroll) {
					m_BypassOverScrollInfo = false;
					m_OverScrollCorrectionInterrupted = false;
				}
			}
		}
	}
}

qreal KineticScroller::easingCubicCoEfficient(qreal index) const
{
	--index;
	return (index * index * index) + 1;
}

void KineticScroller::setMaxOverscroll(qreal overscroll)
{
	Q_ASSERT(overscroll >= 0);
	m_maxOverscroll = qAbs(overscroll);
}

void KineticScroller::setMinValue(qreal minValue)
{
	m_minScroll = minValue;
}

void KineticScroller::setMaxValue(qreal maxValue)
{
	m_maxScroll = maxValue;
}

qreal KineticScroller::getMinScrollValue() const
{
	return m_minScroll;
}

void KineticScroller::overscrollTrigger()
{
	m_overscroll.stop();

	if (!inOverscroll())
		return;

	delete m_overscrollAnimation;

	qreal target = scrollOffset() < m_minScroll ? m_minScroll : m_maxScroll;
	m_overscrollAnimation = new QPropertyAnimation(this, "scrollOffset");
	m_overscrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
	m_overscrollAnimation->setDuration(500);
	m_overscrollAnimation->setEndValue(target);
	m_overscrollAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void KineticScroller::flickTransitionToOverscroll()
{
	if (!m_flickOverscrollAnimation.isNull()) {
		//qDebug() << "starting overscroll animation @" << qAbs(m_flickOverscrollAnimation->endValue().toReal() - m_flickOverscrollAnimation->startValue().toReal()) / m_flickOverscrollAnimation->duration() << "px/ms";
		m_flickOverscrollAnimation->start(QAbstractAnimation::DeleteWhenStopped);
	}
}

qreal KineticScroller::scrollOffset() const
{
	return m_scrollOffset;
}

void KineticScroller::setScrollOffset(qreal offset)
{
	if ((m_minScroll == -qInf() || m_maxScroll == qInf()) && !inOverscroll())
		return;

	if (!qFuzzyCompare(m_scrollOffset, offset)) {
		qSwap(m_scrollOffset, offset);
		Q_EMIT scrolled(offset);
	}
}

void KineticScroller::scrollBy(qreal offset)
{
	if (m_minScroll == -qInf() || m_maxScroll == qInf())
		return;

	checkUserActionOnOverscroll();

	if (inOverscroll())
		offset /= 2;

	if (animatingFlick()) {
		if (sameDirection(offset, instantaneousVelocity())) {
			m_accumulatedScroll += offset;
			if (m_accumulatedScroll * m_accumulatedScroll < Settings::LunaSettings()->tapRadiusSquared)
				return;
		}
		stopFlickAnimation();
	}

	if (!qFuzzyIsNull(offset)) {
		setScrollOffset(scrollOffset() + offset);
	}

#if TRACE_SCROLLER
	if (m_plotMouse)
		*m_plotMouse << now() << "," << scrollOffset() << "\n";
#endif
}

void KineticScroller::handleFlick(qreal velocity)
{
	if (m_minScroll == -qInf() || m_maxScroll == qInf())
		return;

	/*if (inOverscroll()) {
		return;
	}*/

	//	qDebug() << "flick of" << velocity << "(" << (-velocity / sFlickScalar) << "adjusted)";
	if (!m_overscrollAnimation.isNull())
		m_overscrollAnimation->stop();
	m_overscroll.stop();
	m_flickFilter.stop();

	// adjust flick velocity
	velocity = -velocity / sFlickScalar;

	// keep increasing velocity
	qreal newVelocity;
	if ((velocity < 0 && m_previousVelocity >= 0) || (velocity >= 0 && m_previousVelocity < 0)) {
		// reverse direction
		newVelocity = velocity;
	} else {
		// add until we hit maximum
		newVelocity = m_previousVelocity + velocity;
		if (qAbs(newVelocity) > kMaxSpeed) {
			if (newVelocity < 0)
				newVelocity = -kMaxSpeed;
			else
				newVelocity = kMaxSpeed;
		}
	}

	m_previousVelocity = newVelocity;
	zizz(newVelocity);
}

void KineticScroller::handleMouseDown()
{
	m_accumulatedScroll = 0;
	checkUserActionOnOverscroll();
	if (animatingFlick()) {
		m_flickFilter.start();
		m_flickStopOffset = m_scrollOffset;
	} else {
#if TRACE_SCROLLER
		printVsyncState(m_plotMouse);
#endif
	}
}

void KineticScroller::handleMouseUp()
{
	if (m_flickFilter.isActive()) {
		m_flickFilter.stop();
		stopFlickAnimation();
		m_flickAnimation = NULL;
		m_flickOverscrollAnimation = NULL;
	}
	if (!animatingFlick()) {
		// TODO: make this an animation
		if (!qFuzzyIsNull(m_accumulatedScroll))
			scrollBy(m_accumulatedScroll);
		if (inOverscroll())
			overscrollTrigger();
	}
#if TRACE_SCROLLER
	if (m_plotMouse)
		m_plotMouse->flush();
#endif
	m_accumulatedScroll = 0;
}

#if 0
/**
  * Inverse easing for the easing curve used in the flick animation
  */
static qreal flickEaseInv(qreal x)
{
	return cbrtf(x -1) + 1;
}
#endif

void KineticScroller::zizz(qreal inV)
{
	qreal dv, a0;

	// get t0 [this is when we start.]
	m_sReferenceTime = Time::curTimeMs();

	m_sReferenceTime1 = m_sReferenceTime;

	// If we get another flick before kOverScrollCorrectionTimeOut milliseconds have elapsed after m_overScrollCorrectionStart, ignore the fact that we are in overScroll.
	if((m_sReferenceTime - m_overScrollCorrectionStart) < kOverScrollCorrectionTimeOut) {
		m_BypassOverScrollInfo = true;
		m_OverScrollCorrectionInterrupted = true;
	}

	// stop any existing animation
	stopFlickAnimation();

	// Save the initial position.
	m_s0 = scrollOffset();

	m_v0 = inV;

	// direction
	dv = (m_v0 < 0) ? -1 : 1;

	// deceleration due to friction
	a0 = kDefaultFriction * -dv;

	// Get the time we need to spend in regular scrolling.
	m_tRegularScrollingTime = (-m_v0/a0);

	// Compute the distance(s) which the scroller will travel for the time m_tRegularScrollingTime using [S = u*t + 1/2 g*t*t]
	m_s1 = (0.5 * a0 * m_tRegularScrollingTime * m_tRegularScrollingTime) + (m_v0 * m_tRegularScrollingTime) + m_s0;

	// Store the original value for future use
	m_sp = m_s0;

	// Set the necessary variables before calling flickAnimationTimer
	m_flickEventDirection = direction(inV);
	m_flickState = FlickNormal;
	m_flickVelocity = inV;

	// Start a periodic timer that will fire every 10 msec
	m_flickAnimationTimerActive = true;
	m_flickAnimationTimer.setSingleShot(false);
	m_flickAnimationTimer.setInterval(QAbstractAnimation::animationTimerInterval());

	// Call the function directly to determine the next scrolling position. This will start the timer for subsequent ticks.
	flickAnimationTick();

	// Start the timer to fire.
	m_flickAnimationTimer.start(10);
}

bool KineticScroller::inOverscroll() const
{
	return inOverscroll(scrollOffset());
}

bool KineticScroller::inOverscroll(qreal scrollOffset) const
{
	if((m_minScroll == -qInf()) && (scrollOffset < 0))
		return true;

	return !(m_minScroll <= scrollOffset && scrollOffset <= m_maxScroll);
}

void KineticScroller::checkUserActionOnOverscroll()
{
	if (!m_flickAnimation.isNull()) {
		m_overscroll.stop();
		delete m_overscrollAnimation;
	} else if (inOverscroll()) {
		delete m_overscrollAnimation;
		m_overscroll.start();
	}
}

qreal KineticScroller::instantaneousVelocity() const
{
	int duration;
	qreal old_s0;
	qreal old_s1;

	if (m_flickAnimationTimerActive || m_flickAnimationTimer.isActive())
		return m_flickVelocity;

	if (!m_flickAnimation.isNull()) {
		old_s0 = m_flickAnimation->startValue().toReal();
		old_s1 = m_flickAnimation->endValue().toReal();
		duration = m_flickAnimation->duration();
	} else if (!m_flickOverscrollAnimation.isNull()) {
		old_s0 = m_flickOverscrollAnimation->startValue().toReal();
		old_s1 = m_flickOverscrollAnimation->endValue().toReal();
		duration = m_flickOverscrollAnimation->duration();
	} else {
		return 0;
	}

	return (old_s1 - old_s0) / duration;
}

void KineticScroller::stopFlickAnimation()
{
//	if (sender() == &m_flickFilter)
//		qDebug() << "stop flick";

#if TRACE_SCROLLER
	if (m_plot)
		m_plot->flush();
#endif

	// anyone calling stopFlickAnimation might still want to retrieve from the animations
	// if they exist
	m_flickAnimationTimer.stop();
	m_flickFilter.stop();
	m_previousVelocity = 0;
	m_flickAnimationTimerActive = false;

	if (!m_flickAnimation.isNull()) {
		m_flickAnimation->disconnect();
		m_flickAnimation->stop();
	}
	if (!m_flickOverscrollAnimation.isNull()) {
		m_flickOverscrollAnimation->disconnect();
		m_flickOverscrollAnimation->stop();
	}
}

bool KineticScroller::animatingFlick() const
{
	return m_flickAnimationTimerActive || m_flickAnimationTimer.isActive() || !m_flickAnimation.isNull() || !m_flickOverscrollAnimation.isNull();
}

qreal KineticScroller::amountInOverscroll() const
{
	if (!inOverscroll())
		return 0;

	if (scrollOffset() > m_maxScroll)
		return scrollOffset() - m_maxScroll;
	return m_minScroll - scrollOffset();
}

void KineticScroller::stopImmediately() {

	stopFlickAnimation();

	if (!m_overscrollAnimation.isNull())
		m_overscrollAnimation->stop();
	m_overscroll.stop();

}

void KineticScroller::correctOverscroll() {
	overscrollTrigger();
}
