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




#ifndef DOCKMODELAUNCHPOINT_H
#define DOCKMODELAUNCHPOINT_H

#include "Common.h"

#include <set>
#include "DockModeWindowManager.h"
#include "PtrArray.h"
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsSceneMouseEvent>
#include <QGestureEvent>
#include <QList>
#include <QTextLayout>
#include <QPointer>

class Window;
class DockModeWindow;
class LaunchPoint;

class DockModeLaunchPoint : public QObject
{
	Q_OBJECT
public:

	enum State {
		NotLaunched = 0,
		Closed,
		Launching,
		Running, 
		Frozen
	};
	
	DockModeLaunchPoint(int width, int height, const LaunchPoint* lp, bool isPermanent = false);
	~DockModeLaunchPoint() ;

	QRectF boundingRect() const;

	bool isPermanent() const { return m_isPermanent; }
	
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	
	void setState(State newState);
	State state() { return m_state; }
	
	const LaunchPoint* launchPoint() { return m_launchPoint; }
	
    void setOrientation(OrientationEvent::Orientation orient);
	
	void setOpenWindow(Window* win);
	Window* openWindow();

	bool windowJustFinishedLoading();
	bool loadingTimedOut();

	void setAnchorPos(int x, int y) {m_anchorPos = QPoint(x, y);}
	
	void createAppScreenshot(bool closing = false);
	void deleteScreenShot();
    void createSplashImage( QPixmap* img, OrientationEvent::Orientation imgOrientation);
	QPixmap* splashImage() { return m_splashImage; }	
	
	void setPressedState(bool pressed) { m_buttonPressed = pressed; }

	static void deleteStaticImages();

	void resize (int width, int height);
	
public :
	
	
private Q_SLOTS:
	void positionAnimationValueChanged(const QVariant& value);	
	
private:
	
	static QPainter imagePainter;
	static QPixmap  *s_removeIcon, *s_removeIconHighlight;
	static QPixmap *s_scrimPixmap;
	static QPixmap* s_defaultSplash;
	
	static QPixmap* defaultSplashImage();
	static QPixmap* scrimImage();

private:
	
    void copyAndRotateImage(QPixmap* source, OrientationEvent::Orientation srcOrient,
                            QPixmap* destination, OrientationEvent::Orientation dstOrient, qreal scaleFactor);
	
	
	int m_width, m_height;

	qreal m_textHeight;
	QRect m_bounds, m_imageBounds;
	QRect m_removeHitArea;
	QPoint m_anchorPos;
	bool m_beingDeleted;
	bool m_buttonHighlight;
	bool m_buttonPressed;
	bool m_isPermanent;
	
	State m_state;
	bool  m_hasScreenshot;
	const LaunchPoint* m_launchPoint;
	std::string m_splashImgPath;
	QPixmap* m_splashImage;
	QPixmap* m_appScreenShot;
    OrientationEvent::Orientation m_screenShotOrientation;
    OrientationEvent::Orientation m_orientation;
	Window* m_openWindow;
	QFont* m_font;
	QTextLayout m_textLayout;
	qreal m_rotation;
	QPointer<QPropertyAnimation> m_positionAnimationPtr;
	
};


#endif /* DOCKMODELAUNCHPOINT_H */
