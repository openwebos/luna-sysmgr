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




#include "gfxsepiaeffect.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>
//public:
GfxSepiaEffect::GfxSepiaEffect(QObject * parent)
: GfxEffectBase(parent)
, m_p_pix(0)
{

}

//virtual
GfxSepiaEffect::~GfxSepiaEffect()
{
	delete m_p_pix;
}

//protected:

#include <QDebug>

//virtual
void GfxSepiaEffect::draw ( QPainter * painter )
{
	QPoint offset;
	if (!m_p_pix)
	{
		m_p_pix = new QPixmap(sourcePixmap(Qt::LogicalCoordinates, &m_offset));
		sepiaTone(m_p_pix);
	}
	painter->drawPixmap(m_offset, *m_p_pix);
}

//virtual
void GfxSepiaEffect::sepiaTone(QPixmap * p_pix)
{
	QImage img = p_pix->toImage();
	uchar * pMem=img.bits();

	double ored,ogreen,oblue;
	double ired,igreen,iblue;
	for (int j=0;j<img.height();j++) {
		uchar * pLine = img.scanLine(j);
		QRgb * pixel = (QRgb *)pLine;
		for (int i=0;i<img.width();i++) {
			unsigned char max = qMax(qRed(*pixel),(qMax(qGreen(*pixel),qBlue(*pixel))));
			double sf = ((double)max) /255.0;
			ired = qRed(*pixel);
			igreen = qGreen(*pixel);
			iblue = qBlue(*pixel);
			ored = (ired * .393) + (igreen *.769) + (iblue * .189);
			ogreen = (ired * .349) + (igreen *.686) + (iblue * .168);
			oblue = (ired * .272) + (igreen *.534) + (iblue * .131);

			img.setPixel(i,j,qRgb((ored > 255 ? 255 : (unsigned char)ored),
									(ogreen > 255 ? 255 : (unsigned char)ogreen),
									(oblue > 255 ? 255 : (unsigned char)oblue)));
			pixel++;
		}
	}
	p_pix->convertFromImage(img);
}
