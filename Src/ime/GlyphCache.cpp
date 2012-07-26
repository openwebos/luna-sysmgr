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




#include "GlyphCache.h"
#include "Logging.h"
#include "IMEPixmap.h"

void initPixmapFragment(QPainter::PixmapFragment & fragment, const QPointF & topLeft, const QRectF & source)
{
	fragment.width = source.width();					fragment.height = source.height();
	fragment.x = topLeft.x() + fragment.width / 2;		fragment.y = topLeft.y() + fragment.height / 2;
	fragment.sourceLeft = source.left();				fragment.sourceTop = source.top();
	fragment.scaleX = 1;								fragment.scaleY = 1;
	fragment.rotation = 0;								fragment.opacity = 1;
}

void initPixmapFragment(QPainter::PixmapFragment & fragment, const QRectF & dest, const QRectF & source)
{
	fragment.width = source.width();					fragment.height = source.height();
	qreal destleft = dest.left();						qreal destright = dest.right();
	fragment.x = (destright + destleft) / 2;			fragment.scaleX = (destright - destleft) / fragment.width;
	qreal desttop = dest.top();							qreal destbottom = dest.bottom();
	fragment.y = (destbottom + desttop) / 2;			fragment.scaleY = (destbottom - desttop) / fragment.height;
	fragment.sourceLeft = source.left();				fragment.sourceTop = source.top();
	fragment.rotation = 0;								fragment.opacity = 1;
}
