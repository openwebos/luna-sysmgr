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

#include "CardGroup.h"
#include "Settings.h"
#include "AnimationSettings.h"

#include <QPropertyAnimation>
#include <QGraphicsScene>

const int kMaxClosedSpacedCards = 3;
// TODO: this should really be calculated based on the current scale
const int kMaxStationaryCards = 4;
const int kPositionsPerWidth = 3;
const int kVelocityPerPosition = 1000;
const int kPositionsPerVelocity = 1;

CardGroup::CardGroup(qreal curScale, qreal nonCurScale)
	: m_pos(QPointF(0,0))
	, m_curScale(curScale)
	, m_nonCurScale(nonCurScale)
	, m_leftWidth(0)
	, m_rightWidth(0)
	, m_activeCard(0)
	, m_currentPosition(0)
{
	m_cardGroupRotFactor = Settings::LunaSettings()->cardGroupRotFactor;
	m_cardGroupXDistanceFactor = Settings::LunaSettings()->cardGroupingXDistanceFactor;
}

CardGroup::~CardGroup()
{
	Q_ASSERT(m_cards.empty());
}

void CardGroup::addToGroup(CardWindow* c)
{
	if (m_cards.contains(c) || c->cardGroup() == this) 
		return;

	c->setCardGroup(this);

	if (m_cards.empty()) {
		m_cards.append(c);
	}
	else {
		int activeIndex = m_cards.indexOf(m_activeCard);
		m_cards.insert(activeIndex+1, c);
		m_currentPosition = activeIndex;
	}
	clampCurrentPosition();

	m_activeCard = c;

	// set anchor for card
	c->setPos(m_pos);
}

void CardGroup::addToFront(CardWindow* c)
{
	if (m_cards.contains(c) || c->cardGroup() == this)
		return;

	c->setCardGroup(this);

	m_cards.append(c);

	m_activeCard = c;
	m_currentPosition = m_cards.size();
	clampCurrentPosition();

	if (c->attachedToGroup())
		c->setPos(m_pos);
}

void CardGroup::addToBack(CardWindow* c)
{
	if (m_cards.contains(c) || c->cardGroup() == this)
		return;

	c->setCardGroup(this);

	m_cards.prepend(c);

	m_activeCard = c;
	m_currentPosition = 0;
	clampCurrentPosition();

	if (c->attachedToGroup())
		c->setPos(m_pos);
}

bool CardGroup::moveActiveCard(int direction)
{
	if (direction == 0 || m_cards.empty() || !m_activeCard)
		return false;

	direction = (direction > 0 ? 1 : -1);
	int activeIndex = m_cards.indexOf(m_activeCard);
	
	if ((direction > 0 && activeIndex >= m_cards.size()-1) ||
		(direction < 0 && activeIndex == 0)) {
		return false;
	}

	// swap the current active card with the next card
	CardWindow* temp = m_cards[activeIndex+direction];
	m_cards[activeIndex+direction] = m_activeCard;
	m_cards[activeIndex] = temp;

	m_currentPosition = activeIndex+direction;
	clampCurrentPosition();

	return true;
}

bool CardGroup::moveActiveCard()
{
	if (m_cards.empty() || !m_activeCard)
		return false;

	int activeIndex = m_cards.indexOf(m_activeCard);
	if (activeIndex == -1)
		return false;

	qreal activeX = m_activeCard->mapToParent(m_activeCard->pos()).x();

	// check the cards to the left of the active card
	for (int i=0; i<activeIndex; i++) {

		if (activeX < m_cards[i]->mapToParent(m_cards[i]->pos()).x()) {

			m_cards.remove(activeIndex);
			m_cards.insert(i, m_activeCard);

			if (i < m_currentPosition) {
				m_currentPosition = i;
				clampCurrentPosition();
			}
			return true;
		}
	}

	// check the cards to the right of the active card
	for (int i=m_cards.size()-1; i>activeIndex; i--) {

		if (activeX > m_cards[i]->mapToParent(m_cards[i]->pos()).x()) {

			m_cards.remove(activeIndex);
			m_cards.insert(i, m_activeCard);

			if (i > (m_currentPosition + kMaxStationaryCards - 1)) {
				m_currentPosition = i;
				clampCurrentPosition();
			}

			return true;
		}
	}

	return false;
}

bool CardGroup::moveCurrentPosition(int direction)
{
	if (direction == 0 || m_cards.empty() || !m_activeCard)
		return false;

	qreal oldPosition = m_currentPosition;
	m_currentPosition += (direction > 0 ? 1 : -1);
	clampCurrentPosition();
	
	return oldPosition != m_currentPosition;
}

void CardGroup::removeFromGroup(CardWindow* c)
{
	if (!c || c->cardGroup() != this)
		return;

	c->setCardGroup(0);
	int index = m_cards.indexOf(c);
	int activeIndex = m_cards.indexOf(m_activeCard);
	m_cards.remove(index);

	if (m_cards.empty())
		m_activeCard = 0;
	else if (index == activeIndex)
		m_activeCard = index > 0 ? m_cards[index-1] : m_cards[0];

	clampCurrentPosition();
}

QList<QPropertyAnimation*> CardGroup::animateOpen(int duration, 
												  QEasingCurve::Type curve, 
												  bool includeActiveCard)
{
	QList<QPropertyAnimation*> anims;
	QVector<CardWindow::Position> positions = calculateOpenedPositions();

	Q_ASSERT(positions.size() == m_cards.size());
	for (int i = 0; i < m_cards.size(); i++) {

		if (!includeActiveCard && m_cards[i] == m_activeCard)
			continue;

		QPropertyAnimation* anim = new QPropertyAnimation(m_cards[i], "position");

		QVariant end; end.setValue(positions[i]);
		anim->setDuration(duration);
		anim->setEasingCurve(curve);
		anim->setEndValue(end);

		anims.append(anim);
	}
	return anims;
}

QList<QPropertyAnimation*> CardGroup::animateClose(int duration, QEasingCurve::Type curve, bool useGroupPosition)
{
	static const qreal kCardWidth = m_cards[0]->boundingRect().width();

	qreal activeCardWidth = kCardWidth * m_curScale;

	QList<QPropertyAnimation*> anims;

	//Perform no fanning animations for card groups that are going to remain
	//closed anyway
	if (useGroupPosition && (qAbs(m_pos.x()) > activeCardWidth)) {
		return anims;
	}

    // use a large size in the case of non group positioning to garuantee that the group is completely closed
	QVector<CardWindow::Position> positions = calculateOpenedPositions(useGroupPosition ? m_pos.x() : kCardWidth);

	Q_ASSERT(positions.size() == m_cards.size());
	for (int i = 0; i < m_cards.size(); i++) {

		QPropertyAnimation* anim = new QPropertyAnimation(m_cards[i], "position");

		QVariant end; end.setValue(positions[i]);
		anim->setDuration(duration);
		anim->setEasingCurve(curve);
		anim->setEndValue(end);

		anims.append(anim);
	}
	return anims;
}

QList<QPropertyAnimation*> CardGroup::animateCloseWithOffset(int duration, QEasingCurve::Type curve, int groupXOffset)
{
	QVector<CardWindow::Position> positions = calculateOpenedPositions(groupXOffset);

	QList<QPropertyAnimation*> anims;
	Q_ASSERT(positions.size() == m_cards.size());
	for (int i = 0; i < m_cards.size(); i++) {

		QPropertyAnimation* anim = new QPropertyAnimation(m_cards[i], "position");

		QVariant end; end.setValue(positions[i]);
		anim->setDuration(duration);
		anim->setEasingCurve(curve);
		anim->setEndValue(end);

		anims.append(anim);
	}
	return anims;
}

void CardGroup::layoutCards(bool open, bool includeActiveCard)
{
	if(open) {
		QVector<CardWindow::Position> positions = calculateOpenedPositions();

		Q_ASSERT(positions.size() == m_cards.size());
		for (int i = 0; i < m_cards.size(); i++) {

			if (!includeActiveCard && m_cards[i] == m_activeCard)
				continue;

			m_cards[i]->setPosition(positions[i]);
		}
	} else {
		QVector<CardWindow::Position> positions = calculateClosedPositions();

		Q_ASSERT(positions.size() == m_cards.size());
		for (int i = 0; i < m_cards.size(); i++) {
			m_cards[i]->setPosition(positions[i]);

		}
	}
}

QList<QPropertyAnimation*> CardGroup::maximizeActiveCard(qreal centerOffset)
{
	QList<QPropertyAnimation*> anims;
	if (m_activeCard == 0)
		return anims;

	int activeIndex = m_cards.indexOf(m_activeCard);
	QPropertyAnimation* anim = new QPropertyAnimation(m_activeCard, "position");
	anim->setDuration(AS(cardMaximizeDuration));
	anim->setEasingCurve(AS_CURVE(cardMaximizeCurve));
	CardWindow::Position p;
	p.trans.setY(-m_activeCard->y() + centerOffset);
	p.trans.setX(-m_activeCard->x());
	QVariant end; end.setValue(p);
	anim->setEndValue(end);
	anims.append(anim);

	qreal x = m_activeCard->parentItem()->boundingRect().left() * 2;
	p.trans.setX(x);
	p.trans.setZ(m_curScale);
	end.setValue(p);
	for (int i=activeIndex-1; i>=0; i--) {

		// animate cards underneath the active card off to the left
		anim = new QPropertyAnimation(m_cards[i], "position");
		anim->setDuration(AS(cardMaximizeDuration));
		anim->setEasingCurve(AS_CURVE(cardMaximizeCurve));
		anim->setEndValue(end);

		anims.append(anim);
	}

	p.trans.setX(-x);
	end.setValue(p);
	for (int i=activeIndex+1; i<m_cards.size(); i++) {

		// animate cards over the active card off to the right
		anim = new QPropertyAnimation(m_cards[i], "position");
		anim->setDuration(AS(cardMaximizeDuration));
		anim->setEasingCurve(AS_CURVE(cardMaximizeCurve));
		anim->setEndValue(end);

		anims.append(anim);
	}

	return anims;
}

void CardGroup::maximizeActiveCardNoAnimation(qreal centerOffset)
{
	if (m_activeCard == 0)
		return;

	int activeIndex = m_cards.indexOf(m_activeCard);
	CardWindow::Position p;
	p.trans.setY(-m_activeCard->y() + centerOffset);
	p.trans.setX(-m_activeCard->x());
	m_activeCard->setPosition(p);

	qreal x = m_activeCard->parentItem()->boundingRect().left() * 2;
	p.trans.setX(x);
	p.trans.setZ(m_curScale);
	for (int i=activeIndex-1; i>=0; i--) {
		// position cards underneath the active card off to the left
		m_cards[i]->setPosition(p);
	}

	p.trans.setX(-x);
	for (int i=activeIndex+1; i<m_cards.size(); i++) {
		// position cards over the active card off to the right
		m_cards[i]->setPosition(p);
	}
}

bool CardGroup::shouldMaximizeOrScroll(QPointF scenePt) {
	if (m_cards.empty())
		return false;

	// does this point fall on any of the cards in this group?
	// Get the index of the card in question
	QPointF mappedPt;
	int cardTouched = 0;

	for (int i=m_cards.size()-1; i>=0; i--) {
		mappedPt = m_cards[i]->mapFromScene(scenePt);
		if (m_cards[i]->contains(mappedPt)) {
			cardTouched = i;
			break;
		}
	}

	//g_warning("cardTouched: %d, kmaxStationaryCards: %d, m_currentPosition: %f, m_cards.size(): %d", cardTouched, kMaxStationaryCards, m_currentPosition, m_cards.size());

	//We now have the index of the card touched. The next step is to take a
	//guess at whether or not that card is "sufficiently visible" to conclude
	//the user tapped on it with intent to maximize.  This will be device
	//specific and does involve some guesswork.

	if (m_cards.size() <= kMaxStationaryCards) {
		//The card group can't be bunched up, so it's ok to maximize the
		//touched card.
		return true;
	} else {
		//If we're on the rightmost side of the group, then any of those
		//last stationary cards makes a good target.
		if (m_currentPosition > (m_cards.size()-kMaxStationaryCards)) {
					if (cardTouched >= (m_cards.size()-kMaxStationaryCards)) {
						return true;
					} else {
						m_currentPosition -= (kMaxStationaryCards-1);
						clampCurrentPosition();
						return false;
					}
		}
		//If we're somwehre in the middle, then we're
		//going with 1 card to the left and 2 to the right for just now.
		else if (cardTouched <= m_currentPosition) {
			if (cardTouched >= (m_currentPosition-1)) {
				return true;
			} else {
				m_currentPosition -= (kMaxStationaryCards-1);
				clampCurrentPosition();
				return false;
			}
		} else if (cardTouched >= m_currentPosition) {
			if (cardTouched <= (m_currentPosition+2)) {
				return true;
			} else {
				m_currentPosition += (kMaxStationaryCards-1);
				clampCurrentPosition();
				return false;
			}
		}
		//If we're on the leftmost side of the group, then any of the
		//first stationary cards is a legit target
		else if (m_currentPosition <= (kMaxStationaryCards-1)) {
			if (cardTouched <= (kMaxStationaryCards-1)) {
				return true;
			} else {
				m_currentPosition += (kMaxStationaryCards-1);
				clampCurrentPosition();
				return false;
			}
		}
	}

	return false;
}

bool CardGroup::testHit(QPointF scenePt) {
	if (m_cards.empty())
		return false;

	// does this point fall on any of the cards in this group?
	QPointF mappedPt;
	for (int i=m_cards.size()-1; i>=0; i--) {

		mappedPt = m_cards[i]->mapFromScene(scenePt);
		if (m_cards[i]->contains(mappedPt)) {
			return true;
		}
	}
	return false;
}

bool CardGroup::setActiveCard(QPointF scenePt)
{
	if (m_cards.empty())
		return false;

	// does this point fall on any of the cards in this group?
	QPointF mappedPt;
	for (int i=m_cards.size()-1; i>=0; i--) {

		mappedPt = m_cards[i]->mapFromScene(scenePt);
		if (m_cards[i]->contains(mappedPt)) {
			m_activeCard = m_cards[i];
			return true;
		}
	}
	return false;
}

bool CardGroup::setActiveCard(CardWindow* c)
{
	if (!c || c->cardGroup() != this)
		return false;

	m_activeCard = c;

	return true;
}

void CardGroup::moveToActiveCard() {
	for (int i=m_cards.size()-1; i>=0; i--) {
			if (m_cards[i] == m_activeCard) {
				m_currentPosition = i;
				clampCurrentPosition();
			}
	}
}

bool CardGroup::makeNextCardActive()
{
	int index = m_cards.indexOf(m_activeCard);
	if (index == -1 || index == m_cards.size() - 1)
		return false;

	index++;
	m_activeCard = m_cards[index];
	m_currentPosition = index;
	clampCurrentPosition();

	return true;
}

void CardGroup::makeBackCardActive()
{
	if (m_cards.empty())
		return;

	m_activeCard = m_cards.first();
	m_currentPosition = 0;
	clampCurrentPosition();
}

bool CardGroup::makePreviousCardActive()
{
	int index = m_cards.indexOf(m_activeCard);
	if (index == -1 || index == 0)
		return false;

	index--;
	m_activeCard = m_cards[index];
	m_currentPosition = index;
	clampCurrentPosition();

	return true;
}

void CardGroup::makeFrontCardActive()
{
	if (m_cards.empty())
		return;

	m_activeCard = m_cards.last();
	m_currentPosition = m_cards.size()-1;
	clampCurrentPosition();
}

void CardGroup::raiseCards()
{
	if (m_cards.empty())
		return;

	QGraphicsItem* parent = 0;
	Q_FOREACH(CardWindow* win, m_cards) {
		bool restoreFocus = win->hasFocus();
		parent = win->parentItem();
		win->scene()->removeItem(win);
		win->setParentItem(parent);
		if (restoreFocus) {
			win->setFocus();
		}
	}
}

void CardGroup::adjustHorizontally(qreal xDiff)
{
	if (m_cards.empty() || atEdge(xDiff))
		return;

	// m_currentPosition can shift a total of 3 cards with one full drag
	static const qreal pixelsPerPos = m_cards[0]->boundingRect().width() / kPositionsPerWidth;
	m_currentPosition += (-xDiff/pixelsPerPos);
	clampCurrentPosition();
}

bool CardGroup::atEdge(qreal direction) const
{
	// quick test for the common case
	if (m_cards.size() <= kMaxStationaryCards)
		return true;

	if (direction > 0.0) {
		return m_currentPosition == 1.0;
	}
	// direction <= 0.0
	return m_currentPosition == (qreal) (m_cards.size() - kMaxStationaryCards + 1);
}

void CardGroup::flick(int xVelocity)
{
	if (m_cards.empty())
		return;

	// adjust the current position within the group based on the users velocity
	static const qreal velocityPerPos = kVelocityPerPosition / kPositionsPerVelocity;
	m_currentPosition += (-xVelocity/velocityPerPos);
	clampCurrentPosition();	
}

void CardGroup::enableShadows()
{
	Q_FOREACH(CardWindow* c, m_cards) {
		c->enableShadow();
	}
}

void CardGroup::disableShadows()
{
	Q_FOREACH(CardWindow* c, m_cards) {
		c->disableShadow();
	}
}

void CardGroup::setCompositionMode(QPainter::CompositionMode mode)
{
	Q_FOREACH(CardWindow* c, m_cards) {
		c->setPaintCompositionMode(mode);
	}
}

void CardGroup::resize(int width, int height, QRect normalScreenBounds)
{
	Q_FOREACH(CardWindow* c, m_cards) {
		if(c->type() != Window::Type_ModalChildWindowCard) {
			c->resizeWindowBufferEvent(width, height, normalScreenBounds);
		}
	}
}

bool CardGroup::withinColumn(const QPointF& pt) const
{
	return ((pt.x() <= m_pos.x() + m_rightWidth) && (pt.x() >= m_pos.x() - m_leftWidth));
}

void CardGroup::setPos(const QPointF& pos)
{
	// update the anchor point for all cards in the group
	m_pos = pos;

	for (int i=0; i<m_cards.size(); i++) {
		if (m_cards[i]->attachedToGroup())
			m_cards[i]->setPos(m_pos);
	}
}

void CardGroup::setX(const qreal& x)
{
	// update the anchor point for all cards in the group
	m_pos.setX(x);

	for (int i=0; i<m_cards.size(); i++) {
		if (m_cards[i]->attachedToGroup())
			m_cards[i]->setX(x);
	}
}

void CardGroup::setY(const qreal& y)
{
	// update the anchor point for all cards in the group
	m_pos.setY(y);

	for (int i=0; i<m_cards.size(); i++) {
		if (m_cards[i]->attachedToGroup()) {
			m_cards[i]->setY(y);
		}
	}
}

QVector<CardWindow::Position> CardGroup::calculateOpenedPositions(qreal xOffset)
{

	if (m_cards.empty())
		return QVector<CardWindow::Position>();

	static const qreal kCardWidth = m_cards[0]->boundingRect().width();

	QVector<CardWindow::Position> positions(m_cards.size());
	qreal activeCardWidth = kCardWidth * m_curScale;

	clampCurrentPosition();

	qreal rOff = ((m_curScale*50) + activeCardWidth);
	qreal lOff = -(m_curScale*100);
	qreal rot = m_curScale * m_cardGroupRotFactor;
	for (int i=0; i<m_cards.size(); i++) {

		qreal x = ((i - m_currentPosition) / 3.0) * activeCardWidth * m_cardGroupXDistanceFactor;
		if (x > rOff)
			x = (x + (rOff * 4)) / 5;
		else if (x < lOff)
			x = (x + (lOff * 4)) / 5;

		positions[i].trans.setX(x);
		positions[i].trans.setZ(m_curScale);
		positions[i].trans.setY(x > 0 ? x/15 : 0);
		positions[i].zRot = x/rot;

		if (xOffset != 0) {

			qreal maxDistUngrouped = activeCardWidth;
			qreal amtToCollapse = qMax((qreal)1.0, maxDistUngrouped - qAbs(xOffset)) / maxDistUngrouped;
			positions[i].trans.setX((x*amtToCollapse/*+xOffset*/) + (1-amtToCollapse) * 10 * (i));
			positions[i].trans.setY(positions[i].trans.y() * amtToCollapse);
			positions[i].trans.setZ(m_nonCurScale + (m_curScale-m_nonCurScale) * amtToCollapse);
			positions[i].zRot *= amtToCollapse;
		}
	}

	// update final width of group in parent group coords
	m_leftWidth = (int) qAbs(positions.first().toTransform().mapRect(m_cards.first()->boundingRect()).left());
	m_rightWidth = (int) qAbs(positions.last().toTransform().mapRect(m_cards.last()->boundingRect()).right());

	return positions;
}

QVector<CardWindow::Position> CardGroup::calculateClosedPositions()
{
	if (m_cards.empty())
		return QVector<CardWindow::Position>();
	
	QVector<CardWindow::Position> positions(m_cards.size());

	// space out cards in the x
	int xOff = 0;
	int stopSpacingIndex = m_cards.size() > kMaxClosedSpacedCards ? 
						   m_cards.size() - kMaxClosedSpacedCards : 
						   0;
	for (int i=m_cards.size()-1; i>=0; i--) {

		positions[i].trans.setX(xOff);
		positions[i].trans.setZ(m_nonCurScale);

		if (i >= stopSpacingIndex) {
			xOff -= 7; // NOTE: should we scale the spacing?
		}
	}

	// update final width of group in parent group coords
	m_leftWidth = (int) qAbs(positions.first().toTransform().mapRect(m_cards.first()->boundingRect()).left());
	m_rightWidth = (int) qAbs(positions.last().toTransform().mapRect(m_cards.last()->boundingRect()).right());

	return positions;
}

void CardGroup::clampCurrentPosition()
{
	if (m_cards.size() <= 1) {

		m_currentPosition = 0.0;
	}
	else if (m_cards.size() == 2) {

		m_currentPosition = 0.5;
	}
	else if (m_cards.size() > 2 && m_cards.size() <= kMaxStationaryCards) {

		m_currentPosition = 1.0;
	}
	else {

		m_currentPosition = qBound((qreal)1.0, m_currentPosition, (qreal) (m_cards.size() - kMaxStationaryCards + 1));
	}

}

