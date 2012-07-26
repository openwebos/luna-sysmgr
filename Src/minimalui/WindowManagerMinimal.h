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




#ifndef WINDOWMANAGERMINIMAL_H
#define WINDOWMANAGERMINIMAL_H

#include "Common.h"

#include <QList>

#include "WindowManagerBase.h"
#include "GraphicsItemContainer.h"
#include "StatusBar.h"

class CardWindow;

class WindowManagerMinimal : public WindowManagerBase
{
	Q_OBJECT

public:

	WindowManagerMinimal(int maxWidth, int maxHeight);
	virtual ~WindowManagerMinimal();

	virtual void init();

	virtual void prepareAddWindow(Window* win);
	virtual void addWindow(Window* win);
	virtual void removeWindow(Window* win);
	virtual void focusWindow(Window* win);

	virtual const char* name() const { return "Minimal"; }

	virtual void resize (int width, int height);
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

private Q_SLOTS:

	void slotPositiveSpaceChanged(const QRect& r);
	void slotUiRotationAboutToStart();
	void slotUiRotationCompleted();

private:

	void addCardWindow(Window* win);
	void addAlertWindow(Window* win);

	void removeCardWindow(Window* win);
	void removeAlertWindow(Window* win);

	void focusCardWindow(Window* win);
	void focusAlertWindow(Window* win);
	void resizeCardWindow(CardWindow* win, int width, int height);
	
private:

	Window* m_activeWin;
	QRect m_positiveSpace;
	
	StatusBar* m_statusBar;
	
	GraphicsItemContainer* m_cardContainer;
	QList<Window*> m_cardArray;
	
	GraphicsItemContainer* m_alertContainer;
	QList<Window*> m_alertArray;
};

#endif /* WINDOWMANAGERMINIMAL_H */
