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




#include "gogglesblurshadereffect.h"
#include <QFile>
#include <QTextStream>
#include <QGLShaderProgram>

#define shaderprogfilepath QString("/var/luna/preferences/launcher3/blurshader2.frag")

static char const shaderCode[] =
		"uniform lowp float blurAmt;"
		"lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) {"
		"lowp float step = blurAmt / 100.0;"
		"lowp vec4 sample0 = texture2D ( imageTexture,vec2(textureCoords.x - step,textureCoords.y - step));"
		"lowp vec4 sample1 = texture2D ( imageTexture,vec2(textureCoords.x + step,textureCoords.y + step));"
		"lowp vec4 sample2 = texture2D ( imageTexture,vec2(textureCoords.x + step,textureCoords.y - step));"
		"lowp vec4 sample3 = texture2D ( imageTexture,vec2(textureCoords.x - step,textureCoords.y + step));"
		"return ((sample0 + sample1 + sample2 + sample3)/4.0);"
		"}"
;

static char const trivialShaderCode[] =
		"lowp vec4 customShader(lowp sampler2D imageTexture, highp vec2 textureCoords) {"
		"return vec4(1.0,0.0,0.0,1.0);"
		"}"
;

GogglesBlurShaderEffect::GogglesBlurShaderEffect(QObject *parent)
: QGraphicsShaderEffect(parent), blurAmt(1.0)
{
	setPixelShaderFragment(shaderCode);
}

qreal GogglesBlurShaderEffect::effectBlurAmount() const
{
	return blurAmt;
}

void GogglesBlurShaderEffect::setEffectBlurAmount(const qreal& a)
{
	blurAmt = a;
	setUniformsDirty();
}

////protected:

void GogglesBlurShaderEffect::setUniforms(QGLShaderProgram *program)
{
	program->setUniformValue("blurAmt", (GLfloat)blurAmt);
}

//static
QByteArray GogglesBlurShaderEffect::readShaderSourceCode(const QString& filepath)
{
	QFile file(filepath);
	if (!file.open(QIODevice::ReadOnly)) {
		return QByteArray();
	}
	QTextStream in(&file);
	return in.readAll().toAscii();
}
