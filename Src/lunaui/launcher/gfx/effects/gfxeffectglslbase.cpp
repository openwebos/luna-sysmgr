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




static const char fragShader_identity[] = "\
	    lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) { \
	        return texture2D(imageTexture, textureCoords); \
	    }\n";

static char const fragShader_colorize[] =
        "uniform lowp vec4 effectColor;\n"
        "lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) {\n"
        "    vec4 src = texture2D(imageTexture, textureCoords);\n"
        "    float gray = dot(src.rgb, vec3(0.212671, 0.715160, 0.072169));\n"
        "    vec4 colorize = 1.0-((1.0-gray)*(1.0-effectColor));\n"
        "    return vec4(colorize.rgb, src.a);\n"
        "}";

static char const fragShader_grayscale[] =
        "lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) {\n"
        "    vec4 src = texture2D(imageTexture, textureCoords);\n"
        "    vec3 gray = src.rgb * vec3(0.212671, 0.715160, 0.072169);\n"
        "    return vec4(gray, src.a);\n"
        "}";


QMap<QString,QString> GfxEffectGLSLBase::s_fragmentShaderPrograms;

//public:
GfxEffectGLSLBase::GfxEffectGLSLBase(QObject * parent)
: GfxEffectBase(parent)
{
}

//virtual
GfxEffectGLSLBase::~GfxEffectGLSLBase()
{
}

//static
const QMap<QString,QString>& GfxEffectGLSLBase::globalFragmentShaders()
{
	//initializes the static map
	if (s_fragmentShaderPrograms.empty())
	{
		s_fragmentShaderPrograms["identity"] = QString(fragShader_identity);
		s_fragmentShaderPrograms["colorize"] = QString(fragShader_colorize);
		s_fragmentShaderPrograms["grayscale"] = QString(fragShader_grayscale);
	}

	return s_fragmentShaderPrograms;
}

//protected:

//virtual
void GfxEffectGLSLBase::draw ( QPainter * painter )
{
	//do nothing
}
