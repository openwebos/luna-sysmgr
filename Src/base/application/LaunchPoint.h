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




#ifndef LAUNCHPOINT_H
#define LAUNCHPOINT_H

#include "Common.h"

#include <string>
#include <list>
#include <QPixmap>
#include <glib.h>

class ApplicationDescription;
class json_object;

#define DEFAULT_ICON_W	64
#define DEFAULT_ICON_H	64

class LaunchPoint
{
public:

	static LaunchPoint* fromFile(ApplicationDescription* appDesc,
								 const std::string& filePath);

	LaunchPoint(ApplicationDescription* appDesc,
				const std::string& id,
				const std::string& launchPointId,
				const std::string& title,
				const std::string& menuName,
				const std::string& iconPath,
				const std::string& params,
				bool removable);

	~LaunchPoint();

	// NOTE: it is the callers responsibility to json_object_put the return value
	json_object* toJSON() const;

	void setAppDesc(ApplicationDescription* appDesc) { m_appDesc = appDesc; }

	bool updateIconPath(std::string newIconPath);
	void updateTitle(const std::string& titleStr);

	ApplicationDescription* appDesc() const     { return m_appDesc; }
	const std::string& id() const               { return m_id; }
	const std::string& launchPointId() const    { return m_launchPointId; }
	const std::string& title() const            { return m_title.original; }
	const std::string& menuName() const			{ return m_appmenuName; }
	const std::string& iconPath() const         { return m_iconPath; }
	const std::string& params() const           { return m_params; }
	QPixmap icon() const;
	bool				isDefault() const			{return m_bDefault;}
	void				setAsDefault(bool dv=true)	{ m_bDefault=dv;}
	bool				setRemovable(bool v=true)	{ bool pv=m_removable;m_removable=v;return pv;}
	bool				isRemovable() const				{return m_removable;}
	std::string category() const;
	std::string entryPoint() const;

	bool matchesTitle(const gchar* str) const;
	int compareByKeys(const LaunchPoint* lp) const;

	bool				isVisible() const;
private:

	// prevent object copy
	LaunchPoint(const LaunchPoint&);
	LaunchPoint& operator=(const LaunchPoint&) const;

	struct Title {
		Title() : original(""), lowercase(0), keyed(0) { }
		~Title() { cleanup(); }

		void cleanup()
		{
			if (lowercase)
				g_free(lowercase);
			lowercase = 0;
			if (keyed)
				g_free(keyed);
			keyed = 0;
		}

		void set(const std::string& title)
		{
			cleanup();
			original = title;
			lowercase = g_utf8_strdown(title.c_str(), -1);
			keyed = g_utf8_collate_key(lowercase, -1);
		}

		std::string original;	// the title as given by the constructor
		gchar* lowercase;		// a lowercased version of the title
		gchar* keyed;			// a keyed version used for fast sorting
	};

	static LaunchPoint* fromJSON(ApplicationDescription* appDesc,
								 const char* jsonStr,
								 const std::string& launchPointId);

	bool toFile() const;

	ApplicationDescription* m_appDesc;
	std::string m_id;
	std::string m_launchPointId;
	Title m_title;
	std::string	m_appmenuName;
	std::string m_iconPath;
	std::string m_params;
//	QPixmap m_icon;
	bool	m_removable;
	bool	m_bDefault;		//is this the default launch point?
};

typedef std::list<const LaunchPoint*> LaunchPointList;

#endif /* LAUNCHPOINT_H */
