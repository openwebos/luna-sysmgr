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




#ifndef GLYPHCACHE_H
#define GLYPHCACHE_H

#include <qpainter.h>
#include <map>
#include <vector>
#include "Logging.h"
#include "VirtualKeyboard.h"	// for debug options

void initPixmapFragment(QPainter::PixmapFragment & fragment, const QPointF & topLeft, const QRectF & source);
void initPixmapFragment(QPainter::PixmapFragment & fragment, const QRectF & dest, const QRectF & source);

template <class T> class GlyphCache {

public:
	struct Line {
		Line(int height = 0, int width = 0) : m_height(height), m_usedWidth(width) {}
		int	m_height;
		int m_usedWidth;
	};

	typedef std::map<T, QRect> GlyphMap;

	QPixmap &		pixmap()						{ return m_pixmap; }
	QRect *			lookup(const T & ref)			// return NULL if the ref isn't present in the cache
	{
		typename GlyphMap::iterator iter = m_cache.find(ref);
		if (iter != m_cache.end())
			return &(iter->second);
		return NULL;
	}

	QRect &			get(const T & ref)				// never returns NULL, but the QRect may be invalid
	{
		return m_cache[ref];
	}

	bool			allocate(const QSize & size, QRect & rect)
	{
		int usedHeight = 0;
		typename std::vector<Line>::iterator bestMatch = m_lines.end();
		int	bestMatchUsedHeight = 0;
		for (typename std::vector<Line>::iterator line = m_lines.begin(); line != m_lines.end(); ++line)
		{
			if (line->m_height >= size.height() && line->m_usedWidth + size.width() <= m_pixmap.width())
			{
				if (line->m_height == size.height())	// exact height: no need to look any further
					return useLine(line->m_usedWidth, usedHeight, size, rect);
				if (bestMatch == m_lines.end() || bestMatch->m_height > line->m_height)
					bestMatch = line, bestMatchUsedHeight = usedHeight;
			}
			usedHeight += line->m_height;
		}
		// is the best match good enough?
		if (bestMatch != m_lines.end() && bestMatch->m_height <= size.height() * 12 / 10)
			return useLine(bestMatch->m_usedWidth, bestMatchUsedHeight, size, rect);
		// best match not good enough, let's create a new line if possible
		if (usedHeight + size.height() <= m_pixmap.height() && size.width() <= m_pixmap.width())
		{
			m_lines.push_back(Line(size.height()));
			return useLine(m_lines.back().m_usedWidth, usedHeight, size, rect);
		}
		// can the last line be grown verticaly using the last unused pixels to fit in?...
		if (m_lines.size() > 0)
		{
			Line & lastLine = m_lines.back();
			usedHeight -= lastLine.m_height;
			if (usedHeight + size.height() <= m_pixmap.height() && lastLine.m_usedWidth + size.width() <= m_pixmap.width())
			{
				lastLine.m_height = size.height();
				return useLine(lastLine.m_usedWidth, usedHeight, size, rect);
			}
		}
		// best match is the only match, let's use it no matter how bad it is...
		if (bestMatch != m_lines.end())
			return useLine(bestMatch->m_usedWidth, bestMatchUsedHeight, size, rect);
		m_full = true;
		return false;
	}
	bool		isFull() const						{ return m_full; }

	GlyphCache(int height, int lineWidth = 1024) : m_pixmap(lineWidth, height), m_full(false)
	{
		m_pixmap.fill(QColor(0, 0, 0, 0));
	}

private:
	QPixmap				m_pixmap;
	std::vector<Line>	m_lines;
	GlyphMap			m_cache;
	bool				m_full;

	// utility method to set the rect to the right values & update the line's used width
	inline bool	useLine(int & lineUsedWidth, int top, const QSize & size, QRect & rect)
	{
		rect.setRect(lineUsedWidth, top, size.width(), size.height());
		lineUsedWidth += size.width();
		return true;
	}
};

template <class T> struct DirectRenderer {
	// "real" operations that perform the real thing without using the cache, and drawing in the painter object.
	virtual void boundingRect(const QRect & location, const T & textSpec, QFont & font, int flags, QRect & outBoundingRect) = 0;
	virtual void renderNow(QPainter & painter, const QRect & location, const T & textSpec, int flags) = 0;
};

template <class T> class GlyphRenderer {
public:
	GlyphRenderer(GlyphCache<T> & glyphCache, DirectRenderer<T> & directRenderer) :
		m_glyphCache(glyphCache), m_directRenderer(directRenderer) {}

	virtual void render(const QRect & location, const T & textSpec, QFont & font, int flags = Qt::AlignCenter) = 0;
	virtual void renderNow(const QRect & location, const T & textSpec, QFont & font, int flags = Qt::AlignCenter)		{} // doesn't always make sense...
	virtual void flush() {}

protected:
	GlyphCache<T> &		m_glyphCache;
	DirectRenderer<T> &	m_directRenderer;
};

// GlyphRenderer that only populates the cache & doesn't render anything "for real" besides that.
template <class T> class GlyphCachePopulator : public GlyphRenderer<T> {
public:
	GlyphCachePopulator(GlyphCache<T> & glyphCache, DirectRenderer<T> & directRenderer) : GlyphRenderer<T>(glyphCache, directRenderer), m_painter(&glyphCache.pixmap())
	{
		m_painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing, true);
		m_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	}
	void render(const QRect & location, const T & textSpec, QFont & font, int flags = Qt::AlignCenter)
	{
		QRect & ref = GlyphRenderer<T>::m_glyphCache.get(textSpec);
		if (!ref.isValid())
		{
			QRect	boundingRect;
			textSpec.applyFontSettings(font);
			GlyphRenderer<T>::m_directRenderer.boundingRect(location, textSpec, font, flags, boundingRect);
			if (GlyphRenderer<T>::m_glyphCache.allocate(boundingRect.size(), ref) && VERIFY(boundingRect.size() == ref.size()))
			{
				m_painter.setFont(font);
				QRect		paintRect(ref.topLeft() + location.topLeft() - boundingRect.topLeft(), location.size());
#if VKB_SHOW_GLYPH_REGIONS
				m_painter.fillRect(ref, QColor(rand() % 255, rand() % 255, rand() % 255, 255));
#endif
				GlyphRenderer<T>::m_directRenderer.renderNow(m_painter, paintRect, textSpec, flags);
				return;
			}
			g_warning("GlyphCachePopulator::render: could not cache %s...", textSpec.description().toUtf8().data());
		}
	}

private:
	QPainter	m_painter;
};

// GlyphRenderer that will render from the cache if possible, caching if necessary & possible...
template <class T> class CachedGlyphRenderer : public GlyphRenderer<T> {
public:
	CachedGlyphRenderer(QPainter & painter, GlyphCache<T> & glyphCache, DirectRenderer<T> & directRenderer, int expectedFragmentCount) : GlyphRenderer<T>(glyphCache, directRenderer), m_painter(painter), m_cacheMissCount(0)
	{
		m_fragments.reserve(expectedFragmentCount);
	}
	~CachedGlyphRenderer()
	{
		flush();
	}

	void render(const QRect & location, const T & textSpec, QFont & font, int flags = Qt::AlignCenter)
	{
#if VKB_ENABLE_GLYPH_CACHE
		QRect * ref = GlyphRenderer<T>::m_glyphCache.lookup(textSpec);
		if (ref && VERIFY(ref->isValid()))
		{
			int x = location.left();
			int y = location.top();
			if (flags & Qt::AlignHCenter)
				x += (location.width() - ref->width()) / 2;
			else if (flags & Qt::AlignRight)
				x += location.width() - ref->width();
			if (flags & Qt::AlignVCenter)
				y += (location.height() - ref->height()) / 2;
			else if (flags & Qt::AlignBottom)
				y += location.height() - ref->height();
			QPainter::PixmapFragment	fragment;
			initPixmapFragment(fragment, QPoint(x, y), *ref);
			if (m_fragments.size() >= m_fragments.capacity())
				flush();
			m_fragments.push_back(fragment);
			return;
		}
		++m_cacheMissCount;
#if defined(TARGET_DESKTOP)
		g_warning("CachedGlyphRenderer::render: direct rendering of %s, size=%d, bold=%d...", textSpec.description().toUtf8().data(), font.pixelSize(), int(font.bold()));
#endif
#endif
		// last resort: direct draw on screen...
		textSpec.applyFontSettings(font);
		m_painter.setFont(font);
		GlyphRenderer<T>::m_directRenderer.renderNow(m_painter, location, textSpec, flags);
	}
	void renderNow(const QRect & location, const T & textSpec, QFont & font, int flags = Qt::AlignCenter)
	{
		textSpec.applyFontSettings(font);
		m_painter.setFont(font);
		GlyphRenderer<T>::m_directRenderer.renderNow(m_painter, location, textSpec, flags);
	}

	void flush()
	{
		if (m_fragments.size() > 0)
		{
			m_painter.drawPixmapFragments(&m_fragments[0], m_fragments.size(), GlyphRenderer<T>::m_glyphCache.pixmap());
			m_fragments.resize(0);
		}
	}
	int getCacheMissCount() const			{ return m_cacheMissCount; }

private:
	QPainter &								m_painter;
	std::vector<QPainter::PixmapFragment>	m_fragments;
	int										m_cacheMissCount;
};


/*
  Direct renderer that prints the same text twice using two colors and one pixel vertical offset.
  Requirement:
    class T must have the following members:
        QString m_string: the text
        QColor  m_frontColor: color of the text in front (above)
        QColor  m_backColor: color of the text in the back & below by 1 pixel
*/

template <class T> class DoubleDrawRendererT : public DirectRenderer<T> {
public:
    void boundingRect(const QRect & location, const T & textSpec, QFont & font, int flags, QRect & outBoundingRect)
    {
        // some letters need more space left or right... rather that resize everything, just handle these special cases.
        outBoundingRect = QFontMetrics(font).boundingRect(location, flags, textSpec.m_string);
        if (textSpec.m_string.size() > 0)
        {
            // enlarge to the left?
            int left = textSpec.m_string[0].unicode();
            if (left == 'j' || left == Qt::Key_Idiaeresis /* Ï */)
                outBoundingRect.setLeft(outBoundingRect.left() - 1);
            // enlarge to the right?
            int right = textSpec.m_string[textSpec.m_string.size() - 1].unicode();
            if (right == 'F' || right == 'X' || right == 0x152 /* Œ */ || right == 0xef /* ï */ || right == 0xed /* í */ || right == 0x142 /* ł */)
                outBoundingRect.setRight(outBoundingRect.right() + 1);
        }
        if (textSpec.m_backColor != textSpec.m_frontColor)
            outBoundingRect.setBottom(outBoundingRect.bottom() + 1);
    }
    void renderNow(QPainter & painter, const QRect & r, const T & textSpec, int flags = Qt::AlignCenter)
    {
        if (textSpec.m_backColor != textSpec.m_frontColor)
        {
            QRect rect(r);
            rect.setTop(rect.top() + 1);    // draw back, one pixel below. Don't move the bottom: the rect was the frame for both glyphs.
            painter.setPen(textSpec.m_backColor);
            painter.drawText(rect, flags | Qt::TextDontClip, textSpec.m_string);
            painter.setPen(textSpec.m_frontColor);
            rect.adjust(0, -1, 0, -1);      // draw front, one pixel above. Move both top & bottom this time.
            painter.drawText(rect, flags | Qt::TextDontClip, textSpec.m_string);
        }
        else
        {
            painter.setPen(textSpec.m_frontColor);
            painter.drawText(r, flags | Qt::TextDontClip, textSpec.m_string);
        }
    }
};

#endif // GLYPHCACHE_H
