/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef APPLICATIONDESCRIPTION_H
#define APPLICATIONDESCRIPTION_H

#include "Common.h"

#include <string>
#include <stdint.h>
#include <set>
#include <list>

#include "LaunchPoint.h"
#include "KeywordMap.h"
#include "CmdResourceHandlers.h"

struct json_object;
struct ApplicationStatus;

class SysmgrBuiltinLaunchHelper : public QObject
{
	Q_OBJECT
public:

	SysmgrBuiltinLaunchHelper(const std::string& args)
	: m_builtIn_argsAsStringEncodedJson(args) {}

	void launch()
	{
		Q_EMIT signalEntry(m_builtIn_argsAsStringEncodedJson);
	}
	void launch(const std::string& overrideArgs)
	{
		Q_EMIT signalEntry(std::string(overrideArgs));
	}
	void launchNoArgs()
	{
		Q_EMIT signalEntry(std::string(""));
	}

	Q_SIGNALS:
		void signalEntry(const std::string& argsAsStringEncodedJson);
public:

	std::string					m_builtIn_argsAsStringEncodedJson;
};

class ApplicationDescription
{
public:

	enum Type {
		Type_Web = 0,
		Type_Native, 
		Type_PDK,
		Type_SysmgrBuiltin,
        Type_Qt
	};

	enum Status {
		Status_Ready = 0,
		Status_Updating,
		Status_Installing,
		Status_Failed
	};

	enum HardwareFeaturesNeeded {
		HardwareFeaturesNeeded_None          = 0,
		HardwareFeaturesNeeded_Wifi          = 1 << 0,
		HardwareFeaturesNeeded_Bluetooth     = 1 << 1,
		HardwareFeaturesNeeded_Compass       = 1 << 2,
		HardwareFeaturesNeeded_Accelerometer = 1 << 3,
		HardwareFeaturesNeeded_Last          = 1 << 31
	};

	ApplicationDescription();
	~ApplicationDescription();

	static ApplicationDescription* fromFile(const std::string& filePath, const std::string& folderPath);
	static ApplicationDescription* fromJsonString(const char* jsonStr);
	static ApplicationDescription* fromApplicationStatus(const ApplicationStatus& appStatus, bool isUpdating);
	static ApplicationDescription* fromNativeDockApp(const std::string& id, const std::string& title, 
						const std::string& version, const std::string& splashIcon,
						const std::string& splashBackgroundName, const std::string& miniicon,
						const std::string& vendor, const std::string& vendorUrl,
						const std::string& appmenu);
	static std::string	   versionFromFile(const std::string& filePath, const std::string& folderPath);
	
	const std::string& id()         const { return m_id; }
	const std::string& title()         const { return m_title; }
	const std::string& menuName()		const { return m_appmenuName; }
	const std::string& category()   const { return m_category; } 
	std::list<std::string> keywords() const { return m_keywords.allKeywords(); }
	const std::string& entryPoint() const { return m_entryPoint; }
	const std::string& version()    const { return m_version; }
	bool               isHeadLess() const { return m_isHeadLess; }
	bool               hasTransparentWindows() const { return m_hasTransparentWindows; }
	bool			   isRemovable() const { return m_isRemovable; }
    bool               handlesRelaunch() const { return m_handlesRelaunch; }
	bool			   isUserHideable() const { return m_isUserHideable; }
	bool			   isVisible() const { return m_isVisible; }
	const std::string& folderPath() const { return m_folderPath; }
	Type			   type() const { return m_type; }
	Status			   status() const { return m_status; }
	int 			   progress() const { return m_progress; }
	const std::string& attributes() const { return m_attributes; }
	bool			   hasAccounts() const { return m_hasAccounts; }

	bool dockModeStatus() const { return m_dockMode; }
	const std::string&  dockModeTitle() const { return m_dockModeTitle; }
	
	const std::string& miniIconUrl() const {return m_miniIconName;}
	const std::string& vendorName() const {return m_vendorName;}
	const std::string& vendorUrl() const { return m_vendorUrl;}
	uint64_t appSize() const {return m_appSize;}
	void setAppSize(const uint64_t& s) { m_appSize = s;}
	uint32_t blockSize() const { return m_fsBlockSize; }
	void setBlockSize(uint32_t s) { m_fsBlockSize = s;}

	unsigned int runtimeMemoryRequired() const {return m_runtimeMemoryRequired;}

	QPixmap miniIcon() const;

	const std::string& splashIconName() const {
		return m_splashIconName;
	}
    const std::string &splashBackgroundName() const {
        return m_splashBackgroundName;
    }

    const bool launchInNewGroup() const {
    	return m_launchInNewGroup;
    }

	const std::list<ResourceHandler>& mimeTypes() const;
	const std::list<RedirectHandler>& redirectTypes() const;
	
	const LaunchPointList&        launchPoints() const;
	void						  launchPoints(LaunchPointList& launchPointList);		//copy version of launchPoints(). Useful for add/remove of stuff from the list

	void               addLaunchPoint(LaunchPoint* lp);
	const LaunchPoint* findLaunchPoint(const std::string& lpId);
	const LaunchPoint* getDefaultLaunchPoint() const;
	void               removeLaunchPoint(const LaunchPoint* lp);

	// NOTE: it is the callers responsibility to json_object_put the return value	
	json_object* toJSON() const;

	std::string toString() const;

	bool canExecute() const { return !m_executionLock; }
	void executionLock(bool xp=true) { m_executionLock = xp;}

	bool isRemoveFlagged() const { return m_flaggedForRemoval;}
	void flagForRemoval(bool rf=true) { m_flaggedForRemoval = rf;}
	bool setRemovable(bool v=true);
	bool setVisible(bool v=true);
	void setVersion(const std::string& version) { m_version = version; }

	uint32_t hardwareFeaturesNeeded() const { return m_hardwareFeaturesNeeded; }

	// NOTE: only applications which reside in ROM (/usr/palm/applications) 
	// should set this flag to true
	void setUserHideable(bool hideable) { m_isUserHideable = hideable; }

	void setStatus(Status newStatus) { m_status = newStatus; }

	void setHasAccounts(bool hasAccounts) { m_hasAccounts = hasAccounts; }

	bool tapToShareSupported() const  { return m_tapToShareSupported; }

	std::string requestedWindowOrientation() { return m_requestedWindowOrientation; }

	bool operator==(const ApplicationDescription& cmp) const;
	bool operator!=(const ApplicationDescription& cmp) const;

	bool strictCompare(const ApplicationDescription& cmp) const;

	void update(const ApplicationStatus& appStatus, bool isUpdating);
	int  update(const ApplicationDescription& appDesc);
	
	bool	doesMatchKeywordExact(const gchar* keyword) const;
	bool	doesMatchKeywordPartial(const gchar* keyword) const;
	
	void getAppDescriptionString(std::string &descString) const;

	void startSysmgrBuiltIn(const std::string& jsonArgsString) const;
	void startSysmgrBuiltIn() const;
	void startSysmgrBuiltInNoArgs() const;

	bool initSysmgrBuiltIn(QObject * pReceiver,const std::string& entrypt,const std::string& args);

	void dbgSetProgressManually(int progv) { m_progress = progv; }

    bool securityChecksVerified();

private:

	class MimeRegInfo {
	public:
		MimeRegInfo() : stream(false) {}
		//FIXME: don't need this anymore; originally intended to have it handle deep copies from pointers but now it's just the same as the default copy constr.
		MimeRegInfo(const MimeRegInfo& c) {
			mimeType = c.mimeType;
			extension = c.extension;
			urlPattern = c.urlPattern;
			scheme = c.scheme;
			stream = c.stream;
		}
		MimeRegInfo& operator=(const MimeRegInfo& c) {
			if (this == &c)
				return *this;
			mimeType = c.mimeType;
			extension = c.extension;
			urlPattern = c.urlPattern;
			scheme = c.scheme;
			stream = c.stream;
			return *this;
		}
		std::string mimeType;
		std::string extension;
		std::string urlPattern;
		std::string scheme;
		bool stream;
	};

	static int 	utilExtractMimeTypes(struct json_object * jsonMimeTypeArray,std::vector<MimeRegInfo>& extractedMimeTypes);

	std::string            		m_id;
	std::string					m_title;				//copy of default launchpoint's title
	std::string            		m_category;
	std::string            		m_entryPoint;
	std::string            		m_version;
	bool                   		m_isHeadLess;
	std::list<ResourceHandler> 	m_mimeTypes;
	std::list<RedirectHandler> 	m_redirectTypes;
	LaunchPointList        		m_launchPoints;
	std::string					m_splashIconName;
	std::string					m_splashBackgroundName;
	std::string			   		m_miniIconName;
	bool                   		m_hasTransparentWindows;
	std::string            		m_folderPath;
	bool				   		m_executionLock;
	bool				   		m_flaggedForRemoval;
	bool				   		m_isRemovable;
	bool						m_isUserHideable;
	int							m_progress;
	bool				   		m_isVisible;
	uint32_t					m_hardwareFeaturesNeeded;
	Type				   		m_type;
	Status						m_status;
	bool				   		m_hasAccounts;
	bool						m_launchInNewGroup;
	bool						m_tapToShareSupported;
    bool                        m_handlesRelaunch;

	// Dock Mode parameters
	bool						m_dockMode;
	std::string					m_dockModeTitle;

	std::string                 m_attributes;

	std::string					m_vendorName;
	std::string			   		m_vendorUrl;
	uint64_t					m_appSize;			///total size when installed on device (just the app dir and subdirs; not counting dbs, etc.)
	uint32_t					m_fsBlockSize;		/// the blocksize with which m_appSize was calculated; this is useful to have here in case the bsize changes so a recalc can be trigerred...
													///		(won't be needed if proper manifests ever end up getting implemented)
	unsigned int				m_runtimeMemoryRequired; // Amount (in MB) of RAM the application expects to use during runtime
	std::string					m_appmenuName;
	KeywordMap			   		m_keywords;
	std::string 				m_universalSearchJsonStr;
	std::string					m_servicesJsonStr;
	std::string					m_accountsJsonStr;

	std::string            		m_requestedWindowOrientation;

	// if type == SysmgrBuiltin

	SysmgrBuiltinLaunchHelper *		m_pBuiltin_launcher;
	void updateSysmgrBuiltinWithLocalization();
};	


#endif /* APPLICATIONDESCRIPTION_H */
