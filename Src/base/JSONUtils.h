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

#ifndef JSONUTILS_H
#define JSONUTILS_H

#include "Settings.h"

#include <lunaservice.h>
#include <pbnjson.h>
#include <pbnjson.hpp>

/*
 * Helper macros to build schemas in a more reliable, readable & editable way in C++
 */

#define STR(x) #x

extern const char * STANDARD_JSON_SUCCESS;

/**
  * Json Online Schema Validator : http://jsonlint.com/
  * http://davidwalsh.name/json-validation
  */

// Build a standard reply as a const char * string consistently
#define STANDARD_JSON_SUCCESS 						"{\"returnValue\":true}"
#define STANDARD_JSON_ERROR(errorCode, errorText)	"{\"returnValue\":false,\"errorCode\":"STR(errorCode)",\"errorText\":\"" errorText "\"}"
#define MISSING_PARAMETER_ERROR(name, type)			"{\"returnValue\":false,\"errorCode\":2,\"errorText\":\"Missing '" STR(name) "' " STR(type) " parameter.\"}"
#define INVALID_PARAMETER_ERROR(name, type)			"{\"returnValue\":false,\"errorCode\":3,\"errorText\":\"Invalid '" STR(name) "' " STR(type) " parameter value.\"}"

// Test the name of a json parameter to determine if it's a system parameter that we should ignore. Pass a (char *)
#define IS_SYSTEM_PARAMETER(x)	((x) && *(x) == '$')
#define SYSTEM_PARAMETERS "\"$activity\":{\"type\":\"object\",\"optional\":true}"

// Build a schema as a const char * string without any execution overhead
#define SCHEMA_ANY                              "{}"
#define SCHEMA_0                                "{\"type\":\"object\",\"properties\":{" SYSTEM_PARAMETERS "},\"additionalProperties\":false}" // Rejects any parameter. Only valid message is "{}"
#define SCHEMA_1(param)                         "{\"type\":\"object\",\"properties\":{" param "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}" // Ex: SCHEMA_1(REQUIRED(age,integer))
#define SCHEMA_2(p1, p2)                        "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}" // Ex: SCHEMA_2(REQUIRED(age,integer),OPTIONAL(nickname,string)
#define SCHEMA_3(p1, p2, p3)                    "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_4(p1, p2, p3, p4)                "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_5(p1, p2, p3, p4, p5)            "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_6(p1, p2, p3, p4, p5, p6)        "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_7(p1, p2, p3, p4, p5, p6, p7)    "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_8(p1, p2, p3, p4, p5, p6, p7, p8)"{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "," p8 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"


// Macros to use in place of the parameters in the SCHEMA_xxx macros above
#define REQUIRED(name, type) "\"" #name "\":{\"type\":\"" #type "\"}"
#define OPTIONAL(name, type) "\"" #name "\":{\"type\":\"" #type "\",\"optional\":true}"

// Build a Union Schema
#define REQUIRED_UNION_2(name, type1, type2) "\"" #name "\":{\"type\":[{\"type\":\"" #type1 "\"},{\"type\":\"" #type2 "\"}]}"

// Build an Object Schema as a const char * string
#define NAKED_OBJECT_EMPTY(objName)                                          "\"" #objName "\":{\"type\":\"object\",\"properties\":{" SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT_EMPTY(objName)                                               "{\"" #objName "\":{\"type\":\"object\",\"properties\":{" SYSTEM_PARAMETERS "},\"additionalProperties\":false}}"

#define NAKED_OBJECT_REQUIRED_1(objName, p1, type1)                          "\"" #objName "\":{\"type\":\"object\",\"properties\":{" REQUIRED(p1, type1) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT_REQUIRED_1(objName, p1, type1)                               "{\"" #objName "\":{\"type\":\"object\",\"properties\":{" REQUIRED(p1, type1) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}}"

#define NAKED_OBJECT_OPTIONAL_1(objName, p1, type1)                          "\"" #objName "\":{\"type\":\"object\",\"properties\":{" OPTIONAL(p1, type1) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT_OPTIONAL_1(objName, p1, type1)                               "{\"" #objName "\":{\"type\":\"object\",\"properties\":{" OPTIONAL(p1, type1) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}}"

#define NAKED_OBJECT_REQUIRED_2(objName, p1, type1, p2, type2)               "\"" #objName "\":{\"type\":\"object\",\"properties\":{" REQUIRED(p1, type1) "," REQUIRED(p2, type2) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT_REQUIRED_2(objName, p1, type1, p2, type2)                    "{\"" #objName "\":{\"type\":\"object\",\"properties\":{" REQUIRED(p1, type1) "," REQUIRED(p2, type2) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}}"

#define NAKED_OBJECT_OPTIONAL_2(objName, p1, type1, p2, type2)               "\"" #objName "\":{\"type\":\"object\",\"properties\":{" OPTIONAL(p1, type1) "," OPTIONAL(p2, type2) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT_OPTIONAL_2(objName, p1, type1, p2, type2)                    "{\"" #objName "\":{\"type\":\"object\",\"properties\":{" OPTIONAL(p1, type1) "," OPTIONAL(p2, type2) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}}"

#define NAKED_OBJECT_REQUIRED_3(objName, p1, type1, p2, type2, p3, type3)    "\"" #objName "\":{\"type\":\"object\",\"properties\":{" REQUIRED(p1, type1) "," REQUIRED(p2, type2) "," REQUIRED(p3, type3) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT_REQUIRED_3(objName, p1, type1, p2, type2, p3, type3)         "{\"" #objName "\":{\"type\":\"object\",\"properties\":{" REQUIRED(p1, type1) "," REQUIRED(p2, type2) "," REQUIRED(p3, type3) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}}"

#define NAKED_OBJECT_OPTIONAL_3(objName, p1, type1, p2, type2, p3, type3)    "\"" #objName "\":{\"type\":\"object\",\"properties\":{" OPTIONAL(p1, type1) "," OPTIONAL(p2, type2) "," OPTIONAL(p3, type3) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT_OPTIONAL_3(objName, p1, type1, p2, type2, p3, type3)         "{\"" #objName "\":{\"type\":\"object\",\"properties\":{" OPTIONAL(p1, type1) "," OPTIONAL(p2, type2) "," OPTIONAL(p3, type3) "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}}"

// Deprecated Macro comments
#define DEPRECATED_SERVICE_MSG() g_critical("THIS METHOD IS DEPRECATED. PLEASE REVISIT THE CODE.")

class JsonValue
{
public:
	JsonValue(const pbnjson::JValue & value) : mValue(value) {}

	pbnjson::JValue &		get()										{ return mValue; }

	bool					get(const char * name, std::string & str)	{ return mValue[name].asString(str) == CONV_OK; }
	bool					get(const char * name, bool & boolean)		{ return mValue[name].asBool(boolean) == CONV_OK; }
	template <class T> bool	get(const char * name, T & number)			{ return mValue[name].asNumber<T>(number) == CONV_OK; }
	pbnjson::JValue			get(const char * name)						{ return mValue[name]; }

private:
	pbnjson::JValue	mValue;
};

/*
 * Helper class to parse a json message using a schema (if specified)
 */
class JsonMessageParser
{
public:
	JsonMessageParser(const char * json, const char * schema) : mJson(json), mSchema(schema) {}

	bool					parse(const char * callerFunction);
	pbnjson::JValue			get()										{ return mParser.getDom(); }

	// convenience functions to get a parameter directly.
	bool					get(const char * name, std::string & str)	{ return get()[name].asString(str) == CONV_OK; }
	bool					get(const char * name, bool & boolean)		{ return get()[name].asBool(boolean) == CONV_OK; }
	template <class T> bool	get(const char * name, T & number)			{ return get()[name].asNumber<T>(number) == CONV_OK; }
	pbnjson::JValue			get(const char * name)						{ return get()[name]; }

private:
	const char *				mJson;
	pbnjson::JSchemaFragment	mSchema;
	pbnjson::JDomParser			mParser;
};


/**
  * Schema Error Options
  */
enum ESchemaErrorOptions
{
    EIgnore = 0,            /**< Ignore the schema */
    EValidateAndContinue,   /**< Validate, Log the error & Continue */
    EValidateAndError,      /**< Validate, Log the error & Reply with correct schema */
    EDefault                /**< Default, loads the value from settings (luna.conf) file  */
};

/*
 * Small wrapper around LSError. User is responsible for calling Print or Free after the error has been set.
 */
struct CLSError : public LSError
{
    CLSError()
    {
        LSErrorInit(this);
    }
    void Print(const char * where, int line, GLogLevelFlags logLevel = G_LOG_LEVEL_WARNING);
    void Free()
    {
        LSErrorFree(this);
    }
};

/*
 * Helper class to parse json messages coming from an LS service using pbnjson
 */
class LSMessageJsonParser
{
public:
    // Default using any specific schema. Will simply validate that the message is a valid json message.
    LSMessageJsonParser(LSMessage * message, const char * schema);

    /*!
      * \brief Parse the message using the schema passed in constructor.
      * \param callerFunction   -Name of the function
      * \param sender           - If 'sender' is specified, automatically reply in case of bad syntax using standard format.
      * \param errOption        - Schema error option
      * \return true if parsed successfully, false otherwise
      */
    bool                    parse(const char * callerFunction, LSHandle * sender = 0, ESchemaErrorOptions errOption = EIgnore);

    /*! \fn getMsgCategoryMethod
      * \brief function parses the message and creates a string with category & method appended to it
      * \return string with category and method appended
      */
    std::string getMsgCategoryMethod();

    /*! \fn getSender
      * \brief function retrieves the sender name from the message
      * \return sender name if available, empty string otherwise
      */
    std::string getSender();

    pbnjson::JValue         get() { return mParser.getDom(); }
    const char *            getPayload()    { return LSMessageGetPayload(mMessage); }

    // convenience functions to get a parameter directly.
    bool                    get(const char * name, std::string & str)   { return get()[name].asString(str) == CONV_OK; }
    bool                    get(const char * name, bool & boolean)      { return get()[name].asBool(boolean) == CONV_OK; }
    template <class T> bool get(const char * name, T & number)          { return get()[name].asNumber<T>(number) == CONV_OK; }

private:
    LSMessage *                 mMessage;
    const char *                mSchemaText;
    pbnjson::JSchemaFragment    mSchema;
    pbnjson::JDomParser         mParser;
};

/**
  * Commonly used schema Macros
  */

/**
  * Main Validation Code
  */
#define VALIDATE_SCHEMA_AND_RETURN_OPTION(lsHandle, message, schema, schErrOption) {\
                                                                                        LSMessageJsonParser jsonParser(message, schema);                                                        \
                                                                                                                                                                                                \
                                                                                        if (EDefault == schErrOption)                                                                           \
                                                                                            schErrOption = static_cast<ESchemaErrorOptions>(Settings::LunaSettings()->schemaValidationOption);  \
                                                                                                                                                                                                \
                                                                                        if (!jsonParser.parse(__FUNCTION__, lsHandle, schErrOption))                                            \
                                                                                            return true;                                                                                        \
                                                                                    }

#define VALIDATE_SCHEMA_AND_RETURN(lsHandle, message, schema)   {\
                                                                    ESchemaErrorOptions schErrOption = EDefault;                                \
                                                                    VALIDATE_SCHEMA_AND_RETURN_OPTION(lsHandle, message, schema, schErrOption); \
                                                                 }

/**
  * Subscribe Schema : {"subscribe":boolean}
  */
#define SUBSCRIBE_SCHEMA_RETURN(lsHandle, message)   VALIDATE_SCHEMA_AND_RETURN(lsHandle, message, SCHEMA_1(OPTIONAL(subscribe, boolean)))

/**
  * Empty/Any Schema : {}
  */
#define EMPTY_SCHEMA_RETURN(lsHandle, message)    VALIDATE_SCHEMA_AND_RETURN(lsHandle, message, SCHEMA_ANY)

// build a standard reply returnValue & errorCode/errorText if defined
pbnjson::JValue createJsonReply(bool returnValue = true, int errorCode = 0, const char * errorText = 0);

// build a standard json reply string without the overhead of using json schema
std::string createJsonReplyString(bool returnValue = true, int errorCode = 0, const char * errorText = 0);

// serialize a reply
std::string jsonToString(pbnjson::JValue & reply, const char * schema = SCHEMA_ANY);

#endif // JSONUTILS_H
