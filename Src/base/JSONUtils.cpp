/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#include "JSONUtils.h"

#include "Logging.h"
#include "Utils.h"

bool JsonMessageParser::parse(const char * callerFunction)
{
	if (!mParser.parse(mJson, mSchema))
	{
		const char * errorText = "Could not validate json message against schema";
		pbnjson::JSchemaFragment	genericSchema(SCHEMA_ANY);
		if (!mParser.parse(mJson, genericSchema))
			errorText = "Invalid json message";
		g_critical("%s: %s '%s'", callerFunction, errorText, mJson);
		return false;
	}
	return true;
}

pbnjson::JValue createJsonReply(bool returnValue, int errorCode, const char *errorText)
{
	pbnjson::JValue reply = pbnjson::Object();
	reply.put("returnValue", returnValue);
	if (errorCode)
		reply.put("errorCode", errorCode);
	if (errorText)
		reply.put("errorText", errorText);
	return reply;
}

std::string createJsonReplyString(bool returnValue, int errorCode, const char *errorText)
{
	std::string	reply;
	if (returnValue)
		reply = STANDARD_JSON_SUCCESS;
	else if (errorCode)
	{
		if (errorText)
			reply = string_printf("{\"returnValue\":false,\"errorCode\":%d,\"errorText\":\"%s\"}", errorCode, errorText);
		else
			reply = string_printf("{\"returnValue\":false,\"errorCode\":%d}", errorCode);
	}
	else if (errorText)
		reply = string_printf("{\"returnValue\":false,\"errorText\":\"%s\"}", errorText);
	else
		reply = string_printf("{\"returnValue\":false}");
	return reply;
}

std::string	jsonToString(pbnjson::JValue & reply, const char * schema)
{
	pbnjson::JGenerator serializer(NULL);   // our schema that we will be using does not have any external references
	std::string serialized;
	pbnjson::JSchemaFragment responseSchema(schema);
	if (!serializer.toString(reply, responseSchema, serialized)) {
		g_critical("serializeJsonReply: failed to generate json reply");
		return "{\"returnValue\":false,\"errorText\":\"error: Failed to generate a valid json reply...\"}";
	}
	return serialized;
}

LSMessageJsonParser::LSMessageJsonParser(LSMessage * message, const char * schema)
    : mMessage(message)
    , mSchemaText(schema)
    , mSchema(schema)
{
}

std::string LSMessageJsonParser::getMsgCategoryMethod()
{
    std::string context = "";

    if (mMessage) {
        context = "Category: " + std::string(LSMessageGetCategory(mMessage)) + " ";
        context += "Method: " + std::string(LSMessageGetMethod(mMessage));
    }

    return context;
}

std::string LSMessageJsonParser::getSender()
{
    std::string strSender = "";

    if (mMessage) {
        const char * sender = LSMessageGetSenderServiceName(mMessage);
        if (sender || *sender)
            strSender = std::string(LSMessageGetSender(mMessage));
    }

    return strSender;
}

bool LSMessageJsonParser::parse(const char * callerFunction, LSHandle * lssender, ESchemaErrorOptions validationOption)
{
    if (EIgnore == validationOption) return true;

    const char * payload = getPayload();

    // Parse the message with given schema.
    if (!mParser.parse(payload, mSchema))
    {
        // Unable to parse the message with given schema

        const char *    errorText = "Could not validate json message against schema";
        bool            notJson = true; // we know that, it's not a valid json message

        // Try parsing the message with empty schema, just to verify that it is a valid json message
        if (strcmp(mSchemaText, SCHEMA_ANY) != 0)
        {
            pbnjson::JSchemaFragment    genericSchema(SCHEMA_ANY);
            notJson = !mParser.parse(payload, genericSchema);
        }

        if (notJson)
        {
            g_critical("[Schema Error] : [%s : %s]: The message '%s' sent by '%s' is not a valid json message against schema '%s'", callerFunction, getMsgCategoryMethod().c_str(), payload, getSender().c_str(), mSchemaText);
            errorText = "Not a valid json message";
        }
        else
        {
            g_critical("[Schema Error] : [%s :%s]: Could not validate json message '%s' sent by '%s' against schema '%s'.", callerFunction, getMsgCategoryMethod().c_str(), payload, getSender().c_str(), mSchemaText);
        }

        if (EValidateAndError == validationOption)
        {
            if ((lssender) && (!getSender().empty()))
            {
                std::string reply = createJsonReplyString(false, 1, errorText);
                CLSError lserror;
                if (!LSMessageReply(lssender, mMessage, reply.c_str(), &lserror))
                    lserror.Print(callerFunction, 0);
            }

            return false; // throw the error back
        }
    }

    // Message successfully parsed with given schema
    return true;
}

void CLSError::Print(const char * where, int line, GLogLevelFlags logLevel)
{
    if (LSErrorIsSet(this))
    {
        g_log(G_LOG_DOMAIN, logLevel, "%s(%d): Luna Service Error #%d \"%s\",\nin %s line #%d.", where, line, this->error_code, this->message, this->file, this->line);
        LSErrorFree(this);
    }
}

