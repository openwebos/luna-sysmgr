/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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




#include "Common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <cjson/json.h>
#include <cjson/json_util.h>
#include <algorithm>
#include <fcntl.h>

#include <QFile>
#include <QByteArray>
#include <QtDebug>

#include "ApplicationManager.h"
#include "ApplicationDescription.h"
#include "Preferences.h"
#include "Utils.h"

void setPosTopLeft(QGraphicsItem* item, int x, int y)
{
	// position item, @ x,y within the items' parent
	// coordinate system, treating the origin of the item
	// and the parent system at the top,left
	if (!item)
		return;

	QRectF pBounds, iBounds = item->boundingRect();
	QGraphicsItem* p = item->parentItem();
	if (p) {
		pBounds = p->boundingRect();
	}
	x = x + pBounds.x() - iBounds.x();
	y = y + pBounds.y() - iBounds.y();

	item->setPos(x, y);
}

char* readFile(const char* filePath)
{
	if (!filePath)
		return 0;

	FILE* f = fopen(filePath,"r");

	if (!f)
		return 0;

	fseek(f, 0L, SEEK_END);
	long sz = ftell(f);
	fseek( f, 0L, SEEK_SET );
	if (!sz) {
		fclose(f);
		return 0;
	}

	char* ptr = new char[sz+1];
	if( !ptr )
	{
		fclose(f);
		return 0;
	}
	ptr[sz] = 0;

	size_t result = fread(ptr, sz, 1, f);
	Q_UNUSED(result);

	fclose(f);

	return ptr;
}


bool writeFile(const std::string& filePath, const std::string& buffer)
{
	return writeFile(filePath, QByteArray::fromRawData(buffer.c_str(), buffer.size()));
}

bool writeFile(const std::string &filePath, QByteArray buffer)
{
	if (filePath.empty() || buffer.isEmpty())
		return false;

	std::string tmpPath = filePath + ".tmp";

	int tmpOutputFile = creat(tmpPath.c_str(), S_IRUSR | S_IWUSR);
	if (tmpOutputFile == -1) {
		return false;
	}

	ssize_t written = write(tmpOutputFile, buffer.constData(), buffer.size());
	if (written < 0 || written != buffer.size()) {
		unlink(tmpPath.c_str());
		close(tmpOutputFile);
		return false;
	}

	if (0 != fsync(tmpOutputFile))
		g_warning("Failed to sync contents of '%s' prior to rename", tmpPath.c_str());

	bool renamed = rename(tmpPath.c_str(), filePath.c_str()) == 0;
	close(tmpOutputFile);

	if (!renamed)
		unlink(tmpPath.c_str());
	return renamed;
}

bool writeToTempFile(const std::string& data,const std::string& tempDir,std::string& r_outputFile) {
	const std::string fNameTemplate("fileXXXXXX");
	QByteArray templateFileName;
	templateFileName.reserve(tempDir.length() + fNameTemplate.length() + 2);
	templateFileName += QByteArray::fromRawData(tempDir.c_str(), tempDir.size());
	templateFileName += QByteArray::fromRawData(fNameTemplate.c_str(), fNameTemplate.size());

	int fd = mkstemp(templateFileName.data());
	r_outputFile = std::string(templateFileName.data(), templateFileName.size());
	if (fd == -1)
		return false;

	//ssize_t write(int fildes, const void *buf, size_t nbyte);
	ssize_t nwritten = 0;
	ssize_t totalsize = data.size();
	const char * ptr = data.data();
	while (nwritten < totalsize) {
		int r = write(fd,ptr+nwritten,totalsize-nwritten);
		if (r < 0) {
			//fatal error
			break;
		}
		nwritten += r;
	}
	close(fd);
	return (nwritten == totalsize);
}

#define RDBUFFSIZE		(1024*32)
bool  concatToTempFile(std::vector<std::string>& files,const std::string& tempDir,std::string& r_outputFile)
{
	std::string templateStr = tempDir + std::string("fileXXXXXX");
	char * templateFileName = new char[templateStr.length()+2];
	strcpy(templateFileName,templateStr.c_str());
	int fd = mkstemp(templateFileName);
	r_outputFile = std::string(templateFileName);
	delete[] templateFileName;
	if (fd == -1)
		return false;

	char rdbuffer[RDBUFFSIZE];
	bool rc=true;
	for (std::vector<std::string>::iterator it = files.begin();
		it != files.end();++it) {
		std::string& inpfilename = *it;

		FILE * infp = fopen(inpfilename.c_str(),"rb");
		if (infp == NULL)
			continue;			// silent on unable to read a particular file

		size_t nread = 0;
		while ((nread = fread (rdbuffer,1,RDBUFFSIZE,infp)) > 0) {
			//ssize_t write(int fildes, const void *buf, size_t nbyte);
			ssize_t nwritten = 0;
			ssize_t totalsize = nread;
			const char * ptr = rdbuffer;
			while (nwritten < totalsize) {
				int r = write(fd,ptr+nwritten,totalsize-nwritten);
				if (r < 0) {
					//fatal error
					rc = false;
					break;
				}
				nwritten += r;
			}
		}
		fclose(infp);
	}
	close(fd);
	return (rc);
}

bool deleteFile(const char* filePath)
{
	return filePath && unlink(filePath) == 0;
}

time_t localTime()
{
	time_t     timeSecs;
	struct tm* gmTimeBroken;

	time(&timeSecs);
	gmTimeBroken = gmtime(&timeSecs);
	timeSecs = mktime(gmTimeBroken);

	return timeSecs;
}

unsigned long currentTimeMs( )
{
	struct timeval aTimeval;
	struct timezone aTimezone;

	gettimeofday( &aTimeval, &aTimezone );
	return  ( aTimeval.tv_sec * 1000 ) + (unsigned long)(aTimeval.tv_usec / 1000 );
}

std::string getResourceNameFromUrl(const QUrl& url)
{
	QString resourceName;
	if (url.isValid()) {
		resourceName = url.path();
		int slashIndex = resourceName.lastIndexOf('/');
		if (slashIndex != -1)
			resourceName = resourceName.mid(slashIndex+1);
	}
	return resourceName.toStdString();
}

std::string trimWhitespace(const std::string& s,const std::string& drop)
{
 std::string::size_type first = s.find_first_not_of( drop );
 std::string::size_type last  = s.find_last_not_of( drop );

 if( first == std::string::npos || last == std::string::npos ) return std::string( "" );
   return s.substr( first, last - first + 1 );
}

bool getNthSubstring(unsigned int n,std::string& dest,const std::string& str,const std::string& delims) {
	if (n == 0)
		n=1;

	std::string::size_type start = 0;
	std::string::size_type mark = 0;
	unsigned int i=1;
	while (1) {
		//find the start of a non-delim
		start = str.find_first_not_of(delims,mark);
		if (start == std::string::npos)
			break;
		//find the end of the current substring (where the next instance of delim lives, or end of the string)
		mark = str.find_first_of(delims,start);
		if ((mark == std::string::npos) || (i == n))
			break;	//last substring, or Nth one found
		++i;
	}

	if (i != n)
		return false;

	//extract
	dest = str.substr(start,mark-start);
	return true;

}

int splitFileAndPath(const std::string& srcPathAndFile,std::string& pathPart,std::string& filePart) {

	std::vector<std::string> parts;
	printf("splitFileAndPath - input [%s]\n",srcPathAndFile.c_str());
	int s = splitStringOnKey(parts,srcPathAndFile,std::string("/"));
	if ((s == 1) && (srcPathAndFile.at(srcPathAndFile.length()-1) == '/')) {
		//only path part
		pathPart = srcPathAndFile;
		filePart = "";
	}
	else if (s == 1) {
		//only file part
		if (srcPathAndFile.at(0) == '/') {
			pathPart = "/";
		}
		else {
			pathPart = "";
		}
		filePart = parts.at(0);
	}
	else if (s >= 2) {
		for (int i=0;i<s-1;i++) {
			if ((parts.at(i)).size() == 0)
				continue;
			pathPart += std::string("/")+parts.at(i);
			printf("splitFileAndPath - path is now [%s]\n",pathPart.c_str());
		}
		pathPart += std::string("/");
		filePart = parts.at(s-1);
	}

	return s;
}

int splitFileAndExtension(const std::string& srcFileAndExt,std::string& filePart,std::string& extensionPart) {

	std::vector<std::string> parts;
		int s = splitStringOnKey(parts,srcFileAndExt,std::string("."));
		if (s == 1) {
			//only file part; no extension
			filePart = parts.at(0);
		}
		else if (s >= 2) {
			filePart += parts.at(0);
			for (int i=1;i<s-1;i++)
				filePart += std::string(".")+parts.at(i);
			extensionPart = parts.at(s-1);
		}
		return s;
}

int splitStringOnKey(std::vector<std::string>& returnSplitSubstrings,const std::string& baseStr,const std::string& delims) {

	std::string base = trimWhitespace(baseStr);
	std::string::size_type start = 0;
	std::string::size_type mark = 0;
	std::string extracted;

	int i=0;
	while (start < baseStr.size()) {
		//find the start of a non-delims
		start = baseStr.find_first_not_of(delims,mark);
		if (start == std::string::npos)
			break;
		//find the end of the current substring (where the next instance of delim lives, or end of the string)
		mark = baseStr.find_first_of(delims,start);
		if (mark == std::string::npos)
			mark = baseStr.size();

		extracted = baseStr.substr(start,mark-start);
		if (extracted.size() > 0) {
			//valid string...add it
			returnSplitSubstrings.push_back(extracted);
			++i;
		}
		start=mark;
	}

	return i;
}

#define	DUPFD(ofd, nfd)	(dup2(ofd, nfd) == 0  &&  close(ofd) == 0)
int remap_stdx_pipes(int readPipe, int writePipe) {

	if ((readPipe == 0)  &&  (writePipe == 1))
		return 1;

	if ( readPipe >= 1  &&  writePipe > 1 ) {
		return ((DUPFD(readPipe, 0)) && (DUPFD(writePipe, 1)));
	}

	if ((readPipe == 0)  &&  (writePipe >= 1))
		return DUPFD(writePipe, 1);

	if ((readPipe  >= 1)  &&  (writePipe == 1))
		return DUPFD(readPipe, 0);

	if ((readPipe >= 1)   &&  (writePipe == 0)) {
		return ( (DUPFD(writePipe, 1)) && (DUPFD(readPipe, 0)));
	}

	if ((readPipe == 1)  &&  (writePipe == 0)) {
	const int tmp = dup(writePipe);
	return	((tmp > 1) &&  (close(writePipe) == 0) &&  (DUPFD(readPipe, 0)) &&  (DUPFD(tmp,   1)));
	}

	return  0;
}

bool isNonErrorProcExit(int ecode,int normalCode) {

	if (!WIFEXITED(ecode))
		return false;
	if (WEXITSTATUS(ecode) != normalCode)
		return false;

	return true;
}

std::string windowIdentifierFromAppAndProcessId(const std::string& appId, const std::string& processId)
{
	return appId + " " + processId;
}

bool splitWindowIdentifierToAppAndProcessId(const std::string& id, std::string& appId, std::string& processId)
{
	if (id.empty())
		return false;

	gchar** strArray = g_strsplit(id.c_str(), " ", 2);
	if (!strArray)
		return false;

	int index = 0;
	while (strArray[index]) {

		if (index == 0)
			appId = strArray[0];
		else if (index == 1)
			processId = strArray[1];

		++index;
	}

	g_strfreev(strArray);

	return true;
}

std::string getResourcePathFromString(const std::string& entry, const std::string& appId,
									  const std::string& systemResourceFolder)
{
	if (entry.empty())
		return std::string();

	struct stat stBuf;

	// Absolute path?
	if (entry[0] == '/') {

		// check if it exists and is a regular file
		if (::stat(entry.c_str(), &stBuf) != 0 || !S_ISREG(stBuf.st_mode))
			return std::string();

		return entry;
	}

	// relative path. first check in the app folder
	ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(appId);
	if (appDesc) {

        // First check in the locale specific folder
		std::string filePath = appDesc->folderPath() + "/resources/" + Preferences::instance()->locale() + "/" + entry;
		if (::stat(filePath.c_str(), &stBuf) == 0 && (S_ISREG(stBuf.st_mode) || S_ISLNK(stBuf.st_mode)))
			return filePath;

        // Try in the standard app folder path
		filePath = appDesc->folderPath() + "/" + entry;
		if (::stat(filePath.c_str(), &stBuf) == 0 && (S_ISREG(stBuf.st_mode) || S_ISLNK(stBuf.st_mode)))
			return filePath;
	}

	// Look for it in the system folder
	std::string filePath = systemResourceFolder + "/" + entry;
	if (::stat(filePath.c_str(), &stBuf) == 0 && S_ISREG(stBuf.st_mode))
		return filePath;

	// ah well... we give up
	return std::string();
}

//Should follow conventions for virtualHost naming in webkit: WebCore/platform/KURL.cpp
//CAUTION: modifying this to return strange paths is potentially dangerous. See ApplicationInstaller.cpp "remove" case where this fn is used
std::string getHtml5DatabaseFolderNameForApp(const std::string& appId,std::string appFolderPath)
{
	if (appFolderPath.length() == 0)
		return std::string("");

	replace(appFolderPath.begin(),appFolderPath.end(),'/','.');
	std::string r = std::string("file_")+appFolderPath+std::string("_0");
	return r;
}

extern void _malloc_thread_cleanup(void) __attribute__ ((weak));
void threadCleanup()
{
	if (_malloc_thread_cleanup)
		_malloc_thread_cleanup();
}

bool extractFromJson(const std::string& jsonString,const std::string& key,std::string& r_value)
{
	if ((jsonString.length() == 0) || (key.length() == 0))
		return false;

	struct json_object* root = json_tokener_parse(jsonString.c_str());
	struct json_object* label = 0;

	if ((!root) || (is_error(root)))
		return false;

	label = json_object_object_get(root,key.c_str());
	if ((!label) || (is_error(label))) {
		json_object_put(root);
		return false;
	}

	r_value = json_object_get_string(label);
	json_object_put(root);
	return true;
}

bool extractFromJson(struct json_object * root,const std::string& key,std::string& r_value)
{
	if ((!root) || (is_error(root)) || (key.length() == 0))
		return false;

	struct json_object* label = 0;

	label = json_object_object_get(root,key.c_str());
	if ((!label) || (is_error(label))) {
		return false;
	}

	r_value = json_object_get_string(label);
	return true;
}


bool extractFromJson(struct json_object * root,const std::string& key,int& r_value)
{
	if ((!root) || (is_error(root)) || (key.length() == 0))
		return false;

	struct json_object* label = 0;

	label = json_object_object_get(root,key.c_str());
	if ((!label) || (is_error(label))) {
		return false;
	}

	r_value = json_object_get_int(label);
	return true;
}


bool extractFromJson(struct json_object * root,const std::string& key,bool& r_value)
{
	if ((!root) || (is_error(root)) || (key.length() == 0))
		return false;

	struct json_object* label = 0;

	label = json_object_object_get(root,key.c_str());
	if ((!label) || (is_error(label))) {
		return false;
	}

	r_value = json_object_get_boolean(label);
	return true;
}


struct json_object * JsonGetObject(struct json_object * root,const std::string& key)
{
	if ((!root) || (is_error(root)) || (key.length() == 0))
		return NULL;

	struct json_object* label = 0;

	label = json_object_object_get(root,key.c_str());
	if ((!label) || (is_error(label))) {
		return NULL;
	}

	return label;
}

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
	QByteArray decoded = QByteArray::fromRawData((const char *)bytes_to_encode, in_len);
	QByteArray encoded = decoded.toBase64();
	return std::string(encoded.data(), encoded.size());
}

std::string base64_decode(std::string const& encoded)
{
	QByteArray encodedData = QByteArray::fromRawData(encoded.c_str(), encoded.size());
	QByteArray decodedData = QByteArray::fromBase64(encodedData);
	return std::string(decodedData.data(), decodedData.size());
}

bool doesExistOnFilesystem(const char * pathAndFile) {
	struct stat buf;
	return pathAndFile != NULL && 0 == ::stat(pathAndFile, &buf);
}

int fileCopy(const char * srcFileAndPath,const char * dstFileAndPath)
{
	if ((srcFileAndPath == NULL) || (dstFileAndPath == NULL))
		return -1;

	if (QFile::copy(QString(srcFileAndPath), QString(dstFileAndPath)))
		return 1;
	return -1;
}

gboolean compare_regex (const gchar * regex,
		const gchar * string)
{
	regex_t regexec_pattern;

	if (regex == NULL) {
		return TRUE;
	}

	if (regcomp (&regexec_pattern, regex, REG_NOSUB | REG_ICASE | REG_NEWLINE) != 0)
		return FALSE;
	int rc;
	rc = regexec (&regexec_pattern, string, 0, 0, 0);
	regfree(&regexec_pattern);
	if (rc != REG_NOMATCH)
		return TRUE;
	return FALSE;
}

int determineEnclosingDir(const std::string& fileNameAndPath,std::string& r_enclosingDir)
{
	if (fileNameAndPath.empty())
		return 0;
	if (fileNameAndPath[fileNameAndPath.size()-1] == '/')
	{
		r_enclosingDir = fileNameAndPath.substr(0,fileNameAndPath.size()-1);
	}
	else
	{
		std::string::size_type pos;
		if ((pos = fileNameAndPath.rfind("/")) == std::string::npos)
			r_enclosingDir = "/";
		else
			r_enclosingDir = fileNameAndPath.substr(0,pos-1);
	}
	if (r_enclosingDir[0] != '/')
		r_enclosingDir.insert(0,"/");
	return 1;
}

std::string string_printf(const char *format, ...)
{
	if (format == 0)
		return "";
	va_list args;
	va_start(args, format);
	char stackBuffer[1024];
	int result = vsnprintf(stackBuffer, G_N_ELEMENTS(stackBuffer), format, args);
	if (result > -1 && result < (int) G_N_ELEMENTS(stackBuffer))
	{	// stack buffer was sufficiently large. Common case with no temporary dynamic buffer.
		va_end(args);
		return std::string(stackBuffer, result);
	}

	int length = result > -1 ? result + 1 : G_N_ELEMENTS(stackBuffer) * 3;
	char * buffer = 0;
	do
	{
		if (buffer)
		{
			delete[] buffer;
			length *= 3;
		}
		buffer = new char[length];
		result = vsnprintf(buffer, length, format, args);
	} while (result == -1 && result < length);
	va_end(args);
	std::string	str(buffer, result);
	delete[] buffer;
	return str;
}

std::string & append_format(std::string & str, const char * format, ...)
{
	if (format == 0)
		return str;
	va_list args;
	va_start(args, format);
	char stackBuffer[1024];
	int result = vsnprintf(stackBuffer, G_N_ELEMENTS(stackBuffer), format, args);
	if (result > -1 && result < (int) G_N_ELEMENTS(stackBuffer))
	{	// stack buffer was sufficiently large. Common case with no temporary dynamic buffer.
		va_end(args);
		str.append(stackBuffer, result);
		return str;
	}

	int length = result > -1 ? result + 1 : G_N_ELEMENTS(stackBuffer) * 3;
	char * buffer = 0;
	do
	{
		if (buffer)
		{
			delete[] buffer;
			length *= 3;
		}
		buffer = new char[length];
		result = vsnprintf(buffer, length, format, args);
	} while (result == -1 && result < length);
	va_end(args);
	str.append(buffer, result);
	delete[] buffer;
	return str;
}
