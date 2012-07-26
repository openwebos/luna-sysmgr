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




#ifndef ANIMATIONEQUATIONS_H
#define ANIMATIONEQUATIONS_H

#include "Common.h"

typedef int PValue;

class AnimationEquations
{
public:

	enum Type {
		EaseLinear,
		EaseIn,
		EaseOut,
		EaseInOut
	};

	static PValue easeLinear(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = time / duration;
		return initial + (final - initial) * time;
	}

	static PValue easeInQuad(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = time / duration;
		return initial + (final - initial) * (time * time);
	}

	static PValue easeInCubic(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = time / duration;
		return initial + (final - initial) * (time * time * time);
	}

	static PValue easeInQuat(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = time / duration;
		return initial + (final - initial) * (time * time * time * time);
	}

	static PValue easeInGeneric(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = time / duration;
		return initial + (final - initial) * (time ^ (strength / 10));
	}
	
	static PValue easeOutQuad(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = PValue(1) - time / duration;
		return initial + (final - initial) * (PValue(1) - time * time);
	}

	static PValue easeOutCubic(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = PValue(1) - time / duration;
		return initial + (final - initial) * (PValue(1) - time * time * time);
	}

	static PValue easeOutQuat(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = PValue(1) - time / duration;
		return initial + (final - initial) * (PValue(1) - time * time * time * time);
	}

	static PValue easeOutGeneric(PValue time, PValue initial, PValue final, PValue duration, PValue strength) {
		time = PValue(1) - time / duration;
		return initial + (final - initial) * (PValue(1) - (time ^ (strength / 10)));
	}
};

typedef PValue (*AnimationEquation)(PValue time, PValue initial, PValue final, PValue duration, PValue strength);


#endif /* ANIMATIONEQUATIONS_H */
