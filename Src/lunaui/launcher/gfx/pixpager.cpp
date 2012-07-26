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




#include "pixpager.h"
#include "pixmapobject.h"
#include "gfxsettings.h"
#include <QMutableHashIterator>
#include <QtGlobal>
#include <QtCore/qmath.h>
#include <QMap>
#include <QList>
#include <QPainter>

#define DEFAULT_ATLAS_PAGE_XLEADSPACE		4
#define DEFAULT_ATLAS_PAGE_YLEADSPACE		4
#define DEFAULT_ATLAS_PAGE_MIN_XTRAILSPACE	4
#define DEFAULT_ATLAS_PAGE_MIN_YTRAILSPACE	4
#define DEFAULT_ATLAS_PAGE_INTERCOL_SPACE	4
#define DEFAULT_ATLAS_PAGE_INTERROW_SPACE	4

PixPagerPage::PixPagerPage(PixPager * p_pager)
: m_pager(p_pager)
, m_data(0)
, m_pinned(false)
, m_sizeInBytes(0)
, m_accessCounter(0)
, m_accessFrequency(0.0)
, m_initiatedDelete(false)
{
}

//virtual
PixPagerPage::~PixPagerPage()
{
	m_initiatedDelete = true;
	delete m_data;
}

void PixPagerPage::slotPixmapObjectDeleted()
{
	if (m_initiatedDelete)
		return;		//avoid a loop and double deletes

	PixmapObject * pPmo = qobject_cast<PixmapObject*>(sender());

	if (!m_pager.isNull() && (pPmo == m_data.data()))
		m_pager->_pagePixmapDeleted(this);
}

//static
QPair<quint32,quint32> PixPagerAtlasPage::INVALID_LOCATION(UINT_MAX,UINT_MAX);

PixPagerAtlasPage::PixPagerAtlasPage(PixPager * p_pager,quint32 sizeDesignation,QHash<QUuid,PixmapRects> * p_directory)
: PixPagerPage(p_pager)
, m_pixmapSqSize(sizeDesignation)
, m_rows(0)
, m_columns(0)
, m_xLeadingPixelSpace(DEFAULT_ATLAS_PAGE_XLEADSPACE)
, m_yLeadingPixelSpace(DEFAULT_ATLAS_PAGE_YLEADSPACE)
, m_xTrailingPixelSpace(DEFAULT_ATLAS_PAGE_MIN_XTRAILSPACE)
, m_yTrailingPixelSpace(DEFAULT_ATLAS_PAGE_MIN_YTRAILSPACE)
, m_interRowPixelSpace(DEFAULT_ATLAS_PAGE_INTERROW_SPACE)
, m_interColPixelSpace(DEFAULT_ATLAS_PAGE_INTERCOL_SPACE)
, m_lastPositionAllocated(INVALID_LOCATION)
{
	m_pinned = true;
	if (p_directory)
		m_directory = *p_directory;
}

//virtual
PixPagerAtlasPage::~PixPagerAtlasPage()
{
}

QRect PixPagerAtlasPage::targetRectForGridCoordinates(const QPair<quint32,quint32>& gridCoordinates) const
{
	if ((gridCoordinates.first >= m_columns) || (gridCoordinates.second >= m_rows))
		return QRect(0,0,0,0);
	return (QRect(
			m_xLeadingPixelSpace+(m_pixmapSqSize*gridCoordinates.first)+(m_interColPixelSpace*gridCoordinates.first),
			m_yLeadingPixelSpace+(m_pixmapSqSize*gridCoordinates.second)+(m_interRowPixelSpace*gridCoordinates.second),
			m_pixmapSqSize,
			m_pixmapSqSize
	));
}

///////////////////////////////////// PixPager code //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


PixPager::PixPager()
: m_currentSizeInBytes(0)
, m_accessCounter(0)
{
	m_maxSizeInBytes = GraphicsSettings::DiUiGraphicsSettings()->totalCacheSizeLimitInBytes;
}

//virtual
PixPager::~PixPager()
{
}

/*
 * Called from the PixPagerPage itself when its pixmap is deleted underneath it.
 * NOTE: THIS IS NOT CALLED IF THE PixPagerPage itself initiated the deletion, specifically if
 * the PixPagerPage is being deleted and as a result deletes the underlying pixmap
 *
 */
void PixPager::_pagePixmapDeleted(PixPagerPage * p_page)
{
	if (p_page == NULL)
		return;

	//expunge from cache...
	//see if it's an atlas page first
	PixPagerAtlasPage * p_atlasPage = qobject_cast<PixPagerAtlasPage *>(p_page);
	if (p_atlasPage)
	{
		//remove from aliased hash
		m_atlasPages_alias.remove(p_atlasPage->m_pixmapSqSize);
		//run through the hash with the individual icon uid keys and remove the entries that have this page as the value
		QMutableHashIterator<QUuid, PixPagerPage *> i(m_atlasPagesByIndividualUids_alias);
		while (i.hasNext()) {
			i.next();
			if (i.value() == p_page)
				i.remove();
		}
	}

	//remove from the master hash (the actual cache)
	m_pageCache.remove(p_page->m_data->id());
	//decrement from the total size of the cache
	if (p_atlasPage && (GraphicsSettings::DiUiGraphicsSettings()->atlasPagesExemptFromSizeLimit == false))
		m_currentSizeInBytes -= qMax(p_page->m_sizeInBytes,m_currentSizeInBytes);
	//actually delete the pixpagerpage, but clear its m_data so that it doesn't try to delete the pmo that's
	//already being deleted (that deletion is what caused this function to execute)
	p_page->m_data = 0;
	delete p_page;
}

//general,dumb function; will lookup both atlas and non-atlas based uids
//r_coordRect returns the coordinates where the pixmap is stored
PixmapObject * PixPager::getPixmap(const QUuid& uid,QRect& r_coordRect,QSize& r_originalSize)
{
	//try the atlas pages first, since most accesses will probably be for an atlas'd icon
	QHash<QUuid,PixPagerPage *>::const_iterator found = m_atlasPagesByIndividualUids_alias.find(uid);
	if (found != m_atlasPagesByIndividualUids_alias.end())
	{
		//found it , and it is a part of the atlases...
		//it's a PixPagerAtlasPage actually, cast:
		PixPagerAtlasPage * const p_atlasPage = qobject_cast<PixPagerAtlasPage * const>(found.value());
		if (p_atlasPage == NULL)
		{
			qFatal("%s: page found in an atlas index but failed to qcast to atlas page type",__FUNCTION__);
			//execution will abort here
		}
		++m_accessCounter;
		++(p_atlasPage->m_accessCounter);
		p_atlasPage->m_accessFrequency = (qreal)p_atlasPage->m_accessCounter / (qreal)m_accessCounter;
		//find the coordinate rect and size
		QHash<QUuid,PixPagerPage::PixmapRects>::const_iterator dirfound = p_atlasPage->m_directory.find(uid);
		if (dirfound == p_atlasPage->m_directory.end())
		{
			//this would ordinarily mean a cache replacement killed off this (atlas) pixmap but in the case of an icon-in-atlas
			//the deletion would have wiped the m_atlasPagesByIndividualUids_alias entry too and would not have led
			//execution down this far
			qFatal("%s: atlas page's directory does not contain this pixmap",__FUNCTION__);
			//execution will abort here
		}
		r_coordRect = dirfound.value().coordinateRect;
		r_originalSize = dirfound.value().originalSize;
		return p_atlasPage->m_data;
	}

	//didn't find it in the atlas pages, so try the regular ones
	found = m_pageCache.find(uid);
	if (found != m_pageCache.end())
	{
		//found it. It's just a regular page here
		++m_accessCounter;
		++(found.value()->m_accessCounter);
		found.value()->m_accessFrequency = (qreal)found.value()->m_accessCounter / (qreal)m_accessCounter;

		if (found.value()->m_data->data() == NULL)
		{
			qFatal("%s: (on non-atlas page access) underlying qpixmap is null",__FUNCTION__);
			//execution will abort here
		}
		r_originalSize = (*(found.value()->m_data))->size();
		r_coordRect = (*(found.value()->m_data))->rect();
		return found.value()->m_data;
	}

	//not found anywhere
	return 0;
}

QUuid PixPager::addPixmap(QPixmap * p_pixmap,bool pinPage)
{
	if (p_pixmap == NULL)
		return QUuid();

	//check the space requirements
	const quint32 pmsize = PixmapObject::sizeOfPixmap(p_pixmap);
	if (pmsize + m_currentSizeInBytes > m_maxSizeInBytes)
	{
		//have to expunge something
		if (_findAndExpunge(pmsize) == 0)
			return QUuid();		//couldn't add it due to lack of space
	}

	//create a new PixmapObject for the p_pixmap
	PixmapObject * pPmo = new PixmapObject(p_pixmap);
	//and a page for it
	PixPagerPage * pPage = new PixPagerPage(this);
	pPage->m_data = pPmo;
	pPage->m_pinned = pinPage;
	pPage->m_sizeInBytes = pmsize;
	connect(pPmo,SIGNAL(signalObjectDestroyed()),pPage,SLOT(slotPixmapObjectDeleted()));
	//insert to caches
	m_pageCache.insert(pPmo->id(),pPage);
	m_currentSizeInBytes += pmsize;

	return pPmo->id();
}

QUuid PixPager::addPixmapToAtlasPage(QPixmap * p_pixmap,bool allowScale,bool allowPageCreation)
{
	//REMEMBER TO RETURN THE UID OF THE *ICON* (i.e. p_pixmap's inserted entity), NOT THE PAGE'S UID!

	if (p_pixmap == NULL)
		return QUuid();

	const quint32 pmSize = PixmapObject::sizeOfPixmap(p_pixmap);
	//find the atlas page where this might belong
	quint32 sqSize;
	if (PixPager::_isSquarePix(p_pixmap))
		sqSize = _determineSquareSize(p_pixmap,allowScale);
	else
		sqSize = _determineSquareSizeFromRectPixmap(p_pixmap,allowScale);

	//try and find it in the existing atlas page(s) for this size
	PixPagerAtlasPage * pSelectedPage = 0;
	qreal minOccupancy = 1.0;
	QMap<quint32, PixPagerPage *>::iterator i = m_atlasPages_alias.find(sqSize);
	while (i != m_atlasPages_alias.end() && i.key() == sqSize) {
		PixPagerAtlasPage * pAtlasPage = qobject_cast<PixPagerAtlasPage *>(i.value());
		if (!pAtlasPage) continue;
		if ((pSelectedPage == NULL)
				|| (pAtlasPage->occupancyRate() < minOccupancy))
		{
			pSelectedPage = pAtlasPage;
			minOccupancy = pAtlasPage->occupancyRate();
		}
		++i;
	}

	//if there was no page selected, then check to see if page creation is allowed. If not, then bail out now
	if ((!pSelectedPage) && !allowPageCreation)
	{
		return QUuid();
	}
	else if (!pSelectedPage)
	{
		//no page selected but creation is allowed
		QUuid pageId,iconId;
		if (_createAndAddAtlasPageWithInitialEntry(p_pixmap,sqSize,1,1,pageId,iconId) != PageOpsReturnCode::OK)
		{
			//couldn't add page
			return QUuid();
		}
		return iconId;		//Done!
	}
	else
	{
		//there is a candidate page that can take this size icon, but unclear yet if there is enough space on the page
		//to do so (could be completely full)
		QPair<quint32,quint32> targetLocation;
		if (pSelectedPage->unoccupiedLocation(targetLocation) == false)
		{
			//if the page was previously resized, it might have
			//try and expand it, since there is no more space
			if (_copyAndExpandAtlasPage(pSelectedPage,pSelectedPage->m_rows+1,pSelectedPage->m_columns+1,false,&targetLocation)
					!= PageOpsReturnCode::OK)
			{
				//failure. Some kind of allocation problem, or just plain out of cache space. Nothing that can be done
				return QUuid();
			}
		}
		//found location


	}
	return QUuid();
}

quint32 PixPager::_findAndExpunge(quint32 minSize)
{
	//SLOW
	//IMPROVE

	if (minSize > m_maxSizeInBytes)
			return 0;		//not possible to expunge enough, since the size of the cache is smaller than the size requested

	QMap<quint32,PixPagerPage *> sizeMap;

	//search for a page to expunge
	PixPagerPage * pSinglePage = 0;
	quint32 minDelta = UINT_MAX;
	quint32 maxExpungePossible = 0;
	QHash<QUuid,PixPagerPage *>::const_iterator it = m_pageCache.constBegin();
	while (it != m_pageCache.constEnd()) {
		if ((*it)->m_pinned)
			continue;	//pinned page...skip

		maxExpungePossible +=(*it)->m_sizeInBytes;
		if ((*it)->m_sizeInBytes < minSize)
		{
			sizeMap.insertMulti((*it)->m_sizeInBytes,it.value());
			continue;
		}
		if ((*it)->m_sizeInBytes == minSize)
		{
			pSinglePage = *it;
			break;
		}
		//else the page is > minSize
		if (((*it)->m_sizeInBytes - minSize) < minDelta)
		{
			minDelta = ((*it)->m_sizeInBytes - minSize);
			pSinglePage = *it;
		}
	}
	if (pSinglePage)
	{
		//this is the one to expunge
		_deleteRegularPage(pSinglePage);
		return 1;
	}

	//didn't find a single page that fit minsize and wasn't pinned.
	//First, were enough expunge-able pages even found? No sense in expunging good pages if I can't reach my minSize quota
	if (maxExpungePossible < minSize)
		return 0;

	//Ok, a minSize expunge is possible through a series of smaller page expunges
	//Walk the map backwards (it'll be sorted by page size in ASCENDING order at this point), and
	//remove all the pages until total >= minsize
	// None of the pages in the map are pinned.

	quint32 totalExpunged=0;
	quint32 ecount=0;
	QMap<quint32, PixPagerPage *>::const_iterator wi = sizeMap.constEnd();
	while (wi != sizeMap.constBegin())
	{
		--wi;
		totalExpunged += wi.key();
		_deleteRegularPage(*wi);
		++ecount;
		if (totalExpunged >= minSize)
			break;
	}
	return ecount;
}

void PixPager::_deleteRegularPage(PixPagerPage * p_page)
{
	//remove from the master hash (the actual cache)
	m_pageCache.remove(p_page->m_data->id());
	//decrement from the total size of the cache
	m_currentSizeInBytes -= qMax(p_page->m_sizeInBytes,m_currentSizeInBytes);
	//actually delete the pixpagerpage,
	delete p_page;
}

quint32 PixPager::_determineSquareSize(QPixmap * p_pixmap,bool allowScale) const
{
	//SLOW
	//private functions don't check parameters. p_pixmap has to be square. Else, call the ...FromRectPixmap version

	quint32 s = (quint32)(p_pixmap->size().width());
	if ( allowScale && (GraphicsSettings::DiUiGraphicsSettings()->allowedScaleDownPercentage == 0)
			&& (GraphicsSettings::DiUiGraphicsSettings()->allowedScaleUpPercentage == 0))
		return s;		//no scaling is allowed

	//find the nearest smallest and nearest largest size designators of the existing atlas pages
	quint32 less=UINT_MAX;
	quint32 more=UINT_MAX;
	for (QMap<quint32,PixPagerPage *>::const_iterator it = m_atlasPages_alias.constBegin();
			it != m_atlasPages_alias.constEnd();++it)
	{
		if (it.key() < s)
			less = it.key();
		else if (it.key() >= s)
		{
			more = it.key();
			break;
		}
	}
	//try to scale up to the next highest, if available
	if (more != UINT_MAX)
	{
		if ((qreal)(more-s)*100.0 < (qreal)(GraphicsSettings::DiUiGraphicsSettings()->allowedScaleUpPercentage) * (qreal)s)
			return more;	//we're good!
	}
	//try to scale down as a fallback
	if (less != UINT_MAX)
	{
		if ((qreal)(s-less)*100.0 < (qreal)(GraphicsSettings::DiUiGraphicsSettings()->allowedScaleDownPercentage) * (qreal)s)
			return less;	//we're good!
	}

	//none found
	return s;
}

quint32 PixPager::_determineSquareSizeFromRectPixmap(QPixmap * p_pixmap,bool allowScale) const
{
	//IMPROVE

	quint32 s = _forceSquarify(p_pixmap,GraphicsSettings::DiUiGraphicsSettings()->forceSquarifyUsesMaxBoundSquare);

	if (allowScale && (GraphicsSettings::DiUiGraphicsSettings()->allowedAsymmetricScalePercentage == 0))
		return s;

	//find the nearest smallest and nearest largest size designators of the existing atlas pages
	quint32 less=UINT_MAX;
	quint32 more=UINT_MAX;
	for (QMap<quint32,PixPagerPage *>::const_iterator it = m_atlasPages_alias.constBegin();
			it != m_atlasPages_alias.constEnd();++it)
	{
		if (it.key() < s)
			less = it.key();
		else if (it.key() >= s)
		{
			more = it.key();
			break;
		}
	}
	//try to scale up to the next highest, if available
	if (more != UINT_MAX)
	{
		if ((qreal)(more-s)*100.0 < (qreal)(GraphicsSettings::DiUiGraphicsSettings()->allowedAsymmetricScalePercentage) * (qreal)s)
			return more;	//we're good!
	}
	//try to scale down as a fallback
	if (less != UINT_MAX)
	{
		if ((qreal)(s-less)*100.0 < (qreal)(GraphicsSettings::DiUiGraphicsSettings()->allowedAsymmetricScalePercentage) * (qreal)s)
			return less;	//we're good!
	}

	//none found
	return s;
}

quint32 PixPager::_forceSquarify(QPixmap * p_pixmap,bool maxRect) const
{
	//IMPROVE
	//if maxRect is true, then pick the max of the two dimensions
	if (maxRect)
		return (quint32)qMax(p_pixmap->width(),p_pixmap->height());
	else if (p_pixmap->width() == p_pixmap->height())
		return (quint32)(p_pixmap->width());		//it's already square
	//just using the avg between the 2 dimensions, ceil-d up
	return (quint32)(qCeil(((qreal)(p_pixmap->width()+p_pixmap->height()))/2.0));

}

//quint32 largerSides = qMax(p_pixmap->size().height(),p_pixmap->size().width());
//static
bool PixPager::_isSquarePix(QPixmap * p_pixmap)
{
	return (p_pixmap->size().width() == p_pixmap->size().height());
}

PageOpsReturnCode::Enum PixPager::_addAtlasPixmapEntry(PixPagerAtlasPage& page,QPixmap * p_pixmap,quint32 sqSize,const QPair<quint32,quint32>& targetGridLocation,
												QUuid& r_insertedPixmapUid)
{
	QRect targetRect = page.targetRectForGridCoordinates(targetGridLocation);
	if (targetRect.width() == 0)	//sure sign of an error
		return PageOpsReturnCode::InvalidParameters;

	//remove from the free list if it's there
	page.m_freeList.removeOne(targetGridLocation);		//there must only be 1 entry; enforced by the _copyAndExpandAtlasPage
	//insert into this rect...this requires painting
	QPainter copier(*(page.m_data));
	copier.setCompositionMode(QPainter::CompositionMode_Source);
	copier.drawPixmap(targetRect,*p_pixmap);
	copier.end();
	//insert entry into the page directory
	r_insertedPixmapUid = QUuid::createUuid();
	page.m_directory.insert(r_insertedPixmapUid,PixPagerAtlasPage::PixmapRects(targetRect,p_pixmap->size()));
	//set the last allocated position
	page.m_lastPositionAllocated = targetGridLocation;
	return PageOpsReturnCode::OK;
}

PageOpsReturnCode::Enum PixPager::_createAndAddAtlasPageWithInitialEntry(QPixmap * p_pixmap,quint32 sqSize,quint32 initialRows,quint32 initialColumns,
										QUuid& r_pageUid,QUuid& r_insertedPixmapUid,bool allowExpunge)
{
	//Check to see if there is enough space to allocate one that's a square
	//nextpwr2(sqsize+padding)
	const quint32 width = (initialColumns*sqSize)+(initialColumns-1)*DEFAULT_ATLAS_PAGE_INTERCOL_SPACE+DEFAULT_ATLAS_PAGE_XLEADSPACE;
	const quint32 height = (initialRows*sqSize)+(initialRows-1)*DEFAULT_ATLAS_PAGE_INTERROW_SPACE+DEFAULT_ATLAS_PAGE_YLEADSPACE;

	const quint32 squarePageSideSize = nextpwr2(qMax(width+DEFAULT_ATLAS_PAGE_MIN_XTRAILSPACE,height+DEFAULT_ATLAS_PAGE_MIN_YTRAILSPACE));

	if (squarePageSideSize > INT_MAX)
		return PageOpsReturnCode::InvalidParameters;		//deal with the whole int<->uint overflow possibility, unlikely as it may be
															//(but hey, we want theoretically robust code, no?)
	if (squarePageSideSize > 1024)
		return PageOpsReturnCode::PageSizeExceedsMaxTextureSize;

	quint32 minSize = _expungeAmountForMinsize(PixmapObject::sizeOfPixmap(squarePageSideSize,squarePageSideSize));
	if (minSize > 0)
	{
		if (allowExpunge)
		{
			//not enough space...try and expunge something
			if (_findAndExpunge(minSize) == 0)
			{
				//didn't succeed in expunging enough
				return PageOpsReturnCode::PageSizeExceedsMaxCacheSize;
			}
		}
		else return PageOpsReturnCode::PageSizeExceedsMaxCacheSize;
	}

	//there is now enough space...
	//create the new pixmap for the page
	PixmapObject * pPmo = new PixmapObject((int)squarePageSideSize,(int)squarePageSideSize);	//safe cast, see above INT_MAX check

	PixPagerAtlasPage * pPage = new PixPagerAtlasPage(this,sqSize);
	(*pPmo)->fill(Qt::transparent);
	QPainter copier(*pPmo);
	copier.setCompositionMode(QPainter::CompositionMode_Source);
	//since this atlas page was created just for this pixmap, it's at the first possible position
	QRect position = QRect(DEFAULT_ATLAS_PAGE_XLEADSPACE,DEFAULT_ATLAS_PAGE_YLEADSPACE,sqSize,sqSize);
	copier.drawPixmap(position,*p_pixmap);
	pPage->m_rows = initialRows;
	pPage->m_columns = initialColumns;
	copier.end();
	pPage->m_data = pPmo;
	pPage->m_sizeInBytes = (quint32)pPmo->sizeOf();			//WARN: loss of numeric data possibly, but it doesn't matter much
	// since the pager doesn't track more than 2^32 maxSize anyways
	m_currentSizeInBytes += pPage->m_sizeInBytes;
	connect(pPmo,SIGNAL(signalObjectDestroyed()),pPage,SLOT(slotPixmapObjectDeleted()));

	//create the cache entries...
	///first the uid for the individual icon pixmap that was inserted
	r_insertedPixmapUid = QUuid::createUuid();
	///the entry into the page's directory for the rect of this icon
	pPage->m_directory.insert(r_insertedPixmapUid,PixPagerAtlasPage::PixmapRects(position,p_pixmap->size()));
	//and the page into the atlas aliases
	m_atlasPages_alias.insertMulti(sqSize,pPage);			//must be insertMulti!
	m_atlasPagesByIndividualUids_alias.insert(r_insertedPixmapUid,pPage);
	//and into the master for the cache
	m_pageCache.insert(pPmo->id(),pPage);
	//fill in the uid return param for the actual atlas page
	r_pageUid = pPmo->id();
	//set the last pos allocated
	pPage->m_lastPositionAllocated = QPair<quint32,quint32>(0,0);
	//all good!
	return PageOpsReturnCode::OK;

}

PageOpsReturnCode::Enum PixPager::_copyAndExpandAtlasPage(PixPagerAtlasPage * p_atlasPage,
								quint32 numRows,quint32 numColumns,bool makeSquarePage,QPair<quint32,quint32> * r_p_nextAvailableLocation)
{
	//optimally pixmaps of pages should be 2^n x 2^n , and ideally shouldn't be larger than 1024x1024
	// but having it be square will not be enforced here. i.e. it'll create 2^n x 2^m pages as long as both are <= 1024
	// if makeSquarePage == true, then it will try and make the page square
	//check to see if it can be reallocated at the given size
	//assumes that there is already at least 1 row and 1 column in the page

	if ((p_atlasPage->m_rows > numRows) || (p_atlasPage->m_columns > numColumns))
		return PageOpsReturnCode::InvalidParameters;
	const quint32 nextNewRow = ((p_atlasPage->m_rows == numRows) ? 0 : p_atlasPage->m_rows);		//not +1 because indexes are 0-based
	const quint32 nextNewCol = ((p_atlasPage->m_columns == numColumns) ? 0 : p_atlasPage->m_columns);
	const quint32 minNewWidth = ((numColumns*p_atlasPage->m_pixmapSqSize)+(numColumns-1)*p_atlasPage->m_interColPixelSpace
				+ p_atlasPage->m_xLeadingPixelSpace);
	const quint32 minNewHeight = ((numRows*p_atlasPage->m_pixmapSqSize)+(numRows-1)*p_atlasPage->m_interRowPixelSpace
				+ p_atlasPage->m_yLeadingPixelSpace);
	const quint32 minNewWidthP2 = nextpwr2(minNewWidth+DEFAULT_ATLAS_PAGE_MIN_XTRAILSPACE);
	const quint32 minNewHeightP2 = nextpwr2(minNewHeight+DEFAULT_ATLAS_PAGE_MIN_YTRAILSPACE);

	if ((minNewWidthP2 > 1024) || (minNewHeightP2 > 1024))
		return PageOpsReturnCode::PageSizeExceedsMaxTextureSize;

	quint32 newWidth = minNewWidthP2;
	quint32 newHeight = minNewHeightP2;
	if (makeSquarePage)
	{
		newWidth = qMax(minNewWidthP2,minNewHeightP2);
		newHeight = newWidth;
	}
	if ((newWidth > INT_MAX) || (newHeight > INT_MAX))
		return PageOpsReturnCode::InvalidParameters;		//deal with the whole int<->uint overflow possibility, unlikely as it may be
																//(but hey, we want theoretically robust code, no?)

	//create the new pixmap
	PixmapObject * pPmo = new PixmapObject((int)newWidth,(int)newHeight);	//safe, see above
	(*pPmo)->fill(Qt::transparent);
	QPainter copier(*pPmo);
	copier.setCompositionMode(QPainter::CompositionMode_Source);
	copier.drawPixmap(QPoint(0,0),*(*(p_atlasPage->m_data)),
			QRect(0,0,
	(p_atlasPage->m_columns)*(p_atlasPage->m_pixmapSqSize)+(p_atlasPage->m_columns -1)*(p_atlasPage->m_interColPixelSpace)+(p_atlasPage->m_yLeadingPixelSpace),
	(p_atlasPage->m_rows)*(p_atlasPage->m_pixmapSqSize)+(p_atlasPage->m_rows -1)*(p_atlasPage->m_interRowPixelSpace)+(p_atlasPage->m_xLeadingPixelSpace)
	)
					);

	const quint32 oldNumRows = p_atlasPage->m_rows;
	const quint32 oldNumColumns = p_atlasPage->m_columns;
	p_atlasPage->m_rows = numRows;
	p_atlasPage->m_columns = numColumns;
	copier.end();

	//
	//get rid of the old pixmap
	p_atlasPage->m_initiatedDelete = true;		//set temporarily to avoid p_atlasPage thinking someone else yanked the pixmap
												// (see the signal&slot for pixmap deletion)
	pPmo->setId(p_atlasPage->m_data->id());		//assume the identity of the old pixmap. Theoretically violates the Uuid principle but
												// if used carefully, it's fine this way
	delete p_atlasPage->m_data;
	p_atlasPage->m_data = pPmo;

	m_currentSizeInBytes += (quint32)pPmo->sizeOf() - p_atlasPage->m_sizeInBytes;
	//assumed that before this _copyAndExpandAtlasPage function was called, the space increase was checked against the total size
	p_atlasPage->m_sizeInBytes = (quint32)pPmo->sizeOf();			//WARN: loss of numeric data possibly, but it doesn't matter much
																	// since the pager doesn't track more than 2^32 maxSize anyways

	connect(pPmo,SIGNAL(signalObjectDestroyed()),p_atlasPage,SLOT(slotPixmapObjectDeleted()));

	p_atlasPage->m_initiatedDelete = false;		//reset it, we're done w/ the pixswap
	//note that the caches don't need to be re-keyed/modified in any way because the uid of the new pixmapobject is the same
	//as the old one, as are the uids of all the icon pixmaps inside the page.
	//return a new available position
	//new column first, then new row
	if (r_p_nextAvailableLocation)
	{
		if (nextNewCol != 0)
		{
			*r_p_nextAvailableLocation = QPair<quint32,quint32>(nextNewCol,0);
		}
		else //nextNewRow must be != 0
		{
			*r_p_nextAvailableLocation = QPair<quint32,quint32>(0,nextNewRow);
		}
	}
	//add the newly added cells to the free list
	for (quint32 i=oldNumColumns;i<numColumns;++i)
	{
		for (quint32 j=0;j<numRows;++j)
			p_atlasPage->m_freeList.append(QPair<quint32,quint32>(i,j));
	}
	for (quint32 j=oldNumRows;j<numRows;++j)
	{
		for (quint32 i=0;i<oldNumColumns;++i)
				p_atlasPage->m_freeList.append(QPair<quint32,quint32>(i,j));
	}
	return PageOpsReturnCode::OK;
}

//static
quint32 PixPager::nextpwr2(quint32 t)
{
	if (t == 0)
		return 1;
	--t;
	for (quint32 i=1; i<sizeof(quint32)*CHAR_BIT; i<<=1)
		t = t | t >> i;
	return t+1;
}
