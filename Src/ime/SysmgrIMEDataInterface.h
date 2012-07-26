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




#ifndef SYSMGRDATAINTERFACE_H
#define SYSMGRDATAINTERFACE_H

#include "IMEDataInterface.h"
#include "InputMethod.h"
#include <qnamespace.h>

class SysmgrIMEModel : public IMEDataInterface
{
	Q_OBJECT

public:
	SysmgrIMEModel();
	virtual ~SysmgrIMEModel() {}

	virtual void touchEvent(const QTouchEvent& te);
	virtual void paint(QPainter& painter);
	virtual void tapEvent(const QPoint& tapPt);
	virtual void screenEdgeFlickEvent();

	virtual void setComposingText(const std::string& text);
	virtual void commitComposingText();

	virtual void commitText(const std::string& text);

	virtual void performEditorAction(PalmIME::FieldAction action);

    virtual void sendKeyEvent(QEvent::Type type, Qt::Key key, Qt::KeyboardModifiers modifiers);

	virtual void requestHide();

	virtual bool isUIAnimationActive();

	virtual void keyDownAudioFeedback(Qt::Key key);

	void	setInputMethod(InputMethod * inputMethod);

private:
	InputMethod *	m_inputMethod;
};

#endif // SYSMGRDATAINTERFACE_H
