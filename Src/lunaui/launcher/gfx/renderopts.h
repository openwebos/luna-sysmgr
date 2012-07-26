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




#ifndef RENDEROPTS_H_
#define RENDEROPTS_H_

namespace IconRenderStage
{
	enum Enum
	{
		INVALID = 0,

		Icon = 1,
		IconFrame = 2,
		Decorators = 4,
		Label = 8,

		LAST = 8
	};
	enum Bits
	{
		NBITS = 4
	};
}

namespace LabeledDivRenderStage
{
	enum Enum
	{
		INVALID = 0,

		DivPix = 1,
		Label = 2,

		LAST = 2
	};
	enum Bits
	{
		NBITS = 2
	};
}

#endif /* RENDEROPTS_H_ */
