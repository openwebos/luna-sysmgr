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

static const int GESTURE_AREA_HEIGHT = Settings::LunaSettings()->gestureAreaHeight || 50;

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


class GestureStrip : public QWidget
{
	Q_OBJECT

public:

	GestureStrip(QWidget *mainView)
		: m_mainView(mainView)
	{
		setFocusPolicy(Qt::NoFocus);
		setAttribute(Qt::WA_AcceptTouchEvents);

		m_homeButton = new QPushButton(this);
		m_homeButton->setFixedSize(GESTURE_AREA_HEIGHT / 2,
								   GESTURE_AREA_HEIGHT / 2);

		m_homeButton->setAttribute(Qt::WA_AcceptTouchEvents);
		m_homeButton->setFocusPolicy(Qt::NoFocus);

		QHBoxLayout* layout = new QHBoxLayout;
		layout->addStretch();
		layout->addWidget(m_homeButton);
		layout->addStretch();
		setLayout(layout);

		m_seenGesture = false;

		grabGesture((Qt::GestureType) SysMgrGestureFlick);

		connect(m_homeButton, SIGNAL(clicked()), SLOT(slotHomeButtonClicked()));

		m_quickLaunch = false;
		m_quickLaunchFire.setInterval(250);
		m_quickLaunchFire.setSingleShot(true);
		connect(&m_quickLaunchFire, SIGNAL(timeout()), SLOT(slotQuickLaunchGesture()));
	}

	bool event(QEvent* e) {

		if (e->type() == QEvent::Gesture) {

			QGestureEvent* ge = static_cast<QGestureEvent*>(e);
			if (QGesture* gesture = ge->gesture((Qt::GestureType) SysMgrGestureFlick)) {
				if (gesture->state() == Qt::GestureFinished) {
					FlickGesture* fg = static_cast<FlickGesture*>(gesture);
					flickGesture(fg->velocity(), fg->hotSpot().toPoint());
				}
				return true;
			}
		}

		return QWidget::event(e);
	}

	void flickGesture(const QPoint& velocity, const QPoint& startPos) {

		if ((velocity.y() * velocity.y()) > (velocity.x() * velocity.x())) {

			if (velocity.y() > 0)
				postGesture(Qt::Key_CoreNavi_SwipeDown);
			else
				postGesture(Qt::Key_CoreNavi_Launcher);

			m_seenGesture = true;
		}
	}

	virtual void mousePressEvent(QMouseEvent* event) {
		m_mouseDownPos = event->pos();
		m_currentMousePos = event->pos();
		m_quickLaunchFire.stop();
		m_quickLaunch = false;
		m_seenGesture = false;
	}

	virtual void mouseMoveEvent(QMouseEvent* event) {
		m_currentMousePos = event->pos();
		if (m_currentMousePos.y() < 0) {
			if (!m_quickLaunch && !m_quickLaunchFire.isActive())
				m_quickLaunchFire.start();
		} else {
			m_quickLaunchFire.stop();
		}
	}

	virtual void mouseReleaseEvent(QMouseEvent* event) {
		m_quickLaunchFire.stop();
		m_quickLaunch = false;

		if (m_mouseDownPos.isNull() || m_seenGesture) {
			m_mouseDownPos = QPoint();
			m_currentMousePos = QPoint();
			m_seenGesture = false;
			return;
		}

		int x = event->pos().x();
		int y = event->pos().y();

		x = CLAMP(x, 0, width());
		y = CLAMP(y, 0, height());

		int deltaX = x - m_mouseDownPos.x();
		int deltaY = y - m_mouseDownPos.y();

		if ((deltaX * deltaX + deltaY * deltaY) > 100) {

			if (deltaX * deltaX > deltaY * deltaY) {

				// Horizontal movement
				if (deltaX > 0) {
					if (deltaX > width()/2) {
						postGesture(Qt::Key_CoreNavi_Next);
					}
					else {
						postGesture(Qt::Key_CoreNavi_Menu);
					}
				}
				else {
					if (-deltaX > width()/2) {
						postGesture(Qt::Key_CoreNavi_Previous);
					}
					else {
						postGesture(Qt::Key_CoreNavi_Back);
					}
				}
			}
		}

		m_mouseDownPos = QPoint();
		m_currentMousePos = QPoint();
		m_seenGesture = false;
	}

	void postGesture(Qt::Key key) {
		QWidget* window = QApplication::focusWidget();
		if (window) {
			QApplication::postEvent(window, new QKeyEvent(QEvent::KeyPress, key,
														  Qt::NoModifier));
			QApplication::postEvent(window, new QKeyEvent(QEvent::KeyRelease, key,
														  Qt::NoModifier));
		}
	}

	void postMouseUpdate(QPoint pos) {
		QWidget* window = QApplication::focusWidget();
		if (window) {
			pos = mapToGlobal(pos);
			pos = window->mapFromGlobal(pos);
			QApplication::sendEvent(window, new QMouseEvent(QEvent::MouseMove, pos, Qt::LeftButton, Qt::LeftButton,
														  Qt::NoModifier));
		}
	}

private Q_SLOTS:

	void slotHomeButtonClicked() {
		postGesture(Qt::Key_CoreNavi_Home);
	}

	void slotQuickLaunchGesture() {
		m_quickLaunch = true;
		m_seenGesture = true;
		viewport(m_mainView)->grabMouse();
		postGesture(Qt::Key_CoreNavi_QuickLaunch);
		postMouseUpdate(m_currentMousePos);
	}

private:

	QWidget *m_mainView;
	QPushButton* m_homeButton;
	QPoint m_mouseDownPos;
	bool m_seenGesture;
	QPoint m_currentMousePos;
	QTimer m_quickLaunchFire;
	bool m_quickLaunch;
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
        qDebug() << __PRETTY_FUNCTION__ << "Going fullscreen with width" << windowWidth << "height" << (windowHeight - GESTURE_AREA_HEIGHT);
    }

    m_info.displayBuffer = 0;
    m_info.displayWidth = windowWidth;
    m_info.displayHeight = windowHeight - GESTURE_AREA_HEIGHT;
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

	m_widget->setFixedSize(m_info.displayWidth, m_info.displayHeight + GESTURE_AREA_HEIGHT);
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
	GestureStrip* strip = new GestureStrip(view);
	strip->setFixedSize(m_widget->width(), GESTURE_AREA_HEIGHT);

	QVBoxLayout* layout = new QVBoxLayout(m_widget);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(view);
	layout->addWidget(strip);

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
