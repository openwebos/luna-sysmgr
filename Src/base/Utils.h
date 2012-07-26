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




#ifndef UTILS_H
#define UTILS_H

#include "Common.h"

#include <time.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <regex.h>
#include <QByteArray>

#include <QUrl>
#include <QGraphicsItem>
#include <glib.h>

void setPosTopLeft(QGraphicsItem* item, int x, int y);

char* readFile(const char* filePath);
bool  writeFile(const std::string& filePath, const std::string& buffer);
bool  writeFile(const std::string& filePath, QByteArray buffer);
bool  writeToTempFile(const std::string& data,const std::string& tempDir,std::string& r_outputFile);

bool  deleteFile(const char* filePath);
time_t localTime();
unsigned long currentTimeMs( );

template <class T>
std::string toSTLString(const T &arg) {
	std::ostringstream	out;
	out << arg;
	return(out.str());
}

std::string getResourceNameFromUrl(const  QUrl& url);
std::string trimWhitespace(const std::string& s,const std::string& drop = "\r\n\t ");
bool getNthSubstring(unsigned int n,std::string& target, const std::string& str,const std::string& delims = " \t\n\r");
int splitFileAndPath(const std::string& srcPathAndFile,std::string& pathPart,std::string& filePart);
int splitFileAndExtension(const std::string& srcFileAndExt,std::string& filePart,std::string& extensionPart);
int splitStringOnKey(std::vector<std::string>& returnSplitSubstrings,const std::string& baseStr,const std::string& delims);
int remap_stdx_pipes(int readPipe, int writePipe);
bool isNonErrorProcExit(int ecode,int normalCode=0);

std::string windowIdentifierFromAppAndProcessId(const std::string& appId, const std::string& processId);
bool splitWindowIdentifierToAppAndProcessId(const std::string& id, std::string& appId, std::string& processId);

std::string getResourcePathFromString(const std::string& entry, const std::string& appId,
									  const std::string& systemResourceFolder);

std::string getHtml5DatabaseFolderNameForApp(const std::string& appId,std::string appFolderPath);

void threadCleanup();

bool extractFromJson(const std::string& jsonString,const std::string& key,std::string& r_value);
bool extractFromJson(struct json_object * root,const std::string& key,std::string& r_value);
bool extractFromJson(struct json_object * root,const std::string& key,int& r_value);
bool extractFromJson(struct json_object * root,const std::string& key,bool& r_value);
struct json_object * JsonGetObject(struct json_object * root,const std::string& key);

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

bool doesExistOnFilesystem(const char * pathAndFile);
int fileCopy(const char * srcFileAndPath,const char * dstFileAndPath);

gboolean compare_regex (const gchar * regex,const gchar * string);

int determineEnclosingDir(const std::string& fileNameAndPath,std::string& r_enclosingDir);

// Build an std::string using printf-style formatting
std::string string_printf(const char *format, ...) G_GNUC_PRINTF(1, 2);

// Append a printf-style string to an existing std::string
std::string & append_format(std::string & str, const char * format, ...) G_GNUC_PRINTF(2, 3);

#endif /* UTILS_H */
