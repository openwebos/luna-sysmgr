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




#ifndef WEBAPPCACHE_H
#define WEBAPPCACHE_H

#include "Common.h"

class WebAppBase;

class WebAppCache
{
public:

	static void put(WebAppBase* app);
	static void remove(WebAppBase* app);
	static void flush();
};

#endif /* WEBAPPCACHE_H */
