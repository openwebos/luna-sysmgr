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




#include "StatusBarTitle.h"
#include "Settings.h"
#include "stdio.h"
#include "AnimationSettings.h"

#include <QPainter>

#define TITLE_WIDTH_PADDING   2

#define BKGD_LEFT_WIDTH    13
#define BKGD_RIGHT_WIDTH   20

#define NO_BORDER_TITLE_LEFT_PADDING    7
#define BORDER_TITLE_LEFT_PADDING       (-4)
#define TEXT_BASELINE_OFFSET            (-2)

StatusBarTitle::StatusBarTitle(int width, int height, bool classicUi)
	: StatusBarItem(StatusBarItem::AlignLeft)
	, m_font(0)
	, m_showTitleBorder (false)
    , m_inTransition(false)
    , m_newTitleOpacity(0.0)
	, m_anim(this, &StatusBarTitle::animValueChanged)
	, m_forceClassicUi(false)
{
	m_forceClassicUi = classicUi;

	m_maxSize = QRectF(0, -height/2, width, height);
	m_bounds = m_maxSize;

	m_curRect = m_bounds;
	m_newRect = m_bounds;

	const char* fontName = Settings::LunaSettings()->fontStatusBar.c_str();
	m_font = new QFont(fontName, 14);
	m_font->setPixelSize(14);

	m_font->setLetterSpacing(QFont::PercentageSpacing, kStatusBarQtLetterSpacing);

	if (m_font) {
		m_font->setBold(true);
	}

	QFontMetrics fontMetrics (*m_font);

	connect(&m_anim, SIGNAL(finished()), SLOT(slotAnimFinished()));

	Settings* settings = Settings::LunaSettings();
	std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";

	std::string filePath = statusBarImagesPath + "appname-background.png";
	m_titleBackground = QPixmap (filePath.c_str());
	if (m_titleBackground.isNull())
		g_warning ("Unable to open %s", filePath.c_str());


	if(!Settings::LunaSettings()->tabletUi || m_forceClassicUi) {
		m_newTitle = QPixmap(width + BKGD_LEFT_WIDTH + BKGD_RIGHT_WIDTH, height);
		m_currentTitle = QPixmap(width + BKGD_LEFT_WIDTH + BKGD_RIGHT_WIDTH, height);
	} else {
		m_newTitle     = QPixmap(width, height);
		m_currentTitle = QPixmap(width, height);
	}

	m_originalText = " ";
	setTitleString(std::string("Unknown"), false);
}


StatusBarTitle::~StatusBarTitle()
{
	delete m_font;
}


QRectF StatusBarTitle::boundingRect() const
{
	return m_bounds;
}

void StatusBarTitle::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(!m_inTransition) {
		painter->drawPixmap(m_bounds, m_currentTitle, QRectF(0, 0, m_bounds.width(), m_bounds.height()));
	} else {
		qreal oldOpacity = painter->opacity();

		QRectF tmp = m_curRect.intersected(m_bounds);
		painter->setOpacity(1.0 - m_newTitleOpacity);
		painter->drawPixmap(tmp, m_currentTitle, QRectF(0, 0, tmp.width(), tmp.height()));

		tmp = m_newRect.intersected(m_bounds);
		painter->setOpacity(m_newTitleOpacity);
		painter->drawPixmap(tmp, m_newTitle, QRectF(0, 0, tmp.width(), tmp.height()));

		painter->setOpacity(oldOpacity);
	}
}

void StatusBarTitle::setTitleString (std::string title, bool showTitleBorder)
{
	bool showBorder;
	if(!Settings::LunaSettings()->tabletUi) {
		showBorder = showTitleBorder;
	} else if (m_forceClassicUi) {
		showBorder = true;
	} else {
		showBorder = false;
	}

	QString newTitle = QString::fromUtf8(title.c_str());

	if(!m_originalText.compare(newTitle) && (showBorder == m_showTitleBorder))
		return; // no change

	m_showTitleBorder = showBorder;
	m_originalText = newTitle;

	QFontMetrics fontMetrics(*m_font);

	int maxTextWidth;
	if(m_showTitleBorder) {
		maxTextWidth = m_maxSize.width() - BKGD_LEFT_WIDTH - BKGD_RIGHT_WIDTH - BORDER_TITLE_LEFT_PADDING;
	} else {
		maxTextWidth = m_maxSize.width() - NO_BORDER_TITLE_LEFT_PADDING;
	}

	m_elidedText = fontMetrics.elidedText(QString::fromUtf8(title.c_str()), Qt::ElideRight, maxTextWidth);

	m_titleRect = fontMetrics.boundingRect(m_elidedText);

	createTitlePixmap(&m_newTitle, m_newRect);

	animateTitleTransition();
}

void StatusBarTitle::createTitlePixmap(QPixmap *pix, QRectF &rect)
{
	if(pix->isNull())
		return;

	QPainter painter;
	QFontMetrics fontMetrics(*m_font);

	int baseLine = pix->height()/2 + m_titleRect.height()/2 - fontMetrics.descent() + TEXT_BASELINE_OFFSET;

	pix->fill(Qt::transparent);

	painter.begin(pix);
	painter.setFont(*m_font);

	if(m_showTitleBorder) {
		painter.drawPixmap (0, 0, BKGD_LEFT_WIDTH, m_titleBackground.height(),
				             m_titleBackground,
				             0, 0, BKGD_LEFT_WIDTH, m_titleBackground.height());

		painter.drawPixmap (BKGD_LEFT_WIDTH, 0, m_titleRect.width(), m_titleBackground.height(),
				             m_titleBackground,
				             BKGD_LEFT_WIDTH, 0, m_titleBackground.width() - BKGD_LEFT_WIDTH - BKGD_RIGHT_WIDTH, m_titleBackground.height());

		painter.drawPixmap (BKGD_LEFT_WIDTH + m_titleRect.width(), 0, BKGD_RIGHT_WIDTH, m_titleBackground.height(),
				             m_titleBackground,
				             m_titleBackground.width() - BKGD_RIGHT_WIDTH, 0, BKGD_RIGHT_WIDTH, m_titleBackground.height());

		rect.setRect(0, -m_titleBackground.height()/2, BKGD_LEFT_WIDTH + m_titleRect.width() + BKGD_RIGHT_WIDTH, m_titleBackground.height());
	} else {
		rect.setRect(0, -m_titleBackground.height()/2, NO_BORDER_TITLE_LEFT_PADDING + m_titleRect.width(), m_titleBackground.height());
	}

	// paint the text
	painter.setPen(QColor(0xFF, 0xFF, 0xFF, 0xFF));

	if(!m_showTitleBorder) {
		painter.drawText(QPointF(NO_BORDER_TITLE_LEFT_PADDING, baseLine), m_elidedText);
	} else {
		painter.drawText(QPointF(BKGD_LEFT_WIDTH + BORDER_TITLE_LEFT_PADDING, baseLine), m_elidedText);
	}

	painter.end();
}

void StatusBarTitle::animateTitleTransition()
{
	if((m_anim.state() == VariantAnimation<StatusBarTitle>::Stopped) || !m_inTransition) {
		m_newTitleOpacity = 0.0;

		m_anim.setStartValue(0);
		m_anim.setEndValue(100);
		m_anim.setEasingCurve(AS_CURVE(statusBarTitleChangeCurve));
		m_anim.setDuration(AS(statusBarTitleChangeDuration));

		m_inTransition = true;

		if(!Settings::LunaSettings()->tabletUi) {
			m_bounds = m_curRect.united(m_newRect);
		} else {
			m_bounds = m_curRect;
		}

		m_anim.start();
	}
}

void StatusBarTitle::animValueChanged(const QVariant& value)
{
	if(m_inTransition)
	{
		m_newTitleOpacity = (qreal)((qreal)(value.toInt())/ 100.0);

		if(Settings::LunaSettings()->tabletUi) {
			int width = (int)(((1.0 - m_newTitleOpacity) * (qreal)(m_curRect.width())) + (m_newTitleOpacity * (qreal)(m_newRect.width())));
			m_bounds.setRect(m_curRect.x(), m_curRect.y(), width, m_curRect.height());
		}

		update();
		Q_EMIT signalBoundingRectChanged();
	}
}

void StatusBarTitle::slotAnimFinished()
{
	if(m_inTransition)
		m_currentTitle = m_newTitle;
	m_inTransition = false;
	m_curRect = m_newRect;
	m_bounds = m_curRect;
	Q_EMIT signalBoundingRectChanged();
}

