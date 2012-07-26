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




#include "demogoggles2.h"
#include "pixmapobject.h"
#include "dimensionsglobal.h"
#include <QString>
#include <QRectF>
#include <QPainter>
#include <QGLContext>

#define shaderprogfilepath QString("/var/luna/preferences/launcher3/blurshader1.frag")

DemoGoggles2::DemoGoggles2(const QRectF& geometry,PixmapObject * p_backgroundPmo,const QRectF& viewRect)
: ThingPaintable(geometry)
, m_qp_backgroundPmo(p_backgroundPmo)
, m_sceneBgViewrect(viewRect)
{
	if (!m_qp_backgroundPmo)
	{
		setFlag(ItemHasNoContents,true);
	}
	//load the shader
	if (!m_glShader.addShaderFromSourceFile(QGLShader::Fragment,shaderprogfilepath))
	{
		qDebug() << __FUNCTION__ << ": error loading shader:\n" << m_glShader.log();
	}
	else
	{
		qDebug() << __FUNCTION__ << ": shader loaded.";
	}
}

//virtual
DemoGoggles2::~DemoGoggles2()
{
	delete m_qp_backgroundPmo;
}

//virtual
void DemoGoggles2::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget)
{
	//map myself into the scene
	QRectF sceneRect = mapRectToScene(boundingRect());
	QRectF sourceRect = sceneRect.intersected(m_sceneBgViewrect).translated(-m_sceneBgViewrect.topLeft());
	painter->beginNativePainting();
	//invade the current GL context with the new shader;
	m_glShader.bind();
//	//set a value for the uniform blur var (see shader src code)
	m_glShader.setUniformValue("u_blurStep",(GLfloat)5.0);
	m_qp_backgroundPmo->paint(painter,boundingRect().toRect(),sourceRect.toRect());
	painter->endNativePainting();
}

//virtual
void DemoGoggles2::paintOffscreen(QPainter *painter)
{
}

//virtual
void DemoGoggles2::updateBackground(PixmapObject * p_newBgPmo,const QRectF& viewRect)
{
	delete m_qp_backgroundPmo;
	m_qp_backgroundPmo = p_newBgPmo;
	m_sceneBgViewrect = viewRect;
	if (!m_qp_backgroundPmo)
	{
		setFlag(ItemHasNoContents,true);
	}
	else
	{
		setFlag(ItemHasNoContents,false);
	}

}

//virtual
void DemoGoggles2::activate()
{

}

//virtual
void DemoGoggles2::deactivate()
{

}
