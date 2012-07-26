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




#include "pixpagerdebugger.h"
#include "pixpager.h"
#include "gfxsettings.h"
#include "pixmapobject.h"

#include <QTextStream>
#include <QDebug>
#include <QString>
#include <QDateTime>
#include <QDir>
#include <QSize>
#include <QRect>

//static
PixPagerDebugger * PixPagerDebugger::newDebugger(PixPager * target)
{
	if (target == NULL)
		return NULL;

	return new PixPagerDebugger(target);
}

PixPagerDebugger::PixPagerDebugger(PixPager * target)
: m_qp_target(target)
{
}

QTextStream& operator<<(QTextStream& stream,const PixPagerPage::PixmapRects& r)
{
	stream << "[ " << r.coordinateRect.topRight().x()
			<< " , " << r.coordinateRect.topRight().y()
			<< " ; " << r.coordinateRect.width()
			<< " x " << r.coordinateRect.height()
			<< " ] "
			<< " original size: "
			<< r.originalSize.width()
			<< " x "
			<< r.originalSize.height();
	return stream;
}

QTextStream& operator<<(QTextStream& stream,const QSize& r)
{
	stream << " ( " << r.width() << " x " << r.height() << " )";
	return stream;
}

QTextStream& operator<<(QTextStream& stream,const QRect& r)
{
	stream << "[ " << r.topRight().x()
			<< " , " << r.topRight().y()
			<< " ; " << r.width()
			<< " x " << r.height()
			<< " ]";
	return stream;
}

void PixPagerDebugger::dumpPagesAsImagesToDisk(bool includeRegularPages,bool includeAtlasPages)
{
	if (m_qp_target.isNull())
	{
		//qDebug() << __FUNCTION__ << " failed: target is null";
		return;
	}

	QDateTime currentTime = QDateTime::currentDateTime();
	int i=1;
	QString basename = (GraphicsSettings::DiUiGraphicsSettings()->dbg_dumpFunctionsWriteableDirectory)
	+ currentTime.toString("hhmmss.zzz-dd-MM-yyyy");
	basename = QDir::cleanPath(basename);
	QDir cwd = QDir(basename);
	while (cwd.exists())
	{
		QString dirname = basename + "/" + QString::number(i) +"/page_images/";
		cwd = QDir(dirname);
		++i;
	}
	if (cwd.mkpath(cwd.absolutePath()) == false)
	{
		//qDebug() << __FUNCTION__ << " failed: couldn't create " + cwd.absolutePath();
		return;
	}

	//create a file that will store all of the image info
	QFile infoFile(cwd.absoluteFilePath("info.txt"));
	if (!infoFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		//qDebug() << __FUNCTION__ << " warning: failed to open " << infoFile.fileName();
	}

	QTextStream infoFileOut(&infoFile);

	//first some general info about the pager
	infoFileOut << "Pager Max Size: " << m_qp_target->m_maxSizeInBytes << "\n";
	infoFileOut << "Pager Current Size: " << m_qp_target->m_currentSizeInBytes << "\n";
	infoFileOut << "Pager Access Count: " << m_qp_target->m_accessCounter << "\n";
	infoFileOut << "\n--Page Stats--\n\n";
	//walk the pager's pages and create images for all the pages
	for (QHash<QUuid,PixPagerPage *>::const_iterator it = m_qp_target->m_pageCache.constBegin();
			it != m_qp_target->m_pageCache.constEnd(); ++it)
	{
		PixPagerPage * pPage = it.value();
		PixPagerAtlasPage * pAtlasPage = qobject_cast<PixPagerAtlasPage *>(pPage);

		if ((pAtlasPage == 0) && (includeRegularPages == false))
			continue;		//this is a regular page, which was requested to be excluded
		if ((pAtlasPage != 0) && (includeAtlasPages == false))
			continue;		//this is an atlas page, which was requested to be excluded

		infoFileOut << it.key();
		infoFileOut << " size: " << pPage->m_sizeInBytes;
		infoFileOut << " pinned: " << (pPage->m_pinned ? "true" : "false") << "\n";
		infoFileOut << " access count: " << pPage->m_accessCounter
					<< " access r-frequency: " << pPage->m_accessFrequency;
		if (pPage->m_data.isNull() == false)
		{
			infoFileOut << " pixmap dimensions: " << pPage->m_data->size();
		}
		infoFileOut << "\n";
		if (pAtlasPage != 0)
		{
			infoFileOut << "Atlas-info: square size: " << pAtlasPage->m_pixmapSqSize;
			infoFileOut << " free cells: " << pAtlasPage->freeSpace();
			infoFileOut << " occupancy: " << pAtlasPage->occupancyRate() << "\n";
			infoFileOut << "columns (x): " << pAtlasPage->m_columns
						<< " rows (y): " << pAtlasPage->m_rows << "\n";
			for (QHash<QUuid,PixPagerPage::PixmapRects>::const_iterator dir_it = pAtlasPage->m_directory.constBegin();
					dir_it != pAtlasPage->m_directory.constEnd();++dir_it)
			{
				infoFileOut << "\t" << dir_it.key()
						<< " coord: " << dir_it.value().coordinateRect
						<< " original size: " << dir_it.value().originalSize
						<< "\n";
			}
		}
		infoFileOut << "\n\n";
		//dump the image to disk
		if (pPage->m_data.isNull())
		{
			//qDebug() << __FUNCTION__ << " warning: pixmap for page " << it.key() << " is null...skipping image save";
			continue;
		}
		//the filename is just the <uid string> for regular pages,
		//and <uid string>-<atlas sq size> for atlas pages
		QString imgFilename = cwd.absolutePath()+"/"+it.key().toString();
		if (pAtlasPage)
			imgFilename += QString("-")+QString::number(pAtlasPage->m_pixmapSqSize);
		imgFilename += ".jpg";
		if ((*(pPage->m_data))->save(imgFilename,0,100) == false)
		{
			//qDebug() << __FUNCTION__ << " warning: could not write out pixmap for page " << it.key()
//									<< " to file " << imgFilename;
		}
	}
}
