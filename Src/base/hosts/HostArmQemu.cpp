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




#include "Common.h"

#include "HostArm.h"

#include <QObject>
#include <QWidget>
#include <QtDebug>
#include <QGraphicsView>

class HostArmQemuKeyFilter : public QObject
{

protected:
	bool eventFilter(QObject* obj, QEvent* event)
	{
		bool handled = false;
		if ((event->type() == QEvent::KeyPress))
		{
			QWidget* window = NULL;
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

			switch (keyEvent->key())
			{
			case Qt::Key_Left:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyPress, Qt::Key_CoreNavi_Previous, 0));
				}
				handled = true;
				break;
			case Qt::Key_Right:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyPress, Qt::Key_CoreNavi_Next, 0));
				}
				handled = true;
				break;
			case Qt::Key_Home:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyPress, Qt::Key_CoreNavi_Home, 0));
				}
				handled = true;
				break;
			case Qt::Key_Escape:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyPress, Qt::Key_CoreNavi_Back, 0));
				}
				handled = true;
				break;
			case Qt::Key_End:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyPress, Qt::Key_CoreNavi_Launcher, 0));
				}
				handled = true;
				break;
			case Qt::Key_Pause:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Power, keyEvent->modifiers()));
				}
				handled = true;
				break;
			case Qt::Key_F6:
				window = QApplication::focusWidget();
				if (window) {
                    QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Up));
				}
				handled = true;
				break;

			case Qt::Key_F7:
				window = QApplication::focusWidget();
				if (window) {
                    QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Right));
				}
				handled = true;
				break;

			case Qt::Key_F8:
				window = QApplication::focusWidget();
				if (window) {
                    QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Down));
				}
				handled = true;
				break;

			case Qt::Key_F9:
				window = QApplication::focusWidget();
				if (window) {
                    QApplication::postEvent(window, new OrientationEvent(OrientationEvent::Orientation_Left));
				}
				handled = true;
				break;
			}
		} else if (event->type() == QEvent::KeyRelease) {
			QWidget *window = NULL;
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

			switch (keyEvent->key()) {
			case Qt::Key_Left:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Previous, 0));
				}
				handled = true;
				break;
			case Qt::Key_Right:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Next, 0));
				}
				handled = true;
				break;
			case Qt::Key_Home:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Home, 0));
				}
				handled = true;
				break;
			case Qt::Key_Escape:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Back, 0));
				}
				handled = true;
				break;
			case Qt::Key_End:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_CoreNavi_Launcher, 0));
				}
				handled = true;
				break;
			case Qt::Key_Pause:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Power, keyEvent->modifiers()));
				}
				handled = true;
				break;
			}
		}

		return handled;
	}
};

class HostArmQemu : public HostArm
{
public:
	HostArmQemu();
	virtual ~HostArmQemu();

	virtual const char* hardwareName() const{ return "ARM Emulator"; }
	virtual void setCentralWidget(QWidget* view);

	// switches aren't available in the emulator
	virtual void getInitialSwitchStates() { }
	virtual int getNumberOfSwitches() const { return 0; }

private:
	HostArmQemuKeyFilter* m_keyFilter;
};


HostArmQemu::HostArmQemu()
	: m_keyFilter(NULL)
{
}

HostArmQemu::~HostArmQemu()
{
	delete m_keyFilter;
}


void HostArmQemu::setCentralWidget(QWidget* view)
{
    // We need to set the active window here as well so that the
    // event get delivered properly.
    QApplication::setActiveWindow(view);
    m_keyFilter = new HostArmQemuKeyFilter;
    view->installEventFilter(m_keyFilter);
    view->show();
}

