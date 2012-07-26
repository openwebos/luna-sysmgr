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




#include "WebFrame.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <cjson/json.h>

#include "JsSysObjectWrapper.h"
#include "ProcessManager.h"
#include "Utils.h"
#include "WebPage.h"

WebFrame::WebFrame(WebPage* page, Palm::WebFrame* frame, bool isMainFrame)
	: m_page(page)
	, m_webFrame(frame)
	, m_isMainFrame(isMainFrame)
	, m_jsObj(0)
	, m_shuttingDown(false)
{
	if (!m_isMainFrame) {
		std::string procId = ProcessManager::instance()->processIdFactory();
		setProcessId(procId);
	}
}

WebFrame::~WebFrame()
{
	m_shuttingDown = true;
	
	if (m_jsObj) {
		m_jsObj->setFrame(0);
		m_jsObj->release();
		m_jsObj = 0;
	}
}

void WebFrame::urlTitleChanged(const char* uri, const char* title)
{
	if (m_isMainFrame) {
		m_page->urlTitleChanged(uri, title);
		return;
	}

	if (!uri)
		return;

	if (strcmp(uri, m_url.c_str()) == 0)
		return;

	m_url = uri;
	setAppId("");
	m_identifier = std::string();

	QUrl url(uri);
	if (url.scheme() != "file")
		return;

	bool foundAppInfoDir = false;

	QDir parentDir(QFileInfo(url.path()).dir());

	while (!foundAppInfoDir && parentDir.exists() && (parentDir.absolutePath() != "/")) {

		QFileInfo fileInfo(parentDir.absolutePath() + "/appinfo.json");
		if (fileInfo.exists()) {
			foundAppInfoDir = true;
			break;
		}

		parentDir.cdUp();		
	}

	if (!foundAppInfoDir)
		return;

	QString appInfoPath = parentDir.absolutePath() + "/appinfo.json";
	struct json_object* json = json_object_from_file(appInfoPath.toUtf8().data());
	if (!json || is_error(json))
		return;

	struct json_object* label = json_object_object_get(json, "id");
	if (label && json_object_is_type(label, json_type_string)) {
		setAppId(json_object_get_string(label));
	}
	
	json_object_put(json);
}

const char* WebFrame::getIdentifier()
{
	if (m_isMainFrame)
		return m_page->getIdentifier();
	
	if (m_identifier.empty())
		m_identifier = windowIdentifierFromAppAndProcessId(appId(), processId());
	

	return m_identifier.c_str();
}

bool WebFrame::isBusPriviledged()
{
	if (m_isMainFrame)
		return m_page->isBusPriviledged();

	if (appId().empty())
		return false;

	return (appId().find("com.palm.", 0) == 0);
}

void WebFrame::jsObjectCleared()
{
	if (m_shuttingDown) {
		if (m_jsObj) {
			m_jsObj->setFrame(0);
			m_jsObj->release();
			m_jsObj = 0;
		}		
		return;
	}

	if (m_jsObj) {
		m_jsObj->setFrame(0);
		m_jsObj->release();
		m_jsObj = 0;
	}

	m_jsObj = acquireFrameJsSysObject(this);
	if (m_jsObj)
		m_jsObj->setLaunchParams(m_launchParams);
}

void WebFrame::setLaunchParams(const std::string& args)
{
	m_launchParams = args;

	if (m_jsObj)
		m_jsObj->setLaunchParams(args);
}
