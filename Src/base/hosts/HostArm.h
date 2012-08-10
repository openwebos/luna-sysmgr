/**
 * @file
 * 
 * Common functionality for all ARM-based devices.
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




#ifndef HOSTARM_H
#define HOSTARM_H

#include <nyx/nyx_client.h>

#include "Common.h"

#include "HostBase.h"
#include "Event.h"
#include "CustomEvents.h"
#include "NyxSensorConnector.h"
#include "NyxInputControl.h"
#include "NyxLedControl.h"

#include "HidLib.h"
#include "lunaservice.h"

#include <qsocketnotifier.h>
#include <QObject>

#define KEYBOARD_TOKEN		"com.palm.properties.KEYoBRD"
#define KEYBOARD_QWERTY		"z"
#define KEYBOARD_AZERTY		"w"
#define KEYBOARD_QWERTZ		"y"

#define HIDD_LS_KEYPAD_URI		"palm://com.palm.hidd/HidKeypad/"
#define HIDD_RINGER_URI			HIDD_LS_KEYPAD_URI"RingerState"
#define HIDD_SLIDER_URI			HIDD_LS_KEYPAD_URI"SliderState"
#define HIDD_HEADSET_URI		HIDD_LS_KEYPAD_URI"HeadsetState"
#define HIDD_HEADSET_MIC_URI		HIDD_LS_KEYPAD_URI"HeadsetMicState"
#define HIDD_GET_STATE			"{\"mode\":\"get\"}"

#define HIDD_LS_ACCELEROMETER_URI	"palm://com.palm.hidd/HidAccelerometer/"
#define HIDD_ACCELEROMETER_RANGE_URI	HIDD_LS_ACCELEROMETER_URI"Range"

#define HIDD_ACCELEROMETER_MODE			HIDD_LS_ACCELEROMETER_URI"Mode"
#define HIDD_ACCELEROMETER_INTERRUPT_MODE	HIDD_LS_ACCELEROMETER_URI"InterruptMode"
#define HIDD_ACCELEROMETER_POLL_INTERVAL	HIDD_LS_ACCELEROMETER_URI"PollInterval"
#define HIDD_ACCELEROMETER_TILT_TIMER		HIDD_LS_ACCELEROMETER_URI"TiltTimer"
#define HIDD_ACCELEROMETER_SET_DEFAULT_TILT_TIMER	"{\"mode\":\"set\",\"value\":6}"
#define HIDD_ACCELEROMETER_SET_DEFAULT_INTERVAL		"{\"mode\":\"set\",\"value\":2000}"
#define HIDD_ACCELEROMETER_SET_DEFAULT_MODE		"{\"mode\":\"set\",\"value\":\"all\"}"
#define HIDD_ACCELEROMETER_SET_POLL			"{\"mode\":\"set\",\"value\":\"poll\"}"

// Same as default kernel values
#define REPEAT_DELAY	250
#define REPEAT_PERIOD	33

#if 0
// TODO: These don't get included from <linux/input.h> because the OE build
// picks up the include files from the codesourcery toolchain, not our modified headers
#define SW_RINGER		0x5
#define SW_SLIDER		0x6
#define SW_OPTICAL		0x7
#define ABS_ORIENTATION		0x30

// TODO: these should come from hidd headers
#define EV_GESTURE      0x06
#define EV_FINGERID	0x07
#endif

typedef enum
{
    BACK = 0,
    MENU,
    QUICK_LAUNCH,
    LAUNCHER,
    NEXT,
    PREV,
    FLICK,
    DOWN,
    HOME,
    HOME_LEFT,
    HOME_RIGHT,
    NUM_GESTURES
} GestureType_t;

/**
 * Base class for all ARM-based host device classes to derive from
 */
class HostArm : public HostBase, public NYXConnectorObserver
{
    Q_OBJECT
public:
	/**
	 * Constructs an ARM-based host device
	 * 
	 * Initializes common ARM hardware.
	 */
	HostArm();
	
	/**
	 * Shuts down ARM-specific hardware
	 */
	virtual ~HostArm();
	
	//Documented in parent
	virtual void init(int w, int h);
	
	//Documented in parent
	virtual void show();
	
	//Documented in parent
	virtual int getNumberOfSwitches() const;
	
	/**
	 * Asks the HIDD service for the state of each of the switches
	 * 
	 * Posts KeyPress/KeyRelease events to the
	 * event queue of the active window for each
	 * of the hardware switches of this device.
	 * 
	 * Possible keys include:
	 * - Qt::Key_unknown
	 * - Qt::Key_Ringer
	 * - Qt::Key_Slider
	 * - Qt::Key_HeadsetMic
	 * - Qt::Key_Headset
	 */
	virtual void getInitialSwitchStates(void);
	
	/**
	 * Reads input_event structures from a file descriptor and returns the number read
	 * 
	 * Mainly just a utility method for use
	 * within this class.  Very purpose-
	 * specific.
	 * 
	 * @see input_event
	 * 
	 * @param	fd			File descriptor to read from.
	 * @param	eventBuf		Buffer to read input_events into.
	 * @param	bufSize			Amount of allocated memory (in bytes) that eventBuf points to.
	 * @return				Returns the number of events read on success (may be 0) or -1 on failure.
	 */
	int readHidEvents(int fd, struct input_event* eventBuf, int bufSize);
	
	/**
	 * @copybrief HostBase::hardwareName()
	 * 
	 * Since this class is a generic base for
	 * ARM-based devices, this always returns
	 * "ARM Device".
	 * 
	 * @return				Returns the string "ARM Device".
	 */
	virtual const char* hardwareName() const;
	
	//Documented in parent
	virtual InputControl* getInputControlALS();
	
	//Documented in parent
	virtual InputControl* getInputControlBluetoothInputDetect();
	
	//Documented in parent
	virtual InputControl* getInputControlProximity();
	
	//Documented in parent
	virtual InputControl* getInputControlTouchpanel();
	
	//Documented in parent
	virtual InputControl* getInputControlKeys();

	virtual LedControl* getLedControlKeypadAndDisplay();
	
	/**
	 * Function enables/disables the orientation sensor
	 * 
	 * @param	enable			true to enable the orientation sensor, false to disable it.
	 */
	virtual void OrientationSensorOn(bool enable);
	
	//Documented in parent
	virtual void setBluetoothKeyboardActive(bool active);
	
	//Documented in parent
	virtual bool bluetoothKeyboardActive() const;

protected:
	/**
	 * Watches for changes in the ambient light sensor value and posts events to the event queue when they occur
	 * 
	 * Initialized by HostArm::setupInput().
	 * 
	 * @see QSocketNotifier
	 * @see HostArm::setupInput()
	 */
	QSocketNotifier* m_nyxLightNotifier;
	
	/**
	 * Watches for changes in the face proximity sensor value and posts events to the event queue when they occur
	 * 
	 * Initialized by HostArm::setupInput().
	 * 
	 * @see QSocketNotifier
	 * @see HostArm::setupInput()
	 */
	QSocketNotifier* m_nyxProxNotifier;

	/**
	 * Turns the LCD on
	 * 
	 * Not much more to say about this one.
	 * The function of it is pretty
	 * straightforward.
	 */
	virtual void wakeUpLcd();
	
	/**
	 * Remap a raw X value from the touchscreen to a pixel value
	 * 
	 * @todo Confirm this, since there's only one implementation of this at the moment.
	 * 
	 * @param	rawX			Raw X value from the touchscreen.
	 * @param	type			Type of event which gave us this coordinate value.
	 * @return				Screen X of the touch, in pixels.
	 */
	virtual int screenX(int rawX, Event::Type type) { return rawX; }
	
	/**
	 * Remap a raw Y value from the touchscreen to a pixel value
	 * 
	 * @todo Confirm this, since there's only one implementation of this at the moment.
	 * 
	 * @param	rawY			Raw Y value from the touchscreen.
	 * @param	type			Type of event which gave us this coordinate value.
	 * @return				Screen Y of the touch, in pixels.
	 */
	virtual int screenY(int rawY, Event::Type type) { return rawY; }
	
	//Documented in parent
	virtual void setCentralWidget(QWidget* view);
	
	/**
	 * Hardware revision
	 * 
	 * Initialized by the constructor
	 * ({@link HostArm::HostArm() HostArm::HostArm()}
	 * via
	 * {@link HidGetHardwareRevision() HidGetHardwareRevision()}.
	 * 
	 * @todo Document this more fully once HidGetHardwareRevision() is publicly documented.
	 */
	HidHardwareRevision_t m_hwRev;
	
	/**
	 * Hardware platform
	 * 
	 * Initialized by the constructor
	 * ({@link HostArm::HostArm() HostArm::HostArm()}
	 * via
	 * {@link HidGetHardwarePlatform() HidGetHardwarePlatform()}.
	 * 
	 * @todo Document this more fully once HidGetHardwarePlatform() is publicly documented.
	 */
	HidHardwarePlatform_t m_hwPlatform;
	
	/**
	 * File descriptor for the first framebuffer device (/dev/fb0, the LCD)
	 * 
	 * Initialized by
	 * {@link HostArm::init() HostArm::init()}.
	 */
	int m_fb0Fd;
	
	/**
	 * File descriptor for the secondary framebuffer device (/dev/fb1, for direct rendering)
	 * 
	 * Initialized by
	 * {@link HostArm::init() HostArm::init()}.
	 */
	int m_fb1Fd;
	
	/**
	 * Memory-mapped pointer to /dev/fb0
	 * 
	 * Write pixel data to this to display it.
	 */
	void* m_fb0Buffer;
	
	/**
	 * Number of buffers successfully memory mapped for /dev/fb0
	 * 
	 * Looks like this is where you can tell how
	 * many back buffers have been enabled for the
	 * device (in addition to the main buffer).
	 * This should be the total number of buffers
	 * for the device, including back buffers.
	 * 
	 * @todo Confirm the documentation on this.
	 */
	int m_fb0NumBuffers;
	
	/**
	 * Memory-mapped pointer to /dev/fb1
	 * 
	 * Write pixel data to this to display it to
	 * /dev/fb1.
	 */
	void* m_fb1Buffer;
	
	/**
	 * Number of buffers successfully memory mapped for /dev/fb1
	 * 
	 * Looks like this is where you can tell how
	 * many back buffers have been enabled for the
	 * device (in addition to the main buffer).
	 * This should be the total number of buffers
	 * for the device, including back buffers.
	 * 
	 * @todo Confirm the documentation on this.
	 */
	int m_fb1NumBuffers;
	
	/**
	 * IPC system connection
	 * 
	 * Appears to be initialized by
	 * {@link HostArm::startService() HostArm::startService()}.
	 */
	LSHandle* m_service;

	/**
	 * Nyx input control for the ambient light sensor
	 * 
	 * Initialized in
	 * {@link HostArm::getInputControlALS() HostArm::getInputControlALS()}.
	 */
	InputControl* m_nyxInputControlALS;
	
	/**
	 * Nyx input control for the whether or not a Bluetooth input device is connected
	 * 
	 * Initialized in
	 * {@link HostArm::getInputControlBluetoothInputDetect() HostArm::getInputControlBluetoothInputDetect()}.
	 * 
	 * @todo Confirm that this is what the Bluetooth input detect input control is for.
	 */
	InputControl* m_nyxInputControlBluetoothInputDetect;
	
	/**
	 * Nyx input control for the face proximity sensor
	 * 
	 * Initialized in
	 * {@link HostArm::getInputControlProximity() HostArm::getInputControlProximity()}.
	 */
	InputControl* m_nyxInputControlProx;
	
	/**
	 * Nyx input control for the keyboard
	 * 
	 * Initialized in
	 * {@link HostArm::getInputControlKeys() HostArm::getInputControlKeys()}.
	 * 
	 * @todo Confirm that this is in fact for the keyboard.
	 */
	InputControl* m_nyxInputControlKeys;
	
	/**
	 * Nyx input control for the touch panel
	 * 
	 * Initialized in
	 * {@link HostArm::getInputControlTouchpanel() HostArm::getInputControlTouchpanel()}.
	 */
	InputControl* m_nyxInputControlTouchpanel;
	LedControl* m_nyxLedControlKeypadAndDisplay;

	
	/**
	 * Whether or not a Bluetooth keyboard is active
	 * 
	 * Set by
	 * {@link HostArm::setBluetoothKeyboardActive() HostArm::setBluetoothKeyboardActive()}.
	 */
    bool m_bluetoothKeyboardActive;

	
	/**
	 * Nyx sensor connector for the orientation sensor
	 * 
	 * Initialized, enabled, and disabled by
	 * {@link HostArm::OrientationSensorOn() HostArm::OrientationSensorOn()}.
	 */
    NYXOrientationSensorConnector* m_OrientationSensor;

protected:
	/**
	 * Starts talking to this device's sensors
	 * 
	 * Calls both
	 * {@link HostArm::getInputControlALS() HostArm::getInputControlALS()}
	 * and
	 * {@link HostArm::getInputControlProximity() HostArm::getInputControlProximity()}
	 * to connect to HAL for the sensors.  After
	 * each, it also subscribes to notifiers for
	 * them so when either of them changes value
	 * an event is posted to our event queue.
	 * 
	 * @see QSocketNotifier
	 */
	void setupInput(void);
	
	/**
	 * Disconnects from HAL for each of the sensors
	 * 
	 * Sensors which are disconnected from include:
	 * - Ambient light sensor.
	 * - Face proximity sensor.
	 * - Touch panel.
	 * - Control keys (probably either hardware switches or the keyboard).
	 * 
	 * And closes down the HAL notifiers for:
	 * - Face proximity sensor.
	 * - Ambient light sensor.
	 * 
	 * @todo Figure out whether "control keys" refers to hardware switches or the keyboard.
	 */
	void shutdownInput(void);
	
	/**
	 * Attaches the main GLib event loop for this host to the IPC system
	 * 
	 * @todo Confirm this once LSRegister is publicly documented.
	 */
	void startService(void);
	
	/**
	 * Disconnects the main GLib event loop for this host from the IPC system
	 * 
	 * @todo Confirm this once LSUnregister is publicly documented.
	 */
	void stopService(void);
	
	/**
	 * Disable screen blanking and puts the screen into graphics mode
	 * 
	 * Enabled screen blanking to restore the screen
	 * from sleep, then immediately disables it.
	 * 
	 * After that, it puts the screen into grahics
	 * mode to get it ready to draw to (and hide
	 * the text terminal cursor).
	 * 
	 * When deciphering the implementation of this,
	 * see the manpage for console_ioctl,
	 * specifically TIOCLINUX subcodes 0 and 10.
	 */
	void disableScreenBlanking();
	
	//Documented in parent
	virtual void flip();
	
	//Documented in parent
	virtual QImage takeScreenShot() const;
	
	//Documented in parent
	virtual QImage takeAppDirectRenderingScreenShot() const;
	
	//Documented in parent
	virtual void setAppDirectRenderingLayerEnabled(bool enable);
	
	//Documented in parent
	virtual void setRenderingLayerEnabled(bool enable);
	
	/**
	 * Given a return value from an IPC call with a "value" element, returns the integer value of it
	 * 
	 * When an IPC call passes back a JSON structure
	 * in a format such as:
	 * 
	 * <code>
	 * {
	 *   value : "127"
	 * }
	 * </code>
	 * 
	 * It returns the integer value of the "value"
	 * element.
	 * 
	 * @param	msg			Return value message to parse from IPC call.
	 * @param	value			Where to store the integer version of the value.
	 * @return				true if parsing was successful, false if parsing failed.
	 */
	static bool getMsgValueInt(LSMessage* msg, int& value);
	
	/**
	 * Handle an event related to the hadware switches
	 * 
	 * Internal handler for switch state
	 * transitions.  Reads the switch information
	 * in and translates it into either a
	 * {@link QEvent::KeyPress QEvent::KeyPress}
	 * or
	 * {@link QEvent::KeyRelease QEvent::KeyRelease}
	 * based on the new value of the switch which
	 * is then dispatched to the active window's
	 * event queue.
	 * 
	 * @param	handle			IPC handle.
	 * @param	msg			IPC return value text.  Must be in JSON format.  See HostArm::getMsgValueInt().
	 * @param	data			Not a pointer, despite what it looks like.  The switch that changed state.  Can be SW_RINGER, SW_SLIDER, or SW_HEADPHONE_INSERT.
	 */
	static bool switchStateCallback(LSHandle* handle, LSMessage* msg, void* data);

    /**
     * Function gets called whenever there is some data
     * available from NYX.
     *
     * @param[in]   - aSensorType - Sensor which has got some data to report
     */
    virtual void NYXDataAvailable (NYXConnectorBase::Sensor aSensorType);

protected Q_SLOTS:
	/**
	 * Read the value of the ambient light sensor
	 * 
	 * Reads the value of the ambient light sensor
	 * and posts an AlsEvent event to the active
	 * window's event queue with the value IF it
	 * is able to successfully retrieve the value.
	 * 
	 * @note You MUST run HostArm::setupInput() (or HostArm::getInputControlALS() directly if you feel adventurous) before calling this method for it to actually do anything.
	 * 
	 * @see HostArm::getInputControlALS()
	 * @see AlsEvent
	 * 
	 * @todo Document the value range for the ambient light sensor.
	 */
	void readALSData();
	
	/**
	 * Read the value of the face proximity sensor
	 * 
	 * Reads the value of the face proximity
	 * sensor and posts an ProximityEvent event
	 * to the active window's event queue with
	 * the value IF it is able to successfully
	 * retrieve the value.
	 * 
	 * @note You MUST run HostArm::setupInput() (or HostArm::getInputControlProximity() directly if you feel adventurous) before calling this method for it to actually do anything.
	 * 
	 * @see HostArm::getInputControlProximity()
	 * @see ProximityEvent
	 * 
	 * @todo Document the value range for the face proximity sensor.
	 */
	void readProxData();
};

#endif /* HOSTARM_H */
