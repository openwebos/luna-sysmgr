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




#include "icon.h"
#include "iconcmdevents.h"
#include "layoutitem.h"
#include "page.h"
#include "dimensionsmain.h"
#include "dimensionslauncher.h"
#include "dimensionsglobal.h"
#include "pixmapobject.h"
#include "pixmapfilmstripobject.h"
#include "gfxsettings.h"
#include "iconlayoutsettings.h"
#include "icongeometrysettings.h"
#include "dynamicssettings.h"
#include "timedelaytransition.h"
#include "debugglobal.h"
#include "testiconfactory.h"

#include <QCoreApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QPainter>
#include <QString>
#include <QDebug>

#include "QEvent"
#include <QGesture>
#include <QGestureEvent>
#include <QFontMetrics>

#if defined(TARGET_DEVICE)
#include <FlickGesture.h>
#include <SysMgrDefs.h>
#else
#include <FlickGesture.h>
#endif

#include "Settings.h"	//for the font spec

//#define PAINT_GEOM_BOXES	1

//--------public:

const char * IconBase::IconLabelPropertyName = "iconlabel";
const char * IconBase::IconLastPageVisitedIndexPropertyName = "lastpageindex";
const char * IconBase::IconTransferContextPropertyName = "transfercontext";

QFont IconBase::s_iconLabelFont = QFont();

QFont IconBase::staticLabelFontForIcons()
{
	//TODO: specify a proper font
	static bool fontInitalized = false;
	if (!fontInitalized)
	{
		s_iconLabelFont = QFont(QString::fromStdString(Settings::LunaSettings()->fontQuicklaunch));
		quint32 fontSize = qBound((quint32)2,IconGeometrySettings::settings()->labelFontSizePx,(quint32)100);
		s_iconLabelFont.setPixelSize(fontSize);
		s_iconLabelFont.setBold(IconGeometrySettings::settings()->labelFontEmbolden);
	}
	return s_iconLabelFont;
}

//static
QRectF IconBase::GEOM(const QRectF& geom)
{
	if (!(IconGeometrySettings::settings()->useAbsoluteGeom))
	{
		return geom;
	}
	return DimensionsGlobal::realRectAroundRealPoint(
			IconGeometrySettings::settings()->absoluteGeomSizePx);
}

//static
QRectF IconBase::AGEOM(const QRectF& geom)	//alignment geom, if used
{
	if (IconGeometrySettings::settings()->useAlignmentGeom)
	{
		return DimensionsGlobal::realRectAroundRealPoint(
					IconGeometrySettings::settings()->alignmentGeomSizePx);
	}
	if (!(IconGeometrySettings::settings()->useAbsoluteGeom))
	{
		return geom;
	}
	return DimensionsGlobal::realRectAroundRealPoint(
			IconGeometrySettings::settings()->absoluteGeomSizePx);
}

//static
QRectF IconBase::LabelBoxGeomFromGeom(const QRectF& geom)
{
	if (!(IconGeometrySettings::settings()->useAbsoluteLabelBoxGeom))
	{
		quint32 w = geom.width() * IconGeometrySettings::settings()->labelBoxProportionToGeom.width();
		quint32 h = geom.height() * IconGeometrySettings::settings()->labelBoxProportionToGeom.height();
		return DimensionsGlobal::realRectAroundRealPoint(
			QSize(DimensionsGlobal::Even(w),
					DimensionsGlobal::Even(h)
			)
		);
	}
	return DimensionsGlobal::realRectAroundRealPoint(
			IconGeometrySettings::settings()->labelBoxAbsoluteGeomSizePx);
}
//static
QRectF IconBase::FrameGeomFromGeom(const QRectF& geom)
{
	if (!(IconGeometrySettings::settings()->useAbsoluteFrameGeom))
	{
		quint32 w = geom.width() * IconGeometrySettings::settings()->frameBoxProportionToGeom.width();
		quint32 h = geom.height() * IconGeometrySettings::settings()->frameBoxProportionToGeom.height();
		return DimensionsGlobal::realRectAroundRealPoint(
			QSize(DimensionsGlobal::Even(w),
					DimensionsGlobal::Even(h)
			)
		);
	}
	return DimensionsGlobal::realRectAroundRealPoint(
			IconGeometrySettings::settings()->frameBoxAbsoluteGeomSizePx);
}
//static
QRectF IconBase::IconGeomFromGeom(const QRectF& geom)
{
	if (!(IconGeometrySettings::settings()->useAbsoluteMainIconGeom))
	{
		quint32 w = geom.width() * IconGeometrySettings::settings()->mainIconBoxProportionToGeom.width();
		quint32 h = geom.height() * IconGeometrySettings::settings()->mainIconBoxProportionToGeom.height();
		return DimensionsGlobal::realRectAroundRealPoint(
				QSize(DimensionsGlobal::Even(w),
						DimensionsGlobal::Even(h)
				)
		);
	}
	return DimensionsGlobal::realRectAroundRealPoint(
			IconGeometrySettings::settings()->mainIconBoxAbsoluteGeomSizePx);
}

//static
QRectF IconBase::RemoveDeleteDecorGeomFromGeom(const QRectF& geom)
{
	if (!(IconGeometrySettings::settings()->useAbsoluteRemoveDeleteDecoratorGeom))
	{
		quint32 w = geom.width() * IconGeometrySettings::settings()->removeDeleteDecoratorBoxProportionToGeom.width();
		quint32 h = geom.height() * IconGeometrySettings::settings()->removeDeleteDecoratorBoxProportionToGeom.height();
		return DimensionsGlobal::realRectAroundRealPoint(
				QSize(DimensionsGlobal::Even(w),
						DimensionsGlobal::Even(h)
				)
		);
	}
	return DimensionsGlobal::realRectAroundRealPoint(
			IconGeometrySettings::settings()->removeDeleteDecoratorBoxAbsoluteGeomSizePx);
}

//static
QRectF IconBase::InstallStatusGeomFromGeom(const QRectF& geom)
{
	if (!(IconGeometrySettings::settings()->useAbsoluteInstallStatusDecoratorGeom))
	{
		quint32 w = geom.width() * IconGeometrySettings::settings()->installStatusDecoratorBoxProportionToGeom.width();
		quint32 h = geom.height() * IconGeometrySettings::settings()->installStatusDecoratorBoxProportionToGeom.height();
		return DimensionsGlobal::realRectAroundRealPoint(
				QSize(DimensionsGlobal::Even(w),
						DimensionsGlobal::Even(h)
				)
		);
	}
	return DimensionsGlobal::realRectAroundRealPoint(
			IconGeometrySettings::settings()->installStatusDecoratorBoxAbsoluteGeomSizePx);
}

//static
bool IconBase::iconLessThanByLabel(const IconBase * p_a,const IconBase * p_b)
{
	if (!(p_a) || !(p_b))
	{
		return false;
	}
	return (p_a->iconLabel().toLower() < p_b->iconLabel().toLower());
}

//static
IconBase * IconBase::iconFromPix(PixmapObject * p_framePix, PixmapObject * p_mainPix, PixmapObject * p_feedbackPix,Page * p_belongsTo)
{
	if (p_mainPix == 0)
	{
		return 0;   //cannot omit main icon pic
	}
	//determine the geometry to use. If something compatible can't be found, then bail

	//if the icons are to use a fixed, absolute geom, then use that
	QRectF geom = IconBase::GEOM(QRectF());
	if (!geom.isEmpty())
	{
		//yup, it uses an absolute geom.
		return new IconBase(geom,p_framePix,p_mainPix,p_feedbackPix,p_belongsTo);
	}
	//else, attempt to recover it from the inverse proportion of the frame
	if (!p_framePix)
	{
		return 0;	//except if there is no frame...
	}
	QRectF inferredGeom = DimensionsGlobal::realRectAroundRealPoint(
			QSize((1.0/IconGeometrySettings::settings()->frameBoxProportionToGeom.width())*p_framePix->width(),
					(1.0/IconGeometrySettings::settings()->frameBoxProportionToGeom.height())*p_framePix->height())
			);
	return new IconBase(inferredGeom,p_framePix,p_mainPix,p_feedbackPix,p_belongsTo);
}

//only for cloning
IconBase::IconBase(const QRectF& iconGeometry)
: ThingPaintable(iconGeometry)
//, m_qp_currentOwnerPage(0)
, m_qp_masterIcon(0)
, m_useOwnerSetAutopaintClip(false)
, m_qp_layoutItemAssociation(0)
, m_qp_iconPixmap(0)
, m_qp_iconFramePixmap(0)
, m_showWhichDeleteRemove(RemoveDeleteDecoratorSelector::None)
, m_qp_removeDecoratorPixmap(0)
, m_qp_removePressedDecoratorPixmap(0)
, m_qp_removeDecoratorCurrentStatePixmap(0)
, m_qp_deleteDecoratorPixmap(0)
, m_qp_deletePressedDecoratorPixmap(0)
, m_qp_deleteDecoratorCurrentStatePixmap(0)
, m_qp_drDecoratorCurrentlyRenderingPixmap(0)
, m_qp_installStatusDecoratorPixmap(0)
, m_p_buttonFSM(0)
, m_canRequestLauncher(false)
, m_labelColor(Qt::white)
, m_qp_prerenderedLabelPixmap(0)
, m_usePrerenderedLabel(false)
, m_showFeedback(false)
, m_qp_iconFeedbackPixmap(0)
{
	m_alignmentGeomPrecomputed = DimensionsGlobal::realRectAroundRealPoint(IconGeometrySettings::settings()->alignmentGeomSizePx);
	setupFSM();
	m_p_buttonFSM->start();
}

IconBase::IconBase(const QRectF& iconGeometry,Page * p_belongsTo)
: ThingPaintable(iconGeometry)
//, m_qp_currentOwnerPage(p_belongsTo)
, m_qp_masterIcon(0)
, m_useOwnerSetAutopaintClip(false)
, m_qp_layoutItemAssociation(0)
, m_qp_iconPixmap(0)
, m_qp_iconFramePixmap(0)
, m_showWhichDeleteRemove(RemoveDeleteDecoratorSelector::None)
, m_qp_removeDecoratorPixmap(0)
, m_qp_removePressedDecoratorPixmap(0)
, m_qp_removeDecoratorCurrentStatePixmap(0)
, m_qp_deleteDecoratorPixmap(0)
, m_qp_deletePressedDecoratorPixmap(0)
, m_qp_deleteDecoratorCurrentStatePixmap(0)
, m_qp_drDecoratorCurrentlyRenderingPixmap(0)
, m_qp_installStatusDecoratorPixmap(0)
, m_p_buttonFSM(0)
, m_canRequestLauncher(false)
, m_labelColor(Qt::white)
, m_qp_prerenderedLabelPixmap(0)
, m_showLabel(true)
, m_usePrerenderedLabel(false)
, m_showFeedback(false)
, m_qp_iconFeedbackPixmap(0)
{
	m_qp_takerOwner = p_belongsTo;
	m_alignmentGeomPrecomputed = DimensionsGlobal::realRectAroundRealPoint(IconGeometrySettings::settings()->alignmentGeomSizePx);
	//updateGeometry(iconGeometry,false);
	geometryChanged();
	recomputePainterHelpers();

//	if (m_qp_currentOwnerPage)
//		this->setParentItem(m_qp_currentOwnerPage);
	Page * pPage = qobject_cast<Page *>(m_qp_takerOwner);
	if (pPage)
	{
		setParentItem(pPage);
	}
	setVisible(false);
	setupFSM();
	m_p_buttonFSM->start();
}

IconBase::IconBase(const QRectF& iconGeometry,PixmapObject * p_framePix, PixmapObject * p_mainPix,PixmapObject * p_feedbackPix,Page * p_belongsTo)
: ThingPaintable(iconGeometry)
//, m_qp_currentOwnerPage(p_belongsTo)
, m_qp_masterIcon(0)
, m_useOwnerSetAutopaintClip(false)
, m_qp_layoutItemAssociation(0)
, m_qp_iconPixmap(0)
, m_qp_iconFramePixmap(0)
, m_showWhichDeleteRemove(RemoveDeleteDecoratorSelector::None)
, m_qp_removeDecoratorPixmap(0)
, m_qp_removePressedDecoratorPixmap(0)
, m_qp_removeDecoratorCurrentStatePixmap(0)
, m_qp_deleteDecoratorPixmap(0)
, m_qp_deletePressedDecoratorPixmap(0)
, m_qp_deleteDecoratorCurrentStatePixmap(0)
, m_qp_drDecoratorCurrentlyRenderingPixmap(0)
, m_qp_installStatusDecoratorPixmap(0)
, m_p_buttonFSM(0)
, m_canRequestLauncher(false)
, m_labelColor(Qt::white)
, m_qp_prerenderedLabelPixmap(0)
, m_showLabel(true)
, m_usePrerenderedLabel(false)
, m_showFeedback(false)
, m_qp_iconFeedbackPixmap(0)
{

	m_qp_takerOwner = p_belongsTo;
	setupFSM();
	m_p_buttonFSM->start();
	m_alignmentGeomPrecomputed = DimensionsGlobal::realRectAroundRealPoint(IconGeometrySettings::settings()->alignmentGeomSizePx);
	if (p_mainPix == 0)
	{
		return;   //cannot omit main icon pic
	}
	if (p_framePix)
	{
		m_qp_iconFramePixmap = p_framePix;
//		m_iconFramePixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_framePix->sizeF()).toRect();
		//set the pixmap geom...since it could vary from the layout geom
		m_iconFramePixmapGeom = DimensionsGlobal::realRectAroundRealPoint(m_qp_iconFramePixmap->size()).toRect();
	}

	if (p_feedbackPix)
	{
		m_qp_iconFeedbackPixmap = p_feedbackPix;
		//set the pixmap geom...since it could vary from the layout geom
		m_iconFeedbackPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(m_qp_iconFeedbackPixmap->size()).toRect();
	}

	m_qp_iconPixmap = p_mainPix;
//	m_iconPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_mainPix->sizeF()).toRect();
	//set the pixmap geom...since it could vary from the layout geom
	m_iconPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(m_qp_iconPixmap->size()).toRect();

	//updateGeometry(QRectF(m_iconPixmapGeom),QRectF(m_iconFramePixmapGeom));
	geometryChanged();
	recomputePainterHelpers();

//	if (m_qp_currentOwnerPage)
//		this->setParentItem(m_qp_currentOwnerPage);

	Page * pPage = qobject_cast<Page *>(m_qp_takerOwner);
	if (pPage)
	{
		setParentItem(pPage);
	}

	setVisible(false);
}

/*
 * TODO: A PROPER CLONE!!!
 *
 * THE CURRENT VERSION DOES PixmapObject SHARING!
 *
 * for now, this is ok, because pixmaps are never deleted and aren't modified by the individual icons
 * However, in the long run, this is dangerous, since one icon's manipulation of its Pmo will affect all other
 * clones of that icon
 *
 */
//virtual
IconBase * IconBase::clone()
{
	//TODO: consider a Ptr-to-Impl for IconBase, so all these vars could be auto-copied (even shared)
	//			mainly it would make it easier to maintain this class and avoid clone errors when new vars are added
	//			or existing ones deleted/changed
	IconBase * pCloned = new IconBase(m_geom);	//use the cloning ctor

	pCloned->m_iconLabel = m_iconLabel;
	pCloned->m_showLabel = m_showLabel;

	pCloned->m_alignmentGeomPrecomputed = m_alignmentGeomPrecomputed;
	pCloned->m_showIcon = m_showIcon;
	pCloned->m_qp_iconPixmap = m_qp_iconPixmap;
	pCloned->m_iconGeom = m_iconGeom;
	pCloned->m_iconPixmapGeom = m_iconPixmapGeom;
	pCloned->m_iconPosICS = m_iconPosICS;

	pCloned->m_showFrame = m_showFrame;
	pCloned->m_qp_iconFramePixmap = m_qp_iconFramePixmap;		//the "background"
	pCloned->m_iconFrameGeom = m_iconFrameGeom;
	pCloned->m_iconFramePixmapGeom = m_iconFramePixmapGeom;
	pCloned->m_iconFramePosICS = m_iconFramePosICS;

	pCloned->m_showFeedback = m_showFeedback;
	pCloned->m_qp_iconFeedbackPixmap = m_qp_iconFeedbackPixmap;
	pCloned->m_iconFeedbackPixmapGeom = m_iconFeedbackPixmapGeom;
	pCloned->m_iconFeedbackPosICS = m_iconFeedbackPosICS;

	pCloned->m_showWhichDeleteRemove =m_showWhichDeleteRemove;
	pCloned->m_qp_removeDecoratorPixmap =m_qp_removeDecoratorPixmap;
	pCloned->m_qp_removePressedDecoratorPixmap =m_qp_removePressedDecoratorPixmap;
	pCloned->m_qp_removeDecoratorCurrentStatePixmap =m_qp_removeDecoratorCurrentStatePixmap;
	pCloned->m_qp_deleteDecoratorPixmap =m_qp_deleteDecoratorPixmap;
	pCloned->m_qp_deletePressedDecoratorPixmap =m_qp_deletePressedDecoratorPixmap;
	pCloned->m_qp_deleteDecoratorCurrentStatePixmap =m_qp_deleteDecoratorCurrentStatePixmap;
	pCloned->m_qp_drDecoratorCurrentlyRenderingPixmap =m_qp_drDecoratorCurrentlyRenderingPixmap;

	pCloned->m_removeDecoratorGeom =m_removeDecoratorGeom;
	pCloned->m_removeDecoratorPixmapGeom =m_removeDecoratorPixmapGeom;
	pCloned->m_removeDecoratorPosICS =m_removeDecoratorPosICS;
	pCloned->m_removeDecoratorSrcPrecomputed =m_removeDecoratorSrcPrecomputed;
	pCloned->m_deleteDecoratorGeom =m_deleteDecoratorGeom;
	pCloned->m_deleteDecoratorPixmapGeom =m_deleteDecoratorPixmapGeom;
	pCloned->m_deleteDecoratorPosICS =m_deleteDecoratorPosICS;
	pCloned->m_deleteDecoratorSrcPrecomputed =m_deleteDecoratorSrcPrecomputed;

	pCloned->m_qp_installStatusDecoratorPixmap = m_qp_installStatusDecoratorPixmap;
	pCloned->m_installStatusDecoratorGeom = m_installStatusDecoratorGeom;
	pCloned->m_installStatusDecoratorPixmapGeom = m_installStatusDecoratorPixmapGeom;
	pCloned->m_installStatusDecoratorPosICS = m_installStatusDecoratorPosICS;

	pCloned->m_labelColor = m_labelColor;
	pCloned->m_labelPosICS = m_labelPosICS;
	pCloned->m_labelMaxGeom = m_labelMaxGeom;
	pCloned->m_labelGeom = m_labelGeom;

	// clones will by default not generate the prerendered labels, as they are ment for the QL only, where labels are not required
	pCloned->m_usePrerenderedLabel = false;

	if (m_canRequestLauncher)
	{
		//TODO: not quite right since the original might have not used the primary launcher instance,
		// but there is unlikely to be more than 1 launcher instance anytime soon (ever?)
		pCloned->connectRequestsToLauncher();
	}

	pCloned->redoLabelTextLayout();
	pCloned->recomputePainterHelpers();

	//find the master icon - THIS AVOIDS BOTH CYCLES AND CLONE-OF-CLONE PROBLEMS
	IconBase * pMaster = this;
	while (pMaster->m_qp_masterIcon)
	{
		pMaster = pMaster->m_qp_masterIcon;
	}
	pCloned->m_qp_masterIcon = pMaster;
	connect(pMaster,SIGNAL(signalMasterIconMainImageChanged(PixmapObject *)),
			pCloned,SLOT(slotMasterIconMainImageChanged(PixmapObject *)));
	connect(pMaster,SIGNAL(signalMasterIconInstallStatusDecoratorChanged(PixmapObject *)),
			pCloned,SLOT(slotMasterIconInstallStatusDecoratorChanged(PixmapObject *)));
	connect(pMaster,SIGNAL(signalMasterIconInstallStatusDecoratorParamsChanged(int,int,int)),
			pCloned,SLOT(slotMasterIconInstallStatusDecoratorParamsChanged(int,int,int)));

	return pCloned;
}

//virtual
IconBase * IconBase::cloneOf() const
{
	return m_qp_masterIcon;
}

//virtual
IconBase * IconBase::master()
{
	return (m_qp_masterIcon ? m_qp_masterIcon.data() : this);
}

//virtual
IconBase::~IconBase()
{
	if(m_qp_prerenderedLabelPixmap) {
		delete m_qp_prerenderedLabelPixmap;
		m_qp_prerenderedLabelPixmap = 0;
	}

	//I don't own any of the other pixmaps (besides the prerendered label) so DO NOT dealloc them!
}

//virtual
bool IconBase::stateRemoveDeleteActive() const
{
	switch (m_showWhichDeleteRemove)
	{
	case RemoveDeleteDecoratorSelector::Remove:
		return (m_qp_removeDecoratorCurrentStatePixmap == m_qp_removePressedDecoratorPixmap);
		break;
	case RemoveDeleteDecoratorSelector::Delete:
		return (m_qp_deleteDecoratorCurrentStatePixmap == m_qp_deletePressedDecoratorPixmap);
		break;
	default:	//None
		break;
	}
	return false;
}

//virtual
void IconBase::setStateRemoveDeleteActive(bool v)
{
	switch (m_showWhichDeleteRemove)
	{
	case RemoveDeleteDecoratorSelector::Remove:
		m_qp_removeDecoratorCurrentStatePixmap = (v ? m_qp_removePressedDecoratorPixmap : m_qp_removeDecoratorPixmap);
		m_qp_drDecoratorCurrentlyRenderingPixmap = m_qp_removeDecoratorCurrentStatePixmap;
		break;
	case RemoveDeleteDecoratorSelector::Delete:
		m_qp_deleteDecoratorCurrentStatePixmap = (v ? m_qp_deletePressedDecoratorPixmap : m_qp_deleteDecoratorPixmap);
		m_qp_drDecoratorCurrentlyRenderingPixmap = m_qp_deleteDecoratorCurrentStatePixmap;
		break;
	default:	//None
		m_qp_drDecoratorCurrentlyRenderingPixmap = 0;
		break;
	}
	if (parentObject())
	{
		parentObject()->update();
	}
}

//virtual
Page * IconBase::owningPage() const
{
	//return m_qp_currentOwnerPage;
	return qobject_cast<Page *>(m_qp_takerOwner);
}

//virtual
LauncherObject * IconBase::owningLauncher() const
{
	return qobject_cast<LauncherObject *>(m_qp_takerOwner);
}

QString IconBase::iconLabel() const
{
	return m_iconLabel;
}

void IconBase::setIconLabel(const QString& v)
{
	m_iconLabel = v;
	// now text layout can be done and final text box computed (m_labelGeom)
	redoLabelTextLayout(m_usePrerenderedLabel);
	// m_labelGeom is now set...so reposition the text box...
	recalculateLabelPosition();

}

void IconBase::resetIconLabel()
{
	m_iconLabel = QString("?");

	// now text layout can be done and final text box computed (m_labelGeom)
	redoLabelTextLayout(m_usePrerenderedLabel);
	// m_labelGeom is now set...so reposition the text box...
	recalculateLabelPosition();
}

void IconBase::setIconLabelVisibility(bool visible)
{
	m_showLabel = visible;
	update();
}

void IconBase::setUsePrerenderedLabel(bool usePreRendered)
{
	if(m_usePrerenderedLabel == usePreRendered)
		return;

	m_usePrerenderedLabel = usePreRendered;

	if(m_usePrerenderedLabel) {
		redoLabelTextLayout(true);
	} else {
		if(m_qp_prerenderedLabelPixmap) {
			delete m_qp_prerenderedLabelPixmap;
			m_qp_prerenderedLabelPixmap = 0;
		}
	}
}

void IconBase::setLaunchFeedbackVisibility(bool visible)
{
	m_showFeedback = visible;
	update();
}

//virtual
LayoutItem * IconBase::layoutItemAssociation() const
{
	return m_qp_layoutItemAssociation;
}

//virtual
void IconBase::setLayoutItemAssociation(LayoutItem * v)
{
	m_qp_layoutItemAssociation = v;
}

//public Q_SLOTS:

//virtual
void IconBase::slotUpdateIconPic(PixmapObject * p_newPixmap,bool propagate,PixmapObject*& r_p_oldPixmap)
{
	r_p_oldPixmap = m_qp_iconPixmap;
	if (!p_newPixmap)
		return;
	//check it against the geom..if it's too big, reject it
	if (!canUsePixOnIcon(*this,p_newPixmap))
		return;
	m_qp_iconPixmap = p_newPixmap;
	qDebug() << __PRETTY_FUNCTION__ << ": Setting new main icon pic";
	m_iconPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_newPixmap->size()).toRect();
	recomputePainterHelpers();
	if ((propagate) && (cloneOf() == 0))
	{
		//if I am a master icon (m_qp_masterIcon == 0), then there may be clones connected to me
		//...emit a signal that changes their main icon too
		Q_EMIT signalMasterIconMainImageChanged(p_newPixmap);
	}
	update();
}

//virtual
void IconBase::slotMasterIconMainImageChanged(PixmapObject * p_newMainIconPixmap)
{
	// a special case of slotUpdateIconPic
	// sender() here is an IconBase, and it's the master icon for this copy/clone of it
	// if it isn't then the convention was violated and things are about to go very wrong
	IconBase * pMasterIcon = qobject_cast<IconBase *>(sender());
	if ((pMasterIcon != m_qp_masterIcon) || (!p_newMainIconPixmap))
	{
		return;	//safety
	}
	//TODO: PMO-MANAGE: properly dispose old PMO
	m_qp_iconPixmap = p_newMainIconPixmap;
	m_iconPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_newMainIconPixmap->size()).toRect();
	recomputePainterHelpers();
	//note: shouldn't need to propagate the signal because clone() should have assured there is no way
	// 		to have m_qp_masterIcon point to a clone, and thus the sender() here is a master icon,
	// 		and there are no clones with m_qp_masterIcon pointing to this
	update();
}

//virtual
void IconBase::slotMasterIconInstallStatusDecoratorChanged(PixmapObject * p_newPixmap)
{
	IconBase * pMasterIcon = qobject_cast<IconBase *>(sender());
	if (pMasterIcon != m_qp_masterIcon)
	{
		return;	//safety
	}
	//TODO: PMO-MANAGE: properly dispose old PMO

	m_qp_installStatusDecoratorPixmap = p_newPixmap;
	if (p_newPixmap)
	{
		m_installStatusDecoratorPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_newPixmap->size()).toRect();
	}
	else
	{
		m_installStatusDecoratorPixmapGeom = QRect();
	}
	recomputePainterHelpers();
	update();
}

//virtual
void IconBase::slotMasterIconInstallStatusDecoratorParamsChanged(int progressVal,int minProgressVal,int maxProgressVal)
{
	m_minProgressValue = minProgressVal;
	m_maxProgressValue = maxProgressVal;

	if (!m_qp_installStatusDecoratorPixmap)
	{
		return;
	}
	int range = m_minProgressValue - m_maxProgressValue;
	if (range == 0)
	{
		//would result in the divisor == 0, and is obviously the result of a bad update
		return;
	}

	int numFrames = m_qp_installStatusDecoratorPixmap->property(PixmapFilmstripObject::TotalFramesPropertyName).toInt();
	m_lastProgressValue = progressVal - m_minProgressValue;

	qreal f = (qreal)m_lastProgressValue / (qreal)range;
	quint32 frame = (quint32)((qreal)numFrames * f);

	//TODO: make this better. Q_PROPERTIES should have a way to be "weak-binded" (???)
	PixmapFilmstripObject * pFilmstripPmo = qobject_cast<PixmapFilmstripObject *>(m_qp_installStatusDecoratorPixmap);
	if (pFilmstripPmo)
	{
		pFilmstripPmo->setProperty(PixmapFilmstripObject::FrameIndexPropertyName,frame);
	}
	update();
}

//virtual
void IconBase::slotChangeIconVisibility(bool show)
{
	m_showIcon = show;
	update();
}


////////////// TODO: take into account decorators when calcing geoms

//virtual
void IconBase::slotUpdateIconFramePic(PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap)
{
	r_p_oldPixmap = m_qp_iconPixmap;
	if (!p_newPixmap)
		return;
	if (!canUsePixOnIcon(*this,p_newPixmap))
		return;
	m_qp_iconFramePixmap = p_newPixmap;
	m_iconFramePixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_newPixmap->size()).toRect();

	//because the decorators are potentially tied to the frame geometry, redo their positions first
	recalculateDecoratorPositionOnFrameChange();
	recomputePainterHelpers();
	update();
}

//virtual
void IconBase::slotChangeIconFrameVisibility(bool show)
{
	m_showFrame = show;
	update();
}

//virtual
void IconBase::slotUpdateRemoveDecoratorPic(RemoveDeleteDecoratorState::Enum whichStatePic,PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap)
{
	//TODO: perhaps a way to suppress recomputing painter helpers until both state pics have been set.
	// 		chances are, they (the calls to this fn) will come in pairs

	r_p_oldPixmap = (whichStatePic == RemoveDeleteDecoratorState::Normal ? m_qp_removeDecoratorPixmap : m_qp_removePressedDecoratorPixmap);
	if (!p_newPixmap)
		return;
	if (!canUsePixOnIcon(*this,p_newPixmap))
		return;
	(whichStatePic == RemoveDeleteDecoratorState::Normal ? m_qp_removeDecoratorPixmap : m_qp_removePressedDecoratorPixmap) = p_newPixmap;
	m_removeDecoratorPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_newPixmap->size()).toRect();
	recomputePainterHelpers();
	update();
}

//virtual
void IconBase::slotUpdateDeleteDecoratorPic(RemoveDeleteDecoratorState::Enum whichStatePic,PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap)
{
	//TODO: perhaps a way to suppress recomputing painter helpers until both state pics have been set.
	// 		chances are, they (the calls to this fn) will come in pairs

	r_p_oldPixmap = (whichStatePic == RemoveDeleteDecoratorState::Normal ? m_qp_deleteDecoratorPixmap : m_qp_deletePressedDecoratorPixmap);
	if (!p_newPixmap)
		return;
	if (!canUsePixOnIcon(*this,p_newPixmap))
		return;
	(whichStatePic == RemoveDeleteDecoratorState::Normal ? m_qp_deleteDecoratorPixmap : m_qp_deletePressedDecoratorPixmap) = p_newPixmap;
	m_deleteDecoratorPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_newPixmap->size()).toRect();
	recomputePainterHelpers();
	update();
}

//virtual
void IconBase::slotChangeRemoveDeleteDecoratorVisibility(RemoveDeleteDecoratorSelector::Enum showWhich)
{
	switch (showWhich)
	{
	case RemoveDeleteDecoratorSelector::Remove:
		m_showWhichDeleteRemove = RemoveDeleteDecoratorSelector::Remove;
		m_qp_removeDecoratorCurrentStatePixmap = m_qp_removeDecoratorPixmap;
		m_qp_drDecoratorCurrentlyRenderingPixmap = m_qp_removeDecoratorPixmap;
		break;
	case RemoveDeleteDecoratorSelector::Delete:
		m_showWhichDeleteRemove = RemoveDeleteDecoratorSelector::Delete;
		m_qp_deleteDecoratorCurrentStatePixmap = m_qp_deleteDecoratorPixmap;
		m_qp_drDecoratorCurrentlyRenderingPixmap = m_qp_deleteDecoratorPixmap;
		break;
	default:	//None
		m_showWhichDeleteRemove = RemoveDeleteDecoratorSelector::None;
		m_qp_drDecoratorCurrentlyRenderingPixmap = 0;
		break;
	}
	//TODO: optimize; no need for full recompute
	recomputePainterHelpers();
	update();
}

//virtual
void IconBase::slotUpdateInstallStatusDecoratorPic(PixmapObject * p_newPixmap,PixmapObject*& r_p_oldPixmap)
{
	//NOTE THAT UNLIKE THE OTHER UPDATE SLOTS, THIS ONE *CAN* ALLOW A 0 (NULL) NEW PIXMAP.
	// (this is how install "status" is cleared...no sense in having the iconbase obj take up memory with install pixmaps
	//	once installation is done)
	r_p_oldPixmap = m_qp_installStatusDecoratorPixmap;
	m_qp_installStatusDecoratorPixmap = p_newPixmap;
	if (p_newPixmap)
	{
		if (!canUsePixOnIcon(*this,p_newPixmap))
		{
			m_installStatusDecoratorPixmapGeom = QRect();
			return;	//TODO: BROKEN: not quite right; the clones will not update correctly
		}
		m_installStatusDecoratorPixmapGeom = DimensionsGlobal::realRectAroundRealPoint(p_newPixmap->size()).toRect();
	}
	else
	{
		m_installStatusDecoratorPixmapGeom = QRect();
	}

	if (cloneOf() == 0)
	{
		Q_EMIT signalMasterIconInstallStatusDecoratorChanged(m_qp_installStatusDecoratorPixmap);
	}

	recomputePainterHelpers();
	update();
}

//virtual
void IconBase::slotUpdateInstallStatusDecoratorPicNewProgress(int progressVal,bool updateMinMax,int minProgressVal,int maxProgressVal)
{
	if (updateMinMax)
	{
		m_minProgressValue = minProgressVal;
		m_maxProgressValue = maxProgressVal;
	}

	qDebug() << __FUNCTION__ << ": entry: progressVal = " << progressVal << " , updateMinMax = " << updateMinMax << " , minProgressVal = " << minProgressVal << " , maxProgressVal = " << maxProgressVal;
	if (!m_qp_installStatusDecoratorPixmap)
	{
		qDebug() << __FUNCTION__ << ": error: no install status decorator!";
		return;
	}
	int range = m_maxProgressValue - m_minProgressValue;
	if (range == 0)
	{
		//would result in the divisor == 0, and is obviously the result of a bad update
		qDebug() << __FUNCTION__ << ": error: min/max range is 0";
		return;
	}

	int numFrames = m_qp_installStatusDecoratorPixmap->property(PixmapFilmstripObject::TotalFramesPropertyName).toInt();
	m_lastProgressValue = progressVal - m_minProgressValue;

	qreal f = (qreal)m_lastProgressValue / (qreal)range;
	quint32 frame = (quint32)((qreal)numFrames * f);

	//TODO: make this better. Q_PROPERTIES should have a way to be "weak-binded" (???)
	PixmapFilmstripObject * pFilmstripPmo = qobject_cast<PixmapFilmstripObject *>(m_qp_installStatusDecoratorPixmap);
	if (pFilmstripPmo)
	{
		qDebug() << __FUNCTION__ << ": setting filmstrip to frame " << frame;
		pFilmstripPmo->setProperty(PixmapFilmstripObject::FrameIndexPropertyName,frame);
	}

	if (cloneOf() == 0)
	{
		Q_EMIT signalMasterIconInstallStatusDecoratorParamsChanged(progressVal,minProgressVal,maxProgressVal);
	}

	if (parentItem())
	{
		parentItem()->update();
	}
	else if (LauncherObject::primaryInstance())
	{
		LauncherObject::primaryInstance()->update();
	}
}

//virtual
void IconBase::slotUpdateInstallStatusDecoratorResetProgress(int minProgressVal,int maxProgressVal)
{
	m_minProgressValue = minProgressVal;
	m_maxProgressValue = maxProgressVal;
	m_lastProgressValue = 0;
}

//virtual
void IconBase::slotEnableIconAutoRepaint()
{
	setFlag(QGraphicsItem::ItemHasNoContents,false);
	setVisible(true);
	update();
}

//virtual
void IconBase::setAutopaintClipRect(const QRect& clipRectPCS)
{
	if (clipRectPCS.isValid())
	{
		m_ownerSetAutopaintClip = clipRectPCS;
		if (clipRectPCS.isEmpty())
		{
			m_useOwnerSetAutopaintClip = false;
		}
		else
		{
			m_useOwnerSetAutopaintClip = true;
		}
	}
	else
	{
		m_ownerSetAutopaintClip = QRect();
		m_useOwnerSetAutopaintClip = false;
	}
}

//virtual
void IconBase::clearAutopaintClipRect()
{
	m_ownerSetAutopaintClip = QRect();
	m_useOwnerSetAutopaintClip = false;
}

//virtual
void IconBase::slotDisableIconAutoRepaint(bool clearAutoclip)
{
	if (clearAutoclip)
	{
		clearAutopaintClipRect();
	}
	setVisible(false);
	setFlag(QGraphicsItem::ItemHasNoContents,true);
	update();
}

//virtual
void IconBase::update()
{
	//TODO: UPDATE-PAINT-WORKAROUND:
	if (parentItem())
	{
		parentItem()->update();
	}
	else if (LauncherObject::primaryInstance())
	{
		LauncherObject::primaryInstance()->update();
	}

	ThingPaintable::update();
}

//virtual
void IconBase::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	if (m_useOwnerSetAutopaintClip)
	{
		QRect srcRect = untranslateFromPosition(positionRelativeGeometry().intersected(m_ownerSetAutopaintClip)).toRect();
		return paint(painter,srcRect);
	}
	//TODO: PIXEL-ALIGN
	QRect iconGeom = m_geom.toRect();

	qreal savedOpacity = painter->opacity();
	if (m_qp_installStatusDecoratorPixmap)
	{
		//fade the icon
		painter->setOpacity(DynamicsSettings::settings()->iconInstallModeOpacity);
	}

	if (m_showFeedback)
	{
		// paint the launch feedback behind the icon
		QRect sourceFragmentRect = m_iconFeedbackPixmapGeom.translated(m_iconFeedbackPosICS);
			painter->drawPixmap(sourceFragmentRect.topLeft(),
								*(*m_qp_iconFeedbackPixmap),
								sourceFragmentRect.translated(m_iconFeedbackSrcPrecomputed));
	}

	if (m_showFrame)
	{
		QRect sourceFragmentRect = m_iconFramePixmapGeom.translated(m_iconFramePosICS);
			painter->drawPixmap(sourceFragmentRect.topLeft(),
								*(*m_qp_iconFramePixmap),
								sourceFragmentRect.translated(m_iconFrameSrcPrecomputed));
	}
	if (m_showIcon)
	{
		QRect sourceFragmentRect = m_iconPixmapGeom.translated(m_iconPosICS);
		if (!sourceFragmentRect.isEmpty())
		{
			painter->drawPixmap(sourceFragmentRect.topLeft(),
					*(*m_qp_iconPixmap),
					sourceFragmentRect.translated(m_iconSrcPrecomputed));
		}
	}

	if (m_qp_drDecoratorCurrentlyRenderingPixmap)
	{
		QRect sourceFragmentRect = m_drDecoratorCurrentPixmapGeom.translated(m_drDecoratorCurrentPosICS);
		if (!sourceFragmentRect.isEmpty())
		{
			painter->drawPixmap(sourceFragmentRect.topLeft(),
					*(*m_qp_drDecoratorCurrentlyRenderingPixmap),
					sourceFragmentRect.translated(m_drDecoratorCurrentSrcPrecomputed));
		}
	}

	//TODO: PIXEL-ALIGN

	if (m_showLabel) {
		QRect sourceFragmentRect = m_labelGeom.translated(m_labelPosICS);
		if(m_qp_prerenderedLabelPixmap) {
			// use Pre-rendered label image
			if (!sourceFragmentRect.isEmpty()) {
				painter->drawPixmap(sourceFragmentRect.topLeft(),
						*(*m_qp_prerenderedLabelPixmap),
						sourceFragmentRect.translated(m_labelSrcPrecomputed));
			}
		} else {
			QPen savePen = painter->pen();
			painter->setPen(m_labelColor);
			m_textLayoutObject.draw(painter,sourceFragmentRect.topLeft());
		}
	}

	if (m_qp_installStatusDecoratorPixmap)
	{
		//restore
		painter->setOpacity(savedOpacity);
		QRect sourceFragmentRect = m_installStatusDecoratorPixmapGeom.translated(m_installStatusDecoratorPosICS);
		if (!sourceFragmentRect.isEmpty())
		{
//			painter->drawPixmap(sourceFragmentRect.topLeft(),
//					*(*m_qp_installStatusDecoratorPixmap),
//					sourceFragmentRect.translated(m_installStatusDecoratorSrcPrecomputed));
			m_qp_installStatusDecoratorPixmap->paint(painter,sourceFragmentRect,sourceFragmentRect.translated(m_installStatusDecoratorSrcPrecomputed));
		}
	}


#ifdef PAINT_GEOM_BOXES

	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,5);
	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_iconPosICS,m_iconGeom,6);
	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_labelPosICS,m_labelGeom,7);
#endif
}

//virtual
void IconBase::paint(QPainter *painter, const QRectF& sourceItemRect)
{
	/*
	 * sourceItemRect is in ICS of THIS item ; the compatible space is m_geom
	 *
	 */

	//TODO: PIXEL-ALIGN
	QRect iconGeom = m_geom.toRect();
	QRect sourceRect = sourceItemRect.toRect();
	qreal savedOpacity = painter->opacity();

	if (m_qp_installStatusDecoratorPixmap)
	{
		//fade the icon
		painter->setOpacity(DynamicsSettings::settings()->iconInstallModeOpacity);
	}

	if (m_showFeedback)
	{
		QRect sourceFragmentRect = m_iconFeedbackPixmapGeom.translated(m_iconFeedbackPosICS).intersect(sourceRect);	//this brought it into the space of m_iconFramePixmapGeom
		if (!sourceFragmentRect.isEmpty())
		{
			painter->drawPixmap(sourceFragmentRect.topLeft(),
								*(*m_qp_iconFeedbackPixmap),
								sourceFragmentRect.translated(m_iconFeedbackSrcPrecomputed));
		}
	}

	if (m_showFrame)
	{
		QRect sourceFragmentRect = m_iconFramePixmapGeom.translated(m_iconFramePosICS).intersect(sourceRect);	//this brought it into the space of m_iconFramePixmapGeom
		if (!sourceFragmentRect.isEmpty())
		{
			painter->drawPixmap(sourceFragmentRect.topLeft(),
								*(*m_qp_iconFramePixmap),
								sourceFragmentRect.translated(m_iconFrameSrcPrecomputed));
		}
	}
	if (m_showIcon)
	{
		QRect sourceFragmentRect = m_iconPixmapGeom.translated(m_iconPosICS).intersect(sourceRect); //this brought it into the space of m_iconPixmapGeom
		if (!sourceFragmentRect.isEmpty())
		{
			painter->drawPixmap(sourceFragmentRect.topLeft(),
					*(*m_qp_iconPixmap),
					sourceFragmentRect.translated(m_iconSrcPrecomputed));
		}
	}

	if (m_qp_drDecoratorCurrentlyRenderingPixmap)
		{
			QRect sourceFragmentRect = m_drDecoratorCurrentPixmapGeom.translated(m_drDecoratorCurrentPosICS).intersect(sourceRect);
			if (!sourceFragmentRect.isEmpty())
			{
				painter->drawPixmap(sourceFragmentRect.topLeft(),
						*(*m_qp_drDecoratorCurrentlyRenderingPixmap),
						sourceFragmentRect.translated(m_drDecoratorCurrentSrcPrecomputed));
			}
		}

	if (m_showLabel) {
		QRect sourceFragmentRect = m_labelGeom.translated(m_labelPosICS).intersect(sourceRect);
		if(m_qp_prerenderedLabelPixmap) {
			// use Pre-rendered label image
			if (!sourceFragmentRect.isEmpty()) {
				painter->drawPixmap(sourceFragmentRect.topLeft(),
						*(*m_qp_prerenderedLabelPixmap),
						sourceFragmentRect.translated(m_labelSrcPrecomputed));
			}
		} else if(sourceRect.contains(m_labelGeom.translated(m_labelPosICS))){
			//the label ... if it isn't prerendered, it cannot be partially drawn (at least not vertically partial; can
			//	be horizontally/width partial). Thus if the whole label doesn't fit into the sourceRect, don't paint it
			//TODO: PIXEL-ALIGN

			QPen savePen = painter->pen();
			painter->setPen(m_labelColor);
			m_textLayoutObject.draw(painter,sourceFragmentRect.topLeft());
		}
	}

	if (m_qp_installStatusDecoratorPixmap)
	{
		//restore
		painter->setOpacity(savedOpacity);
		QRect sourceFragmentRect = m_installStatusDecoratorPixmapGeom.translated(m_installStatusDecoratorPosICS).intersect(sourceRect);
		if (!sourceFragmentRect.isEmpty())
		{
//			painter->drawPixmap(sourceFragmentRect.topLeft(),
//					*(*m_qp_installStatusDecoratorPixmap),
//					sourceFragmentRect.translated(m_installStatusDecoratorSrcPrecomputed));
			m_qp_installStatusDecoratorPixmap->paint(painter,sourceFragmentRect,sourceFragmentRect.translated(m_installStatusDecoratorSrcPrecomputed));
		}
	}

#ifdef PAINT_GEOM_BOXES

	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,5);
	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_iconPosICS,m_iconGeom,6);
	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_labelPosICS,m_labelGeom,7);
#endif
}

//virtual
void IconBase::paint(QPainter *painter, const QRectF& sourceItemRect,qint32 renderStage)
{
	/*
	 * sourceItemRect is in ICS of THIS item ; the compatible space is m_geom
	 *
	 */

	//TODO: PIXEL-ALIGN
	QRect iconGeom = m_geom.toRect();
	QRect sourceRect = sourceItemRect.toRect();
	qreal savedOpacity = painter->opacity();

	if (m_qp_installStatusDecoratorPixmap)
	{
		//fade the icon
		painter->setOpacity(DynamicsSettings::settings()->iconInstallModeOpacity);
	}

	if (renderStage & IconRenderStage::IconFrame)
	{
		if (m_showFeedback)
		{
			QRect sourceFragmentRect = m_iconFeedbackPixmapGeom.translated(m_iconFeedbackPosICS).intersect(sourceRect);	//this brought it into the space of m_iconFramePixmapGeom
			if (!sourceFragmentRect.isEmpty())
			{
				painter->drawPixmap(sourceFragmentRect.topLeft(),
									*(*m_qp_iconFeedbackPixmap),
									sourceFragmentRect.translated(m_iconFeedbackSrcPrecomputed));
			}
		}

		if (m_showFrame)
		{
			QRect sourceFragmentRect = m_iconFramePixmapGeom.translated(m_iconFramePosICS).intersect(sourceRect);	//this brought it into the space of m_iconFramePixmapGeom
			if (!sourceFragmentRect.isEmpty())
			{
				painter->drawPixmap(sourceFragmentRect.topLeft(),
									*(*m_qp_iconFramePixmap),
									sourceFragmentRect.translated(m_iconFrameSrcPrecomputed));
			}
		}
	}

	if (renderStage & IconRenderStage::Icon)
	{
		if (m_showIcon)
		{
			QRect sourceFragmentRect = m_iconPixmapGeom.translated(m_iconPosICS).intersect(sourceRect); //this brought it into the space of m_iconPixmapGeom
			if (!sourceFragmentRect.isEmpty())
			{
				painter->drawPixmap(sourceFragmentRect.topLeft(),
						*(*m_qp_iconPixmap),
						sourceFragmentRect.translated(m_iconSrcPrecomputed));
			}
		}
	}

	if (renderStage & IconRenderStage::Decorators)
	{
		if (m_qp_drDecoratorCurrentlyRenderingPixmap)
		{
			QRect sourceFragmentRect = m_drDecoratorCurrentPixmapGeom.translated(m_drDecoratorCurrentPosICS).intersect(sourceRect);
			if (!sourceFragmentRect.isEmpty())
			{
				painter->drawPixmap(sourceFragmentRect.topLeft(),
						*(*m_qp_drDecoratorCurrentlyRenderingPixmap),
						sourceFragmentRect.translated(m_drDecoratorCurrentSrcPrecomputed));
			}
		}
	}

	if (renderStage & IconRenderStage::Label)
	{
		if (m_showLabel) {
			QRect sourceFragmentRect = m_labelGeom.translated(m_labelPosICS).intersect(sourceRect);
			if(m_qp_prerenderedLabelPixmap) {
				// use Pre-rendered label image
				if (!sourceFragmentRect.isEmpty()) {
					painter->drawPixmap(sourceFragmentRect.topLeft(),
							*(*m_qp_prerenderedLabelPixmap),
							sourceFragmentRect.translated(m_labelSrcPrecomputed));
				}
			} else if(sourceRect.contains(m_labelGeom.translated(m_labelPosICS))){
				//the label ... if it isn't prerendered, it cannot be partially drawn (at least not vertically partial; can
				//	be horizontally/width partial). Thus if the whole label doesn't fit into the sourceRect, don't paint it
				//TODO: PIXEL-ALIGN

				QPen savePen = painter->pen();
				painter->setPen(m_labelColor);
				m_textLayoutObject.draw(painter,sourceFragmentRect.topLeft());
			}
		}
	}

	if (m_qp_installStatusDecoratorPixmap)
	{
		//restore
		painter->setOpacity(savedOpacity);
	}
	if (renderStage & IconRenderStage::Decorators)
	{
		if (m_qp_installStatusDecoratorPixmap)
		{
			QRect sourceFragmentRect = m_installStatusDecoratorPixmapGeom.translated(m_installStatusDecoratorPosICS).intersect(sourceRect);
			if (!sourceFragmentRect.isEmpty())
			{
				m_qp_installStatusDecoratorPixmap->paint(painter,sourceFragmentRect,sourceFragmentRect.translated(m_installStatusDecoratorSrcPrecomputed));
			}
		}
	}


#ifdef PAINT_GEOM_BOXES

	DimensionsDebugGlobal::dbgPaintBoundingRect(painter,m_geom,5);
	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_iconPosICS,m_iconGeom,6);
	DimensionsDebugGlobal::dbgPaintBoundingRectWithTranslate(painter,m_labelPosICS,m_labelGeom,7);
#endif
}

//virtual
void IconBase::paintOffscreen(QPainter *painter)
{
	//TODO: unimplemented / IMPLEMENT
}

//virtual
void IconBase::paintOffscreen(QPainter *painter,const QRect& sourceRect,const QPoint& targetOrigin)
{
	//TODO: unimplemented / IMPLEMENT
}

//virtual
void IconBase::paintOffscreen(QPainter *painter,const QRect& sourceRect,const QRect& targetRect)
{
	//TODO: unimplemented / IMPLEMENT
}

//protected Q_SLOTS:

//virtual
void IconBase::slotAnimationFinished()
{

}

//virtual
void IconBase::slotRemoveDeleteActivated()
{
	//qDebug() << __FUNCTION__ << ": requesting launcher for Remove/Delete";
	g_warning("[ICON-TAP-TRACE] %s: requesting launcher for Remove/Delete",__FUNCTION__);
//	Q_EMIT signalIconActionRequest((int)(IconActionRequest::RequestRemoveDelete));
	QCoreApplication::postEvent(LauncherObject::primaryInstance(),new IconCmdRequestEvent(this,(int)(IconActionRequest::RequestRemoveDelete)));
}

//---------protected:

//static
QSize IconBase::maxIconSize(const IconList& icons)
{
	QSize maxs = QSize(0,0);
	QSize csize;
	for (IconListConstIter it = icons.constBegin();
			it != icons.constEnd();++it)
	{
		csize = (*it)->geometry().size().toSize();
		if (csize.width() > maxs.width())
		{
			maxs.setWidth(csize.width());
		}
		if (csize.height() > maxs.height())
		{
			maxs.setHeight(csize.height());
		}
	}
	return maxs;
}

//static
QSize IconBase::minIconSize(const IconList& icons)
{
	QSize mins = QSize(INT_MAX,INT_MAX);
	QSize csize;
	for (IconListConstIter it = icons.constBegin();
			it != icons.constEnd();++it)
	{
		csize = (*it)->geometry().size().toSize();
		if (csize.width() < mins.width())
		{
			mins.setWidth(csize.width());
		}
		if (csize.height() < mins.height())
		{
			mins.setHeight(csize.height());
		}
	}
	return mins;
}
//static
void IconBase::iconSizeBounds(const IconList& icons,QSize& r_minSize,QSize& r_maxSize)
{
	r_minSize = minIconSize(icons);
	r_maxSize = maxIconSize(icons);
}

//static
bool IconBase::canUseFramePixOnIcon(const IconBase& icon,PixmapObject * p_pix)
{
	return (canUsePixOnIcon(icon,p_pix));
}
//static
bool IconBase::canUseMainPixOnIcon(const IconBase& icon,PixmapObject * p_pix)
{
	return (canUsePixOnIcon(icon,p_pix));
}
//static
bool IconBase::canUseDecorPixOnIcon(const IconBase& icon,PixmapObject * p_pix)
{
	return (canUsePixOnIcon(icon,p_pix));
}

//virtual
bool IconBase::usingInstallDecorator() const
{
	return (m_qp_installStatusDecoratorPixmap != 0);
}

//
////virtual
//bool IconBase::resize(quint32 w, quint32 h) 	// called by the ui owner when the ui itself resizes
//{
//	//TODO: handle it fully - partially punt for now, and this will probbly f&$(# up layout
//
//	if (!w || !h)
//	{
//		return false;
//	}
//
//	//TODO: TEMP
//	//for now, be dumb about it. adjust geom, and accept the resize, but don't resize pixmaps or anything advanced
//
//	updateGeometry(DimensionsGlobal::realRectAroundRealPoint(QSize(w,h)));
//	return true;
//}

//virtual
QRectF IconBase::geometry() const
{
	if (IconGeometrySettings::settings()->useAlignmentGeom)
	{
		return m_alignmentGeomPrecomputed;
	}
	return ThingPaintable::geometry();
}

//virtual
QRectF IconBase::positionRelativeGeometry() const
{
	return geometry().translated(pos());

}

//virtual
QPointF	IconBase::activePos() const
{
	return pos()+m_activePositionOffsetFromPos;
}

//virtual
bool IconBase::resize(quint32 w, quint32 h) 	// called by the ui owner when the ui itself resizes
{
	if (!w || !h)
	{
		return false;
	}
	return resize(QSize(w,h));
}

//virtual
bool IconBase::resize(const QSize& s)
{
	//TODO: TEMP
	//for now, be dumb about it. adjust geom, and accept the resize, but don't resize pixmaps or anything advanced
	// however, do reposition text , icons , and do a text layout pass

	ThingPaintable::resize(s);
	geometryChanged();
	return true;

}

//virtual
bool IconBase::expand(const quint32 widthEx,const quint32 heightEx)
{
	if ((widthEx == 0) && (heightEx == 0))
	{
		return true;
	}
	QSize newSize = (m_geom.size() + QSizeF((qreal)widthEx,(qreal)heightEx)).toSize();
	return resize(newSize.width(),newSize.height());
}

////virtual
//bool IconBase::take(Thing * p_takerThing)
//{
//	Page * pTakerPage = qobject_cast<Page *>(p_takerThing);
//	if (pTakerPage)
//	{
//		if ((m_qp_currentOwnerPage) && (p_takerThing != qobject_cast<Thing *>(m_qp_currentOwnerPage.data())))
//		{
//			if (!(m_qp_currentOwnerPage->taking(this,p_takerThing)))
//			{
//				return false;		//abduction blocked by current owner
//			}
//			m_qp_currentOwnerPage->taken(this,p_takerThing);
//		}
//		m_qp_currentOwnerPage = pTakerPage;
//		setParentItem(pTakerPage);
//		return true;
//	}
//
//	QuickLaunchBar * pQuicklaunch = qobject_cast<QuickLaunchBar *>(p_takerThing);
//	if (pQuicklaunch)
//	{
//
//	}
//	return true;
//}

//virtual
bool IconBase::offer(Thing * p_offer,Thing * p_offeringThing)
{
	//icons don't accept anything
	return false;
}

//virtual
bool IconBase::take(Thing * p_takerThing)
{
	if (m_qp_takerOwner && (m_qp_takerOwner == p_takerThing))
	{
		//self take...
		return false;
	}

	//TODO: split up to the Page / Quicklaunch cases, even though they're the same in this function.
	// This is just for reference purposes, to show that different processing can be done, depending
	// I will condense this later when this isn't needed for ref anymore.

	Page * pPage = qobject_cast<Page *>(p_takerThing);
	if (pPage)
	{
		//the page is taking the icon away from another page

		bool ok = (m_qp_takerOwner ? m_qp_takerOwner->taking(this,p_takerThing) : true);
		if ((ok) && (m_qp_takerOwner))
		{
			m_qp_takerOwner->taken(this,p_takerThing);
			m_qp_takerOwner = p_takerThing;
		}
		else if (!m_qp_takerOwner)
		{
			m_qp_takerOwner = p_takerThing;
		}
		return ok;
	}

	QuickLaunchBar * pQuicklaunch = qobject_cast<QuickLaunchBar *>(p_takerThing);
	if (pQuicklaunch)
	{
		//if the quicklaunch is taking the icon away from a page (since there cannot be 2 quicklaunch bars)
		bool ok = (m_qp_takerOwner ? m_qp_takerOwner->taking(this,p_takerThing) : true);
		if ((ok) && (m_qp_takerOwner))
		{
			m_qp_takerOwner->taken(this,p_takerThing);
			m_qp_takerOwner = p_takerThing;
		}
		else if (!m_qp_takerOwner)
		{
			m_qp_takerOwner = p_takerThing;
		}
		return ok;
	}

	LauncherObject * pLauncher = qobject_cast<LauncherObject *>(p_takerThing);
	if (pLauncher)
	{
		//the launcher is temporarily assuming control...this is for page reorders and tab-based transfers to pages
		// the pages will be animating and the icon needs to be out of the way
		bool ok = (m_qp_takerOwner ? m_qp_takerOwner->taking(this,p_takerThing) : true);
		if ((ok) && (m_qp_takerOwner))
		{
			m_qp_takerOwner->taken(this,p_takerThing);
			m_qp_takerOwner = p_takerThing;
		}
		else if (!m_qp_takerOwner)
		{
			m_qp_takerOwner = p_takerThing;
		}
		return ok;
	}
	return true;
}

//virtual
bool IconBase::taking(Thing * p_victimThing, Thing * p_takerThing)
{
	return false;		//icons don't own anything that can be taken at this time
}

//virtual
void IconBase::taken(Thing * p_takenThing,Thing * p_takerThing)
{
	//won't ever be called. Icons don't have anything take-able
}

//virtual
bool IconBase::sceneEvent(QEvent * event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		ge->accept();
		return true;
	}
	else if (event->type() == QEvent::Gesture) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture * g = 0;
		//		QGesture* g = ge->gesture(Qt::TapGesture);
		//		if (g) {
		//			QTapGesture* tap = static_cast<QTapGesture*>(g);
		//			if (tap->state() == Qt::GestureFinished) {
		//				tapGestureEvent(tap);
		//			}
		//			return true;
		//		}
		//		g = ge->gesture(Qt::TapAndHoldGesture);
		//		if (g) {
		//			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
		//			if (hold->state() == Qt::GestureFinished) {
		//				tapAndHoldGestureEvent(hold);
		//			}
		//			return true;
		//		}
		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g) {
			FlickGesture* flick = static_cast<FlickGesture*>(g);
			if (flick->state() == Qt::GestureFinished) {
				flickGesture(flick);
			}
			return true;
		}
	}
	return QGraphicsObject::sceneEvent(event);
}

//virtual
bool IconBase::swipeGesture(QSwipeGesture *swipeEvent)
{
	return true;
}

//virtual
bool IconBase::flickGesture(FlickGesture *flickEvent)
{
	return true;
}

//virtual
bool IconBase::tapIntoIcon(const QPointF& touchCoordinateICS,IconInternalHitAreas::Enum& r_hitArea)
{
	if ( (m_showWhichDeleteRemove == RemoveDeleteDecoratorSelector::Delete) ||  (m_showWhichDeleteRemove == RemoveDeleteDecoratorSelector::Remove))
	{
//		g_warning("[ICON-TAP-TRACE] %s: icon decorator for Delete or Remove IS shown",__FUNCTION__);
		if (
				(m_drDecoratorCurrentPixmapGeom.translated(m_drDecoratorCurrentPosICS).contains(touchCoordinateICS.toPoint()))
		)
		{
//			g_warning("[ICON-TAP-TRACE] %s: tap WAS inside the decorator area, >>TRUE EXIT<<",__FUNCTION__);
			//hit...
			Q_EMIT signalFSMActivate();
			r_hitArea = IconInternalHitAreas::RemoveDeleteDecorator;
			return true;
		}
		else
		{
			g_warning("[ICON-TAP-TRACE] %s: tap NOT inside the decorator area",__FUNCTION__);
			r_hitArea = IconInternalHitAreas::Other;
		}
	}
	else
	{
		g_warning("[ICON-TAP-TRACE] %s: icon decorator for Delete or Remove NOT shown",__FUNCTION__);
		r_hitArea = IconInternalHitAreas::Other;
	}

	return false;
}

//virtual
bool IconBase::touchStartIntoIcon(const QPointF& touchCoordinateICS)
{
	if ( ( (m_showWhichDeleteRemove == RemoveDeleteDecoratorSelector::Delete) ||  (m_showWhichDeleteRemove == RemoveDeleteDecoratorSelector::Remove))
			&& (m_drDecoratorCurrentPixmapGeom.translated(m_drDecoratorCurrentPosICS).contains(touchCoordinateICS.toPoint()))
	)
	{
		//hit...
		Q_EMIT signalFSMActivate();
		return true;
	}
	return false;
}
//virtual
bool IconBase::touchEndIntoIcon(const QPointF& touchCoordinateICS)
{
	if ( ( (m_showWhichDeleteRemove == RemoveDeleteDecoratorSelector::Delete) ||  (m_showWhichDeleteRemove == RemoveDeleteDecoratorSelector::Remove))
			&& (m_drDecoratorCurrentPixmapGeom.translated(m_drDecoratorCurrentPosICS).contains(touchCoordinateICS.toPoint()))
	)
	{
		//hit...
		Q_EMIT signalFSMDeactivate();
		return true;
	}
	return false;
}

//virtual
void IconBase::setupFSM()
{
	m_p_buttonFSM = new QStateMachine(this);
	m_p_buttonFSM->setObjectName("fsm");

	m_p_stateNormal        = DimensionsGlobal::createFSMState("normal",m_p_buttonFSM);
	m_p_stateActive     = DimensionsGlobal::createFSMState("active",m_p_buttonFSM);

	TimeDelayTransition * delayTransition = new TimeDelayTransition(300);
	delayTransition->setTargetState(m_p_stateNormal);

	// ------------------- STATE: normal -----------------------------------
	//	normal PROPERTIES
	m_p_stateNormal->assignProperty(this,"stateRemoveDeleteActive", false);
	//  normal TRANSITIONS
	m_p_stateNormal->addTransition(this,SIGNAL(signalFSMActivate()),m_p_stateActive);
	//  normal SIDE-EFFECTS
	connect(m_p_stateNormal,SIGNAL(entered()),delayTransition,SLOT(slotAbort()));

	// ------------------- STATE: active ------------------------------------
	// active PROPERTIES
	m_p_stateActive->assignProperty(this,"stateRemoveDeleteActive", true);
	// active TRANSITIONS
	QSignalTransition * pCompletedTransition = m_p_stateActive->addTransition(this, SIGNAL(signalFSMDeactivate()),m_p_stateNormal);
	m_p_stateActive->addTransition(delayTransition);
	delayTransition->setParent(m_p_stateActive);
	//  active SIDE-EFFECTS
	connect(m_p_stateActive,SIGNAL(propertiesAssigned()),delayTransition,SLOT(slotRestartTimer()));
	connect(pCompletedTransition,SIGNAL(triggered()),this,SLOT(slotRemoveDeleteActivated()));
	connect(delayTransition,SIGNAL(triggered()),this,SLOT(slotRemoveDeleteActivated()));

//	m_p_buttonFSM->addState(m_p_stateNormal);
//	m_p_buttonFSM->addState(m_p_stateActive);

	m_p_buttonFSM->setInitialState(m_p_stateNormal);
}

//virtual
bool IconBase::connectRequestsToLauncher(LauncherObject * launcherObj)
{

//	if (m_canRequestLauncher)
//	{
//		qDebug() << __FUNCTION__ << "---->Can already request";
//		return true;
//	}
//	if (launcherObj == 0)
//	{
//		launcherObj = LauncherObject::primaryInstance();
//	}

	//TODO: REMOVED: No longer needed; requests are done via events
//	m_canRequestLauncher = connect(this,SIGNAL(signalIconActionRequest(int)),
//									launcherObj,SLOT(slotIconActionRequest(int)),
//									Qt::QueuedConnection);
	m_canRequestLauncher = true;
//
//	if (!m_canRequestLauncher)
//	{
//		qDebug() << __FUNCTION__ << "---->FAIL!!!";
//	}
	return m_canRequestLauncher;
}

//virtual
void IconBase::geometryChanged()
{
	//assumed that the newGeom is valid. This is a protected function so if the param came from outside, the public facing fn
	// caller should have checked it

	//TODO: PIXEL-ALIGN, though they should be ok if EVEN constraint policy is being followed
	//m_geom is considered to be valid now. Compute the other geoms
	m_labelMaxGeom = LabelBoxGeomFromGeom(m_geom).toRect();
	m_iconFrameGeom = FrameGeomFromGeom(m_geom).toRect();
	m_iconGeom = IconGeomFromGeom(m_geom).toRect();
	m_removeDecoratorGeom = RemoveDeleteDecorGeomFromGeom(m_geom).toRect();
	m_deleteDecoratorGeom = RemoveDeleteDecorGeomFromGeom(m_geom).toRect();
	m_installStatusDecoratorGeom = InstallStatusGeomFromGeom(m_geom).toRect();
	//position the frame, main icon, and decorators
	m_iconPosICS = IconGeometrySettings::settings()->mainIconOffsetFromGeomOriginPx;
	m_iconFramePosICS = IconGeometrySettings::settings()->frameOffsetFromGeomOriginPx;
	m_iconFeedbackPosICS = IconGeometrySettings::settings()->feedbackOffsetFromGeomOriginPx;

	//the name sounds inappropriate but it's common code that's applicable here
	recalculateDecoratorPositionOnFrameChange();

	//rerun the text layout
	redoLabelTextLayout(m_usePrerenderedLabel);

	//reposition the text
	recalculateLabelPosition();

	//and the painter helpers
	recomputePainterHelpers();
}

//virtual
void IconBase::recomputePainterHelpers()
{
	m_iconFrameSrcPrecomputed = -m_iconFramePixmapGeom.translated(m_iconFramePosICS).topLeft();
	m_iconFeedbackSrcPrecomputed = -m_iconFeedbackPixmapGeom.translated(m_iconFeedbackPosICS).topLeft();
	m_iconSrcPrecomputed = -m_iconPixmapGeom.translated(m_iconPosICS).topLeft();
	m_labelSrcPrecomputed = -m_labelGeom.translated(m_labelPosICS).topLeft();
	m_removeDecoratorSrcPrecomputed = -m_removeDecoratorPixmapGeom.translated(m_removeDecoratorPosICS).topLeft();
	m_deleteDecoratorSrcPrecomputed = -m_deleteDecoratorPixmapGeom.translated(m_deleteDecoratorPosICS).topLeft();
	m_installStatusDecoratorSrcPrecomputed =
			-m_installStatusDecoratorPixmapGeom.translated(m_installStatusDecoratorPosICS).topLeft();

	//reset the currently-rendering decorator items
	switch (m_showWhichDeleteRemove)
	{
	case RemoveDeleteDecoratorSelector::Remove:
		m_showWhichDeleteRemove = RemoveDeleteDecoratorSelector::Remove;
		m_qp_drDecoratorCurrentlyRenderingPixmap = m_qp_removeDecoratorCurrentStatePixmap;
		m_drDecoratorCurrentPixmapGeom = m_removeDecoratorPixmapGeom;
		m_drDecoratorCurrentPosICS = m_removeDecoratorPosICS;
		m_drDecoratorCurrentSrcPrecomputed = m_removeDecoratorSrcPrecomputed;
		break;
	case RemoveDeleteDecoratorSelector::Delete:
		m_showWhichDeleteRemove = RemoveDeleteDecoratorSelector::Delete;
		m_qp_drDecoratorCurrentlyRenderingPixmap = m_qp_deleteDecoratorCurrentStatePixmap;
		m_drDecoratorCurrentPixmapGeom = m_deleteDecoratorPixmapGeom;
		m_drDecoratorCurrentPosICS = m_deleteDecoratorPosICS;
		m_drDecoratorCurrentSrcPrecomputed = m_deleteDecoratorSrcPrecomputed;
		break;
	default:	//None
		m_showWhichDeleteRemove = RemoveDeleteDecoratorSelector::None;
		m_qp_drDecoratorCurrentlyRenderingPixmap = 0;
		break;
	}


}

//static
bool IconBase::canUsePixOnIcon(const IconBase& icon,PixmapObject * p_pix)
{
	if (!p_pix)
		return false;
	if ( (p_pix->sizeF().width() > icon.m_geom.width()) || (p_pix->sizeF().height() > icon.m_geom.height()))
	{
		qDebug() << __PRETTY_FUNCTION__ << ": icon uid = "<< icon.uid() << " : REJECTING NEW PIXMAP (uid = " <<p_pix->id() << " , size = " << p_pix->sizeF() << ") FOR USE IN THE ICON!";
		return false;
	}
	return true;
}

//virtual
void	IconBase::redoLabelTextLayout(bool renderLabelPixmap)
{
	m_labelColor = IconGeometrySettings::settings()->labelFontColor;
	QString label = m_iconLabel;
	if (label.length() == 0)
	{
		delete m_qp_prerenderedLabelPixmap;
		m_qp_prerenderedLabelPixmap = 0;
		m_textLayoutObject.clearLayout();
		m_labelGeom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(m_labelMaxGeom.width(), m_labelMaxGeom.height())).toAlignedRect();
		return;
	}
	QTextOption textOpts;
	textOpts.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	textOpts.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	bool	complete = false;
	qreal	height;
	do {
		height = 0;
		m_textLayoutObject.clearLayout();
		m_textLayoutObject.setText(label);
		m_textLayoutObject.setFont(staticLabelFontForIcons());
		m_textLayoutObject.setTextOption(textOpts);
		m_textLayoutObject.beginLayout();
		int lineCount = 0;
		while (height < m_labelMaxGeom.height()) {
			QTextLine line = m_textLayoutObject.createLine();
			if (!line.isValid())
			{	// there isn't anymore text to add: we're done!
				complete = true;
				break;
			}
			++lineCount;
			line.setLineWidth(m_labelMaxGeom.width());
			line.setPosition(QPointF(0, height));
			height += line.height();
		}
		m_textLayoutObject.endLayout();
		if (!complete && label.length() > 1)
		{
			bool chopLabel = true;
			if (label == m_iconLabel && --lineCount > 0)	// we exited the loop because we had one too many line, reduce the usable line count by one, and make sure we have at least one...
			{	// the first time around, let's ask Qt for help: chop the string to the size of n lines... This will avoid looping too many times if the label is crazy long.
				QFontMetrics fm(staticLabelFontForIcons());
				label = fm.elidedText(label, Qt::ElideRight, m_labelMaxGeom.width() * lineCount);
				if (label != m_iconLabel)
					chopLabel = false;
				//g_message("Chopping '%s' to '%s' to fit in %d lines", m_iconLabel.toUtf8().data(), label.toUtf8().data(), lineCount);
			}
			if (chopLabel)
			{
				label.chop(2);	// remove ellipsis + 1 character, or two chars the first time around if elidedText did nothing.
				label.push_back(QChar(0x2026));	// '...', as a single unicode character
				//g_message("...to '%s'", label.toUtf8().data());
			}
		}
	} while (!complete && label.length() > 0);

	height = qMin((quint32)DimensionsGlobal::roundUp(height),(quint32)m_labelMaxGeom.height());
	height = DimensionsGlobal::roundDown(height) - (DimensionsGlobal::roundDown(height) % 2);	//force to an even #

	//TODO: PIXEL-ALIGN
	m_labelGeom = DimensionsGlobal::realRectAroundRealPoint(QSizeF(m_textLayoutObject.boundingRect().width(),height)).toAlignedRect();

	if(m_qp_prerenderedLabelPixmap && (m_qp_prerenderedLabelPixmap->width() != m_labelGeom.width() || m_qp_prerenderedLabelPixmap->height() != m_labelGeom.height())) {
		delete m_qp_prerenderedLabelPixmap;
		m_qp_prerenderedLabelPixmap = 0;
	}

	if(renderLabelPixmap) {
		if(!m_qp_prerenderedLabelPixmap) {
			m_qp_prerenderedLabelPixmap = new PixmapObject(m_labelGeom.width(), m_labelGeom.height());
		}

		m_qp_prerenderedLabelPixmap->fill(QColor(0,0,0,0));
		QPainter painter;
		painter.begin(m_qp_prerenderedLabelPixmap->data());
		painter.setPen(m_labelColor);
		m_textLayoutObject.draw(&painter, QPointF(0, 0));
		painter.end();
	}
}

////virtual
//void	IconBase::recalculateLabelPosition()
//{
//	m_labelPosICS = QPointF(m_iconPixmapGeom.center().x(),m_iconPixmapGeom.bottom() - (qreal)m_labelGeom.top());
//	QPointF offset = m_labelPosICS + m_labelMaxGeom.topLeft() - m_geom.topLeft();
//	offset += QPointF(DimensionsGlobal::centerRectInRectPixelAlign(m_labelMaxGeom,m_labelGeom));
//	m_labelPosPntCS = DimensionsGlobal::realPointAsPixelPosition(offset);
//}

//virtual
void	IconBase::recalculateLabelPosition()
{
	//reposition the text
	m_labelPosICS = QPoint(0,m_iconGeom.translated(m_iconPosICS).bottom()
			-m_labelGeom.top()
			+IconGeometrySettings::settings()->labelVerticalSpacingPx);

}

//virtual
void IconBase::recalculateDecoratorPositionOnFrameChange()
{
	if (IconGeometrySettings::settings()->useAbsoluteRemoveDeleteDecoratorOffsetFromGeomOrigin)
	{
		m_removeDecoratorPosICS = IconGeometrySettings::settings()->removeDeleteDecoratorOffsetFromGeomOriginPx;
		m_deleteDecoratorPosICS = IconGeometrySettings::settings()->removeDeleteDecoratorOffsetFromGeomOriginPx;
	}
	else if (!m_iconFramePixmapGeom.isEmpty())
	{
		m_removeDecoratorPosICS = m_iconFramePixmapGeom.translated(m_iconFramePosICS).topLeft();
		m_deleteDecoratorPosICS = m_iconFramePixmapGeom.translated(m_iconFramePosICS).topLeft();
	}
	else
	{
		m_removeDecoratorPosICS = m_iconFrameGeom.translated(m_iconFramePosICS).topLeft();
		m_deleteDecoratorPosICS = m_iconFrameGeom.translated(m_iconFramePosICS).topLeft();
	}

	if (IconGeometrySettings::settings()->useAbsoluteInstallStatusDecoratorOffsetFromGeomOrigin)
	{
		m_installStatusDecoratorPosICS = IconGeometrySettings::settings()->installStatusDecoratorOffsetFromGeomOriginPx;
	}
	else if (!m_iconFramePixmapGeom.isEmpty())
	{
		m_installStatusDecoratorPosICS = m_iconFramePixmapGeom.translated(m_iconFramePosICS).topLeft();
	}
	else
	{
		m_installStatusDecoratorPosICS = m_iconFrameGeom.translated(m_iconFramePosICS).topLeft();
	}
}
