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




#ifndef ICONHEAP_H_
#define ICONHEAP_H_

#include <QObject>
#include <QUuid>
#include <QString>
#include <QMap>
#include <QPointer>
#include <QList>
#include <QSize>

class PixmapObject;
class IconBase;

//IconAttributes is essentially just a public version of IconWrapper
// (use it to be able to filter out stuff returned to clients while being able to keep some
// data hidden in IconWrapper)
class IconAttributes
{
public:
	IconAttributes();
	IconAttributes(const QUuid& _uidOfOriginal, const QString& _appId, const QString& _launchPtId);
	QUuid uidOfOriginal;
	QString appId;
	QString launchPtId;
};

class IconWrapper
{
public:
	IconWrapper();
	IconWrapper(const QUuid& _uid,IconBase * p_icon);
	IconWrapper(const QUuid& _uid,const QString& _appId,const QString& _lptId,IconBase * p_icon);
	IconWrapper(const QUuid& _uid,const QString& _appId,const QString& _lptId,IconBase * p_icon,const QUuid& _originalUid);
	QUuid uid;
	QString appId;
	QString launchPtId;
	QPointer<IconBase> pIcon;
	QUuid copyOf;			//if not null, then the icon is a copy, and this is its original's uid
	QUuid locationTag;		// the page or other entity where the icon currently resides; this is JUST AN OPTIONAL HINT!
							// that's set voluntarily; it's mostly just for initial scan and page distribution of icons
							// for GEMSTONE-RD
	IconAttributes toAttribute() const;

};

namespace FindIconHint
{
	enum Enum
	{
		INVALID,
		Original,
		Copied
	};
}

namespace CommonlyUsedImages
{
	enum Enum
	{
		INVALID,
		ID_IconDecoratorRemoveNormal,
		ID_IconDecoratorRemovePressed,
		ID_IconDecoratorDeleteNormal,
		ID_IconDecoratorDeletePressed,
		ID_IconDecoratorWarning,
		ID_IconDecoratorProgressFilmstrip,
	};
}

class IconHeap : public QObject
{
	Q_OBJECT

public:

	static IconHeap * iconHeap();

	IconBase * getIcon(const QUuid& iconUid,FindIconHint::Enum searchHint = FindIconHint::Original);
//	IconBase * getIcon(const QString& appId);
	IconBase * getIcon(const QString& appId,const QString& launchPointId);

	IconBase * getIconEx(const QUuid& iconUid,IconAttributes& r_attr,FindIconHint::Enum searchHint = FindIconHint::Original);

	IconBase * copyIcon(const QUuid& iconUid);

	QList<IconBase *> findCopies(const QUuid& masterUid);

	bool	addIcon(IconBase * p_icon);
	bool	addIcon(IconBase * p_icon,const QString& appId,const QString& launchPointId = QString());

	void	annotateIcon(const QUuid& iconUid,const QString& appId,const QString& launchPointId = QString());
	bool	tagIconWithLocationUid(const QUuid& iconUid,const QUuid& tagUid,bool overwriteExistingTag=false);
	QUuid	iconLocationTag(const QUuid& iconUid);

	void 	deleteIconUnguarded(const QUuid& iconUid);

	//this one will only delete icons that are copies
	// TODO: make this one public, and protect the Unguarded variant
	void 	deleteIconCopy(const QUuid& copiedIconUid);

	///////TODO: possibly split these into a loader
	static IconBase * makeIcon(const QString& mainIconFilePath,const QString& frameIconFilePath,const QList<QString>& decoratorsFilePaths, const QString& feedbackIconFilePath);
	static IconBase * makeIconStandardFrameAndDecorators(const QString& mainIconFilePath);

	static IconBase * makeIconConstrained(const QString& mainIconFilePath,const QString& frameIconFilePath,
										const QList<QString>& decoratorsFilePaths, const QString& feedbackIconFilePath,
										const QSize& size,bool limitOnly=true);

	static IconBase * makeIconConstrainedStandardFrameAndDecorators(const QString& mainIconFilePath,const QSize& size,bool limitOnly=true);

	//// SOME COMMONLY USED ICON IMAGES
	// TODO: this belongs in a pixmap (pmo) heap. Since I'm running short on time and don't have any other need to write one at the moment
	//			i'll just do it here. But when the pixpager and a pixmapheap get added, move this code there

	void 	loadCommonlyUsedIconImages();

	//convenience accessors
	PixmapObject * commonImageRemoveDecoratorNormal() const;
	PixmapObject * commonImageRemoveDecoratorPressed() const;
	PixmapObject * commonImageDeleteDecoratorNormal() const;
	PixmapObject * commonImageDeleteDecoratorPressed() const;
	PixmapObject * commonImageWarningDecorator() const;

	//actually is a PixmapFilmstripObject...and a bit different in how it loads (see .cpp file)
	PixmapObject * commonImageProgressFilmstrip() const;


private:

	IconBase * find(const QUuid& iconUid);
	IconBase * findInCopies(const QUuid& iconUid);
	IconBase * find(const QString& combinedAppLpId);
	bool find(const QUuid& iconUid,IconWrapper& r_wrap);
	bool find(const QUuid& iconUid,IconWrapper** r_wrap);
	bool findInCopies(const QUuid& iconUid,IconWrapper& r_wrap);
	bool find(const QString& combinedAppLpId,IconWrapper& r_wrap);

	IconHeap();
	~IconHeap();

private:

	static QPointer<IconHeap> s_qp_instance;
	static QString s_standardFrameFilePath;
	static QString s_standardFeedbackFilePath;
	static QList<QString> s_standardDecoratorsFilePaths;

	typedef QMap<CommonlyUsedImages::Enum,QPointer<PixmapObject> >::const_iterator CommonIconImageNameMapConstIter;
	QMap<CommonlyUsedImages::Enum,QPointer<PixmapObject> > m_commonIconImagesMapById;

	typedef QMap<QUuid,IconWrapper>::iterator MainMapIter;
	QMap<QUuid,IconWrapper> m_mainMapByUid;
	typedef QMap<QString,IconWrapper>::iterator AliasMapIter;
	QMap<QString,IconWrapper> m_aliasMapByAppLaunchPointCombinedId;

	typedef QMap<QUuid,IconWrapper>::iterator CopyMapIter;
	QMap<QUuid,IconWrapper> m_copyMapByUid;		//copies of the icons of the main map go here.
												// it is the responsibility of the thing that requested the copy to maintain
												// the uid handle to it so it can request it to be cleaned out later when no
												// longer needed

};

#endif /* ICONHEAP_H_ */
