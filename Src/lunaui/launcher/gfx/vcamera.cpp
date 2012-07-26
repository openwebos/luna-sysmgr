/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#include "vcamera.h"
#include "thingpaintable.h"
#include "pixmapobject.h"
#include "pixmaphugeobject.h"
#include "dimensionsglobal.h"
#include "thingpaintable.h"
#include "dimensionslauncher.h"
#include "gfxsettings.h"
#include "expblur.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPainter>
#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
#include <QGLContext>
#include <QGLFramebufferObject>
#endif
#include <QDebug>

////public:
VirtualCamera::VirtualCamera(LauncherObject * p_mainUi)
: m_p_mainUi(p_mainUi)
, m_triggerAble(false)
{
}

//virtual
VirtualCamera::~VirtualCamera()
{
}

//virtual
PixmapObject * VirtualCamera::lastImageCaptured(QRectF * r_pViewRect)
{
	if (r_pViewRect)
	{
		*r_pViewRect = m_viewport;
	}

	PixmapObject * pPmo = 0;
	if (m_img.width() > GraphicsSettings::settings()->maxPixSize.width()
			|| m_img.height() > GraphicsSettings::settings()->maxPixSize.height())
	{
		pPmo = new PixmapHugeObject(m_img.width(),m_img.height());
		((PixmapHugeObject *)pPmo)->copyToPixmaps(platformCompatibleImage());
	}
	else
	{
		pPmo = new PixmapObject(new QPixmap(QPixmap::fromImage(platformCompatibleImage())));
	}
	return pPmo;
}

//virtual
PixmapObject * VirtualCamera::lastImageCapturedBlurred(QRectF * r_pViewRect)
{
	if (r_pViewRect)
	{
		*r_pViewRect = m_viewport;
	}

	PixmapObject * pPmo = 0;
	if (m_img.width() > GraphicsSettings::settings()->maxPixSize.width()
			|| m_img.height() > GraphicsSettings::settings()->maxPixSize.height())
	{
		pPmo = new PixmapHugeObject(m_img.width(),m_img.height());
		((PixmapHugeObject *)pPmo)->copyToPixmaps(
						convertToPlatformCompatibleImage(BlurExponential::blurImage(m_img,5)));
	}
	else
	{
		pPmo = new PixmapObject(
				new QPixmap(QPixmap::fromImage(
						convertToPlatformCompatibleImage(BlurExponential::blurImage(m_img,5)))));
	}
	return pPmo;
}

//virtual
void VirtualCamera::setup(const QRectF& sceneViewport)
{
	return setup(NULL,QList<QPointer<ThingPaintable> >(),sceneViewport);
}

//virtual
void VirtualCamera::setup(const QList<QPointer<ThingPaintable> >& exclusionList,const QRectF& sceneViewport)
{
	return setup(NULL,exclusionList,sceneViewport);
}

//virtual
void VirtualCamera::setup(const QGraphicsItem * farBoundItem,const QList<QPointer<ThingPaintable> >& exclusionList,
							const QRectF& sceneViewport)
{
	m_triggerAble = false;
	//grab the scene pointer, or bail if not available
	QGraphicsScene * pScene = DimensionsGlobal::globalGraphicsScene();
	if (!pScene)
	{
		qDebug() << __FUNCTION__ << ": global graphics scene not available";
		return;
	}

	QList<QGraphicsItem *> rawItems;
	QSize rectSize;
	//if the sceneViewport isn't empty, then just get the items that are w/in it
	// use Qt::IntersectsItemBoundingRect to speed things up; this doesn't have to be exact, since the actual render by
	// trigger() will have to deal with source regions anyways
	if (!sceneViewport.isEmpty())
	{
		rectSize = sceneViewport.size().toSize();
		if ((rectSize.width() == 0) || (rectSize.height() == 0))
		{
			//would cause problems later
			m_viewport = QRectF();
			return;
		}
		m_viewport = sceneViewport;
		rawItems = pScene->items(sceneViewport,Qt::IntersectsItemBoundingRect,Qt::AscendingOrder);
		//SHOULD be ok to convert toSize(), w.r.t. trunc-ing off decimal bits and getting a smaller whole size
		// painter will have to do a similar thing so it SHOULD be compatible.
		// if things crash, look here first!
	}
	else //else get everything
	{
		rectSize = pScene->sceneRect().size().toSize();
		if ((rectSize.width() == 0) || (rectSize.height() == 0))
		{
			m_viewport = QRectF();
			return;
		}
		m_viewport = pScene->sceneRect();
		rawItems = pScene->items(pScene->sceneRect(),Qt::IntersectsItemBoundingRect,Qt::AscendingOrder);
	}

	//re-init the excludes
	m_excludes = QSet<QPointer<ThingPaintable> >::fromList(exclusionList);
	//clear the subjects
	m_subjects.clear();

	//the list is in ascending stack order. Toss out anything:
	// 1. below the farBoundItem
	// 2. not a ThingPaintable
	// 3. in the exclusion list

	//can't directly use "visible" as a determinant; the scene *can* provide an indicator of whether or not an item is occluded.
	// However, tossing out items by 1,2,3, above may actually reveal an item that was previously occluded, so more complicated logic
	// would be needed. It's ok though. The likelihood of it giving a significant reduction is minimal against the expense to
	// determine it.

	bool farItemReached = (farBoundItem == 0 ? true : false);		//if no far item was specified then set as already reached
	for (QList<QGraphicsItem *>::iterator it = rawItems.begin();
			it != rawItems.end();++it)
	{
		QGraphicsItem * pItem = *it;
		if ((!pItem) || (!farItemReached))
		{
			continue;
		}
		if (pItem == farBoundItem)
		{
			farItemReached = true;
		}
		ThingPaintable * pThingPaintable = ThingPaintable::thingpaintable_cast(pItem);
		if (!pThingPaintable)
		{
			continue;
		}
		if (m_excludes.contains(QPointer<ThingPaintable>(pThingPaintable)))
		{
			qDebug() << __FUNCTION__ << ": excluding thingpaintable [" << pThingPaintable->objectName() << "]";
			continue;
		}
		//ok it made it through the gauntlet...append to subjects list
		m_subjects << QPointer<ThingPaintable>(pThingPaintable);
	}

	//if the subjects list contains anything, then set up a pixmapobject...
	if (m_subjects.isEmpty())
	{
		//nada...bail
		m_viewport = QRectF();
		return;
	}

	setupBackingFb(pScene->sceneRect().size().toSize());
	m_triggerAble = true;
}

///public Q_SLOTS:

//virtual
void VirtualCamera::trigger()
{
	//if either the size is 0,0 or the pixmap is null, then setup hasn't run (successfully) so bail
//	if ((m_viewport.isEmpty()) || (!m_qp_pix))
//	{
//		qDebug() << __FUNCTION__ << ": failed. Re-run setup()";
//		return;
//	}
	if (!m_triggerAble)
	{
		qDebug() << __FUNCTION__ << ": failed. Re-run setup()";
		return;
	}

	QSize psize;
	if (GraphicsSettings::settings()->useFixedVCamPixSize)
	{
		psize = GraphicsSettings::settings()->fixedVCamPixSize;
	}
	else
	{
		psize = QSize(qMin((quint32)GraphicsSettings::settings()->maxVCamPixSize.width(),(quint32)m_viewport.width()),
						qMin((quint32)GraphicsSettings::settings()->maxVCamPixSize.height(),(quint32)m_viewport.height()));
	}

	qDebug() << "Vcam using output size of " << psize;
	qDebug() << "Viewport is: " << m_viewport;

	qreal Xscale = (GraphicsSettings::settings()->vCamHorizontalFlip ? -1.0 : 1.0);
	qreal Yscale = (GraphicsSettings::settings()->vCamVerticalFlip ? -1.0 : 1.0);

#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
		QGLContext* gc = (QGLContext*) QGLContext::currentContext();
		if (!gc)
		{
			qDebug() << __FUNCTION__ << ": couldn't get QGL context. exiting.";
			return;
		}

		QGLFramebufferObject fbo(psize.width(),psize.height());
		fbo.bind();

		QPainter painter(&fbo);
		painter.scale(Xscale,Yscale);
		render(&painter);
		painter.end();

		m_img = fbo.toImage();
		fbo.release();
#else

	QPixmap pix(psize.width(),psize.height());
	QPainter painter(&pix);
	painter.scale(Xscale,Yscale);
	render(&painter);
	painter.end();

	m_img = pix.toImage();

#endif

	if (GraphicsSettings::settings()->dbgSaveVcamOutput)
	{
		m_img.save(QString("/media/internal/camerasave.png"),0,100);
	}
}

//virtual
void VirtualCamera::render(QPainter * p_painter)
{
	//go through the list, and paint everything in order, giving it the viewport rect mapped to the item rect as the parameter to
	// the special thingpaintable's paint(painter,srcrect)...AND pre-applying each item's full transform to the painter before it
	// goes.

	//the main ui first
	if (m_p_mainUi)
	{
		p_painter->setTransform(m_p_mainUi->sceneTransform());
		m_p_mainUi->paint(p_painter);
	}
	for (QList<QPointer<ThingPaintable> >::iterator it = m_subjects.begin();
			it != m_subjects.end();++it)
	{
		ThingPaintable * pTp = *it;
		if (!pTp)
		{
			continue;	//item was probably deleted in the meantime since setup()
		}
		p_painter->setTransform(pTp->sceneTransform());
		//pTp->paint(&localPainter,pTp->mapRectFromScene(m_viewport));
		pTp->paint(p_painter);
	}
}

////protected:
//virtual
void VirtualCamera::setupBackingFb(const QSize& fbsize)
{
	//TODO: IMPROVE:
	// currently the normal paint path will be used in ThingPaintable-s, which don't know that they will
	// paint to a pixmap. Thus, a full scene rect size pixmap has to exist, so that they can paint in their native coords
	// When I bring paintOffscreen() functions back - they're currently very broken - then this will be able to be optimized
	// Caveat: if the scene rect is > 1024 in any dimension (w,h), then this will fail badly (opengl thing)
}

//TODO: BROKEN-OUT (not broken =) ) for future convenience
//static
QImage VirtualCamera::convertToPlatformCompatibleImage(const QImage& img)
{
#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
	return img;
#else
	return img;
#endif
}

QImage VirtualCamera::platformCompatibleImage()
{
#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
	return m_img;
#else
	return m_img;
#endif

}

