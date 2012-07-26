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




#ifndef CARDTRANSITION_H
#define CARDTRANSITION_H

#include "Common.h"

#include <string>
#include <stdint.h>

#include <QPainter>
#include <QPixmap>

#include "AnimationEquations.h"

class CardWindow;

class CardTransition
{
public:

	CardTransition(CardWindow* win,
				   QPixmap* fromPixmap,
				   const std::string& transitionName);
	~CardTransition();

	bool finished();
	
	void markCompleted(bool val) { m_completed = val; }
	bool completed() const { return m_completed; }

	// Returns true if transition is done
	void draw(QPainter* painter, const QPainterPath &path, bool maximized);

private:

	enum TransitionType {
		Transition_Invalid,
		Transition_ZoomAndCrossFade,
		Transition_CrossFade
	};

	struct AnimObject {
		PValue startScale;
		PValue targetScale;
		int startAlpha;
		int targetAlpha;
	};

private:

	void zoomAndCrossFadeTick(QPainter* painter, const QPainterPath &path, bool maximized);
	void crossFadeTick(QPainter* painter, const QPainterPath &path, bool maximized);

private:

	CardWindow* m_win;
	
	QPixmap* m_fromScenePixmap;

	AnimObject m_fromSceneAnimObject;
	AnimObject m_toSceneAnimObject;

	TransitionType m_transitionType;
	bool m_transitionIsPop;

	uint32_t m_startTime;
	bool m_completed;

	AnimationEquation m_equation;
};

#endif /* CARDTRANSITION_H */
