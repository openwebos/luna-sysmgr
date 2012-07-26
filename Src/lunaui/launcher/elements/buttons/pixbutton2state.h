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




#ifndef PIXBUTTON2STATE_H_
#define PIXBUTTON2STATE_H_

#include "pixbutton.h"
#include <QPointer>
#include <QStateMachine>
#include <QState>

class PixButton2State : public PixButton
{
	Q_OBJECT
	Q_PROPERTY(bool stateActive READ stateActive WRITE setStateActive NOTIFY signalStateActiveChanged)

public:
	PixButton2State(const QString& label,PixmapObject * p_pixStateNormal,PixmapObject * p_pixStateActive);
	virtual ~PixButton2State();
	virtual void commonCtor();

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0,QWidget *widget=0);
	virtual void paintOffscreen(QPainter * painter);
	//TODO: need other paintOffscreen flavors, e.g. PixmapHugeObject

	virtual bool valid();

	virtual bool stateActive() const;
	virtual void setStateActive(bool v);

Q_SIGNALS:

	void signalStateActiveChanged();
	void signalFSMActivate();
	void signalFSMDeactivate();

protected:

	//overriding these to suppress them from generating signals
	virtual bool tapAndHoldGesture(QTapAndHoldGesture *tapHoldEvent);
	virtual bool tapGesture(QTapGesture *tapEvent);

	virtual void setupFSM();

protected:

	QPointer<PixmapObject>  m_qp_pixNormal;
	QPointer<PixmapObject>	m_qp_pixActive;
	QPointer<PixmapObject>	m_qp_currentlyRenderingPmo;

	bool			m_stateActive;
	QStateMachine *	m_p_buttonFSM;
	QState * m_p_stateNormal;
	QState * m_p_stateActive;
	bool m_valid;
};

#endif /* PIXBUTTON2STATE_H_ */
