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




#ifndef VCAMERA_H_
#define VCAMERA_H_

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QList>
#include <QSet>
#include <QImage>

class ThingPaintable;
class PixmapObject;
class QGraphicsItem;
class LauncherObject;
class VirtualCamera : public QObject
{
	Q_OBJECT
public:
	VirtualCamera(LauncherObject * p_mainUi);
	virtual ~VirtualCamera();

	virtual PixmapObject * lastImageCaptured(QRectF * r_pViewRect=0);
	virtual PixmapObject * lastImageCapturedBlurred(QRectF * r_pViewRect=0);

	virtual void setup(const QRectF& sceneViewport = QRectF());
	virtual void setup(const QList<QPointer<ThingPaintable> >& exclusionList,const QRectF& sceneViewport = QRectF());
	virtual void setup(const QGraphicsItem * farBoundItem,const QList<QPointer<ThingPaintable> >& exclusionList,const QRectF& sceneViewport = QRectF());

	static QImage convertToPlatformCompatibleImage(const QImage& img);

public Q_SLOTS:

	virtual void trigger();

protected:

	virtual void setupBackingFb(const QSize& fbsize);
	virtual void render(QPainter * p_painter);

	QImage platformCompatibleImage();

	//TODO: TEMP: the main ui (DimensionsUi). It's not a ThingPaintable since it derives Window and I don't want to do multi-inheritance
	// there are a few good ways around this, but I just want to do proof-of-concept for now so i'm hacking this in as-is
	// the assumption is that this comes in front of all 'subjects', stacking wise
	LauncherObject * m_p_mainUi;
	QList<QPointer<ThingPaintable> > m_subjects;
	QSet<QPointer<ThingPaintable> > m_excludes;
	bool m_triggerAble;
	QRectF m_viewport;
	QImage m_img;
};

#endif /* VCAMERA_H_ */
