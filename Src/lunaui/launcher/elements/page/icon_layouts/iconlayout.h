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




#ifndef ICONLAYOUT_H_
#define ICONLAYOUT_H_

#include <QObject>
#include <QList>
#include <QPointer>
#include <QRectF>
#include <QSizeF>
#include <QDebug>
#include <QTransform>

#include "dimensionsglobal.h"
#include "scrollableobject.h"
#include "layoutitem.h"

class IconBase;
class QPainter;
class IconRow;
class IconLayout;
class Page;
class PixmapObject;
class PixmapHugeObject;

//utility classes, not to be used directly
class IconCell : public LayoutItem
{
	Q_OBJECT
public:
	IconCell(IconRow * p_owner);
	virtual ~IconCell();
	IconCell(IconRow * p_owner,IconBase * p_icon);
	IconCell(IconRow * p_owner,IconBase * p_icon,const QSizeF& size);
	IconCell(IconRow * p_owner,IconBase * p_icon,const QSize& size);

	virtual void clear();

	virtual QRectF geometry();
	virtual QRectF relativeGeometry();
	virtual QRectF untranslateGeometry(const QRectF& geom);	//inverse of relativeGeometry()
	virtual QPointF position();

	//returns the previously set icon, if any.
	// it will call its owner IconRow's cellGeomChangedDueToNewIcon() function if the new icon's geom is incompatible
	//	with the current cell geom (since this will require re-layouts)
	// the bool repositionIconToCell parameter, if true, will call IconBase->setPos() with the
	// cell's position, which will instantly put the icon's position in line with the layout
	// false will leave the icon's current position alone
	virtual IconBase * setNewIcon(IconBase * p_newIcon,bool repositionIconToCell=false);

	//this one will set the associated icon's position to the cell's current position, which moves
	// it back in line with the layout instantly.
	virtual void resetIconPosition();

	//called as a part of reorder commit (the final stage of reorder) to swap the pending icon into the actual
	//icon (m_qp_icon) variable herein. It also disables the icon's auto-paint and locks its position to the cell
	// TAKE CARE THAT THE ICON THAT WAS PREVIOUSLY IN THIS CELL (OCCUPIED m_qp_icon) IS REFERENCED ELSEWHERE,
	// OR ELSE IT'S GOING TO BE LOST AFTER THIS FN.
	virtual void commitPendingIcon();

	//returns the old point/coord for convenience
	QPointF reposition(const QPointF& newCenterPointICS);
	qreal repositionX(const qreal x);
	qreal repositionY(const qreal y);

	void moveBy(const QPointF& d);
	void moveBy(const qreal dx,const qreal dy);

	virtual void paint(QPainter * painter);
	//	"clipping", but by only drawing from the source area specified. sourceRect guaranteed to be within
	//		m_geom space
	//		see IconLayout, IconRow functions of the same signature
	virtual void paint(QPainter * painter, const QRectF& sourceRect);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt);

	virtual void paintOffscreen(QPainter * painter);
	virtual void paintOffscreen(PixmapObject * p_hugePmo);
	virtual void paintOffscreen(PixmapHugeObject * p_hugePmo);

	QPointer<IconBase> m_qp_icon;		//DOES NOT OWN the icon

	//this cell is the target of a reorder...the icon is referenced here while it is "in-flight"
	// during the animation (if any). When a commit is run by the layout, this will be swapped into the
	// main icon
	QPointer<IconBase> m_qp_pendingReorderIcon;

	QRectF  m_geom;				//ICS
	QPointF m_pos;				//absolute ICS of *layout* (NOT Page!)

	QRect m_lastPaintedPixmapTargetRect;		//records where the paintOffscreen functions last painted into a pixmap. This is to facilitate
													// easy(er) retrieval of icons based on UI events (e.g. a mouse press on a pre-rendered pixmap)

	IconRow * m_p_layoutRowObject;		//ptr back to the owning layout row

	friend QDataStream & operator<< (QDataStream& stream, const IconCell& s);
	friend QDataStream & operator>> (QDataStream& stream, IconCell& s);
	friend QDebug operator<<(QDebug dbg, const IconCell &s);

Q_SIGNALS:

	void signalChangedIcon(const IconBase * p_icon);
};

class IconRow : public LayoutItem
{
	Q_OBJECT
public:
	IconRow(IconLayout * p_owner);
	IconRow(IconLayout * p_owner,const QRectF& geom);
	virtual ~IconRow();

	QRectF	m_geom;							//real geometry - defined as 0,0 centered rect from icon[0].topleft to icon[last].bottomright
	QPointF m_pos;

	QRect m_lastPaintedPixmapTargetRect;		//records where the paintOffscreen functions last painted into a pixmap. This is to facilitate
												// easy(er) retrieval of icons based on UI events (e.g. a mouse press on a pre-rendered pixmap)

	typedef QList<IconCell *> IconCellList;
	typedef IconCellList::const_iterator IconCellListConstIter;
	typedef IconCellList::iterator IconCellListIter;

	IconCellList m_iconList;						//DOES NOT OWN the icons
	IconLayout * m_p_layoutObject;	//ptr back to owner IconLayout

	quint32 m_spaceToUpperAdjacentRow;					//this is for bookkeeping. setting these doesn't auto-update positions to keep them valid
	quint32 m_spaceToLowerAdjacentRow;					// "

	quint32 numIcons() const;

	virtual QRectF geometry();
	virtual QRectF relativeGeometry() const;
	virtual void recomputeGeometry(bool adjustPosition=false);
	virtual void repositionAboveRow(IconRow& refRow,const quint32 space);
	virtual void repositionBelowRow(IconRow& refRow,const quint32 space);
	virtual void repositionAboveRow(IconRow& refRow);
	virtual void repositionBelowRow(IconRow& refRow);
	virtual void moveBy(const QPointF& d);
	virtual void moveBy(const qreal dx,const qreal dy);
	virtual void appendCell(IconCell * p_cell);
	virtual void addCell(IconCell * p_cell,const quint32 index);	//index > list size = append
	virtual void appendCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 alignToCellIndex);
	virtual void addCellAndAlignVerticallyToCell(IconCell * p_cell,const quint32 index,const quint32 alignToCellIndex);
	virtual void removeCell(const quint32 index);	//index > list size = remove tail

	virtual void paint(QPainter * painter);

	//	"clipping", but by only drawing from the source area specified. sourceRect guaranteed to be within
	//		m_geom space
	//		see IconLayout function of the same signature
	virtual void paint(QPainter * painter, const QRectF& sourceRect);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt);

	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt);

	virtual void paintOffscreen(QPainter * painter);
	virtual void paintOffscreen(PixmapObject * p_pmo);
	virtual void paintOffscreen(PixmapHugeObject * p_hugePmo);

	virtual void relayout(bool force=false);
	virtual void relayout(const qreal leftEdgeX,const quint32 spacing,bool force);

	// redistributeIconsHorizontally(): will only touch horizontal positions of the icons, not the vertical.
	//takes params to make it a little more generic, unlike relayout() which accesses owner layout vars directly
	virtual void redistributeIconsHorizontally(const qreal leftEdgeX,const quint32 spacing);
	virtual void alignIconsVerticallyCentered(const qreal yPosition);

	virtual void cellGeomChangedDueToNewIcon(IconCell * reportingCell);

	/////NEED A VERSION THAT REMOVES BY CELL PARAM...-> I NEED op== on IconCell.class
	friend QDataStream & operator<< (QDataStream& stream, const IconRow& s);
	friend QDataStream & operator>> (QDataStream& stream, IconRow& s);
	friend QDebug operator<<(QDebug dbg, const IconRow &s);
};

//TODO: can remove m_valid and use the Name INVALID...
class IconOperation
{
public:

	enum Name
	{
		INVALID = 0,
		Id,					//special type used for synchronizing possibly interleaved op lists
		Add,
		Remove,
		Reorder
	};

	static QString Names[];

	IconOperation();
	IconOperation(const Name op,const quint32 origin_column,const quint32 origin_row,
							const quint32 destination_column,const quint32 destination_row);
	IconOperation(const Name op,const quint32 origin_column,const quint32 origin_row,
								const quint32 destination_column,const quint32 destination_row,
								const quint32 seq);

	bool	m_valid;
	Name	m_opname;
	quint32 m_originRow;
	quint32 m_originColumn;
	quint32 m_destinationRow;
	quint32 m_destinationColumn;
	quint64 m_sequenceNum;			//optional, for reordering that occurs sequentially. 0 by default, > 0 if used
									// also used for the id number when opname == Id
	friend QDataStream & operator<< (QDataStream& stream, const IconOperation& s);
	friend QDataStream & operator>> (QDataStream& stream, IconOperation& s);
	friend QDebug operator<<(QDebug dbg, const IconOperation &s);

	static quint64 s_idcounter;		//for id assignments

};
typedef QList<IconOperation> IconOpList;
typedef IconOpList::const_iterator IconOpListConstIter;
typedef IconOpList::iterator IconOpListIter;

class IconLayout : public LayoutItem
{
	Q_OBJECT
public:

	friend class Page;

	IconLayout(Page * p_owner);
	virtual ~IconLayout();

	/*
	 *
	 * This base class assumes a simple model of a fixed rectangular icon size (all icons are the same size)
	 * and fixed spacing between icons
	 *
	 */

	virtual void paint(QPainter * painter);
	virtual void paint(const QPointF& translate,QPainter * painter);

	//	(ScrollableObject-->ScrollingLayoutRenderer-->here)
	//	"clipping", but by only drawing from the source area specified. sourceRect guaranteed to be within
	//		m_geom space
	virtual void paint(QPainter * painter, const QRectF& sourceRect);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,qint32 renderOpt);

	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate);
	virtual void paint(QPainter * painter, const QRectF& sourceRect,const QPointF& painterTranslate,qint32 renderOpt);

	virtual void paintOffscreen(QPainter * painter);
	virtual void paintOffscreen(PixmapObject * p_pmo);
	virtual void paintOffscreen(PixmapHugeObject * p_hugePmo);

	virtual void enableAutoPaint();
	virtual void disableAutoPaint();

	virtual QRectF geometry();
	virtual QRectF relativeGeometry();

	//relayout() sets absolute positioning within the item (Page or a Page-subclass) of layout, rows, and cells
	virtual void relayout(bool force=false);
	virtual void resizeWidth(const quint32 w);
	virtual void setPosition(const QPointF& pos);
	virtual QPointF position();
	virtual void setUniformCellSize(const QSize& size);

	// intended to tell a cell that it is losing control of an icon (usually for moving/animating it to a new cell)
	//TODO: make this more generic for layouts that may not use a system that can describe coordinates as x,y
	virtual void iconCellReleaseIcon(const QPoint& cellCoordinate);

	virtual IconCell * iconCellAtLayoutCoordinate(const QPointF& coordinate);
	virtual IconCell * iconCellAtLayoutCoordinate(const QPointF& layoutCoordinate,QPoint& r_gridCoordinate);

	//returns the rect area of the row specified, in layout CS (it's the geom of the row, translated to its layout CS position)
	virtual QRectF rowArea(quint32 rowIndex) const;
	virtual qint32 rowAtLayoutCoordinate(const QPointF& layoutCoordinate,bool clipMinMax=false);

	//uses the owner page as a reference CS, and maps the given point to the scene...equivalent to calling mapToScene() from inside the page, but
	// this is a convenience pass-through so cells and rows and things that have references to the layout can use mapToScene from page's context.
	// if there is no owner page, then the point passed in is returned unmodified
	virtual QPointF sceneCoordinateFromLayoutCoordinate(const QPointF& layoutCoordinate);
	// and its helper (see Page::pageCoordinateFromLayoutCoordinate)
	virtual QPointF pageCoordinateFromLayoutCoordinate(const QPointF& layoutCoordinate);

	virtual QPointF rawPageCoordinateFromLayoutCoordinate(const QPointF& layoutCoordinate) const;
	virtual QPointF rawLayoutCoordinateFromPageCoordinate(const QPointF& pageCoordinate) const;
	virtual QRectF  rawPageRectFromLayoutRect(const QRectF& layoutRect) const;
	virtual QRectF	rawLayoutRectFromPageRect(const QRectF& pageRect) const;

	virtual void	recomputePageLayoutTransforms();

	virtual qreal	verticalDistanceToNearestUpperRow(const QPointF& layoutCoordinate);
	virtual qreal	verticalDistanceToNearestLowerRow(const QPointF& layoutCoordinate);

	virtual quint32 maximumRowHeight() const;
	virtual quint32 minimumRowHeight() const;

	// all the icon cells, starting with the leftmost cell of the top row, and ending with the rightmost cell in the bottom row
	//WARNING: DO NOT HOLD REF TO ANYTHING FROM THE RETURN OF THIS FN. It could be invalidated at any time (whenever an add/remove or relayout happens)
	virtual QList<IconCell *> iconCellsInFlowOrder();

	friend QDataStream & operator<< (QDataStream& stream, const IconLayout& s);
	friend QDataStream & operator>> (QDataStream& stream, IconLayout& s);
	friend QDebug operator<<(QDebug dbg, const IconLayout &s);

protected:

//	static IconRow& removeIconFromIconRow(IconRow& ref_iconRow,const quint32 index);	//in place remove and shift of the icon in the row

	QPointer<Page> m_qp_ownerPage;
	QRectF m_geom;
	QPointF m_pos;		//pos is in term of Page ICS
	QTransform	m_layoutToPageTran;
	QTransform 	m_pageToLayoutTran;
};

QDataStream & operator<< (QDataStream& stream, const IconCell& s);
QDataStream & operator>> (QDataStream& stream, IconCell& s);
QDataStream & operator<< (QDataStream& stream, const IconRow& s);
QDataStream & operator>> (QDataStream& stream, IconRow& s);
QDataStream & operator<< (QDataStream& stream, const IconOperation& s);
QDataStream & operator>> (QDataStream& stream, IconOperation& s);
QDataStream & operator<< (QDataStream& stream, const IconLayout& s);
QDataStream & operator>> (QDataStream& stream, IconLayout& s);

QDebug operator<<(QDebug dbg, const IconCell &s);
QDebug operator<<(QDebug dbg, const IconRow &s);
QDebug operator<<(QDebug dbg, const IconOperation &s);
QDebug operator<<(QDebug dbg, const IconLayout &s);

#endif /* ICONLAYOUT_H_ */
