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




#ifndef RENDEREDLABEL_H_
#define RENDEREDLABEL_H_

#include <QPixmap>
#include <QString>
#include <QFont>
#include <QPen>
#include <QSize>
#include <QList>

class RenderedLabel : public QPixmap
{

	RenderedLabel * createRenderedLabel(const QString& text);
	RenderedLabel * createRenderedLabel(const QString& text,const QFont& font,const QPen& pen);
	RenderedLabel * createRenderedLabel(const QString& text,const QFont& font,const QPen& pen, const QSize& maxBoundBox);
	void render();

	virtual ~RenderedLabel();
protected:
	RenderedLabel();
	RenderedLabel(const QString& text);
	RenderedLabel(const QString& text,const QFont& font);
	RenderedLabel(const QString& text,const QFont& font,const QSize& maxBoundBox);
	RenderedLabel(const QString& text,const QFont& font,const QPen& pen);
	RenderedLabel(const QString& text,const QFont& font,const QPen& pen, const QSize& maxBoundBox);

	static QString createElidedLabel(const QString& srcText, const QFont& font,const QSize& bounds);
	static void textLabelElementsByLineBreaks(const QString& str,QList<QString>& elements,int offsetIntoString=0);
	static void customBreakString(const QString& text,
								const int textBoundWidth,
								const int textBoundHeight,
								const QFontMetrics& fm,
								QList<QString>& elements,
								int * pLastIndexUsed = 0);

	bool m_valid;
	bool m_rendered;

	QString m_text;
	QFont m_font;
	bool m_usePen;
	QPen m_pen;
	QSize m_maxBoundBox;
};

#endif /* RENDEREDLABEL_H_ */
