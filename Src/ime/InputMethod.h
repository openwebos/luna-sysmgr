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
 * class description
 */
class InputMethod : public QObject
{
    Q_OBJECT

public:
    /*! desc
     * \param te */
    virtual void touchEvent(const QTouchEvent &te) = 0;

    /*! desc
     * \param painter */
    virtual void paint(QPainter &painter) = 0;

    /*! desc
     * \param tapPt */
    virtual void tapEvent(const QPoint &tapPt) = 0;

    /*! desc */
    virtual void screenEdgeFlickEvent() = 0;
};

#endif

