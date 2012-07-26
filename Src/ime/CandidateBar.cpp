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



#include "CandidateBar.h"
#include "PalmIMEHelpers.h"

#include "Utils.h"
#include "qpropertyanimation.h"

CandidateBar::CandidateBar(Mapper_IF& mapper, IMEDataInterface * dataInterface) : QObject(NULL),
	m_mapper(mapper), m_IMEDataInterface(dataInterface), m_enabled(false), m_traceAllowed(false), m_scrollOffset(0), m_font("Prelude", 24), m_spacing(15), m_traceEntry(false)
{
}

void CandidateBar::setScrollOffset(int offset)
{
	if (m_scrollOffset != offset)
	{
		m_scrollOffset = offset;
		Q_EMIT needsRedraw();
	}
}

void CandidateBar::setSpacing(int spacing)
{
	if (m_spacing != spacing)
	{
		m_spacing = spacing;
		Q_EMIT needsRedraw();
	}
}

void CandidateBar::commit(const QString & text)
{
	if (m_enabled)
	{
		if (text.size() > 0)
			m_IMEDataInterface->setComposingText(text.toUtf8().data());
		m_IMEDataInterface->commitComposingText();
		m_taps.clear();
		m_firstKey = Qt::Key(0);
	}
	clearCandidates();
}

bool CandidateBar::isTraceActive() const
{
	return m_enabled && m_traceAllowed;
}

void CandidateBar::setEditorState(const PalmIME::EditorState & editorState)
{
	clearCandidates();
	m_traceAllowed = (editorState.type == PalmIME::FieldType_Text);
}

void CandidateBar::clearCandidates()
{
	m_candidates.clear();
	m_scrollOffset = 0;
	m_bestCandidate.clear();
}

void CandidateBar::paint(QPainter & painter, const QColor & color)
{
	if (m_enabled && m_frame.height() > 0)
	{
		QRect	r(m_frame);
		painter.fillRect(r, QColor(0, 0, 0));
		int adjust = 2;
		r.adjust(adjust, adjust, -adjust, -adjust);
		painter.setFont(m_font);
		int	offset = m_spacing / 2 + m_scrollOffset;
		QFontMetrics	fontMetrics(m_font);
		int index = 0;
		for (QStringList::iterator iter = m_candidates.begin(); iter != m_candidates.end(); ++iter, ++index)
		{
			QString	str = *iter;
			r.moveLeft(offset);
			if (str == m_bestCandidate)
				painter.setPen(color);
			else
				painter.setPen(QColor(255, 255, 255));
			painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, str);
			offset += fontMetrics.width(str) + m_spacing;
			if (offset > m_frame.width())
				break;
		}
		QPen	pen(QColor(155, 155, 155));
		pen.setWidth(3);
		painter.setPen(pen);
		r = m_frame;
		r.adjust(adjust - 1, adjust, -adjust+1, -adjust);
		painter.drawRect(r);
	}
}

void CandidateBar::paintTrace(QPainter & painter, int top, const QColor & color, qreal width)
{
	if (isTraceActive())
	{
		painter.setPen(QPen(QBrush(color), width, Qt::SolidLine, Qt::RoundCap));
		const size_t cChunkSize = 100;
		QPointF	points[cChunkSize + 1];
		size_t	index = 0;
		while (index < m_tracePoints.size())
		{
			size_t chunk;
			for (chunk = 0; chunk < cChunkSize + 1 && index + chunk < m_tracePoints.size(); chunk++)
			{
				points[chunk].setX(int(m_tracePoints[index + chunk].m_x));			// cast to int, because nX & nY are unsigned & might have wrapped around
				points[chunk].setY(int(m_tracePoints[index + chunk].m_y) + top);
			}
			painter.drawPolyline(points, chunk);
			index += qMin<size_t>(chunk, cChunkSize);
		}
	}
}

void CandidateBar::releaseTouch(int move)
{
	int newOffset = m_scrollOffset;
	if (move != 0)
	{
		bool alignLeft = move > 0;
		int align = alignLeft ? 0 : m_frame.right();	// which side to align to
		int pos = m_scrollOffset + m_spacing / 2;
		if (pos < align)
		{
			bool reached = false;
			Q_FOREACH(QString str, m_candidates)
			{
				int dim = QFontMetrics(m_font).width(str);
				if (pos + dim + m_spacing >= align)
				{
					if (alignLeft)
						newOffset += align - pos + m_spacing / 2;
					else
						newOffset += align - pos - dim - m_spacing / 2;
					reached = true;
					break;
				}
				pos += dim + m_spacing;
			}
			if (!reached)
				newOffset += align - pos + m_spacing / 2;
		}
	}
	if (newOffset > 0)
		newOffset = 0;
	if (newOffset != m_scrollOffset)
	{
		QPropertyAnimation * animation = new QPropertyAnimation(this, "scrollOffset");
		animation->setEasingCurve(QEasingCurve::OutCubic);
		animation->setDuration(500);
		animation->setEndValue(newOffset);
		animation->start(QAbstractAnimation::DeleteWhenStopped);
	}
}

inline int sqr(int a)	{ return a * a; }

inline int sqr_distance(const CandidateBar::TracePoint & a, const CandidateBar::TracePoint & b)
{
	return sqr(int(a.m_x) -  int(b.m_x)) + sqr(int(a.m_y) - int(b.m_y));
}

bool CandidateBar::tracePoint(QPoint point, Qt::Key key, int fingerID, bool newPoint)
{
	if (isTraceActive())
	{
		if (newPoint)
		{
			if (m_tracePoints.size() == 0)
			{
				//g_debug("Starting new trace for finger %d", fingerID);
				m_traceFingerID = fingerID;
				m_firstKey = key;
			}
		}
		else if (fingerID != m_traceFingerID || m_tracePoints.size() == 0)
			return false;	// we're not tracing, for a reason or an other

		m_tracePoints.push_back(TracePoint(point.x(), point.y()));
		if (m_firstKey && m_firstKey != key && sqr_distance(m_tracePoints.front(), m_tracePoints.back()) > sqr(35))
		{
			g_debug("Chosing this is a trace, not a tap... %d", sqr_distance(m_tracePoints.front(), m_tracePoints.back()));
			m_firstKey = Qt::Key(0);
			if (m_bestCandidate.length())
				commit(m_bestCandidate + ' ');
		}
		//g_debug("Tracing finger %d, point # %u, key: %s, first key: %s", fingerID, m_tracePoints.size(), QString(key).toUtf8().data(), QString(m_firstKey).toUtf8().data());

		Q_EMIT needsRedraw();

		return !m_firstKey;
	}
	return false;
}

bool CandidateBar::endTrace(int fingerID)
{
    if (isTraceActive() && m_tracePoints.size() > 0 && fingerID == m_traceFingerID)
    {
        if (m_firstKey) // the user stayed on a single key: this is a tap, not a trace...
        {
            m_tracePoints.clear();
            if (m_traceEntry && m_bestCandidate.length())  // if we have a trace entry pending, validate it
                commit(m_bestCandidate + ' ');
            return false;
        }

        processTrace();

        m_tracePoints.clear();

		return true;
	}
	return false;
}

void CandidateBar::tapEvent(const QPoint & tapPt)
{
	if (m_frame.contains(tapPt))
	{
		int x = tapPt.x();
		int	offset = m_scrollOffset;
		int index = 0;
		for (QStringList::iterator iter = m_candidates.begin(); iter != m_candidates.end(); ++iter, ++index)
		{
			QString	str = *iter;
			offset += QFontMetrics(m_font).width(str) + m_spacing;
			if (x < offset)
			{
				commit(str + ' ');
				Q_EMIT needsRedraw();
				break;
			}
		}
	}
}

void CandidateBar::sendKeyDownUp(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
	if (m_IMEDataInterface) {
		m_IMEDataInterface->sendKeyEvent(QEvent::KeyPress, key, modifiers);
		m_IMEDataInterface->sendKeyEvent(QEvent::KeyRelease, key, modifiers);
	}
}
