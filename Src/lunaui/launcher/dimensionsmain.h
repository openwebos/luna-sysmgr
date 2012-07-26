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




#ifndef DIMENSIONSMAIN_H_
#define DIMENSIONSMAIN_H_

#include "dimensionstypes.h"
#include "quicklaunchbar.h"
#include "Window.h"
#include <QPointer>
#include <QMap>
#include <QUuid>

namespace DimensionsSystemInterface
{
class ExternalApp;
class WebOSApp;
class PageSaver;
class PageRestore;
}

class LauncherObject;
class TextBox;

class DimensionsUI : public Window
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:

	friend class LauncherObject;		//to keep from having to write passthru's; DimensionsUI is just a wrapper for LauncherObject anyways

	//TODO: should make this return a const obj, and assure all functions that are useful to the outside are const as well
	//  or, DimensionsUI could be a proper singleton...but that precludes some eeeevil, advanced ideas I have for it....
	static DimensionsUI * primaryInstance();

	DimensionsUI(quint32 width,quint32 height);
	virtual ~DimensionsUI();

	virtual void resize(int w, int h);

	QRectF 	geometry() const;

	static LauncherObject * launcher();

public Q_SLOTS:

	void slotQuicklaunchFullyOpen();
	void slotQuicklaunchFullyClosed();
	void slotLauncherFullyOpen();
	void slotLauncherFullyClosed(bool reCreate=false);

	void slotDestroyLauncher();
	void slotCreateLauncher();
	void slotReCreateLauncher();

Q_SIGNALS:
	void signalReady();
	void signalNotReady();

	void signalHideMe(DimensionsTypes::HideCause::Enum cause = DimensionsTypes::HideCause::None);
	void signalShowMe(DimensionsTypes::ShowCause::Enum cause = DimensionsTypes::ShowCause::None);

	void signalDropIconOnQuicklaunch(const QString&);

	void signalRelayOWMHidingLauncher();
	void signalRelayOWMShowingLauncher();
	void signalRelayOWMHidingUniversalSearch();
	void signalRelayOWMShowingUniversalSearch();

protected:

	void setupProxySignals();

	QRectF quickLaunchArea() const;
	QPointF quickLaunchPosition() const;

protected:
	static QPointer<DimensionsUI> s_qp_primaryInstance;
	static QPointer<LauncherObject> s_qp_primaryLauncher;

	QSize m_lastResize;
};





class Quicklauncher : public Window
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)
	Q_PROPERTY(qreal backgroundOpacity READ backgroundOpacity WRITE setBackgroundOpacity)

public:
	friend class QuickLaunchBar;

	static Quicklauncher * primaryInstance();
	Quicklauncher(quint32 width,quint32 height);
	virtual ~Quicklauncher();

	virtual void resize(int w, int h);

	virtual QRectF 	geometry() const;
	virtual int normalHeight() const;

	static QuickLaunchBar * quickLaunchBar();

	virtual void save();
	virtual void restore();

	//property 'backgroundTransparency'
	qreal backgroundOpacity() const { return m_bgOpacity; }
	void setBackgroundOpacity(const qreal& v);

	void setHighlightAt(int x, int y);
	void removeHighlight();

public Q_SLOTS:

	void slotDestroyQuickLauncher();
	void slotCreateQuickLauncher();
	void slotReCreateQuickLauncher();

Q_SIGNALS:
	void signalReady();
	void signalNotReady();

	void signalHideMe(DimensionsTypes::HideCause::Enum cause = DimensionsTypes::HideCause::None);
	void signalShowMe(DimensionsTypes::ShowCause::Enum cause = DimensionsTypes::ShowCause::None);

protected:

	void setupProxySignals();

protected:
	qreal	m_bgOpacity;

	static QPointer<Quicklauncher> s_qp_primaryInstance;
	static QPointer<QuickLaunchBar> s_qp_primaryQuickLaunchBar;

	QSize m_lastResize;


};

#endif /* DIMENSIONSMAIN_H_ */
