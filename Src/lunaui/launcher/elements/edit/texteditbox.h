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




#ifndef TEXTEDITBOX_H_
#define TEXTEDITBOX_H_

#include <QGraphicsTextItem>
#include <QObject>

class QString;
class QFocusEvent;
class QGraphicsSceneMouseEvent;

class TextEditBox : public QGraphicsTextItem
{
	Q_OBJECT

public:
	TextEditBox(QGraphicsItem * p_parent = 0);
	TextEditBox(const QString& initialText,QGraphicsItem * p_parent = 0);
	virtual ~TextEditBox();

protected:

	virtual void focusInEvent(QFocusEvent * event);

	virtual void mousePressEvent (QGraphicsSceneMouseEvent * event);
};

#endif /* TEXTEDITBOX_H_ */
