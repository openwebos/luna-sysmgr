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




#ifndef CARDROUNDEDCORNERSHADERSTAGE_H_
#define CARDROUNDEDCORNERSHADERSTAGE_H_

#include "Common.h"

#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
#define USE_ROUNDEDCORNER_SHADER 1
#else
#undef USE_ROUNDEDCORNER_SHADER
#endif

#if defined(USE_ROUNDEDCORNER_SHADER)

#include <QtOpenGL/qglcustomshaderstage_p.h>
#include <QDebug>

class CardRoundedCornerShaderStage : public QGLCustomShaderStage
{
public:

        CardRoundedCornerShaderStage()
		: m_srcWidth(0)
		, m_srcHeight(0)
		, m_dstWidth(0)
		, m_dstHeight(0)
		, m_radius(0)
		, m_scale(1.0)
        , m_active(1.0)
	{

		static char const shaderSrc[] =
            " \n\
             uniform highp vec2 Start; \n\
             uniform highp vec2 Center; \n\
             uniform highp float Delta; \n\
             uniform highp float Active; \n\
             lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) \n\
             { \n\
                 highp vec2 Coord  = max((abs(textureCoords - Center) - Start) / (Center - Start), vec2(0.0)); \n\
                 lowp float Alpha = smoothstep(1.0, 1.0 - Delta, length(Coord)); \n\
                 return vec4(texture2D(imageTexture, textureCoords).rgb * Alpha * Active, Alpha); \n\
             }\n";

		setSource(shaderSrc);
	}

	virtual void setUniforms(QGLShaderProgram* program) {

		float horizRadiusFactor = 0.5f - (m_radius * 1.0f)/m_dstWidth;
		float vertRadiusFactor = 0.5f - (m_radius * 1.0f)/m_dstHeight;


                if(m_radius>60){//EmulatedCardWindow
                    if (m_dstWidth > m_dstHeight) {
                            horizRadiusFactor = 0.491f;
                            vertRadiusFactor = 0.49f;
                    }
                    else {
                            horizRadiusFactor = 0.491f;
                            vertRadiusFactor = 0.49f;
                    }
                }
                else if(m_radius>50){//CardHostWindow
                    if (m_dstWidth > m_dstHeight) {
                            horizRadiusFactor = 0.491f;
                            vertRadiusFactor = 0.473f;
                    }
                    else {
                            horizRadiusFactor = 0.483f;
                            vertRadiusFactor = 0.478f;
                    }
                }
                else if (m_radius>45){//CardHostWindow in Port
                    horizRadiusFactor = 0.481f;
                    vertRadiusFactor = 0.487f;
                }
                else {
                    if (m_dstWidth > m_dstHeight) {
                            horizRadiusFactor = 0.491f;
                            vertRadiusFactor = 0.473f;
                    }
                    else {
                            horizRadiusFactor = 0.491f;
                            vertRadiusFactor = 0.478f;
                    }
                }


                //to keep from demonimator from going nevitive/breaking the shader
                float CenterX=(m_dstWidth * 0.5f) / m_srcWidth;
                float CenterY=(m_dstHeight * 0.5f) / m_srcHeight;

                horizRadiusFactor = qMin(horizRadiusFactor, CenterX);
                vertRadiusFactor  = qMin(vertRadiusFactor, CenterY);

                /*not tuned for sharper rounded corners
                if (m_scale > 0.5f) {
			horizRadiusFactor += (0.5f - horizRadiusFactor) * (m_scale - 0.5f) / 0.5f;
			vertRadiusFactor  += (0.5f - vertRadiusFactor)  * (m_scale - 0.5f) / 0.5f;
                        horizRadiusFactor = qMin(horizRadiusFactor, 0.48f);
                        vertRadiusFactor  = qMin(vertRadiusFactor, 0.48f);
                }*/

                program->setUniformValue("Start", horizRadiusFactor, vertRadiusFactor);
                program->setUniformValue("Center", (m_dstWidth * 0.5f) / m_srcWidth, (m_dstHeight * 0.5f) / m_srcHeight);
                program->setUniformValue("Delta", qFuzzyCompare(m_scale, 1.0f) ? 0.01f : 0.3f);
                program->setUniformValue("Active", m_active);
        }

	void setParameters(int srcWidth, int srcHeight, int dstWidth, int dstHeight, int radius, float active = 1.0f) {

		m_srcWidth = srcWidth;
		m_srcHeight = srcHeight;
		m_dstWidth = dstWidth;
		m_dstHeight = dstHeight;
		m_radius = radius;
        m_active = active;

		setUniformsDirty();
	}

    void setSourceParameters(int srcWidth, int srcHeight) {

        m_srcWidth = srcWidth;
        m_srcHeight = srcHeight;

        setUniformsDirty();
    }

    void dumpParameters() {
        qDebug() << "srcW" << m_srcWidth
                << "srcH" << m_srcHeight
                << "dstW" << m_dstWidth
                << "dstH" << m_dstHeight
                << "radius" << m_radius
                << "scale" << m_scale
                << "active" << m_active;
    }

    void clone(CardRoundedCornerShaderStage* other) const {

        other->m_srcWidth = m_srcWidth;
        other->m_srcHeight = m_srcHeight;
        other->m_dstWidth = m_dstWidth;
        other->m_dstHeight = m_dstHeight;
        other->m_radius = m_radius;
        other->m_scale = m_scale;
        other->m_active = m_active;
    }

    void setScale(float scale) {
        m_scale = scale;
        if (m_scale < 0.5f)
            m_scale = 0.5f;
        else if (m_scale > 1.0f)
            m_scale = 1.0f;
   }

private:

	int m_srcWidth;
	int m_srcHeight;
	int m_dstWidth;
	int m_dstHeight;
	int m_radius;
	float m_scale;
    float m_active;
};

#endif

#endif

