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




#ifndef STATUSBARTITLE_H
#define STATUSBARTITLE_H


#include <QGraphicsObject>
#include <QTextLayout>
#include "StatusBarItem.h"
#include "VariantAnimation.h"

#include "StatusBar.h"


class StatusBarTitle : public StatusBarItem
{
	Q_OBJECT

public:
	StatusBarTitle(int width, int height, bool classicui = false);
	~StatusBarTitle();

	QRectF boundingRect() const;  // This item is Left Aligned (The position  of the icon is the position of the LEFT EDGE of the bounding rect)
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	int width() const { return m_bounds.width(); }
	int height() const { return m_bounds.height(); }

	void setTitleString (std::string title, bool showTitleBorder);

private:

	void createTitlePixmap(QPixmap *pix, QRectF &rect);
	void animValueChanged(const QVariant& value);

	void animateTitleTransition();

private Q_SLOTS:
	void slotAnimFinished();

private:

	QFont* m_font;
	QPixmap m_titleBackground;
	QPixmap m_currentTitle, m_newTitle;
	QRectF m_curRect, m_newRect;

	QString m_originalText;
	QString m_elidedText;
	bool m_showTitleBorder;
	QRectF m_titleRect;
	QRectF m_maxSize;
	bool m_forceClassicUi;

	bool m_inTransition;
	qreal m_newTitleOpacity;
	VariantAnimation<StatusBarTitle> m_anim;

};



#endif /* STATUSBARTITLE_H */
