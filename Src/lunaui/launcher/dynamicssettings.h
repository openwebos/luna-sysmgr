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




#ifndef DYNAMICSSETTINGS_H_
#define DYNAMICSSETTINGS_H_

#include "Common.h"

#include <glib.h>
#include <QEasingCurve>
#include <QtGlobal>

namespace IconAnimationType
{
	enum Enum
	{
		INVALID,
		Fade
	};
}

class DynamicsSettings
{
public:

	quint32 snapbackAnimTime;
	QEasingCurve snapbackAnimCurve;

	quint32 iconReorderSampleRate;
	quint32 maxVelocityForSampling;
	quint32 timeNormalizationUnit;
	quint32 distanceMagFactor;

	quint32 iconReorderIconMoveAnimTime;
	QEasingCurve iconReorderIconMoveAnimCurve;
	bool	alwaysWaitForIconReorderAnimToFinish;

	bool	iconReorderTrackedIconUseDistanceBasedAnimTime;
	quint32 iconReorderTrackedIconMinAnimTime;					///used only if iconReorderTrackedIconUseDistanceBasedAnimTime = true

	bool	alphaIconMoveTrackedIconUseAnimation;
	IconAnimationType::Enum alphaIconMoveTrackedIconAnimType;
	QEasingCurve alphaIconMoveTrackedIconAnimCurve;
	quint32 alphaIconMoveTrackedIconAnimTime;

	quint32 pageScrollDelayMs;

	//TODO: TEMP: temporarily in place to avoid having to immediately implement and solve a bigger problem of determining, dynamically, the right amount
	//				to scroll vertically when an icon is being reordered and pulled to the top or bottom page border.
	//				This temp impl. will work as long as the row sizes are uniform (and the spacings between). It should be set to approx 1x row height + inter-row space
	quint32 pageScrollAmount;
	quint32 pageScrollAnimTime;

	quint32	pagePanForIconMoveDelayMs;

	qreal	iconInstallModeOpacity;		//when the app is installing/updating/failed, this is the opacity of the main icon painted

	quint32 appInfoDialogFadeInTime;
	quint32 appInfoDialogFadeOutTime;

	quint32 launchFeedbackTimeout;

public:
	static DynamicsSettings* DiUiDynamics() {

		if (G_UNLIKELY(s_instance == 0))
			new DynamicsSettings;

		return s_instance;
	}

	static DynamicsSettings* settings() {
		return DiUiDynamics();
	}

	static IconAnimationType::Enum animName2Type(const QString& name);
	static QString animType2Name(IconAnimationType::Enum a);

private:

	static DynamicsSettings* s_instance;

private:

	DynamicsSettings();
	~DynamicsSettings();

	void readSettings(const char* filePath);
	void verify();

};

// -------------------------------------------------------------------------------------------------------------

#define AS_CURVE(x) (static_cast<QEasingCurve::Type>(AnimationSettings::instance()->x))

#endif /* ANIMATIONSETTINGS_H */
