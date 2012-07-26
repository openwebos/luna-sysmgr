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




#ifndef DIMENSIONSTYPES_H_
#define DIMENSIONSTYPES_H_

namespace DimensionsTypes
{

namespace AnimationType
{
	/*								AnimationType
	 *
	 * This attributes either animations for individual items (e.g. QPropertyAnimation on a qgfxitem), or groups of animations
	 * (e.g. QParallelAnimationGroup). Some tags only make sense in certain contexts; mostly this is all interpreted in
	 * "animation finished" signaled slots, to figure out how to finalize operations.
	 */
	enum Enum
	{
		INVALID,
		None,					//no tag (usually on anim group that contains multiple types)
		Add_HPan,				//horiz. pan that was needed to insert (add) a page - usually attributed to other pages; not the add'd page
		Add_Move,				//move (i.e. manual pos set) that was needed to insert a page - also for other pages
		Add,					//the actual add'd page's anim
		Remove_HPan,			// pan needed to remove a page	- you get the idea...
		Remove_Move,			// move needed to remove a page - ...
		Remove,					// the actual remove-ing page's anim
		HPan,					// general h-pan for scrolling everything right or left
		ZoomOut,				// zooming out of an item or multiple items
		ZoomIn,
		Rotate,					// for rotating things individually or multiply...
		PageVScroll				// for pages, scrolling up and down within them, including overscroll correction animations
	};
}

namespace ShowCause
{
	enum Enum
	{
		INVALID,
		None,
		User,					// index; use specific below
		UserQuicklaunch,
		UserSwipeup
	};
}

namespace HideCause
{
	enum Enum
	{
		INVALID,
		None,
		User,					//index
		UserQuicklaunch,
		UserHomePress,
		System,					//index
		SystemApplaunch
	};
}

namespace UiState
{
	enum Enum
	{
		INVALID,
		Shown,
		Hidden
	};
}

}
#endif /* DIMENSIONSTYPES_H_ */
