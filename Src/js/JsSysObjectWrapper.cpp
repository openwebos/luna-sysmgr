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

#include "JsSysObject.h"
#include "JsSysObjectWrapper.h"
#include "WebFrame.h"

static NPObject*
PrvObjAllocate(NPP npp, NPClass* klass)
{
	return new JsSysObject(npp);
}

static void
PrvObjDeallocate(NPObject* obj)
{
	delete static_cast<JsSysObject*>(obj);	
}

static void
PrvObjInvalidate(NPObject* obj)
{
	static_cast<JsSysObject*>(obj)->invalidate();
}

static bool
PrvObjHasMethod(NPObject* obj, NPIdentifier name)
{
	return static_cast<JsSysObject*>(obj)->hasMethod(name);
}

static bool
PrvObjInvoke(NPObject *obj, NPIdentifier name,
			 const NPVariant *args, uint32_t argCount,
			 NPVariant *result)
{
	return static_cast<JsSysObject*>(obj)->invoke(name, args, argCount, result);
}

static bool
PrvObjInvokeDefault(NPObject *obj, const NPVariant *args,
					uint32_t argCount, NPVariant *result)
{
	return static_cast<JsSysObject*>(obj)->invokeDefault(args, argCount, result);
}

static bool
PrvObjHasProperty(NPObject *obj, NPIdentifier name)
{
	return static_cast<JsSysObject*>(obj)->hasProperty(name);
}

static bool
PrvObjGetProperty(NPObject *obj, NPIdentifier name,
				  NPVariant *result)
{
	return static_cast<JsSysObject*>(obj)->getProperty(name, result);
}

static bool
PrvObjSetProperty(NPObject *obj, NPIdentifier name,
				  const NPVariant *value)
{
	return static_cast<JsSysObject*>(obj)->setProperty(name, value);
}

static bool
PrvObjRemoveProperty(NPObject *obj, NPIdentifier name)
{
	return static_cast<JsSysObject*>(obj)->removeProperty(name);
}

static bool
PrvObjEnumerate(NPObject *obj, NPIdentifier **value,
				uint32_t *count)
{
	return static_cast<JsSysObject*>(obj)->enumerate(value, count);
}

static bool
PrvObjConstruct(NPObject *obj, const NPVariant *args,
				uint32_t argCount, NPVariant *result)
{
	return static_cast<JsSysObject*>(obj)->construct(args, argCount, result);
}
			   

static NPClass sObjClass = {
	NP_CLASS_STRUCT_VERSION_CTOR,
	PrvObjAllocate,
	PrvObjDeallocate,
	PrvObjInvalidate,
	PrvObjHasMethod,
	PrvObjInvoke,
	PrvObjInvokeDefault,
	PrvObjHasProperty,
	PrvObjGetProperty,
	PrvObjSetProperty,
	PrvObjRemoveProperty,
	PrvObjEnumerate,
	PrvObjConstruct
};


JsSysObject* acquireFrameJsSysObject(WebFrame* frame)
{
	NPNetscapeFuncs* browserFuncs = 0;	
	NPObject*        obj          = 0;

	frame->webkitFrame()->createJsObject(&sObjClass, "PalmSystem", (void**) &obj, (void**) &browserFuncs);	
	
	if (!obj)
		return 0;

	JsSysObject* sysObj = static_cast<JsSysObject*>(obj);

	sysObj->setBrowserFuncs(browserFuncs);
	sysObj->setFrame(frame);

	return sysObj;
}
