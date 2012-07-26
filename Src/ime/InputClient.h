/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef INPUTCLIENT_H
#define INPUTCLIENT_H

#include <palmimedefines.h>

#include <string>

class InputClient
{
public:
    InputClient() : m_focus(false) {}

	virtual void setComposingText(const std::string& text) {}
	virtual void commitComposingText() {}

	virtual void commitText(const std::string& text) {}

	virtual void performEditorAction(PalmIME::FieldAction action) {}

    virtual void removeInputFocus();

    virtual bool inputFocus() const { return m_focus; }
    virtual const PalmIME::EditorState& inputState() const { return m_state; }
    void setInputFocus(bool focus) { m_focus = focus; }

protected:
    //void setInputFocus(bool focus) { m_focus = focus; }
    void setInputState(const PalmIME::EditorState& state) { m_state = state; }

private:
    PalmIME::EditorState m_state;
    bool m_focus;
};

#endif

