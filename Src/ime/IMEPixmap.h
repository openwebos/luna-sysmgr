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




#ifndef IME_PIXMAP_H
#define IME_PIXMAP_H

#include <qpixmap.h>
#include <map>
#include <vector>
#include <set>

#include <glib.h>
#include <stdint.h>

/*
	Thin layer above QPixmap to define the resource once, but only load it if & when first needed.
	Note: the name is expected to be a const char * constant string which can be persisted.
*/

class IMEPixmap
{
public:
	IMEPixmap(const char * name = "") : m_name(name) { s_PalmPixmaps.push_back(this); }

	void			setName(const char * name)	{ m_name = name; }

					operator QPixmap &()		{ return pixmap(); }
	QPixmap &		pixmap();

	int				width()				{ return pixmap().width(); }
	int				height()			{ return pixmap().height(); }
	QSize			size()				{ return pixmap().size();  }
	QRect			rect()				{ return pixmap().rect(); }

	static void		setDefaultLocation(const char * defaultLocation)		{ s_defaultLocation = defaultLocation; }
	static int		count()				{ return s_PalmPixmaps.size(); }
	static void		load(int index)		{ if (index >= 0 && index < (int)s_PalmPixmaps.size()) s_PalmPixmaps[index]->pixmap(); }

private:
	QPixmap			m_pixmap;
	const char *	m_name;

	static const char * s_defaultLocation;

	static std::vector<IMEPixmap*>	s_PalmPixmaps;
};

struct NineTileCorner
{
	NineTileCorner(qreal cornerSizeH, qreal cornerSizeV, qreal trimH = 0, qreal trimV = 0) : m_cornerSizeH(cornerSizeH), m_cornerSizeV(cornerSizeV), m_trimH(trimH), m_trimV(trimV) {}

	qreal m_cornerSizeH;
	qreal m_cornerSizeV;
	qreal m_trimH;
	qreal m_trimV;
};

class NineTileSprites {

	struct SpriteRef
	{
		SpriteRef() : m_pixmap(NULL) {}
		SpriteRef(const QPixmap & pixmap, const QSize & position) : m_pixmap(&pixmap), m_position(position.width(), position.height()) {}
		SpriteRef(const QPixmap & pixmap, const QPoint & position) : m_pixmap(&pixmap), m_position(position) {}

		bool	operator==(const SpriteRef & rhs) const	 { return m_pixmap == rhs.m_pixmap && m_position == rhs.m_position; }
		bool	operator<(const SpriteRef & rhs) const
		{
			if (m_pixmap < rhs.m_pixmap)
				return true;
			if (m_pixmap == rhs.m_pixmap)
			{
				int x = m_position.x();
				int rhs_x = rhs.m_position.x();
				if (x < rhs_x)
					return true;
				if (x == rhs_x && m_position.y() < rhs.m_position.y())
					return true;
			}
			return false;
		}

		const QPixmap *	m_pixmap;
		QPoint			m_position;
	};

	struct SpritePos
	{
		SpritePos() : m_x(-1) {}

		operator int & ()	{ return m_x; }

		int	m_x;
	};

	typedef std::map<SpriteRef, SpritePos> TSprites;

public:
	NineTileSprites() : m_pixmap(NULL), m_size(0, 0) {}
	~NineTileSprites();

	void	reserve(bool start);
	void	reserve(QSize size, int count, QPixmap & pixmap);
	void	draw(QPainter & painter, const QRect & location, QPixmap & pixmap, bool pressed, int count, const NineTileCorner & corner);

private:
	void	nineTileDraw(QPainter & painter, const QRect & location, QPixmap & pixmap, bool pressed, const NineTileCorner & corner);
	bool	cache(int count)					{ return count <= 2; }

	TSprites	m_sprites;
	QSize		m_size;
	QPixmap *	m_pixmap;
};

class PixmapCache
{
public:
	static PixmapCache & instance();

	void		dispose(QPixmap * & pointer);
	QPixmap *	get(int width, int height)		{ return get(QSize(width, height)); }
	QPixmap *	get(const QSize & size);

	void		suspendPurge(bool suspend);

private:
	PixmapCache() : m_suspended(false), m_purgeTime(0) {}

	static gboolean	delayedPurge(gpointer);
	gboolean		delayedPurge();
	void			schedulePurge();

	typedef std::set<QPixmap *> TCache;

	TCache		m_stock;
	bool		m_suspended;
	uint64_t	m_purgeTime;
};

#endif // IME_PIXMAP_H
