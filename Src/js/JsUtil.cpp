/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "JsUtil.h"

#include <string.h>
#include <stdio.h>


std::string JsonEscapeString(const char *inStr )
{
	const char *kHex = "0123456789abcdef";	
	std::string result;
  int pos = 0, start_offset = 0;
  unsigned char c;
  char buffer[12];
  do {
    c = inStr[pos];
    switch(c) {
    case '\0':
      break;
    case '\b':
    case '\n':
    case '\r':
    case '\t':
    case '"':
    case '\\':
    case '/':
      if(pos - start_offset > 0)
		  result += std::string( inStr + start_offset, pos - start_offset);
      if(c == '\b') result += ("\\b");
      else if(c == '\n') result += ("\\n" );
      else if(c == '\r') result += ("\\r");
      else if(c == '\t') result += ("\\t");
      else if(c == '"') result += ("\\\"");
      else if(c == '\\') result += ("\\\\");
      else if(c == '/') result += ("\\/");
      start_offset = ++pos;
      break;
    default:
      if(c < ' ') {
	if(pos - start_offset > 0)
		result += std::string( inStr + start_offset, pos - start_offset);
	sprintf(buffer, "\\u00%c%c",
		  kHex[c >> 4],
		  kHex[c & 0xf]);
	result += buffer;
	start_offset = ++pos;
      } else pos++;
    }
  } while(c);
  if(pos - start_offset > 0)
	  result += std::string( inStr + start_offset, pos - start_offset);
  return result;
}
