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




#ifndef ALPHABETICONLAYOUT_H_
#define ALPHABETICONLAYOUT_H_

#include "iconlayout.h"
#include "icon.h"

#include <QList>
#include <QString>
#include <QChar>
#include <QDebug>
#include <QMap>
#include <QSet>
#include <QPair>

class AlphabetIconLayout;
class AlphabetPage;
class PixmapObject;
class HorizontalDivider;
class HorizontalLabeledDivider;
class IconReorderAnimation;
class QStateMachine;
class QState;
class QAbstractAnimation;
class QPropertyAnimation;

class IconRowAlphaRange
{
public:
	quint32 m_startRowNum;
	quint32 m_endRowNum;
};
class IconRowAlpha : public IconRow
{
public:
	IconRowAlpha(AlphabetIconLayout * p_owner);
	IconRowAlpha(AlphabetIconLayout * p_owner,const QString& alphaDesignator);
	IconRowAlpha(AlphabetIconLayout * p_owner,const QChar& alphaDesignator);
	virtual ~IconRowAlpha();

	virtual void appendCell(IconCell * p_cell);
	virtual void addCell(IconCell * p_cell,const quint32 index);	//index > list size = append
	virtual void appendCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 alignToCellIndex);
	virtual void addCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 index,const quint32 alignToCellIndex);

	virtual void paintOffscreen(PixmapHugeObject * p_hugePmo);

	//remove() is done by the IconRow base class...remove fn is the same regardless of alpha nature of this row

	static bool iconCellLessThan(const IconCell * p_a, const IconCell * p_b);

	virtual void relayout(bool force=false);

	QString m_alphaDesignator;
	bool m_layoutSync;		//true if no append/add/remove has occured since last relayout of the row and cell geoms
							// false if a relayout is needed due to a mod of the icon list
							// expected to be used by the AlphabetIconLayout class to keep track of batched mods

	friend QDataStream & operator<< (QDataStream& stream, const IconRowAlpha& s);
	friend QDataStream & operator>> (QDataStream& stream, IconRowAlpha& s);
	friend QDebug operator<<(QDebug dbg, const IconRowAlpha &s);
};

class AlphabetIconLayout : public IconLayout
{
	Q_OBJECT
public:

	friend class IconRowAlpha;
	friend class AlphabetPage;

	AlphabetIconLayout(AlphabetPage * p_owner);
	virtual ~AlphabetIconLayout();

	QRectF constGeometry() const { return m_geom; }

	static void initDefaultEmptyLayoutFullEnglishAlpha(AlphabetIconLayout& layout);
	static void initLayoutFromSequentialIconListFullEnglishAlpha(AlphabetIconLayout& layout,
												const IconList iconList);

	//this version disregards m_intraAlphaRowSpace and m_interAlphaRowSpace and resets them to rowSpace
	virtual void setLayoutUniformRowSpacing(const qreal rowSpace,const quint32 anchorRowNum=0);
	//variant that allows indiv. spec
	virtual void setLayoutRowSpacing(const qreal intraAlphaRowSpace,
											const qreal interAlphaRowSpace,
											const quint32 anchorRowNum=0);

	//this version does it according to whatever is stored in IconLayoutSettings
	virtual void resetLayoutRowSpacingToDefaultSettings();

	//each returns a list of operations needed to move the OTHER icons around (icons already in the layout)
	virtual QList<IconOperation> addIconAt(quint32 row,quint32 column);
	virtual QList<IconOperation> removeIconFrom(quint32 row,quint32 column);

	virtual QList<quint32> rowListForAlpha(const QString& alphaDesignator);

	virtual void paint(QPainter * painter);
	virtual void paint(const QPointF& translate,QPainter * painter);
	//	(ScrollableObject-->ScrollingLayoutRenderer-->here)
	//	"clipping", but by only drawing from the source area specified. sourceRect guaranteed to be within
	//		m_geom space
	virtual void paint(QPainter * painter, const QRectF& sourceRect);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt);

	virtual void paintOffscreen(QPainter * painter);
	virtual void paintOffscreen(PixmapObject * p_pmo);
	virtual void paintOffscreen(PixmapHugeObject * p_hugePmo);

	virtual void enableAutoPaint();
	virtual void disableAutoPaint();

	virtual void relayout(bool force=false);
	virtual void resizeWidth(const quint32 w);
	virtual void setPosition(const QPointF& pos);
	virtual void setUniformCellSize(const QSize& size);

	virtual IconCell * iconCellAtLayoutCoordinate(const QPointF& coordinate);
	virtual IconCell * iconCellAtLayoutCoordinate(const QPointF& layoutCoordinate,QPoint& r_gridCoordinate);

	virtual IconCell * findIconByUid(const QUuid& iconUid,QPoint& r_gridCoordinate);

	virtual qreal	verticalDistanceToNearestUpperRow(const QPointF& layoutCoordinate);
	virtual qreal	verticalDistanceToNearestLowerRow(const QPointF& layoutCoordinate);

	virtual quint32 maximumRowHeight() const;
	virtual quint32 minimumRowHeight() const;

	friend QDataStream & operator<< (QDataStream& stream, const AlphabetIconLayout& s);
	friend QDataStream & operator>> (QDataStream& stream, AlphabetIconLayout& s);
	friend QDebug operator<<(QDebug dbg, const AlphabetIconLayout &s);

///////// icon tracking

	// returns false if can't track at the moment
	// if return == true, then the r_iconUid param holds value1 = ORIGINAL icon uid, and value2 = COPY (floating icon; the one being
	// designed so that iconCellAtLayoutCoordinate() result can be passed right in
	// 	(it will correctly handle null (not found) icon cell)
	virtual bool startTrackingIcon(const QPointF& layoutCoordinate,QPair<QUuid,QUuid>& r_iconUids);
	//trackedIconLeavingLayout(): used instead of stopTrackingIcon() when the icon has changed hands;
	//	e.g. been moved to another page/layout
	virtual bool trackedIconLeavingLayout(const QUuid& trackedIconUid);
	//stopTrackingIcon(): used when the icon has stayed in the page/layout, but just been dropped
	//	by the user
	virtual void stopTrackingIcon(const QUuid& trackedIconUid);
	//provide the TRACKED icon's uid, not the original's uid
	virtual IconBase * getTrackedIcon(const QUuid& trackedIconUid);

	virtual void stopTrackingAll();
	virtual bool commitTracked(const QUuid& iconUid);

	//animationForTrackedIconFinal(): create and return an animation that is made to "finalize" the moved icon. In the case of an alpha layout,
	// this will simply make the icon disappear.
	// NOTE: when the icon is actually sent to another page, than THAT page will handle its final animation,
	// 		so it would not be correct to create animations here that do this type of thing.
	virtual QAbstractAnimation * animationForTrackedIconFinal(const QUuid& trackedIconUid);

	//Just like stopTracking__() and commit___() operate under normal circumstances and try to let animations finish,
	//cancelAllReorder() kills all animations instantly and immediately locks all icons into their final state, as if
	// the normal reorders were allowed to finish. This is useful in case of some drastic UI change, like an incoming call,
	// a rotation of the screen, the launcher being dismissed, apps being deleted, etc...
	// the function has no return value because it cannot fail; if it fails to do anything, then a crash will likely follow
	// (or at the very least, a very inconsistent, mostly unusable launcher UI state)
	virtual void cancelAllReorder();
	virtual void commitTrackedImmediately(const QUuid& iconUid);

	virtual bool removeIconCell(const QPoint& gridCoordinate);

protected:
	typedef QList<IconRowAlpha *> IconRowAlphaList;
	typedef IconRowAlphaList::const_iterator IconRowConstIter;
	typedef IconRowAlphaList::iterator IconRowIter;
	IconRowAlphaList m_iconRows;	//always sorted in order of IconRow::m_logicalRowNumber,
										//which for the alpha layout is also alpha order

	typedef QMap<QString,HorizontalDivider *> RowDividerMap;
	typedef RowDividerMap::const_iterator RowDividerMapConstIter;
	typedef RowDividerMap::iterator RowDividerMapIter;
	RowDividerMap m_rowDividers;

	bool m_layoutSync;			//true if no append/add/remove has occured since last relayout of the row and cell geoms
								// false if a relayout is needed due to a mod of the icon list
								// expected to be used by owner Page to keep track of batched mods
								// (see IconRowAlpha)
	quint32 m_relayoutCount;	//every time relayout() is run, this gets incremented. It's used for both positioning initially,
								// when no anchor row has a valid position, and also for statistics and performance monitoring

	bool						m_disabledPaint;		//keeps the layout from painting itself (see paint())

	bool	m_usingReorderModeGraphics;

	//all units here in pixels
	quint32 m_maxWidthForRows;
	quint32 m_maxIconsPerRow;
	qint32 m_horizIconSpacingAdjust;

	//	Layout adjustments - all of this comes from IconLayoutSettings
	quint32 m_leftMarginForDividers;
	quint32 m_topMarginForDividers;
	quint32 m_leftMarginForRows;
	quint32 m_topMarginForRows;
	/// ----------------


	quint32 m_intraAlphaRowSpace;		//between two rows in the same alpha designator
	quint32 m_interAlphaRowSpace;		//between two rows in adjacent alpha designators;
										//make sure to leave room for separator bar, etc

	quint32 m_anchorRow;			//the current anchor row (from which (re)layouts are performed)

	QSize	m_iconCellSize;				//calculated from page size and m_maxIconsPerRow...see initLayout...()
	quint32 m_horizIconSpacing;			//(ditto)...takes into account m_horizIconSpacingAdjust;

	QSize	m_layoutSizeInPixels;		//for rendering offscreen

	quint32 m_minRowHeight;
	quint32 m_maxRowHeight;

protected:

	//disposes of the icon copy made while tracking a floating icon ("reorder", 'cept alphalayouts don't really reorder)
	virtual void destroyTrackedIcon(const QUuid& iconUid);

	//deletes all the rows (but not the cells contained) and re-inits my state variables to reflect the change
	/// used by relayoutExisting()
	virtual void destroyAllRows();

	// all the icon cells, starting with the leftmost cell of the top row, and ending with the rightmost cell in the bottom row
	//WARNING: DO NOT HOLD REF TO ANYTHING FROM THE RETURN OF THIS FN. It could be invalidated at any time (whenever an add/remove or relayout happens)
	virtual QList<IconCell *> iconCellsInFlowOrder();

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

	static bool iconRowLessThan(const IconRowAlpha * p_a,const IconRowAlpha * p_b);

	/*
	 * TODO: 	a higher level function that does true alpha insert (see caution)
	 *
	 * CAUTION: not a true alphabetic insert! It only inserts into the proper alpha ROW. It will NOT re-alpha-sort the icons within the row
	 * 			and more importantly, it will not swap between rows that are the same Alpha, in the case where an extra row of the same Alpha was added
	 * 			because the max number of icons per row were reached in the existing ones.
	 * 		The way to use this function to maintain true alphabetic order within rows and between rows is to pre-alphasort a list of icons to be inserted
	 * 		and then call this function sequentially on the list
	 *
	 * Inserts the icon into the row where it alphabetically belongs.
	 * This is totally different than the addIconAt function which isn't "smart"; this function actually
	 * chooses the row and column where the icon belongs.
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
	 *
	 */
	virtual quint32 insertAlphabetically(IconBase * p_icon);

	//this is its counterpart, used for relayoutExisting()
	virtual quint32 insertAlphabetically(IconCell * p_cell);

	virtual HorizontalLabeledDivider * createNewDivider(const QString& alphaLabel);
	virtual QSize horizontalDividerSize();

	QPropertyAnimation * fadeAnimationForTrackedIcon(IconBase * p_icon);

Q_SIGNALS:

	//		These two signals are the overall indicators of when a reordering begins and ends.
	//		reordering here means more than just "reorder pending" (vs moving/float icon tracking),
	//		it means *overall* reorder , i.e. the state is not consistent from the POV of the outside world
	void signalReorderStarted();
	void signalReorderEnded();

	//		these are for the FSM !ONLY!
	void signalFSMTrackStarted_Trigger();
	void signalFSMTrackEnded_Trigger();
	void signalFSMLastTrackEndedTrigger();			//TODO:  see slotTrackForIconEnded() below
	void signalFSMReorderStarted_Trigger();
	void signalFSMReorderEnded_Trigger();

protected Q_SLOTS:

	virtual void slotTrackedIconAnimationFinished();

	//TODO:  TEMP: the FSM should actually count the number of in-flight trackings and switch states accordingly
	//		Qt's guarded (conditional) transitions are kind of weird so i'm holding off on implementing this for now
	//		this function will check the number of in-flights and emit the signalFSMLastTrackEndedTrigger as appropriate
	//			I'm weaving it this way with a signal->slot inside this (same) class - usually a bad idea - to minimize the
	//			changes needed when this gets done correctly
	virtual void slotTrackForIconEnded();

	virtual void dbg_reorderFSMStateEntered();

protected:			///// icon layouts

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

	quint32 m_reorderEventSampleRate;	//determines when to check a moving icon against the layout to see what
										// if anything, needs to be reordered. For now, it will count instances
										// of "move" events (e.g. mouseMove) but I'll add other ways to threshold
										// This is useful because checking boundaries on icons and rows in very large
										// layouts could be expensive, as is generating operation lists and animations

	static const char *	TrackedAnimationPropertyName_iconUid;
	QMap<QUuid,QPointer<QAbstractAnimation> > m_trackedIconAnimations;	//key = the icon being animated, which is the COPY in this case
	QMap<QUuid,QPointer<IconBase> >	m_trackedIcons;			//key = uid of the COPY of the icon on the layout that was "picked up"
												// value = the COPY that's being moved around
	//The map of the icon back to the ORIGINAL icon on the layout is not needed, because the icon heap holds all
	//this info already - IconHeap::getIconEx() will get this info in the attributes.  Use the copy hint for slightly faster access
	// TODO: MAJOR-MOD-WARNING: if the icon heap is no longer holding ALL of the icon refs for the system and pages and layouts are allowed
	// 		to have their own icon copies, then this has to change


	static const char * ReorderFSMPropertyName_isConsistent;

	QStateMachine * m_p_reorderFSM;
	QState * m_p_fsmStateConsistent;
	QState * m_p_fsmStateTrackedFloating;
};

QDataStream & operator<< (QDataStream& stream, const IconRowAlpha& s);
QDataStream & operator>> (QDataStream& stream, IconRowAlpha& s);
QDataStream & operator<< (QDataStream& stream, const AlphabetIconLayout& s);
QDataStream & operator>> (QDataStream& stream, AlphabetIconLayout& s);
QDebug operator<<(QDebug dbg, const IconRowAlpha &c);
QDebug operator<<(QDebug dbg, const AlphabetIconLayout &s);

#endif /* ALPHABETICONLAYOUT_H_ */
