/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef DISPLAYSTATES_H
#define DISPLAYSTATES_H

#include "Common.h"

#include "Timer.h"
#include "Event.h"
#include "Settings.h"

// DisplayState: Different states of the display - each associated with a specific behavior
enum DisplayState {
    DisplayStateOff,
    DisplayStateOffOnCall,
    DisplayStateOn,
    DisplayStateOnLocked,
    DisplayStateDim,
    DisplayStateOnPuck,
    DisplayStateDockMode,
    DisplayStateOffSuspended,
    DisplayStateMax
};

// DisplayEvent: Events that could trigger a change in the display state
enum DisplayEvent {
    DisplayEventPowerKeyPress,
    DisplayEventPowerKeyHold,
    DisplayEventOnPuck,
    DisplayEventOffPuck,
    DisplayEventUsbIn,
    DisplayEventUsbOut,
    DisplayEventIncomingCall,
    DisplayEventIncomingCallDone,
    DisplayEventOnCall,
    DisplayEventOffCall,
    DisplayEventSliderOpen,
    DisplayEventSliderClose,
    DisplayEventAlsChange,
    DisplayEventProximityOn,
    DisplayEventProximityOff,
    DisplayEventApiOn,
    DisplayEventApiDim,
    DisplayEventApiOff,
    DisplayEventUserActivity,
    DisplayEventUpdateBrightness,
    DisplayEventLockScreen,
    DisplayEventUnlockScreen,
    DisplayEventTimeout,
    DisplayEventApiDock,
    DisplayEventApiUndock,
    DisplayEventPowerdSuspend,
    DisplayEventPowerdResume,
    DisplayEventUserActivityExternalInput,
    DisplayEventHomeKeyPress,
    DisplayEventMax
};

enum DisplayLockState {
    DisplayLockInvalid,
    DisplayLockLocked,
    DisplayLockUnlocked,
    DisplayLockDockMode
};

// forward declaration needed to use display manager
class DisplayManager;

// DisplayStateBase: base class of a display state
// specifies functions that all display states need to implement
class DisplayStateBase {
    protected:
        DisplayManager* dm;

    public:
        DisplayStateBase();

	virtual DisplayState state() const = 0;

	virtual void enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event = NULL) = 0;
	virtual void leave (DisplayState state, DisplayEvent displayEvent, sptr<Event> event = NULL) = 0;

	virtual void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL) = 0;

	virtual bool updateBrightness (int alsRegion);

	void changeDisplayState (DisplayState state, DisplayEvent displayEvent, sptr<Event> event = NULL);
	void updateLockState (DisplayLockState, DisplayEvent displayEvent);

        bool isDisplayUnlocked();
        bool isUSBCharging();
        bool isOnCall();
        bool isSliderOpen();
        bool isDNAST();
        bool isOnPuck();
        bool isProximityActivated();
        int  getCurrentAlsRegion();
	bool isBacklightOn();
	bool isDisplayOn();
	bool isTouchpanelOn();
	bool isDemo();

	int dimTimeout();
	int offTimeout();
	int lockedOffTimeout();
	int lastEvent();

        void updateLastEvent();
        virtual void stopInactivityTimer();
        virtual void startInactivityTimer();

	void displayOn(bool als);
	void displayDim();
	void displayOff();

	bool proximityOff();
	bool proximityOn();
    bool orientationSensorOn();
    bool orientationSensorOff();
	void enablePainting();
	void disablePainting();

	void emitDisplayStateChange(int);

	bool postPenCancel (int id = 0);
};


// DisplayOff: device on, display off
// This will change out of the current state for the following events
// change to OnLocked when power key up, phonecall in
// change to On usb in, slider open
// change to nightstand when placed on puck
// all other events do not cause change in state
class DisplayOff: public DisplayStateBase {
    public:
	DisplayState state() const { return DisplayStateOff; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);

};

// DisplayOffOnCall: device on, in phonecall, proximity triggered, display off
// This will change out of the current state for the following events
// change to On - proxmity triggered
// all other events do not cause change in state
class DisplayOffOnCall : public DisplayStateBase {
    public:
        DisplayOffOnCall();
	DisplayState state() const { return DisplayStateOffOnCall; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);

};


// DisplayOn: device on, display on, unlocked
// This will change out of the current state for the following events
// change to Off when power key up, quick inactivity fired
// change to daystand when placed on puck
// change to dim on inactivity
// all other events do not cause change in state
// incoming phonecall will turn on proximity sensor
// if state change was due to event, start slow inactivity timer
// if state change was due to direct call, start quick inactivity timer
// any change in als will cause brightness and core navi update

class DisplayOn : public DisplayStateBase {
    private:
	Timer<DisplayOn> *m_timerUser;
	Timer<DisplayOn> *m_timerInternal;

        void startUserInactivityTimer();
        void startInternalInactivityTimer();
    public:
        DisplayOn();

	DisplayState state() const { return DisplayStateOn; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);

        bool updateBrightness (int alsRegion);
	bool timeoutUser();
	bool timeoutInternal();
        void stopInactivityTimer();
        void startInactivityTimer();
};


// DisplayOnLocked: device on, display on, locked
// This will change out of the current state for the following events
// change to Off when power key up, inactivity fired
// change to DockMode when placed on puck
// change to On when USB plugged in, slider opened
// all other events do not cause change in state
// when state is entered, start inactivity timer
// any change in als will cause brightness and core navi update

class DisplayOnLocked : public DisplayStateBase {
    private:
	Timer<DisplayOnLocked> *m_timer;

    public:
        DisplayOnLocked();

	DisplayState state() const { return DisplayStateOnLocked; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);

        bool updateBrightness (int alsRegion);
	bool timeout();
        void stopInactivityTimer();
        void startInactivityTimer();
};

// DisplayDim: device on, display dim, unlocked
// This will change out of the current state for the following events
// change to On when usb in, incoming phonecall, slider open
// change to Nightstand when on puck,
// change to Off when slider close, inactivity
// all other events do not cause change in state
// when state is entered, start inactivity timer
// als events are ignored

class DisplayDim : public DisplayStateBase {
    private:
	Timer<DisplayDim> *m_timer;

    public:
        DisplayDim();
	DisplayState state() const { return DisplayStateDim; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);
	bool timeout();

        void stopInactivityTimer();
        void startInactivityTimer();
};


// DisplayOnPuck: device on, display on, on the puck
// This will change out of the current state for the following events
// change to On when off the puck
// change to DockMode when power key up, slider close, low als
// all other events do not cause change in state

class DisplayOnPuck : public DisplayStateBase {
    private:
	Timer<DisplayOnPuck> *m_timer;

    public:
        DisplayOnPuck();
	DisplayState state() const { return DisplayStateOnPuck; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);

        bool updateBrightness (int alsRegion);
	bool timeout();

        void stopInactivityTimer();
        void startInactivityTimer();
};


// DisplayDockMode: device on, display dim, on the puck
// This will change out of the current state for the following events
// change to On when off the puck
// change to OnPuck when power key up, incoming phonecall, usb in
// all other events do not cause change in state

class DisplayDockMode : public DisplayStateBase {
    public:
        DisplayDockMode();
	DisplayState state() const { return DisplayStateDockMode; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);

	bool timeoutExit();

    private:
	Timer<DisplayDockMode> * m_timerExit;

};

// DisplayOffSuspended: device off, display off
// This will change out of the current state for the following events
// powerd sends a resume call
// all other events do not cause change in state
class DisplayOffSuspended: public DisplayStateBase {
    private:
	DisplayState		   m_restoreState;
	DisplayEvent		   m_restoreDisplayEvent;
	sptr<Event>		   m_restoreEvent;

    public:
	DisplayOffSuspended();
	DisplayState state() const { return DisplayStateOffSuspended; }

	void enter (DisplayState, DisplayEvent, sptr<Event> = NULL);
	void leave (DisplayState, DisplayEvent, sptr<Event> = NULL);

	void handleEvent (DisplayEvent displayEvent, sptr<Event> event = NULL);

};


#endif // DISPLAYSTATES_H
