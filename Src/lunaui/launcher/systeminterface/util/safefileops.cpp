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




#include "safefileops.h"
#include <QFile>
#include <QFileInfo>
#include <glib.h>
#include <stdio.h>
#include <cerrno>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <QDebug>

namespace DimensionsSystemInterface
{

static int localRename(const char* fileA, const char* fileB)
{
	int ret = rename(fileA, fileB);
	if (ret) {
		g_debug("Failed to rename: %s to %s", fileA, fileB);
	}
	else {
		g_debug("Successfully rename: %s to %s", fileA, fileB);
	}
	return ret;
}

static bool fileCopy(const QString& source,const QString& copied)
{
	QFile srcFile(source);
	bool rc = srcFile.copy(copied);
	srcFile.close();
	return rc;
}

static int makeTempFile(const QString& base,QString& r_filename)
{
	gchar * tempname = 0;
	int fh = g_file_open_tmp((base+QString("_tmpXXXXXX")).toAscii().constData(),&tempname,0);
	if (fh != -1)
	{
		r_filename = QString((const char *)tempname);
	}
	return fh;
}

static QString makeUniqueTempFilename(const QString& base)
{
	QString tmpDir = QString((const char *)g_get_tmp_dir()) + QString("/");
	QString tempName = tmpDir + base+QString(".%1").arg( rand()) + QString(".qsettmp");
	while (QFile::exists(tempName))
	{
		tempName = tmpDir + base+QString(".%1%2").arg( time(0)).arg( rand()) + QString(".qsettmp");
	}
	return tempName;
}

SafeFileOperator::SafeFileOperator(OpType op,const QString& fileName,QSettings::Format format, QObject *parent)
: m_opMode(op)
, m_tmpFh(-1)
, m_unsafe(false)
, m_p_settings(0)
{
	QFileInfo targetFile(fileName);
	m_actualName = targetFile.absoluteFilePath();
	QString baseFilename = targetFile.fileName();
	m_originalPath = targetFile.absolutePath() + QString("/");

	if (m_opMode == Read)
	{
		//copy fileName to a temp file (in a temp dir) and open that as settings; this is necessary because even if only reading,
		//Qt will "volatile-ize" the qsettings file and could potentially overwrite it
		m_tempName = makeUniqueTempFilename(baseFilename);
		//make sure this file is truly gone, or else the copy below will fail
		 unlink(m_tempName.toAscii().constData());
		if (!(fileCopy(fileName,m_tempName)))
		{
			//failed to copy! Fall back to unsafe mode, and use the original file
			g_warning("%s: copy operation for %s mode access to [%s] (target for copy was [%s]) --> FAILED. Falling back into unsafe mode",
										__FUNCTION__,
										((m_opMode == Write) ? "WRITE" : "READ"),
										qPrintable(fileName),
										qPrintable(m_tempName));
			m_unsafe = true;
			m_tempName = fileName;
		}
		else
		{
			//open the temp file, so i can have a little control over it
			m_tmpFh =  open(m_tempName.toAscii().constData(), O_RDONLY | O_RSYNC,S_IRWXU);
			m_unsafe = false;
			if (m_tmpFh == -1)
			{
				//still can't create a temp file!
				//fallback to unsafe mode
				g_warning("%s: temp file open operation for %s mode access to [%s] (target for open was [%s]) --> FAILED. Falling back into unsafe mode",
						__FUNCTION__,
						((m_opMode == Write) ? "WRITE" : "READ"),
						qPrintable(fileName),
						qPrintable(m_tempName));
				m_unsafe = true;
				m_tempName = fileName;
			}
		}
	}
	else if (m_opMode == Write)
	{
		m_tmpFh = makeTempFile(baseFilename,m_tempName);
		if (m_tmpFh != -1)
		{
			g_debug("%s: Using auto-created temp file [%s]",__FUNCTION__,qPrintable(m_tempName));
		}
		else
		{
			m_tempName = makeUniqueTempFilename(baseFilename);
			g_debug("%s: Failed to auto-create temp file with makeTempFile()...falling back to manually created temp file [%s]",
					__FUNCTION__,
					qPrintable(m_tempName));
			m_tmpFh =  open(m_tempName.toAscii().constData(), O_WRONLY | O_DSYNC | O_SYNC | O_CREAT | O_TRUNC,S_IRWXU);
			if (m_tmpFh == -1)
			{
				//still can't create a temp file!
				//fallback to unsafe mode
				g_warning("%s: temp file open operation for %s mode access to [%s] (target for open was [%s]) --> FAILED. Falling back into unsafe mode",
						__FUNCTION__,
						((m_opMode == Write) ? "WRITE" : "READ"),
						qPrintable(fileName),
						qPrintable(m_tempName));
				m_unsafe = true;
				m_tempName = fileName;
			}
		}
	}
	else
	{
		g_warning("%s: Unimplemented file op mode: %d",__FUNCTION__,(int)m_opMode);
		m_tempName = QString();
		m_unsafe = true;
	}

	m_tempBasename = QFileInfo(m_tempName).fileName();

	g_debug("%s: Initializing temp qsettings with file [%s] for %s mode access to [%s]",
			__FUNCTION__,qPrintable(m_tempName),
			((m_opMode == Write) ? "WRITE" : "READ"),
			qPrintable(fileName)
			);
	m_p_settings = new QSettings(m_tempName,QSettings::IniFormat);
	if (m_p_settings->status() != QSettings::NoError)
	{
		g_warning("%s: Initializing temp qsettings with file [%s] for %s mode access to [%s] --> FAILED",
				__FUNCTION__,qPrintable(m_tempName),qPrintable(fileName),
				((m_opMode == Write) ? "WRITE" : "READ")
		);
	}
	else
	{
		m_p_settings->setValue("nul",0);
	}
}

//virtual
SafeFileOperator::~SafeFileOperator()
{
	delete m_p_settings;

	//if unsafe mode was NOT used, then close the temp file handle
	if ((!m_unsafe) && (m_tmpFh != -1))
	{
		 close(m_tmpFh);
	}

	if (!m_unsafe)
	{
		//if the op was a write, them rename the tmp file back to original
		if (m_opMode == Write)
		{

			//copy the temp file, because it's probably not on the same filesys (so rename will fail)
			// (these files are small so it should be no big perf issue)
			QString targetCopy = m_originalPath + m_tempBasename;
			//first make sure an old stale temp of this name doesn't exist - delete it.
			 unlink(targetCopy.toAscii().constData());
			bool rc = fileCopy(m_tempName,targetCopy);
			if (!rc)
			{
			g_warning("%s: copy-to-rename [%s] -> [%s]... %s",__FUNCTION__,qPrintable(m_tempName),qPrintable(targetCopy),
					( (rc) ? "SUCCESS" : "FAIL")
			);
			}
			int v = localRename(targetCopy.toAscii().constData(),m_actualName.toAscii().constData());
			if (v != 0)
			{
			g_warning("%s: trying to rename [%s] -> [%s]... %s (%d , errno = %d)",__FUNCTION__,qPrintable(targetCopy),qPrintable(m_actualName),
								( (v == 0) ? "SUCCESS" : "FAIL"),v,
								( (v != 0 ? errno : 0))
						);
			}

			if (v)
			{
				//make sure the temp-copy goes away in the case of an error
				int rc =  unlink(targetCopy.toAscii().constData());
				if (rc != 0)
				{
				g_warning("%s: trying to delete temp-copy file [%s]... %s",__FUNCTION__,qPrintable(targetCopy),
									( ( rc == 0) ? "SUCCESS" : "FAIL")
							);
				}
			}
		}

		//nuke the temp...(it will still exist in some cases; in others, it will have been renamed)
		if (QFile::exists(m_tempName.toAscii().constData()))
		{
			int rc = unlink(m_tempName.toAscii().constData());
			if (rc != 0)
			{
			g_warning("%s: trying to delete temp file [%s]... %s",__FUNCTION__,qPrintable(m_tempName),
					( (rc == 0) ? "SUCCESS" : "FAIL")
			);
			}
		}
	}
}

QSettings& SafeFileOperator::safeSettings()
{
	return *m_p_settings;
}

}	//end namespace
