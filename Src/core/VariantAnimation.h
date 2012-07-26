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




#ifndef VARIANTANIMATION_H
#define VARIANTANIMATION_H

#include "Common.h"

#include <QVariantAnimation>

template <class Target>
class VariantAnimation : public QVariantAnimation
{
public:

	typedef void (Target::*CurrentValueChangedFunction)(const QVariant&);

	VariantAnimation(Target* target, CurrentValueChangedFunction func)
		: QVariantAnimation(target)
		, m_target(target)
		, m_func(func) {}

private:

	virtual void updateCurrentValue (const QVariant& value) {
		(m_target->*m_func)(value);
	}

	Target* m_target;
	CurrentValueChangedFunction m_func;
};

#endif /* VARIANTANIMATION_H */
