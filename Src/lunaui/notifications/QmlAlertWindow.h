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




#ifndef QMLALERTWINDOW_H
#define QMLALERTWINDOW_H

#include "Common.h"

#include "AlertWindow.h"

class QDeclarativeComponent;
class QGraphicsObject;

class QmlAlertWindow : public AlertWindow
{
	Q_OBJECT

public:

	QmlAlertWindow(const QString& path, int width, int height);
	virtual ~QmlAlertWindow();

	virtual void resizeEventSync(int w, int h);
	virtual void close();

private Q_SLOTS:

	void slotClose();
	void slotMore();

private:

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	QDeclarativeComponent* m_qmlComp;
	QGraphicsObject* m_gfxObj;
};

#endif /* MEMORYALERTWINDOW_H */
