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




#include "dimensionsmain.h"
#include "dimensionslauncher.h"
#include "dimensionsglobal.h"

#include <QCoreApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEvent>
#include <QSize>

#include <QDebug>

#include <limits.h>

#include "Settings.h"
#include "OverlayWindowManager.h"

//TEST:////////////////// TEST DEFINES////////////////////////////////////

#define TEST_DISABLE_LAUNCHER3  0
#define TEST_DISALLOW_RESIZE	0

//TEST://///////////////// END

QPointer<DimensionsUI> DimensionsUI::s_qp_primaryInstance = 0;
QPointer<LauncherObject> DimensionsUI::s_qp_primaryLauncher = 0;

//static
DimensionsUI * DimensionsUI::primaryInstance()
{
	return s_qp_primaryInstance;
}

DimensionsUI::DimensionsUI(quint32 width,quint32 height)
: Window(Window::Type_QtNativePaintWindow,width,height)
{

	if (!s_qp_primaryInstance)
	{
		s_qp_primaryInstance = this;
	}

	setFlag(ItemHasNoContents,true);
	setPos(0.0,(qreal)Settings::LunaSettings()->positiveSpaceTopPadding / 2.0);

	if ((width == 0) || (height == 0))
	{
		//bail, not ready yet
		return;
	}

	s_qp_primaryLauncher = new LauncherObject(DimensionsGlobal::realRectAroundRealPoint(QSize(width,height)),this);
	//TODO: IMPROVE: just do a full init w/o resize (this was done TEMP: during graft of the LauncherObject/split of the DimensionsUI
	// object)
	s_qp_primaryLauncher->resize(width,height);
	s_qp_primaryLauncher->setParentItem(this);
	s_qp_primaryLauncher->setVisible(true);
	setupProxySignals();
	m_lastResize = QSize(width,height);
}

//virtual
DimensionsUI::~DimensionsUI()
{
	delete s_qp_primaryLauncher;
}

//virtual
void DimensionsUI::resize(int w, int h)
{
	if ((w == 0) || (h == 0))
	{
		//reject
		return;
	}

	m_lastResize = QSize(w,h);
	if (!s_qp_primaryLauncher)
	{
		//TODO: IMPROVE: again, when the launcher is newly created, avoid extra resize
		s_qp_primaryLauncher = new LauncherObject(DimensionsGlobal::realRectAroundRealPoint(QSize(w,h)),this);
		s_qp_primaryLauncher->resize(w,h);
		s_qp_primaryLauncher->setParentItem(this);
		s_qp_primaryLauncher->setVisible(true);
		setupProxySignals();
	}
	else
	{
		s_qp_primaryLauncher->resize(w,h);
	}
}

void DimensionsUI::slotQuicklaunchFullyOpen()
{
	if (s_qp_primaryLauncher)
	{
		s_qp_primaryLauncher->slotQuicklaunchFullyOpen();
	}
}

void DimensionsUI::slotQuicklaunchFullyClosed()
{
	if (s_qp_primaryLauncher)
	{
		s_qp_primaryLauncher->slotQuicklaunchFullyClosed();
	}
}

void DimensionsUI::slotLauncherFullyOpen()
{
	if (s_qp_primaryLauncher)
	{
		s_qp_primaryLauncher->slotLauncherFullyOpen();
	}
}

void DimensionsUI::slotLauncherFullyClosed(bool reCreate)
{
	if (s_qp_primaryLauncher)
	{
		if (reCreate)
		{
			//remake the launcher object, fully reinitializing it
			slotReCreateLauncher();
			s_qp_primaryLauncher->slotLauncherFullyClosed();
		}
		s_qp_primaryLauncher->slotLauncherFullyClosed();
	}
}

void DimensionsUI::slotDestroyLauncher()
{
	delete s_qp_primaryLauncher;
}

void DimensionsUI::slotCreateLauncher()
{
	if (s_qp_primaryLauncher)
	{
		return;
	}

	s_qp_primaryLauncher = new LauncherObject(DimensionsGlobal::realRectAroundRealPoint(m_lastResize),this);
	//TODO: IMPROVE: just do a full init w/o resize (this was done TEMP: during graft of the LauncherObject/split of the DimensionsUI
	// object)
	s_qp_primaryLauncher->resize(m_lastResize.width(),m_lastResize.height());
	s_qp_primaryLauncher->setParentItem(this);
	s_qp_primaryLauncher->setVisible(true);
	setupProxySignals();
}

void DimensionsUI::slotReCreateLauncher()
{
	slotDestroyLauncher();
	slotCreateLauncher();
}

//virtual
QRectF 	DimensionsUI::geometry() const
{
	if (s_qp_primaryLauncher)
	{
		return s_qp_primaryLauncher->geometry();
	}
	return QRectF();
}

void DimensionsUI::setupProxySignals()
{
	////------ LauncherObject -> DimensionsUI
//	connect(s_qp_primaryLauncher,SIGNAL(signalReady()),
//			this,SIGNAL(signalReady()));
//	connect(s_qp_primaryLauncher,SIGNAL(signalReady()),
//			this,SIGNAL(signalReady()));
//	connect(s_qp_primaryLauncher,SIGNAL(signalShowMe(DimensionsTypes::ShowCause::Enum)),
//			this, SIGNAL(signalShowMe(DimensionsTypes::ShowCause::Enum)));
//	connect(s_qp_primaryLauncher,SIGNAL(signalHideMe(DimensionsTypes::HideCause::Enum)),
//			this, SIGNAL(signalHideMe(DimensionsTypes::HideCause::Enum)));
	connect(s_qp_primaryLauncher,SIGNAL(signalDropIconOnQuicklaunch(const QString&)),
			this, SIGNAL(signalDropIconOnQuicklaunch(const QString&)));


	////------  DimensionsUI -> LauncherObject
	connect(this,SIGNAL(signalRelayOWMHidingLauncher()),
			s_qp_primaryLauncher,SLOT(slotSystemHidingLauncher()));
	connect(this,SIGNAL(signalRelayOWMShowingLauncher()),
			s_qp_primaryLauncher,SLOT(slotSystemShowingLauncher()));

	connect(this,SIGNAL(signalRelayOWMHidingUniversalSearch()),
			s_qp_primaryLauncher,SLOT(slotSystemHidingLauncherOverlay()));
	connect(this,SIGNAL(signalRelayOWMShowingUniversalSearch()),
			s_qp_primaryLauncher,SLOT(slotSystemShowingLauncherOverlay()));

}

//static
LauncherObject * DimensionsUI::launcher()
{
	return s_qp_primaryLauncher;
}

QRectF DimensionsUI::quickLaunchArea() const
{
	//need the OverlayWindowManager
	OverlayWindowManager * pOwm = OverlayWindowManager::systemActiveInstance();
	if (pOwm)
	{
		///do this through the property so it can be isolated easier; don't call the fn directly
		return mapRectFromItem(pOwm,pOwm->property("quickLaunchVisibleArea").toRectF());
	}
	return QRectF();
}

QPointF DimensionsUI::quickLaunchPosition() const
{
	//need the OverlayWindowManager
	OverlayWindowManager * pOwm = OverlayWindowManager::systemActiveInstance();
	if (pOwm)
	{
		///do this through the property so it can be isolated easier; don't call the fn directly
		return mapFromItem(pOwm,pOwm->property("quickLaunchVisiblePosition").toPointF());
	}
	return QPointF();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////									Quicklauncher

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QPointer<Quicklauncher> Quicklauncher::s_qp_primaryInstance = 0;
QPointer<QuickLaunchBar> Quicklauncher::s_qp_primaryQuickLaunchBar = 0;

//static
Quicklauncher * Quicklauncher::primaryInstance()
{
	return s_qp_primaryInstance;
}

Quicklauncher::Quicklauncher(quint32 width,quint32 height)
: Window(Window::Type_QtNativePaintWindow,width,height)
    , m_bgOpacity(1.0)
{

	if (!s_qp_primaryInstance)
	{
		s_qp_primaryInstance = this;
	}

	setFlag(ItemHasNoContents,true);

	if ((width == 0) || (height == 0))
	{
		//bail, not ready yet
		return;
	}

	s_qp_primaryQuickLaunchBar = new QuickLaunchBar(DimensionsGlobal::realRectAroundRealPoint(QSize(width,height)),this);
	s_qp_primaryQuickLaunchBar->fullInit(width,height);
	s_qp_primaryQuickLaunchBar->setParentItem(this);
	s_qp_primaryQuickLaunchBar->setVisible(true);
	setupProxySignals();
	m_lastResize = QSize(width,height);
}

//virtual
Quicklauncher::~Quicklauncher()
{
	delete s_qp_primaryQuickLaunchBar;
}

//virtual
void Quicklauncher::resize(int w, int h)
{
	// w and h are the total SCREEN size we are resizing the UI to.

	if ((w == 0) || (h == 0))
	{
		//reject
		return;
	}

	m_lastResize = QSize(w,h);

	bool success = false;
	if (!s_qp_primaryQuickLaunchBar)
	{
		//TODO: IMPROVE: again, when the launcher is newly created, avoid extra resize
		s_qp_primaryQuickLaunchBar = new QuickLaunchBar(DimensionsGlobal::realRectAroundRealPoint(QSize(w,h)),this);
		success = s_qp_primaryQuickLaunchBar->fullInit(w,h);
		s_qp_primaryQuickLaunchBar->setParentItem(this);
		s_qp_primaryQuickLaunchBar->setVisible(true);
		setupProxySignals();
	}
	else
	{
		QSize s = QuickLaunchBar::QuickLaunchSizeFromScreenSize(w,h);
		success = s_qp_primaryQuickLaunchBar->resize(s);
	}
}

//virtual
QRectF 	Quicklauncher::geometry() const
{
	if (s_qp_primaryQuickLaunchBar)
	{
		return s_qp_primaryQuickLaunchBar->geometry();
	}
	return QRectF();
}

//virtual
int Quicklauncher::normalHeight() const
{
	return (geometry().size().toSize().height());
}

void Quicklauncher::setupProxySignals()
{

}

//static
QuickLaunchBar * Quicklauncher::quickLaunchBar()
{
	return s_qp_primaryQuickLaunchBar;
}

//virtual
void Quicklauncher::save()
{

}

//virtual
void Quicklauncher::restore()
{

}

void Quicklauncher::setBackgroundOpacity(const qreal& v)
{
	if (v != m_bgOpacity)
	{
		m_bgOpacity = v;
		update();
	}
}

void Quicklauncher::setHighlightAt(int x, int y)
{

}

void Quicklauncher::removeHighlight()
{
	if(s_qp_primaryQuickLaunchBar) {
		s_qp_primaryQuickLaunchBar->cancelLaunchFeedback();
	}
}

void Quicklauncher::slotDestroyQuickLauncher()
{
	delete s_qp_primaryQuickLaunchBar;
}

void Quicklauncher::slotCreateQuickLauncher()
{
	if (s_qp_primaryQuickLaunchBar)
	{
		return;
	}
	s_qp_primaryQuickLaunchBar = new QuickLaunchBar(DimensionsGlobal::realRectAroundRealPoint(m_lastResize),this);
	s_qp_primaryQuickLaunchBar->fullInit(m_lastResize.width(),m_lastResize.height());
	s_qp_primaryQuickLaunchBar->setParentItem(this);
	s_qp_primaryQuickLaunchBar->setVisible(true);
	setupProxySignals();
}

void Quicklauncher::slotReCreateQuickLauncher()
{
	slotDestroyQuickLauncher();
	slotCreateQuickLauncher();
}
