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




#include "Common.h"

#include <errno.h>
#include <glib.h>
#include <string.h>

#if defined(HAS_AFFINITY)
#include <libaffinity.h>
#endif

#include "CpuAffinity.h"

void setCpuAffinity(int pid, int processor)
{
#if defined(HAS_AFFINITY)
	int ret = libaffinity_set_affinity(pid, processor);
	if (ret)
		g_warning("Failed to set cpu affinity for process %d to processor %d: retVal: %d, errno: %s",
				  pid, processor, ret, strerror(errno));
	else
		g_message("Successfully set cpu affinity for process %d to processor %d",
				  pid, processor);
#endif	
}

void resetCpuAffinity(int pid)
{
#if defined(HAS_AFFINITY)
    int ret = libaffinity_reset_affinity(pid);
	if (ret)
		g_warning("Failed to reset cpu affinity for process %d: retVal: %d, errno: %s",
				  pid, ret, strerror(errno));
	else
		g_message("Successfully reset cpu affinity for process %d", pid);
#endif    
}
