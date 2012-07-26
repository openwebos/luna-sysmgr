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




#ifndef ANIMATIONSETTINGS_H
#define ANIMATIONSETTINGS_H

#include "Common.h"

#include <glib.h>
#include <map>
#include <string>

#include "AnimationEquations.h"

class AnimationSettings
{
public:

	static AnimationSettings* instance() {

		if (G_UNLIKELY(s_instance == 0))
			new AnimationSettings;

		return s_instance;
	}

	bool setValue(const std::string& key, int value);
	bool getValue(const std::string& key, int& value) const;
	std::map<std::string, int> getAllValues() const;

    AnimationEquation easeInEquation(int strength) const;
    AnimationEquation easeOutEquation(int strength) const;

	// Animation FPS ------------------------------------

	int normalFPS;
	int slowFPS;
	
	// Card animations -----------------------------------
	
	int cardLaunchDuration;
	int cardLaunchCurve;
	int cardSlideDuration;
	int cardSlideCurve;
    int cardTrackGroupCurve;
    int cardTrackGroupDuration;
    int cardTrackCurve;
    int cardTrackDuration;
	int cardMaximizeDuration;
	int cardMaximizeCurve;
	int cardMinimizeDuration;
	int cardMinimizeCurve;
	int cardDeleteDuration;
	int cardDeleteCurve;
	int cardScootAwayOnLaunchDuration;
	int cardScootAwayOnLaunchCurve;
	int cardMoveNormalDuration;
	int cardMoveNormalCurve;
	int cardMoveOverviewDuration;
	int cardMoveOverviewCurve;
	int cardShuffleReorderDuration;
	int cardShuffleReorderCurve;
	int cardGroupReorderDuration;
	int cardGroupReorderCurve;

	int cardSwitchReachEndMaximizedDuration;
	int cardSwitchReachEndMaximizedCurve;
	int cardSwitchMaximizedDuration;
	int cardSwitchMaximizedCurve;

	int cardBeforeAddDelay;
	int cardPrepareAddDuration;
	int modalCardPrepareAddDuration;
	int cardAddMaxDuration;
	int modalCardAddMaxDuration;

	int cardLoadingPulsePauseDuration;
	int cardLoadingPulseDuration;
	int cardLoadingPulseCurve;
	int cardLoadingCrossFadeDuration;
	int cardLoadingCrossFadeCurve;
	int cardLoadingTimeBeforeShowingPulsing;

	int cardTransitionDuration;
	int cardTransitionCurve;

    int cardGhostDuration;
    int cardGhostCurve;

    int cardDimmingDuration;
    int cardDimmingCurve;

	// Positive/negative space animations -----------------

	int positiveSpaceChangeDuration;
	int positiveSpaceChangeCurve;

	// Launcher/QuickLaunch -------------------------------

	int quickLaunchDuration;
	int quickLaunchCurve;
	int quickLaunchSlideToStacheDuration;
	int quickLaunchSlideToStacheCurve;
	int quickLaunchFadeDuration;
	int quickLaunchFadeCurve;
	int launcherDuration;
	int launcherCurve;
	int universalSearchCrossFadeDuration;
	int universalSearchCrossFadeCurve;

	int launcherNormalModeArrowSlideDuration;
	int launcherNormalModeArrowSlideCurve;

	int launcherNormalModeScrollDuration;
	int launcherNormalModeScrollCurve;

	int launcherNormalModeSnapbackDuration;
	int launcherNormalModeSnapbackCurve;

	int launcherTransitionNormalToReorderDuration;
	int launcherTransitionNormalToReorderCurve;

	int launcherTransitionReorderToNormalDuration;
	int launcherTransitionReorderToNormalCurve;

	int launcherTransitionReorderToMiniDuration;
	int launcherTransitionReorderToMiniCurve;

	int launcherTransitionMiniToReorderDuration;
	int launcherTransitionMiniToReorderCurve;

	int launcherTransitionToItemReorderDuration;
	int launcherTransitionToItemReorderCurve;

	int launcherMiniModeCardSlideUnderDuration;
	int launcherMiniModeCardSlideUnderCurve;

	int launcherMiniModeScrollDuration;
	int launcherMiniModeScrollCurve;

	int launcherAddCardSlideCardsDuration;
	int launcherAddCardSlideCardsCurve;

	int launcherAddCardDuration;
	int launcherAddCardCurve;

	int launcherDeleteCardSlideCardsDuration;
	int launcherDeleteCardSlideCardsCurve;

	int launcherItemReorderVScrollDuration;
	int launcherItemReorderVScrollCurve;

	int launcherItemFadeDuration;
	int launcherItemFadeCurve;

	int launcherCardInnerVScrollDuration;

	int	launcherCardReorderScrollPauseDuration;

	// Reticle animations ---------------------------------
	
	int reticleDuration;
	int reticleCurve;

	// MSM animations -------------------------------------
	
	int brickDuration;
	int brickCurve;
	int progressPulseDuration;
	int progressPulseCurve;
	int progressFinishDuration;
	int progressFinishCurve;

	// Lock Screen animations -----------------------------
	
	int lockWindowFadeDuration;
	int lockWindowFadeCurve;
	int lockPinDuration;
	int lockPinCurve;
	int lockFadeDuration;
	int lockFadeCurve;

	// Dashboard animations -------------------------------
	
	int dashboardSnapDuration;
	int dashboardSnapCurve;
	int dashboardDeleteDuration;
	int dashboardDeleteCurve;

	// Status Bar animations -----------------------------------

	int statusBarFadeDuration;
	int statusBarFadeCurve;
	int statusBarColorChangeDuration;
	int statusBarColorChangeCurve;
	int statusBarTitleChangeDuration;
	int statusBarTitleChangeCurve;
	int statusBarTabFadeDuration;
	int statusBarTabFadeCurve;
	int statusBarArrowSlideDuration;
	int statusBarArrowSlideCurve;
	int statusBarItemSlideDuration;
	int statusBarItemSlideCurve;
	int statusBarMenuFadeDuration;
	int statusBarMenuFadeCurve;


	// DockMode animations -------------------------------
	int dockFadeScreenAnimationDuration;
	int dockFadeDockAnimationDuration;
	int dockFadeDockStartDelay;
	int dockFadeAnimationCurve;
	int dockRotationTransitionDuration;
	int dockCardSlideDuration;
	int dockCardSlideCurve;
	int dockMenuScrollDuration;
	int dockMenuScrollCurve;

	// Rotation animations -------------------------------
	int rotationAnimationDuration;
private:

	static AnimationSettings* s_instance;
	std::map<std::string, int*> m_values;

private:

	AnimationSettings();
	~AnimationSettings();

	void readSettings(const char* filePath);
};

// -------------------------------------------------------------------------------------------------------------

#define AS(x) (AnimationSettings::instance()->x)

#define AS_CURVE(x) (static_cast<QEasingCurve::Type>(AnimationSettings::instance()->x))

#define AS_EASEOUT(x) (AnimationSettings::instance()->easeOutEquation(x))

#define AS_EASEIN(x) (AnimationSettings::instance()->easeInEquation(x))

#endif /* ANIMATIONSETTINGS_H */
