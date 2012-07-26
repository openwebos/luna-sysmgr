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




#ifndef SIGNALSLOT_H
#define SIGNALSLOT_H

#include "Common.h"

#include <set>
#include <stdio.h>

class Trackable;

class Sender
{
public:
	
	virtual void disconnectTrackable(Trackable* recv) = 0;
};

class Trackable {
public:

	virtual ~Trackable() {
		for (std::set<Sender*>::iterator it = m_senders.begin();
			 it != m_senders.end(); ++it) {
			(*it)->disconnectTrackable(this);
		}
	}

	void connected(Sender* sender) {
		m_senders.insert(sender);
	}

	void disconnected(Sender* sender) {
		m_senders.erase(sender);
	}

private:

	std::set<Sender*> m_senders;
};


template <class Function>
class FunctionWrapper
{
public:

	inline Trackable* receiver() const {
		return m_receiver;
	}
	
protected:

	Trackable* m_receiver;
	Function m_function;
};


template <class Arg0=void, class Arg1=void, class Arg2=void, class Arg3=void, class Arg4=void>
class SlotBase
{
public:

	virtual void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) = 0;
};

template <>
class SlotBase<void, void, void, void, void>
{
public:
	
	virtual void fire() = 0;
};

template <class Arg0>
class SlotBase<Arg0, void, void, void, void>
{
public:
	
	virtual void fire(Arg0 arg0) = 0;
};

template <class Arg0, class Arg1>
class SlotBase<Arg0, Arg1, void, void, void>
{
public:
	
	virtual void fire(Arg0 arg0, Arg1 arg1) = 0;
};

template <class Arg0, class Arg1, class Arg2>
class SlotBase<Arg0, Arg1, Arg2, void, void>
{
public:
	
	virtual void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2) = 0;
};

template <class Arg0, class Arg1, class Arg2, class Arg3>
class SlotBase<Arg0, Arg1, Arg2, Arg3, void>
{
public:
	
	virtual void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3) = 0;
};



template <class Receiver, class Arg0=void, class Arg1=void, class Arg2=void, class Arg3=void, class Arg4=void>
class Slot : public SlotBase<Arg0, Arg1, Arg2, Arg3, Arg4>,
			 public FunctionWrapper<void (*)()>
{
public:

	typedef void (Receiver::*Function)(Arg0, Arg1, Arg2, Arg3, Arg4);
	
	Slot(Receiver* rec, Function func) {
		this->m_receiver = rec;
		this->m_function = func;
	}

	void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
		(static_cast<Receiver*>(this->m_receiver)->*(this->m_function))(arg0, arg1, arg2, arg3, arg4);
	}	
};

template <class Receiver>
class Slot<Receiver, void, void, void, void, void> : public SlotBase<>,
													 public FunctionWrapper<void (Receiver::*)()>
{
public:

	typedef void (Receiver::*Function)();

	Slot(Receiver* rec, Function func) {
		this->m_receiver = rec;
		this->m_function = func;
	}

	void fire() {
		(static_cast<Receiver*>(this->m_receiver)->*(this->m_function))();
	}
};

template <class Receiver, class Arg0>
class Slot<Receiver, Arg0, void, void, void, void> : public SlotBase<Arg0>,
													 public FunctionWrapper<void (Receiver::*)(Arg0)>

{
public:

	typedef void (Receiver::*Function)(Arg0);
	
	Slot(Receiver* rec, Function func) {
		Trackable* t = rec;
		this->m_receiver = t;
		this->m_function = func;
	}

	void fire(Arg0 arg) {
		(static_cast<Receiver*>(this->m_receiver)->*(this->m_function))(arg);
	}
};

template <class Receiver, class Arg0, class Arg1>
class Slot<Receiver, Arg0, Arg1, void, void, void> : public SlotBase<Arg0, Arg1>,
													 public FunctionWrapper<void (Receiver::*)(Arg0,Arg1)>
{
public:

	typedef void (Receiver::*Function)(Arg0, Arg1);
	
	Slot(Receiver* rec, Function func) {
		this->m_receiver = rec;
		this->m_function = func;
	}

	void fire(Arg0 arg0, Arg1 arg1) {
		(static_cast<Receiver*>(this->m_receiver)->*(this->m_function))(arg0, arg1);
	}	
};

template <class Receiver, class Arg0, class Arg1, class Arg2>
class Slot<Receiver, Arg0, Arg1, Arg2, void, void> : public SlotBase<Arg0, Arg1, Arg2>,
													 public FunctionWrapper<void (Receiver::*)(Arg0,Arg1,Arg2)>
{
public:

	typedef void (Receiver::*Function)(Arg0, Arg1, Arg2);
	
	Slot(Receiver* rec, Function func) {
		this->m_receiver = rec;
		this->m_function = func;
	}

	void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2) {
		(static_cast<Receiver*>(this->m_receiver)->*(this->m_function))(arg0, arg1, arg2);
	}	
};

template <class Receiver, class Arg0, class Arg1, class Arg2, class Arg3>
class Slot<Receiver, Arg0, Arg1, Arg2, Arg3, void> : public SlotBase<Arg0, Arg1, Arg2, Arg3>,
													 public FunctionWrapper<void (Receiver::*)(Arg0,Arg1,Arg2,Arg3)>
{
public:

	typedef void (Receiver::*Function)(Arg0, Arg1, Arg2, Arg3);
	
	Slot(Receiver* rec, Function func) {
		this->m_receiver = rec;
		this->m_function = func;
	}

	void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3) {
		(this->m_receiver->*(this->m_function))(arg0, arg1, arg2, arg3);
	}	
};





template <class Arg0=void, class Arg1=void, class Arg2=void, class Arg3=void, class Arg4=void>
class SignalBase : public Sender
{
public:

	typedef Slot<Trackable, Arg0, Arg1, Arg2, Arg3, Arg4> Sl;
	
	virtual ~SignalBase() {
		for (typename SlotSet::const_iterator it = this->m_slots.begin();
			 it != this->m_slots.end(); ++it) {
			Sl* s = static_cast<Sl*>(*it);
			s->receiver()->disconnected(this);
			delete (*it);
		}
	}

	void connect(SlotBase<Arg0, Arg1, Arg2, Arg3, Arg4>* slot) {
		m_slots.insert(slot);
	}

	void connect(Trackable* recv, SlotBase<Arg0, Arg1, Arg2, Arg3, Arg4>* slot) {
		m_slots.insert(slot);
		recv->connected(this);
	}

	void disconnect(Trackable* recv) {
		typename SlotSet::iterator it = this->m_slots.begin();
		typename SlotSet::iterator itEnd = this->m_slots.end();
		while (it != itEnd) {
			Sl* s = static_cast<Sl*>(*it);
			if (s->receiver() == recv) {
				delete (*it);
				this->m_slots.erase(it++);
				continue;
			}

			++it;
		}
	}

	virtual void disconnectTrackable(Trackable* recv) {
		disconnect(recv);
	}
	
protected:

	typedef std::set<SlotBase<Arg0, Arg1, Arg2, Arg3, Arg4>* > SlotSet;
	SlotSet m_slots;
};

template <class Arg0=void, class Arg1=void, class Arg2=void, class Arg3=void, class Arg4=void>
class Signal : public SignalBase<Arg0, Arg1, Arg2, Arg3, Arg4>
{
public:

	template <class Receiver>
	void connect(Receiver* rec, void (Receiver::*func)(Arg0, Arg1, Arg2, Arg3, Arg4)) {
		SignalBase<Arg0>::connect(rec, new Slot<Receiver, Arg0, Arg1, Arg2, Arg3, Arg4>(rec, func));
	}

	void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
		typename std::set<SlotBase<Arg0, Arg1, Arg2, Arg3, Arg4>* >::iterator it = this->m_slots.begin();
		while (it != this->m_slots.end()) {
			(*it++)->fire(arg0, arg1, arg2, arg3, arg4);
		}
	}
};

	
template <>
class Signal<void, void, void, void, void> : public SignalBase<>
{
public:

	template <class Receiver>
	void connect(Receiver* rec, void (Receiver::*func)()) {
		SignalBase<>::connect(rec, new Slot<Receiver>(rec, func));
	}
		
	void fire() {
		std::set<SlotBase<>* >::iterator it = this->m_slots.begin();
		while (it != this->m_slots.end()) {
			(*it++)->fire();
		}
	}
};

template <class Arg0>
class Signal<Arg0, void, void, void, void> : public SignalBase<Arg0>
{
public:

	template <class Receiver>
	void connect(Receiver* rec, void (Receiver::*func)(Arg0)) {
		SignalBase<Arg0>::connect(rec, new Slot<Receiver, Arg0>(rec, func));
	}

	void fire(Arg0 arg0) {
		typename std::set<SlotBase<Arg0>* >::iterator it = this->m_slots.begin();
		while (it != this->m_slots.end()) {
			(*it++)->fire(arg0);
		}
	}
};

template <class Arg0, class Arg1>
class Signal<Arg0, Arg1, void, void, void> : public SignalBase<Arg0, Arg1>
{
public:

	template <class Receiver>
	void connect(Receiver* rec, void (Receiver::*func)(Arg0, Arg1)) {
		SignalBase<Arg0, Arg1>::connect(rec, new Slot<Receiver, Arg0, Arg1>(rec, func));
	}

	void fire(Arg0 arg0, Arg1 arg1) {
		typename std::set<SlotBase<Arg0, Arg1>* >::iterator it = this->m_slots.begin();
		while (it != this->m_slots.end()) {
			(*it++)->fire(arg0, arg1);
		}
	}
};

template <class Arg0, class Arg1, class Arg2>
class Signal<Arg0, Arg1, Arg2, void, void> : public SignalBase<Arg0, Arg1, Arg2>
{
public:

	template <class Receiver>
	void connect(Receiver* rec, void (Receiver::*func)(Arg0, Arg1, Arg2)) {
		SignalBase<Arg0, Arg1, Arg2>::connect(rec, new Slot<Receiver, Arg0, Arg1, Arg2>(rec, func));
	}

	void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2) {
		typename std::set<SlotBase<Arg0, Arg1, Arg2>* >::iterator it = this->m_slots.begin();
		while (it != this->m_slots.end()) {
			(*it++)->fire(arg0, arg1, arg2);
		}
	}
};

template <class Arg0, class Arg1, class Arg2, class Arg3>
class Signal<Arg0, Arg1, Arg2, Arg3, void> : public SignalBase<Arg0, Arg1, Arg2, Arg3>
{
public:

	template <class Receiver>
	void connect(Receiver* rec, void (Receiver::*func)(Arg0, Arg1, Arg2, Arg3)) {
		SignalBase<Arg0, Arg1, Arg2, Arg3>::connect(rec, new Slot<Receiver, Arg0, Arg1, Arg2, Arg3>(rec, func));
	}

	void fire(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3) {
		typename std::set<SlotBase<Arg0, Arg1, Arg2, Arg3>* >::iterator it = this->m_slots.begin();
		while (it != this->m_slots.end()) {
			(*it++)->fire(arg0, arg1, arg2, arg3);
		}
	}
};

/*
 *
 * Testing code 

class Rec : public Trackable {
public:

	void memFun0() {
		printf("%s\n", __PRETTY_FUNCTION__);
	}

	void memFun1(int a0) {
		printf("%s: %d\n", __PRETTY_FUNCTION__, a0);
	}

	void memFun2(int a0, double a1) {
		printf("%s: %d, %g\n", __PRETTY_FUNCTION__, a0, a1);
	}

	void memFun3(int a0, double a1, char a2) {
		printf("%s: %d, %g, %d\n", __PRETTY_FUNCTION__, a0, a1, a2);
	}

	void memFun4(int a0, double a1, char a2, float a3) {
		printf("%s: %d, %g, %d, %f\n", __PRETTY_FUNCTION__, a0, a1, a2, a3);
	}
};

int main() {

	Signal<> sig0;
	Signal<int> sig1;
	Signal<int, double> sig2;
	Signal<int, double, char> sig3;
	Signal<int, double, char, float> sig4;
	
	{
		Rec r;
		
		sig0.connect(&r, &Rec::memFun0);
		sig1.connect(&r, &Rec::memFun1);
		sig2.connect(&r, &Rec::memFun2);
		sig3.connect(&r, &Rec::memFun3);
		sig4.connect(&r, &Rec::memFun4);

		sig0.fire();
		sig1.fire(1);
		sig2.fire(1, 2.0);
		sig3.fire(1, 2.0, 3);
		sig4.fire(1, 2.0, 3, 4.0f);
	}

	sig0.fire();
	sig1.fire(1);
	sig2.fire(1, 2.0);
	sig3.fire(1, 2.0, 3);
	sig4.fire(1, 2.0, 3, 4.0f);
}

*/

#endif // SIGNALSLOT_H


