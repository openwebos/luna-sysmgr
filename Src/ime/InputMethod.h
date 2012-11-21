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

// TODO (efigs): write proper doxygen docs

#ifndef INPUTMETHOD_H
#define INPUTMETHOD_H

#include <QTouchEvent>
#include <QPainter>

/*! \class InputMethod
 * \brief Interface for delivering events to input methods.
 *
 * Input methods such as virtual keyboards need to implement this interface to
 * be notified of and handle screen events and need to paint themselves.
 */
class InputMethod : public QObject
{
    Q_OBJECT

public:
    /*! \brief Pass a touch event to the input method.
     * \param te Touch event that occurred.
     */
    virtual void touchEvent(const QTouchEvent &te) = 0;

    /*! \brief Make the input method draw itself.
     * \param painter QPainter object to paint with.
     */
    virtual void paint(QPainter &painter) = 0;

    /*! \brief Notify input method that the screen was tapped.
     * \param tapPt The point of screen that was tapped.
     */
    virtual void tapEvent(const QPoint &tapPt) = 0;

    //! \brief Notify input method of a screen edge flick event.
    virtual void screenEdgeFlickEvent() = 0;
};

#endif

