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




#include "renderedlabel.h"
#include <QFontMetrics>
#include <QTextBoundaryFinder>

RenderedLabel * RenderedLabel::createRenderedLabel(const QString& text)
{
	if (text.isEmpty())
		return NULL;

	return NULL;
}

RenderedLabel * RenderedLabel::createRenderedLabel(const QString& text,const QFont& font,const QPen& pen)
{
	if (text.isEmpty())
		return NULL;

	return NULL;
}

RenderedLabel * RenderedLabel::createRenderedLabel(const QString& text,const QFont& font,const QPen& pen,const QSize& maxBoundBox)
{
	if (text.isEmpty())
		return NULL;

	return NULL;
}

void RenderedLabel::render()
{
	if (!m_valid || m_rendered)
		return;

	QFontMetrics fm(m_font);
	QString modText = createElidedLabel(m_text,m_font,m_maxBoundBox);
	QRect br = fm.boundingRect(QRect(0, 0, m_maxBoundBox.width(), m_maxBoundBox.height()),
			Qt::AlignHCenter | Qt::TextWordWrap, modText);

	if (br.height() > fm.height()*2) {
		//Some word combinations fill 2 widths of text but word wrap onto 3 lines
		//we can't have that, and so we have to do a homebrew elide to fix it.
		int trimPoint = 0;
		if (modText.endsWith(QString("..."))) {
			trimPoint = modText.length()-3-1; //-1 to adjust length to point of last char
		} else {
			trimPoint = modText.length()-1; //-1 to adjust length to point of last char
			modText = modText.append(QString("..."));
		}

		while (br.height() > fm.height()*2 && trimPoint >= 0) {
			modText = modText.remove(trimPoint,1);
			trimPoint--;
			br = fm.boundingRect(QRect(0, 0, m_maxBoundBox.width(), m_maxBoundBox.height()),
					Qt::AlignHCenter | Qt::TextWordWrap, modText);

		}
	}

//	QPixmap labelPixmap(br.width(), br.height());
//	labelPixmap.fill(Qt::transparent);
//
//	QPainter labelPainter(&labelPixmap);
//	labelPainter.setPen(pen);
//	labelPainter.setFont(font);
//	labelPainter.setRenderHint(QPainter::TextAntialiasing);
//	if (Settings::LunaSettings()->launcherUsesHwAA)
//		labelPainter.setRenderHint(QPainter::HighQualityAntialiasing);
//	labelPainter.setCompositionMode(QPainter::CompositionMode_Source);
//	labelPainter.drawText(QRect(0, 0, br.width(), br.height()), modText, QTextOption(Qt::AlignHCenter));
//	labelPainter.end();
//
//	return labelPixmap;

}

//virtual
RenderedLabel::~RenderedLabel()
{
}

RenderedLabel::RenderedLabel()
: m_valid(false)
, m_rendered(false)
, m_usePen(false)
{
}

RenderedLabel::RenderedLabel(const QString& text)
: m_valid(false)
, m_rendered(false)
, m_text(text)
, m_usePen(false)
{
}

RenderedLabel::RenderedLabel(const QString& text,const QFont& font)
: m_valid(false)
, m_rendered(false)
, m_text(text)
, m_font(font)
{
}

RenderedLabel::RenderedLabel(const QString& text,const QFont& font,const QSize& maxBoundBox)
: m_valid(false)
, m_rendered(false)
, m_text(text)
, m_font(font)
, m_usePen(false)
, m_maxBoundBox(maxBoundBox)
{
}

RenderedLabel::RenderedLabel(const QString& text,const QFont& font,const QPen& pen)
: m_valid(false)
, m_rendered(false)
, m_text(text)
, m_font(font)
, m_usePen(true)
, m_pen(pen)
{
}

RenderedLabel::RenderedLabel(const QString& text,const QFont& font,const QPen& pen, const QSize& maxBoundBox)
: m_valid(false)
, m_rendered(false)
, m_text(text)
, m_font(font)
, m_usePen(true)
, m_pen(pen)
, m_maxBoundBox(maxBoundBox)
{

}

//static
void RenderedLabel::customBreakString(const QString& text,
									const int textBoundWidth,
									const int textBoundHeight,
									const QFontMetrics& fm,
									QList<QString>& elements,
									int * pLastIndexUsed)
{
	int ci=0;
	do {
		++ci;
		if (ci >= text.size())
			break;
	} while (fm.boundingRect(QRect(0, 0,textBoundWidth, textBoundHeight),Qt::AlignHCenter,text.left(ci)).width() < textBoundWidth);
	elements.push_back(text.left(ci));
	if (ci < text.size())
	{
		elements.push_back(text.right(text.size()-ci));
	}
	if (pLastIndexUsed)
		*pLastIndexUsed = ci;
}

//static
void RenderedLabel::textLabelElementsByLineBreaks(const QString& str,QList<QString>& elements,int offsetIntoString)
{
	QString s = str.simplified();
	QTextBoundaryFinder bf(QTextBoundaryFinder::Line,s.mid(offsetIntoString));
	int pb=offsetIntoString;
	int cb=0;
	while ((cb = bf.toNextBoundary()) != -1)
	{
		cb += offsetIntoString;
		if (cb == s.size()-1)
			++cb;
		elements.push_back(s.mid(pb,cb-pb));
		pb = cb+1;
		if (pb >= s.size())
			return;		//prevent the double boundary result found at the end of a string
	}
}

//static
QString RenderedLabel::createElidedLabel(const QString& srcText, const QFont& font,const QSize& bounds)
{

	QFontMetrics fm(font);

	int maxTextWidth = bounds.width();
	//check to see if the text can be simply elided by the default Qt algo
	QString modText = fm.elidedText (srcText, Qt::ElideRight,maxTextWidth*2);
	QRect br = fm.boundingRect(QRect(0, 0, maxTextWidth, bounds.height()),
								   Qt::AlignHCenter | Qt::TextWordWrap, modText);
	if (br.width() > maxTextWidth)
	{
		//it overflowed...have to do a custom eliding and linebreaking
		QList<QString> elements;
		textLabelElementsByLineBreaks(srcText,elements);
		//add elements one by one to fill the first line. If the first element doesn't fit, then split that element into 2...
		QList<QString> firstElementParts;
		customBreakString(elements[0],maxTextWidth,bounds.height(),fm,firstElementParts);
		modText.clear();
		for (int i=0;i<firstElementParts.size();++i)
			modText.append(firstElementParts.at(i)).append(' ');
		for (int i=1;i<elements.size();++i)
			modText.append(elements.at(i)).append(' ');
		modText = fm.elidedText(modText,Qt::ElideRight,maxTextWidth*2);
	}
	return modText;
}
