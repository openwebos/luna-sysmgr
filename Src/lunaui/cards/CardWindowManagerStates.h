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




#ifndef CARDWINDOWMANAGERSTATES_H
#define CARDWINDOWMANAGERSTATES_H

#include "Common.h"

#include <QState>
#include <QGraphicsSceneMouseEvent>
#include <QRect>
#include <QSignalTransition>
#include <QGraphicsItem>
#include <stdint.h>

class CardWindow;
class CardWindowManager;

QT_BEGIN_NAMESPACE
class QGestureEvent;
class QTapGesture;
class QTapAndHoldGesture;
QT_END_NAMESPACE

class CardWindowManagerState : public QState
{
	Q_OBJECT

public:
	CardWindowManagerState(CardWindowManager* wm);

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) {}
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {}
	virtual void flickGestureEvent(QGestureEvent* event) {}
	virtual void tapGestureEvent(QTapGesture* event) {}
	virtual void tapAndHoldGestureEvent(QTapAndHoldGesture* event) {}

	virtual void windowAdded(CardWindow* win);
	virtual void windowRemoved(CardWindow* win);
	virtual void windowTimedOut(CardWindow* win) {}

	virtual void positiveSpaceAboutToChange(const QRect& r, bool fullScreen) {}
	virtual void positiveSpaceChangeFinished(const QRect& r) {}
	virtual void positiveSpaceChanged(const QRect& r) {}

	virtual void animationsFinished() {}
	virtual void changeCardWindow(bool) {}

	virtual void focusMaximizedCardWindow(bool focus);

	virtual bool supportLauncherOverlay() const;

	virtual void relayout(const QRectF& r, bool animate=true) {}

    virtual void processTouchToShareTransfer(const std::string& appId) {}

    virtual bool handleKeyNavigation(QKeyEvent* keyEvent) { return true; }

protected:
	virtual void onEntry(QEvent* event);
	bool lastWindowAddedType() const;
	void resizeWindow(CardWindow* w, int width, int height);

	CardWindowManager* m_wm;
};

// -----------------------------------------------------------------------------------

class MinimizeState : public CardWindowManagerState
{
	Q_OBJECT

public:
	MinimizeState(CardWindowManager* wm) 
				: CardWindowManagerState(wm) { setObjectName("Minimize"); }

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

	virtual void flickGestureEvent(QGestureEvent* event);
	virtual void tapGestureEvent(QTapGesture* event);
	virtual void tapAndHoldGestureEvent(QTapAndHoldGesture* event);

	virtual void animationsFinished();
	virtual void changeCardWindow(bool);

	virtual void relayout(const QRectF& r, bool animate);

    virtual bool handleKeyNavigation(QKeyEvent* keyEvent);

protected:
	virtual void onEntry(QEvent* event);

Q_SIGNALS:
	void signalFirstCardRun();
};

// -----------------------------------------------------------------------------------

class MaximizeState : public CardWindowManagerState
{
	Q_OBJECT

public:
	MaximizeState(CardWindowManager* wm);

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);

	virtual void windowAdded(CardWindow* win);
	virtual void windowRemoved(CardWindow* win);

	virtual void positiveSpaceAboutToChange(const QRect& r, bool fullScreen);
	virtual void positiveSpaceChangeFinished(const QRect& r);
	virtual void positiveSpaceChanged(const QRect& r);

	virtual void animationsFinished();
	virtual void changeCardWindow(bool);

	virtual void relayout(const QRectF& r, bool animate=true);

	virtual void focusMaximizedCardWindow(bool focus);

    virtual void processTouchToShareTransfer(const std::string& appId);

    virtual bool handleKeyNavigation(QKeyEvent* keyEvent) { return false; }

protected:
	virtual void onExit(QEvent* event);
	virtual void onEntry(QEvent* event);

private Q_SLOTS:
	void slotIncomingPhoneCall();

private:
	void finishMaximizingActiveWindow();

	bool m_exiting; // are we exiting this state?
	int m_disableDirectRendering; // number of requests to disable direct rendering
};

// -----------------------------------------------------------------------------------

class FocusState : public CardWindowManagerState
{
public:
	FocusState(CardWindowManager* wm)
		: CardWindowManagerState(wm) { setObjectName("Focus"); }

	virtual void animationsFinished();

protected:
	virtual void onEntry(QEvent* event);
};

// conditional transition from MaximizeState to FocusState
class MaximizeToFocusTransition : public QSignalTransition
{
public:
	MaximizeToFocusTransition(CardWindowManager* wm, QState* target);

protected:
	virtual bool eventTest(QEvent* event);
};

// -----------------------------------------------------------------------------------

class PreparingState : public CardWindowManagerState
{
public:
	PreparingState(CardWindowManager* wm)
		: CardWindowManagerState(wm) { setObjectName("Preparing"); }

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);

	virtual void windowAdded(CardWindow* win);
	virtual void windowTimedOut(CardWindow* win);

	virtual bool supportLauncherOverlay() const;

protected:
	virtual void onEntry(QEvent* event);
};

// -----------------------------------------------------------------------------------

class LoadingState : public CardWindowManagerState
{
public:
	LoadingState(CardWindowManager* wm)
		: CardWindowManagerState(wm) { setObjectName("Loading"); }

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);

	virtual void windowAdded(CardWindow* win);

	virtual void animationsFinished();

	virtual bool supportLauncherOverlay() const;

    virtual void relayout(const QRect& r, bool animate);

protected:
	virtual void onExit(QEvent* event);
};

// -----------------------------------------------------------------------------------

class ReorderGrid : public QGraphicsItem
{
public:
	ReorderGrid(QGraphicsItem* parent, int slice);

	void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

	virtual QRectF boundingRect() const { return m_boundingRect; }

private:
	QRectF m_boundingRect;
	int m_slice;
};

class ReorderState : public CardWindowManagerState
{
public:
	ReorderState(CardWindowManager* wm)
		: CardWindowManagerState(wm), m_grid(0) { setObjectName("Reorder"); }

	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

	virtual void animationsFinished();

protected:
	virtual void onExit(QEvent* event);
	virtual void onEntry(QEvent* event);

private:
	ReorderGrid* m_grid;
};

#endif /* CARDWINDOWMANAGERSTATES_H */
