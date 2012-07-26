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




#ifndef BOOTUPANIMATION_H
#define BOOTUPANIMATION_H

#include "Common.h"

#include <glib.h>
#include <vector>
#include <string>
#include <QObject>
#include <QWidget>
#include <QPointer>
#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTextLayout>
#include <QSocketNotifier>


#define BOOT_ANIM_MESSAGE_LENGHT            (int)(sizeof(char) + 2 * sizeof(int))
#define BOOT_ANIM_MESSAGE_END_ANIMATION     0x01
#define BOOT_ANIM_MESSAGE_START_PROGRESS    0x02
#define BOOT_ANIM_MESSAGE_END_PROGRESS      0x03
#define BOOT_ANIM_MESSAGE_PROGRESS_UPDATE   0x04

extern int bootAnimPipeFd; 
extern pid_t bootAnimPid;

class PGSurface;
class PGContext;
class PGFont;
class PGFallbackFonts;

class BootupAnimation : public QObject
{
	Q_OBJECT
	//Q_PROPERTY(qreal glowOpacity READ glowOpacity WRITE setGlowOpacity)
	//Q_PROPERTY(int spinnerProgress READ spinnerProgress WRITE setSpinnerProgress)
	
public:
	// static functions invoked by the main process to communicate with the Animation Process
	static void stopBootupAnimation();
	static void startActivityAnimation();
	static void stopActivityAnimation();
	static void setActivityAnimationProgress(int val, int total);
	
public:

	BootupAnimation(int readPipeFd);
	~BootupAnimation();

	void start();


protected:
    void paintEvent(QPaintEvent *event);
	void paint(QPainter* painter);

private Q_SLOTS:
	void pipeDataAvailable(int pipe);

private:

	void init();
	void deinit();

	void generateUtf16AndGlyphOffsets();
	std::vector<gunichar2> convertToUtf16(const std::string& s) const;
	std::vector<int> getGlyphOffsets(const std::vector<gunichar2>& s, int height) const;

	void updateScreen(PGSurface* surf);
	void updateScreen(PGSurface* surf, int dx, int dy, int dr, int db);

	int spinnerProgress() { return m_activitySpinner; }
	void setSpinnerProgress(int progress) { m_activitySpinner = progress; }
	
	void stop();

	void startActivity();
	void stopActivity();

	void setActivityProgress(int val, int total);

	void renderInStateLogo();
	void renderInStateActivity();

	static gpointer renderThread(gpointer arg);
	void renderThread();

private:

	enum State {
		StateIdle,
		StateLogo,
		StateActivity
	};

	GThread* m_renderThread;

	State m_state;
	PGContext* m_ctxt;
	PGFont* m_font;
	PGFallbackFonts* m_fallbackFonts;
	PGSurface* m_logoSurf;
	PGSurface* m_logoBrightSurf;
	PGSurface* m_activityStaticSurf;
	PGSurface* m_activityProgressSurf;
	PGSurface* m_activitySpinnerSurf;
	
	int m_readPipeFd;
	QSocketNotifier m_readNotifier;
	
	int m_textHeight1;
	int m_textHeight2;
	
	
	float m_logoScale;
	int m_logoAlpha;
	int m_uiFrame;
	int m_uiAlpha;
	float m_uiScale;
	int m_activityProgress;
	int m_activitySpinner;
	int m_rotation;
	
	std::vector<gunichar2> m_textLine1;
	std::vector<gunichar2> m_textLine2;
	std::vector<int> m_glyphOffsetsLine1;
	std::vector<int> m_glyphOffsetsLine2;
	int m_widthLine1;
	int m_widthLine2;

};

// ====================================================================
// animation transition class used by the SysMgr process to simulate the animation animation fade when the bootup process is done.


class BootupAnimationTransition : public QGraphicsObject
{
	Q_OBJECT
	
public:

	BootupAnimationTransition();
	~BootupAnimationTransition();

	void start();
	void stop();

	QRectF boundingRect() const;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

private Q_SLOTS:
	void fadeAnimationFinished();

private:

	void init();
	void deinit();
	
private:
	QRect m_bounds;
	QPixmap* m_screenPixmap;
	int m_rotation;

	QPointer<QPropertyAnimation> m_opacityAnimationPtr;
	QPointer<QPropertyAnimation> m_scaleAnimationPtr;
	QPointer<QParallelAnimationGroup> m_fadeAnimationGroupPtr;
};




#endif /* BOOTUPANIMATION_H */
