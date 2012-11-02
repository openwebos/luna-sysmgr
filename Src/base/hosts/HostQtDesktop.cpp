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




#include "Common.h"

#include "HostQtDesktop.h"
#include "Settings.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGestureEvent>
#include <QDesktopWidget>
#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QtDebug>
#include <QGraphicsView>

#include <SysMgrDefs.h>

#include "CustomEvents.h"
#include "FlickGesture.h"
#include "FlickGestureRecognizer.h"

static QWidget *viewport(QWidget *widget)
{
	QGraphicsView *gView = qobject_cast<QGraphicsView *>(widget);
	if (gView)
		return gView->viewport();
	return widget;
}

// This is a temporary widget that allows as to query the current geometry
// for a widget that has been maximized. This value will be used by
// HostQtDesktop::init to go to fullscreen when width or height configured
// as negative value
class InspectorWidget : public QWidget
{
public:
    InspectorWidget(QWidget* parent = 0) : QWidget(parent)
    {
        showMaximized();
        installEventFilter(this);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    ~InspectorWidget() { }

    bool eventFilter(QObject *object, QEvent *event)
    {
        if (object == this && event->type() == QEvent::Resize) {
            maxGeometry = geometry();
            QApplication::closeAllWindows();
        }

        return QWidget::eventFilter(object, event);
    }

public:
    QRect maxGeometry;
};

class HostQtDesktopMouseFilter : public QObject
{
	Q_OBJECT

protected:
	bool eventFilter(QObject *obj, QEvent *event)
	{
		bool handled = false;
		if (event->type() == QEvent::MouseButtonRelease) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			Qt::KeyboardModifiers modifiers = mouseEvent->modifiers();
			if (modifiers & Qt::ControlModifier) {
				modifiers &= ~Qt::ControlModifier;
				modifiers |= Qt::AltModifier;
				mouseEvent->setModifiers(modifiers);
			}
			if (mouseEvent->button() == Qt::LeftButton) {
				QWidget *grabber = QWidget::mouseGrabber();
				if (grabber) {
					grabber->releaseMouse();
				}
			}
		} else if (event->type() == QEvent::MouseButtonPress) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			Qt::KeyboardModifiers modifiers = mouseEvent->modifiers();
			if (modifiers & Qt::ControlModifier) {
				modifiers &= ~Qt::ControlModifier;
				modifiers |= Qt::AltModifier;
				mouseEvent->setModifiers(modifiers);
			}
		} else if (event->type() == QEvent::MouseMove) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			Qt::KeyboardModifiers modifiers = mouseEvent->modifiers();
			if (modifiers & Qt::ControlModifier) {
				modifiers &= ~Qt::ControlModifier;
				modifiers |= Qt::AltModifier;
				mouseEvent->setModifiers(modifiers);
			}
		}

		return handled;
	}
};

class HostQtDesktopKeyFilter : public QObject
{
	Q_OBJECT

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
			case Qt::Key_F2:
				window = QApplication::focusWidget();
				if (window) {
					QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Keyboard, keyEvent->modifiers()));
				}
				handled = true;
				break;
			}
		} else if (event->type() == QEvent::MouseButtonRelease) {
			//qDebug() << "!!!!mouse release";
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			if (mouseEvent->button() == Qt::LeftButton) {
				//qDebug() << "!!!!left release";
				QWidget *grabber = QWidget::mouseGrabber();
				if (grabber) {
					//qDebug() << "!!!!grabber release";
					grabber->releaseMouse();
				}
			}
		} else {
//			qDebug() << "event filter for" << event << "targetting" << event->
		}

		return handled;
	}
};

HostQtDesktop::HostQtDesktop()
	: m_keyFilter(NULL), m_mouseFilter(NULL)
{
}

HostQtDesktop::~HostQtDesktop()
{
	delete m_keyFilter;
	delete m_mouseFilter;
}

void HostQtDesktop::init(int w, int h)
{
    int windowWidth = w;
    int windowHeight = h;
    if (windowWidth < 0 || windowHeight < 0) {
        int argc = 0;
        char** argv = 0;
        QApplication a(argc, argv);
        InspectorWidget tmp;

        tmp.show();
        a.exec();

        windowWidth = tmp.maxGeometry.width();
        windowHeight = tmp.maxGeometry.height();
        qDebug() << __PRETTY_FUNCTION__ << "Going fullscreen with width" << windowWidth << "height" << windowHeight;
    }

    m_info.displayBuffer = 0;
    m_info.displayWidth = windowWidth;
    m_info.displayHeight = windowHeight;
    m_info.displayDepth = 32;
}

void HostQtDesktop::show()
{
	QGestureRecognizer::registerRecognizer(new FlickGestureRecognizer);

	m_widget = new QWidget;
	m_widget->setWindowFlags(Qt::CustomizeWindowHint |
							 Qt::WindowTitleHint |
							 Qt::WindowCloseButtonHint);

	m_widget->setAttribute(Qt::WA_AcceptTouchEvents);
    m_widget->setWindowTitle("Open webOS");

	m_widget->setFixedSize(m_info.displayWidth, m_info.displayHeight);
	m_widget->show();
}

unsigned short HostQtDesktop::translateKeyWithMeta( unsigned short key, bool withShift, bool withAlt)
{
	return 0;
}

const char* HostQtDesktop::hardwareName() const
{
	return "Desktop";
}

void HostQtDesktop::setCentralWidget(QWidget* view)
{
	QVBoxLayout* layout = new QVBoxLayout(m_widget);
    layout->setSizeConstraint(QLayout::SetFixedSize);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(view);

	m_keyFilter = new HostQtDesktopKeyFilter;
	qApp->installEventFilter(m_keyFilter);

	m_mouseFilter = new HostQtDesktopMouseFilter;
	viewport(view)->installEventFilter(m_mouseFilter);
}

bool HostQtDesktop::hasAltKey(Qt::KeyboardModifiers modifiers)
{
	return modifiers & (Qt::AltModifier | Qt::ControlModifier);
}


#include "HostQtDesktop.moc"
