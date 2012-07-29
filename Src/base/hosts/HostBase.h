/**
 * @file
 * 
 * Base class for different system host classes.
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

#ifndef HOSTBASE_H
#define HOSTBASE_H

#include "Common.h"



#include "TaskBase.h"
#include "Mutex.h"
#include "CustomEvents.h"
#include "InputControl.h"
#include "LedControl.h"

#include <Qt>
#include <QImage>

class QWidget;

/**
 * Holds information about the device's display
 */
struct HostInfo
{
	/**
	 * Pointer to display memory buffer
	 */
	void* displayBuffer;
	
	/**
	 * Number of bytes per row
	 * 
	 * Some devices have extra padding memory after each row
	 * of actual pixel data to align the start of each row to
	 * a more convenient place in memory. This number is the
	 * number of actual bytes per row including that padding.
	 */
	int   displayRowBytes;
	
	/**
	 * Display width in number of pixels
	 */
	int   displayWidth;
	
	/**
	 * Display height in number of pixels
	 */
	int   displayHeight;
	
	/**
	 * Number of bits per pixel of the display
	 */
	int   displayDepth;
	
	/**
	 * Number of bits within each pixel that are devoted to the red channel
	 */
	int   displayRedLength;
	
	/**
	 * Number of bits within each pixel that are devoted to the green channel
	 */
	int   displayGreenLength;
	
	/**
	 * Number of bits within each pixel that are devoted to the blue channel
	 */
	int   displayBlueLength;
	
	/**
	 * Number of bits within each pixel that are devoted to the alpha channel
	 */
	int   displayAlphaLength;
};

/**
 * Base functionality for interfacing with the hardware of all devices which LunaSysMgr can run on
 * 
 * Provides information and capabilities such as:
 * - Screen size and other display information.
 * - Hardware keypress (dot, shift, and symbol keys) to system keypress translation (handled here since it could be different for different keyboards).
 * - Taking screenshots.
 * - Turning on and off diffferent device sensors.
 * - Bluetooth keyboard handling.
 */
class HostBase : public QObject,
                 public TaskBase
{
    Q_OBJECT
public:
	/**
	 * Gets a pointer to an instance of the current device-specific Host class
	 * 
	 * Since each host device will have different capabilities, this method
	 * allows you to retrieve the correct host information for the current
	 * device.
	 */
	static HostBase* instance();
	
	/**
	 * Clean up device-specific resources
	 * 
	 * Closes down any hardware initialized when this host was constructed.
	 * Designed to be overridden by each Host class for each type of device
	 * so that all hardware of specific devices is correctly shut down.
	 * 
	 * Just runs {@link HostBase::quit() HostBase::quit()} unless overridden
	 * by a derived class.
	 */
	virtual ~HostBase();
	
	/**
	 * Initialize device-specific hardware
	 * 
	 * Initializes hardware, fetches display info, and generally prepares
	 * the host device for LunaSysMgr to run on it.
	 * 
	 * Must be overridden for each type of device Host class needed.
	 * 
	 * @param	w			Hint as to what the screen width probably is.  Ignored by most devices since they can query the screen for its capabilities.
	 * @param	h			Hint as to what the screen height probably is.  Ignored by most devices since they can query the screen for its capabilities.
	 */
	virtual void init(int w, int h) = 0;
	
	/**
	 * Grab access to the hardware (including the display and input devices) for use by LunaSysMgr
	 * 
	 * Run this when you're ready to start using the host to display things.
	 * It grabs exclusive access to the screen and input devices (on most
	 * devices) so it can actually start displaying graphics on the screen.
	 * 
	 * Must be overridden per-device since it's fairly device-specific what
	 * needs to be enabled to be able to run LunaSysMgr.
	 */
	virtual void show() {}
	
	/**
	 * Gets a structure of information about the current host device
	 * 
	 * The HostInfo structure mostly contains information about the display
	 * device.  This allows retrieval of that from a central point.
	 * 
	 * @return				Structure full of information about the current host device.
	 */
	inline const HostInfo& getInfo() const { return m_info; }

	//Documented in parent.
	virtual void run();
	
	//Documented in parent.
	virtual void quit();
	
	/**
	 * Gets whether or not the current host is running under a Qemu virtual machine
	 * 
	 * @return				true if running under a Qemu virtual machine, otherwise false.
	 */
	static  bool hostIsQemu ();
	
	/**
	 * Return the specific name (including model revision) of the current host device
	 * 
	 * As an example, for a generic ARM device for which more information
	 * is not available, it returns "ARM Device".  For a Palm Pixie device,
	 * it might return something like "Pixie DVT1".
	 * 
	 * @return				Name of the hardware of the current host device.
	 */
	virtual const char* hardwareName() const = 0;

	/**
	 * Translate a hardware keycode to a system keycode depending on modifier keys
	 * 
	 * Takes a hardware keycode and which modifiers are currently pressed
	 * and returns a system modifier.  For example, if the key is the key
	 * code for "a" and shift is down, it translates it to the key code
	 * for "A".
	 * 
	 * @param	key			Input hardware key code.
	 * @param	withShift		Whether Shift is currently pressed.
	 * @param	withAlt			Whether Alt (dot key) is currently pressed.
	 * @return				Translated key code.
	 */
	virtual unsigned short translateKeyWithMeta( unsigned short key, bool withShift, bool withAlt );
	
	/**
	 * Locks the display's back buffer to draw to
	 * 
	 * If the display is currently being used by another thread, this
	 * method waits until the other method is done and exclusive access
	 * is granted before returning.
	 */
	void lockPainting() { m_paintMutex.lock(); }
	
	/**
	 * Unlocks the display's back buffer so other threads can draw to it
	 */
	void unlockPainting() { m_paintMutex.unlock(); }
	
	/**
	 * Sets whether or not the meta key is currently pressed
	 * 
	 * @param	metaKeyDown		true if the meta key is currently pressed, otherwise false.
	 */
	virtual void setMetaModifier( bool metaKeyDown ) { m_metaKeyDown = metaKeyDown; }
	
	/**
	 * Gets whether or not the meta key is currently pressed
	 * 
	 * @return				true if the meta key is currently pressed, otherwise false.
	 */
	virtual bool metaModifier() { return m_metaKeyDown; }
	
	/**
	 * Gets the number of hardware switches the current host device has
	 * 
	 * As an example, the Palm Pre has 3 switches:
	 * - Ringer on/off.
	 * - Slider opened/closed.
	 * - Headphones inserted/no headphones.
	 * 
	 * @return				Number of hardware switches the device has.
	 */
	virtual int getNumberOfSwitches() const { return 0; }
	
	/**
	 * Enables/disables subscribing to turbo mode
	 * 
	 * Counts the number of times callers turn on turbo mode and turns
	 * it on as long as anyone needs it.
	 * 
	 * @todo Figure out why it's exposed like this instead of having {@link HostBase::turboMode() HostBase::turboMode()} exposed directly.
	 * 
	 * @param	add			true to ask that turbo mode be turned on, false to tell HostBase that it's no longer needed.
	 */
	void turboModeSubscription(bool add);
	
	/**
	 * Sets the Qt widget that the display should reside in
	 * 
	 * Mostly unused except under the QtDesktop and Qemu hosts.
	 * 
	 * @todo Document this a bit more thoroughly once we figure out exactly why this is needed.
	 * 
	 * @param	view			Qt widget to display within.
	 */
	virtual void setCentralWidget(QWidget* view) {}
	
	/**
	 * Flips the back buffer to the display
	 * 
	 * Displays to the screen what is currently contained in the back
	 * buffer.  Since there is a back buffer, drawing can be completed
	 * on the back buffer at the program's leisure before displaying
	 * it in one swift move with a flip.
	 */
	virtual void flip() {}
	
	/**
	 * Maps Qt's version of the state of the modifier keys to whether or not the Alt (dot) key is pressed
	 * 
	 * On HostQtDesktop, both the Ctrl and Alt keys are mapped to the
	 * LunaSysMgr version of the Alt (dot) key.
	 * 
	 * @param	modifiers		Keybaord modifiers, as reported by Qt
	 * @return				Whether or not the current state of the hardware modifier keys indicates that the Alt (dot) key is pressed.
	 */
	virtual bool hasAltKey(Qt::KeyboardModifiers modifiers);
	
	/**
	 * Takes a screenshot
	 * 
	 * Currently only implemented on ARM devices.
	 * 
	 * @return				An image showing the current state of the display.
	 */
	virtual QImage takeScreenShot() const { return QImage(); }
	
	/**
	 * Takes a screenshot, including anything rendered directly to the display by an app
	 * 
	 * Currently only implemented on ARM devices, and on them is the same as a regular screenshot.
	 * 
	 * @return				An image showing the current state of the display, even if an app has drawn to it directly.
	 */
	virtual QImage takeAppDirectRenderingScreenShot() const { return QImage(); }
	
	/**
	 * Unknown purpose at this time
	 * 
	 * This is used in very few other places, so I don't know exactly what it's used for.
	 * On ARM devices, executes an FBIO command that I cannot currently find documentation
	 * for.  Seems to have something to do with blanking.
	 * 
	 * @todo Document what this is used for and how it works once it's figured out.
	 * 
	 * @param	enable			Unknown.
	 */
	virtual void setAppDirectRenderingLayerEnabled(bool enable) {}
	
	/**
	 * Sets the device's current orientation
	 * 
	 * Changes the display's transformation matrix so on-screen items
	 * display with the correct orientation.
	 * 
	 * @param	o			One of OrientationEvent::Orientation_Up, OrientationEvent::Orientation_Down, OrientationEvent::Orientation_Right, OrientationEvent::Orientation_Left indicating which edge of the screen is vertically "up".
	 */
	void setOrientation(OrientationEvent::Orientation o);
	
	/**
	 * Map a screen touch sensor point to a point on the display screen taking into account device orientation
	 * 
	 * @param	pt			Touchscreen point coordinates.
	 * @return				Translated display window coordinates translated using the same orientation rotation as the objects on-screen.
	 */
	QPoint map(const QPoint& pt) { return m_trans.map(pt); }
	
	/**
	 * Unknown purpose at this time
	 * 
	 * Almost the same as HostBase::setAppDirectRenderingLayerEnabled, but on a different
	 * fb device.  Has something to do with blanking, but still don't really understand
	 * what this is used for.
	 * 
	 * @param	enable			Unknown.
	 */
	virtual void setRenderingLayerEnabled(bool enable) {}
	
	/**
	 * Gets an InputControl pointer for the ambient light sensor
	 * 
	 * Allows the ambient light sensor to be turned on/off.
	 * 
	 * @return				Ambient light sensor control.
	 */
	virtual InputControl* getInputControlALS()           { return 0; }
	
	/**
	 * Gets an InputControl pointer for Bluetooth input detection
	 * 
	 * Allows the Bluetooth input detection to be turned on/off.
	 * 
	 * @return				Bluetooth input sensor control.
	 */
	virtual InputControl* getInputControlBluetoothInputDetect() { return 0; }
	
	/**
	 * Gets an InputControl pointer for the face proximity sensor
	 * 
	 * Allows the face proximity sensor to be turned on/off.
	 * 
	 * @return				Face proximity sensor control.
	 */
	virtual InputControl* getInputControlProximity()     { return 0; }
	
	/**
	 * Gets an InputControl pointer for the touch panel
	 * 
	 * Allows the touch panel to be turned on/off.
	 * 
	 * @return				Touch panel sensor control.
	 */
	virtual InputControl* getInputControlTouchpanel()    { return 0; }
	
	/**
	 * Gets an InputControl pointer for the keyboard
	 * 
	 * Allows the keyboard to be turned on/off.
	 * 
	 * @todo Confirm documentation on this.
	 * 
	 * @return				Control keys sensor control.
	 */
	virtual InputControl* getInputControlKeys()          { return 0; }
	
	virtual LedControl* getLedControlKeypadAndDisplay()  { return 0; }
	
	/**
	 * Function enables/disables the orientation sensor
	 * 
	 * @param	enable			true to enable the orientation sensor, false to disable it.
	 */
	virtual void OrientationSensorOn(bool enable) {}
	
	/**
	 * Function processes the Orientation of the device and returns the
	 * correct screen(display) orientation.
	 *
	 * @param currOrientation - Current device orientation
	 *
	 * @return returns the correct device specific orientation to be used for
	 *         device screen orientation
	 *
	 * @note ownership of the returned object is transferred back to the caller.
	 */
	virtual OrientationEvent* postProcessDeviceOrientation(OrientationEvent *currOrientation)
	{
	    if (currOrientation)
	    {
	        return (new OrientationEvent(currOrientation->orientation(),0,0));
	    }
	    return 0;
	}
	
	/**
	 * Presumably whether the Home button wakes up the screen
	 * 
	 * This is seldom used and largely undocumented.  Description is
	 * more or less a guess.
	 * 
	 * @todo Determine with certainty what this method is for.
	 * 
	 * @return				Presumably true if pressing the Home button wakes up the screen and false otherwise.
	 */
	virtual bool homeButtonWakesUpScreen() { return false; }
	
	/**
	 * Enable/disable Bluetooth keyboards
	 * 
	 * @todo Verify that the documentation for this method is correct.
	 * 
	 * @param	active			true to enable Bluetooth keyboards, false to disable
	 */
	virtual void setBluetoothKeyboardActive(bool active) {}
	
	/**
	 * Checks whether Bluetooth keyboard functionality is enabled
	 * 
	 * @todo Verify that the documentation for this method is correct.
	 * 
	 * @return				true if Bluetooth keyboard functionality is enabled, otherwise false.
	 */
	virtual bool bluetoothKeyboardActive() const { return false; }
	
	/**
	 * Unknown at this time
	 * 
	 * Do not currently know how this works or where it is defined.
	 * 
	 * @param	active			Unknown.
	 */
Q_SIGNALS:
	void signalBluetoothKeyboardActive(bool active);

protected:
	/**
	 * Constructor for HostBase
	 * 
	 * Defined as protected since HostBase cannot be constructed
	 * without being derived from.
	 * 
	 * Initializes GLib and resets a number of the "current state"
	 * variables to default states.
	 */
	HostBase();
	
	//Documented in parent.
	virtual void handleEvent(sptr<Event>) {}
	
	/**
	 * Enables/disables turbo mode
	 * 
	 * Changes CPU frequency scaling.  When turbo mode is enabled,
	 * the CPU is set to run at full speed.  When disabled, it
	 * turns the speed down to save on power usage.
	 * 
	 * @param	enable			true to enable turbo mode, false to disable it
	 */
	virtual void turboMode(bool enable) {}
	
	/**
	 * Host device info generated in HostBase::init() and cached here so it can be retrieved quickly
	 */
	HostInfo m_info;
	
	/**
	 * Mutex that allows locking of the back buffer while a function paints to it
	 */
	Mutex m_paintMutex;
	
	/**
	 * Whether or not the Meta key is currently pressed
	 */
	bool m_metaKeyDown;
	
	/**
	 * Current orientation of device
	 * 
	 * @see HostBase::setOrientation()
	 */
	OrientationEvent::Orientation m_orientation;
	
	/**
	 * Current transformation matrix used to rotate the display to match the current device orientation (so the screen image always faces "up")
	 */
	QTransform m_trans;
	
	/**
	 * Presumably the number of back buffers
	 * 
	 * Currently appears to be unused.
	 * 
	 * @todo Confirm documentation of this member variable.
	 * @todo Remove this if possible.
	 */
	int m_numBuffers;
	
	/**
	 * Number of running pieces of code which currently need turbo mode enabled
	 * 
	 * @see HostBase::turboModeSubscription()
	 */
	unsigned m_turboModeSubscriptions;
};

#endif /* HOSTBASE_H */
