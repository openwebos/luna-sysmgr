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



#ifndef CANDIDATEBAR_H
#define CANDIDATEBAR_H

#include "IMEDataInterface.h"

#include "qrect.h"
#include "qstringlist.h"
#include "qfont.h"

#include "PalmIMEHelpers.h"

class CandidateBar : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int scrollOffset READ scrollOffset WRITE setScrollOffset)

public:

	CandidateBar(Mapper_IF& mapper, IMEDataInterface * dataInterface);

			bool	enabled() const						{ return m_enabled; }
	virtual void	setEnabled(bool enable) = 0;

			bool	isTraceActive() const;
			void	setEditorState(const PalmIME::EditorState & editorState);

			// where the scrollbar is. Can be set that way, but you're responsible for updates...
			QRect &	frame()								{ return m_frame; }

			int		scrollOffset() const				{ return m_scrollOffset; }
			void	setScrollOffset(int offset);

			void	setSpacing(int spacing);
			QFont &	font()								{ return m_font; }
			const QString &	autoSelectCandidate() const	{ return m_bestCandidate; }

			void	paint(QPainter & painter, const QColor & color);
	virtual bool	keyboardTap(QPointF where, Qt::Key car) = 0;	// returns true if the touch was "used" and no key should be sent
			void	releaseTouch(int move);
			void	tapEvent(const QPoint & tapPt);
	virtual bool	backspace(bool shiftDown) = 0;			// returns true if backspace was "used"

	virtual bool	loadKeyboardLayoutFile(const char * fullPath, quint16 primaryID, quint16 secondaryID) = 0;
	virtual void	setLanguage(const std::string & languageName) = 0;
	virtual void	updateKeyboardLayout(const char * layoutName, quint32 page, const QRect & keyboardFrame, bool shiftActive, bool capsLockActive, bool autoCapActive) = 0;
	virtual void	updateSuggestions(bool trace) = 0;
	virtual void	processTrace() = 0;

			void	commit(const QString & text = QString());
	virtual void	clearCandidates();

			bool	tracePoint(QPoint point, Qt::Key key, int fingerID, bool newPoint);
			bool	endTrace(int fingerID);
			void	paintTrace(QPainter & painter, int top, const QColor & color, qreal width);

	virtual void	drawXT9Regions(QPixmap * pixmap, int topPadding) = 0;

Q_SIGNALS:
	void        needsRedraw();
	void		resized();

public Q_SLOTS:

public:

    struct TracePoint {

        TracePoint(quint32 x, quint32 y) : m_x(x), m_y(y) {}

        // don't change memory layout: needs to be compatible with ET9TracePoint
        quint32 m_x;
        quint32 m_y;
    };

	struct TapRecord {
		TapRecord(quint32 x, quint32 y, Qt::Key car, bool shift) : m_p(x, y), m_char(car), m_shift(shift) {}

		TracePoint	m_p;
		Qt::Key		m_char;
		bool		m_shift;
	};

protected:

	void	sendKeyDownUp(Qt::Key key, Qt::KeyboardModifiers modifiers);

	Mapper_IF &					m_mapper;
	IMEDataInterface *			m_IMEDataInterface;

	bool		m_enabled;
	bool		m_traceAllowed;
	QRect		m_frame;
	int			m_scrollOffset;
	QStringList	m_candidates;
	QString		m_bestCandidate;
	QFont		m_font;
	int			m_spacing;

	std::vector<TapRecord>	m_taps;
	std::vector<TracePoint>	m_tracePoints;
	int			m_traceFingerID;
	Qt::Key		m_firstKey;
	bool		m_traceEntry;	// last entry was made using trace: backspace deletes all
};

#include "CandidateBarRemote.h"
#define CANDIDATEBAR CandidateBarRemote

#endif // CANDIDATEBAR_H
