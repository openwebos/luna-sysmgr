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




#ifndef ICON_H_
#define ICON_H_

#include <QUuid>
#include <QRectF>
#include <QRect>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QList>
#include <QFont>
#include <QTextLayout>
#include <QStateMachine>

#include "thingpaintable.h"
#include "pixmapobject.h"
#include "icondecorator.h"
#include "renderopts.h"

class DimensionsUI;
class LauncherObject;
class QGraphicsSceneMouseEvent;
class QGesture;
class QGestureEvent;
class QTouchEvent;
class QPanGesture;
class QSwipeGesture;
class QPinchGesture;
class QTapAndHoldGesture;
class QTapGesture;
class FlickGesture;
class QState;
class QStateMachine;
class Page;
class LayoutItem;
class IconBase;

typedef QList<IconBase *> IconList;
typedef QList<IconBase *>::const_iterator IconListConstIter;
typedef QList<IconBase *>::iterator IconListIter;

namespace DecoratorDesignation
{
	enum Enum
	{
		INVALID,
		RemoveIcon,
		DeleteIcon,
		UpdatePillBox
	};
}

namespace RemoveDeleteDecoratorSelector
{
	enum Enum
	{
		INVALID,
		None,
		Remove,
		Delete,
	};
}

namespace RemoveDeleteDecoratorState
{
	enum Enum
	{
		INVALID,
		Normal,
		Activated,
	};
}

namespace IconActionRequest
{
	enum Enum
	{
		INVALID = 0,
		RequestRemoveDelete,	//the Remove/Delete area of the icon was triggered; icon won't take action itself on this - it's a signaling to an upper level to take action
		LAST_INVALID				//(used because of blind casts from int...see usages of this Enum in the rest of the code)
	};
}

namespace IconInternalHitAreas
{
	enum Enum
	{
		INVALID = 0,
		RemoveDeleteDecorator,
		Other				//TODO: extend to describe other parts of the icon
	};
}

class IconBase : public ThingPaintable
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	Q_PROPERTY(QString iconlabel READ iconLabel WRITE setIconLabel RESET resetIconLabel NOTIFY signalIconLabelChanged)
	Q_PROPERTY(LayoutItem * layoutItemAssociation READ layoutItemAssociation WRITE setLayoutItemAssociation)
	Q_PROPERTY(bool stateRemoveDeleteActive READ stateRemoveDeleteActive WRITE setStateRemoveDeleteActive NOTIFY signalStateRemoveDeleteActiveChanged)

public:

	friend class Page;
	friend class IconCell;

	static const char * IconLabelPropertyName;		//keep in sync with Q_PROPERTY for the icon label
	static const char * IconLastPageVisitedIndexPropertyName;
	static const char * IconTransferContextPropertyName;   //TODO: this should be changed into a better way to communicate context for offer/take/taken sequences involving icons
															// (but the Demo must go on...so this is what it is for now)
	static IconBase * iconFromPix(PixmapObject * p_framePix, PixmapObject * p_mainPix, PixmapObject * p_feedbackPix,Page * p_belongsTo);

	IconBase(const QRectF& iconGeometry,Page * p_belongsTo);
	IconBase(const QRectF& iconGeometry,PixmapObject * p_framePix, PixmapObject * p_mainPix,PixmapObject * p_feedbackPix,Page * p_belongsTo);

	virtual IconBase * clone();

	//returns the master icon of this icon, or 0 if this is the master icon
	virtual IconBase * cloneOf() const;
	//convenience of the above; returns the master icon, or this if it is the master (never 0)
	virtual IconBase * master();

	virtual ~IconBase();

	virtual bool stateRemoveDeleteActive() const;
	virtual void setStateRemoveDeleteActive(bool v);

	virtual Page * owningPage() const;
	virtual LauncherObject * owningLauncher() const;		//this is only valid if the icon is in the launcher's Limbo

	QString iconLabel() const;
	void setIconLabel(const QString& v);
	void resetIconLabel();
	void setIconLabelVisibility(bool visible);
	void setUsePrerenderedLabel(bool usePreRendered);
	void setLaunchFeedbackVisibility(bool visible);


	//if null (0) passed in, then the primary launcher instance will be used
	// returns true if complely successful, false if any failures occur in connecting
	// (in case multiple connections; any fail will result in a false here)
	virtual bool connectRequestsToLauncher(LauncherObject * launcherObj = 0);

	virtual LayoutItem * layoutItemAssociation() const;
	virtual void setLayoutItemAssociation(LayoutItem * v);

	// override the geometry logic from the base class...it must be done in order to make use of "alignment" geometry, if it is used
	//	see the icongeometrysettings.h/cpp files
	// essentially, it's a way to trick the users of this class into thinking the geometry is actually different (intended only to be < than the actual geom)
	virtual QRectF geometry() const;
	virtual QRectF positionRelativeGeometry() const;
	virtual QPointF		activePos() const;	//see m_activePositionOffsetFromPos

	virtual bool resize(quint32 newWidth,quint32 newHeight);
	virtual bool resize(const QSize& s);
	virtual bool expand(const quint32 widthEx,const quint32 heightEx);

	virtual bool offer(Thing * p_offer,Thing * p_offeringThing);
	virtual bool take(Thing * p_takerThing);
	virtual bool taking(Thing * p_victimThing, Thing * p_takerThing);
	virtual void taken(Thing * p_takenThing,Thing * p_takerThing);

	static QSize maxIconSize(const IconList& icons);
	static QSize minIconSize(const IconList& icons);
	static void iconSizeBounds(const IconList& icons,QSize& r_minSize,QSize& r_maxSize);

	static bool canUseFramePixOnIcon(const IconBase& icon,PixmapObject * p_pix);
	static bool canUseMainPixOnIcon(const IconBase& icon,PixmapObject * p_pix);
	static bool canUseDecorPixOnIcon(const IconBase& icon,PixmapObject * p_pix);

	virtual bool usingInstallDecorator() const;

	//TODO: temporary; remove when a better system (like styles) goes in place
	static QFont staticLabelFontForIcons();

	static bool iconLessThanByLabel(const IconBase * p_a,const IconBase * p_b);

	/// this one is called into during reorder mode, since the layout/page handles all input, and not the icon itself
	/// returns true if the touch was internally meaningfull and the caller can safely
	// ignore it. This is currently used in the case where a tap-n-hold took the icons into reorder mode
	// and a tap or tap-n-hold on a remove/delete decorator happens. in this case, the layout shouldn't proceed
	// to move the icon as it would ordinarily, so this function would return true, and instead
	// signal the layout/page a different way that a remove/delete was triggered
	//   (see signalIconActionRequest)
	// note that unlike the touch input functions, this is like a "tap", in that it signifies
	// a complete interaction, not a start/end  (for that usage, see touchStartIntoIcon and touchEndIntoIcon)
	// a false return means that the caller can potentially take further action; the area that was hit will be in r_hitArea
	virtual bool tapIntoIcon(const QPointF& touchCoordinateICS,IconInternalHitAreas::Enum& r_hitArea);
	virtual bool touchStartIntoIcon(const QPointF& touchCoordinateICS);
	virtual bool touchEndIntoIcon(const QPointF& touchCoordinateICS);

	//TODO: UPDATE-PAINT-WORKAROUND:
	virtual void update();
Q_SIGNALS:

	// params:
	//[0] old state
	//[1] new state
	void signalIconLabelChanged(const QString&,const QString&);
	void signalIconLabelChanged();

	void signalMasterIconMainImageChanged(PixmapObject * p_newMainIconPixmap);
	void signalMasterIconInstallStatusDecoratorChanged(PixmapObject * p_newPixmap);
	void signalMasterIconInstallStatusDecoratorParamsChanged(int progressVal,int minProgressVal,int maxProgressVal);

	void signalStateRemoveDeleteActiveChanged();
	void signalFSMActivate();
	void signalFSMDeactivate();


	////WARNING!!!: Only connect these below with QueuedConnection as the connection type argument to connect() !!!!
Q_SIGNALS:

	// 'cmdRequest' is actually a IconActionRequest::Enum but enums are slightly annoying and awkward w.r.t. Qt's metatype system
	//    and a queued connect requires the params to be reg'd metatypes
	void signalIconActionRequest(int cmdRequest);

public Q_SLOTS:

	//WARNING: this way of passing ptr ref for a param.-based-"return" could cause problems if the signal came
	//		across a thread boundary (see Qt doc)
	virtual void slotUpdateIconPic(PixmapObject * p_newPixmap,bool propagate,PixmapObject*& r_p_oldPixmap);
	virtual void slotChangeIconVisibility(bool show);

	virtual void slotUpdateIconFramePic(PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap);
	virtual void slotChangeIconFrameVisibility(bool show);

	virtual void slotUpdateRemoveDecoratorPic(RemoveDeleteDecoratorState::Enum whichStatePic,PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap);
	virtual void slotUpdateDeleteDecoratorPic(RemoveDeleteDecoratorState::Enum whichStatePic,PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap);

	// also resets the state to Normal
	virtual void slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::Enum showWhich);

	virtual void slotUpdateInstallStatusDecoratorPic(PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap);
	virtual void slotUpdateInstallStatusDecoratorPicNewProgress(int progressVal,bool updateMinMax=false,int minProgressVal = 0,int maxProgressVal = 0);
	virtual void slotUpdateInstallStatusDecoratorResetProgress(int minProgressVal = 0,int maxProgressVal = 100);

	// These two toggle this item's painting and repainting of itself as a part of the QGraphics View system
	// when AutoRepaint is disabled, the item will only be painted when its paint function is called explicitly
	virtual void slotEnableIconAutoRepaint();
	virtual void slotDisableIconAutoRepaint(bool clearAutoclip=true);

	// this is a type of clip-rect similar to what QPainter achieves with setClipRect()
	// however, it is computed manually when the icon is auto repainting; that is, when it isn't painted as a part
	// of the layout. It will basically just redirect a paint() call as done by the QGView/Scene into
	// a paint(QPainter *painter, const QRectF& sourceItemRect) as done by the manual layout paint,
	// using the clipRectPCS with the current pos() of the icon to create the sourceItemRect
	virtual void setAutopaintClipRect(const QRect& clipRectPCS);
	virtual void clearAutopaintClipRect();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paint(QPainter *painter, const QRectF& sourceItemRect);
	//this one is for "staged rendering"
	virtual void paint(QPainter *painter, const QRectF& sourceRect,qint32 renderOpt);

	virtual void paintOffscreen(QPainter *painter);
	virtual void paintOffscreen(QPainter *painter,const QRect& sourceRect,const QPoint& targetOrigin);
	virtual void paintOffscreen(QPainter *painter,const QRect& sourceRect,const QRect& targetRect);

protected Q_SLOTS:

	// a special case of slotUpdateIconPic, intended to only be signaled by the master icon to its copies
	virtual void slotMasterIconMainImageChanged(PixmapObject * p_newMainIconPixmap);
	virtual void slotMasterIconInstallStatusDecoratorChanged(PixmapObject * p_newPixmap);
	virtual void slotMasterIconInstallStatusDecoratorParamsChanged(int progressVal,int minProgressVal,int maxProgressVal);

	virtual void slotAnimationFinished();

	//used as the completion step of the FSM
	virtual void slotRemoveDeleteActivated();

protected:

	IconBase(const QRectF& iconGeometry);		//only used for cloning!
	//TODO: make it a proper virt.ctor idiom by completing clone() in ThingPaintable and Thing

	virtual bool touchEvent(QTouchEvent * event) { return true; }
	virtual bool touchStartEvent(QTouchEvent *event) { return true; }
	virtual bool touchUpdateEvent(QTouchEvent *event) { return true; }
	virtual bool touchEndEvent(QTouchEvent *event) { return true; }

	virtual bool sceneEvent(QEvent * event);
	virtual bool gestureEvent(QGestureEvent *gestureEvent) { return true; }
	virtual bool panGesture(QPanGesture *panEvent) { return true; }
	virtual bool swipeGesture(QSwipeGesture *swipeEvent);
	virtual bool flickGesture(FlickGesture *flickEvent);
	virtual bool pinchGesture(QPinchGesture *pinchEvent) { return true; }
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent) { return true; }
	virtual bool tapGesture(QTapGesture *tapEvent) { return true; }
	virtual bool customGesture(QGesture *customGesture) { return true; }

	virtual void setupFSM();

	static QRectF GEOM(const QRectF& geom);
	static QRectF AGEOM(const QRectF& geom);	//alignment geom, if used
	static QRectF LabelBoxGeomFromGeom(const QRectF& geom);
	static QRectF FrameGeomFromGeom(const QRectF& geom);
	static QRectF IconGeomFromGeom(const QRectF& geom);
	static QRectF RemoveDeleteDecorGeomFromGeom(const QRectF& geom);
	static QRectF InstallStatusGeomFromGeom(const QRectF& geom);

//	virtual void updateGeometry(const QRectF& mainIconGeom,const QRectF& frameIconGeom);
//	virtual void updateGeometry(const QRectF& newGeom,bool doesGeomIncludeLabel=true);

	virtual void geometryChanged();
	virtual void recomputePainterHelpers();
	virtual void recalculateDecoratorPositionOnFrameChange();

//	// recalculateLabelBoundsForCurrentGeom() works in conjunction with updateGeometry...
//	// two possibilities when a new geometry is specified (corresponds to the two variants of updateGeometry()):
//	// 1. updateGeometry(icon geom,frame icon geom) -- only the icon picture geoms were specified, and the label
//	//		box needs to be calculated from this and added to the given geoms to make m_geom (the final, total icon geom)
//	// 2. updateGeometry(new geom for whole icon) -- t//he geom specified is the total geom which is frame icon pic and/or main
//	// 		icon pic AND the label box, inclusive. The label size must be deduced from this, and nothing is added to the geom
//	//		specified (it's already the total geom)
//	//		(can be overriden with the bool parameter, passed through to this variant of updateGeometry)
//	//	PREREQUISITE: m_geom must be valid, at least its size()
//	//  RESULT: m_labelMaxGeom will be set correctly
//	virtual void recalculateLabelBoundsForCurrentGeom(bool doesCurrentGeomIncludeLabel);

	static bool canUsePixOnIcon(const IconBase& icon,PixmapObject * p_pix);

	//	PREREQUISITE: m_labelMaxGeom must be valid, at least its size()
	//  RESULT: m_textLayoutObject will have the correct layout, m_labelGeom will be set correctly for the current label,
	virtual void	redoLabelTextLayout(bool renderLabelPixmap = false);		//run this after the geometry changes. This is absolutely necessary
												// ALSO run this after the label changes. don't ask, just do it. Not necessary now, but it most likely will be later

	//	PREREQUISITE: m_geom , m_labelMaxGeom and m_labelGeom are set
	//	RESULT:	m_labelPosICS and m_labelPosPntCS are set
	virtual void	recalculateLabelPosition();

protected:

	bool	m_showLabel;
	QString m_iconLabel;
//	QPointer<Page> m_qp_currentOwnerPage;
	QPointer<IconBase> m_qp_masterIcon;			//ptr to the master icon if this one is a clone, or 0 if this is a master
	bool	m_useOwnerSetAutopaintClip;
	QRect	m_ownerSetAutopaintClip;			//something like a clip-rect for QPainter, but manually done. see setAutopaintClipRect()
	QPointer<LayoutItem> m_qp_layoutItemAssociation;

	///  m_activePositionOffsetFromPos:  The concept of "active position" is that the icon's QGraphicsItem pos() is the place where it is drawn, but that pos may
	//		actually be a position above or below e.g. a finger touch hotspot, so that the icon isn't occluded by the finger.
	//		In some cases, I may want the actual position of the item, as far as the rest of the system is concerned, to be different than this visible location
	//		Therefore, this variable is used to keep track of that. It is initialized from settings, but may be overriden at runtime, even animated (bouncy icon effect)
	QPointF					m_activePositionOffsetFromPos;

	QRectF					m_alignmentGeomPrecomputed;		//populated only if IconGeometrySettings::useAlignmentGeom is set true

	bool					m_showIcon;
	QPointer<PixmapObject>  m_qp_iconPixmap;
	QRect					m_iconGeom;				//this is the actual geom for the layout calculation
	QRect					m_iconPixmapGeom;		//this one is just for the painter, to deal with overpainting. it will be == or > m_iconGeom
	QPoint					m_iconPosICS;		//this relates to m_iconGeom and m_iconPixmapGeom; both have the origin in the same place (at this point)
	QPoint					m_iconSrcPrecomputed;		//a precomputed translator to speed up paint (see paint(QPainter *painter, const QRectF& sourceItemRect))

	bool					m_showFrame;
	QPointer<PixmapObject>  m_qp_iconFramePixmap;		//the "background"
	QRect					m_iconFrameGeom;			// this is the actual geom for the layout calculation
	QRect					m_iconFramePixmapGeom;		//this one is just for the painter, to deal with overpainting. it will be == or > m_iconFrameGeom
	QPoint					m_iconFramePosICS;			//this relates to m_iconFrameGeom and m_iconFramePixmapGeom; both have the origin in the same place (at this point)
	QPoint					m_iconFrameSrcPrecomputed;	//a precomputed translator to speed up paint (see paint(QPainter *painter, const QRectF& sourceItemRect))

	bool					m_showFeedback;
	QPointer<PixmapObject>  m_qp_iconFeedbackPixmap;		//the launch feedback image
	QRect					m_iconFeedbackPixmapGeom;		//this one is just for the painter, to deal with overpainting. it will be == or > m_iconFrameGeom
	QPoint					m_iconFeedbackPosICS;			//this relates to m_iconFeedbackGeom and m_iconFeedbackPixmapGeom; both have the origin in the same place (at this point)
	QPoint					m_iconFeedbackSrcPrecomputed;	//a precomputed translator to speed up paint (see paint(QPainter *painter, const QRectF& sourceItemRect))

	RemoveDeleteDecoratorSelector::Enum m_showWhichDeleteRemove;
	QPointer<PixmapObject>	m_qp_removeDecoratorPixmap;				//these pairs better have the same geoms
	QPointer<PixmapObject>	m_qp_removePressedDecoratorPixmap;
	QPointer<PixmapObject>	m_qp_removeDecoratorCurrentStatePixmap;
	QPointer<PixmapObject>	m_qp_deleteDecoratorPixmap;				// ""
	QPointer<PixmapObject>	m_qp_deletePressedDecoratorPixmap;
	QPointer<PixmapObject>	m_qp_deleteDecoratorCurrentStatePixmap;

	QPointer<PixmapObject>  m_qp_drDecoratorCurrentlyRenderingPixmap;		//set outside paint() so that paint can avoid checking "which"
	QRect					m_drDecoratorCurrentPixmapGeom;
	QPoint					m_drDecoratorCurrentPosICS;
	QPoint					m_drDecoratorCurrentSrcPrecomputed;

	QRect					m_removeDecoratorGeom;			// these are analogues to how icon and iconFrame do things, above
	QRect					m_removeDecoratorPixmapGeom;
	QPoint					m_removeDecoratorPosICS;
	QPoint					m_removeDecoratorSrcPrecomputed;
	QRect					m_deleteDecoratorGeom;			// these are analogues to how icon and iconFrame do things, above
	QRect					m_deleteDecoratorPixmapGeom;
	QPoint					m_deleteDecoratorPosICS;
	QPoint					m_deleteDecoratorSrcPrecomputed;

	QPointer<PixmapObject>	m_qp_installStatusDecoratorPixmap;
	QRect					m_installStatusDecoratorGeom;
	QRect					m_installStatusDecoratorPixmapGeom;
	QPoint					m_installStatusDecoratorPosICS;
	QPoint					m_installStatusDecoratorSrcPrecomputed;
	int						m_lastProgressValue;
	int						m_minProgressValue;
	int						m_maxProgressValue;

	QStateMachine	* m_p_buttonFSM;
	QState * m_p_stateNormal;
	QState * m_p_stateActive;

	bool	m_canRequestLauncher;

	//text layout for the label
	static QFont 			s_iconLabelFont;
	QColor					m_labelColor;
	QPoint					m_labelPosICS;				// position in ICS, and corresponds to m_labelGeom
	QRect					m_labelMaxGeom;			// precomputed by recalculateLabelBoundsForCurrentGeom() , based on current icon size and settings
													//	(proportions of icon pic to label box)
	QPoint					m_labelSrcPrecomputed;		//a precomputed translator to speed up paint (see paint(QPainter *painter, const QRectF& sourceItemRect))
	QRect					m_labelGeom;			//precomputed by redoLabelTextLayout(); this one is the CURRENT label's box (always <= m_labelMaxGeom)
	QTextLayout				m_textLayoutObject;
	QPointer<PixmapObject>	m_qp_prerenderedLabelPixmap;
	bool 					m_usePrerenderedLabel;

};

#endif /* ICON_H_ */
