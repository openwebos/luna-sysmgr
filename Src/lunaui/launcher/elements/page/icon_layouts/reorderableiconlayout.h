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




#include "iconlayout.h"
#include "icon.h"

#include <QList>
#include <QString>
#include <QDebug>
#include <QMap>
#include <QPointer>
#include <QUuid>

#ifndef REORDERABLEICONLAYOUT_H_
#define REORDERABLEICONLAYOUT_H_

class ReorderablePage;
class PixmapObject;
class QAnimationGroup;
class IconLayoutAnimationControl;
class IconReorderAnimation;
class QStateMachine;
class QState;

class ReorderableIconLayout : public IconLayout
{
	Q_OBJECT
public:

	friend class ReorderablePage;

	ReorderableIconLayout(ReorderablePage * p_owner);
	virtual ~ReorderableIconLayout();

	virtual void setLayoutRowSpacing(const qreal interRowSpace,
									const quint32 anchorRowNum=0);

	virtual void resetLayoutRowSpacingToDefaultSettings();

	virtual void paint(QPainter * painter);
	//	(ScrollableObject-->ScrollingLayoutRenderer-->here)
		//	"clipping", but by only drawing from the source area specified. sourceRect guaranteed to be within
		//		m_geom space
	virtual void paint(QPainter * painter, const QRectF& sourceRect);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt);
	virtual void paint(const QPointF& translate,QPainter * painter);

	virtual void paintOffscreen(QPainter * painter);
	virtual void paintOffscreen(PixmapObject * p_pmo);
	virtual void paintOffscreen(PixmapHugeObject * p_hugePmo);

	virtual void enableAutoPaint();
	virtual void disableAutoPaint();

	//relayout() sets absolute positioning within the item (Page or a Page-subclass) of layout, rows, and cells
	virtual void relayout(bool force=false);
	virtual void resizeWidth(const quint32 w);
	virtual void setPosition(const QPointF& pos);
	virtual void setUniformCellSize(const QSize& size);

	// intended to tell a cell that the icon is losing control of an icon (usually for moving/animating it to a new cell)
	virtual void iconCellReleaseIcon(const QPoint& cellCoordinate);

	virtual IconCell * iconCellAtLayoutCoordinate(const QPointF& coordinate);
	virtual IconCell * iconCellAtLayoutCoordinate(const QPointF& layoutCoordinate,QPoint& r_gridCoordinate);

	//returns the rect area of the row specified, in layout CS (it's the geom of the row, translated to its layout CS position)
	virtual QRectF rowArea(quint32 rowIndex) const;

	virtual QPoint lastOccupiedGridPosition() const;
	virtual QPoint nextAppendGridPosition() const;

	virtual qint32 rowAtLayoutCoordinate(const QPointF& layoutCoordinate,bool clipMinMax=false);
	//this variant will ignore the gaps between rows; if the point happens to fall on a gap between the rect or row "i" and row "i+1", "i" will be returned
	virtual	qint32 rowAtLayoutCoordinateFuzzy(const QPointF& layoutCoordinate,bool clipMinMax=false);

	//these two can only be used with grid coordinates that already exist. e.g. it can't determine coordinates
	// for a grid that would result if a row needs to be added; This would happen for example, if an icon
	// add would end up adding a new row for "overflow" of a row with too many icons
	virtual IconCell * iconCellAtGridCoordinate(const QPoint& gridCoordinate);
	virtual bool layoutCoordinateForGridCoordinate(const QPoint& gridCoordinate,QPointF& r_layoutCoordinate);

	virtual IconCell * findIconByUid(const QUuid& iconUid,QPoint& r_gridCoordinate,bool includePendingIconsInCells = false);

	// all the icon cells, starting with the leftmost cell of the top row, and ending with the rightmost cell in the bottom row
	//WARNING: DO NOT HOLD REF TO ANYTHING FROM THE RETURN OF THIS FN. It could be invalidated at any time (whenever an add/remove or relayout happens)
	virtual QList<IconCell *> iconCellsInFlowOrder();

	// returns 0 (null) if tracking this icon isn't possible at the moment
	// designed so that iconCellAtLayoutCoordinate() result can be passed right in
	// 	(it will correctly handle null (not found) icon cell
	virtual IconBase * startTrackingIcon(const QPoint& gridCoord,bool includePendingIconsInCells = false);
	//startTrackingIconFromTransfer(): gridCoord needs to be an empty cell; see addEmptyCell
	virtual IconBase * startTrackingIconFromTransfer(const QPoint& gridCoord,IconBase * iconTracked);
	//this variant starts tracking an icon by finding it in the layout by its actual pointer. it is used in the case where the icon
	// comes back from the quicklaunch during one, continuous tracking operation (the following pattern: move icon to QL, don't let go
	// of the icon on the QL but immediately move it back to the page). Returns true if the icon was found and can be tracked,
	//	and puts its current grid coord in r_gridCoord.
	virtual bool startTrackingIcon(IconBase * p_icon,QPoint& r_gridCoord);

	virtual bool trackedIconMovedTo(IconBase * p_icon,const QPoint& gridCoord,QPoint& r_newGridCoord);
	virtual void stopTrackingIcon(IconBase * p_icon);
	virtual void stopTrackingAll();
	//trackedIconLeavingLayout(): used instead of stopTrackingIcon() when the icon has changed hands;
	//	e.g. been moved to another page/layout
	virtual bool trackedIconLeavingLayout(const QUuid& trackedIconUid);
	//convenience function to look up the uid against m_trackedIconLastPosition and retrieve from icon heap
	virtual IconBase * getTrackedIconByUid(const QUuid& trackedIconUid);
	virtual bool lastTrackedPosition(const QUuid& iconUid,QPoint& r_lastGridCoord);
	virtual QList<IconOperation> opListForMove(const QPoint& sourceColumnRow,const QPoint& destColumnRow);
	//returns the animations to perform the actions described in the opList
	virtual QAnimationGroup * animationsForOpList(QList<IconOperation>& opList);
	//uses the op list to set m_reorderPendingCellList which will be needed to commit/finalize the layout;
	// this must be done regardless of whether or not animations will be used. It's being broken out as a separate function
	// despite this being a hard requirement, so that this sequence can be better controlled.
	// it will fail (return false) if attempting to init from a list with a lower id than has already been used
	virtual bool initializePendingCellListFromOpList(QList<IconOperation>& opList);
	//this operates on an already populated m_reorderPendingCellList. It's used to finalize a layout
	virtual bool commitPending();
	//this one will do all the tracked (floating) icons
	virtual bool commitTracked(const QUuid& iconUid);
	//variant to avoid multiple lookups when the caller already did a lookup
	virtual bool commitTracked(const QUuid& iconUid,const QPoint& lastGridCoord);
	// essentially the same thing as calling animationsForOpList + commitOpList as an uninterrupted sequence
	//	returns false if the list can't be commited (usually because a list of higher id has already been committed)
	//	The success / failure is all-or-nothing...nothing will be changed by this call of it if return = false
	virtual bool executeOpList(QList<IconOperation>& opList);

	//will create and return an animation that is made to return the tracked icon in question to the last cell that
	// it moved to (see trackedIconMovedTo()). Use this animation IMMEDIATELY, and certainly before any other calls to
	// trackedIconMovedTo(), or any of the others in that group (that change the grid position of a tracked icon) occur.
	virtual IconReorderAnimation * animationForTrackedIconFinal(const QUuid& trackedIconUid);
	virtual IconReorderAnimation * animationForTrackedIconFinal(const QUuid& trackedIconUid,const QPoint& lastGridCoord);
	//Just like stopTracking__() and commit___() operate under normal circumstances and try to let animations finish,
	//cancelAllReorder() kills all animations instantly and immediately locks all icons into their final state, as if
	// the normal reorders were allowed to finish. This is useful in case of some drastic UI change, like an incoming call,
	// a rotation of the screen, the launcher being dismissed, apps being deleted, etc...
	// the function has no return value because it cannot fail; if it fails to do anything, then a crash will likely follow
	// (or at the very least, a very inconsistent, mostly unusable launcher UI state)
	virtual void cancelAllReorder();
	virtual void commitPendingImmediately();
	virtual void commitTrackedImmediately(const QUuid& iconUid);
	virtual void commitTrackedImmediately(const QUuid& iconUid,const QPoint& lastGridCoord);

	virtual bool areTherePendingReorderAnimations();

///------
	friend QDataStream & operator<< (QDataStream& stream, const ReorderableIconLayout& s);
	friend QDataStream & operator>> (QDataStream& stream, ReorderableIconLayout& s);
	friend QDebug operator<<(QDebug dbg, const ReorderableIconLayout &s);

	/*
	 * 				addIcon(IconBase * p_icon)
	 *
	 * Inserts the icon into the row and column that represents the "end" of the layout
	 * Since ReorderableIconLayout doesn't have any specific ordering (it is arbitrarily ordered by the user),
	 * the end of the layout is just the next available space
	 *
	 * Mostly useful for initializing a layout initially. Generally NOT useful at user-visible runtime since
	 * it will not calculate any of the animations needed to display the "add"
	 *
	 * Also, it will not automatically adjust the layout geoms of the icon cells. It is expected a caller
	 * will be handling this.
	 *
	 * returns either 0 if it inserted into an existing row or took no action, or (row index)+1 if it had to insert a new row
	 * this value helps the caller determine if it needs to run a relayout of rows, and possibly optimize the relayout
	 * over only some rows
	 *
	 * It also sets the m_layoutSync variable if any layout state was modified (this disambiguates return=0)
	 */
	virtual quint32 addIcon(IconBase * p_icon);

	//this is its counterpart, used for relayoutExisting()
	virtual quint32 addIcon(IconCell * p_cell);

	//returns the grid coordinate of the new cell, or (-1,-1) if no new cell can be added
	virtual QPoint addEmptyCell();

	//versions of the above that insert into specific locations. If gridCoordinate is invalid, then it will default to (0,0)
	// return convention is the same as for addIcon
	// however, if these return > 0 , relayoutExisting() MUST BE RUN SOMETIME SOON AFTER!
	// (it's an optimization to avoid having to "ripple down" the right most icon at each row and adding the
	//	final "leftover icon" into the last row or creating a new row at the bottom. relayoutExisting already
	//  does all this so it'll be a waste to do it here. Philosophically, it would be more correct to have this
	//	function at least maintain the rows w.r.t. the ordering and max column count, but like i said, this is an
	//	optimization)
	virtual quint32 addIconAt(IconBase * p_icon,const QPoint& gridCoordinate);
	virtual quint32 addIconAt(IconCell * p_cell,const QPoint& gridCoordinate);

	//(this one also has the same return convention as addEmptyCell, and will default to appending to the end of the layout (same as addEmptyCell)
	//	if gridCoordinate is invalid)
	virtual QPoint addEmptyCellAt(const QPoint& gridCoordinate);

	virtual bool removeIconCell(const QPoint& gridCoordinate);

	///////////////// init functions ///////////////////////////////////////////////////////
	/*
		These are versions of common functions like "insert" which are used
		at runtime, but these variants should ONLY be used during initialization, as they
		will likely and frequently omit setting/checking/returning/etc state that is required
		to make things "look right" in the UI.
		They are all prefixed with "init" should it should be easy to identify them and keep
		track of this rule
	*/
	////////////////////////////////////////////////////////////////////////////////////////

	static void initLayoutFromSequentialIconList(ReorderableIconLayout& layout,
														const IconList iconList);
Q_SIGNALS:

//		These two signals are the overall indicators of when a reordering begins and ends.
//		reordering here means more than just "reorder pending" (vs moving/float icon tracking),
//		it means *overall* reorder , i.e. the state is not consistent from the POV of the outside world
	void signalReorderStarted();
	void signalReorderEnded();

//		these are for the FSM !ONLY!
	void signalFSMTrackStarted_Trigger();
	void signalFSMTrackEnded_Trigger();
	void signalFSMLastTrackEndedTrigger();			//TODO: HACK: see slotTrackForIconEnded() below
	void signalFSMReorderStarted_Trigger();
	void signalFSMReorderEnded_Trigger();

protected Q_SLOTS:

	virtual void slotReorderAnimationsFinished();
	virtual void slotTrackedIconReplacementAnimationFinished();

	//TODO: HACK: TEMP: the FSM should actually count the number of in-flight trackings and switch states accordingly
	//		Qt's guarded (conditional) transitions are kind of weird so i'm holding off on implementing this for now
	//		this function will check the number of in-flights and emit the signalFSMLastTrackEndedTrigger as appropriate
	//			I'm weaving it this way with a signal->slot inside this (same) class - usually a bad idea - to minimize the
	//			changes needed when this gets done correctly
	virtual void slotTrackForIconEnded();

	virtual void dbg_reorderFSMStateEntered();

protected:

	//deletes all the rows (but not the cells contained) and re-inits my state variables to reflect the change
	/// used by relayoutExisting()
	virtual void destroyAllRows();

	//TODO: return an error code as appropriate; calculations can fail if either the page width is too small or icon size if too great
	virtual qint32 calculateAndSetHorizontalSpaceParameters();

	/*
	 * when resizing the layout when it has already been done, there is a potential that rows may need to be added
	 * (when the width shrinks) or rows may need to be removed (when the width expands)
	 *
	 * This function will handle these adds and removes. It is a counterpart to relayout(); that function is kind of a misnomer
	 *	since it doesn't really RE-layout...it is more for initial layouts
	 *
	 *	TODO: initially, this will delete all the rows but keep the cells, and then recreate the rows, adding/removing as necessary.
	 *		This can be made more efficient, but I don't have time right now, due to Demo-Demo-Demo!
	 */
	virtual void relayoutExisting();

	virtual void setupReorderFSM();
	virtual void startReorderFSM();
	virtual void stopReorderFSM();

	virtual bool isReorderStateConsistent() const;

	virtual void switchIconsToReorderGraphics();
	virtual void switchIconsToNormalGraphics();
	virtual void switchIconToReorderGraphics(IconCell * p_iconCell);
	virtual void switchIconToNormalGraphics(IconCell * p_iconCell);
	virtual void switchIconToReorderGraphics(IconBase * p_icon);
	virtual void switchIconToNormalGraphics(IconBase * p_icon);

protected:

	typedef QList<IconRow *> IconRowList;
	typedef IconRowList::const_iterator IconRowConstIter;
	typedef IconRowList::iterator IconRowIter;

	IconRowList m_iconRows;

	bool	m_layoutSync;
	quint32 m_relayoutCount;
	bool	m_disabledAutoPaint;

	bool	m_usingReorderModeGraphics;

	//all units here in pixels
	quint32 m_maxWidthForRows;
	quint32 m_maxIconsPerRow;
	qint32 m_horizIconSpacingAdjust;
	quint32 m_interRowSpace;		//between two rows

	//	Layout adjustments - all of this comes from IconLayoutSettings
	quint32 m_leftMarginForRows;
	quint32 m_topMarginForRows;
	/// ----------------

	quint32 m_anchorRow;			//the current anchor row (from which (re)layouts are performed)

	QSize	m_iconCellSize;				//calculated from page size and m_maxIconsPerRow...see initLayout...()
	quint32 m_horizIconSpacing;			//(ditto)...takes into account m_horizIconSpacingAdjust

	QSize	m_layoutSizeInPixels;		//for rendering offscreen


	quint32 m_reorderEventSampleRate;	//determines when to check a moving icon against the layout to see what
										// if anything, needs to be reordered. For now, it will count instances
										// of "move" events (e.g. mouseMove) but I'll add other ways to threshold
										// This is useful because checking boundaries on icons and rows in very large
										// layouts could be expensive, as is generating operation lists and animations
	quint32 m_maxVelocityForSampling;
	quint64 m_timeNormalizationUnit;
	quint32 m_magFactor;
	quint64 m_lastTimeUsed;

	quint64						m_listIdInUse;
	QList<QPointer<IconCell> > m_reorderPendingCellList;
	QPointer<QAnimationGroup> m_qp_reorderAnimationGroup;
	QMap<QUuid,QPointer<IconReorderAnimation> > m_trackedIconAnimations;
	QMap<QUuid,QPoint> 	m_trackedIconLastPosition;

	static const char * ReorderFSMPropertyName_isConsistent;

	QStateMachine * m_p_reorderFSM;
	QState * m_p_fsmStateConsistent;
	QState * m_p_fsmStateReorderPending;
	QState * m_p_fsmStateTrackedFloating;
	QState * m_p_fsmStateReorderPendingAndTrackedFloating;

};

QDataStream & operator<< (QDataStream& stream, const ReorderableIconLayout& s);
QDataStream & operator>> (QDataStream& stream, ReorderableIconLayout& s);
QDebug operator<<(QDebug dbg, const ReorderableIconLayout &s);

#endif /* REORDERABLEICONLAYOUT_H_ */
