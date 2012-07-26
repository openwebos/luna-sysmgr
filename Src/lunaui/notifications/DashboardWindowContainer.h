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




#ifndef DASHBOARDWINDOWCONTAINER_H
#define DASHBOARDWINDOWCONTAINER_H

#include "Common.h"

#include <QList>
#include <QObject>
#include <QParallelAnimationGroup>
#include <QSet>
#include <QWeakPointer>
#include <QPointer>
#include <QPixmap>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
QT_END_NAMESPACE

#include "GraphicsItemContainer.h"
#include "VariantAnimation.h"

class QGraphicsPixmapItem;
class QGraphicsSceneMouseEvent;
class DashboardWindow;
class DashboardWindowManager;

class DashboardWindowContainer : public GraphicsItemContainer
{
	Q_OBJECT
public:

	DashboardWindowContainer(DashboardWindowManager* wm, int width, int height);
	virtual ~DashboardWindowContainer();

	void layoutAllWindowsInMenu();
	void setScrollBottom(int newBottom);
	void addWindow(DashboardWindow* win);
	void removeWindow(DashboardWindow* win);
	bool empty() const;

	inline int scrollBottom() const { return m_scrollBottom; }
	inline int viewportHeight() const { return m_viewportHeight; }
	inline int contentsHeight() const { return m_contentsHeight; }

	void resizeWindowsEventSync(int w);
	void focusAllWindows(bool focus);
	void sendClickToDashboardWindow(int num, QPointF tap, bool whileLocked);
	void resetLocalState(bool forceReset = false);
	int  getMaximumHeightForMenu() const;

	QRectF boundingRect() const;

	Q_INVOKABLE int getWidth() { return boundingRect().width(); }
	Q_INVOKABLE int getHeight() { return boundingRect().height(); }
	Q_INVOKABLE void mouseWasGrabbedByParent();

	QList<DashboardWindow*> windows() const { return m_items; }

	static int getDashboardWindowHeight() {
		return sDashboardWindowHeight;
	}

private:

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);
	virtual bool sceneEvent(QEvent* event);
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);
	virtual void paintInsideMenu(QPainter* painter);
	void paintHoriz3Tile(QPainter* painter, QPixmap* maskImg, int x, int y, int width, int height, int leftOffset, int rightOffset);
	void animateResize(int width, int height);
	void heightAnimationValueChanged( const QVariant & value);

	enum Operation {
		Invalid = 0,
		SingleWindowAdded,
		SingleWindowRemoved,
		MultipleWindowsRemoved
	};

	enum DeviceShowMasks {
		ShowNoMasks=0,
		TopMaskOnly,
		BottomMaskOnly,
		ShowBothMasks
	};

	enum FlickDirection {
		Ignore=0,
		FlickDown,
		FlickUp
	};

	enum WindowPosition {
		Unknown=0,
		InvisibleAboveViewport,
		Visible,
		InvisibleBelowViewport,
		VisibleAboveDeletedItem,
		VisibleBelowDeletedItem
	};

Q_SIGNALS:

	void signalWindowAdded(DashboardWindow* w);
	void signalWindowsRemoved(DashboardWindow* w);
	void signalViewportHeightChanged();
	void signalEmpty();
	void signalContainerSizeChanged();
	void signalItemDragState(bool itemBeingDragged);

private Q_SLOTS:

	void slotProcessAnimationComplete();
	void slotDeleteAnimationFinished();
	void slotNegativeSpaceAboutToChange(const QRect& r, bool, bool screenResizing);
	void slotNegativeSpaceChanged(const QRect& r);
	void slotNegativeSpaceChangeFinished(const QRect& r);

private:

	void initPixmaps();
	void calculateScrollProperties();
	void restoreNonDeletedItems(bool recalcScrollBottom=true);
	void showOrHideMasks();
	void handleTap(const QPointF& pos);
	void triggerItemDelete(DashboardWindow* win);
	void animateWindowsToFinalDestination(int yCoOrd);
	void animateWindowsToFinalDestinationInMenu(int topCoord);
	void setWindowsToFinalDestinationInMenu(int topCoord);
	void setMaskVisibility(bool& topMask, bool& bottomMask);

	//bool isViewPortShowingAllItems();

	DeviceShowMasks m_maskDisplayStatus;
	Operation m_operation;
	DashboardWindowManager* m_wm;
	bool m_trackingMouseDirection;
	bool m_vertLockedMovement;
	QList<DashboardWindow*> m_items;
	QSet<DashboardWindow*> m_pendingDeleteItems;
	QSet<DashboardWindow*> m_deletedItems;
	QPointer<QPropertyAnimation> m_updateViewPortAnimation;
	QWeakPointer<DashboardWindow> m_draggedWindow;

	bool m_isMenu;

	static const int sDashboardWindowHeight;
	static const int sDashboardBadgeWidth;
	int m_itemsDeleted;
	int m_contentsHeight;
	int m_viewportHeight;
	int m_scrollBottom;
	int m_IndexOfDeletedItem;
	bool m_verticalMouseMoveInProgress;
	bool m_seenFlick;
	bool m_isViewPortAnimationInProgress;
	bool m_isWindowBeingDeleted;
	bool m_animateVisibleViewportHeight;
	FlickDirection m_FlickDirection;
	mutable int m_DashboardTopPadding;
	mutable int m_BottomMaskHeightCorrection;
	int m_menuSeparatorHeight;
	bool m_dashboardManualDrag;

	QParallelAnimationGroup m_anim;
	QParallelAnimationGroup m_deleteAnim;
	VariantAnimation<DashboardWindowContainer> m_heightAnimation;
	QPixmap* m_tabletBackground;
	QPixmap* m_tabletTopMask;
	QPixmap* m_tabletbottomMask;
	QPixmap* m_tabletArrowUp;
	QPixmap* m_tabletArrowDown;
	QPixmap* m_menuSwipeBkg;
	QPixmap* m_menuSwipeHighlight;
	QPixmap* m_itemSeparator;
};

#endif /* DASHBOARDWINDOWCONTAINER_H */
