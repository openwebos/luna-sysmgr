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




#include "IMEPixmap.h"
#include "Settings.h"
#include "Logging.h"
#include "Utils.h"
#include "SingletonTimer.h"
#include "GlyphCache.h"

#include <QPainter>

#define CACHE_PIXMAP 1
#define DELAY_PURGE_PIXMAP 1

std::vector<IMEPixmap*>	IMEPixmap::s_PalmPixmaps;

const char * IMEPixmap::s_defaultLocation = NULL;

QPixmap &	IMEPixmap::pixmap()
{
	if (m_pixmap.isNull())
	{
		if (*m_name == '/')
			m_pixmap.load(m_name);
		else if (s_defaultLocation)
		{
			std::string path = Settings::LunaSettings()->lunaSystemResourcesPath + '/' + s_defaultLocation + '/' + m_name;
			m_pixmap.load((Settings::LunaSettings()->lunaSystemResourcesPath + '/' + s_defaultLocation + '/' + m_name).c_str());
		}
		else
			m_pixmap.load((Settings::LunaSettings()->lunaSystemResourcesPath + "/keyboard/" + m_name).c_str());
		if (m_pixmap.isNull())
			FAILURE(string_printf("IMEPixmap failed to load keyboard asset '%s'", m_name).c_str());
	}
	return m_pixmap;
}

NineTileSprites::~NineTileSprites()
{
	PixmapCache::instance().dispose(m_pixmap);
}

void NineTileSprites::reserve(bool start)
{
	if (start)
	{
		m_sprites.clear();
		m_size = QSize(0, 0);
	}
	else
	{
#if CACHE_PIXMAP
		if (!m_pixmap || m_pixmap->width() != m_size.width() || m_pixmap->height() < m_size.height() || (m_pixmap->height() - m_size.height()) * m_pixmap->width() > 150 * 1024)
#else
		if (!m_pixmap || m_pixmap->size() != m_size)
#endif
		{
			PixmapCache::instance().dispose(m_pixmap);
			m_pixmap = PixmapCache::instance().get(m_size);
		}
		m_pixmap->fill(QColor(0, 0, 0, 0));
		m_size = QSize(0, 0);
		m_sprites.clear();
	}
}

void NineTileSprites::reserve(QSize size, int count, QPixmap & pixmap)
{
	if (cache(count))
	{
		int & pos = m_sprites[SpriteRef(pixmap, size)];
		if (pos < 0)
		{
			pos = m_size.width();
			m_size = m_size.expandedTo(QSize(m_size.width() + size.width(), size.height() * 2));
		}
	}
}

void NineTileSprites::draw(QPainter & painter, const QRect & location, QPixmap & pixmap, bool pressed, int count, const NineTileCorner & corner)
{
	int width = location.width();
	int height = location.height();
	int & pos = m_sprites[SpriteRef(pixmap, location.size())];
	if (cache(count) && pos < 0 && m_pixmap && m_size.width() + width <= m_pixmap->width() && m_pixmap->height() >= height * 2)
	{	// not cached yet and we have enough room to create a cached version
		pos = m_size.width();
		m_size.setWidth(m_size.width() + width);
		QPainter	spritePainter(m_pixmap);
		spritePainter.setRenderHints(QPainter::SmoothPixmapTransform, true);
		nineTileDraw(spritePainter, QRect(pos, 0, width, height), pixmap, false, corner);
		nineTileDraw(spritePainter, QRect(pos, height, width, height), pixmap, true, corner);
	}
	if (cache(count) && VERIFY(pos >= 0))
		painter.drawPixmap(location.left(), location.top(), *m_pixmap, pos, (pressed ? height : 0), width, height);
	else
		nineTileDraw(painter, location, pixmap, pressed, corner);	// can't cache: draw directly...
}

#if 0
#define FRAGMENTS_INIT(n)
#define FRAMENT(n, destination, source)		painter.drawPixmap(destination, pixmap, source)
#define FRAMENTS_DRAW(n)
#else
#define FRAGMENTS_INIT(n)					QPainter::PixmapFragment	fragments[n]
#define FRAMENT(n, destination, source)		initPixmapFragment(fragments[n], destination, source)
#define FRAMENTS_DRAW(n)					painter.drawPixmapFragments(fragments, n, pixmap)
#endif

void NineTileSprites::nineTileDraw(QPainter & painter, const QRect & location, QPixmap & pixmap, bool pressed, const NineTileCorner & corner)
{
	qreal		source_half_height = pixmap.height() / 2;
	qreal		sourceTop = pressed ? source_half_height : 0;
	qreal		source_half_heightF = source_half_height - 2 * corner.m_trimV;
	qreal		sourceTopF = (pressed ? source_half_height : 0) + corner.m_trimV;

	int	shrinkX = pixmap.width() - 2 * corner.m_trimH - location.width();
	int	shrinkY = source_half_height - 2 * corner.m_trimV - location.height();
	if (shrinkY == 0)
	{
		if (shrinkX == 0)	// no scaling at all!
		{
			//painter.fillRect(location, QColor(0, 255, 0));
			painter.drawPixmap(QPointF(location.left(), location.top()), pixmap, QRectF(corner.m_trimH, sourceTopF, pixmap.width() - 2 * corner.m_trimH, source_half_heightF));
		}
		else
		{	// Extending horizontally: 3-tile
			//painter.fillRect(location, QColor(0, 0, 255));

			qreal nudgedCornerSizeX = corner.m_cornerSizeH - corner.m_trimH;

			qreal pixmap_x25 = corner.m_cornerSizeH;
			qreal pixmap_x75 = pixmap.width() - corner.m_cornerSizeH;
			qreal loc_top = location.top();
			qreal loc_left = location.left();
			qreal loc_x25 = loc_left + nudgedCornerSizeX;
			qreal loc_x75 = location.right() - nudgedCornerSizeX + 1;

			FRAGMENTS_INIT(3);
			FRAMENT(0, QPointF(loc_left, loc_top),										QRectF(corner.m_trimH, sourceTopF, nudgedCornerSizeX, source_half_heightF));
			FRAMENT(1, QPointF(loc_x75, loc_top),										QRectF(pixmap_x75, sourceTopF, nudgedCornerSizeX, source_half_heightF));
			FRAMENT(2, QRectF(loc_x25, loc_top, loc_x75 - loc_x25, location.height()),	QRectF(pixmap_x25, sourceTopF, pixmap_x75 - pixmap_x25, source_half_heightF));
			FRAMENTS_DRAW(3);
		}
	}
	else	// Sizes don't match, use full 9-tile
	{
		//painter.fillRect(location, QColor(255, 0, 0));

		qreal nudgedCornerSizeX = corner.m_cornerSizeH - corner.m_trimH;
		qreal nudgedCornerSizeY = corner.m_cornerSizeV - corner.m_trimV;

		qreal pixmap_x25 = corner.m_cornerSizeH;
		qreal pixmap_x75 = pixmap.width() - corner.m_cornerSizeH;
		qreal pixmap_y25 = sourceTop + corner.m_cornerSizeV;
		qreal pixmap_y75 = sourceTop + source_half_height - corner.m_cornerSizeV;
		qreal pixmap_gx = pixmap_x75 - pixmap_x25;
		qreal pixmap_gy = pixmap_y75 - pixmap_y25;

		qreal loc_top = location.top();
		qreal loc_left = location.left();
		qreal loc_x25 = loc_left + nudgedCornerSizeX;
		qreal loc_x75 = location.right() - nudgedCornerSizeX + 1;
		qreal loc_y25 = loc_top + nudgedCornerSizeY;
		qreal loc_y75 = location.bottom() - nudgedCornerSizeY + 1;
		qreal loc_gx = loc_x75 - loc_x25;
		qreal loc_gy = loc_y75 - loc_y25;

		FRAGMENTS_INIT(9);

		FRAMENT(0, QPointF(loc_left, loc_top),					QRectF(corner.m_trimH, sourceTopF, nudgedCornerSizeX, nudgedCornerSizeY));		// top-left
		FRAMENT(1, QPointF(loc_x75, loc_top),					QRectF(pixmap_x75, sourceTopF, nudgedCornerSizeX, nudgedCornerSizeY));			// top-right
		FRAMENT(2, QPointF(loc_left, loc_y75),					QRectF(corner.m_trimH, pixmap_y75, nudgedCornerSizeX, nudgedCornerSizeY));		// bottom-left
		FRAMENT(3, QPointF(loc_x75, loc_y75),					QRectF(pixmap_x75, pixmap_y75, nudgedCornerSizeX, nudgedCornerSizeY));			// bottom-right

		FRAMENT(4, QRectF(loc_x25, loc_top, loc_gx, nudgedCornerSizeY),		QRectF(pixmap_x25, sourceTopF, pixmap_gx, nudgedCornerSizeY));		// top
		FRAMENT(5, QRectF(loc_x25, loc_y75, loc_gx, nudgedCornerSizeY),		QRectF(pixmap_x25, pixmap_y75, pixmap_gx, nudgedCornerSizeY));		// bottom
		FRAMENT(6, QRectF(loc_left, loc_y25, nudgedCornerSizeX, loc_gy),	QRectF(corner.m_trimH, pixmap_y25, nudgedCornerSizeX, pixmap_gy));	// left
		FRAMENT(7, QRectF(loc_x75, loc_y25, nudgedCornerSizeX, loc_gy),		QRectF(pixmap_x75, pixmap_y25, nudgedCornerSizeX, pixmap_gy));		// right

		FRAMENT(8, QRectF(loc_x25, loc_y25, loc_gx, loc_gy),	QRectF(pixmap_x25, pixmap_y25, pixmap_gx, pixmap_gy));							// center

		FRAMENTS_DRAW(9);
	}
}

PixmapCache & PixmapCache::instance()
{
	static PixmapCache sInstance;
	return sInstance;
}

void PixmapCache::dispose(QPixmap * & pointer)
{
	if (pointer == NULL)
		return;
	//g_debug("PixmapCache::dispose: caching %dx%d", pointer->width(), pointer->height());
#if CACHE_PIXMAP
	m_stock.insert(pointer);
	schedulePurge();
#else
	delete pointer;
#endif
	pointer = NULL;
}

QPixmap * PixmapCache::get(const QSize & size)
{
	for (TCache::iterator iter = m_stock.begin(); iter != m_stock.end(); ++iter)
	{
		QPixmap * pixmap = *iter;
		if (pixmap->size() == size)
		{
			//g_debug("PixmapCache::get: recycling %dx%d", size.width(), size.height());
			m_stock.erase(iter);
			return pixmap;
		}
	}
	//g_debug("PixmapCache::get: creating %dx%d", size.width(), size.height());
	return new QPixmap(size);
}

void PixmapCache::suspendPurge(bool suspend)
{
	m_suspended = suspend;
	if (!m_suspended)
		schedulePurge();
}

void PixmapCache::schedulePurge()
{
	if (!m_suspended && m_stock.size() > 0)
	{
#if DELAY_PURGE_PIXMAP
		if (m_purgeTime == 0)
			g_timeout_add_seconds(1, delayedPurge, NULL);
		m_purgeTime = SingletonTimer::currentTime() + 1000;
#else
		delayedPurge();
#endif
	}
}

gboolean PixmapCache::delayedPurge(gpointer)
{
	return instance().delayedPurge();
}

gboolean PixmapCache::delayedPurge()
{
	if (!m_suspended)
	{
		if (SingletonTimer::currentTime() >= m_purgeTime)
		{
			if (m_stock.size() > 0)
			{
				//g_debug("PixmapCache::delayedPurge: deleting %u images", m_stock.size());
				for (TCache::iterator iter = m_stock.begin(); iter != m_stock.end(); ++iter)
				{
					//g_debug("PixmapCache::delayedPurge: deleting %dx%d", (*iter)->width(), (*iter)->height());
					delete *iter;
				}
				//g_debug("PixmapCache::delayedPurge: deletions complete!");
				m_stock.clear();
			}
			m_purgeTime = 0;
			return FALSE;
		}
		return TRUE;
	}
	m_purgeTime = 0;
	return FALSE;
}
