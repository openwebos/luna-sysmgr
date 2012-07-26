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




#include "iconheap.h"
#include "icon.h"
#include "pixmaploader.h"
#include "gfxsettings.h"
#include "stringtranslator.h"

#include "Settings.h"

QPointer<IconHeap> IconHeap::s_qp_instance = 0;
QString IconHeap::s_standardFrameFilePath = QString();
QString IconHeap::s_standardFeedbackFilePath = QString();
QList<QString> IconHeap::s_standardDecoratorsFilePaths = QList<QString>();

#define COMMONIMAGE_DECORATOR_REMOVE_MULTI_FILENAME	QString("edit-button-remove.png")
#define COMMONIMAGE_DECORATOR_REMOVE_MULTI_NORMALRECT	QRect(2,1,32,32)
#define COMMONIMAGE_DECORATOR_REMOVE_MULTI_ACTIVERECT	QRect(2,41,32,32)

#define COMMONIMAGE_DECORATOR_DELETE_MULTI_FILENAME	QString("edit-button-delete.png")
#define COMMONIMAGE_DECORATOR_DELETE_MULTI_NORMALRECT	QRect(2,1,32,32)
#define COMMONIMAGE_DECORATOR_DELETE_MULTI_ACTIVERECT	QRect(2,41,32,32)

#define COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_FILESRCDIR (StringTranslator::inputString(Settings::LunaSettings()->lunaSystemResourcesPath + "/"))
#define COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_FILENAME QString("loading-strip.png")
#define COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_FRAMESIZE QSize(32,32)
#define COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_STARTOFFSET QPoint(0,0)
#define COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_NUMFRAMES 19
#define COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_PROGRESSIVEFRAMEDIRECTION	(FrameDirection::South)

#define COMMONIMAGE_DECORATOR_WARNING_FILESRCDIR (StringTranslator::inputString(Settings::LunaSettings()->lunaSystemResourcesPath + "/"))
#define COMMONIMAGE_DECORATOR_WARNING_FILENAME QString("warning-icon.png")
///public:

//static
IconHeap * IconHeap::iconHeap()
{
	if (!s_qp_instance)
	{
		//initialize the other complex statics
		//TODO: CUSTOMIZE: defaulted for now
		s_standardFrameFilePath = GraphicsSettings::settings()->graphicsAssetBaseDirectory + QString("/edit-icon-bg.png");
		s_standardFeedbackFilePath = GraphicsSettings::settings()->graphicsAssetBaseDirectory + QString("/launcher-touch-feedback.png");
		s_qp_instance = new IconHeap();
	}
	return s_qp_instance;
}

IconBase * IconHeap::getIcon(const QUuid& iconUid,FindIconHint::Enum searchHint)
{
	if (searchHint == FindIconHint::Copied)
	{
		IconBase * pI = findInCopies(iconUid);
		if (!pI)
		{
			pI = find(iconUid);
		}
		return pI;
	}
	else
	{
		IconBase * pI = find(iconUid);
		if (!pI)
		{
			pI = findInCopies(iconUid);
		}
		return pI;
	}
	return 0;	//never reached
}

IconBase * IconHeap::getIconEx(const QUuid& iconUid,IconAttributes& r_attr,FindIconHint::Enum searchHint)
{
	if (searchHint == FindIconHint::Copied)
	{
		IconWrapper iw;
		if (!findInCopies(iconUid,iw))
		{
			if (!find(iconUid,iw))
			{
				return 0;
			}
		}
		r_attr = iw.toAttribute();
		return iw.pIcon;
	}
	else
	{
		IconWrapper iw;
		if (!find(iconUid,iw))
		{
			if (!findInCopies(iconUid,iw))
			{
				return 0;
			}
		}
		r_attr = iw.toAttribute();
		return iw.pIcon;
	}
	return 0;	//never reached
}

//IconBase * IconHeap::getIcon(const QString& appId)
//{
//	return find(appId);
//}

IconBase * IconHeap::getIcon(const QString& appId,const QString& launchPointId)
{
	return find(appId+launchPointId);
}

IconBase * IconHeap::copyIcon(const QUuid& iconUid)
{
	//get the master icon, including its wrapper
	// Important to note that only main/master icons can be copied. Cannot make a copy-of-a-copy
	IconWrapper iw;
	if (!find(iconUid,iw))
	{
		return 0;
	}
	IconBase * pI = iw.pIcon;
	if (!pI)
	{
		//serious error! not fatal at this pt though so just bail
		return 0;
	}

	//clone it
	IconBase * pClone = pI->clone();
	if (!pClone)
	{
		return 0;
	}
	m_copyMapByUid.insert(pClone->uid(),IconWrapper(pClone->uid(),iw.appId,iw.launchPtId,pClone,iw.uid));
	return pClone;
}

QList<IconBase *> IconHeap::findCopies(const QUuid& masterUid)
{
	//TODO: SLOW

	QList<IconBase *> rlist;
	for (CopyMapIter it = m_copyMapByUid.begin();
			it != m_copyMapByUid.end();++it)
	{
		if (it->copyOf == masterUid)
		{
			rlist << it->pIcon;
		}
	}
	return rlist;
}

bool IconHeap::addIcon(IconBase * p_icon)
{
	if (!p_icon)
	{
		return false;
	}
	if (find(p_icon->uid()))
	{
		return false;	//don't accept duplicates
	}
	m_mainMapByUid.insert(p_icon->uid(),IconWrapper(p_icon->uid(),p_icon));
	return true;
}

#include <QDebug>
bool IconHeap::addIcon(IconBase * p_icon,const QString& appId,const QString& launchPointId)
{
	if (!p_icon)
	{
		return false;
	}
	if (find(p_icon->uid()))
	{
		return false;	//don't accept duplicates
	}
	m_mainMapByUid.insert(p_icon->uid(),IconWrapper(p_icon->uid(),appId,launchPointId,p_icon));
	m_aliasMapByAppLaunchPointCombinedId.insert(appId+launchPointId,IconWrapper(p_icon->uid(),appId,launchPointId,p_icon));
	return true;
}

void IconHeap::annotateIcon(const QUuid& iconUid,const QString& appId,const QString& launchPointId)
{
	//find by uid
	IconWrapper iw;
	if (!find(iconUid,iw))
	{
		return;
	}
	if (!iw.appId.isEmpty())
	{
		//remove it from the alias map under its old appid+lpid
		m_aliasMapByAppLaunchPointCombinedId.remove(iw.appId+iw.launchPtId);
	}
	m_mainMapByUid.insert(iconUid,IconWrapper(iconUid,appId,launchPointId,iw.pIcon));
	m_aliasMapByAppLaunchPointCombinedId.insert(appId+launchPointId,IconWrapper(iconUid,appId,launchPointId,iw.pIcon));
}

bool IconHeap::tagIconWithLocationUid(const QUuid& iconUid,const QUuid& tagUid,bool overwriteExistingTag)
{
	//find by uid
	IconWrapper* iw = 0;
	if (!find(iconUid,&iw))
	{
		return false;
	}

	if (!iw)
		return false; 
	
	if ((!(iw->locationTag.isNull())) && (!overwriteExistingTag))
	{
		return false;
	}
	iw->locationTag = tagUid;
	return true;
}

QUuid IconHeap::iconLocationTag(const QUuid& iconUid)
{
	//find by uid
	IconWrapper iw;
	if (!find(iconUid,iw))
	{
		return QUuid();
	}
	return iw.locationTag;
}

//TODO: delete functions can be improved; the find result can be used for remove() to avoid a second map search
void IconHeap::deleteIconUnguarded(const QUuid& iconUid)
{
	IconWrapper iw;
	if (!find(iconUid,iw))
	{
		return;
	}
	if (!iw.appId.isEmpty())
	{
		//remove it from the alias map under its appid+lpid
		m_aliasMapByAppLaunchPointCombinedId.remove(iw.appId+iw.launchPtId);
	}
	m_mainMapByUid.remove(iconUid);
	delete iw.pIcon;
}

void IconHeap::deleteIconCopy(const QUuid& copiedIconUid)
{
	IconWrapper iw;
	if (!findInCopies(copiedIconUid,iw))
	{
		return;
	}
	m_copyMapByUid.remove(copiedIconUid);
	delete iw.pIcon;
}

#include <QDebug>
//TODO: WARNING: THIS WILL LEAK MEMORY IF ICONS ARE REMOVED! PixmapLoader doesn't manage the PixmapObject-s
// (this is what the pixpager will be doing; it's coming online much later though)
//static
IconBase * IconHeap::makeIcon(const QString& mainIconFilePath,const QString& frameIconFilePath,const QList<QString>& decoratorsFilePaths, const QString& feedbackIconFilePath)
{
	PixmapObject * pMainIconPmo = PixmapObjectLoader::instance()->quickLoad(mainIconFilePath);
	//if the main icon couldn't load, then it's an immediate fail
	if (!pMainIconPmo)
	{
		return 0;
	}
	PixmapObject * pFrameIconPmo = PixmapObjectLoader::instance()->quickLoad(frameIconFilePath);
	if (!pFrameIconPmo)
	{
		return 0;
		//TODO: MEMLEAK: pMainIconPmo?????
	}
	PixmapObject * pLaunchFeedbackPmo = PixmapObjectLoader::instance()->quickLoad(feedbackIconFilePath);
	if (!pLaunchFeedbackPmo)
	{
		return 0;
		//TODO: MEMLEAK: pMainIconPmo?????
	}
	//create the icon.
	IconBase * pIcon = IconBase::iconFromPix(pFrameIconPmo,pMainIconPmo,pLaunchFeedbackPmo,0);
	pIcon->slotChangeIconFrameVisibility(false);

	return pIcon;
}

//static
IconBase * IconHeap::makeIconStandardFrameAndDecorators(const QString& mainIconFilePath)
{
	return makeIcon(mainIconFilePath,s_standardFrameFilePath,QList<QString>(), s_standardFeedbackFilePath);
}

IconBase * IconHeap::makeIconConstrained(const QString& mainIconFilePath,const QString& frameIconFilePath,
							const QList<QString>& decoratorsFilePaths, const QString& feedbackIconFilePath,
							const QSize& size,bool limitOnly)
{
	PixmapObject * pMainIconPmo = PixmapObjectLoader::instance()->quickLoad(mainIconFilePath,size,limitOnly);
	//if the main icon couldn't load, then it's an immediate fail
	if (!pMainIconPmo)
	{
		return 0;
	}
	PixmapObject * pFrameIconPmo = PixmapObjectLoader::instance()->quickLoad(frameIconFilePath);
	if (!pFrameIconPmo)
	{
		return 0;
		//TODO: MEMLEAK: pMainIconPmo?????
	}
	PixmapObject * pLaunchFeedbackPmo = PixmapObjectLoader::instance()->quickLoad(feedbackIconFilePath);
	if (!pLaunchFeedbackPmo)
	{
		return 0;
		//TODO: MEMLEAK: pMainIconPmo?????
	}
	//create the icon.
	IconBase * pIcon = IconBase::iconFromPix(pFrameIconPmo,pMainIconPmo,pLaunchFeedbackPmo,0);
	pIcon->slotChangeIconFrameVisibility(false);

	return pIcon;
}

//static
IconBase * IconHeap::makeIconConstrainedStandardFrameAndDecorators(const QString& mainIconFilePath,const QSize& size,bool limitOnly)
{
	return makeIconConstrained(mainIconFilePath,s_standardFrameFilePath,QList<QString>(), s_standardFeedbackFilePath,size,limitOnly);
}

////private:

inline IconBase * IconHeap::find(const QUuid& iconUid)
{
	MainMapIter it = m_mainMapByUid.find(iconUid);
	if (it != m_mainMapByUid.end())
	{
		return it.value().pIcon;
	}
	return 0;
}

inline IconBase * IconHeap::findInCopies(const QUuid& iconUid)
{
	CopyMapIter it = m_copyMapByUid.find(iconUid);
	if (it != m_copyMapByUid.end())
	{
		return it.value().pIcon;
	}
	return 0;
}

inline IconBase * IconHeap::find(const QString& combinedAppLpId)
{
	AliasMapIter it = m_aliasMapByAppLaunchPointCombinedId.find(combinedAppLpId);
	if (it != m_aliasMapByAppLaunchPointCombinedId.end())
	{
		return it.value().pIcon;
	}
	return 0;
}

bool IconHeap::find(const QUuid& iconUid,IconWrapper& r_wrap)
{
	MainMapIter it = m_mainMapByUid.find(iconUid);
	if (it != m_mainMapByUid.end())
	{
		r_wrap = it.value();
		return true;
	}
	return false;
}

bool IconHeap::find(const QUuid& iconUid,IconWrapper** r_wrap)
{
	MainMapIter it = m_mainMapByUid.find(iconUid);
	if (it != m_mainMapByUid.end())
	{
		*r_wrap = &(it.value());
		return true;
	}
	return false;
}

bool IconHeap::findInCopies(const QUuid& iconUid,IconWrapper& r_wrap)
{
	CopyMapIter it = m_copyMapByUid.find(iconUid);
	if (it != m_copyMapByUid.end())
	{
		r_wrap = it.value();
		return true;
	}
	return false;
}

bool IconHeap::find(const QString& combinedAppLpId,IconWrapper& r_wrap)
{
	AliasMapIter it = m_aliasMapByAppLaunchPointCombinedId.find(combinedAppLpId);
	if (it != m_aliasMapByAppLaunchPointCombinedId.end())
	{
		r_wrap = it.value();
		return true;
	}
	return false;
}

IconHeap::IconHeap()
{
	loadCommonlyUsedIconImages();
}

IconHeap::~IconHeap()
{
	//TODO:  dispose of commonly used images
}

IconWrapper::IconWrapper() : pIcon(0) {}
IconWrapper::IconWrapper(const QUuid& _uid,IconBase * p_icon)
		: uid(_uid) , pIcon(p_icon) {}
IconWrapper::IconWrapper(const QUuid& _uid,const QString& _appId,const QString& _lptId,IconBase * p_icon)
	: uid(_uid) , appId(_appId) , launchPtId(_lptId) , pIcon(p_icon) {}
IconWrapper::IconWrapper(const QUuid& _uid,const QString& _appId,const QString& _lptId,IconBase * p_icon,const QUuid& _originalUid)
	: uid(_uid) , appId(_appId) , launchPtId(_lptId) , pIcon(p_icon) , copyOf(_originalUid) {}
IconAttributes::IconAttributes() {}
IconAttributes::IconAttributes(const QUuid& _uidOfOriginal, const QString& _appId, const QString& _launchPtId)
	: uidOfOriginal(_uidOfOriginal)
	, appId(_appId)
	, launchPtId(_launchPtId) {}

IconAttributes IconWrapper::toAttribute() const
{
	return IconAttributes(copyOf,appId,launchPtId);
}

/////////////////// TODO: to be relocated (see .h file) ////////////////////

void IconHeap::loadCommonlyUsedIconImages()
{
	QList<QRect> regionList;
	regionList << COMMONIMAGE_DECORATOR_REMOVE_MULTI_NORMALRECT << COMMONIMAGE_DECORATOR_REMOVE_MULTI_ACTIVERECT;
	QList<PixmapObject *> pmoList =
			PixmapObjectLoader::instance()->loadMulti(
					regionList,GraphicsSettings::settings()->graphicsAssetBaseDirectory + COMMONIMAGE_DECORATOR_REMOVE_MULTI_FILENAME);

	if (!pmoList.isEmpty())
	{
		m_commonIconImagesMapById[CommonlyUsedImages::ID_IconDecoratorRemoveNormal] = QPointer<PixmapObject>(pmoList.at(0));
		m_commonIconImagesMapById[CommonlyUsedImages::ID_IconDecoratorRemovePressed] = QPointer<PixmapObject>(pmoList.at(1));
	}
	regionList.empty();
	regionList << COMMONIMAGE_DECORATOR_DELETE_MULTI_NORMALRECT << COMMONIMAGE_DECORATOR_DELETE_MULTI_ACTIVERECT;
	pmoList =
			PixmapObjectLoader::instance()->loadMulti(
					regionList,GraphicsSettings::settings()->graphicsAssetBaseDirectory + COMMONIMAGE_DECORATOR_DELETE_MULTI_FILENAME);

	if (!pmoList.isEmpty())
	{
		m_commonIconImagesMapById[CommonlyUsedImages::ID_IconDecoratorDeleteNormal] = QPointer<PixmapObject>(pmoList.at(0));
		m_commonIconImagesMapById[CommonlyUsedImages::ID_IconDecoratorDeletePressed] = QPointer<PixmapObject>(pmoList.at(1));
	}

	m_commonIconImagesMapById[CommonlyUsedImages::ID_IconDecoratorWarning] =
			QPointer<PixmapObject>(PixmapObjectLoader::instance()->quickLoad
			(COMMONIMAGE_DECORATOR_WARNING_FILESRCDIR + COMMONIMAGE_DECORATOR_WARNING_FILENAME));
}

PixmapObject * IconHeap::commonImageRemoveDecoratorNormal() const
{
	CommonIconImageNameMapConstIter it = m_commonIconImagesMapById.constFind(CommonlyUsedImages::ID_IconDecoratorRemoveNormal);
	if (it != m_commonIconImagesMapById.constEnd())
	{
		return *it;
	}
	return 0;
}

PixmapObject * IconHeap::commonImageRemoveDecoratorPressed() const
{
	CommonIconImageNameMapConstIter it = m_commonIconImagesMapById.constFind(CommonlyUsedImages::ID_IconDecoratorRemovePressed);
	if (it != m_commonIconImagesMapById.constEnd())
	{
		return *it;
	}
	return 0;
}

PixmapObject * IconHeap::commonImageDeleteDecoratorNormal() const
{
	CommonIconImageNameMapConstIter it = m_commonIconImagesMapById.constFind(CommonlyUsedImages::ID_IconDecoratorDeleteNormal);
	if (it != m_commonIconImagesMapById.constEnd())
	{
		return *it;
	}
	return 0;
}
PixmapObject * IconHeap::commonImageDeleteDecoratorPressed() const
{
	CommonIconImageNameMapConstIter it = m_commonIconImagesMapById.constFind(CommonlyUsedImages::ID_IconDecoratorDeletePressed);
	if (it != m_commonIconImagesMapById.constEnd())
	{
		return *it;
	}
	return 0;
}

PixmapObject * IconHeap::commonImageWarningDecorator() const
{
	CommonIconImageNameMapConstIter it = m_commonIconImagesMapById.constFind(CommonlyUsedImages::ID_IconDecoratorWarning);
	if (it != m_commonIconImagesMapById.constEnd())
	{
		return *it;
	}
	return 0;
}

PixmapObject * IconHeap::commonImageProgressFilmstrip() const
{
	//This one cannot be loaded like the others because it cannot be shared; it's internal state will be modified
	// by its "client icon". The effect of this would be the progress filmstrip changing for all icons the same way

	//TODO: should return a default image of some sort if the filmstrip load fails
		return PixmapObjectLoader::instance()->quickLoadFilmstrip
	    (     COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_FRAMESIZE,
	    		COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_NUMFRAMES,
	    		COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_PROGRESSIVEFRAMEDIRECTION,
	    		COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_FILESRCDIR + COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_FILENAME,
	    		COMMONIMAGE_DECORATOR_PROGRESS_FILMSTRIP_STARTOFFSET
	    );
}
