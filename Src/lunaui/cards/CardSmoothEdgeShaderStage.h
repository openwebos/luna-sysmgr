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




#ifndef CARDSMOOTHEDGESHADERSTAGE_H_
#define CARDSMOOTHEDGESHADERSTAGE_H_

#include "Common.h"

#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
#define USE_SMOOTHEDGE_SHADER 1
#else
#undef USE_SMOOTHEDGE_SHADER
#endif

#if defined(USE_SMOOTHEDGE_SHADER)

#include <QtOpenGL/qglcustomshaderstage_p.h>
#include <QDebug>

class CardSmoothEdgeShaderStage : public QGLCustomShaderStage
{
public:

        CardSmoothEdgeShaderStage()
		: m_srcWidth(0)
		, m_srcHeight(0)
		, m_dstWidth(0)
                , m_dstHeight(0)
		, m_scale(1.0)
        {
            static char const shaderSrc[] =
                    " \n\
                    uniform highp vec2 Start; \n\
                    uniform highp vec2 Center; \n\
                    uniform highp float Delta; \n\
                     lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) \n\
                     { \n\
                        highp vec2 CoordH = max((abs(textureCoords - (/*vec2(1.0,1.0) -*/ Center)) / Center), vec2(0.0)); \n\
                        lowp float AlphaH = smoothstep(1.0, (1.0-Delta), CoordH.y); \n\
                        highp vec2 CoordV = max((abs(textureCoords - Center)) / (Center), vec2(0.0)); \n\
                        lowp float AlphaV = smoothstep(1.0, (1.0-Delta), CoordV.x); \n\
                        lowp float Alpha = min(AlphaH, AlphaV); \n\
                        return vec4(texture2D(imageTexture, textureCoords).rgb * Alpha, Alpha); \n\
                     }\n";

            setSource(shaderSrc);
        }

        virtual void setUniforms(QGLShaderProgram* program) {
            program->setUniformValue("Center", (m_dstWidth * 0.5f) / m_srcWidth, (m_dstHeight * 0.5f) / m_srcHeight);
            program->setUniformValue("Delta", qFuzzyCompare(m_scale, 1.0f) ? 0.01f : 0.09f);
        }

        void setParameters(int srcWidth, int srcHeight, int dstWidth, int dstHeight) {

		m_srcWidth = srcWidth;
		m_srcHeight = srcHeight;
		m_dstWidth = dstWidth;
                m_dstHeight = dstHeight;

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
                << "scale" << m_scale;
    }

    void clone(CardSmoothEdgeShaderStage* other) const {

        other->m_srcWidth = m_srcWidth;
        other->m_srcHeight = m_srcHeight;
        other->m_dstWidth = m_dstWidth;
        other->m_dstHeight = m_dstHeight;
        other->m_scale = m_scale;
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
	float m_scale;
};

#endif

#endif

