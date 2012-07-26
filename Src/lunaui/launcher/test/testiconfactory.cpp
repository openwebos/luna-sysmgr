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




#include "testiconfactory.h"
#include <QDir>
#include <QFileInfoList>
#include <QDebug>
#include <time.h>

namespace DimensionsUITest
{

IconFactory * IconFactory::s_instance = 0;

//static
IconFactory * IconFactory::instance()
{
	if (s_instance == 0)
		s_instance = new IconFactory();
	return s_instance;
}

//static
void IconFactory::destroy()
{
	//Callers had better connect()'ed to the destroyed signal of the static if this is ever used
	if (s_instance)
	{
		delete s_instance;
		s_instance = 0;
	}
}

QList<IconBase *> IconFactory::quickCreateFromDir(const QString& directoryPath)
{
	QList<IconBase *> rlist;
	QDir dir = QDir(directoryPath);
	dir.setFilter(QDir::Files | QDir::Hidden);
	QStringList filters;
	filters << "*.jpg" << "*.png";
	dir.setNameFilters(filters);
	QFileInfoList list = dir.entryInfoList();
	//qDebug() << "processing dir " << dir.absolutePath() << " , " << list.size() << " entries";
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		//qDebug() << qPrintable(QString("Processing %1").arg(fileInfo.fileName()));
		PixmapObject * pIconPix = loadPixmapObject(fileInfo.absoluteFilePath());
		if (pIconPix == 0)
		{
			continue;
		}
		IconBase * pIcon = IconBase::iconFromPix(0,pIconPix,0,0);
		if (pIcon)
		{
			pIcon->slotChangeIconVisibility(true);
			rlist.append(pIcon);
		}
	}
	return rlist;
}

QList<IconBase *> IconFactory::quickCreateFromDir(const QString& directoryPath,
										quint32 minWidth,quint32 maxWidth,
										quint32 minHeight,quint32 maxHeight,
										bool squareOnly)
{
	QList<IconBase *> rlist;
	QDir dir;
	dir.setFilter(QDir::Files | QDir::Hidden);
	QStringList filters;
	filters << "*.jpg" << "*.png";
	dir.setNameFilters(filters);
	QFileInfoList list = dir.entryInfoList();
	if (maxWidth == 0)
		maxWidth = (quint32)(-1);
	if (maxHeight == 0)
		maxHeight = (quint32)(-1);

	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		//qDebug() << qPrintable(QString("Processing %1").arg(fileInfo.fileName()));
		PixmapObject * pIconPix = loadPixmapObject(fileInfo.absoluteFilePath());
		if (pIconPix == 0)
		{
			continue;
		}
		//check against constraints
		if (squareOnly && !(pIconPix->isSquare()))
		{
			releasePixmapObject(fileInfo.absoluteFilePath());
			continue;
		}
		if (( (quint32)(pIconPix->size().width()) < minWidth) || ( (quint32)(pIconPix->size().width()) > maxWidth))
		{
			releasePixmapObject(fileInfo.absoluteFilePath());
			continue;
		}
		if (( (quint32)(pIconPix->size().height()) < minHeight) || ( (quint32)(pIconPix->size().height()) > maxHeight))
		{
			releasePixmapObject(fileInfo.absoluteFilePath());
			continue;
		}

		IconBase * pIcon = IconBase::iconFromPix(0,pIconPix,0,0);
		if (pIcon)
		{
			pIcon->slotChangeIconVisibility(true);
			rlist.append(pIcon);
		}
	}
	return rlist;
}

QList<IconBase *> IconFactory::quickCreateFromDirWithFrame(const QString& directoryPath,const QString& framePicAbsFilename)
{
	//don't load any of the other icons if the frame can't be loaded...this will help catch errors in test cases themselves
	//it works out ok because of the loaded map, which will only load the frame once

	PixmapObject * pFrameIconPix = loadPixmapObject(framePicAbsFilename);
	if (pFrameIconPix == 0)
	{
		return QList<IconBase *>();
	}
	QList<IconBase *> rlist = quickCreateFromDir(directoryPath);
	addFrameToIcons(framePicAbsFilename,rlist);
	return rlist;
}

QList<IconBase *> IconFactory::quickCreateFromDirWithFrame(const QString& directoryPath,const QString& framePicAbsFilename,
											quint32 minWidth,quint32 maxWidth,
											quint32 minHeight,quint32 maxHeight,
											bool squareOnly)
{
	//don't load any of the other icons if the frame can't be loaded...this will help catch errors in test cases themselves
	//it works out ok because of the loaded map, which will only load the frame once

	PixmapObject * pFrameIconPix = loadPixmapObject(framePicAbsFilename);
	if (pFrameIconPix == 0)
	{
		return QList<IconBase *>();
	}
	QList<IconBase *> rlist = quickCreateFromDir(directoryPath,minWidth,maxWidth,minHeight,maxHeight,squareOnly);
	addFrameToIcons(framePicAbsFilename,rlist);
	return rlist;
}

bool IconFactory::addFrameToIcons(const QString& framePicPath,QList<IconBase *>& icons)
{
	PixmapObject * pFrameIconPix = loadPixmapObject(framePicPath);
	if (pFrameIconPix == 0)
	{
		return false;
	}
	Q_FOREACH (IconBase * pIcon,icons)
	{
		if (pIcon)
		{
			PixmapObject * po = 0;
			pIcon->resize(pFrameIconPix->size());
			pIcon->slotUpdateIconFramePic(pFrameIconPix,po);
			pIcon->slotChangeIconFrameVisibility(true);
		}
	}
	return true;
}

bool IconFactory::addLabelsToIcons(const QString& wordFileAbsPath,const quint32 maxCharsPerWord,QList<IconBase *>& icons)
{
	//try and load the wordfile
	//expecting newline delimiting each "word" (so words can have spaces in them)

	QFile file(wordFileAbsPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	bool assignedOne = false;
	QTextStream in(&file);
	if (in.atEnd())
	{
		return false;		//empty file;
	}
	Q_FOREACH (IconBase * pIcon , icons)
	{
		QString line;
		do
		{
			if (in.atEnd())
			{
				if (!assignedOne)
				{
					//no words in the file...all blank lines
					return false;
				}
				//reset and start from beginning
				in.seek(0);
			}
			line = in.readLine().simplified();
			if (maxCharsPerWord > 0)
			{
				//trunc it
				line.truncate(maxCharsPerWord);
				line = line.simplified();	//just in case trunc created whitespace
			}
		}
		while (line.isEmpty());
		pIcon->setProperty(IconBase::IconLabelPropertyName,line);
		assignedOne = true;
	}
	return true;
}

bool IconFactory::addLabelsToIconsAtRandom(const QString& dictionaryFileAbsPath,
											const quint32 maxCharsPerWord,
											int randSeed,
											QList<IconBase *>& icons)
{
	//WARNING: potentially uses a lot of memory
	//expecting newline delimiting each "word" (so words can have spaces in them)
	//read the whole file into a QStringList
	QFile file(dictionaryFileAbsPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	if (randSeed > 0)
	{
		qsrand (randSeed);
	}
	else
	{
		qsrand((int)time(0));
	}

	QStringList wordList;
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine().simplified();
		if (maxCharsPerWord > 0)
		{
			//trunc it
			line.truncate(maxCharsPerWord);
			line = line.simplified();	//just in case trunc created whitespace
		}
		if (line.isEmpty())
		{
			continue;
		}
		wordList << line;
	}
	if (wordList.empty())
	{
		return false;		//no words in the file
	}
	Q_FOREACH (IconBase * pIcon , icons)
	{
		//pick a random line
		QString randLine = wordList.at(qrand() % wordList.size());
		pIcon->setProperty(IconBase::IconLabelPropertyName,randLine);
	}

	return true;
}

//public Q_SLOTS:

void IconFactory::slotPmoDestroyed(QObject * p_qobject)
{
	PixmapObject * pPmo =  qobject_cast<PixmapObject *>(p_qobject);
	if (pPmo == 0)
	{
		return;	//either not pmo (??!?) or it was null
	}
	for (LoadedPmoMapIter it = m_loadedPmoMap.begin();
			it != m_loadedPmoMap.end();++it)
	{
		if (it->data() == pPmo)
		{
			m_loadedPmoMap.erase(it);
			return;
		}
	}
}

//protected:

//virtual
IconFactory::~IconFactory()
{
	for (LoadedPmoMapConstIter it = m_loadedPmoMap.constBegin();
			it != m_loadedPmoMap.constEnd();++it)
	{
		delete it.value();
	}
}

PixmapObject * IconFactory::loadPixmapObject(const QString& absFilePath)
{
	LoadedPmoMapConstIter f = m_loadedPmoMap.constFind(absFilePath);
	if (f != m_loadedPmoMap.constEnd())
	{
		if (!(f.value().isNull()))
		{
			return f.value();
		}
	}
	PixmapObject * pIconPix = new PixmapObject(absFilePath);
	if (pIconPix->data()->isNull())
	{
		delete pIconPix;
		return 0;
	}
	connect(pIconPix,SIGNAL(destroyed(QObject *)),this,SLOT(slotPmoDestroyed(QObject *)));
	m_loadedPmoMap.insert(absFilePath,QPointer<PixmapObject>(pIconPix));
	return pIconPix;
}

void IconFactory::releasePixmapObject(PixmapObject * p_pix)
{
	LoadedPmoMapIter it = m_loadedPmoMap.begin();
	for (;it != m_loadedPmoMap.end();++it)
	{
		if (it->data() == p_pix)
		{
			delete it.value();
			break;
		}
	}
	if (it != m_loadedPmoMap.end())
	{
		m_loadedPmoMap.erase(it);
	}
}

void IconFactory::releasePixmapObject(const QString& absFilePath)
{
	LoadedPmoMapIter f = m_loadedPmoMap.find(absFilePath);
	if (f != m_loadedPmoMap.end())
	{
		if (!(f.value().isNull()))
		{
			delete f.value();
		}
		m_loadedPmoMap.erase(f);
	}
}

} //end namespace
