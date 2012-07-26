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




#include "dimensionsglobal.h"
#include "pixmaphugeobject.h"
#include "gfxsettings.h"
#include <QPainter>
#include <QPointF>
#include <QImage>

PixmapHugeObject::PixmapHugeObject()
: m_arraySize(0,0)
, m_hugePixmapSize(0,0)
, m_valid(false)
{
}

PixmapHugeObject::PixmapHugeObject ( int width, int height )
: m_arraySize(0,0)
, m_hugePixmapSize(0,0)
, m_valid(false)
{
	//calculate how many pixmaps are needed
	QSize edgeSizes;
	qDebug() << __FUNCTION__ << " max pix size: " << GraphicsSettings::DiUiGraphicsSettings()->maxPixSize;
	m_arraySize = pixmapToArraySize(QSize(width,height),GraphicsSettings::DiUiGraphicsSettings()->maxPixSize,edgeSizes);
	if ((m_arraySize.width() == 0) || (m_arraySize.height() == 0))
	{
		return;
	}

	allocatePixmaps(m_arraySize,GraphicsSettings::DiUiGraphicsSettings()->maxPixSize,edgeSizes);

}

PixmapHugeObject::PixmapHugeObject ( const QString & fileName, const char * format, Qt::ImageConversionFlags flags)
: m_arraySize(0,0)
, m_hugePixmapSize(0,0)
, m_valid(false)
{
	//load using QImage, so that (hopefully!) the software side of things does the loading.
	//fileName is expected to be an image file containing an image larger than max pix size, so it would
	// most likely fail with a QPixmap for the same reason why PixmapHugeObject is needed in the first place
	//flags are ignored for now...mostly here just to maintain signature compat with the other pixmap loading fns

	QImage img = QImage(fileName,format);
	if (img.isNull())
	{
		//invalid image
		return;
	}

	//determine size needed (array layout of my hugepixmap)
	QSize edgeSizes =  QSize(0,0);
	m_arraySize = pixmapToArraySize(QSize(img.width(),img.height()),
									GraphicsSettings::DiUiGraphicsSettings()->maxPixSize,edgeSizes);

	if ((m_arraySize.width() == 0) || (m_arraySize.height() == 0))
	{
		return;
	}
	//create the pixmaps
	allocatePixmaps(m_arraySize,GraphicsSettings::DiUiGraphicsSettings()->maxPixSize,edgeSizes);

	//paint onto the pixmaps
	copyToPixmaps(img);
}

PixmapHugeObject::PixmapHugeObject ( const QList<QString>& fileNames, const QSize& fileLayout,const char * format, Qt::ImageConversionFlags flags)
: m_arraySize(0,0)
, m_hugePixmapSize(0,0)
, m_valid(false)
{
	//TODO: unimplemented
	// it's not trivial since any filename in the list might be larger than max filesize, so it would result in sort of a
	// "recursive" hugepixmap. Logic needs to be in place to resize/scale if necessary so that the resulting tile-layout
	// of pixmaps is symmetric

}

PixmapHugeObject::PixmapHugeObject ( const QList<QPixmap>& pixmaps, const QSize& layout)
: m_arraySize(0,0)
, m_hugePixmapSize(0,0)
, m_valid(false)
{
	//TODO: unimplemented.  See above
}

//virtual
PixmapHugeObject::~PixmapHugeObject()
{
	//this signal is sensitive to where it is placed, since upstream receivers might access things in this object
	//due to this signal...
	// must make sure the base class destructor doesn't
	Q_EMIT signalObjectDestroyed();
	m_destroyEmitted = true;
	for (int i = 0;i<m_pixmaps.size();++i)
	{
		delete m_pixmaps[i];
	}
}

//virtual
bool PixmapHugeObject::valid() const
{
	return m_valid;
}

//virtual
QSize PixmapHugeObject::size() const
{
	return m_hugePixmapSize;
}

//virtual
int PixmapHugeObject::width() const
{
	return m_hugePixmapSize.width();
}

//virtual
int PixmapHugeObject::height() const
{
	return m_hugePixmapSize.height();
}

//virtual
QSizeF PixmapHugeObject::sizeF() const
{
	return QSizeF(m_hugePixmapSize);
}

//virtual
quint64 PixmapHugeObject::sizeOf() const
{
	if (!valid())
	{
		return 0;
	}
	// all pixmaps are the same Bpp (depth) in this version of pixmaphugeobject
	return (m_hugePixmapSize.width() * m_hugePixmapSize.height() * (quint32)m_pixmaps.at(0)->depth());
}

//virtual
bool PixmapHugeObject::isSquare() const
{
	return (m_hugePixmapSize.width() == m_hugePixmapSize.height());
}

///NOTE: none of the paint() functions check validity, for speed sake

//virtual
void PixmapHugeObject::paint(QPainter * painter)
{
	//paint at the current painter origin, all of the pixmaps
	QPoint destPoint = QPoint(0,0);
	for (int y=0;y<m_arraySize.height();++y)
	{
		int base = y*m_arraySize.width();
		for (int x=0;x<m_arraySize.width();++x)
		{
			painter->drawPixmap(destPoint,*(m_pixmaps[base+x]));
			destPoint.setX(destPoint.x()+m_pixmaps[base+x]->width());
		}
		destPoint.setX(0);
		destPoint.setY(destPoint.y()+m_pixmaps[base]->height());
	}
}

//virtual
void PixmapHugeObject::paint(QPainter * painter,const QPointF& targetOriginInPainterCS)
{
	QPoint destPoint = DimensionsGlobal::realPointAsPixelPosition(targetOriginInPainterCS);
	for (int y=0;y<m_arraySize.height();++y)
	{
		int base = y*m_arraySize.width();
		for (int x=0;x<m_arraySize.width();++x)
		{
			painter->drawPixmap(destPoint,*(m_pixmaps[base+x]));
			destPoint.setX(destPoint.x()+m_pixmaps[base+x]->width());
		}
		destPoint.setX(0);
		destPoint.setY(destPoint.y()+m_pixmaps[base]->height());
	}
}

//virtual
void PixmapHugeObject::paint(QPainter * painter,const QRectF& targetRectInPainterCS)
{
	//TODO: unimplemented
	// if targetRectInPainterCS != m_hugePixmapSize, then need to scale each pixmap in the array
	// so that the the final stitched pixmap fits into the target rect

}

//virtual
void PixmapHugeObject::paint(QPainter * painter,const QRect& targetRectInPainterCS,
										  const QRect& sourceRect)
{
	QSize normalTileSize = m_pixmapRectCoords[0].size();		//ASSUMPTION:  all tile sizes except right,bottom edges are the same
	//TODO: INCOMPLETE disregards the size of targetRect...it won't scale yet...so it better be 1:1 size
	//need to find start pixmap at top left
	qint32 sx = sourceRect.left() / normalTileSize.width();
	qint32 sy = sourceRect.top() / normalTileSize.height();

	QRect currentPixRect;
	QPoint targetPoint = targetRectInPainterCS.topLeft();
	int targetAreaLeft = targetPoint.x();
	for (qint32 y=sy;y<m_arraySize.height();++y)
	{
		int base = y*m_arraySize.width();
		int pc=0;
		QRect sourcePixRect;

		for (qint32 x=sx;x<m_arraySize.width();++x)
		{
			//intersect the sourceRect and this pixmap's rect...this is the paint area
			QRect intersectRect = sourceRect.intersect(m_pixmapRectCoords[base+x]);
			if (intersectRect.isEmpty())
			{
				break;
			}
			++pc;
			sourcePixRect = intersectRect.translated(-(m_pixmapRectCoords[base+x].topLeft()));
			painter->drawPixmap(targetPoint,*(m_pixmaps[base+x]),sourcePixRect);
			targetPoint.setX(targetPoint.x()+sourcePixRect.width());
		}
		if (!pc)
		{
			break;	//no paint last horiz sweep; no more vertical intersect rects either, then...done.
		}
		targetPoint.setX(targetAreaLeft);
		targetPoint.setY(targetPoint.y()+sourcePixRect.height());		//use the last sourcePixRect that was used in the inner loop;
																		// again, makes ASSUMPTION on tile sizes in a row being ==
	}
}

//virtual
QPoint PixmapHugeObject::translatePaintTargetPointToPixmapPoint(const QPoint& point,const QRect& sourceRect,const QRect& destRect)
{
	//TODO: INCOMPLETE - it won't use the source or dest rect sizes (i.e. uses 1x scale)
	//			see PixmapHugeObject::paint(QPainter * painter,const QRect& targetRectInPainterCS,const QRect& sourceRect)

	// this variant doesn't have a way to return the index of the pixmap so it just returns the point in the "hugespace"
	//		(i.e. as if the hugepixmap was one large pixmap)

	QSize normalTileSize = m_pixmapRectCoords[0].size();		//ASSUMPTION:  all tile sizes except right,bottom edges are the same

	qint32 sx = point.x() / normalTileSize.width();
	qint32 sy = point.y() / normalTileSize.height();

	//qDebug() << "sourceRect: " << sourceRect << " , destRect: " << destRect;
	QTransform tran = QTransform().scale( ((qreal)sourceRect.width()) / (qreal)(destRect.width()) , ((qreal)sourceRect.height()) / (qreal)(destRect.height()))
										.translate( (qreal)(sourceRect.center().x() - destRect.center().x()) , (qreal)(sourceRect.center().y() - destRect.center().y()) );
	QPoint translatedPt = tran.map(point);
	return translatedPt;
}

//virtual
QVector<qint32> PixmapHugeObject::translatePaintTargetPointToPixmapPointEx(const QPoint& point,const QRect& sourceRect,const QRect& destRect)
{
	//TODO: INCOMPLETE - it won't use the source or dest rects (i.e. uses 1x scale and no translation).
	//			see PixmapHugeObject::paint(QPainter * painter,const QRect& targetRectInPainterCS,const QRect& sourceRect)

	//this variant can return more information. See .h file for details -- repeated here for convenience
	// vec[0] = point.x in hugespace , [1] = point.y in hugespace , [2] = point.x on local pixmap , [3] = point.y on local pixmap , [4] = index of local pixmap
	QSize normalTileSize = m_pixmapRectCoords[0].size();		//ASSUMPTION:  all tile sizes except right,bottom edges are the same

	qint32 sx = point.x() / normalTileSize.width();
	qint32 sy = point.y() / normalTileSize.height();


	QTransform tran = QTransform().scale( ((qreal)sourceRect.width()) / (qreal)(destRect.width()) , ((qreal)sourceRect.height()) / (qreal)(destRect.height()))
										.translate( (qreal)(destRect.center().x() - sourceRect.center().x()) , (qreal)(destRect.center().y() - sourceRect.center().y()) );
	QPoint translatedPt = tran.map(point);

	QPoint translatedLocalPt = translatedPt - m_pixmapRectCoords[sy*(m_arraySize.width())+sx].topLeft();
	return (QVector<qint32>() 	<< translatedPt.x() << translatedPt.y()
								<< translatedLocalPt.x() << translatedLocalPt.y()
								<< (sy*(m_arraySize.width())+sx));
}

QVector<int> PixmapHugeObject::hugeSpaceCoordinatesOfPoint(const QPoint& pointInAbsolutePainterCS)
{
	//TODO: unimplemented
	return QVector<int>();
}

QVector<int> PixmapHugeObject::hugeSpaceCoordinatesOfPoint(const int x,const int y)
{
	//TODO: unimplemented
	return QVector<int>();
}

QVector<PixmapHugeObject::FragmentedPaintCoordinate> PixmapHugeObject::paintCoordinates(const QRect& rectInAbsolutePainterCS)
{
	/*
	 * return values:
	 *
	 * FragmentedPaintCoordinate is a "paint command" telling a caller that would ordinarily have done a single drawPixmap into a
	 * single pixmap, how to paint into a hugepixmap
	 *
	 * pixmapIndex:
	 * 		which pixmap in the m_pixmaps list of pixmaps the coordinates herein refer to
	 *
	 * sourceRect:
	 * 		a subrect of the parameter, rectInAbsolutePainterCS. It tells the caller which piece of the total rect to be painted (which
	 * 		is what the param is), is to be painted onto this index pixmap
	 *
	 * targetRec:
	 *		a subrect of 0,0->m_pixmapRectCoords[index].size(), which tells the caller where in the hugepixmap's individual pixmap tile
	 *		to paint the source specified by sourceRect
	 *
	 *  it's implicit that sourceRect.size == targetRect.size
	 *
	 *
	 *
	 */
	//assumption: tile index 0 (m_pixmaps[0] has a rect thats the size of all the other "normal" rects (and is >= the right and bottom edge rects)
	quint32 absXstart = ( rectInAbsolutePainterCS.topLeft().x() < 0 ? 0 : rectInAbsolutePainterCS.topLeft().x());
	quint32 absYstart = ( rectInAbsolutePainterCS.topLeft().y() < 0 ? 0 : rectInAbsolutePainterCS.topLeft().y());
	quint32 arrayXstart = (absXstart/ m_pixmapRectCoords[0].size().width());
	quint32 arrayYstart = (absYstart / m_pixmapRectCoords[0].size().height()) * m_arraySize.width();
	quint32 indexStart = (absYstart / m_pixmapRectCoords[0].size().height()) * m_arraySize.width() + (absXstart/ m_pixmapRectCoords[0].size().width());

	quint32 absXend = ( rectInAbsolutePainterCS.bottomRight().x() < 0 ? absXstart : rectInAbsolutePainterCS.bottomRight().x());
	quint32 absYend = ( rectInAbsolutePainterCS.bottomRight().y() < 0 ? absYstart : rectInAbsolutePainterCS.bottomRight().y());
	quint32 arrayXend = (absXend/ m_pixmapRectCoords[0].size().width());
	quint32 arrayYend = (absYend / m_pixmapRectCoords[0].size().height()) * m_arraySize.width();
	quint32 indexEnd = (absYend / m_pixmapRectCoords[0].size().height()) * m_arraySize.width() + (absXend/ m_pixmapRectCoords[0].size().width());

	QVector<PixmapHugeObject::FragmentedPaintCoordinate> coords;
	if (indexStart == indexEnd)
	{
		//all inside same tile
		QRect targetRect = rectInAbsolutePainterCS.intersect(m_pixmapRectCoords[indexStart]).translated(-(m_pixmapRectCoords[indexStart].topLeft()));
		QRect sourceRect = rectInAbsolutePainterCS.intersect(m_pixmapRectCoords[indexStart]);

		coords << PixmapHugeObject::FragmentedPaintCoordinate(indexStart,
															sourceRect,
															targetRect);
		return coords;
	}
	//spans tiles
	for (quint32 y=arrayYstart;y<=arrayYend;++y)
	{
		quint32 base = y*m_arraySize.width();
		for (quint32 x=arrayXstart;x<=arrayXend;++x)
		{
			quint32 index = base+x;
			//intersect the rect with the tile rect
			QRect targetRect = rectInAbsolutePainterCS.intersect(m_pixmapRectCoords[index]).translated(-(m_pixmapRectCoords[index].topLeft()));
			QRect sourceRect = rectInAbsolutePainterCS.intersect(m_pixmapRectCoords[index]);
			coords << PixmapHugeObject::FragmentedPaintCoordinate(index,
													sourceRect,
													targetRect);
		}
	}
	return coords;
}

//virtual
QPixmap * PixmapHugeObject::pixAt(const quint32 index) const
{
	if (m_pixmaps.empty())
	{
		return 0;
	}
	return m_pixmaps.at(qBound((quint32)0,index,(quint32)(m_pixmaps.size()-1)));
}

//virtual
QPixmap* PixmapHugeObject::operator->() const
{
	return static_cast<QPixmap*>(const_cast<QPixmap*>(m_pixmaps[0]));
}

//virtual
QPixmap& PixmapHugeObject::operator*() const
{
	return *static_cast<QPixmap*>(const_cast<QPixmap*>(m_pixmaps[0]));
}

//virtual
PixmapHugeObject::operator QPixmap*() const
{
	return static_cast<QPixmap*>(const_cast<QPixmap*>(m_pixmaps[0]));
}

//virtual
QPixmap* PixmapHugeObject::data() const
{
	return static_cast<QPixmap*>(const_cast<QPixmap*>(m_pixmaps[0]));
}

//static
QSize PixmapHugeObject::pixmapToArraySize(const QSize& pixmapSize,const QSize& maxSinglePixmapSize)
{
	if ((maxSinglePixmapSize.width() <= 0) || (maxSinglePixmapSize.height() <= 0))
	{
		return QSize(0,0);
	}
	quint32 w = qAbs(pixmapSize.width() / maxSinglePixmapSize.width())
					+ (qAbs(pixmapSize.width()) % qAbs(maxSinglePixmapSize.width()) ? 1 : 0);
	quint32 h = qAbs(pixmapSize.height() / maxSinglePixmapSize.height())
						+ (qAbs(pixmapSize.height()) % qAbs(maxSinglePixmapSize.height()) ? 1 : 0);
	return QSize(w,h);
}

//static
QSize PixmapHugeObject::pixmapToArraySize(const QSize& pixmapSize,const QSize& maxSinglePixmapSize,QSize& r_rightAndBottomEdgeSizes)
{
	if ((maxSinglePixmapSize.width() <= 0) || (maxSinglePixmapSize.height() <= 0))
	{
		return QSize(0,0);
	}
	quint32 w = qAbs(pixmapSize.width() / maxSinglePixmapSize.width())
								+ (qAbs(pixmapSize.width()) % qAbs(maxSinglePixmapSize.width()) ? 1 : 0);
	quint32 h = qAbs(pixmapSize.height() / maxSinglePixmapSize.height())
									+ (qAbs(pixmapSize.height()) % qAbs(maxSinglePixmapSize.height()) ? 1 : 0);

	r_rightAndBottomEdgeSizes.setWidth(qAbs(pixmapSize.width()) % qAbs(maxSinglePixmapSize.width()));
	r_rightAndBottomEdgeSizes.setHeight(qAbs(pixmapSize.height()) % qAbs(maxSinglePixmapSize.height()));
	return QSize(w,h);
}

///protected:

//DANGEROUS VARIANT! Assumes ownership of pixmaps; must never be pixmaps on the stack!
PixmapHugeObject::PixmapHugeObject (const QVector<QPixmap *> p_pixmaps,const QSize& layout)
: m_arraySize(0,0)
, m_hugePixmapSize(0,0)
, m_valid(false)
{
	if (p_pixmaps.size() < (layout.width()*layout.height())
			|| (p_pixmaps.size() == 0) || (layout.width() == 0)
			|| (layout.height() == 0))
	{
		return;
	}

	m_pixmaps = p_pixmaps;
	m_arraySize = layout;

	//total pixmap size - of all the pixmaps stitched together - is calculated by iterating according to layout
	// the only correct configuration is to have pixmaps of the same size throughout, except possibly the right and
	// bottom edges (see pixmapToArraySize() ).
	// violating this may lead to unexpected results when painting

	int w = 0;
	int h = 0;
	for (int y=0;y<layout.height();++y)
	{
		h += p_pixmaps[y*layout.width()]->height();
	}
	for (int x=0;x<layout.width();++x)
	{
		w += p_pixmaps[x]->width();
	}
	m_hugePixmapSize = QSize(w,h);
	QPoint srcPoint = QPoint(0,0);
	for (int y=0;y<m_arraySize.height();++y)
	{
		int base = y*m_arraySize.width();
		for (int x=0;x<m_arraySize.width();++x)
		{
			m_pixmapRectCoords[base+x] = QRect(srcPoint,QSize(m_pixmaps[base+x]->width(),m_pixmaps[base+x]->height()));
			srcPoint.setX(srcPoint.x()+m_pixmaps[base+x]->width());
		}
		srcPoint.setX(0);
		srcPoint.setY(srcPoint.y()+m_pixmaps[base]->height());
	}
}

//virtual
void PixmapHugeObject::allocatePixmaps(const QSize& arraySize,const QSize& normalSize,const QSize& leftoverSize)
{
	//(no error checks)
	QSize s;
	QPoint srcPoint = QPoint(0,0);
	for (int y=0;y<arraySize.height();++y)
	{
		if ( arraySize.height() == 1)
		{
			s.setHeight(normalSize.height());
		}
		else if (y == arraySize.height()-1)
		{
			s.setHeight(leftoverSize.height());
		}
		else
		{
			s.setHeight(normalSize.height());
		}
		for (int x=0;x<arraySize.width();++x)
		{
			if ( arraySize.width() == 1)
			{
				s.setWidth(normalSize.width());
			}
			else if (x == arraySize.width()-1)
			{
				s.setWidth(leftoverSize.width());
			}
			else
			{
				s.setWidth(normalSize.width());
			}
			QPixmap * pPm = new QPixmap(s);
			pPm->fill(Qt::transparent);
			m_pixmaps << pPm;
			m_pixmapRectCoords << QRect(srcPoint,s);
			srcPoint.setX(srcPoint.x()+s.width());
		}
		srcPoint.setX(0);
		srcPoint.setY(srcPoint.y()+s.height());
	}
	m_hugePixmapSize = QSize((arraySize.width()-1)*normalSize.width() + leftoverSize.width(),
							(arraySize.height()-1)*normalSize.height() + leftoverSize.height());
}

//virtual
void PixmapHugeObject::copyToPixmaps(const QImage& sourceImage)
{
	//uses the (hopefully) initialized member variables
	QPoint destPoint = QPoint(0,0);
	for (int y=0;y<m_arraySize.height();++y)
	{
		int base = y*m_arraySize.width();
		for (int x=0;x<m_arraySize.width();++x)
		{
			QPainter painter(m_pixmaps[base+x]);
			painter.drawImage(destPoint,sourceImage,
					QRect(destPoint.x(),destPoint.y(),m_pixmaps[base+x]->width(),m_pixmaps[base+x]->height()));
			destPoint.setX(destPoint.x()+m_pixmaps[base+x]->width());
		}
		destPoint.setX(0);
		destPoint.setY(destPoint.y()+m_pixmaps[base]->height());
	}
}

/////////////////////DEBUG://///////////// DEBUG FUNCTIONS /////////////////////////////////////

void PixmapHugeObject::dbg_save(const QString& baseName)
{
	for (int i=0;i<m_pixmaps.size();++i)
	{
		m_pixmaps[i]->save(baseName+QString("%1").arg(i)+".png");
	}
}

QDebug operator<<(QDebug dbg, const PixmapHugeObject& c)
{
	//MINIMAL printouts...just the size and rect coords; no pixmap data
	dbg.nospace() << "\narraySize: " << c.m_arraySize
			<< "\nhugeSize: " << c.m_hugePixmapSize
			<< "\nrects: ";
	for (int y=0;y<c.m_arraySize.height();++y)
	{
		QString row;
		int base = y*c.m_arraySize.width();
		for (int x=0;x<c.m_arraySize.width();++x)
		{
			dbg.nospace() << "index "<<base+x << ": " << c.m_pixmapRectCoords[base+x];
		}
	}
	return dbg.space();
}

QDebug operator<<(QDebug dbg, const PixmapHugeObject::FragmentedPaintCoordinate &c)
{
	dbg.nospace() << c.pixmapIndex << " , " << c.sourceRect << " , " << c.targetRect;
	return dbg.space();
}
