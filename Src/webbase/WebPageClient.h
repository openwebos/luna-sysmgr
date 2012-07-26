/**
 * @file
 * 
 * Abstract base class to derive WebAppBase functionality from
 *
 * @author Hewlett-Packard Development Company, L.P.
 * @author tyrok1
 *
 * @section LICENSE
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
 */




#ifndef WEBPAGECLIENT_H
#define WEBPAGECLIENT_H

#include "Common.h"

#include <stdint.h>

#include <palmimedefines.h>
#include <palmwebviewclient.h>
#include <palmwebtypes.h>

class WebPage;
class ApplicationDescription;

namespace Palm {
	class WebGLES2Context;
}

/**
 * Abstract base class to derive WebAppBase functionality from
 * 
 * @todo Figure out why this is not set up to use smart pointers (sptr<>) for the page it attaches to.  If it did, it would be much safer for the caller since they would be far less likely to be holding an invalid pointer after our instance is destroyed.
 */
class WebPageClient
{
public:
	/**
	 * Initializes this class instance
	 * 
	 * Doesn't do anything - see derived classes'
	 * documentation for specifics.
	 */
	WebPageClient() {}
	
	/**
	 * Cleans up this class instance
	 * 
	 * Doesn't do anything - see derived classes'
	 * documentation for specifics.
	 */
	virtual ~WebPageClient() {}
	
	/**
	 * Attaches to a WebPage to allow us to control attributes of it
	 * 
	 * This method is designed to set up the content to
	 * display in this WebPageClient instance.  See
	 * specific derived classes' docs for more info.
	 * 
	 * @param	page			The page to display within our container.
	 */
	virtual void attach(WebPage* page) = 0;
	
	/**
	 * Detaches this instance from the WebPage it was previously managing
	 * 
	 * @return				The previously-managed WebPage instance containing this app's content.
	 */
	virtual WebPage* detach() = 0;
	
	/**
	 * Purpose is unclear
	 * 
	 * Implemented in WebAppBase as a protected method
	 * that returns a constant.
	 * 
	 * @todo Find a reason for this to exist and remove it if unnecessary.  If necessary, document this further.
	 * 
	 * @return				Unknown.
	 */
	virtual int  getKey() const = 0;

	virtual void focus() = 0;
	virtual void unfocus() = 0;
	
	/**
	 * Asks WebAppManager to close our app for us
	 * 
	 * @see WebAppManager
	 */
	virtual void close() = 0;
	virtual void windowSize(int& width, int& height) = 0;
	virtual void resizedContents(int contentsWidth, int contentsHeight) = 0;
	virtual void zoomedContents(double scaleFactor, int contentsWidth, int contentsHeight,
								int newScrollOffsetX, int newScrollOffsetY) = 0;
	virtual void invalContents(int x, int y, int width, int height) = 0;
	virtual void scrolledContents(int newContentsX, int newContentsY) = 0;
	virtual void uriChanged(const char* url) = 0;
	virtual void titleChanged(const char* title) = 0;
	virtual void statusMessage(const char* message) = 0;
	virtual void dispatchFailedLoad(const char* domain, int errorCode,
			const char* failingURL, const char* localizedDescription) = 0;
	virtual void loadFinished() = 0;

	virtual void editorFocusChanged(bool focused, const PalmIME::EditorState& state) = 0;
	
	/**
	 * Enables/disables auto-capitalization in fields in this app
	 * 
	 * @param	enabled			true to enable auto-capitalization, false to disable it.
	 */
	virtual void autoCapEnabled(bool enabled) = 0;
	virtual void needTouchEvents(bool needTouchEvents) = 0;
	
	virtual ApplicationDescription* getAppDescription() = 0;
	virtual Palm::WebGLES2Context* getGLES2Context() = 0;

	virtual void suspendAppRendering() = 0;
	virtual void resumeAppRendering() = 0;
	virtual void screenSize(int& width, int& height) = 0;
    /**
     * Function initializes(opens)/De-initializes the appropriate sensor
     *
     * @param[in] aSensorType   - Sensor Type
     * @param[in] aNeedEvents   - flag indicating whether to start or stop the sensor
     *
     * @return true if successful, false otherwise
     */
    virtual bool enableSensor(Palm::SensorType aSensorType, bool aNeedEvents) = 0;

    /**
      * Function sets the Accelerometer rate to highest level.
      * Currently the there is no means of setting fastAccelerometer to OFF.
      * So, the enable argument is not utilized at this point of time.
      *
      * @param[in] enable   - Whether to enable/disable
      */
    virtual void fastAccelerometerOn(bool enable) = 0;
};	

#endif /* WEBPAGECLIENT_H */
