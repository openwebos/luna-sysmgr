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




#ifndef TESTICONFACTORY_H_
#define TESTICONFACTORY_H_

#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QPointer>
#include "icon.h"

namespace DimensionsUITest
{

class IconFactory : public QObject
{
	Q_OBJECT
public:

	static IconFactory * instance();
	static void destroy();

	QList<IconBase *> quickCreateFromDir(const QString& directoryPath);
	QList<IconBase *> quickCreateFromDir(const QString& directoryPath,
										quint32 minWidth,quint32 maxWidth,
										quint32 minHeight,quint32 maxHeight,
										bool squareOnly);

	QList<IconBase *> quickCreateFromDirWithFrame(const QString& directoryPath,const QString& framePicAbsFilename);
	QList<IconBase *> quickCreateFromDirWithFrame(const QString& directoryPath,const QString& framePicAbsFilename,
											quint32 minWidth,quint32 maxWidth,
											quint32 minHeight,quint32 maxHeight,
											bool squareOnly);

	//modifies in place...returns true if successfully applied frame to everyone
	bool addFrameToIcons(const QString& framePicPath,QList<IconBase *>& icons);

	//modifies in place...returns true if successfully applied label to everyone
	// maxCharsPerWord = 0 -> unlimited word size
	// does not guarantee uniqueness of labels...will wrap around if the word file doesn't have enough
	// usable lines
	bool addLabelsToIcons(const QString& wordFileAbsPath,const quint32 maxCharsPerWord,QList<IconBase *>& icons);
	//randSeed is there for repeatability. use 0 to use time()
	bool addLabelsToIconsAtRandom(const QString& dictionaryFileAbsPath,const quint32 maxCharsPerWord,int randSeed,QList<IconBase *>& icons);

public Q_SLOTS:

	void slotPmoDestroyed(QObject * p_qobject);

protected:

	virtual ~IconFactory();

	PixmapObject * loadPixmapObject(const QString& absFilePath);
	void releasePixmapObject(PixmapObject * p_pix);		//oh yeah, v.dangerous
	void releasePixmapObject(const QString& absFilePath);

	static IconFactory * s_instance;
	typedef QMap<QString,QPointer<PixmapObject> >::const_iterator LoadedPmoMapConstIter;
	typedef QMap<QString,QPointer<PixmapObject> >::iterator LoadedPmoMapIter;
	QMap<QString,QPointer<PixmapObject> > m_loadedPmoMap;
};

} //end namespace

#endif /* TESTICONFACTORY_H_ */
