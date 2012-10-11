/**
 *  Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#ifndef WEBAPPBASE_H
#define WEBAPPBASE_H

#include "Common.h"

#include <glib.h>
#include <list>
#include <lunaservice.h>
#include <QMap>
#include <QPointer>

#include "ProcessBase.h"
#include "WebPageClient.h"
#include "WebAppManager.h"

#if defined(HAS_NYX)
    #include "NyxSensorConnector.h"
#endif

class WebPage;
class ApplicationDescription;
namespace Palm {
	class WebGLES2Context;
}

class WebAppBase : public WebPageClient
        #if defined(HAS_NYX)
            ,public NYXConnectorObserver
        #endif
{
public:
    // Don't change these enums. They are published API
    typedef enum {
        WebKitShake_Invalid =-1,
        WebKitShake_Start   = 0,
        WebKitShake_Shaking = 1,
        WebKitShake_End     = 2
    }WebKitShakeEvent;

    typedef enum {
        WebKitOrientation_Invalid   =-1,
        WebKitOrientation_FaceUp    = 0,
        WebKitOrientation_FaceDown  = 1,
        WebKitOrientation_Up        = 2,
        WebKitOrientation_Down      = 3,
        WebKitOrientation_Left      = 4,
        WebKitOrientation_Right     = 5
    }WebKitOrientationEvent;

	WebAppBase();
	virtual ~WebAppBase();

	virtual void attach(WebPage* page);
	virtual WebPage* detach();

	virtual void thawFromCache() {}
	virtual void freezeInCache() {}
	void markInCache(bool val);

	void setKeepAlive(bool val);
	bool keepAlive() const { return m_keepAlive; }
	
	WebPage* page() const { return m_page; }

	virtual bool isWindowed() const { return false; }
	virtual bool isCardApp() const { return false; }
	virtual bool isChildApp() const { return false; }
	virtual bool isDashboardApp() const { return false; }
	virtual bool isAlertApp() const { return false; }

	void relaunch(const char* args, const char* launchingAppId, const char* launchingProcId);
    virtual void stagePreparing();
    virtual void stageReady();

	std::string appId() const { return m_appId; }
	std::string processId() const { return m_processId; }
	std::string url() const { return m_url; }
	ApplicationDescription* getAppDescription();
	void setAppDescription(ApplicationDescription* appDesc);

	virtual Palm::WebGLES2Context* getGLES2Context() { return 0; }

	virtual void setExplicitEditorFocus(bool focused, const PalmIME::EditorState & editorState);

    void setManualEditorFocusEnabled(bool enable);
    virtual void setManualEditorFocus(bool focused, const PalmIME::EditorState & editorState);

	virtual void suspendAppRendering() {}
	virtual void resumeAppRendering() {}

    /**
     * Function initializes(opens)/De-initializes the appropriate sensor
     *
     * @param[in] aSensorType   - Sensor Type
     * @param[in] aNeedEvents   - flag indicating whether to start or stop the sensor
     *
     * @return true if successful, false otherwise
     */
    virtual bool enableSensor(Palm::SensorType aSensorType, bool aNeedEvents);

    /**
      * Function sets the Accelerometer rate to highest level.
      * Currently the there is no means of setting fastAccelerometer to OFF.
      * So, the enable argument is not utilized at this point of time.
      *
      * @param[in] enable   - Whether to enable/disable
      */
    virtual void fastAccelerometerOn(bool enable);
protected:

	// WebPage Client
	virtual int  getKey() const { return 0; }
	virtual void focus() {}
	virtual void unfocus() {}
	virtual void close();
	virtual void windowSize(int& width, int& height) { width = 0; height = 0; }
	virtual void screenSize(int& width, int& height) {	width = WebAppManager::instance()->currentUiWidth();
														height = WebAppManager::instance()->currentUiHeight();}

	virtual void resizedContents(int contentsWidth, int contentsHeight);
	virtual void zoomedContents(double scaleFactor, int contentsWidth, int contentsHeight,
								int newScrollOffsetX, int newScrollOffsetY);
	virtual void invalContents(int x, int y, int width, int height);
	virtual void scrolledContents(int newContentsX, int newContentsY);
	virtual void uriChanged(const char* url);
	virtual void titleChanged(const char* title);
	virtual void statusMessage(const char* msg);
	virtual void dispatchFailedLoad(const char* domain, int errorCode,
			const char* failingURL, const char* localizedDescription);
	virtual void loadFinished() {}
//	virtual void enableCompass(bool enable);
	virtual void editorFocusChanged(bool focused, const PalmIME::EditorState& state) {}
    virtual void autoCapEnabled(bool enabled) {}
	virtual void needTouchEvents(bool needTouchEvents) {}

protected:

	void createActivity();
	void destroyActivity();
	void focusActivity();
	void blurActivity();

	void cleanResources();

    /**
     * Destroys all the sensors
     */
    void destroyAllSensors();

    /**
      * Function Maps the Screen orientation to Current Window Orientation
      */
    virtual Event::Orientation orientationForThisCard(Event::Orientation orient)
    {
        return orient;
    }

    /**
      * Function allows derived class members to do extra post processing before the orientation
      * event is given to the WebApp
      */
    virtual Event::Orientation postProcessOrientationEvent(Event::Orientation aInputEvent)
    {
        return aInputEvent;
    }

protected:

	WebPage* m_page;

	bool m_inCache;
	bool m_keepAlive;
	std::string m_appId;
	std::string m_processId;
	std::string m_url;
	ApplicationDescription *appDescImage;
	LSMessageToken m_activityManagerToken;
#if defined(HAS_NYX)
    int m_OrientationAngle;
#endif
private:

    WebAppBase& operator=(const WebAppBase&);
    WebAppBase(const WebAppBase&);

#if defined(HAS_NYX)
    /**
     * Function gets called whenever there is some data
     * available from NYX.
     *
     * @param[in]   - aSensorType - Sensor which has got some data to report
     */
    virtual void NYXDataAvailable (NYXConnectorBase::Sensor aSensorType);

    /**
     * Function initializes(opens) the appropriate sensor
     *
     * @param[in] aSensorType   - Sensor Type
     *
     * @return true if successful, false otherwise
     */
    bool enableSensor(NYXConnectorBase::Sensor aSensorType);

    /**
     * Function disables(closes) the appropriate sensor
     *
     * @param[in] aSensorType   - Sensor Type
     *
     * @return true if successful, false otherwise
     */
    bool disableSensor(NYXConnectorBase::Sensor aSensorType);

    bool sendCompassEvent();
    bool sendAccelerationEvent();
    bool sendShakeEvent();
    bool sendLogicalOrientationEvent();

    WebKitShakeEvent mapShakeEvent(ShakeEvent::Shake aShakeState);
    WebKitOrientationEvent mapOrientationEvent(OrientationEvent::Orientation aOrientation);
    NYXConnectorBase::Sensor mapSensorType(Palm::SensorType aSensorType);

    QMap<NYXConnectorBase::Sensor, QPointer<NYXConnectorBase> > m_SensorList;
#endif
};


#endif /* WEBAPPBASE_H */
