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




#include "Common.h"

#include "AnimationSettings.h"
#include <QEasingCurve>

static const char* kSettingsFile = "/etc/palm/lunaAnimations.conf";
static const char* kSettingsFilePlatform = "/etc/palm/lunaAnimations-platform.conf";

AnimationSettings* AnimationSettings::s_instance = 0;

#define SETUP_INTEGER(cat, var)									\
	{															\
		int _v;													\
		GError* _error = 0;										\
		_v=g_key_file_get_integer(keyFile,cat,#var,&_error);	\
		if( !_error ) { var=_v; }								\
		else { g_error_free(_error); }							\
																\
		m_values[#var] = &var;									\
	}

AnimationSettings::AnimationSettings()
	: normalFPS(35), slowFPS(20)
	, cardLaunchDuration(400), cardLaunchCurve(40)
	, cardSlideDuration(300), cardSlideCurve(10)
	, cardTrackGroupDuration(300), cardTrackGroupCurve(0)
	, cardTrackDuration(300), cardTrackCurve(10)
	, cardMaximizeDuration(300), cardMaximizeCurve(10)
	, cardMinimizeDuration(350), cardMinimizeCurve(10)
	, cardDeleteDuration(300), cardDeleteCurve(6)
	, cardScootAwayOnLaunchDuration(400), cardScootAwayOnLaunchCurve(30)
	, cardMoveNormalDuration(100), cardMoveNormalCurve(30)
	, cardMoveOverviewDuration(100), cardMoveOverviewCurve(30)
	, cardShuffleReorderDuration(350), cardShuffleReorderCurve(9)
	, cardGroupReorderDuration(350), cardGroupReorderCurve(9)
	, cardSwitchReachEndMaximizedDuration(200), cardSwitchReachEndMaximizedCurve(6)
	, cardSwitchMaximizedDuration(350), cardSwitchMaximizedCurve(6)
	, cardBeforeAddDelay(0)
	, cardPrepareAddDuration(150)
	, modalCardPrepareAddDuration(5)
	, cardAddMaxDuration(750)
	, modalCardAddMaxDuration(10)
	, cardLoadingPulsePauseDuration(1000)
	, cardLoadingPulseDuration(1000), cardLoadingPulseCurve(1)
	, cardLoadingCrossFadeDuration(300), cardLoadingCrossFadeCurve(0)
	, cardLoadingTimeBeforeShowingPulsing(900)
	, cardTransitionDuration(300), cardTransitionCurve(20)
    , cardGhostDuration(300), cardGhostCurve(0)
    , cardDimmingDuration(300), cardDimmingCurve(6)
	, positiveSpaceChangeDuration(400), positiveSpaceChangeCurve(6)
	, quickLaunchDuration(350), quickLaunchCurve(6)
	, quickLaunchSlideToStacheDuration(150), quickLaunchSlideToStacheCurve(6)
	, quickLaunchFadeDuration(200), quickLaunchFadeCurve(6)
	, launcherDuration(200), launcherCurve(2)
	, universalSearchCrossFadeDuration(200), universalSearchCrossFadeCurve(6)
	, launcherNormalModeArrowSlideDuration(1000)
	, launcherNormalModeArrowSlideCurve(QEasingCurve::OutExpo)
	, launcherNormalModeScrollDuration(200)
	, launcherNormalModeScrollCurve(2)
	, launcherNormalModeSnapbackDuration(200)
	, launcherNormalModeSnapbackCurve(2)
	, launcherTransitionNormalToReorderDuration(200)
	, launcherTransitionNormalToReorderCurve(2)
	, launcherTransitionReorderToNormalDuration(200)
	, launcherTransitionReorderToNormalCurve(2)
	, launcherTransitionReorderToMiniDuration(200)
	, launcherTransitionReorderToMiniCurve(2)
	, launcherTransitionMiniToReorderDuration(200)
	, launcherTransitionMiniToReorderCurve(2)
	, launcherTransitionToItemReorderDuration(200)
	, launcherTransitionToItemReorderCurve(2)
	, launcherMiniModeCardSlideUnderDuration(200)
	, launcherMiniModeCardSlideUnderCurve(2)
	, launcherMiniModeScrollDuration(200)
	, launcherMiniModeScrollCurve(2)
	, launcherAddCardSlideCardsDuration(200)
	, launcherAddCardSlideCardsCurve(2)
	, launcherAddCardDuration(200)
	, launcherAddCardCurve(2)
	, launcherDeleteCardSlideCardsDuration(200)
	, launcherDeleteCardSlideCardsCurve(2)
	, launcherItemReorderVScrollDuration(200)
	, launcherItemReorderVScrollCurve(2)
	, launcherItemFadeDuration(200)
	, launcherItemFadeCurve(2)
	, launcherCardInnerVScrollDuration(200)
	, launcherCardReorderScrollPauseDuration(1000)
	, reticleDuration(200), reticleCurve(0)
	, brickDuration(300), brickCurve(0)
	, progressPulseDuration(2000), progressPulseCurve(1)
	, progressFinishDuration(500), progressFinishCurve(1)
	, lockWindowFadeDuration(150), lockWindowFadeCurve(1)
	, lockPinDuration(250), lockPinCurve(15)
	, lockFadeDuration(200), lockFadeCurve(0)
	, dashboardSnapDuration(200), dashboardSnapCurve(6)
	, dashboardDeleteDuration(200), dashboardDeleteCurve(0)
	, statusBarFadeDuration(300), statusBarFadeCurve(0)
	, statusBarColorChangeDuration(300), statusBarColorChangeCurve(0)
	, statusBarTitleChangeDuration(300), statusBarTitleChangeCurve(0)
	, statusBarTabFadeDuration(500), statusBarTabFadeCurve(0)
	, statusBarArrowSlideDuration(500), statusBarArrowSlideCurve(0)
	, statusBarItemSlideDuration(500), statusBarItemSlideCurve(0)
	, statusBarMenuFadeDuration(200), statusBarMenuFadeCurve(0)
	, dockFadeScreenAnimationDuration(900)
	, dockFadeDockAnimationDuration(500)	
	, dockFadeDockStartDelay(270)
	, dockFadeAnimationCurve(3)
	, dockRotationTransitionDuration(600)
	, dockCardSlideDuration(300), dockCardSlideCurve(7)
	, dockMenuScrollDuration (150), dockMenuScrollCurve (10)
	, rotationAnimationDuration(300)
{
	s_instance = this;

	readSettings(kSettingsFile);
	readSettings(kSettingsFilePlatform);
}

AnimationSettings::~AnimationSettings()
{
    s_instance = 0;
}

void AnimationSettings::readSettings(const char* filePath)
{
    GKeyFile* keyFile = g_key_file_new();

	if (!g_key_file_load_from_file(keyFile, filePath,
								   G_KEY_FILE_NONE, NULL)) {
		goto done;
	}

	SETUP_INTEGER("FPS", normalFPS);
	SETUP_INTEGER("FPS", slowFPS);
	
	SETUP_INTEGER("Cards", cardLaunchDuration);
	SETUP_INTEGER("Cards", cardLaunchCurve);
		
	SETUP_INTEGER("Cards", cardSlideDuration);
	SETUP_INTEGER("Cards", cardSlideCurve);

	SETUP_INTEGER("Cards", cardTrackDuration);
	SETUP_INTEGER("Cards", cardTrackCurve);
	
    SETUP_INTEGER("Cards", cardTrackGroupDuration);
	SETUP_INTEGER("Cards", cardTrackGroupCurve);
	
	SETUP_INTEGER("Cards", cardMaximizeDuration);
	SETUP_INTEGER("Cards", cardMaximizeCurve);
	
	SETUP_INTEGER("Cards", cardMinimizeDuration);
	SETUP_INTEGER("Cards", cardMinimizeCurve);
	
	SETUP_INTEGER("Cards", cardDeleteDuration);
	SETUP_INTEGER("Cards", cardDeleteCurve);
	
	SETUP_INTEGER("Cards", cardScootAwayOnLaunchDuration);
	SETUP_INTEGER("Cards", cardScootAwayOnLaunchCurve);

	SETUP_INTEGER("Cards", cardMoveNormalDuration);
	SETUP_INTEGER("Cards", cardMoveNormalCurve);

	SETUP_INTEGER("Cards", cardMoveOverviewDuration);
	SETUP_INTEGER("Cards", cardMoveOverviewCurve);

	SETUP_INTEGER("Cards", cardShuffleReorderDuration);
	SETUP_INTEGER("Cards", cardShuffleReorderCurve);
	SETUP_INTEGER("Cards", cardGroupReorderDuration);
	SETUP_INTEGER("Cards", cardGroupReorderCurve);

	SETUP_INTEGER("Cards", cardSwitchReachEndMaximizedDuration);
	SETUP_INTEGER("Cards", cardSwitchReachEndMaximizedCurve);

	SETUP_INTEGER("Cards", cardSwitchMaximizedDuration);
	SETUP_INTEGER("Cards", cardSwitchMaximizedCurve);

	SETUP_INTEGER("Cards", cardBeforeAddDelay);
	SETUP_INTEGER("Cards", cardPrepareAddDuration);
	SETUP_INTEGER("Cards", modalCardPrepareAddDuration);
	SETUP_INTEGER("Cards", cardAddMaxDuration);
	SETUP_INTEGER("Cards", modalCardAddMaxDuration);
	
	SETUP_INTEGER("Cards", cardLoadingPulseDuration);
	SETUP_INTEGER("Cards", cardLoadingPulseCurve);

	SETUP_INTEGER("Cards", cardLoadingCrossFadeDuration);
	SETUP_INTEGER("Cards", cardLoadingCrossFadeCurve);

	SETUP_INTEGER("Cards", cardLoadingTimeBeforeShowingPulsing);

	SETUP_INTEGER("Cards", cardTransitionDuration);
	SETUP_INTEGER("Cards", cardTransitionCurve);

    SETUP_INTEGER("Cards", cardGhostDuration);
    SETUP_INTEGER("Cards", cardGhostCurve);

    SETUP_INTEGER("Cards", cardDimmingDuration);
    SETUP_INTEGER("Cards", cardDimmingCurve);

	SETUP_INTEGER("Spaces", positiveSpaceChangeDuration);
	SETUP_INTEGER("Spaces", positiveSpaceChangeCurve);
	
	SETUP_INTEGER("Launcher", quickLaunchDuration);
	SETUP_INTEGER("Launcher", quickLaunchCurve);
	
	SETUP_INTEGER("Launcher", quickLaunchSlideToStacheDuration);
	SETUP_INTEGER("Launcher", quickLaunchSlideToStacheCurve);
	
	SETUP_INTEGER("Launcher", quickLaunchFadeDuration);
	SETUP_INTEGER("Launcher", quickLaunchFadeCurve);
	
	SETUP_INTEGER("Launcher", launcherDuration);
	SETUP_INTEGER("Launcher", launcherCurve);

	SETUP_INTEGER("Launcher", universalSearchCrossFadeDuration);
	SETUP_INTEGER("Launcher", universalSearchCrossFadeCurve);

	SETUP_INTEGER("Launcher",launcherNormalModeArrowSlideDuration);
	SETUP_INTEGER("Launcher",launcherNormalModeArrowSlideCurve);
	SETUP_INTEGER("Launcher",launcherNormalModeScrollDuration);
	SETUP_INTEGER("Launcher",launcherNormalModeScrollCurve);
	SETUP_INTEGER("Launcher",launcherNormalModeSnapbackDuration);
	SETUP_INTEGER("Launcher",launcherNormalModeSnapbackCurve);
	SETUP_INTEGER("Launcher",launcherTransitionNormalToReorderDuration);
	SETUP_INTEGER("Launcher",launcherTransitionNormalToReorderCurve);
	SETUP_INTEGER("Launcher",launcherTransitionReorderToNormalDuration);
	SETUP_INTEGER("Launcher",launcherTransitionReorderToNormalCurve);
	SETUP_INTEGER("Launcher",launcherTransitionReorderToMiniDuration);
	SETUP_INTEGER("Launcher",launcherTransitionReorderToMiniCurve);
	SETUP_INTEGER("Launcher",launcherTransitionMiniToReorderDuration);
	SETUP_INTEGER("Launcher",launcherTransitionMiniToReorderCurve);
	SETUP_INTEGER("Launcher",launcherTransitionToItemReorderDuration);
	SETUP_INTEGER("Launcher",launcherTransitionToItemReorderCurve);
	SETUP_INTEGER("Launcher",launcherMiniModeCardSlideUnderDuration);
	SETUP_INTEGER("Launcher",launcherMiniModeCardSlideUnderCurve);
	SETUP_INTEGER("Launcher",launcherMiniModeScrollDuration);
	SETUP_INTEGER("Launcher",launcherMiniModeScrollCurve);
	SETUP_INTEGER("Launcher",launcherAddCardSlideCardsDuration);
	SETUP_INTEGER("Launcher",launcherAddCardSlideCardsCurve);
	SETUP_INTEGER("Launcher",launcherAddCardDuration);
	SETUP_INTEGER("Launcher",launcherAddCardCurve);
	SETUP_INTEGER("Launcher",launcherDeleteCardSlideCardsDuration);
	SETUP_INTEGER("Launcher",launcherDeleteCardSlideCardsCurve);
	SETUP_INTEGER("Launcher",launcherItemReorderVScrollDuration);
	SETUP_INTEGER("Launcher",launcherItemReorderVScrollCurve);
	SETUP_INTEGER("Launcher",launcherItemFadeDuration);
	SETUP_INTEGER("Launcher",launcherItemFadeCurve);
	SETUP_INTEGER("Launcher",launcherCardInnerVScrollDuration);
	SETUP_INTEGER("Launcher",launcherCardReorderScrollPauseDuration);

	SETUP_INTEGER("Reticle", reticleDuration);
	SETUP_INTEGER("Reticle", reticleCurve);

	SETUP_INTEGER("MSM", brickDuration);
	SETUP_INTEGER("MSM", brickCurve);

	SETUP_INTEGER("Lock", lockWindowFadeDuration);
	SETUP_INTEGER("Lock", lockWindowFadeCurve);

	SETUP_INTEGER("Lock", lockPinDuration);
	SETUP_INTEGER("Lock", lockPinCurve);

	SETUP_INTEGER("Lock", lockFadeDuration);
	SETUP_INTEGER("Lock", lockFadeCurve);

	SETUP_INTEGER("Dashboard", dashboardSnapDuration);
	SETUP_INTEGER("Dashboard", dashboardSnapCurve);	
	
	SETUP_INTEGER("Dock", dockFadeScreenAnimationDuration);
	SETUP_INTEGER("Dock", dockFadeDockAnimationDuration);	
	SETUP_INTEGER("Dock", dockFadeDockStartDelay);
	SETUP_INTEGER("Dock", dockFadeAnimationCurve);
	SETUP_INTEGER("Dock", dockRotationTransitionDuration);
	SETUP_INTEGER("Dock", dockCardSlideDuration);
	SETUP_INTEGER("Dock", dockCardSlideCurve);
	SETUP_INTEGER("Dock", dockMenuScrollDuration);
	SETUP_INTEGER("Dock", dockMenuScrollCurve);

	SETUP_INTEGER("StatusBar", statusBarFadeDuration);
	SETUP_INTEGER("StatusBar", statusBarFadeCurve);

	SETUP_INTEGER("StatusBar", statusBarColorChangeDuration);
	SETUP_INTEGER("StatusBar", statusBarColorChangeCurve);

	SETUP_INTEGER("StatusBar", statusBarTitleChangeDuration);
	SETUP_INTEGER("StatusBar", statusBarTitleChangeCurve);

	SETUP_INTEGER("StatusBar", statusBarTabFadeDuration);
	SETUP_INTEGER("StatusBar", statusBarTabFadeCurve);

	SETUP_INTEGER("StatusBar", statusBarArrowSlideDuration);
	SETUP_INTEGER("StatusBar", statusBarArrowSlideCurve);

	SETUP_INTEGER("StatusBar", statusBarItemSlideDuration);
	SETUP_INTEGER("StatusBar", statusBarItemSlideCurve);

	SETUP_INTEGER("StatusBar", statusBarMenuFadeDuration);
	SETUP_INTEGER("StatusBar", statusBarMenuFadeCurve);

	SETUP_INTEGER("Rotation", rotationAnimationDuration);
done:

	if (keyFile) {
		g_key_file_free(keyFile);
	}
}

bool AnimationSettings::setValue(const std::string& key, int value)
{
	std::map<std::string, int*>::iterator it = m_values.find(key);
	if (it == m_values.end())
		return false;

	*((*it).second) = value;
	return true;
}

bool AnimationSettings::getValue(const std::string& key, int& value) const
{
	std::map<std::string, int*>::const_iterator it = m_values.find(key);
	if (it == m_values.end())
		return false;

	value = *((*it).second);
	return true;    
}

std::map<std::string, int> AnimationSettings::getAllValues() const
{
	std::map<std::string, int> allValues;
	
    for (std::map<std::string, int*>::const_iterator it = m_values.begin();
		 it != m_values.end(); ++it) {
		allValues[(*it).first] = *((*it).second);
	}

	return allValues;
}

AnimationEquation AnimationSettings::easeInEquation(int strength) const
{
	if (G_UNLIKELY(strength < 10))
		strength = 10;
	
	switch (strength) {
	case (10):
		return AnimationEquations::easeLinear;
	case (20):
		return AnimationEquations::easeInQuad;
	case (30):
		return AnimationEquations::easeInQuat;
	case (40):
		return AnimationEquations::easeInCubic;
	default:
		return AnimationEquations::easeInGeneric;
    }
}

AnimationEquation AnimationSettings::easeOutEquation(int strength) const
{
	if (G_UNLIKELY(strength < 10))
		strength = 10;
	
	switch (strength) {
	case (10):
		return AnimationEquations::easeLinear;
	case (20):
		return AnimationEquations::easeOutQuad;
	case (30):
		return AnimationEquations::easeOutCubic;
	case (40):
		return AnimationEquations::easeOutQuat;
	default:
		return AnimationEquations::easeOutGeneric;
    }    
}

