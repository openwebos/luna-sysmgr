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




#ifndef IMEDATAINTERFACE_H
#define IMEDATAINTERFACE_H

#include "IMEData.h"

#if 0

/*

	The IMEData classes are the explicit equivalent to the following template class IMEData<T>
	Unfortunately, qmake doesn't support templates nor macros that define a QObject class.

	Expect the following methods:

	const T & get();				// access the last set value
	void set(const T &);			// change a value, which will trigger the signal...
	void valueChanged(const T &);	// ... to be fired.

*/

template <class T> class IMEData : public QObject
{
	Q_OBJECT

public:
	IMEData() { }

	const T &	get() const				{ return m_value; }
	void		set(const T & value)
	{
		if (!(value == m_value))
		{
			m_value = value;
			Q_EMIT valueChanged(value);
		}
	}

Q_SIGNALS:
	void valueChanged(const T & newValue);

private:
	T	m_value;
};
#endif


class IMEDataInterface : public QObject
{
	Q_OBJECT

public:

	//***************
	//* Host -> IME *
	//***************

	IMEData_QSize		m_screenSize;		// Total screen space. Will change when the device is rotated.
	IMEData_QRect		m_availableSpace;	// Space available for the keyboard, in absolute screen coordinate, within the screen space.
	IMEData_bool		m_visible;			// Tells whether the keyboard should be shown or hidden.
	IMEData_EditorState	m_editorState;		// Specifies the type of field currently focused. IME should IGNORE the shiftMode field and only refer to m_shiftMode below!
	IMEData_bool		m_autoCap;			// Tells that the SmartKey service will auto-capitalize the next charactet, which the keyboard should show.

	virtual void touchEvent(const QTouchEvent& te) = 0;
	virtual void tapEvent(const QPoint& tapPt) = 0;
	virtual void paint(QPainter& painter) = 0;
	virtual void screenEdgeFlickEvent() = 0;

	//***************
	//* IME -> Host *
	//***************

	IMEData_qint32		m_keyboardHeight;	// Height of virtual keyboard's main view, which includes the suggestion picker banner if there's one.
	IMEData_QRegion		m_hitRegion;		// defines an addition hit region that the IME wants input for.

    virtual void sendKeyEvent(QEvent::Type type, Qt::Key key, Qt::KeyboardModifiers modifiers) = 0;
	virtual void invalidateRect(const QRect& rect) { Q_EMIT signalInvalidateRect(rect); }

	virtual void setComposingText(const std::string& text) = 0;
	virtual void commitComposingText() = 0;

	virtual void commitText(const std::string& text) = 0;

	virtual void performEditorAction(PalmIME::FieldAction action) = 0;

	//*****************
	//* IME -> System *
	//*****************
	virtual void requestHide() = 0;
	virtual bool isUIAnimationActive() = 0;
	virtual void keyDownAudioFeedback(Qt::Key key) = 0;

Q_SIGNALS:
    void signalInvalidateRect(const QRect& rect);

public:
	virtual ~IMEDataInterface() {}
};

#endif // IMEDATAINTERFACE_H
