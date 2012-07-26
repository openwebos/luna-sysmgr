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



 /*
 *      This is a simple implementation of what is known colloquially as a "pixmap atlas"
 *      It also manages something similar to the QPixmapCache, except it operates on PixmapObjects instead
 *      (see pixmapobject.h/cpp)
 *      The general idea is that these operate on two levels. Small pixmaps are aggregated into "atlas" pages
 *      so that they can be accessed faster. The atlas pages along with larger pixmaps that won't fit onto atlas pages
 *      are cached into a pixmap cache
 *
 *		a word on the atlas pages:
 *			Atlases were written up by many others (Nvidia most prominently) as a way to contain small, tile or icon -type
 *			pixmaps in a larger pixmap which could be held in (gpu) memory. This way, fragment shaders and textures don't need
 *			to be swapped in and out of memory; One simply provides the coordinate in the big texture of the smaller texture
 *			desired. This can be expressed very efficiently in the shader program as just a texture mapping matrix
 *
 *			In this implementation, I'll use the same basic idea with a few tweaks. First, because my atlas is geared towards
 *			basically just storing icons and things of that nature, and never larger items, it can have some constraints. Most
 *			icons are either 16x16,32x32,48x48,64x64,128x128,or 256x256. These can then be partitioned into separate pages so that
 *			the icons can be packed within w/ minimal waste of space. So e.g. one page is designated the "32 page" and only
 *			holds 32x32 icons, and these can all be packed neatly into row-columns since they're the same, square size.
 *			Each page that is an atlas page has a directory member that is a hash keyed on the individual icon's uid,
 *			and holds a value that's a rectangle R and a size S. The R is the coordinate rect inside the pixmap of that page
 *			(i.e. the whole atlas page with all the icons inside it) that describes where the icon actually is, and S is the
 *			size of the *original* icon, before it was placed inside the atlas page. The reason for S is that under a certain
 *			optimization scenario, an icon that is just slightly different in size from the size of the icons that a particular
 *			atlas page supports, then the icon will be scaled up/down to match the size. When it is retrieved, this has to be
 *			reversed, so the original size is needed. e.g. a 30x30 icon could be stored in a 32x32-designated page.
 *			If the scaling factor is small (especially if it's a scale-up), then the negative visual effects will be minimal
 *
 *		NOT THREAD SAFE! among other things, the most blatant problem is that getPixmap()'s returned QPixmap must remain
 *		valid throughout the function that made the call. This will not be guaranteed with multiple threads, as a cache purge
 *		triggered by another thread could wipe out that QPixmap
 */

#ifndef PIXPAGER_H_
#define PIXPAGER_H_

#include <QObject>
#include <QPointer>
#include <QUuid>
#include <QRect>
#include "dimensionsglobal.h"
#include <QHash>
#include <QMap>
#include <QPair>
#include <QList>

/////HOW TO KEEP TRACK OF HOLES!?

class PixPager;
class PixmapObject;
class QPixmap;

/*
 * PixPagerPage is not meant to be used externally; it's for PixPager use only
 *
 * PixPagerPage shouldn't do anything 'smart' on its own. It's simply to contain data for
 * PixPager to manage, and to notify pixpager when stuff happens (like the pixmap deleted by someone else;
 * which should be kept to a minimum with proper pixpager usage)
 *
 */

class PixPagerPage : public QObject
{
	Q_OBJECT
public:

	class PixmapRects
	{
	public:
		PixmapRects() {}
		PixmapRects(const QRect& r,const QSize& s)
		: coordinateRect(r) , originalSize(s) {}
		QRect coordinateRect;
		QSize originalSize;		//will be different than the size of the coordinateRect if the pixmap was
								//scaled to fit into the atlas page (see explanation of atlas pages at the
								//top of this file)
	};

	PixPagerPage(PixPager * p_pager);
	virtual ~PixPagerPage();

	QPointer<PixPager> m_pager;
	QPointer<PixmapObject> m_data;
	bool	m_pinned;
	quint32 m_sizeInBytes;

	quint32 m_accessCounter;
	qreal m_accessFrequency;				//a proportion of accesses to this page vs the total of accesses for all pages

	bool m_initiatedDelete;

public Q_SLOTS:

	virtual void slotPixmapObjectDeleted();

};

class PixPagerAtlasPage : public PixPagerPage
{
	Q_OBJECT
public:

	PixPagerAtlasPage(PixPager * p_pager,quint32 sizeDesignation,QHash<QUuid,PixmapRects> * p_directory = 0);
	virtual ~PixPagerAtlasPage();

	//NOTE: the way to determine the occupancy (and thus free space) of this atlas page is:
	// free cells = m_rows * m_columns - m_p_directory->size()
	quint32 freeSpace() const
	{
		return (m_rows * m_columns - m_directory.size());
	}
	qreal occupancyRate() const
	{
		if ((quint32)(m_directory.size()) == m_rows*m_columns)
			return 1.0;		//make sure that below doesn't return something dumb like 0.99999999....
		return (qreal)(m_directory.size())/(qreal)(m_rows*m_columns);
	}

	bool getFromFreelist(QPair<quint32,quint32>& r_location)
	{
		if (m_freeList.empty())
			return false;
		r_location = m_freeList.takeFirst();
		return true;
	}
	bool unoccupiedLocation(QPair<quint32,quint32>& r_location)
	{
		if (m_lastPositionAllocated == INVALID_LOCATION)
			return getFromFreelist(r_location);
		//go sequentially
		if ((m_lastPositionAllocated.first == m_columns-1) && (m_lastPositionAllocated.second == m_rows-1))
			return getFromFreelist(r_location);
		else if (m_lastPositionAllocated.first == m_columns-1)
		{
			//next row, col=0
			r_location = QPair<quint32,quint32>(0,m_lastPositionAllocated.second+1);
		}
		else
		{
			//next column, same row
			r_location = QPair<quint32,quint32>(m_lastPositionAllocated.first+1,m_lastPositionAllocated.second);
		}
		return true;
	}

	QRect targetRectForGridCoordinates(const QPair<quint32,quint32>& gridCoordinates) const;

	QHash<QUuid,PixmapRects> m_directory;		//keeps the locations of the contained
															//mini-pm's keyed on their uids null if the page isn't an atlas
	quint32	m_pixmapSqSize;			//the size designation for this atlas page...32 -> 32x32 sq. pixmaps
										//this is also the classId passed back to the add functions
	QList<QPair<quint32,quint32> > m_freeList;

	quint32 m_rows;
	quint32 m_columns;
	quint32 m_xLeadingPixelSpace;
	quint32 m_yLeadingPixelSpace;
	quint32 m_xTrailingPixelSpace;
	quint32 m_yTrailingPixelSpace;
	quint32 m_interRowPixelSpace;
	quint32 m_interColPixelSpace;
	QPair<quint32,quint32> m_lastPositionAllocated;
	static QPair<quint32,quint32> INVALID_LOCATION;
};

namespace PageOpsReturnCode
{
	enum Enum
	{
		OK,
		InvalidParameters,
		PageSizeExceedsMaxTextureSize,
		PageSizeExceedsMaxCacheSize
	};
}

class PixPager : public QObject
{
	Q_OBJECT

public:
	PixPager();
	virtual ~PixPager();

	//general function; will lookup both atlas and non-atlas based uids
	PixmapObject * getPixmap(const QUuid& uid,QRect& r_coordRect,QSize& r_originalSize);

	//returns a uid as a handle, which will not be valid if the add failed
	// * pinPage will make sure that the page in question in never expunged from the cache.
	QUuid addPixmap(QPixmap * p_pixmap,bool pinPage=false);

	/*
	 *
	 * allowScale will try and scale the pixmap up or down to fit on an existing atlas page. The allowed scale amounts
	 * are in the gfxsettings file. It will try to scale up first, because it's usually less lossy when it is scaled back
	 * down later to return it to normal
	 * allowPageCreation will allow a new atlas page to be created if none is found to fit this pixmap (even w/ scaling,
	 * if it was allowed). If it is false, and a page can't be found, then the add will fail
	 *
	 * if both settings are true, a scaling will be tried, followed by the creation of a page
	 *
	 * by default, atlas pages are always pinned and will never be expunged by automatic replacement means,
	 * so use allowPageCreation carefully
	 */
	QUuid addPixmapToAtlasPage(QPixmap * p_pixmap,bool allowScale=true,bool allowPageCreation=false);

	friend class PixPagerPage;
	friend class PixPagerDebugger;

private:

	void _pagePixmapDeleted(PixPagerPage * p_page);
	//returns # of pages expunged
	quint32  _findAndExpunge(quint32 minSize);
	void _deleteRegularPage(PixPagerPage * p_page);
	// try and figure out if the pixmap can be "squarified" to an existing atlas page size requirement
	//		given the current atlas page size designations and the allowed scale factors in the gfxsettings
	//		file. If yes, the new square size is returned (i.e. the quint returned is the length of a side
	//		of the square.
	//		If no, then just the existing size is returned
	quint32 _determineSquareSize(QPixmap * p_pixmap,bool allowScale) const;
	quint32 _determineSquareSizeFromRectPixmap(QPixmap * p_pixmap,bool allowScale) const;
	quint32 _forceSquarify(QPixmap * p_pixmap,bool maxRect=false) const;

	static bool _isSquarePix(QPixmap * p_pixmap);

	PageOpsReturnCode::Enum _addAtlasPixmapEntry(PixPagerAtlasPage& page,QPixmap * p_pixmap,quint32 sqSize,const QPair<quint32,quint32>& targetGridLocation,
												QUuid& r_insertedPixmapUid);
	PageOpsReturnCode::Enum _createAndAddAtlasPageWithInitialEntry(QPixmap * p_pixmap,quint32 sqSize,quint32 initialRows,quint32 initialColumns,
																	QUuid& r_pageUid,QUuid& r_insertedPixmapUid,bool allowExpunge=true);
	PageOpsReturnCode::Enum _copyAndExpandAtlasPage(PixPagerAtlasPage * p_atlasPage,quint32 numRows,
													quint32 numColumns,bool makeSquarePage=false,QPair<quint32,quint32> * r_p_nextAvailableLocation=0);
	static quint32 nextpwr2(quint32 t);

	inline quint32 _spaceRemaining() const
	{ return (m_maxSizeInBytes <= m_currentSizeInBytes ? 0 : (m_maxSizeInBytes - m_currentSizeInBytes)); }
	inline quint32 _expungeAmountForMinsize(quint32 minSize) const
	{
		//since maxSizeInBytes is not a hard limit (it can be exceeded slightly on a temporary basis), max - current = s , s < 0 is possible
		//this convenience function will take that negative s and add it to minSize so that the minimal amount that needs to be
		//expunged to allocate a minSize page is returned
		return ((m_maxSizeInBytes <= m_currentSizeInBytes) ? (minSize+(m_currentSizeInBytes-m_maxSizeInBytes))
				: ((m_maxSizeInBytes-m_currentSizeInBytes >= minSize) ? 0
						:(minSize - (m_maxSizeInBytes-m_currentSizeInBytes))));
	}

	quint32 m_maxSizeInBytes;
	quint32 m_currentSizeInBytes;
	quint32 m_accessCounter;		//getPixmap access for all pages

	//keyed by the size of the small pixmaps within ...so 64 -> 64x64 pixmaps
	//alias hashes, so they don't own the pixpagerpages
	// there may be multiple entries with the same size (using insertMulti)
	QMap<quint32,PixPagerPage *> m_atlasPages_alias;
	QHash<QUuid,PixPagerPage *> m_atlasPagesByIndividualUids_alias;		//key: the little pixmap inside the atlas's uid,
																		//value: the pixpagerpage that contains the atlas for it

	//this one owns the pages
	QHash<QUuid,PixPagerPage *> m_pageCache;

};

#endif /* PIXPAGER_H_ */
