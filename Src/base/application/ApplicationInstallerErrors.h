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




#ifndef APPLICATIONINSTALLERERRORS_H
#define APPLICATIONINSTALLERERRORS_H

#include "Common.h"

enum {
	AI_ERR_NONE  = 0,
	AI_ERR_INTERNAL,
	AI_ERR_INVALID_ARGS,
	AI_ERR_INSTALL_FAILEDUNPACK,
	AI_ERR_INSTALL_NOTENOUGHTEMPSPACE,
	AI_ERR_INSTALL_NOTENOUGHINSTALLSPACE,
	AI_ERR_INSTALL_TARGETNOTFOUND,
	AI_ERR_INSTALL_BADPACKAGE,
	AI_ERR_INSTALL_FAILEDVERIFY,
	AI_ERR_INSTALL_FAILEDIPKGINST,
	AI_ERR_REMOVE_FAILEDIPKGREMOVE,
	AI_ERR_LAST = 1 << 31
};

#endif /* APPLICATIONINSTALLERERRORS_H */
