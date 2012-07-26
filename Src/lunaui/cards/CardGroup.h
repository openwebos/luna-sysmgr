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




#ifndef CARDGROUP_H_
#define CARDGROUP_H_

#include "Common.h"

#include <QVector>
#include <QList>
#include <QEasingCurve>
#include <QObject>
#include <QPointF>

#include "CardWindow.h"

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
QT_END_NAMESPACE

class CardGroup : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QPointF pos READ pos WRITE setPos)
	Q_PROPERTY(qreal x READ x WRITE setX)
	Q_PROPERTY(qreal y READ y WRITE setY)

public:

	CardGroup(qreal curScale, qreal nonCurScale);
	~CardGroup();

	// adds an ungrouped card to this group and makes it the active card
	void addToGroup(CardWindow* c);
	// removes the card from this group (if grouped) and sets the card 
	// to the left as the new active card
	void removeFromGroup(CardWindow* c);
	// adds an ungrouped card to the top of the groups stacking order
	void addToFront(CardWindow* c);
	// adds an ungrouped card to the bottom of the groups stacking order
	void addToBack(CardWindow* c);

	// raises/lowers the current active card to the next/previous spot
	// within the group. - direction lowers, + direction raises.
	// returns whether the move was successful.
	// NOTE: does not modify the GraphicsScene stacking order, you must 
	// explicitly call raiseCards().
	bool moveActiveCard(int direction);

	// raises/lowers the current active card to the next/previous spot 
	// within the group. changes positions if the center point of the active
	// card, passes the center of another card.
	bool moveActiveCard();

	// moves the current position within the group to the next/previous whole
	// increment. - direction lowers, + direction raises.
	// returns whether the move was successful.
	bool moveCurrentPosition(int direction);

	QList<QPropertyAnimation*> animateOpen(int duration, 
										   QEasingCurve::Type curve, 
										   bool includeActiveCard=true);
	QList<QPropertyAnimation*> animateClose(int duration,
											QEasingCurve::Type curve,
                                            bool useGroupPosition=false);
    // calculate a list of animations for fanning the group based on the
    // group x offset passed instead of the groups current x offset
    QList<QPropertyAnimation*> animateCloseWithOffset(int duration,
                                            QEasingCurve::Type curve,
                                            int groupXOffset);

	QList<QPropertyAnimation*> maximizeActiveCard(qreal centerOffset);

	void layoutCards(bool open,bool includeActiveCard);

	void maximizeActiveCardNoAnimation(qreal centerOffset);

	// sets the active card that contains scenePt,
	// returns false if scenePt doesn't fall within a card
	bool setActiveCard(QPointF scenePt);
	// sets the active card if card is contained in this group
	// returns false if the card doesn't belong to this group
	bool setActiveCard(CardWindow* card);

	// changes the active card to be the next in the group.
	// returns false if the active card wasn't changed.
	bool makeNextCardActive();
	// changes the active card to be the last card in the group.
	void makeFrontCardActive();

	// changes the active card to be the previous in the group.
	// returns false if the active card wasn't changed.
	bool makePreviousCardActive();
	// changes the active card to be the first card in the group.
	void makeBackCardActive();

	// move all cards in this group to the top within their parent
	// preserving stacking order within the group
	void raiseCards();

	CardWindow* activeCard() const { return m_activeCard; }

	int size() const { return m_cards.size(); }
	bool empty() const { return m_cards.empty(); }

	// are there any cards to the left/right of the current active card?
	bool atEdge(qreal direction) const;
	// shift the current position within the group by some ratio
	void adjustHorizontally(qreal xDelta);
	// adjust the current position within the group based on some velocity
	void flick(int xVelocity);

	void enableShadows();
	void disableShadows();
	void setCompositionMode(QPainter::CompositionMode mode);
	void resize(int width, int height, QRect normalScreenBounds);

	// returns the width of the final transformed group
	int width() const { return m_leftWidth + m_rightWidth; }
	// returns the width to the left of the groups origin
	int left() const { return m_leftWidth; }
	// returns the width to the right of the groups origin
	int right() const { return m_rightWidth; }
	// returns true if the pt falls within the column of this group
	bool withinColumn(const QPointF& pt) const;

	// updates the center position of the group in parent coordinates
	QPointF pos() const { return m_pos; }
	void setPos(const QPointF& pos);

	qreal x() const { return m_pos.x(); }
	void setX(const qreal& x);

	qreal y() const { return m_pos.y(); }
	void setY(const qreal& y);
	bool shouldMaximizeOrScroll(QPointF scenePt);
	bool testHit(QPointF scenePt);
	void moveToActiveCard();

	QVector<CardWindow*> cards() const { return m_cards; }
private:

	QVector<CardWindow::Position> calculateOpenedPositions(qreal xOffset = 0.0);
	QVector<CardWindow::Position> calculateClosedPositions();

	void clampCurrentPosition();

	QPointF m_pos;
	qreal m_curScale;
	qreal m_nonCurScale;
	int m_leftWidth;
	int m_rightWidth;
	QVector<CardWindow*> m_cards;
	CardWindow* m_activeCard;
	int m_cardGroupRotFactor;
	double m_cardGroupXDistanceFactor;
	// currentPosition is the position within an open card group.
	// 0: 	1 card is in the center of the group
	// .5: 2 cards are placed half way from the center of the group
	// 1: 	3-4 cards with the second card being the center of the group
	// N:	>4 cards where valid positions are between 1.0 and N - 4 + 1
	qreal m_currentPosition;
};

Q_DECLARE_METATYPE(CardWindow::Position)

#endif

