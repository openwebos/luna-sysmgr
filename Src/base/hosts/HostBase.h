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

struct HostInfo
{
	void* displayBuffer;
	int   displayRowBytes;
	int   displayWidth;
	int   displayHeight;
	int   displayDepth;
	int   displayRedLength;
	int   displayGreenLength;
	int   displayBlueLength;
	int   displayAlphaLength;
};

class HostBase : public QObject,
                 public TaskBase
{
    Q_OBJECT
public:

	static HostBase* instance();
	virtual ~HostBase();

	virtual void init(int w, int h) = 0;

	virtual void show() {}
	inline const HostInfo& getInfo() const { return m_info; }

	virtual void run();
	virtual void quit();
    static  bool hostIsQemu ();
	virtual const char* hardwareName() const = 0;

	virtual unsigned short translateKeyWithMeta( unsigned short key, bool withShift, bool withAlt );

	void lockPainting() { m_paintMutex.lock(); }
	void unlockPainting() { m_paintMutex.unlock(); }

	virtual void setMetaModifier( bool metaKeyDown ) { m_metaKeyDown = metaKeyDown; }
	virtual bool metaModifier() { return m_metaKeyDown; }

	virtual int getNumberOfSwitches() const { return 0; }

	void turboModeSubscription(bool add);

	virtual void setCentralWidget(QWidget* view) {}

	virtual void flip() {}

	virtual bool hasAltKey(Qt::KeyboardModifiers modifiers);

	virtual QImage takeScreenShot() const { return QImage(); }
	virtual QImage takeAppDirectRenderingScreenShot() const { return QImage(); }
	virtual void setAppDirectRenderingLayerEnabled(bool enable) {}

    void setOrientation(OrientationEvent::Orientation o);
	QPoint map(const QPoint& pt) { return m_trans.map(pt); }

	virtual void setRenderingLayerEnabled(bool enable) {}

	virtual InputControl* getInputControlALS()           { return 0; }
	virtual InputControl* getInputControlBluetoothInputDetect() { return 0; }
	virtual InputControl* getInputControlProximity()     { return 0; }
	virtual InputControl* getInputControlTouchpanel()    { return 0; }
	virtual InputControl* getInputControlKeys()          { return 0; }
	virtual LedControl* getLedControlKeypadAndDisplay()  { return 0; }

    /**
      * Function enables/disables the orientation sensor
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

	virtual bool homeButtonWakesUpScreen() { return false; }

    virtual void setBluetoothKeyboardActive(bool active) {}
    virtual bool bluetoothKeyboardActive() const { return false; }

Q_SIGNALS:
    void signalBluetoothKeyboardActive(bool active);

protected:

	HostBase();

	virtual void handleEvent(sptr<Event>) {}
	virtual void turboMode(bool enable) {}

	HostInfo m_info;
	Mutex m_paintMutex;
	bool m_metaKeyDown;
    OrientationEvent::Orientation m_orientation;
	QTransform m_trans;
	int m_numBuffers;
	unsigned m_turboModeSubscriptions;
};

#endif /* HOSTBASE_H */
